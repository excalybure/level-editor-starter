#pragma once

#include "Command.h"
#include "CommandContext.h"
#include "CommandProfiler.h"
#include <memory>
#include <vector>
#include <deque>

/**
 * @brief Manages command history with configurable size and memory limits
 * 
 * Provides undo/redo functionality with automatic cleanup based on:
 * - Maximum number of commands (default: 100)
 * - Maximum memory usage (default: 100MB)
 */
class CommandHistory
{
public:
	using CommandPtr = std::unique_ptr<Command>;

	// Default configuration
	static constexpr size_t kDefaultMaxCommands = 100;
	static constexpr size_t kDefaultMaxMemoryUsage = 100 * 1024 * 1024; // 100MB

	/**
     * @brief Construct CommandHistory with default limits
     */
	CommandHistory()
		: CommandHistory( kDefaultMaxCommands, kDefaultMaxMemoryUsage ) {}

	/**
     * @brief Construct CommandHistory with custom limits
     * @param maxCommands Maximum number of commands to store
     * @param maxMemoryUsage Maximum memory usage in bytes
     */
	CommandHistory( size_t maxCommands, size_t maxMemoryUsage )
		: m_maxCommands( maxCommands ), m_maxMemoryUsage( maxMemoryUsage ), m_currentIndex( 0 ), m_currentMemoryUsage( 0 ) {}

	/**
     * @brief Get the number of commands currently in history
     * @return Number of commands stored
     */
	size_t getCommandCount() const
	{
		return m_commands.size();
	}

	/**
     * @brief Get the maximum number of commands that can be stored
     * @return Maximum command count limit
     */
	size_t getMaxCommands() const
	{
		return m_maxCommands;
	}

	/**
     * @brief Get the maximum memory usage limit
     * @return Maximum memory usage in bytes
     */
	size_t getMaxMemoryUsage() const
	{
		return m_maxMemoryUsage;
	}

	/**
     * @brief Get the current memory usage of all commands
     * @return Current memory usage in bytes
     */
	size_t getCurrentMemoryUsage() const
	{
		return m_currentMemoryUsage;
	}

	/**
     * @brief Check if the history is empty
     * @return true if no commands are stored
     */
	bool isEmpty() const
	{
		return m_commands.empty();
	}

	/**
     * @brief Check if undo operation is possible
     * @return true if there's a command to undo
     */
	bool canUndo() const
	{
		return m_currentIndex > 0 && !m_commands.empty();
	}

	/**
     * @brief Check if redo operation is possible
     * @return true if there's a command to redo
     */
	bool canRedo() const
	{
		return m_currentIndex < m_commands.size();
	}

	/**
     * @brief Execute a command and add it to history
     * @param command The command to execute
     * @return true if execution was successful
     */
	bool executeCommand( CommandPtr command )
	{
		PROFILE_COMMAND_OPERATION( "CommandHistory::executeCommand" );

		if ( !command )
			return false;

		// Try to execute the command first
		{
			PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::execute", command->getMemoryUsage() );
			if ( !command->execute() )
				return false;
		}

		// Create context for the command
		const auto timestamp = std::chrono::steady_clock::now();
		const size_t memUsage = command->getMemoryUsage();
		CommandContext context( timestamp, memUsage );

		// Clear any redo history when executing new command
		if ( m_currentIndex < m_commands.size() )
		{
			PROFILE_COMMAND_OPERATION( "CommandHistory::clearRedoHistory" );
			// Remove commands from current index to end
			const auto it = m_commands.begin() + m_currentIndex;
			for ( auto iter = it; iter != m_commands.end(); ++iter )
			{
				m_currentMemoryUsage -= iter->context.getMemoryUsage();
			}
			m_commands.erase( it, m_commands.end() );
		}

		// Add command to history
		m_commands.emplace_back( std::move( command ), context );
		m_currentMemoryUsage += memUsage;
		m_currentIndex = m_commands.size();

		// Clean up old commands if we exceed limits
		{
			PROFILE_COMMAND_OPERATION( "CommandHistory::cleanup" );
			cleanupOldCommands();
		}

		return true;
	}

	/**
     * @brief Execute a command with merging support
     * @param command The command to execute (may be merged with previous command)
     * @return true if execution was successful
     */
	bool executeCommandWithMerging( CommandPtr command )
	{
		PROFILE_COMMAND_OPERATION( "CommandHistory::executeCommandWithMerging" );

		if ( !command )
			return false;

		// Try to merge with the last command if possible
		if ( !m_commands.empty() && m_currentIndex == m_commands.size() )
		{
			auto &lastEntry = m_commands.back();
			const auto now = std::chrono::steady_clock::now();
			const auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>( now - lastEntry.context.getTimestamp() );

			// Check if commands can be merged and are within time window
			const auto mergeTimeWindow = std::chrono::milliseconds( 100 ); // 100ms merge window
			if ( timeDiff <= mergeTimeWindow && lastEntry.command->canMergeWith( command.get() ) )
			{
				// Try to execute the new command first
				{
					PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::execute", command->getMemoryUsage() );
					if ( !command->execute() )
						return false;
				}

				// Attempt merge
				{
					PROFILE_COMMAND_OPERATION( "Command::mergeWith" );
					if ( lastEntry.command->mergeWith( std::move( command ) ) )
					{
						// Update the context with new timestamp and memory usage
						lastEntry.context.updateTimestamp( now );
						lastEntry.context.updateMemoryUsage( lastEntry.command->getMemoryUsage() );
						return true;
					}
					else
					{
						// Merge failed, need to undo the command we just executed
						command->undo();
						return false;
					}
				}
			}
		}

		// Cannot merge, execute as a separate command
		return executeCommand( std::move( command ) );
	}

	/**
     * @brief Undo the last executed command
     * @return true if undo was successful
     */
	bool undo()
	{
		PROFILE_COMMAND_OPERATION( "CommandHistory::undo" );

		if ( !canUndo() )
			return false;

		--m_currentIndex;

		{
			PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::undo",
				m_commands[m_currentIndex].context.getMemoryUsage() );
			return m_commands[m_currentIndex].command->undo();
		}
	}

	/**
     * @brief Redo the next undone command
     * @return true if redo was successful
     */
	bool redo()
	{
		PROFILE_COMMAND_OPERATION( "CommandHistory::redo" );

		if ( !canRedo() )
			return false;

		bool success;
		{
			PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::redo",
				m_commands[m_currentIndex].context.getMemoryUsage() );
			success = m_commands[m_currentIndex].command->execute();
		}

		if ( success )
		{
			++m_currentIndex;
		}
		return success;
	}

	/**
     * @brief Get profiling data for command operations
     * @return Reference to the command profiler
     */
	const CommandProfiler &getProfiler() const
	{
		return g_commandProfiler;
	}

	/**
     * @brief Reset profiling data
     */
	void resetProfiling()
	{
		g_commandProfiler.reset();
	}

	/**
     * @brief Get commands that exceed performance thresholds
     * @param threshold Maximum acceptable duration (default: 1ms)
     * @return List of slow operation names
     */
	std::vector<std::string> getSlowOperations( std::chrono::microseconds threshold = std::chrono::milliseconds( 1 ) ) const
	{
		return g_commandProfiler.getSlowOperations( threshold );
	}

	/**
     * @brief Get more accurate memory usage calculation
     * @return Actual memory usage including overhead
     */
	size_t getActualMemoryUsage() const
	{
		return calculateActualMemoryUsage();
	}

private:
	/**
	 * @brief Intelligently remove oldest commands to stay within limits
	 */
	void cleanupOldCommands()
	{
		// Early exit if no cleanup needed
		if ( m_commands.size() <= m_maxCommands && m_currentMemoryUsage <= m_maxMemoryUsage )
			return;

		// Calculate how much we need to clean up
		const size_t commandsToRemove = m_commands.size() > m_maxCommands ? m_commands.size() - m_maxCommands : 0;
		const size_t memoryToFree = m_currentMemoryUsage > m_maxMemoryUsage ? m_currentMemoryUsage - m_maxMemoryUsage : 0;

		// Priority cleanup: remove commands that are large in memory first
		std::vector<size_t> candidateIndices;
		size_t removedCommands = 0;
		size_t freedMemory = 0;

		// Simple approach: remove from front (oldest commands)
		// This maintains the invariant and is cache-friendly
		while ( !m_commands.empty() &&
			( removedCommands < commandsToRemove || freedMemory < memoryToFree ) )
		{
			const auto &oldestEntry = m_commands.front();
			const size_t commandMemory = oldestEntry.context.getMemoryUsage();

			m_currentMemoryUsage -= commandMemory;
			freedMemory += commandMemory;
			++removedCommands;

			m_commands.pop_front();

			// Update current index to maintain correct position
			if ( m_currentIndex > 0 )
				--m_currentIndex;
		}
	}

	/**
	 * @brief Compress command data to reduce memory usage
	 */
	void compressCommands()
	{
		// For now, this is a placeholder for future compression implementation
		// Could implement delta compression for similar commands
		// or compress large data payloads in commands
	}

	/**
	 * @brief Estimate memory usage with better accuracy
	 */
	size_t calculateActualMemoryUsage() const
	{
		size_t total = sizeof( *this );
		for ( const auto &entry : m_commands )
		{
			total += sizeof( entry );
			total += entry.command->getMemoryUsage();
		}
		return total;
	}

	struct CommandEntry
	{
		CommandPtr command;
		CommandContext context;

		CommandEntry( CommandPtr cmd, const CommandContext &ctx )
			: command( std::move( cmd ) ), context( ctx ) {}
	};

	std::deque<CommandEntry> m_commands;
	size_t m_maxCommands;
	size_t m_maxMemoryUsage;
	size_t m_currentIndex;
	size_t m_currentMemoryUsage;
};
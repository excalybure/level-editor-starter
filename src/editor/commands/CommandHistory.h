#pragma once

#include "Command.h"
#include "CommandContext.h"
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
		if ( !command )
			return false;

		// Try to execute the command first
		if ( !command->execute() )
			return false;

		// Create context for the command
		const auto timestamp = std::chrono::steady_clock::now();
		const size_t memUsage = command->getMemoryUsage();
		CommandContext context( timestamp, memUsage );

		// Clear any redo history when executing new command
		if ( m_currentIndex < m_commands.size() )
		{
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

		// TODO: Implement cleanup for size/memory limits
		return true;
	}

	/**
     * @brief Undo the last executed command
     * @return true if undo was successful
     */
	bool undo()
	{
		if ( !canUndo() )
			return false;

		--m_currentIndex;
		return m_commands[m_currentIndex].command->undo();
	}

	/**
     * @brief Redo the next undone command
     * @return true if redo was successful
     */
	bool redo()
	{
		if ( !canRedo() )
			return false;

		bool success = m_commands[m_currentIndex].command->execute();
		if ( success )
		{
			++m_currentIndex;
		}
		return success;
	}

private:
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
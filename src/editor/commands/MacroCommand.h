#pragma once

#include "Command.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @brief Command that batches multiple commands for atomic execution
 * 
 * MacroCommand allows grouping multiple commands to be executed as a single
 * operation. All commands execute in order, and undo reverses them in reverse
 * order. If any command fails during execution, the macro stops and reports failure.
 */
class MacroCommand : public Command
{
public:
	using CommandPtr = std::unique_ptr<Command>;

	/**
     * @brief Construct MacroCommand with a description
     * @param description User-friendly description for this macro
     */
	explicit MacroCommand( const std::string &description )
		: m_description( description ), m_executed( false ) {}

	/**
     * @brief Construct MacroCommand by moving from another instance
     * @param other MacroCommand to move from
     */
	MacroCommand( MacroCommand &&other ) noexcept
		: m_description( std::move( other.m_description ) ), m_commands( std::move( other.m_commands ) ), m_executed( other.m_executed )
	{
		other.m_executed = false;
	}

	/**
     * @brief Move assignment operator
     * @param other MacroCommand to move from
     * @return Reference to this object
     */
	MacroCommand &operator=( MacroCommand &&other ) noexcept
	{
		if ( this != &other )
		{
			m_description = std::move( other.m_description );
			m_commands = std::move( other.m_commands );
			m_executed = other.m_executed;
			other.m_executed = false;
		}
		return *this;
	}

	// Delete copy operations to prevent issues with unique_ptr
	MacroCommand( const MacroCommand & ) = delete;
	MacroCommand &operator=( const MacroCommand & ) = delete;

	virtual ~MacroCommand() = default;

	/**
     * @brief Add a command to this macro
     * @param command Command to add (will be consumed)
     */
	void addCommand( CommandPtr command )
	{
		if ( command )
		{
			m_commands.push_back( std::move( command ) );
		}
	}

	/**
     * @brief Check if this macro contains no commands
     * @return true if no commands have been added
     */
	bool isEmpty() const
	{
		return m_commands.empty();
	}

	/**
     * @brief Get the number of commands in this macro
     * @return Number of sub-commands
     */
	size_t getCommandCount() const
	{
		return m_commands.size();
	}

	// Command interface implementation
	bool execute() override
	{
		if ( m_executed )
			return true; // Already executed

		// Execute all commands in order
		for ( size_t i = 0; i < m_commands.size(); ++i )
		{
			if ( !m_commands[i]->execute() )
			{
				// Execution failed - do not undo successful commands
				// They remain executed, but the macro as a whole failed
				return false;
			}
		}

		m_executed = true;
		return true;
	}

	bool undo() override
	{
		if ( !m_executed )
			return false;

		// Undo all commands in reverse order
		for ( auto it = m_commands.rbegin(); it != m_commands.rend(); ++it )
		{
			( *it )->undo();
		}

		m_executed = false;
		return true;
	}

	std::string getDescription() const override
	{
		return m_description;
	}

	size_t getMemoryUsage() const override
	{
		size_t total = sizeof( *this ) + m_description.size();
		for ( const auto &cmd : m_commands )
		{
			total += cmd->getMemoryUsage();
		}
		return total;
	}

	bool canMergeWith( const Command */* other */ ) const override
	{
		// MacroCommand doesn't support merging by default
		return false;
	}

	bool mergeWith( std::unique_ptr<Command> /* other */ ) override
	{
		// MacroCommand doesn't support merging by default
		return false;
	}

private:
	std::string m_description;
	std::vector<CommandPtr> m_commands;
	bool m_executed;
};
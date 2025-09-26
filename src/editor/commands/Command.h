#pragma once

#include <memory>
#include <string>

/**
 * @brief Abstract base class for all commands in the editor
 * 
 * Provides the core command pattern interface with execute/undo capabilities,
 * memory usage tracking, and command merging support for smooth operations.
 */
class Command
{
public:
	virtual ~Command() = default;

	/**
     * @brief Execute the command
     * @return true if execution was successful, false otherwise
     */
	virtual bool execute() = 0;

	/**
     * @brief Undo the command, reverting its effects
     * @return true if undo was successful, false otherwise
     */
	virtual bool undo() = 0;

	/**
     * @brief Get a user-friendly description of the command
     * @return Human-readable description for UI display
     */
	virtual std::string getDescription() const = 0;

	/**
     * @brief Get the memory usage of this command in bytes
     * @return Approximate memory usage for tracking and cleanup
     */
	virtual size_t getMemoryUsage() const = 0;

	/**
     * @brief Check if this command can be merged with another command
     * @param other The command to potentially merge with
     * @return true if merging is possible, false otherwise
     */
	virtual bool canMergeWith( const Command *other ) const = 0;

	/**
     * @brief Merge another command into this command
     * @param other The command to merge (will be consumed)
     * @return true if merge was successful, false otherwise
     */
	virtual bool mergeWith( std::unique_ptr<Command> other ) = 0;
};
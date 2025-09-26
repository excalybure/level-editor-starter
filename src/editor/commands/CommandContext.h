#pragma once

#include <chrono>

/**
 * @brief Context information for command execution
 * 
 * Tracks metadata about command execution including timestamp and memory usage.
 * Used by CommandHistory for tracking and cleanup operations.
 */
class CommandContext
{
public:
	using TimePoint = std::chrono::steady_clock::time_point;

	/**
     * @brief Construct CommandContext with timestamp and memory usage
     * @param timestamp When the command was executed
     * @param memoryUsage Memory usage in bytes
     */
	CommandContext( TimePoint timestamp, size_t memoryUsage )
		: m_timestamp( timestamp ), m_memoryUsage( memoryUsage ) {}

	/**
     * @brief Get the execution timestamp
     * @return Timestamp when command was executed
     */
	TimePoint getTimestamp() const { return m_timestamp; }

	/**
     * @brief Get the memory usage in bytes
     * @return Memory usage of the associated command
     */
	size_t getMemoryUsage() const { return m_memoryUsage; }

	/**
     * @brief Update the timestamp
     * @param timestamp New timestamp value
     */
	void updateTimestamp( TimePoint timestamp ) { m_timestamp = timestamp; }

	/**
     * @brief Update the memory usage
     * @param memoryUsage New memory usage value
     */
	void updateMemoryUsage( size_t memoryUsage ) { m_memoryUsage = memoryUsage; }

private:
	TimePoint m_timestamp;
	size_t m_memoryUsage;
};
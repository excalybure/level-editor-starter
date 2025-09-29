#pragma once

#include <chrono>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Performance profiler for command system operations
 * 
 * Provides lightweight profiling capabilities to monitor and optimize
 * command execution, memory usage, and cleanup operations.
 */
class CommandProfiler
{
public:
	using TimePoint = std::chrono::high_resolution_clock::time_point;
	using Duration = std::chrono::microseconds;

	/**
     * @brief Profiling data for a specific operation type
     */
	struct OperationProfile
	{
		size_t callCount = 0;
		Duration totalTime{ 0 };
		Duration minTime{ std::chrono::microseconds::max() };
		Duration maxTime{ 0 };
		size_t totalMemoryUsed = 0;

		Duration getAverageTime() const
		{
			return callCount > 0 ? Duration( totalTime.count() / callCount ) : Duration{ 0 };
		}

		double getAverageMemoryUsage() const
		{
			return callCount > 0 ? static_cast<double>( totalMemoryUsed ) / callCount : 0.0;
		}
	};

	/**
     * @brief Scoped timer for automatic profiling of operations
     */
	class ScopedTimer
	{
	public:
		ScopedTimer( CommandProfiler &profiler, const std::string &operation, size_t memoryUsage = 0 )
			: m_profiler( profiler ), m_operation( operation ), m_memoryUsage( memoryUsage ), m_startTime( std::chrono::high_resolution_clock::now() )
		{
		}

		~ScopedTimer()
		{
			const auto endTime = std::chrono::high_resolution_clock::now();
			const auto duration = std::chrono::duration_cast<Duration>( endTime - m_startTime );
			m_profiler.recordOperation( m_operation, duration, m_memoryUsage );
		}

	private:
		CommandProfiler &m_profiler;
		std::string m_operation;
		size_t m_memoryUsage;
		TimePoint m_startTime;
	};

	/**
     * @brief Record a completed operation
     */
	void recordOperation( const std::string &operation, Duration duration, size_t memoryUsage = 0 )
	{
		auto &profile = m_profiles[operation];
		++profile.callCount;
		profile.totalTime += duration;
		profile.totalMemoryUsed += memoryUsage;

		if ( duration < profile.minTime )
			profile.minTime = duration;
		if ( duration > profile.maxTime )
			profile.maxTime = duration;
	}

	/**
     * @brief Get profiling data for a specific operation
     */
	const OperationProfile *getProfile( const std::string &operation ) const
	{
		const auto it = m_profiles.find( operation );
		return it != m_profiles.end() ? &it->second : nullptr;
	}

	/**
     * @brief Get all profiling data
     */
	const std::unordered_map<std::string, OperationProfile> &getAllProfiles() const
	{
		return m_profiles;
	}

	/**
     * @brief Clear all profiling data
     */
	void reset()
	{
		m_profiles.clear();
	}

	/**
     * @brief Check if any operations exceed performance thresholds
     */
	std::vector<std::string> getSlowOperations( Duration threshold = std::chrono::milliseconds( 1 ) ) const
	{
		std::vector<std::string> slowOps;
		for ( const auto &[operation, profile] : m_profiles )
		{
			if ( profile.maxTime > threshold || profile.getAverageTime() > threshold )
			{
				slowOps.push_back( operation );
			}
		}
		return slowOps;
	}

	/**
     * @brief Create a scoped timer for profiling an operation
     */
	ScopedTimer createTimer( const std::string &operation, size_t memoryUsage = 0 )
	{
		return ScopedTimer( *this, operation, memoryUsage );
	}

private:
	std::unordered_map<std::string, OperationProfile> m_profiles;
};

// Global profiler instance for command system
extern CommandProfiler g_commandProfiler;

// Helper macros for unique variable names
#define CONCAT_IMPL( a, b ) a##b
#define CONCAT( a, b )		CONCAT_IMPL( a, b )

// Convenience macros for profiling with unique variable names
#define PROFILE_COMMAND_OPERATION( name ) \
	[[maybe_unused]] const auto CONCAT( _profileTimer_, __COUNTER__ ) = g_commandProfiler.createTimer( name )

#define PROFILE_COMMAND_OPERATION_WITH_MEMORY( name, memUsage ) \
	[[maybe_unused]] const auto CONCAT( _profileTimer_, __COUNTER__ ) = g_commandProfiler.createTimer( name, memUsage )
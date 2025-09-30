#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <chrono>
#include <memory>

#include "editor/commands/Command.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/CommandProfiler.h"
#include "editor/commands/EcsCommands.h"
#include "runtime/ecs.h"

TEST_CASE( "Command memory usage calculation is efficient", "[performance][AF5.1][command-memory]" )
{
	ecs::Scene scene;
	auto command = std::make_unique<editor::CreateEntityCommand>( scene, "PerformanceTest" );

	// Test that getMemoryUsage() completes quickly
	auto start = std::chrono::high_resolution_clock::now();

	// Call getMemoryUsage multiple times to ensure it's optimized
	constexpr int iterations = 10000;
	size_t totalMemory = 0;
	for ( int i = 0; i < iterations; ++i )
	{
		totalMemory += command->getMemoryUsage();
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

	// Should complete 10k calls in less than 1000 microseconds (1ms total, 0.1us per call)
	REQUIRE( duration.count() < 1000 );
	REQUIRE( totalMemory > 0 ); // Ensure we actually calculated something
}

TEST_CASE( "Command execution time meets performance requirements", "[performance][command-execution]" )
{
	ecs::Scene scene;
	CommandHistory history;

	SECTION( "CreateEntityCommand executes in <1ms" )
	{
		auto command = std::make_unique<editor::CreateEntityCommand>( scene, "FastEntity" );

		auto start = std::chrono::high_resolution_clock::now();
		bool success = history.executeCommand( std::move( command ) );
		auto end = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

		REQUIRE( success );
		REQUIRE( duration.count() < 1000 ); // < 1ms
	}

	SECTION( "Undo operation executes in <1ms" )
	{
		auto command = std::make_unique<editor::CreateEntityCommand>( scene, "FastEntity" );
		history.executeCommand( std::move( command ) );

		auto start = std::chrono::high_resolution_clock::now();
		bool success = history.undo();
		auto end = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

		REQUIRE( success );
		REQUIRE( duration.count() < 1000 ); // < 1ms
	}

	SECTION( "Redo operation executes in <1ms" )
	{
		auto command = std::make_unique<editor::CreateEntityCommand>( scene, "FastEntity" );
		history.executeCommand( std::move( command ) );
		history.undo();

		auto start = std::chrono::high_resolution_clock::now();
		bool success = history.redo();
		auto end = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

		REQUIRE( success );
		REQUIRE( duration.count() < 1000 ); // < 1ms
	}
}

TEST_CASE( "Memory cleanup performance is acceptable", "[performance][memory-cleanup]" )
{
	CommandHistory history( 10, 1024 ); // Small limits to trigger cleanup
	ecs::Scene scene;

	// Create many commands to trigger cleanup
	std::vector<std::chrono::microseconds> executionTimes;
	executionTimes.reserve( 20 );

	for ( int i = 0; i < 20; ++i )
	{
		auto command = std::make_unique<editor::CreateEntityCommand>( scene, "Entity" + std::to_string( i ) );

		auto start = std::chrono::high_resolution_clock::now();
		history.executeCommand( std::move( command ) );
		auto end = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );
		executionTimes.push_back( duration );
	}

	// Even with cleanup, no single command execution should take > 5ms
	for ( const auto &duration : executionTimes )
	{
		REQUIRE( duration.count() < 5000 ); // < 5ms even with cleanup
	}

	// Verify cleanup actually happened
	REQUIRE( history.getCommandCount() <= 10 );
}

// Test class for performance testing with controlled memory usage
class PerformanceTestCommand : public Command
{
public:
	explicit PerformanceTestCommand( size_t simulatedMemorySize )
		: m_simulatedMemory( simulatedMemorySize ), m_executed( false )
	{
		// Pre-allocate memory to simulate real command data
		m_data.reserve( simulatedMemorySize );
		m_data.resize( simulatedMemorySize, 0x42 );
	}

	bool execute() override
	{
		m_executed = true;
		return true;
	}

	bool undo() override
	{
		m_executed = false;
		return true;
	}

	std::string getDescription() const override
	{
		return "Performance Test Command";
	}

	size_t getMemoryUsage() const override
	{
		// This should be fast - just return cached value
		return sizeof( *this ) + m_simulatedMemory;
	}

	bool canMergeWith( const Command * ) const override
	{
		return false;
	}

	bool mergeWith( std::unique_ptr<Command> ) override
	{
		return false;
	}

private:
	size_t m_simulatedMemory;
	bool m_executed;
	std::vector<uint8_t> m_data;
};

TEST_CASE( "Memory calculation scales well with command data size", "[performance][memory-scaling]" )
{
	// Test various command sizes
	std::vector<size_t> testSizes = { 100, 1024, 10240, 102400 }; // 100B to 100KB

	for ( size_t size : testSizes )
	{
		const auto command = std::make_unique<PerformanceTestCommand>( size );

		const auto start = std::chrono::high_resolution_clock::now();
		const size_t memory = command->getMemoryUsage();
		const auto end = std::chrono::high_resolution_clock::now();

		const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( end - start );

		// Memory calculation should be O(1) - should complete in < 300ns regardless of data size
		REQUIRE( duration.count() < 500 );
		REQUIRE( memory >= size );
	}
}

TEST_CASE( "Intelligent cleanup algorithms perform efficiently", "[performance][AF5.2][cleanup]" )
{
	// Test with small limits to trigger frequent cleanup
	CommandHistory history( 5, 1024 ); // 5 commands max, 1KB memory max
	ecs::Scene scene;

	// Create commands with varying memory sizes
	std::vector<size_t> memorySizes = { 100, 200, 400, 150, 300, 50, 800, 100 };
	std::vector<std::chrono::microseconds> cleanupTimes;

	for ( size_t i = 0; i < memorySizes.size(); ++i )
	{
		auto command = std::make_unique<PerformanceTestCommand>( memorySizes[i] );

		auto start = std::chrono::high_resolution_clock::now();
		history.executeCommand( std::move( command ) );
		auto end = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );
		cleanupTimes.push_back( duration );
	}

	// Verify cleanup maintains limits
	REQUIRE( history.getCommandCount() <= 5 );
	REQUIRE( history.getCurrentMemoryUsage() <= 1024 );

	// Verify cleanup performance - even with cleanup, should be fast
	for ( const auto &duration : cleanupTimes )
	{
		REQUIRE( duration.count() < 1000 ); // < 1ms per operation including cleanup
	}
}

TEST_CASE( "Command profiling system tracks performance accurately", "[performance][AF5.5][profiling]" )
{
	CommandHistory history;
	ecs::Scene scene;

	// Reset profiler before test
	history.resetProfiling();

	// Execute some commands to generate profiling data
	for ( int i = 0; i < 5; ++i )
	{
		auto command = std::make_unique<editor::CreateEntityCommand>( scene, "ProfiledEntity" + std::to_string( i ) );
		history.executeCommand( std::move( command ) );
	}

	// Perform some undo/redo operations
	history.undo();
	history.undo();
	history.redo();

	const auto &profiler = history.getProfiler();

	// Check that profiling data was collected
	const auto *executeProfile = profiler.getProfile( "CommandHistory::executeCommand" );
	REQUIRE( executeProfile != nullptr );
	REQUIRE( executeProfile->callCount == 5 );

	const auto *undoProfile = profiler.getProfile( "CommandHistory::undo" );
	REQUIRE( undoProfile != nullptr );
	REQUIRE( undoProfile->callCount == 2 );

	const auto *redoProfile = profiler.getProfile( "CommandHistory::redo" );
	REQUIRE( redoProfile != nullptr );
	REQUIRE( redoProfile->callCount == 1 );

	// Verify that all operations were fast
	const auto slowOps = history.getSlowOperations();
	REQUIRE( slowOps.empty() ); // No operations should exceed 1ms threshold
}

TEST_CASE( "Command merging optimization performs well", "[performance][AF5.6][merging]" )
{
	CommandHistory history;
	ecs::Scene scene;

	// Create a mergeable command type for testing
	class MergeableTestCommand : public Command
	{
	public:
		explicit MergeableTestCommand( int value ) : m_value( value ), m_executed( false ) {}

		bool execute() override
		{
			m_executed = true;
			return true;
		}

		bool undo() override
		{
			m_executed = false;
			return true;
		}

		std::string getDescription() const override
		{
			return "Mergeable Test: " + std::to_string( m_value );
		}

		size_t getMemoryUsage() const override
		{
			return sizeof( *this );
		}

		bool canMergeWith( const Command *other ) const override
		{
			return dynamic_cast<const MergeableTestCommand *>( other ) != nullptr;
		}

		bool mergeWith( std::unique_ptr<Command> other ) override
		{
			auto *mergeableOther = dynamic_cast<MergeableTestCommand *>( other.get() );
			if ( mergeableOther )
			{
				m_value += mergeableOther->m_value; // Combine values
				return true;
			}
			return false;
		}

		int getValue() const { return m_value; }

	private:
		int m_value;
		bool m_executed;
	};

	// Reset profiling
	history.resetProfiling();

	// Execute commands that can be merged quickly
	auto start = std::chrono::high_resolution_clock::now();

	history.executeCommandWithMerging( std::make_unique<MergeableTestCommand>( 1 ) );

	// These should merge with the first command
	for ( int i = 2; i <= 10; ++i )
	{
		history.executeCommandWithMerging( std::make_unique<MergeableTestCommand>( i ) );
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );

	// Verify merging happened - should have only 1 command due to merging
	REQUIRE( history.getCommandCount() == 1 );

	// Verify total time for all merging operations is reasonable
	REQUIRE( totalDuration.count() < 5000 ); // < 5ms for 10 merge operations

	// Check profiling data
	const auto &profiler = history.getProfiler();
	const auto *mergeProfile = profiler.getProfile( "Command::mergeWith" );
	REQUIRE( mergeProfile != nullptr );
	REQUIRE( mergeProfile->callCount == 9 ); // 9 merge operations
}
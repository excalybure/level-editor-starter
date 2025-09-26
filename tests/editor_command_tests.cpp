#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <chrono>

// This should fail compilation until we create the Command interface
#include "editor/commands/Command.h"
#include "editor/commands/CommandContext.h"
#include "editor/commands/CommandHistory.h"

// Test Command implementation for testing the interface
class TestCommand : public Command
{
public:
	TestCommand( const std::string &desc, bool shouldSucceed = true )
		: m_description( desc ), m_shouldSucceed( shouldSucceed ), m_executed( false ) {}

	virtual ~TestCommand() = default;

	bool execute() override
	{
		if ( !m_shouldSucceed )
			return false;
		m_executed = true;
		return true;
	}

	bool undo() override
	{
		if ( !m_executed )
			return false;
		m_executed = false;
		return true;
	}

	std::string getDescription() const override
	{
		return m_description;
	}

	size_t getMemoryUsage() const override
	{
		return sizeof( *this ) + m_description.size();
	}

	bool canMergeWith( const Command *other ) const override
	{
		return false; // Simple test command doesn't support merging
	}

	bool mergeWith( std::unique_ptr<Command> other ) override
	{
		return false; // Simple test command doesn't support merging
	}

	bool isExecuted() const { return m_executed; }

private:
	std::string m_description;
	bool m_shouldSucceed;
	bool m_executed;
};

TEST_CASE( "Command interface basic contract", "[command][unit][AF1.1]" )
{
	SECTION( "Command can be constructed with description" )
	{
		TestCommand cmd( "Test operation" );
		REQUIRE( cmd.getDescription() == "Test operation" );
		REQUIRE_FALSE( cmd.isExecuted() );
	}

	SECTION( "Command execute changes state" )
	{
		TestCommand cmd( "Test operation" );
		REQUIRE( cmd.execute() );
		REQUIRE( cmd.isExecuted() );
	}

	SECTION( "Command undo reverses state" )
	{
		TestCommand cmd( "Test operation" );
		cmd.execute();
		REQUIRE( cmd.undo() );
		REQUIRE_FALSE( cmd.isExecuted() );
	}

	SECTION( "Command undo fails if not executed" )
	{
		TestCommand cmd( "Test operation" );
		REQUIRE_FALSE( cmd.undo() );
	}

	SECTION( "Command provides memory usage" )
	{
		TestCommand cmd( "Test operation" );
		size_t memUsage = cmd.getMemoryUsage();
		REQUIRE( memUsage > 0 );
		REQUIRE( memUsage >= sizeof( TestCommand ) );
	}

	SECTION( "Command merging interface" )
	{
		TestCommand cmd1( "Test 1" );
		TestCommand cmd2( "Test 2" );

		REQUIRE_FALSE( cmd1.canMergeWith( &cmd2 ) );
		REQUIRE_FALSE( cmd1.mergeWith( std::make_unique<TestCommand>( "Test 3" ) ) );
	}

	SECTION( "Command execution can fail" )
	{
		TestCommand cmd( "Failing command", false );
		REQUIRE_FALSE( cmd.execute() );
		REQUIRE_FALSE( cmd.isExecuted() );
	}
}

TEST_CASE( "CommandContext construction and metadata", "[command][unit][AF1.2]" )
{
	SECTION( "CommandContext can be constructed with timestamp" )
	{
		auto now = std::chrono::steady_clock::now();
		CommandContext context( now, 1024 );

		REQUIRE( context.getTimestamp() == now );
		REQUIRE( context.getMemoryUsage() == 1024 );
	}

	SECTION( "CommandContext provides execution metadata" )
	{
		auto timestamp = std::chrono::steady_clock::now();
		size_t memUsage = 2048;
		CommandContext context( timestamp, memUsage );

		REQUIRE( context.getTimestamp() == timestamp );
		REQUIRE( context.getMemoryUsage() == memUsage );
	}

	SECTION( "CommandContext can be updated" )
	{
		auto timestamp1 = std::chrono::steady_clock::now();
		CommandContext context( timestamp1, 1024 );

		auto timestamp2 = std::chrono::steady_clock::now();
		context.updateTimestamp( timestamp2 );
		context.updateMemoryUsage( 2048 );

		REQUIRE( context.getTimestamp() == timestamp2 );
		REQUIRE( context.getMemoryUsage() == 2048 );
	}
}

TEST_CASE( "CommandHistory construction with limits", "[command][unit][AF1.3]" )
{
	SECTION( "CommandHistory can be constructed with default limits" )
	{
		CommandHistory history;

		REQUIRE( history.getCommandCount() == 0 );
		REQUIRE( history.getMaxCommands() > 0 );
		REQUIRE( history.getMaxMemoryUsage() > 0 );
		REQUIRE( history.getCurrentMemoryUsage() == 0 );
	}

	SECTION( "CommandHistory can be constructed with custom limits" )
	{
		const size_t maxCommands = 50;
		const size_t maxMemory = 1024 * 1024; // 1MB
		CommandHistory history( maxCommands, maxMemory );

		REQUIRE( history.getCommandCount() == 0 );
		REQUIRE( history.getMaxCommands() == maxCommands );
		REQUIRE( history.getMaxMemoryUsage() == maxMemory );
		REQUIRE( history.getCurrentMemoryUsage() == 0 );
	}

	SECTION( "CommandHistory provides basic properties" )
	{
		CommandHistory history( 100, 2048 );

		REQUIRE( history.isEmpty() );
		REQUIRE_FALSE( history.canUndo() );
		REQUIRE_FALSE( history.canRedo() );
	}
}

TEST_CASE( "Command execution with automatic history management", "[command][unit][AF1.4]" )
{
	SECTION( "CommandHistory can execute commands" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd = std::make_unique<TestCommand>( "Test command" );

		REQUIRE( history.isEmpty() );
		REQUIRE( history.executeCommand( std::move( cmd ) ) );
		REQUIRE_FALSE( history.isEmpty() );
		REQUIRE( history.getCommandCount() == 1 );
		REQUIRE( history.canUndo() );
		REQUIRE_FALSE( history.canRedo() );
	}

	SECTION( "CommandHistory tracks memory usage during execution" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd = std::make_unique<TestCommand>( "Test command" );
		const size_t expectedMemory = cmd->getMemoryUsage();

		REQUIRE( history.getCurrentMemoryUsage() == 0 );
		REQUIRE( history.executeCommand( std::move( cmd ) ) );
		REQUIRE( history.getCurrentMemoryUsage() >= expectedMemory );
	}

	SECTION( "CommandHistory handles command execution failure" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd = std::make_unique<TestCommand>( "Failing command", false );

		REQUIRE_FALSE( history.executeCommand( std::move( cmd ) ) );
		REQUIRE( history.isEmpty() );
		REQUIRE( history.getCurrentMemoryUsage() == 0 );
	}

	SECTION( "CommandHistory clears redo stack on new command" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );
		auto cmd3 = std::make_unique<TestCommand>( "Command 3" );

		// Execute and undo to have redo stack
		history.executeCommand( std::move( cmd1 ) );
		history.executeCommand( std::move( cmd2 ) );
		history.undo(); // This would create redo stack

		REQUIRE( history.canRedo() ); // Assume undo/redo work for this test

		// New command should clear redo stack
		history.executeCommand( std::move( cmd3 ) );
		REQUIRE_FALSE( history.canRedo() );
	}
}

TEST_CASE( "Undo/redo operations with proper state management", "[command][unit][AF1.5]" )
{
	SECTION( "Undo reverts command execution" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd = std::make_unique<TestCommand>( "Test command" );
		const TestCommand *cmdPtr = cmd.get();

		history.executeCommand( std::move( cmd ) );
		REQUIRE( cmdPtr->isExecuted() );

		REQUIRE( history.undo() );
		REQUIRE_FALSE( cmdPtr->isExecuted() );
		REQUIRE( history.canRedo() );
		REQUIRE_FALSE( history.canUndo() );
	}

	SECTION( "Redo re-executes previously undone command" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd = std::make_unique<TestCommand>( "Test command" );
		const TestCommand *cmdPtr = cmd.get();

		history.executeCommand( std::move( cmd ) );
		history.undo();

		REQUIRE( history.redo() );
		REQUIRE( cmdPtr->isExecuted() );
		REQUIRE( history.canUndo() );
		REQUIRE_FALSE( history.canRedo() );
	}

	SECTION( "Multiple undo/redo operations work correctly" )
	{
		CommandHistory history( 10, 1024 );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );
		auto cmd3 = std::make_unique<TestCommand>( "Command 3" );

		const TestCommand *cmd1Ptr = cmd1.get();
		const TestCommand *cmd2Ptr = cmd2.get();
		const TestCommand *cmd3Ptr = cmd3.get();

		// Execute commands
		history.executeCommand( std::move( cmd1 ) );
		history.executeCommand( std::move( cmd2 ) );
		history.executeCommand( std::move( cmd3 ) );

		REQUIRE( cmd1Ptr->isExecuted() );
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE( cmd3Ptr->isExecuted() );

		// Undo twice
		REQUIRE( history.undo() ); // Undo cmd3
		REQUIRE_FALSE( cmd3Ptr->isExecuted() );
		REQUIRE( history.undo() ); // Undo cmd2
		REQUIRE_FALSE( cmd2Ptr->isExecuted() );
		REQUIRE( cmd1Ptr->isExecuted() ); // cmd1 still executed

		// Redo once
		REQUIRE( history.redo() ); // Redo cmd2
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE_FALSE( cmd3Ptr->isExecuted() ); // cmd3 still undone
	}

	SECTION( "Undo/redo bounds checking" )
	{
		CommandHistory history( 10, 1024 );

		// Cannot undo empty history
		REQUIRE_FALSE( history.canUndo() );
		REQUIRE_FALSE( history.undo() );

		// Cannot redo without undone commands
		REQUIRE_FALSE( history.canRedo() );
		REQUIRE_FALSE( history.redo() );

		// Add and test bounds
		auto cmd = std::make_unique<TestCommand>( "Test command" );
		history.executeCommand( std::move( cmd ) );

		REQUIRE( history.canUndo() );
		REQUIRE_FALSE( history.canRedo() );
		REQUIRE( history.undo() );

		REQUIRE_FALSE( history.canUndo() );
		REQUIRE( history.canRedo() );
		REQUIRE_FALSE( history.undo() ); // Cannot undo further
		REQUIRE( history.redo() );

		REQUIRE_FALSE( history.canRedo() );
		REQUIRE_FALSE( history.redo() ); // Cannot redo further
	}
}
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

// This should fail compilation until we create the Command interface
#include "editor/commands/Command.h"
#include "editor/commands/CommandContext.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/MacroCommand.h"

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

// Mergeable test command for testing command merging
class MergeableCommand : public Command
{
public:
	MergeableCommand( const std::string &type, int value )
		: m_type( type ), m_value( value ), m_executed( false ) {}

	virtual ~MergeableCommand() = default;

	bool execute() override
	{
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
		return m_type + " with value " + std::to_string( m_value );
	}

	size_t getMemoryUsage() const override
	{
		return sizeof( *this ) + m_type.size();
	}

	bool canMergeWith( const Command *other ) const override
	{
		const auto *mergeable = dynamic_cast<const MergeableCommand *>( other );
		return mergeable != nullptr && mergeable->m_type == m_type;
	}

	bool mergeWith( std::unique_ptr<Command> other ) override
	{
		auto mergeable = dynamic_cast<MergeableCommand *>( other.get() );
		if ( !mergeable || mergeable->m_type != m_type )
			return false;

		// Merge by adding values
		m_value += mergeable->m_value;
		return true;
	}

	int getValue() const { return m_value; }
	bool isExecuted() const { return m_executed; }
	const std::string &getType() const { return m_type; }

private:
	std::string m_type;
	int m_value;
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

TEST_CASE( "MacroCommand batching multiple operations", "[command][unit][AF1.6]" )
{
	SECTION( "MacroCommand can be constructed and executed" )
	{
		MacroCommand macro( "Batch operations" );

		REQUIRE( macro.getDescription() == "Batch operations" );
		REQUIRE( macro.isEmpty() );
		REQUIRE( macro.getCommandCount() == 0 );
	}

	SECTION( "MacroCommand can add and execute multiple commands" )
	{
		MacroCommand macro( "Multi-command batch" );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );
		auto cmd3 = std::make_unique<TestCommand>( "Command 3" );

		const TestCommand *cmd1Ptr = cmd1.get();
		const TestCommand *cmd2Ptr = cmd2.get();
		const TestCommand *cmd3Ptr = cmd3.get();

		macro.addCommand( std::move( cmd1 ) );
		macro.addCommand( std::move( cmd2 ) );
		macro.addCommand( std::move( cmd3 ) );

		REQUIRE( macro.getCommandCount() == 3 );
		REQUIRE_FALSE( macro.isEmpty() );

		// Execute all commands
		REQUIRE( macro.execute() );
		REQUIRE( cmd1Ptr->isExecuted() );
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE( cmd3Ptr->isExecuted() );
	}

	SECTION( "MacroCommand undo reverses commands in reverse order" )
	{
		MacroCommand macro( "Undo batch test" );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );

		const TestCommand *cmd1Ptr = cmd1.get();
		const TestCommand *cmd2Ptr = cmd2.get();

		macro.addCommand( std::move( cmd1 ) );
		macro.addCommand( std::move( cmd2 ) );
		macro.execute();

		// Both should be executed
		REQUIRE( cmd1Ptr->isExecuted() );
		REQUIRE( cmd2Ptr->isExecuted() );

		// Undo should reverse in opposite order
		REQUIRE( macro.undo() );
		REQUIRE_FALSE( cmd1Ptr->isExecuted() );
		REQUIRE_FALSE( cmd2Ptr->isExecuted() );
	}

	SECTION( "MacroCommand handles execution failure correctly" )
	{
		MacroCommand macro( "Failure test" );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto failCmd = std::make_unique<TestCommand>( "Failing command", false );
		auto cmd3 = std::make_unique<TestCommand>( "Command 3" );

		const TestCommand *cmd1Ptr = cmd1.get();
		const TestCommand *cmd3Ptr = cmd3.get();

		macro.addCommand( std::move( cmd1 ) );
		macro.addCommand( std::move( failCmd ) );
		macro.addCommand( std::move( cmd3 ) );

		// Should fail and not execute any remaining commands
		REQUIRE_FALSE( macro.execute() );
		REQUIRE( cmd1Ptr->isExecuted() );		// First command executed
		REQUIRE_FALSE( cmd3Ptr->isExecuted() ); // Third command not reached
	}

	SECTION( "MacroCommand calculates total memory usage" )
	{
		MacroCommand macro( "Memory test" );
		auto cmd1 = std::make_unique<TestCommand>( "Short" );
		auto cmd2 = std::make_unique<TestCommand>( "Much longer command name" );

		const size_t mem1 = cmd1->getMemoryUsage();
		const size_t mem2 = cmd2->getMemoryUsage();

		macro.addCommand( std::move( cmd1 ) );
		macro.addCommand( std::move( cmd2 ) );

		const size_t totalMemory = macro.getMemoryUsage();
		REQUIRE( totalMemory >= mem1 + mem2 );
	}

	SECTION( "MacroCommand works with CommandHistory" )
	{
		CommandHistory history( 10, 1024 );
		MacroCommand macro( "History integration test" );
		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );

		const TestCommand *cmd1Ptr = cmd1.get();
		const TestCommand *cmd2Ptr = cmd2.get();

		macro.addCommand( std::move( cmd1 ) );
		macro.addCommand( std::move( cmd2 ) );

		auto macroPtr = std::make_unique<MacroCommand>( std::move( macro ) );
		REQUIRE( history.executeCommand( std::move( macroPtr ) ) );

		REQUIRE( cmd1Ptr->isExecuted() );
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE( history.canUndo() );

		// Undo macro should undo all sub-commands
		REQUIRE( history.undo() );
		REQUIRE_FALSE( cmd1Ptr->isExecuted() );
		REQUIRE_FALSE( cmd2Ptr->isExecuted() );
	}
}

TEST_CASE( "Command merging for continuous operations", "[command][unit][AF1.7]" )
{
	SECTION( "CommandHistory supports command merging" )
	{
		CommandHistory history( 10, 1024 );

		// Add first mergeable command
		auto cmd1 = std::make_unique<MergeableCommand>( "transform", 5 );
		const MergeableCommand *cmd1Ptr = cmd1.get();
		REQUIRE( history.executeCommand( std::move( cmd1 ) ) );
		REQUIRE( history.getCommandCount() == 1 );
		REQUIRE( cmd1Ptr->getValue() == 5 );

		// Add second mergeable command that should merge with the first
		auto cmd2 = std::make_unique<MergeableCommand>( "transform", 3 );
		REQUIRE( history.executeCommandWithMerging( std::move( cmd2 ) ) );

		// Should still have only 1 command (merged)
		REQUIRE( history.getCommandCount() == 1 );
		REQUIRE( cmd1Ptr->getValue() == 8 ); // 5 + 3 merged
	}

	SECTION( "Commands that cannot merge are stored separately" )
	{
		CommandHistory history( 10, 1024 );

		// Add first command
		auto cmd1 = std::make_unique<MergeableCommand>( "transform", 5 );
		REQUIRE( history.executeCommand( std::move( cmd1 ) ) );
		REQUIRE( history.getCommandCount() == 1 );

		// Add incompatible command
		auto cmd2 = std::make_unique<MergeableCommand>( "rotation", 90 );
		REQUIRE( history.executeCommandWithMerging( std::move( cmd2 ) ) );

		// Should have 2 separate commands
		REQUIRE( history.getCommandCount() == 2 );
	}

	SECTION( "Command merging respects time window" )
	{
		CommandHistory history( 10, 1024 );

		// Add first command
		auto cmd1 = std::make_unique<MergeableCommand>( "transform", 5 );
		REQUIRE( history.executeCommand( std::move( cmd1 ) ) );

		// Simulate time passing beyond merge window
		std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) ); // Default merge window is 100ms

		// Add second command - should not merge due to time
		auto cmd2 = std::make_unique<MergeableCommand>( "transform", 3 );
		REQUIRE( history.executeCommandWithMerging( std::move( cmd2 ) ) );

		// Should have 2 separate commands due to time window
		REQUIRE( history.getCommandCount() == 2 );
	}

	SECTION( "Merged commands undo as a single unit" )
	{
		CommandHistory history( 10, 1024 );

		auto cmd1 = std::make_unique<MergeableCommand>( "transform", 5 );
		const MergeableCommand *cmd1Ptr = cmd1.get();
		history.executeCommand( std::move( cmd1 ) );

		auto cmd2 = std::make_unique<MergeableCommand>( "transform", 3 );
		history.executeCommandWithMerging( std::move( cmd2 ) );

		REQUIRE( cmd1Ptr->isExecuted() );
		REQUIRE( cmd1Ptr->getValue() == 8 );

		// Undo should revert the entire merged command
		REQUIRE( history.undo() );
		REQUIRE_FALSE( cmd1Ptr->isExecuted() );
	}

	SECTION( "Basic canMergeWith and mergeWith interface works" )
	{
		MergeableCommand cmd1( "transform", 10 );
		MergeableCommand cmd2( "transform", 5 );
		MergeableCommand cmd3( "rotation", 90 );

		// Compatible commands can merge
		REQUIRE( cmd1.canMergeWith( &cmd2 ) );
		REQUIRE( cmd1.mergeWith( std::make_unique<MergeableCommand>( "transform", 5 ) ) );
		REQUIRE( cmd1.getValue() == 15 ); // 10 + 5

		// Incompatible commands cannot merge
		REQUIRE_FALSE( cmd1.canMergeWith( &cmd3 ) );
		REQUIRE_FALSE( cmd3.mergeWith( std::make_unique<MergeableCommand>( "transform", 5 ) ) );
	}
}

TEST_CASE( "Memory tracking and automatic cleanup", "[command][unit][AF1.8]" )
{
	SECTION( "CommandHistory enforces memory limits" )
	{
		// Create history with very small memory limit
		const size_t maxMemory = 200; // Very small limit to trigger cleanup
		CommandHistory history( 10, maxMemory );

		// Add commands that will exceed memory limit
		auto cmd1 = std::make_unique<TestCommand>( "First command with long description to use more memory" );
		auto cmd2 = std::make_unique<TestCommand>( "Second command with even longer description to definitely exceed memory limit" );

		REQUIRE( history.executeCommand( std::move( cmd1 ) ) );
		REQUIRE( history.getCommandCount() == 1 );

		// This should trigger cleanup
		REQUIRE( history.executeCommand( std::move( cmd2 ) ) );

		// After cleanup, memory should be within limits
		REQUIRE( history.getCurrentMemoryUsage() <= maxMemory );
	}

	SECTION( "CommandHistory enforces command count limits" )
	{
		// Create history with very small command limit
		const size_t maxCommands = 2;
		CommandHistory history( maxCommands, 10240 );

		// Add more commands than the limit
		REQUIRE( history.executeCommand( std::make_unique<TestCommand>( "Command 1" ) ) );
		REQUIRE( history.executeCommand( std::make_unique<TestCommand>( "Command 2" ) ) );
		REQUIRE( history.getCommandCount() == 2 );

		// This should trigger cleanup
		REQUIRE( history.executeCommand( std::make_unique<TestCommand>( "Command 3" ) ) );

		// After cleanup, should not exceed limit
		REQUIRE( history.getCommandCount() <= maxCommands );
	}

	SECTION( "Cleanup preserves most recent commands" )
	{
		CommandHistory history( 3, 10240 ); // Limit to 3 commands

		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );
		auto cmd3 = std::make_unique<TestCommand>( "Command 3" );
		auto cmd4 = std::make_unique<TestCommand>( "Command 4" );

		const TestCommand *cmd2Ptr = cmd2.get();
		const TestCommand *cmd3Ptr = cmd3.get();
		const TestCommand *cmd4Ptr = cmd4.get();

		history.executeCommand( std::move( cmd1 ) );
		history.executeCommand( std::move( cmd2 ) );
		history.executeCommand( std::move( cmd3 ) );

		// All commands executed so far
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE( cmd3Ptr->isExecuted() );

		// Add fourth command - should trigger cleanup of oldest
		history.executeCommand( std::move( cmd4 ) );

		// Most recent commands should still be executed
		REQUIRE( cmd2Ptr->isExecuted() );
		REQUIRE( cmd3Ptr->isExecuted() );
		REQUIRE( cmd4Ptr->isExecuted() );

		// Should have 3 or fewer commands
		REQUIRE( history.getCommandCount() <= 3 );
	}

	SECTION( "Cleanup updates memory tracking correctly" )
	{
		CommandHistory history( 2, 10240 ); // Limit to 2 commands

		auto cmd1 = std::make_unique<TestCommand>( "Short" );
		auto cmd2 = std::make_unique<TestCommand>( "Medium description" );
		auto cmd3 = std::make_unique<TestCommand>( "Very long command description for memory tracking test" );

		const size_t mem1 = cmd1->getMemoryUsage();
		const size_t mem2 = cmd2->getMemoryUsage();
		const size_t mem3 = cmd3->getMemoryUsage();

		history.executeCommand( std::move( cmd1 ) );
		history.executeCommand( std::move( cmd2 ) );

		const size_t memoryBefore = history.getCurrentMemoryUsage();
		REQUIRE( memoryBefore >= mem1 + mem2 );

		// Add third command - should cleanup first
		history.executeCommand( std::move( cmd3 ) );

		const size_t memoryAfter = history.getCurrentMemoryUsage();
		// Should have approximately memory of cmd2 + cmd3 (first removed)
		REQUIRE( memoryAfter >= mem2 + mem3 );
		REQUIRE( memoryAfter < memoryBefore + mem3 ); // Should be less than if we kept all commands
	}

	SECTION( "No cleanup needed when within limits" )
	{
		CommandHistory history( 10, 10240 ); // Large limits

		auto cmd1 = std::make_unique<TestCommand>( "Command 1" );
		auto cmd2 = std::make_unique<TestCommand>( "Command 2" );

		history.executeCommand( std::move( cmd1 ) );
		history.executeCommand( std::move( cmd2 ) );

		// Both commands should be preserved
		REQUIRE( history.getCommandCount() == 2 );
		REQUIRE( history.getCurrentMemoryUsage() > 0 );
	}
}
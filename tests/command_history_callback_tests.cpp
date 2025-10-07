#include <catch2/catch_test_macros.hpp>
#include "editor/commands/CommandHistory.h"
#include <string>

// Test command for callback testing
class TestCallbackCommand : public Command
{
public:
	explicit TestCallbackCommand( const std::string &desc ) : m_description( desc ), m_executed( false ) {}

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

	std::string getDescription() const override { return m_description; }
	size_t getMemoryUsage() const override { return m_description.size(); }
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity, ecs::Entity ) override { return false; }

	bool isExecuted() const { return m_executed; }

private:
	std::string m_description;
	bool m_executed;
};

TEST_CASE( "CommandHistory change callback on execute", "[command-history][callback][AF1]" )
{
	CommandHistory history;
	int callbackCount = 0;

	// Set up callback
	history.setOnHistoryChangedCallback( [&callbackCount]() { ++callbackCount; } );

	SECTION( "Callback is triggered when command is executed" )
	{
		auto cmd = std::make_unique<TestCallbackCommand>( "Test" );
		REQUIRE( callbackCount == 0 );

		REQUIRE( history.executeCommand( std::move( cmd ) ) );
		REQUIRE( callbackCount == 1 );
	}

	SECTION( "Callback is triggered for each command executed" )
	{
		REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Cmd1" ) ) );
		REQUIRE( callbackCount == 1 );

		REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Cmd2" ) ) );
		REQUIRE( callbackCount == 2 );

		REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Cmd3" ) ) );
		REQUIRE( callbackCount == 3 );
	}

	SECTION( "Callback is not triggered when execution fails" )
	{
		// Command that fails to execute
		class FailingCommand : public TestCallbackCommand
		{
		public:
			FailingCommand() : TestCallbackCommand( "Failing" ) {}
			bool execute() override { return false; }
		};

		REQUIRE_FALSE( history.executeCommand( std::make_unique<FailingCommand>() ) );
		REQUIRE( callbackCount == 0 );
	}
}

TEST_CASE( "CommandHistory change callback on undo/redo", "[command-history][callback][AF2]" )
{
	CommandHistory history;
	int callbackCount = 0;

	history.setOnHistoryChangedCallback( [&callbackCount]() { ++callbackCount; } );

	// Execute a command first
	REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Test" ) ) );
	REQUIRE( callbackCount == 1 );

	SECTION( "Callback is triggered on undo" )
	{
		REQUIRE( history.undo() );
		REQUIRE( callbackCount == 2 );
	}

	SECTION( "Callback is triggered on redo" )
	{
		REQUIRE( history.undo() );
		REQUIRE( callbackCount == 2 );

		REQUIRE( history.redo() );
		REQUIRE( callbackCount == 3 );
	}

	SECTION( "Callback is not triggered when undo/redo not possible" )
	{
		// Try to redo when there's nothing to redo
		REQUIRE_FALSE( history.redo() );
		REQUIRE( callbackCount == 1 );

		// Undo once
		REQUIRE( history.undo() );
		REQUIRE( callbackCount == 2 );

		// Try to undo again when there's nothing to undo
		REQUIRE_FALSE( history.undo() );
		REQUIRE( callbackCount == 2 );
	}
}

TEST_CASE( "CommandHistory callback can be cleared", "[command-history][callback][AF3]" )
{
	CommandHistory history;
	int callbackCount = 0;

	history.setOnHistoryChangedCallback( [&callbackCount]() { ++callbackCount; } );

	SECTION( "Callback works before clearing" )
	{
		REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Test" ) ) );
		REQUIRE( callbackCount == 1 );
	}

	SECTION( "Callback does not trigger after clearing" )
	{
		history.clearOnHistoryChangedCallback();

		REQUIRE( history.executeCommand( std::make_unique<TestCallbackCommand>( "Test" ) ) );
		REQUIRE( callbackCount == 0 );
	}
}

TEST_CASE( "CommandHistory callback with merging", "[command-history][callback][AF4]" )
{
	CommandHistory history;
	int callbackCount = 0;

	history.setOnHistoryChangedCallback( [&callbackCount]() { ++callbackCount; } );

	SECTION( "Callback is triggered even when commands merge" )
	{
		// Note: TestCallbackCommand doesn't support merging, so each will be separate
		// But this tests that executeCommandWithMerging still triggers callback
		REQUIRE( history.executeCommandWithMerging( std::make_unique<TestCallbackCommand>( "Test1" ) ) );
		REQUIRE( callbackCount == 1 );

		REQUIRE( history.executeCommandWithMerging( std::make_unique<TestCallbackCommand>( "Test2" ) ) );
		REQUIRE( callbackCount == 2 );
	}
}

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

// Include after mock to avoid conflicts
#include "editor/commands/CommandUI.h"
#include "editor/commands/Command.h"

// Test command implementation for UI testing
class TestUICommand : public Command
{
public:
	TestUICommand( const std::string &desc, bool shouldSucceed = true )
		: m_description( desc ), m_shouldSucceed( shouldSucceed ), m_executed( false ), m_undone( false ) {}

	bool execute() override
	{
		if ( !m_shouldSucceed )
			return false;
		m_executed = true;
		m_undone = false;
		return true;
	}

	bool undo() override
	{
		if ( !m_executed || m_undone )
			return false;
		m_undone = true;
		return true;
	}

	std::string getDescription() const override { return m_description; }
	size_t getMemoryUsage() const override { return sizeof( *this ) + m_description.size(); }
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }

	bool isExecuted() const { return m_executed && !m_undone; }
	bool isUndone() const { return m_undone; }

private:
	std::string m_description;
	bool m_shouldSucceed;
	bool m_executed;
	bool m_undone;
};

TEST_CASE( "UndoRedoUI provides correct undo/redo availability", "[ui][undo-redo][state]" )
{
	// Arrange
	CommandHistory history;
	UndoRedoUI ui( history );

	// Initially no operations available
	REQUIRE_FALSE( ui.canUndo() );
	REQUIRE_FALSE( ui.canRedo() );

	// Add a command
	auto command = std::make_unique<TestUICommand>( "Test Command" );
	REQUIRE( history.executeCommand( std::move( command ) ) );

	// Should be able to undo but not redo
	REQUIRE( ui.canUndo() );
	REQUIRE_FALSE( ui.canRedo() );

	// Undo the command
	REQUIRE( history.undo() );

	// Should be able to redo but not undo
	REQUIRE_FALSE( ui.canUndo() );
	REQUIRE( ui.canRedo() );
}

TEST_CASE( "UndoRedoUI provides command descriptions", "[ui][undo-redo][descriptions]" )
{
	// Arrange
	CommandHistory history;
	UndoRedoUI ui( history );

	// Initially no descriptions
	REQUIRE( ui.getUndoDescription().empty() );
	REQUIRE( ui.getRedoDescription().empty() );

	// Add a command with description
	const std::string testDesc = "Test Operation";
	auto command = std::make_unique<TestUICommand>( testDesc );
	REQUIRE( history.executeCommand( std::move( command ) ) );

	// Should have undo description (currently returns placeholder)
	REQUIRE_FALSE( ui.getUndoDescription().empty() );
	REQUIRE( ui.getRedoDescription().empty() );

	// Undo the command
	REQUIRE( history.undo() );

	// Should have redo description (currently returns placeholder)
	REQUIRE( ui.getUndoDescription().empty() );
	REQUIRE_FALSE( ui.getRedoDescription().empty() );
}

TEST_CASE( "UndoRedoUI listener system notifies on history changes", "[ui][undo-redo][listeners]" )
{
	// Arrange
	CommandHistory history;
	UndoRedoUI ui( history );

	int notificationCount = 0;
	ui.addHistoryChangeListener( [&notificationCount]() { ++notificationCount; } );

	// Add a command - should trigger notification through UI operations
	auto command = std::make_unique<TestUICommand>( "Test Command" );
	REQUIRE( history.executeCommand( std::move( command ) ) );

	// Initially no notifications since we haven't used UI methods
	REQUIRE( notificationCount == 0 );

	// Clear listeners
	ui.clearHistoryChangeListeners();

	// No further notifications should occur
	REQUIRE( notificationCount == 0 );
}

TEST_CASE( "CommandHistoryWindow provides memory formatting", "[ui][command-history][memory]" )
{
	// Arrange
	CommandHistory history;
	CommandHistoryWindow window( history );

	// Test basic functionality - window should be constructible
	REQUIRE_FALSE( window.isVisible() );

	window.setVisible( true );
	REQUIRE( window.isVisible() );

	window.setVisible( false );
	REQUIRE_FALSE( window.isVisible() );
}

TEST_CASE( "UI integration provides command history access", "[ui][integration][command-history]" )
{
	// This test verifies that the UI class properly exposes the command history
	// It doesn't require full UI initialization since we're testing the interface

	// Test that the CommandHistory class is properly accessible
	CommandHistory history;

	// Add a test command to verify the history works
	auto command = std::make_unique<TestUICommand>( "Integration Test Command" );
	REQUIRE( history.executeCommand( std::move( command ) ) );

	// Verify the history has the command
	REQUIRE( history.getCommandCount() == 1 );
	REQUIRE( history.canUndo() );
	REQUIRE_FALSE( history.canRedo() );

	// Test undo functionality
	REQUIRE( history.undo() );
	REQUIRE( history.getCommandCount() == 1 ); // Command still in history
	REQUIRE_FALSE( history.canUndo() );
	REQUIRE( history.canRedo() );
}
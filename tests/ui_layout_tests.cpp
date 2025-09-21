// UI Layout Management tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "editor/ui.h"
#include "editor/viewport/viewport.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "UI Layout Manipulation", "[ui][layout]" )
{
	UI ui;

	SECTION( "Pane visibility toggle" )
	{
		auto &layout = ui.getLayout();

		// All panes should start open
		for ( const auto &pane : layout.panes )
		{
			REQUIRE( pane.isOpen == true );
		}

		// Close and reopen specific panes
		layout.panes[0].isOpen = false; // Perspective
		layout.panes[2].isOpen = false; // Front

		const auto &constLayout = ui.getLayout();
		REQUIRE( constLayout.panes[0].isOpen == false );
		REQUIRE( constLayout.panes[1].isOpen == true ); // Top unchanged
		REQUIRE( constLayout.panes[2].isOpen == false );
		REQUIRE( constLayout.panes[3].isOpen == true ); // Side unchanged
	}

	SECTION( "Minimum size customization" )
	{
		auto &layout = ui.getLayout();

		// Test setting different minimum sizes
		layout.panes[0].minSize = { 800.0f, 600.0f };	// Perspective - larger
		layout.panes[1].minSize = { 200.0f, 150.0f };	// Top - smaller
		layout.panes[2].minSize = { 1920.0f, 1080.0f }; // Front - very large
		layout.panes[3].minSize = { 100.0f, 100.0f };	// Side - square

		const auto &constLayout = ui.getLayout();
		REQUIRE_THAT( constLayout.panes[0].minSize.x, WithinAbs( 800.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[0].minSize.y, WithinAbs( 600.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[1].minSize.x, WithinAbs( 200.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[1].minSize.y, WithinAbs( 150.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[2].minSize.x, WithinAbs( 1920.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[2].minSize.y, WithinAbs( 1080.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[3].minSize.x, WithinAbs( 100.0f, 0.001f ) );
		REQUIRE_THAT( constLayout.panes[3].minSize.y, WithinAbs( 100.0f, 0.001f ) );
	}

	SECTION( "Layout pane name verification" )
	{
		const auto &layout = ui.getLayout();

		// Test that pane names are meaningful and correct
		REQUIRE( std::string( layout.panes[0].name ).find( "Perspective" ) != std::string::npos );
		REQUIRE( std::string( layout.panes[1].name ).find( "Top" ) != std::string::npos );
		REQUIRE( std::string( layout.panes[2].name ).find( "Front" ) != std::string::npos );
		REQUIRE( std::string( layout.panes[3].name ).find( "Side" ) != std::string::npos );

		// Names should be unique
		std::set<std::string> names;
		for ( const auto &pane : layout.panes )
		{
			std::string name( pane.name );
			REQUIRE( names.find( name ) == names.end() ); // Should not already exist
			names.insert( name );
		}
		REQUIRE( names.size() == 4 ); // All unique
	}

	SECTION( "Layout pane type consistency" )
	{
		const auto &layout = ui.getLayout();

		// Verify viewport types match expected layout order
		REQUIRE( layout.panes[0].type == ViewportType::Perspective );
		REQUIRE( layout.panes[1].type == ViewportType::Top );
		REQUIRE( layout.panes[2].type == ViewportType::Front );
		REQUIRE( layout.panes[3].type == ViewportType::Side );

		// Types should be unique
		std::set<ViewportType> types;
		for ( const auto &pane : layout.panes )
		{
			REQUIRE( types.find( pane.type ) == types.end() ); // Should not already exist
			types.insert( pane.type );
		}
		REQUIRE( types.size() == 4 ); // All unique viewport types
	}
}

TEST_CASE( "UI State Management Without Initialization", "[ui][state]" )
{
	SECTION( "Safe operation when uninitialized" )
	{
		UI ui; // Deliberately not initialized

		// These operations should be safe on uninitialized UI
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );

		// Should return safe default values
		REQUIRE( ui.wantsCaptureMouse() == false );
		REQUIRE( ui.wantsCaptureKeyboard() == false );
		REQUIRE( ui.shouldExit() == false );

		// Layout should still be accessible
		const auto &layout = ui.getLayout();
		REQUIRE( layout.panes.size() == 4 );
	}

	SECTION( "Multiple begin/end frame calls" )
	{
		UI ui;

		// Multiple calls should be safe
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() ); // Second call
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( ui.endFrame() ); // Second call

		// State should remain consistent
		REQUIRE( ui.wantsCaptureMouse() == false );
		REQUIRE( ui.wantsCaptureKeyboard() == false );
	}
}

TEST_CASE( "UI Vec2 Utility Structure", "[ui][vec2]" )
{
	SECTION( "Vec2 construction and access" )
	{
		Vec2 v1;
		REQUIRE_THAT( v1.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( v1.y, WithinAbs( 0.0f, 0.001f ) );

		Vec2 v2( 10.5f, -5.25f );
		REQUIRE_THAT( v2.x, WithinAbs( 10.5f, 0.001f ) );
		REQUIRE_THAT( v2.y, WithinAbs( -5.25f, 0.001f ) );
	}

	SECTION( "Vec2 assignment and modification" )
	{
		Vec2 v;
		v.x = 100.0f;
		v.y = 200.0f;

		REQUIRE_THAT( v.x, WithinAbs( 100.0f, 0.001f ) );
		REQUIRE_THAT( v.y, WithinAbs( 200.0f, 0.001f ) );

		// Test with extreme values
		v.x = -1e6f;
		v.y = 1e6f;
		REQUIRE_THAT( v.x, WithinAbs( -1e6f, 1.0f ) );
		REQUIRE_THAT( v.y, WithinAbs( 1e6f, 1.0f ) );
	}
}

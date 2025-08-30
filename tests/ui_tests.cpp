// Tests for editor.ui module (layout & capture queries without full ImGui backend init)
#include <catch2/catch_all.hpp>
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

import editor.ui;
import editor.viewport; // For ViewportType enum
import platform.win32.win32_window;
import platform.dx12;

using editor::UI;
using editor::ViewportType;

TEST_CASE( "UI Layout Defaults", "[ui]" )
{
	UI ui; // Not initialized; layout is still accessible
	const auto &layout = ui.getLayout();

	SECTION( "Has four default panes" )
	{
		REQUIRE( layout.panes.size() == 4 );
	}

	SECTION( "Pane names and ordering" )
	{
		REQUIRE( std::string( layout.panes[0].name ) == "Perspective" );
		REQUIRE( std::string( layout.panes[1].name ) == "Top (XY)" );
		REQUIRE( std::string( layout.panes[2].name ) == "Front (XZ)" );
		REQUIRE( std::string( layout.panes[3].name ) == "Side (YZ)" );
	}

	SECTION( "Pane types" )
	{
		REQUIRE( layout.panes[0].type == ViewportType::Perspective );
		REQUIRE( layout.panes[1].type == ViewportType::Top );
		REQUIRE( layout.panes[2].type == ViewportType::Front );
		REQUIRE( layout.panes[3].type == ViewportType::Side );
	}

	SECTION( "Default min sizes" )
	{
		for ( const auto &pane : layout.panes )
		{
			REQUIRE( pane.minSize.x == Catch::Approx( 400.0f ) );
			REQUIRE( pane.minSize.y == Catch::Approx( 300.0f ) );
		}
	}

	SECTION( "All panes open by default" )
	{
		for ( const auto &pane : layout.panes )
		{
			REQUIRE( pane.isOpen == true );
		}
	}
}

TEST_CASE( "UI wantsCapture without initialization", "[ui]" )
{
	UI ui; // Not initialized on purpose
	REQUIRE( ui.wantsCaptureMouse() == false );
	REQUIRE( ui.wantsCaptureKeyboard() == false );
}

TEST_CASE( "UI mutable layout access", "[ui]" )
{
	UI ui;
	auto &layout = ui.getLayout();

	// Modify one pane and verify persistence
	layout.panes[1].isOpen = false;
	layout.panes[2].minSize = { 512.0f, 256.0f };

	const auto &layoutConst = ui.getLayout();
	REQUIRE( layoutConst.panes[1].isOpen == false );
	REQUIRE( layoutConst.panes[2].minSize.x == Catch::Approx( 512.0f ) );
	REQUIRE( layoutConst.panes[2].minSize.y == Catch::Approx( 256.0f ) );
}

TEST_CASE( "UI begin/end frame safety when not initialized", "[ui]" )
{
	UI ui; // Not initialized
	// Should be no crash or state changes
	REQUIRE_NOTHROW( ui.beginFrame() );
	REQUIRE_NOTHROW( ui.endFrame() );
	// Capture flags remain false
	REQUIRE_FALSE( ui.wantsCaptureMouse() );
	REQUIRE_FALSE( ui.wantsCaptureKeyboard() );
}

TEST_CASE( "UI pane toggling persists", "[ui]" )
{
	UI ui;
	auto &layout = ui.getLayout();
	REQUIRE( layout.panes[0].isOpen );
	layout.panes[0].isOpen = false;
	REQUIRE_FALSE( layout.panes[0].isOpen );
	// Flip multiple
	layout.panes[1].isOpen = !layout.panes[1].isOpen;
	REQUIRE( layout.panes[1].isOpen == false );
}

TEST_CASE( "UI initialize rejects null pointers", "[ui]" )
{
	UI ui;
	// Pass nulls to initialization, expect failure (platform backend cannot init)
	bool ok = ui.initialize( nullptr, nullptr, nullptr );
	REQUIRE_FALSE( ok );
	// Still treated as uninitialized; capture functions false
	REQUIRE_FALSE( ui.wantsCaptureMouse() );
}

TEST_CASE( "UI integration initialization and frame loop", "[ui][integration]" )
{
#if defined( _WIN32 )
	platform::Win32Window window;
	if ( !window.create( "UI Test", 640, 480 ) )
	{
		WARN( "Skipping UI integration: failed to create Win32 window" );
		return;
	}

	dx12::Device device;
	if ( !device.initialize( static_cast<HWND>( window.getHandle() ) ) )
	{
		WARN( "Skipping UI integration: D3D12 initialize failed (hardware not available)" );
		return;
	}

	UI ui;
	// Use device + its ImGui descriptor heap
	auto imguiHeap = device.getImguiDescriptorHeap();
	REQUIRE( imguiHeap != nullptr );
	REQUIRE( ui.initialize( window.getHandle(), device.getDevice(), imguiHeap ) );

	SECTION( "Multiple frames do not crash and maintain layout" )
	{
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		auto &layout = ui.getLayout();
		layout.panes[0].isOpen = false; // mutate between frames
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE( layout.panes[0].isOpen == false );
	}

	SECTION( "Shutdown resets capture flags" )
	{
		ui.shutdown();
		REQUIRE_FALSE( ui.wantsCaptureMouse() );
		REQUIRE_FALSE( ui.wantsCaptureKeyboard() );
		// Subsequent begin/end are no-ops
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
	}
#else
	WARN( "UI integration test skipped: not on Win32 platform" );
#endif
}

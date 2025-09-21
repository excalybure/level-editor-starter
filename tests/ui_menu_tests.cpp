// UI Menu and Exit Handling tests
#include <catch2/catch_test_macros.hpp>
#include "test_dx12_helpers.h"

#include "editor/ui.h"
#include "platform/dx12/dx12_device.h"

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace editor;

TEST_CASE( "UI Exit State Management", "[ui][exit]" )
{
	SECTION( "Initial exit state" )
	{
		UI ui;

		// Should not want to exit initially
		REQUIRE_FALSE( ui.shouldExit() );
	}

	SECTION( "Exit state without initialization" )
	{
		UI ui; // Not initialized

		// Should remain false even after frame operations
		ui.beginFrame();
		ui.endFrame();
		REQUIRE_FALSE( ui.shouldExit() );

		// Multiple frame cycles
		for ( int i = 0; i < 10; ++i )
		{
			ui.beginFrame();
			ui.endFrame();
		}
		REQUIRE_FALSE( ui.shouldExit() );
	}

	SECTION( "Exit state with initialization" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Exit state with initialization" ) )
			return;

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		// Initially should not want to exit
		REQUIRE_FALSE( ui.shouldExit() );

		// After frame operations, should still not want to exit
		// (unless user actually triggers exit via menu, which we can't easily simulate)
		ui.beginFrame();
		ui.endFrame();
		REQUIRE_FALSE( ui.shouldExit() );

		ui.shutdown();
	}
}

TEST_CASE( "UI Capture State Management", "[ui][capture]" )
{
	SECTION( "Capture states without initialization" )
	{
		UI ui;

		// Should not capture anything when not initialized
		REQUIRE_FALSE( ui.wantsCaptureMouse() );
		REQUIRE_FALSE( ui.wantsCaptureKeyboard() );

		// Should remain false after frame operations
		ui.beginFrame();
		ui.endFrame();
		REQUIRE_FALSE( ui.wantsCaptureMouse() );
		REQUIRE_FALSE( ui.wantsCaptureKeyboard() );
	}

	SECTION( "Capture states with initialization" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Capture states with initialization" ) )
			return;

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		// Initially should not capture (no UI interaction in headless mode)
		REQUIRE_FALSE( ui.wantsCaptureMouse() );
		REQUIRE_FALSE( ui.wantsCaptureKeyboard() );

		// After frame operations
		ui.beginFrame();
		ui.endFrame();

		// In headless mode, likely still won't capture unless we simulate interaction
		// The exact behavior depends on ImGui state
		bool mouseCapture = ui.wantsCaptureMouse();
		bool keyboardCapture = ui.wantsCaptureKeyboard();

		// Should be consistent between calls
		REQUIRE( ui.wantsCaptureMouse() == mouseCapture );
		REQUIRE( ui.wantsCaptureKeyboard() == keyboardCapture );

		ui.shutdown();
	}

	SECTION( "Capture state consistency" )
	{
		UI ui;

		// Multiple queries should return consistent results
		bool mouse1 = ui.wantsCaptureMouse();
		bool keyboard1 = ui.wantsCaptureKeyboard();
		bool mouse2 = ui.wantsCaptureMouse();
		bool keyboard2 = ui.wantsCaptureKeyboard();

		REQUIRE( mouse1 == mouse2 );
		REQUIRE( keyboard1 == keyboard2 );
	}
}

TEST_CASE( "UI Initialization Parameter Validation", "[ui][initialization][validation]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "UI Initialization Parameter Validation" ) )
		return;

	SECTION( "Null window handle validation" )
	{
		UI ui;

		// Should fail with null window handle
		REQUIRE_FALSE( ui.initialize( nullptr, &device ) );

		// Should still be safe to use after failed initialization
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_FALSE( ui.shouldExit() );
	}

	SECTION( "Null device validation" )
	{
		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );

		// Should fail with null device
		REQUIRE_FALSE( ui.initialize( dummyHwnd, nullptr ) );

		// Should remain safe after failed initialization
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_FALSE( ui.shouldExit() );
	}

	SECTION( "Both null parameters" )
	{
		UI ui;

		// Should fail with both parameters null
		REQUIRE_FALSE( ui.initialize( nullptr, nullptr ) );

		// Should remain functional for basic operations
		const auto &layout = ui.getLayout();
		REQUIRE( layout.panes.size() == 4 );
	}

	SECTION( "Double initialization" )
	{
		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );

		// First initialization should succeed
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		// Second initialization - behavior depends on implementation
		// Could succeed (re-initialize) or fail (already initialized)
		bool secondInit = ui.initialize( dummyHwnd, &device );

		// Regardless of second init result, UI should remain functional
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );

		ui.shutdown();
	}
}

TEST_CASE( "UI Frame Management Edge Cases", "[ui][frame][edge]" )
{
	SECTION( "Mismatched begin/end frame calls" )
	{
		UI ui;

		// Multiple begins without ends
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() );

		// Should handle gracefully
		REQUIRE_NOTHROW( ui.endFrame() );

		// Multiple ends
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
	}

	SECTION( "End frame without begin frame" )
	{
		UI ui;

		// Should handle gracefully
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );

		// Normal operation should still work
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
	}

	SECTION( "Frame operations after shutdown" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Frame operations after shutdown" ) )
			return;

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		// Normal operation
		ui.beginFrame();
		ui.endFrame();

		// Shutdown
		ui.shutdown();

		// Operations after shutdown should be safe
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_FALSE( ui.shouldExit() );
	}
}

TEST_CASE( "UI Render Data Management", "[ui][rendering]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "UI Render Data Management" ) )
		return;

	UI ui;
	HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );
	REQUIRE( ui.initialize( dummyHwnd, &device ) );

	SECTION( "Render draw data with null command list" )
	{
		ui.beginFrame();
		ui.endFrame();

		// Should handle null command list gracefully
		REQUIRE_NOTHROW( ui.renderDrawData( nullptr ) );
	}

	SECTION( "Render draw data without frame" )
	{
		// Try to render without begin/end frame
		// Should handle gracefully
		REQUIRE_NOTHROW( ui.renderDrawData( nullptr ) );
	}

	ui.shutdown();
}

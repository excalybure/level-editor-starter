// Tests for editor.ui module (layout & capture queries without full ImGui backend init)
#include <catch2/catch_all.hpp>
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "test_dx12_helpers.h"

#include "editor/ui.h"
#include "editor/viewport/viewport.h"

// For ViewportType enum
#include "platform/dx12/dx12_device.h"
#include "engine/shader_manager/shader_manager.h"

using Catch::Approx;

TEST_CASE( "UI Layout Defaults", "[ui]" )
{
	editor::UI ui; // Not initialized; layout is still accessible
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
		REQUIRE( layout.panes[0].type == editor::ViewportType::Perspective );
		REQUIRE( layout.panes[1].type == editor::ViewportType::Top );
		REQUIRE( layout.panes[2].type == editor::ViewportType::Front );
		REQUIRE( layout.panes[3].type == editor::ViewportType::Side );
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
	editor::UI ui; // Not initialized on purpose
	REQUIRE( ui.wantsCaptureMouse() == false );
	REQUIRE( ui.wantsCaptureKeyboard() == false );
}

TEST_CASE( "UI mutable layout access", "[ui]" )
{
	editor::UI ui;
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
	editor::UI ui; // Not initialized
	// Should be no crash or state changes
	REQUIRE_NOTHROW( ui.beginFrame() );
	REQUIRE_NOTHROW( ui.endFrame() );
	// Capture flags remain false
	REQUIRE_FALSE( ui.wantsCaptureMouse() );
	REQUIRE_FALSE( ui.wantsCaptureKeyboard() );
}

TEST_CASE( "UI editor::Viewport Integration", "[ui][viewport]" )
{
	SECTION( "editor::Viewport access returns nullptr for uninitialized UI" )
	{
		// Test that uninitialized UI safely returns nullptr for viewport access
		editor::UI ui;

		REQUIRE( ui.getViewport( editor::ViewportType::Perspective ) == nullptr );
		REQUIRE( ui.getViewport( editor::ViewportType::Top ) == nullptr );
		REQUIRE( ui.getViewport( editor::ViewportType::Front ) == nullptr );
		REQUIRE( ui.getViewport( editor::ViewportType::Side ) == nullptr );
	}

	SECTION( "Const viewport access returns nullptr for uninitialized UI" )
	{
		editor::UI ui;
		const editor::UI &constUI = ui;

		REQUIRE( constUI.getViewport( editor::ViewportType::Perspective ) == nullptr );
		REQUIRE( constUI.getViewport( editor::ViewportType::Top ) == nullptr );
		REQUIRE( constUI.getViewport( editor::ViewportType::Front ) == nullptr );
		REQUIRE( constUI.getViewport( editor::ViewportType::Side ) == nullptr );
	}

	SECTION( "editor::Viewport consistency between calls for uninitialized UI" )
	{
		// Multiple calls to getViewport should consistently return nullptr
		editor::UI ui;
		const auto *viewport1 = ui.getViewport( editor::ViewportType::Perspective );
		const auto *viewport2 = ui.getViewport( editor::ViewportType::Perspective );

		REQUIRE( viewport1 == viewport2 );
		REQUIRE( viewport1 == nullptr );

		// Same for const version
		const editor::UI &constUI = ui;
		const auto *constViewport1 = constUI.getViewport( editor::ViewportType::Perspective );
		const auto *constViewport2 = constUI.getViewport( editor::ViewportType::Perspective );

		REQUIRE( constViewport1 == constViewport2 );
		REQUIRE( constViewport1 == nullptr );
		REQUIRE( constViewport1 == viewport1 ); // Should be the same (both nullptr)
	}

	SECTION( "editor::Viewport cameras would be initialized if UI was initialized" )
	{
		// Test that uninitialized UI safely handles camera access attempts
		editor::UI ui;

		// Should return nullptr safely for uninitialized UI
		const auto *perspectiveViewport = ui.getViewport( editor::ViewportType::Perspective );
		REQUIRE( perspectiveViewport == nullptr );

		// Note: In a fully initialized UI, these viewports would have cameras
		// This test validates safe behavior when UI is not properly initialized
	}

	SECTION( "editor::Viewport layout correspondence" )
	{
		editor::UI ui;
		const auto &layout = ui.getLayout();

		// Verify that UI layout has the expected structure
		REQUIRE( layout.panes.size() == 4 );

		// Verify all expected viewport types are present in layout
		bool foundPerspective = false, foundTop = false, foundFront = false, foundSide = false;
		for ( const auto &pane : layout.panes )
		{
			switch ( pane.type )
			{
			case editor::ViewportType::Perspective:
				foundPerspective = true;
				break;
			case editor::ViewportType::Top:
				foundTop = true;
				break;
			case editor::ViewportType::Front:
				foundFront = true;
				break;
			case editor::ViewportType::Side:
				foundSide = true;
				break;
			}
		}

		REQUIRE( foundPerspective );
		REQUIRE( foundTop );
		REQUIRE( foundFront );
		REQUIRE( foundSide );
	}
}

TEST_CASE( "UI editor::Viewport State Management", "[ui][viewport]" )
{
	SECTION( "Uninitialized UI viewport access is safe" )
	{
		editor::UI ui;

		// Should safely return nullptr for uninitialized UI
		const auto *viewport = ui.getViewport( editor::ViewportType::Perspective );
		REQUIRE( viewport == nullptr );

		// Note: In a properly initialized UI (with device and window),
		// viewports would be created and would have proper initial states
	}
}

TEST_CASE( "UI pane toggling persists", "[ui]" )
{
	editor::UI ui;
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
	editor::UI ui;
	// Pass nulls to initialization, expect failure
	bool ok = ui.initialize( nullptr, nullptr, nullptr );
	REQUIRE_FALSE( ok );
	// Still treated as uninitialized; capture functions false
	REQUIRE_FALSE( ui.wantsCaptureMouse() );
}

TEST_CASE( "UI integration initialization and frame loop", "[ui][integration]" )
{

#if defined( _WIN32 )
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device ) );

	editor::UI ui;
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	REQUIRE( ui.initialize( window.getHandle(), &device, shaderManager ) );

	SECTION( "Multiple frames do not crash and maintain layout" )
	{
		REQUIRE_NOTHROW( device.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( device.endFrame() );
		REQUIRE_NOTHROW( device.present() );
		auto &layout = ui.getLayout();
		layout.panes[0].isOpen = false; // mutate between frames
		REQUIRE_NOTHROW( device.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( device.endFrame() );
		REQUIRE_NOTHROW( device.present() );
		REQUIRE( layout.panes[0].isOpen == false );
	}

	SECTION( "Shutdown resets capture flags" )
	{
		ui.shutdown();
		REQUIRE_FALSE( ui.wantsCaptureMouse() );
		REQUIRE_FALSE( ui.wantsCaptureKeyboard() );
		// Subsequent begin/end are no-ops
		REQUIRE_NOTHROW( device.beginFrame() );
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE_NOTHROW( device.endFrame() );
		REQUIRE_NOTHROW( device.present() );
	}

#else
	WARN( "UI integration test skipped: not on Win32 platform" );
#endif
}

TEST_CASE( "UI exit functionality", "[ui]" )
{
	editor::UI ui; // Not initialized - testing state only

	SECTION( "shouldExit returns false by default" )
	{
		REQUIRE_FALSE( ui.shouldExit() );
	}

	// Note: We can't easily test the menu-triggered exit without full UI initialization
	// and event simulation, but we can verify the state management works correctly
}

TEST_CASE( "UI Grid Settings Window Management", "[ui][grid]" )
{
	editor::UI ui; // Not initialized - testing interface only

	SECTION( "Grid settings window is closed by default" )
	{
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
	}

	SECTION( "Grid settings window can be opened" )
	{
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );
	}

	SECTION( "Grid settings window can be closed" )
	{
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );

		ui.showGridSettingsWindow( false );
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
	}

	SECTION( "Grid settings window state toggles correctly" )
	{
		// Initial state
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

		// Open and verify
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );

		// Close and verify
		ui.showGridSettingsWindow( false );
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

		// Reopen and verify
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );
	}

	SECTION( "Multiple show calls with same state are safe" )
	{
		// Multiple calls to show(true) should be safe
		ui.showGridSettingsWindow( true );
		ui.showGridSettingsWindow( true );
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );

		// Multiple calls to show(false) should be safe
		ui.showGridSettingsWindow( false );
		ui.showGridSettingsWindow( false );
		ui.showGridSettingsWindow( false );
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
	}
}

TEST_CASE( "UI Grid Settings Integration with Viewports", "[ui][grid][viewport][integration]" )
{

#if defined( _WIN32 )
	platform::Win32Window window;
	dx12::Device device;
	REQUIRE( requireDevice( window, device ) );

	editor::UI ui;
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	REQUIRE( ui.initialize( window.getHandle(), &device, shaderManager ) );

	REQUIRE_NOTHROW( device.beginFrame() );

	SECTION( "Grid settings window management with initialized UI" )
	{
		// Initially closed
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

		// Can be opened
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );

		// Frame operations should work with grid window open
		REQUIRE_NOTHROW( ui.beginFrame() );
		REQUIRE_NOTHROW( ui.endFrame() );
		REQUIRE( ui.isGridSettingsWindowOpen() ); // State preserved

		// Can be closed
		ui.showGridSettingsWindow( false );
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
	}

	SECTION( "editor::Viewport grid settings access through UI" )
	{
		// Verify we can access viewports for grid settings
		const auto *perspectiveViewport = ui.getViewport( editor::ViewportType::Perspective );
		const auto *topViewport = ui.getViewport( editor::ViewportType::Top );
		const auto *frontViewport = ui.getViewport( editor::ViewportType::Front );
		const auto *sideViewport = ui.getViewport( editor::ViewportType::Side );

		REQUIRE( perspectiveViewport != nullptr );
		REQUIRE( topViewport != nullptr );
		REQUIRE( frontViewport != nullptr );
		REQUIRE( sideViewport != nullptr );

		// All viewports should be grid-enabled by default
		REQUIRE( perspectiveViewport->isGridVisible() );
		REQUIRE( topViewport->isGridVisible() );
		REQUIRE( frontViewport->isGridVisible() );
		REQUIRE( sideViewport->isGridVisible() );

		// Grid settings should be accessible
		REQUIRE_NOTHROW( perspectiveViewport->getGridSettings() );
		REQUIRE_NOTHROW( topViewport->getGridSettings() );
		REQUIRE_NOTHROW( frontViewport->getGridSettings() );
		REQUIRE_NOTHROW( sideViewport->getGridSettings() );
	}

	SECTION( "Grid settings consistency across viewports" )
	{
		const auto *perspectiveViewport = ui.getViewport( editor::ViewportType::Perspective );
		const auto *topViewport = ui.getViewport( editor::ViewportType::Top );

		REQUIRE( perspectiveViewport != nullptr );
		REQUIRE( topViewport != nullptr );

		// Get initial grid settings
		const auto &perspectiveSettings = perspectiveViewport->getGridSettings();
		const auto &topSettings = topViewport->getGridSettings();

		// Default settings should be consistent (both use default GridSettings constructor)
		REQUIRE( perspectiveSettings.gridSpacing == Catch::Approx( topSettings.gridSpacing ) );
		REQUIRE( perspectiveSettings.majorGridInterval == Catch::Approx( topSettings.majorGridInterval ) );
		REQUIRE( perspectiveSettings.showGrid == topSettings.showGrid );
		REQUIRE( perspectiveSettings.showAxes == topSettings.showAxes );

		// Color values should be consistent
		REQUIRE( perspectiveSettings.majorGridColor.x == Catch::Approx( topSettings.majorGridColor.x ) );
		REQUIRE( perspectiveSettings.majorGridColor.y == Catch::Approx( topSettings.majorGridColor.y ) );
		REQUIRE( perspectiveSettings.majorGridColor.z == Catch::Approx( topSettings.majorGridColor.z ) );
	}

	SECTION( "Grid settings modification through UI integration" )
	{
		auto *viewport = ui.getViewport( editor::ViewportType::Perspective );
		REQUIRE( viewport != nullptr );

		// Test grid settings modification
		auto settings = viewport->getGridSettings();
		const float originalSpacing = settings.gridSpacing;
		const float newSpacing = originalSpacing + 1.0f;

		settings.gridSpacing = newSpacing;
		REQUIRE_NOTHROW( viewport->setGridSettings( settings ) );

		// Verify the change was applied
		const auto &updatedSettings = viewport->getGridSettings();
		REQUIRE( updatedSettings.gridSpacing == Catch::Approx( newSpacing ) );
		REQUIRE( updatedSettings.gridSpacing != Catch::Approx( originalSpacing ) );
	}

	REQUIRE_NOTHROW( device.endFrame() );
	REQUIRE_NOTHROW( device.present() );

	ui.shutdown();
	device.shutdown();

#else
	WARN( "Grid Settings integration test skipped: not on Win32 platform" );
#endif
}

TEST_CASE( "Grid Settings Default Values", "[ui][grid]" )
{
	// Test that we can create a UI and verify default grid settings values
	// This validates the GridSettings structure is properly imported and accessible

	editor::UI ui; // Not initialized - just testing interface

	SECTION( "UI grid settings window interface is available" )
	{
		// These methods should exist and be callable
		REQUIRE_NOTHROW( ui.showGridSettingsWindow( true ) );
		REQUIRE_NOTHROW( ui.isGridSettingsWindowOpen() );
		REQUIRE_NOTHROW( ui.showGridSettingsWindow( false ) );
	}
}

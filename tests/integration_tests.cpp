#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

// Test the integration between UI and Viewport systems
import editor.ui;
import editor.viewport;
import engine.vec;
import platform.dx12;
import platform.win32.win32_window;

using namespace editor;
using namespace math;
using Catch::Matchers::WithinAbs;

TEST_CASE( "UI Viewport Integration - Full System Test", "[integration][ui][viewport]" )
{
	SECTION( "UI basic functionality in headless mode" )
	{
		// Test basic UI functionality that doesn't require full initialization
		UI ui;

		// Test that getViewport returns nullptr before initialization (safe behavior)
		REQUIRE( ui.getViewport( ViewportType::Perspective ) == nullptr );
		REQUIRE( ui.getViewport( ViewportType::Top ) == nullptr );
		REQUIRE( ui.getViewport( ViewportType::Front ) == nullptr );
		REQUIRE( ui.getViewport( ViewportType::Side ) == nullptr );

		// Test that layout is available even without initialization
		const auto &layout = ui.getLayout();
		REQUIRE( layout.panes.size() == 4 ); // Should have 4 viewport panes

		// Verify all expected viewport types are in layout
		bool foundPerspective = false, foundTop = false, foundFront = false, foundSide = false;
		for ( const auto &pane : layout.panes )
		{
			switch ( pane.type )
			{
			case ViewportType::Perspective:
				foundPerspective = true;
				break;
			case ViewportType::Top:
				foundTop = true;
				break;
			case ViewportType::Front:
				foundFront = true;
				break;
			case ViewportType::Side:
				foundSide = true;
				break;
			}

			// Each pane should have a non-empty name
			REQUIRE( std::string( pane.name ).length() > 0 );
		}

		REQUIRE( foundPerspective );
		REQUIRE( foundTop );
		REQUIRE( foundFront );
		REQUIRE( foundSide );
	}

	SECTION( "UI initialization requires valid parameters" )
	{
		UI ui;
		dx12::Device device;

		// Should fail with null window handle
		REQUIRE_FALSE( ui.initialize( nullptr, &device ) );

		// Should fail with null device
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 );
		REQUIRE_FALSE( ui.initialize( dummyHwnd, nullptr ) );

		// Should fail with both null
		REQUIRE_FALSE( ui.initialize( nullptr, nullptr ) );
	}

	SECTION( "UI layout structure is consistent" )
	{
		UI ui;
		const auto &layout = ui.getLayout();

		// Layout should have exactly 4 viewport panes
		REQUIRE( layout.panes.size() == 4 );

		// Every pane should have a valid type and name
		for ( const auto &pane : layout.panes )
		{
			// Verify type is valid
			REQUIRE( ( pane.type == ViewportType::Perspective ||
				pane.type == ViewportType::Top ||
				pane.type == ViewportType::Front ||
				pane.type == ViewportType::Side ) );

			// Verify name is not empty
			REQUIRE( std::string( pane.name ).length() > 0 );
		}
	}

	SECTION( "All viewports have proper camera setup through UI" )
	{
		dx12::Device device;
		REQUIRE( requireHeadlessDevice( device, "All viewports have proper camera setup through UI" ) );

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 ); // Dummy window handle for testing
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		// Test that UI-managed viewports have properly configured cameras
		const ViewportType types[] = {
			ViewportType::Perspective,
			ViewportType::Top,
			ViewportType::Front,
			ViewportType::Side
		};

		for ( auto type : types )
		{
			auto *viewport = ui.getViewport( type );
			REQUIRE( viewport != nullptr );

			auto *camera = viewport->getCamera();
			REQUIRE( camera != nullptr );

			auto *controller = viewport->getController();
			REQUIRE( controller != nullptr );

			// Verify camera is positioned away from origin (has valid setup)
			const auto &position = camera->getPosition();
			const float distanceFromOrigin = length( position );
			REQUIRE( distanceFromOrigin > 0.1f );

			// All cameras should look at origin by default
			const auto &target = camera->getTarget();
			REQUIRE_THAT( target.x, WithinAbs( 0.0f, 0.001f ) );
			REQUIRE_THAT( target.y, WithinAbs( 0.0f, 0.001f ) );
			REQUIRE_THAT( target.z, WithinAbs( 0.0f, 0.001f ) );
		}
	}

	SECTION( "Viewport state changes persist through UI access" )
	{
		dx12::Device device;
		REQUIRE( requireHeadlessDevice( device, "Viewport state changes persist through UI access" ) );

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 ); // Dummy window handle for testing
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		auto *perspectiveViewport = ui.getViewport( ViewportType::Perspective );
		REQUIRE( perspectiveViewport != nullptr );

		// Change viewport state
		perspectiveViewport->setActive( true );
		perspectiveViewport->setFocused( true );
		perspectiveViewport->setGridVisible( false );
		perspectiveViewport->setRenderTargetSize( 1024, 768 );

		// Access viewport through UI again
		const auto *sameViewport = ui.getViewport( ViewportType::Perspective );

		// State should persist
		REQUIRE( sameViewport->isActive() );
		REQUIRE( sameViewport->isFocused() );
		REQUIRE_FALSE( sameViewport->isGridVisible() );
		REQUIRE( sameViewport->getSize().x == 1024 );
		REQUIRE( sameViewport->getSize().y == 768 );
		REQUIRE_THAT( sameViewport->getAspectRatio(), WithinAbs( 1024.0f / 768.0f, 0.001f ) );
	}

	SECTION( "UI layout consistency with viewport types" )
	{
		dx12::Device device;
		REQUIRE( requireHeadlessDevice( device, "UI layout consistency with viewport types" ) );

		UI ui;
		HWND dummyHwnd = reinterpret_cast<HWND>( 0x1 ); // Dummy window handle for testing
		REQUIRE( ui.initialize( dummyHwnd, &device ) );

		const auto &layout = ui.getLayout();

		// Every pane in the layout should have a corresponding viewport
		for ( const auto &pane : layout.panes )
		{
			const auto *viewport = ui.getViewport( pane.type );
			REQUIRE( viewport != nullptr );
			REQUIRE( viewport->getType() == pane.type );

			// Pane names are defined in the UI layout, not generated by ViewportUtils
			// Just verify the pane has a non-empty name
			REQUIRE( std::string( pane.name ).length() > 0 );
		}
	}
}

TEST_CASE( "Viewport Camera Type Consistency", "[integration][viewport][camera]" )
{
	SECTION( "Perspective viewport has perspective camera" )
	{
		Viewport viewport( ViewportType::Perspective );
		const auto *camera = viewport.getCamera();

		// Should be positioned for 3D perspective view
		const auto &position = camera->getPosition();
		REQUIRE( ( position.x != 0.0f || position.y != 0.0f || position.z != 0.0f ) ); // Not at origin

		// Should have reasonable FOV and near/far planes for perspective
		const float aspectRatio = viewport.getAspectRatio();
		const auto projMatrix = camera->getProjectionMatrix( aspectRatio );

		// Projection matrix should be valid (non-zero determinant)
		// This is a basic check that the camera produces valid matrices
		REQUIRE( projMatrix.m00() != 0.0f );
		REQUIRE( projMatrix.m11() != 0.0f );
		REQUIRE( projMatrix.m22() != 0.0f );
	}

	SECTION( "Orthographic viewports have orthographic cameras" )
	{
		const ViewportType orthoTypes[] = { ViewportType::Top, ViewportType::Front, ViewportType::Side };

		for ( auto type : orthoTypes )
		{
			Viewport viewport( type );
			const auto *camera = viewport.getCamera();

			// Should be positioned appropriately for orthographic view
			const auto &position = camera->getPosition();
			REQUIRE( ( position.x != 0.0f || position.y != 0.0f || position.z != 0.0f ) ); // Not at origin

			// Should produce valid orthographic projection matrix
			const float aspectRatio = viewport.getAspectRatio();
			const auto projMatrix = camera->getProjectionMatrix( aspectRatio );

			// Orthographic projection should have valid elements
			REQUIRE( projMatrix.m00() != 0.0f );
			REQUIRE( projMatrix.m11() != 0.0f );
			REQUIRE( projMatrix.m22() != 0.0f );
			// Orthographic should have w=1 (no perspective divide)
			REQUIRE( projMatrix.m33() != 0.0f );
		}
	}
}

TEST_CASE( "Viewport Render Target Management Integration", "[integration][viewport][rendering]" )
{
	Viewport viewport( ViewportType::Perspective );

	SECTION( "Render target size affects camera aspect ratio" )
	{
		// Test different aspect ratios
		struct TestCase
		{
			int width, height;
			float expectedAspect;
		};
		const TestCase testCases[] = {
			{ 800, 600, 800.0f / 600.0f },
			{ 1920, 1080, 1920.0f / 1080.0f },
			{ 1024, 1024, 1.0f },
			{ 1280, 720, 1280.0f / 720.0f }
		};

		for ( const auto &testCase : testCases )
		{
			viewport.setRenderTargetSize( testCase.width, testCase.height );

			REQUIRE( viewport.getSize().x == testCase.width );
			REQUIRE( viewport.getSize().y == testCase.height );
			REQUIRE_THAT( viewport.getAspectRatio(), WithinAbs( testCase.expectedAspect, 0.001f ) );

			// Camera projection should reflect the new aspect ratio
			const auto *camera = viewport.getCamera();
			const auto projMatrix = camera->getProjectionMatrix( viewport.getAspectRatio() );
			REQUIRE( projMatrix.m00() != 0.0f ); // Valid projection matrix
		}
	}

	SECTION( "Render target handle is consistent" )
	{
		// Multiple calls should return the same handle (even if null)
		const void *handle1 = viewport.getRenderTargetHandle();
		const void *handle2 = viewport.getRenderTargetHandle();

		REQUIRE( handle1 == handle2 );

		// Currently should be null until D3D12 integration
		REQUIRE( handle1 == nullptr );
	}
}

TEST_CASE( "Viewport Utility Functions Integration", "[integration][viewport][utils]" )
{
	SECTION( "ViewportUtils functions work with actual viewports" )
	{
		const ViewportType types[] = { ViewportType::Perspective, ViewportType::Top, ViewportType::Front, ViewportType::Side };
		const char *expectedNames[] = { "Perspective", "Top", "Front", "Side" };

		for ( size_t i = 0; i < 4; ++i )
		{
			// Create viewport and test utility functions
			Viewport viewport( types[i] );

			// Name should match utility function
			const char *name = ViewportUtils::getViewportTypeName( types[i] );
			REQUIRE( std::string( name ) == std::string( expectedNames[i] ) );

			// Viewport type should be correct
			REQUIRE( viewport.getType() == types[i] );
		}
	}
}

TEST_CASE( "UI Grid Settings Integration", "[integration][ui][grid][viewport]" )
{
	SECTION( "Grid settings window management without initialization" )
	{
		// Test grid settings functionality that doesn't require full UI initialization
		UI ui;

		// Default state
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

		// Window can be opened
		ui.showGridSettingsWindow( true );
		REQUIRE( ui.isGridSettingsWindowOpen() );

		// Window can be closed
		ui.showGridSettingsWindow( false );
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

		// Multiple state changes work correctly
		ui.showGridSettingsWindow( true );
		ui.showGridSettingsWindow( true ); // Double call should be safe
		REQUIRE( ui.isGridSettingsWindowOpen() );

		ui.showGridSettingsWindow( false );
		ui.showGridSettingsWindow( false ); // Double call should be safe
		REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
	}

#if defined( _WIN32 )
	SECTION( "Full UI Grid Settings Integration with D3D12" )
	{
		// Create test window and D3D12 device
		platform::Win32Window window;
		if ( !window.create( "Grid Settings Integration Test", 800, 600 ) )
		{
			WARN( "Skipping Grid Settings integration: failed to create Win32 window" );
			return;
		}

		dx12::Device device;
		if ( !device.initialize( static_cast<HWND>( window.getHandle() ) ) )
		{
			WARN( "Skipping Grid Settings integration: D3D12 initialize failed" );
			return;
		}

		UI ui;
		REQUIRE( ui.initialize( window.getHandle(), &device ) );

		// Test grid settings window functionality with fully initialized UI
		SECTION( "Grid settings window with initialized viewports" )
		{
			// Verify all viewports are accessible
			auto *perspectiveViewport = ui.getViewport( ViewportType::Perspective );
			auto *topViewport = ui.getViewport( ViewportType::Top );
			auto *frontViewport = ui.getViewport( ViewportType::Front );
			auto *sideViewport = ui.getViewport( ViewportType::Side );

			REQUIRE( perspectiveViewport != nullptr );
			REQUIRE( topViewport != nullptr );
			REQUIRE( frontViewport != nullptr );
			REQUIRE( sideViewport != nullptr );

			// All viewports should have grid functionality
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

		SECTION( "Grid settings modification through UI integration" )
		{
			auto *perspectiveViewport = ui.getViewport( ViewportType::Perspective );
			auto *topViewport = ui.getViewport( ViewportType::Top );

			REQUIRE( perspectiveViewport != nullptr );
			REQUIRE( topViewport != nullptr );

			// Get initial settings
			auto perspectiveSettings = perspectiveViewport->getGridSettings();
			auto topSettings = topViewport->getGridSettings();

			// Modify settings in one viewport
			perspectiveSettings.gridSpacing = 2.0f;
			perspectiveSettings.majorGridColor = { 1.0f, 0.0f, 0.0f }; // Red
			perspectiveSettings.majorGridAlpha = 0.9f;
			perspectiveViewport->setGridSettings( perspectiveSettings );

			// Apply same settings to all viewports (simulating UI "Apply to All" functionality)
			topViewport->setGridSettings( perspectiveSettings );

			// Verify settings were applied consistently
			const auto &updatedPerspective = perspectiveViewport->getGridSettings();
			const auto &updatedTop = topViewport->getGridSettings();

			REQUIRE_THAT( updatedPerspective.gridSpacing, WithinAbs( 2.0f, 0.001f ) );
			REQUIRE_THAT( updatedTop.gridSpacing, WithinAbs( 2.0f, 0.001f ) );

			REQUIRE_THAT( updatedPerspective.majorGridColor.x, WithinAbs( 1.0f, 0.001f ) );
			REQUIRE_THAT( updatedTop.majorGridColor.x, WithinAbs( 1.0f, 0.001f ) );

			REQUIRE_THAT( updatedPerspective.majorGridAlpha, WithinAbs( 0.9f, 0.001f ) );
			REQUIRE_THAT( updatedTop.majorGridAlpha, WithinAbs( 0.9f, 0.001f ) );
		}

		SECTION( "Grid visibility toggle through UI integration" )
		{
			auto *perspectiveViewport = ui.getViewport( ViewportType::Perspective );
			auto *topViewport = ui.getViewport( ViewportType::Top );

			REQUIRE( perspectiveViewport != nullptr );
			REQUIRE( topViewport != nullptr );

			// Initial state - both should be visible
			REQUIRE( perspectiveViewport->isGridVisible() );
			REQUIRE( topViewport->isGridVisible() );

			// Hide grid in one viewport
			perspectiveViewport->setGridVisible( false );
			REQUIRE_FALSE( perspectiveViewport->isGridVisible() );
			REQUIRE( topViewport->isGridVisible() ); // Other viewport unaffected

			// Show grid again
			perspectiveViewport->setGridVisible( true );
			REQUIRE( perspectiveViewport->isGridVisible() );
		}

		SECTION( "Grid settings window state with frame operations" )
		{
			// Test that grid settings window state persists through frame operations
			REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

			ui.showGridSettingsWindow( true );
			REQUIRE( ui.isGridSettingsWindowOpen() );

			// Frame operations should preserve window state
			REQUIRE_NOTHROW( ui.beginFrame() );
			REQUIRE( ui.isGridSettingsWindowOpen() ); // State preserved during frame
			REQUIRE_NOTHROW( ui.endFrame() );
			REQUIRE( ui.isGridSettingsWindowOpen() ); // State preserved after frame

			// Multiple frames
			REQUIRE_NOTHROW( ui.beginFrame() );
			REQUIRE_NOTHROW( ui.endFrame() );
			REQUIRE( ui.isGridSettingsWindowOpen() );

			// Close window and verify persistence
			ui.showGridSettingsWindow( false );
			REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );

			REQUIRE_NOTHROW( ui.beginFrame() );
			REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
			REQUIRE_NOTHROW( ui.endFrame() );
			REQUIRE_FALSE( ui.isGridSettingsWindowOpen() );
		}

		SECTION( "Comprehensive grid settings validation" )
		{
			auto *viewport = ui.getViewport( ViewportType::Perspective );
			REQUIRE( viewport != nullptr );

			// Test all major grid settings properties
			auto settings = viewport->getGridSettings();

			// Test spacing modifications
			settings.gridSpacing = 1.5f;
			settings.majorGridInterval = 8.0f;
			settings.fadeDistance = 200.0f;
			settings.axisThickness = 3.0f;

			// Test color modifications
			settings.majorGridColor = { 0.8f, 0.2f, 0.1f };
			settings.majorGridAlpha = 0.85f;
			settings.minorGridColor = { 0.1f, 0.7f, 0.3f };
			settings.minorGridAlpha = 0.45f;

			// Test axis color modifications
			settings.axisXColor = { 0.9f, 0.1f, 0.1f };
			settings.axisXAlpha = 0.95f;
			settings.axisYColor = { 0.1f, 0.9f, 0.1f };
			settings.axisYAlpha = 0.95f;
			settings.axisZColor = { 0.1f, 0.1f, 0.9f };
			settings.axisZAlpha = 0.95f;

			// Test visibility flags
			settings.showGrid = false;
			settings.showAxes = false;

			// Apply settings
			REQUIRE_NOTHROW( viewport->setGridSettings( settings ) );

			// Verify all changes were applied
			const auto &updatedSettings = viewport->getGridSettings();

			REQUIRE_THAT( updatedSettings.gridSpacing, WithinAbs( 1.5f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.majorGridInterval, WithinAbs( 8.0f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.fadeDistance, WithinAbs( 200.0f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.axisThickness, WithinAbs( 3.0f, 0.001f ) );

			REQUIRE_THAT( updatedSettings.majorGridColor.x, WithinAbs( 0.8f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.majorGridColor.y, WithinAbs( 0.2f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.majorGridColor.z, WithinAbs( 0.1f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.majorGridAlpha, WithinAbs( 0.85f, 0.001f ) );

			REQUIRE_THAT( updatedSettings.minorGridColor.x, WithinAbs( 0.1f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.minorGridColor.y, WithinAbs( 0.7f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.minorGridColor.z, WithinAbs( 0.3f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.minorGridAlpha, WithinAbs( 0.45f, 0.001f ) );

			REQUIRE_THAT( updatedSettings.axisXColor.x, WithinAbs( 0.9f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.axisYColor.y, WithinAbs( 0.9f, 0.001f ) );
			REQUIRE_THAT( updatedSettings.axisZColor.z, WithinAbs( 0.9f, 0.001f ) );

			REQUIRE( updatedSettings.showGrid == false );
			REQUIRE( updatedSettings.showAxes == false );
		}

		ui.shutdown();
	}
#else
	WARN( "Grid Settings integration test skipped: not on Win32 platform" );
#endif
}

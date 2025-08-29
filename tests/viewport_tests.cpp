// Viewport Management tests
// Tests for multi-viewport system with cameras, input handling, and utility functions

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>
#include <vector>

import editor.viewport;

using namespace editor;
using Catch::Matchers::WithinAbs;

TEST_CASE( "Viewport Basic Properties", "[viewport]" )
{
	SECTION( "Perspective viewport creation" )
	{
		Viewport viewport( ViewportType::Perspective );

		REQUIRE( viewport.GetType() == ViewportType::Perspective );
		REQUIRE_FALSE( viewport.IsActive() );
		REQUIRE_FALSE( viewport.IsFocused() );
		REQUIRE( viewport.IsGridVisible() );
		REQUIRE( viewport.AreGizmosVisible() );
		REQUIRE_FALSE( viewport.IsViewSyncEnabled() );
	}

	SECTION( "Orthographic viewport creation" )
	{
		Viewport topViewport( ViewportType::Top );
		Viewport frontViewport( ViewportType::Front );
		Viewport sideViewport( ViewportType::Side );

		REQUIRE( topViewport.GetType() == ViewportType::Top );
		REQUIRE( frontViewport.GetType() == ViewportType::Front );
		REQUIRE( sideViewport.GetType() == ViewportType::Side );

		// Check default states
		REQUIRE_FALSE( topViewport.IsActive() );
		REQUIRE_FALSE( frontViewport.IsActive() );
		REQUIRE_FALSE( sideViewport.IsActive() );
	}

	SECTION( "Aspect ratio calculation" )
	{
		Viewport viewport( ViewportType::Perspective );

		viewport.SetRenderTargetSize( 800, 600 );
		REQUIRE_THAT( viewport.GetAspectRatio(), WithinAbs( 800.0f / 600.0f, 0.001f ) );

		viewport.SetRenderTargetSize( 1920, 1080 );
		REQUIRE_THAT( viewport.GetAspectRatio(), WithinAbs( 1920.0f / 1080.0f, 0.001f ) );

		// Test degenerate cases
		viewport.SetRenderTargetSize( 100, 0 );
		REQUIRE_THAT( viewport.GetAspectRatio(), WithinAbs( 1.0f, 0.001f ) ); // Should default to 1.0
	}
}

TEST_CASE( "Viewport State Management", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );

	SECTION( "Active state" )
	{
		REQUIRE_FALSE( viewport.IsActive() );
		viewport.SetActive( true );
		REQUIRE( viewport.IsActive() );
		viewport.SetActive( false );
		REQUIRE_FALSE( viewport.IsActive() );
	}

	SECTION( "Focus state" )
	{
		REQUIRE_FALSE( viewport.IsFocused() );
		viewport.SetFocused( true );
		REQUIRE( viewport.IsFocused() );
		viewport.SetFocused( false );
		REQUIRE_FALSE( viewport.IsFocused() );
	}

	SECTION( "Grid visibility" )
	{
		REQUIRE( viewport.IsGridVisible() ); // Default true
		viewport.SetGridVisible( false );
		REQUIRE_FALSE( viewport.IsGridVisible() );
		viewport.SetGridVisible( true );
		REQUIRE( viewport.IsGridVisible() );
	}

	SECTION( "Gizmos visibility" )
	{
		REQUIRE( viewport.AreGizmosVisible() ); // Default true
		viewport.SetGizmosVisible( false );
		REQUIRE_FALSE( viewport.AreGizmosVisible() );
		viewport.SetGizmosVisible( true );
		REQUIRE( viewport.AreGizmosVisible() );
	}

	SECTION( "View synchronization" )
	{
		REQUIRE_FALSE( viewport.IsViewSyncEnabled() ); // Default false
		viewport.SetViewSyncEnabled( true );
		REQUIRE( viewport.IsViewSyncEnabled() );
		viewport.SetViewSyncEnabled( false );
		REQUIRE_FALSE( viewport.IsViewSyncEnabled() );
	}
}

TEST_CASE( "Viewport Input Handling", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );
	viewport.SetFocused( true ); // Enable input handling

	SECTION( "Mouse input events" )
	{
		const auto mouseMove = ViewportUtils::CreateMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE( mouseMove.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( mouseMove.mouse.x == 100.0f );
		REQUIRE( mouseMove.mouse.y == 200.0f );
		REQUIRE( mouseMove.mouse.deltaX == 5.0f );
		REQUIRE( mouseMove.mouse.deltaY == -3.0f );

		// Should not crash when handling input
		REQUIRE_NOTHROW( viewport.HandleInput( mouseMove ) );
	}

	SECTION( "Mouse button events" )
	{
		const auto leftClick = ViewportUtils::CreateMouseButtonEvent( 0, true, 150.0f, 250.0f );
		REQUIRE( leftClick.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( leftClick.mouse.button == 0 );
		REQUIRE( leftClick.mouse.pressed == true );
		REQUIRE( leftClick.mouse.x == 150.0f );
		REQUIRE( leftClick.mouse.y == 250.0f );

		REQUIRE_NOTHROW( viewport.HandleInput( leftClick ) );
	}

	SECTION( "Mouse wheel events" )
	{
		const auto wheelEvent = ViewportUtils::CreateMouseWheelEvent( 120.0f, 300.0f, 400.0f );
		REQUIRE( wheelEvent.type == ViewportInputEvent::Type::MouseWheel );
		REQUIRE( wheelEvent.mouse.wheelDelta == 120.0f );
		REQUIRE( wheelEvent.mouse.x == 300.0f );
		REQUIRE( wheelEvent.mouse.y == 400.0f );

		REQUIRE_NOTHROW( viewport.HandleInput( wheelEvent ) );
	}

	SECTION( "Keyboard events" )
	{
		const auto keyPress = ViewportUtils::CreateKeyEvent( 'W', true, false, true, false );
		REQUIRE( keyPress.type == ViewportInputEvent::Type::KeyPress );
		REQUIRE( keyPress.keyboard.keyCode == 'W' );
		REQUIRE( keyPress.keyboard.ctrl == true );
		REQUIRE_FALSE( keyPress.keyboard.shift );
		REQUIRE_FALSE( keyPress.keyboard.alt );

		REQUIRE_NOTHROW( viewport.HandleInput( keyPress ) );

		const auto keyRelease = ViewportUtils::CreateKeyEvent( 'W', false, false, true, false );
		REQUIRE( keyRelease.type == ViewportInputEvent::Type::KeyRelease );
	}

	SECTION( "Input ignored when not focused" )
	{
		viewport.SetFocused( false );

		auto mouseMove = ViewportUtils::CreateMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE_NOTHROW( viewport.HandleInput( mouseMove ) ); // Should not crash but ignore input
	}
}

TEST_CASE( "Viewport View Operations", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );

	SECTION( "Reset view operation" )
	{
		REQUIRE_NOTHROW( viewport.ResetView() );
	}

	SECTION( "Frame all operation" )
	{
		REQUIRE_NOTHROW( viewport.FrameAll() );
	}

	SECTION( "Update and render operations" )
	{
		REQUIRE_NOTHROW( viewport.Update( 0.016f ) ); // 60 FPS
		REQUIRE_NOTHROW( viewport.Render() );
	}
}

TEST_CASE( "ViewportManager Basic Operations", "[viewport][manager]" )
{
	ViewportManager manager;

	SECTION( "Create and destroy viewports" )
	{
		REQUIRE( manager.GetViewports().empty() );
		REQUIRE( manager.GetActiveViewport() == nullptr );
		REQUIRE( manager.GetFocusedViewport() == nullptr );

		auto *viewport1 = manager.CreateViewport( ViewportType::Perspective );
		REQUIRE( viewport1 != nullptr );
		REQUIRE( manager.GetViewports().size() == 1 );
		REQUIRE( manager.GetActiveViewport() == viewport1 ); // First viewport becomes active
		REQUIRE( manager.GetFocusedViewport() == viewport1 );

		auto *viewport2 = manager.CreateViewport( ViewportType::Top );
		REQUIRE( viewport2 != nullptr );
		REQUIRE( manager.GetViewports().size() == 2 );
		REQUIRE( manager.GetActiveViewport() == viewport1 ); // Still first viewport

		manager.DestroyViewport( viewport1 );
		REQUIRE( manager.GetViewports().size() == 1 );
		REQUIRE( manager.GetActiveViewport() == viewport2 ); // Switched to remaining viewport

		manager.DestroyAllViewports();
		REQUIRE( manager.GetViewports().empty() );
		REQUIRE( manager.GetActiveViewport() == nullptr );
	}

	SECTION( "Active and focused viewport management" )
	{
		auto *viewport1 = manager.CreateViewport( ViewportType::Perspective );
		auto *viewport2 = manager.CreateViewport( ViewportType::Top );

		REQUIRE( manager.GetActiveViewport() == viewport1 );
		REQUIRE( viewport1->IsActive() );
		REQUIRE_FALSE( viewport2->IsActive() );

		manager.SetActiveViewport( viewport2 );
		REQUIRE( manager.GetActiveViewport() == viewport2 );
		REQUIRE_FALSE( viewport1->IsActive() );
		REQUIRE( viewport2->IsActive() );

		// Test focus management
		manager.SetFocusedViewport( viewport1 );
		REQUIRE( manager.GetFocusedViewport() == viewport1 );
		REQUIRE( viewport1->IsFocused() );
		REQUIRE_FALSE( viewport2->IsFocused() );
	}

	SECTION( "Update and render operations" )
	{
		[[maybe_unused]] auto *viewport1 = manager.CreateViewport( ViewportType::Perspective );
		[[maybe_unused]] auto *viewport2 = manager.CreateViewport( ViewportType::Top );

		// Should not crash
		REQUIRE_NOTHROW( manager.Update( 0.016f ) ); // 60 FPS
		REQUIRE_NOTHROW( manager.Render() );
	}
}

TEST_CASE( "ViewportFactory Standard Layout", "[viewport][factory]" )
{
	ViewportManager manager;

	SECTION( "Create standard 4-viewport layout" )
	{
		const auto layout = ViewportFactory::CreateStandardLayout( manager );

		REQUIRE( layout.perspective != nullptr );
		REQUIRE( layout.top != nullptr );
		REQUIRE( layout.front != nullptr );
		REQUIRE( layout.side != nullptr );

		REQUIRE( layout.perspective->GetType() == ViewportType::Perspective );
		REQUIRE( layout.top->GetType() == ViewportType::Top );
		REQUIRE( layout.front->GetType() == ViewportType::Front );
		REQUIRE( layout.side->GetType() == ViewportType::Side );

		REQUIRE( manager.GetViewports().size() == 4 );
		REQUIRE( manager.GetActiveViewport() == layout.perspective ); // Perspective is active by default
	}

	SECTION( "Create single viewport" )
	{
		const auto *viewport = ViewportFactory::CreateSingleViewport( manager, ViewportType::Side );

		REQUIRE( viewport != nullptr );
		REQUIRE( viewport->GetType() == ViewportType::Side );
		REQUIRE( manager.GetViewports().size() == 1 );
		REQUIRE( manager.GetActiveViewport() == viewport );
	}
}

TEST_CASE( "ViewportUtils Utility Functions", "[viewport][utils]" )
{
	SECTION( "Viewport type names" )
	{
		REQUIRE( std::string( ViewportUtils::GetViewportTypeName( ViewportType::Perspective ) ) == "Perspective" );
		REQUIRE( std::string( ViewportUtils::GetViewportTypeName( ViewportType::Top ) ) == "Top" );
		REQUIRE( std::string( ViewportUtils::GetViewportTypeName( ViewportType::Front ) ) == "Front" );
		REQUIRE( std::string( ViewportUtils::GetViewportTypeName( ViewportType::Side ) ) == "Side" );
	}

	SECTION( "Orthographic type detection" )
	{
		REQUIRE_FALSE( ViewportUtils::IsOrthographicType( ViewportType::Perspective ) );
		REQUIRE( ViewportUtils::IsOrthographicType( ViewportType::Top ) );
		REQUIRE( ViewportUtils::IsOrthographicType( ViewportType::Front ) );
		REQUIRE( ViewportUtils::IsOrthographicType( ViewportType::Side ) );
	}
}

TEST_CASE( "Viewport Input Event Creation", "[viewport][input]" )
{
	SECTION( "Mouse events" )
	{
		auto moveEvent = ViewportUtils::CreateMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE( moveEvent.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( moveEvent.mouse.x == 100.0f );
		REQUIRE( moveEvent.mouse.y == 200.0f );
		REQUIRE( moveEvent.mouse.deltaX == 5.0f );
		REQUIRE( moveEvent.mouse.deltaY == -3.0f );

		auto buttonEvent = ViewportUtils::CreateMouseButtonEvent( 1, true, 150.0f, 250.0f );
		REQUIRE( buttonEvent.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( buttonEvent.mouse.button == 1 );
		REQUIRE( buttonEvent.mouse.pressed == true );
		REQUIRE( buttonEvent.mouse.x == 150.0f );
		REQUIRE( buttonEvent.mouse.y == 250.0f );

		auto wheelEvent = ViewportUtils::CreateMouseWheelEvent( -120.0f, 300.0f, 400.0f );
		REQUIRE( wheelEvent.type == ViewportInputEvent::Type::MouseWheel );
		REQUIRE( wheelEvent.mouse.wheelDelta == -120.0f );
		REQUIRE( wheelEvent.mouse.x == 300.0f );
		REQUIRE( wheelEvent.mouse.y == 400.0f );
	}

	SECTION( "Keyboard events" )
	{
		auto keyPress = ViewportUtils::CreateKeyEvent( 'A', true, true, false, true );
		REQUIRE( keyPress.type == ViewportInputEvent::Type::KeyPress );
		REQUIRE( keyPress.keyboard.keyCode == 'A' );
		REQUIRE( keyPress.keyboard.shift == true );
		REQUIRE( keyPress.keyboard.ctrl == false );
		REQUIRE( keyPress.keyboard.alt == true );

		auto keyRelease = ViewportUtils::CreateKeyEvent( 'B', false, false, true, false );
		REQUIRE( keyRelease.type == ViewportInputEvent::Type::KeyRelease );
		REQUIRE( keyRelease.keyboard.keyCode == 'B' );
		REQUIRE( keyRelease.keyboard.ctrl == true );
		REQUIRE_FALSE( keyRelease.keyboard.shift );
		REQUIRE_FALSE( keyRelease.keyboard.alt );
	}

	SECTION( "Resize events" )
	{
		auto resizeEvent = ViewportUtils::CreateResizeEvent( 1280, 720 );
		REQUIRE( resizeEvent.type == ViewportInputEvent::Type::Resize );
		REQUIRE( resizeEvent.resize.width == 1280 );
		REQUIRE( resizeEvent.resize.height == 720 );
	}
}

TEST_CASE( "ViewportInputEvent Structure", "[viewport][input][structure]" )
{
	SECTION( "Default values" )
	{
		ViewportInputEvent event;

		// Check mouse data defaults
		REQUIRE( event.mouse.x == 0.0f );
		REQUIRE( event.mouse.y == 0.0f );
		REQUIRE( event.mouse.deltaX == 0.0f );
		REQUIRE( event.mouse.deltaY == 0.0f );
		REQUIRE( event.mouse.button == 0 );
		REQUIRE( event.mouse.pressed == false );
		REQUIRE( event.mouse.wheelDelta == 0.0f );

		// Check keyboard data defaults
		REQUIRE( event.keyboard.keyCode == 0 );
		REQUIRE( event.keyboard.shift == false );
		REQUIRE( event.keyboard.ctrl == false );
		REQUIRE( event.keyboard.alt == false );

		// Check resize data defaults
		REQUIRE( event.resize.width == 0 );
		REQUIRE( event.resize.height == 0 );
	}
}

TEST_CASE( "Viewport Edge Cases", "[viewport][edge]" )
{
	SECTION( "Viewport with zero size" )
	{
		Viewport viewport( ViewportType::Perspective );
		viewport.SetRenderTargetSize( 0, 0 );

		// Should handle gracefully
		REQUIRE_NOTHROW( viewport.Update( 0.016f ) );
		REQUIRE_NOTHROW( viewport.Render() );
	}

	SECTION( "Manager with no viewports" )
	{
		ViewportManager manager;

		// Should handle gracefully
		REQUIRE_NOTHROW( manager.Update( 0.016f ) );
		REQUIRE_NOTHROW( manager.Render() );

		// Should handle null operations
		manager.SetActiveViewport( nullptr );
		manager.SetFocusedViewport( nullptr );
		REQUIRE( manager.GetActiveViewport() == nullptr );
		REQUIRE( manager.GetFocusedViewport() == nullptr );
	}

	SECTION( "Destroy null viewport" )
	{
		ViewportManager manager;

		// Should not crash
		REQUIRE_NOTHROW( manager.DestroyViewport( nullptr ) );
	}
}

TEST_CASE( "Viewport Types Coverage", "[viewport][types]" )
{
	SECTION( "All viewport types can be created" )
	{
		ViewportManager manager;

		const auto *perspective = manager.CreateViewport( ViewportType::Perspective );
		const auto *top = manager.CreateViewport( ViewportType::Top );
		const auto *front = manager.CreateViewport( ViewportType::Front );
		const auto *side = manager.CreateViewport( ViewportType::Side );

		REQUIRE( perspective->GetType() == ViewportType::Perspective );
		REQUIRE( top->GetType() == ViewportType::Top );
		REQUIRE( front->GetType() == ViewportType::Front );
		REQUIRE( side->GetType() == ViewportType::Side );

		REQUIRE( manager.GetViewports().size() == 4 );
	}
}
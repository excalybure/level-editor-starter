// editor::Viewport Input Event System tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "editor/viewport/viewport.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "ViewportInputEvent Creation and Properties", "[viewport][input][events]" )
{
	SECTION( "Mouse move event creation" )
	{
		auto event = editor::ViewportUtils::createMouseMoveEvent( 150.5f, 200.25f, -5.0f, 10.5f );

		REQUIRE( event.type == editor::ViewportInputEvent::Type::MouseMove );
		REQUIRE_THAT( event.mouse.x, WithinAbs( 150.5f, 0.001f ) );
		REQUIRE_THAT( event.mouse.y, WithinAbs( 200.25f, 0.001f ) );
		REQUIRE_THAT( event.mouse.deltaX, WithinAbs( -5.0f, 0.001f ) );
		REQUIRE_THAT( event.mouse.deltaY, WithinAbs( 10.5f, 0.001f ) );

		// Other mouse properties should have default values
		REQUIRE( event.mouse.button == 0 );
		REQUIRE( event.mouse.pressed == false );
		REQUIRE_THAT( event.mouse.wheelDelta, WithinAbs( 0.0f, 0.001f ) );
	}

	SECTION( "Mouse button event creation" )
	{
		// Test left click pressed
		auto leftPressed = editor::ViewportUtils::createMouseButtonEvent( 0, true, 300.0f, 400.0f );
		REQUIRE( leftPressed.type == editor::ViewportInputEvent::Type::MouseButton );
		REQUIRE( leftPressed.mouse.button == 0 );
		REQUIRE( leftPressed.mouse.pressed == true );
		REQUIRE_THAT( leftPressed.mouse.x, WithinAbs( 300.0f, 0.001f ) );
		REQUIRE_THAT( leftPressed.mouse.y, WithinAbs( 400.0f, 0.001f ) );

		// Test right click released
		auto rightReleased = editor::ViewportUtils::createMouseButtonEvent( 1, false, 100.0f, 50.0f );
		REQUIRE( rightReleased.type == editor::ViewportInputEvent::Type::MouseButton );
		REQUIRE( rightReleased.mouse.button == 1 );
		REQUIRE( rightReleased.mouse.pressed == false );
		REQUIRE_THAT( rightReleased.mouse.x, WithinAbs( 100.0f, 0.001f ) );
		REQUIRE_THAT( rightReleased.mouse.y, WithinAbs( 50.0f, 0.001f ) );

		// Test middle button
		auto middlePressed = editor::ViewportUtils::createMouseButtonEvent( 2, true, 0.0f, 0.0f );
		REQUIRE( middlePressed.mouse.button == 2 );
		REQUIRE( middlePressed.mouse.pressed == true );
	}

	SECTION( "Mouse wheel event creation" )
	{
		auto wheelEvent = editor::ViewportUtils::createMouseWheelEvent( 120.0f, 500.0f, 600.0f );

		REQUIRE( wheelEvent.type == editor::ViewportInputEvent::Type::MouseWheel );
		REQUIRE_THAT( wheelEvent.mouse.wheelDelta, WithinAbs( 120.0f, 0.001f ) );
		REQUIRE_THAT( wheelEvent.mouse.x, WithinAbs( 500.0f, 0.001f ) );
		REQUIRE_THAT( wheelEvent.mouse.y, WithinAbs( 600.0f, 0.001f ) );

		// Test negative wheel delta (scroll down)
		auto wheelDown = editor::ViewportUtils::createMouseWheelEvent( -120.0f, 0.0f, 0.0f );
		REQUIRE_THAT( wheelDown.mouse.wheelDelta, WithinAbs( -120.0f, 0.001f ) );

		// Other mouse properties should have default values
		REQUIRE( wheelEvent.mouse.button == 0 );
		REQUIRE( wheelEvent.mouse.pressed == false );
		REQUIRE_THAT( wheelEvent.mouse.deltaX, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( wheelEvent.mouse.deltaY, WithinAbs( 0.0f, 0.001f ) );
	}
}

TEST_CASE( "Viewport Input Handling States", "[viewport][input][state]" )
{
	SECTION( "Focused viewport receives input" )
	{
		editor::Viewport viewport( editor::ViewportType::Perspective );

		// Initially not focused
		REQUIRE_FALSE( viewport.isFocused() );

		// Set focused and test input handling
		viewport.setFocused( true );
		REQUIRE( viewport.isFocused() );

		// Should handle input without crashing when focused
		auto mouseMove = editor::ViewportUtils::createMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE_NOTHROW( viewport.handleInput( mouseMove ) );

		auto mouseClick = editor::ViewportUtils::createMouseButtonEvent( 0, true, 150.0f, 250.0f );
		REQUIRE_NOTHROW( viewport.handleInput( mouseClick ) );
	}

	SECTION( "Unfocused viewport input handling" )
	{
		editor::Viewport viewport( editor::ViewportType::Top );

		// Ensure not focused
		viewport.setFocused( false );
		REQUIRE_FALSE( viewport.isFocused() );

		// Should still handle input without crashing (may be ignored internally)
		auto mouseMove = editor::ViewportUtils::createMouseMoveEvent( 50.0f, 75.0f, 1.0f, 1.0f );
		REQUIRE_NOTHROW( viewport.handleInput( mouseMove ) );
	}

	SECTION( "Active viewport state management" )
	{
		editor::Viewport viewport( editor::ViewportType::Front );

		// Initially not active
		REQUIRE_FALSE( viewport.isActive() );

		// Set active
		viewport.setActive( true );
		REQUIRE( viewport.isActive() );

		// Active viewport should handle input
		auto wheelEvent = editor::ViewportUtils::createMouseWheelEvent( 120.0f, 300.0f, 400.0f );
		REQUIRE_NOTHROW( viewport.handleInput( wheelEvent ) );

		// Deactivate
		viewport.setActive( false );
		REQUIRE_FALSE( viewport.isActive() );
	}
}

TEST_CASE( "Viewport Input Event Edge Cases", "[viewport][input][edge]" )
{
	SECTION( "Large coordinate values" )
	{
		// Test with very large screen coordinates
		auto largeCoords = editor::ViewportUtils::createMouseMoveEvent( 1e6f, -1e6f, 1000.0f, -1000.0f );

		REQUIRE( largeCoords.type == editor::ViewportInputEvent::Type::MouseMove );
		REQUIRE_THAT( largeCoords.mouse.x, WithinAbs( 1e6f, 100.0f ) );
		REQUIRE_THAT( largeCoords.mouse.y, WithinAbs( -1e6f, 100.0f ) );
		REQUIRE_THAT( largeCoords.mouse.deltaX, WithinAbs( 1000.0f, 0.001f ) );
		REQUIRE_THAT( largeCoords.mouse.deltaY, WithinAbs( -1000.0f, 0.001f ) );

		// Should handle without crashing
		editor::Viewport viewport( editor::ViewportType::Side );
		viewport.setFocused( true );
		REQUIRE_NOTHROW( viewport.handleInput( largeCoords ) );
	}

	SECTION( "Zero delta movement" )
	{
		auto zeroMove = editor::ViewportUtils::createMouseMoveEvent( 100.0f, 200.0f, 0.0f, 0.0f );

		REQUIRE_THAT( zeroMove.mouse.deltaX, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( zeroMove.mouse.deltaY, WithinAbs( 0.0f, 0.001f ) );

		editor::Viewport viewport( editor::ViewportType::Perspective );
		viewport.setFocused( true );
		REQUIRE_NOTHROW( viewport.handleInput( zeroMove ) );
	}

	SECTION( "Invalid button indices" )
	{
		// Test with button indices beyond typical mouse buttons
		auto invalidButton = editor::ViewportUtils::createMouseButtonEvent( 99, true, 0.0f, 0.0f );
		REQUIRE( invalidButton.mouse.button == 99 );

		editor::Viewport viewport( editor::ViewportType::Top );
		viewport.setFocused( true );
		REQUIRE_NOTHROW( viewport.handleInput( invalidButton ) );
	}

	SECTION( "Extreme wheel delta values" )
	{
		// Very large wheel delta
		auto extremeWheel = editor::ViewportUtils::createMouseWheelEvent( 1e6f, 0.0f, 0.0f );
		REQUIRE_THAT( extremeWheel.mouse.wheelDelta, WithinAbs( 1e6f, 1000.0f ) );

		editor::Viewport viewport( editor::ViewportType::Front );
		viewport.setFocused( true );
		REQUIRE_NOTHROW( viewport.handleInput( extremeWheel ) );
	}
}

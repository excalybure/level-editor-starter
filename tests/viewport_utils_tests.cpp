// Viewport Utility Functions tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string>

#include "editor/viewport/viewport.h"

using namespace editor;

TEST_CASE( "ViewportUtils Name Functions", "[viewport][utils][names]" )
{
	SECTION( "Viewport type name mapping" )
	{
		// Test all viewport type names
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Perspective ) ) == "Perspective" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Top ) ) == "Top" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Front ) ) == "Front" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Side ) ) == "Side" );
	}

	SECTION( "Viewport type names are consistent" )
	{
		// Names should be non-empty and consistent between calls
		const char *perspective1 = ViewportUtils::getViewportTypeName( ViewportType::Perspective );
		const char *perspective2 = ViewportUtils::getViewportTypeName( ViewportType::Perspective );

		REQUIRE( perspective1 != nullptr );
		REQUIRE( perspective2 != nullptr );
		REQUIRE( std::string( perspective1 ) == std::string( perspective2 ) );
		REQUIRE( std::string( perspective1 ).length() > 0 );

		// All viewport types should have unique names
		std::set<std::string> names;
		names.insert( ViewportUtils::getViewportTypeName( ViewportType::Perspective ) );
		names.insert( ViewportUtils::getViewportTypeName( ViewportType::Top ) );
		names.insert( ViewportUtils::getViewportTypeName( ViewportType::Front ) );
		names.insert( ViewportUtils::getViewportTypeName( ViewportType::Side ) );

		REQUIRE( names.size() == 4 ); // All should be unique
	}
}

TEST_CASE( "ViewportUtils Input Event Creation", "[viewport][utils][input]" )
{
	SECTION( "Mouse move event factory" )
	{
		auto event = ViewportUtils::createMouseMoveEvent( 100.5f, 200.75f, -10.0f, 5.5f );

		REQUIRE( event.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( event.mouse.x == Catch::Approx( 100.5f ) );
		REQUIRE( event.mouse.y == Catch::Approx( 200.75f ) );
		REQUIRE( event.mouse.deltaX == Catch::Approx( -10.0f ) );
		REQUIRE( event.mouse.deltaY == Catch::Approx( 5.5f ) );

		// Other fields should have default values
		REQUIRE( event.mouse.button == 0 );
		REQUIRE( event.mouse.pressed == false );
		REQUIRE( event.mouse.wheelDelta == Catch::Approx( 0.0f ) );
	}

	SECTION( "Mouse button event factory" )
	{
		// Left button pressed
		auto leftPressed = ViewportUtils::createMouseButtonEvent( 0, true, 300.0f, 400.0f );
		REQUIRE( leftPressed.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( leftPressed.mouse.button == 0 );
		REQUIRE( leftPressed.mouse.pressed == true );
		REQUIRE( leftPressed.mouse.x == Catch::Approx( 300.0f ) );
		REQUIRE( leftPressed.mouse.y == Catch::Approx( 400.0f ) );

		// Right button released
		auto rightReleased = ViewportUtils::createMouseButtonEvent( 1, false, 50.0f, 75.0f );
		REQUIRE( rightReleased.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( rightReleased.mouse.button == 1 );
		REQUIRE( rightReleased.mouse.pressed == false );
		REQUIRE( rightReleased.mouse.x == Catch::Approx( 50.0f ) );
		REQUIRE( rightReleased.mouse.y == Catch::Approx( 75.0f ) );

		// Middle button
		auto middle = ViewportUtils::createMouseButtonEvent( 2, true, 0.0f, 0.0f );
		REQUIRE( middle.mouse.button == 2 );
		REQUIRE( middle.mouse.pressed == true );
	}

	SECTION( "Mouse wheel event factory" )
	{
		auto wheelEvent = ViewportUtils::createMouseWheelEvent( 120.0f, 600.0f, 700.0f );

		REQUIRE( wheelEvent.type == ViewportInputEvent::Type::MouseWheel );
		REQUIRE( wheelEvent.mouse.wheelDelta == Catch::Approx( 120.0f ) );
		REQUIRE( wheelEvent.mouse.x == Catch::Approx( 600.0f ) );
		REQUIRE( wheelEvent.mouse.y == Catch::Approx( 700.0f ) );

		// Test negative wheel delta
		auto wheelDown = ViewportUtils::createMouseWheelEvent( -240.0f, 100.0f, 200.0f );
		REQUIRE( wheelDown.mouse.wheelDelta == Catch::Approx( -240.0f ) );
		REQUIRE( wheelDown.mouse.x == Catch::Approx( 100.0f ) );
		REQUIRE( wheelDown.mouse.y == Catch::Approx( 200.0f ) );
	}
}

TEST_CASE( "Viewport Display Name Functionality", "[viewport][utils][display]" )
{
	SECTION( "Individual viewport display names" )
	{
		Viewport perspectiveViewport( ViewportType::Perspective );
		Viewport topViewport( ViewportType::Top );
		Viewport frontViewport( ViewportType::Front );
		Viewport sideViewport( ViewportType::Side );

		// Get display names
		const char *perspectiveName = perspectiveViewport.getDisplayName();
		const char *topName = topViewport.getDisplayName();
		const char *frontName = frontViewport.getDisplayName();
		const char *sideName = sideViewport.getDisplayName();

		// Should be non-null and non-empty
		REQUIRE( perspectiveName != nullptr );
		REQUIRE( topName != nullptr );
		REQUIRE( frontName != nullptr );
		REQUIRE( sideName != nullptr );

		REQUIRE( std::string( perspectiveName ).length() > 0 );
		REQUIRE( std::string( topName ).length() > 0 );
		REQUIRE( std::string( frontName ).length() > 0 );
		REQUIRE( std::string( sideName ).length() > 0 );

		// Should be meaningful names
		REQUIRE_THAT( perspectiveName, Catch::Matchers::ContainsSubstring( "Perspective" ) );
		REQUIRE_THAT( topName, Catch::Matchers::ContainsSubstring( "Top" ) );
		REQUIRE_THAT( frontName, Catch::Matchers::ContainsSubstring( "Front" ) );
		REQUIRE_THAT( sideName, Catch::Matchers::ContainsSubstring( "Side" ) );
	}

	SECTION( "Display name consistency" )
	{
		Viewport viewport( ViewportType::Perspective );

		// Multiple calls should return the same name
		const char *name1 = viewport.getDisplayName();
		const char *name2 = viewport.getDisplayName();

		REQUIRE( std::string( name1 ) == std::string( name2 ) );
	}

	SECTION( "Display name uniqueness" )
	{
		Viewport perspective( ViewportType::Perspective );
		Viewport top( ViewportType::Top );
		Viewport front( ViewportType::Front );
		Viewport side( ViewportType::Side );

		// All display names should be unique
		std::set<std::string> displayNames;
		displayNames.insert( perspective.getDisplayName() );
		displayNames.insert( top.getDisplayName() );
		displayNames.insert( front.getDisplayName() );
		displayNames.insert( side.getDisplayName() );

		REQUIRE( displayNames.size() == 4 );
	}
}

TEST_CASE( "Viewport Camera Type Mapping", "[viewport][utils][camera]" )
{
	SECTION( "Camera view type consistency" )
	{
		Viewport perspectiveViewport( ViewportType::Perspective );
		Viewport topViewport( ViewportType::Top );
		Viewport frontViewport( ViewportType::Front );
		Viewport sideViewport( ViewportType::Side );

		// Get camera view types
		auto perspectiveViewType = perspectiveViewport.getCameraViewType();
		auto topViewType = topViewport.getCameraViewType();
		auto frontViewType = frontViewport.getCameraViewType();
		auto sideViewType = sideViewport.getCameraViewType();

		// Perspective should have different camera view type than orthographic
		REQUIRE( perspectiveViewType != topViewType );
		REQUIRE( perspectiveViewType != frontViewType );
		REQUIRE( perspectiveViewType != sideViewType );

		// Orthographic views should be different from each other
		REQUIRE( topViewType != frontViewType );
		REQUIRE( topViewType != sideViewType );
		REQUIRE( frontViewType != sideViewType );
	}

	SECTION( "Camera view type stability" )
	{
		Viewport viewport( ViewportType::Top );

		// Camera view type should be consistent
		auto viewType1 = viewport.getCameraViewType();
		auto viewType2 = viewport.getCameraViewType();

		REQUIRE( viewType1 == viewType2 );
	}
}

TEST_CASE( "ViewportUtils Input Event Edge Cases", "[viewport][utils][input][edge]" )
{
	SECTION( "Extreme coordinate values" )
	{
		// Very large coordinates
		auto largeEvent = ViewportUtils::createMouseMoveEvent( 1e10f, -1e10f, 1e5f, -1e5f );
		REQUIRE( largeEvent.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( largeEvent.mouse.x == Catch::Approx( 1e10f ).margin( 1e6f ) );
		REQUIRE( largeEvent.mouse.y == Catch::Approx( -1e10f ).margin( 1e6f ) );

		// Very small coordinates
		auto smallEvent = ViewportUtils::createMouseMoveEvent( 1e-10f, -1e-10f, 1e-5f, -1e-5f );
		REQUIRE( smallEvent.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( smallEvent.mouse.x == Catch::Approx( 1e-10f ).margin( 1e-12f ) );
		REQUIRE( smallEvent.mouse.y == Catch::Approx( -1e-10f ).margin( 1e-12f ) );
	}

	SECTION( "Button index edge cases" )
	{
		// Negative button index
		auto negativeButton = ViewportUtils::createMouseButtonEvent( -1, true, 0.0f, 0.0f );
		REQUIRE( negativeButton.mouse.button == -1 );
		REQUIRE( negativeButton.mouse.pressed == true );

		// Very large button index
		auto largeButton = ViewportUtils::createMouseButtonEvent( 999, false, 0.0f, 0.0f );
		REQUIRE( largeButton.mouse.button == 999 );
		REQUIRE( largeButton.mouse.pressed == false );
	}

	SECTION( "Extreme wheel delta values" )
	{
		// Very large positive wheel delta
		auto largeWheel = ViewportUtils::createMouseWheelEvent( 1e6f, 0.0f, 0.0f );
		REQUIRE( largeWheel.mouse.wheelDelta == Catch::Approx( 1e6f ).margin( 1000.0f ) );

		// Very large negative wheel delta
		auto negativeWheel = ViewportUtils::createMouseWheelEvent( -1e6f, 0.0f, 0.0f );
		REQUIRE( negativeWheel.mouse.wheelDelta == Catch::Approx( -1e6f ).margin( 1000.0f ) );

		// Zero wheel delta
		auto zeroWheel = ViewportUtils::createMouseWheelEvent( 0.0f, 100.0f, 200.0f );
		REQUIRE( zeroWheel.mouse.wheelDelta == Catch::Approx( 0.0f ) );
		REQUIRE( zeroWheel.mouse.x == Catch::Approx( 100.0f ) );
		REQUIRE( zeroWheel.mouse.y == Catch::Approx( 200.0f ) );
	}
}

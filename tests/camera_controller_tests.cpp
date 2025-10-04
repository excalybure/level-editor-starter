// camera::Camera Controller Tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "engine/camera/camera_controller.h"
#include "engine/camera/camera.h"
#include "math/vec.h"
#include "math/math.h"

using Catch::Approx;


constexpr float EPSILON = 0.001f;

// Helper function to create test input state
camera::InputState createTestInput()
{
	camera::InputState input;
	input.deltaTime = 0.016f; // 60 FPS
	return input;
}

TEST_CASE( "Camera Controller Creation", "[camera][controller]" )
{
	SECTION( "Create controller for perspective camera" )
	{
		auto perspCamera = std::make_unique<camera::PerspectiveCamera>();
		auto controller = camera::ControllerFactory::createController( perspCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->getCamera() == perspCamera.get() );
		REQUIRE( controller->isEnabled() );
	}

	SECTION( "Create controller for orthographic camera" )
	{
		auto orthoCamera = std::make_unique<camera::OrthographicCamera>();
		auto controller = camera::ControllerFactory::createController( orthoCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->getCamera() == orthoCamera.get() );
		REQUIRE( controller->isEnabled() );
	}

	SECTION( "Create specific perspective controller" )
	{
		auto perspCamera = std::make_unique<camera::PerspectiveCamera>();
		auto controller = camera::ControllerFactory::createPerspectiveController( perspCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->getCamera() == perspCamera.get() );
	}

	SECTION( "Create specific orthographic controller" )
	{
		auto orthoCamera = std::make_unique<camera::OrthographicCamera>();
		auto controller = camera::ControllerFactory::createOrthographicController( orthoCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->getCamera() == orthoCamera.get() );
	}

	SECTION( "Null camera returns null controller" )
	{
		auto controller = camera::ControllerFactory::createController( nullptr );
		REQUIRE( controller == nullptr );

		auto perspController = camera::ControllerFactory::createPerspectiveController( nullptr );
		REQUIRE( perspController == nullptr );

		auto orthoController = camera::ControllerFactory::createOrthographicController( nullptr );
		REQUIRE( orthoController == nullptr );
	}
}

TEST_CASE( "Perspective Camera Controller", "[camera][controller][perspective]" )
{
	auto camera = std::make_unique<camera::PerspectiveCamera>();
	auto controller = std::make_unique<camera::PerspectiveCameraController>( camera.get() );

	SECTION( "Controller initialization" )
	{
		REQUIRE( controller->getOrbitSensitivity() == Catch::Approx( 0.5f ) );
		REQUIRE( controller->getPanSensitivity() == Catch::Approx( 1.0f ) );
		REQUIRE( controller->getZoomSensitivity() == Catch::Approx( 1.0f ) );
		REQUIRE( controller->getKeyboardMoveSpeed() == Catch::Approx( 10.0f ) );
		REQUIRE_FALSE( controller->getAutoRotate() );
	}

	SECTION( "Sensitivity settings" )
	{
		controller->setOrbitSensitivity( 2.0f );
		controller->setPanSensitivity( 1.5f );
		controller->setZoomSensitivity( 0.8f );
		controller->setKeyboardMoveSpeed( 20.0f );

		REQUIRE( controller->getOrbitSensitivity() == Catch::Approx( 2.0f ) );
		REQUIRE( controller->getPanSensitivity() == Catch::Approx( 1.5f ) );
		REQUIRE( controller->getZoomSensitivity() == Catch::Approx( 0.8f ) );
		REQUIRE( controller->getKeyboardMoveSpeed() == Catch::Approx( 20.0f ) );
	}

	SECTION( "Auto rotation" )
	{
		controller->setAutoRotate( true );
		controller->setAutoRotateSpeed( 45.0f );

		REQUIRE( controller->getAutoRotate() );

		// Test auto rotation by updating with time
		auto input = createTestInput();
		input.deltaTime = 1.0f; // 1 second

		const auto initialPos = camera->getPosition();
		controller->update( input );
		const auto newPos = camera->getPosition();
		const auto newTarget = camera->getTarget();
		// Position should have changed due to auto rotation
		REQUIRE_FALSE( math::approxEqual( initialPos, newPos ) );
	}

	SECTION( "Mouse orbit input" )
	{
		auto input = createTestInput();
		input.mouse.leftButton = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;
		const auto initialTarget = camera->getTarget();

		// First update establishes drag state
		controller->update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f; // Move mouse right
		input.mouse.y = 90.0f;	// Move mouse up
		controller->update( input );

		// camera::Camera should have orbited (position changed, target same)
		REQUIRE( math::approxEqual( camera->getTarget(), initialTarget ) );
		// Position should have changed due to orbit
		// (Exact values depend on orbit implementation)
	}

	SECTION( "Mouse pan input with shift" )
	{
		auto input = createTestInput();
		input.mouse.leftButton = true;
		input.keyboard.shift = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;
		const auto initialDistance = camera->getDistance();

		// First update establishes drag state
		controller->update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f;
		input.mouse.y = 110.0f;
		controller->update( input );

		// Distance should remain approximately the same for pan
		REQUIRE( camera->getDistance() == Catch::Approx( initialDistance ).margin( 0.1f ) );
	}

	SECTION( "Mouse wheel zoom" )
	{
		auto input = createTestInput();
		input.mouse.wheelDelta = 1.0f; // Zoom in
		const auto initialDistance = camera->getDistance();
		controller->update( input );

		// Distance should have decreased (zoomed in)
		REQUIRE( camera->getDistance() < initialDistance );

		// Test zoom out
		input.mouse.wheelDelta = -1.0f;
		const auto afterZoomInDistance = camera->getDistance();
		controller->update( input );

		// Distance should have increased (zoomed out)
		REQUIRE( camera->getDistance() > afterZoomInDistance );
	}

	SECTION( "Keyboard WASD movement" )
	{
		auto input = createTestInput();
		input.keyboard.w = true; // Move forward

		const auto initialPos = camera->getPosition();
		const auto initialTarget = camera->getTarget();

		controller->update( input );

		const auto newPos = camera->getPosition();
		const auto newTarget = camera->getTarget();

		// Both position and target should have moved forward
		const auto movement = newPos - initialPos;
		const auto targetMovement = newTarget - initialTarget;

		REQUIRE( math::approxEqual( movement, targetMovement ) );
		REQUIRE( length( movement ) > 0.001f );
	}

	SECTION( "Focus functionality" )
	{
		const math::Vec3<> targetPoint{ 5.0f, 5.0f, 5.0f };
		const float targetDistance = 15.0f;

		controller->focusOnPoint( targetPoint, targetDistance );

		// Since focusing is animated, we need to simulate time passing
		auto input = createTestInput();
		for ( int i = 0; i < 100; ++i ) // Simulate 1.6 seconds
		{
			controller->update( input );
		}

		// camera::Camera should be looking at the target point
		REQUIRE( math::approxEqual( camera->getTarget(), targetPoint, 0.1f ) );
		REQUIRE( camera->getDistance() == Catch::Approx( targetDistance ).margin( 0.1f ) );
	}

	SECTION( "Controller enable/disable" )
	{
		const auto initialPos = camera->getPosition();

		controller->setEnabled( false );
		REQUIRE_FALSE( controller->isEnabled() );

		// Input should be ignored when disabled
		auto input = createTestInput();
		input.keyboard.w = true;
		controller->update( input );

		REQUIRE( math::approxEqual( camera->getPosition(), initialPos ) );

		// Re-enable and test
		controller->setEnabled( true );
		controller->update( input );

		REQUIRE_FALSE( math::approxEqual( camera->getPosition(), initialPos ) );
	}
}

TEST_CASE( "Orthographic Camera Controller", "[camera][controller][orthographic]" )
{
	auto camera = std::make_unique<camera::OrthographicCamera>();
	auto controller = std::make_unique<camera::OrthographicCameraController>( camera.get() );

	{
		REQUIRE( controller->getPanSensitivity() == Catch::Approx( 1.0f ) );
		REQUIRE( controller->getZoomSensitivity() == Catch::Approx( 1.0f ) );
		REQUIRE( controller->getMinZoom() == Catch::Approx( 0.1f ) );
		REQUIRE( controller->getMaxZoom() == Catch::Approx( 1000.0f ) );
	}

	SECTION( "Sensitivity and zoom limit settings" )
	{
		controller->setPanSensitivity( 2.0f );
		controller->setZoomSensitivity( 1.5f );
		controller->setZoomLimits( 0.5f, 500.0f );

		REQUIRE( controller->getPanSensitivity() == Catch::Approx( 2.0f ) );
		REQUIRE( controller->getZoomSensitivity() == Catch::Approx( 1.5f ) );
		REQUIRE( controller->getMinZoom() == Catch::Approx( 0.5f ) );
		REQUIRE( controller->getMaxZoom() == Catch::Approx( 500.0f ) );
	}

	SECTION( "Mouse pan input" )
	{
		auto input = createTestInput();
		input.mouse.leftButton = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;
		const auto initialPos = camera->getPosition();

		// First update establishes drag state
		controller->update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f;
		input.mouse.y = 110.0f;
		controller->update( input );

		// Position should have changed due to panning
		REQUIRE_FALSE( math::approxEqual( camera->getPosition(), initialPos ) );
	}

	SECTION( "Mouse wheel zoom" )
	{
		auto input = createTestInput();
		input.mouse.wheelDelta = 1.0f; // Zoom in
		const auto initialSize = camera->getOrthographicSize();
		controller->update( input );

		// Size should have decreased (zoomed in)
		REQUIRE( camera->getOrthographicSize() < initialSize );

		// Test zoom out
		input.mouse.wheelDelta = -1.0f;
		const auto afterZoomInSize = camera->getOrthographicSize();
		controller->update( input );

		// Size should have increased (zoomed out)
		REQUIRE( camera->getOrthographicSize() > afterZoomInSize );
	}

	SECTION( "Zoom limits enforcement" )
	{
		controller->setZoomLimits( 5.0f, 20.0f );

		// Set camera to minimum zoom
		camera->setOrthographicSize( 5.0f );

		// Try to zoom in further
		auto input = createTestInput();
		input.mouse.wheelDelta = 10.0f; // Large zoom in
		controller->update( input );

		// Should be clamped to minimum
		REQUIRE( camera->getOrthographicSize() >= 5.0f );

		// Set camera to maximum zoom
		camera->setOrthographicSize( 20.0f );

		// Try to zoom out further
		input.mouse.wheelDelta = -10.0f; // Large zoom out
		controller->update( input );

		// Should be clamped to maximum
		REQUIRE( camera->getOrthographicSize() <= 20.0f );
	}

	SECTION( "Frame bounds" )
	{
		const math::Vec3<> center{ 10.0f, 10.0f, 0.0f };
		const math::Vec3<> size{ 4.0f, 6.0f, 2.0f };

		controller->frameBounds( center, size );

		// camera::Camera should be positioned to frame the bounds
		// (Exact behavior depends on frameBounds implementation)
		REQUIRE( math::length( camera->getPosition() - center ) > 0.0f );
	}
}

TEST_CASE( "Input Utility Functions", "[camera][controller][utils]" )
{
	SECTION( "Screen to NDC conversion" )
	{
		const math::Vec2<> screenSize{ 800.0f, 600.0f };

		// Center of screen should be (0,0) in NDC
		auto ndc = camera::InputUtils::ScreenToNDC( { 400.0f, 300.0f }, screenSize );
		REQUIRE( ndc.x == Catch::Approx( 0.0f ).margin( 0.001f ) );
		REQUIRE( ndc.y == Catch::Approx( 0.0f ).margin( 0.001f ) );

		// Top-left should be (-1, 1)
		ndc = camera::InputUtils::ScreenToNDC( { 0.0f, 0.0f }, screenSize );
		REQUIRE( ndc.x == Catch::Approx( -1.0f ) );
		REQUIRE( ndc.y == Catch::Approx( 1.0f ) );

		// Bottom-right should be (1, -1)
		ndc = camera::InputUtils::ScreenToNDC( { 800.0f, 600.0f }, screenSize );
		REQUIRE( ndc.x == Catch::Approx( 1.0f ) );
		REQUIRE( ndc.y == Catch::Approx( -1.0f ) );
	}

	SECTION( "Distance-based sensitivity" )
	{
		const float baseSpeed = 1.0f;
		const float minSpeed = 0.1f;

		// Close distance should give low sensitivity
		const auto sensitivity1 = camera::InputUtils::CalculateDistanceBasedSensitivity( baseSpeed, 1.0f, minSpeed );

		// Far distance should give higher sensitivity
		const auto sensitivity2 = camera::InputUtils::CalculateDistanceBasedSensitivity( baseSpeed, 100.0f, minSpeed );

		REQUIRE( sensitivity2 > sensitivity1 );
		REQUIRE( sensitivity1 >= minSpeed );
		REQUIRE( sensitivity2 >= minSpeed );
	}

	SECTION( "Smooth input filtering" )
	{
		const float current = 0.0f;
		const float target = 10.0f;
		const float smoothing = 5.0f;
		const float deltaTime = 0.1f;

		const auto smoothed = camera::InputUtils::SmoothInput( current, target, smoothing, deltaTime );

		// Should be between current and target
		REQUIRE( smoothed > current );
		REQUIRE( smoothed < target );

		// Test Vec2 version
		const math::Vec2<> current2{ 0.0f, 0.0f };
		const math::Vec2<> target2{ 10.0f, 5.0f };

		const auto smoothed2 = camera::InputUtils::SmoothInput( current2, target2, smoothing, deltaTime );

		REQUIRE( smoothed2.x > 0.0f );
		REQUIRE( smoothed2.x < 10.0f );
		REQUIRE( smoothed2.y > 0.0f );
		REQUIRE( smoothed2.y < 5.0f );
	}

	SECTION( "Deadzone processing" )
	{
		const float deadzone = 0.2f;

		// Input below deadzone should be zero
		REQUIRE( camera::InputUtils::ApplyDeadzone( 0.1f, deadzone ) == Catch::Approx( 0.0f ) );
		REQUIRE( camera::InputUtils::ApplyDeadzone( -0.15f, deadzone ) == Catch::Approx( 0.0f ) );

		// Input above deadzone should be scaled
		const auto result = camera::InputUtils::ApplyDeadzone( 0.6f, deadzone );
		REQUIRE( result > 0.0f );
		REQUIRE( result < 0.6f ); // Should be remapped

		// Test Vec2 version
		const math::Vec2<> input{ 0.1f, 0.1f }; // Below deadzone
		const auto result2 = camera::InputUtils::ApplyDeadzone( input, deadzone );
		REQUIRE( math::length( result2 ) == Catch::Approx( 0.0f ) );

		const math::Vec2<> input3{ 0.8f, 0.6f }; // Above deadzone
		const auto result3 = camera::InputUtils::ApplyDeadzone( input3, deadzone );
		REQUIRE( math::length( result3 ) > 0.0f );
		REQUIRE( math::length( result3 ) < ( math::length( input3 ) + 0.001f ) ); // Allow small precision error
	}
}

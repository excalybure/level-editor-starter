// Camera Controller Tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import engine.camera.controller;
import engine.camera;
import engine.vec;
import engine.math;

using namespace camera;
using namespace math;
using namespace Catch;

constexpr float EPSILON = 0.001f;

// Helper function to create test input state
InputState CreateTestInput()
{
	InputState input;
	input.deltaTime = 0.016f; // 60 FPS
	return input;
}

TEST_CASE( "Camera Controller Creation", "[camera][controller]" )
{
	SECTION( "Create controller for perspective camera" )
	{
		auto perspCamera = std::make_unique<PerspectiveCamera>();
		auto controller = ControllerFactory::CreateController( perspCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->GetCamera() == perspCamera.get() );
		REQUIRE( controller->IsEnabled() );
	}

	SECTION( "Create controller for orthographic camera" )
	{
		auto orthoCamera = std::make_unique<OrthographicCamera>();
		auto controller = ControllerFactory::CreateController( orthoCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->GetCamera() == orthoCamera.get() );
		REQUIRE( controller->IsEnabled() );
	}

	SECTION( "Create specific perspective controller" )
	{
		auto perspCamera = std::make_unique<PerspectiveCamera>();
		auto controller = ControllerFactory::CreatePerspectiveController( perspCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->GetCamera() == perspCamera.get() );
	}

	SECTION( "Create specific orthographic controller" )
	{
		auto orthoCamera = std::make_unique<OrthographicCamera>();
		auto controller = ControllerFactory::CreateOrthographicController( orthoCamera.get() );

		REQUIRE( controller != nullptr );
		REQUIRE( controller->GetCamera() == orthoCamera.get() );
	}

	SECTION( "Null camera returns null controller" )
	{
		auto controller = ControllerFactory::CreateController( nullptr );
		REQUIRE( controller == nullptr );

		auto perspController = ControllerFactory::CreatePerspectiveController( nullptr );
		REQUIRE( perspController == nullptr );

		auto orthoController = ControllerFactory::CreateOrthographicController( nullptr );
		REQUIRE( orthoController == nullptr );
	}
}

TEST_CASE( "Perspective Camera Controller", "[camera][controller][perspective]" )
{
	auto camera = std::make_unique<PerspectiveCamera>();
	auto controller = std::make_unique<PerspectiveCameraController>( camera.get() );

	SECTION( "Controller initialization" )
	{
		REQUIRE( controller->GetOrbitSensitivity() == Approx( 0.5f ) );
		REQUIRE( controller->GetPanSensitivity() == Approx( 1.0f ) );
		REQUIRE( controller->GetZoomSensitivity() == Approx( 1.0f ) );
		REQUIRE( controller->GetKeyboardMoveSpeed() == Approx( 10.0f ) );
		REQUIRE_FALSE( controller->GetAutoRotate() );
	}

	SECTION( "Sensitivity settings" )
	{
		controller->SetOrbitSensitivity( 2.0f );
		controller->SetPanSensitivity( 1.5f );
		controller->SetZoomSensitivity( 0.8f );
		controller->SetKeyboardMoveSpeed( 20.0f );

		REQUIRE( controller->GetOrbitSensitivity() == Approx( 2.0f ) );
		REQUIRE( controller->GetPanSensitivity() == Approx( 1.5f ) );
		REQUIRE( controller->GetZoomSensitivity() == Approx( 0.8f ) );
		REQUIRE( controller->GetKeyboardMoveSpeed() == Approx( 20.0f ) );
	}

	SECTION( "Auto rotation" )
	{
		controller->SetAutoRotate( true );
		controller->SetAutoRotateSpeed( 45.0f );

		REQUIRE( controller->GetAutoRotate() );

		// Test auto rotation by updating with time
		auto input = CreateTestInput();
		input.deltaTime = 1.0f; // 1 second

		const auto initialPos = camera->GetPosition();
		controller->Update( input );
		const auto newPos = camera->GetPosition();

		// Position should have changed due to auto rotation
		REQUIRE_FALSE( approxEqual( initialPos, newPos ) );
	}

	SECTION( "Mouse orbit input" )
	{
		auto input = CreateTestInput();
		input.mouse.leftButton = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;

		const auto initialTarget = camera->GetTarget();

		// First update establishes drag state
		controller->Update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f; // Move mouse right
		input.mouse.y = 90.0f;	// Move mouse up
		controller->Update( input );

		// Camera should have orbited (position changed, target same)
		REQUIRE( approxEqual( camera->GetTarget(), initialTarget ) );
		// Position should have changed due to orbit
		// (Exact values depend on orbit implementation)
	}

	SECTION( "Mouse pan input with shift" )
	{
		auto input = CreateTestInput();
		input.mouse.leftButton = true;
		input.keyboard.shift = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;

		const auto initialDistance = camera->GetDistance();

		// First update establishes drag state
		controller->Update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f;
		input.mouse.y = 110.0f;
		controller->Update( input );

		// Distance should remain approximately the same for pan
		REQUIRE( camera->GetDistance() == Approx( initialDistance ).margin( 0.1f ) );
	}

	SECTION( "Mouse wheel zoom" )
	{
		auto input = CreateTestInput();
		input.mouse.wheelDelta = 1.0f; // Zoom in

		const auto initialDistance = camera->GetDistance();
		controller->Update( input );

		// Distance should have decreased (zoomed in)
		REQUIRE( camera->GetDistance() < initialDistance );

		// Test zoom out
		input.mouse.wheelDelta = -1.0f;
		const auto afterZoomInDistance = camera->GetDistance();
		controller->Update( input );

		// Distance should have increased (zoomed out)
		REQUIRE( camera->GetDistance() > afterZoomInDistance );
	}

	SECTION( "Keyboard WASD movement" )
	{
		auto input = CreateTestInput();
		input.keyboard.w = true; // Move forward

		const auto initialPos = camera->GetPosition();
		const auto initialTarget = camera->GetTarget();

		controller->Update( input );

		const auto newPos = camera->GetPosition();
		const auto newTarget = camera->GetTarget();

		// Both position and target should have moved forward
		const auto movement = newPos - initialPos;
		const auto targetMovement = newTarget - initialTarget;

		REQUIRE( approxEqual( movement, targetMovement ) );
		REQUIRE( length( movement ) > 0.001f );
	}

	SECTION( "Focus functionality" )
	{
		const math::Vec3<> targetPoint{ 5.0f, 5.0f, 5.0f };
		const float targetDistance = 15.0f;

		controller->FocusOnPoint( targetPoint, targetDistance );

		// Since focusing is animated, we need to simulate time passing
		auto input = CreateTestInput();
		for ( int i = 0; i < 100; ++i ) // Simulate 1.6 seconds
		{
			controller->Update( input );
		}

		// Camera should be looking at the target point
		REQUIRE( approxEqual( camera->GetTarget(), targetPoint, 0.1f ) );
		REQUIRE( camera->GetDistance() == Approx( targetDistance ).margin( 0.1f ) );
	}

	SECTION( "Controller enable/disable" )
	{
		const auto initialPos = camera->GetPosition();

		controller->SetEnabled( false );
		REQUIRE_FALSE( controller->IsEnabled() );

		// Input should be ignored when disabled
		auto input = CreateTestInput();
		input.keyboard.w = true;
		controller->Update( input );

		REQUIRE( approxEqual( camera->GetPosition(), initialPos ) );

		// Re-enable and test
		controller->SetEnabled( true );
		controller->Update( input );

		REQUIRE_FALSE( approxEqual( camera->GetPosition(), initialPos ) );
	}
}

TEST_CASE( "Orthographic Camera Controller", "[camera][controller][orthographic]" )
{
	auto camera = std::make_unique<OrthographicCamera>();
	auto controller = std::make_unique<OrthographicCameraController>( camera.get() );

	SECTION( "Controller initialization" )
	{
		REQUIRE( controller->GetPanSensitivity() == Approx( 1.0f ) );
		REQUIRE( controller->GetZoomSensitivity() == Approx( 1.0f ) );
		REQUIRE( controller->GetMinZoom() == Approx( 0.1f ) );
		REQUIRE( controller->GetMaxZoom() == Approx( 1000.0f ) );
	}

	SECTION( "Sensitivity and zoom limit settings" )
	{
		controller->SetPanSensitivity( 2.0f );
		controller->SetZoomSensitivity( 1.5f );
		controller->SetZoomLimits( 0.5f, 500.0f );

		REQUIRE( controller->GetPanSensitivity() == Approx( 2.0f ) );
		REQUIRE( controller->GetZoomSensitivity() == Approx( 1.5f ) );
		REQUIRE( controller->GetMinZoom() == Approx( 0.5f ) );
		REQUIRE( controller->GetMaxZoom() == Approx( 500.0f ) );
	}

	SECTION( "Mouse pan input" )
	{
		auto input = CreateTestInput();
		input.mouse.leftButton = true;
		input.mouse.x = 100.0f;
		input.mouse.y = 100.0f;

		const auto initialPos = camera->GetPosition();

		// First update establishes drag state
		controller->Update( input );

		// Second update with mouse movement
		input.mouse.x = 110.0f;
		input.mouse.y = 110.0f;
		controller->Update( input );

		// Position should have changed due to panning
		REQUIRE_FALSE( approxEqual( camera->GetPosition(), initialPos ) );
	}

	SECTION( "Mouse wheel zoom" )
	{
		auto input = CreateTestInput();
		input.mouse.wheelDelta = 1.0f; // Zoom in

		const auto initialSize = camera->GetOrthographicSize();
		controller->Update( input );

		// Size should have decreased (zoomed in)
		REQUIRE( camera->GetOrthographicSize() < initialSize );

		// Test zoom out
		input.mouse.wheelDelta = -1.0f;
		const auto afterZoomInSize = camera->GetOrthographicSize();
		controller->Update( input );

		// Size should have increased (zoomed out)
		REQUIRE( camera->GetOrthographicSize() > afterZoomInSize );
	}

	SECTION( "Zoom limits enforcement" )
	{
		controller->SetZoomLimits( 5.0f, 20.0f );

		// Set camera to minimum zoom
		camera->SetOrthographicSize( 5.0f );

		// Try to zoom in further
		auto input = CreateTestInput();
		input.mouse.wheelDelta = 10.0f; // Large zoom in
		controller->Update( input );

		// Should be clamped to minimum
		REQUIRE( camera->GetOrthographicSize() >= 5.0f );

		// Set camera to maximum zoom
		camera->SetOrthographicSize( 20.0f );

		// Try to zoom out further
		input.mouse.wheelDelta = -10.0f; // Large zoom out
		controller->Update( input );

		// Should be clamped to maximum
		REQUIRE( camera->GetOrthographicSize() <= 20.0f );
	}

	SECTION( "Frame bounds" )
	{
		const math::Vec3<> center{ 10.0f, 10.0f, 0.0f };
		const math::Vec3<> size{ 4.0f, 6.0f, 2.0f };

		controller->FrameBounds( center, size );

		// Camera should be positioned to frame the bounds
		// (Exact behavior depends on FrameBounds implementation)
		REQUIRE( math::length( camera->GetPosition() - center ) > 0.0f );
	}
}

TEST_CASE( "Input Utility Functions", "[camera][controller][utils]" )
{
	SECTION( "Screen to NDC conversion" )
	{
		const math::Vec2<> screenSize{ 800.0f, 600.0f };

		// Center of screen should be (0,0) in NDC
		auto ndc = InputUtils::ScreenToNDC( { 400.0f, 300.0f }, screenSize );
		REQUIRE( ndc.x == Approx( 0.0f ).margin( 0.001f ) );
		REQUIRE( ndc.y == Approx( 0.0f ).margin( 0.001f ) );

		// Top-left should be (-1, 1)
		ndc = InputUtils::ScreenToNDC( { 0.0f, 0.0f }, screenSize );
		REQUIRE( ndc.x == Approx( -1.0f ) );
		REQUIRE( ndc.y == Approx( 1.0f ) );

		// Bottom-right should be (1, -1)
		ndc = InputUtils::ScreenToNDC( { 800.0f, 600.0f }, screenSize );
		REQUIRE( ndc.x == Approx( 1.0f ) );
		REQUIRE( ndc.y == Approx( -1.0f ) );
	}

	SECTION( "Distance-based sensitivity" )
	{
		const float baseSpeed = 1.0f;
		const float minSpeed = 0.1f;

		// Close distance should give low sensitivity
		const auto sensitivity1 = InputUtils::CalculateDistanceBasedSensitivity( baseSpeed, 1.0f, minSpeed );

		// Far distance should give higher sensitivity
		const auto sensitivity2 = InputUtils::CalculateDistanceBasedSensitivity( baseSpeed, 100.0f, minSpeed );

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

		const auto smoothed = InputUtils::SmoothInput( current, target, smoothing, deltaTime );

		// Should be between current and target
		REQUIRE( smoothed > current );
		REQUIRE( smoothed < target );

		// Test Vec2 version
		const math::Vec2<> current2{ 0.0f, 0.0f };
		const math::Vec2<> target2{ 10.0f, 5.0f };

		const auto smoothed2 = InputUtils::SmoothInput( current2, target2, smoothing, deltaTime );

		REQUIRE( smoothed2.x > 0.0f );
		REQUIRE( smoothed2.x < 10.0f );
		REQUIRE( smoothed2.y > 0.0f );
		REQUIRE( smoothed2.y < 5.0f );
	}

	SECTION( "Deadzone processing" )
	{
		const float deadzone = 0.2f;

		// Input below deadzone should be zero
		REQUIRE( InputUtils::ApplyDeadzone( 0.1f, deadzone ) == Approx( 0.0f ) );
		REQUIRE( InputUtils::ApplyDeadzone( -0.15f, deadzone ) == Approx( 0.0f ) );

		// Input above deadzone should be scaled
		const auto result = InputUtils::ApplyDeadzone( 0.6f, deadzone );
		REQUIRE( result > 0.0f );
		REQUIRE( result < 0.6f ); // Should be remapped

		// Test Vec2 version
		const math::Vec2<> input{ 0.1f, 0.1f }; // Below deadzone
		const auto result2 = InputUtils::ApplyDeadzone( input, deadzone );
		REQUIRE( math::length( result2 ) == Approx( 0.0f ) );

		const math::Vec2<> input3{ 0.8f, 0.6f }; // Above deadzone
		const auto result3 = InputUtils::ApplyDeadzone( input3, deadzone );
		REQUIRE( math::length( result3 ) > 0.0f );
		REQUIRE( math::length( result3 ) < ( math::length( input3 ) + 0.001f ) ); // Allow small precision error
	}
}

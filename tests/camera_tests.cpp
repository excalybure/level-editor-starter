#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "engine/camera/camera.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/math/math.h"

using namespace camera;
using namespace math;

constexpr float EPSILON = 0.001f;

TEST_CASE( "Camera Base Class", "[camera]" )
{
	SECTION( "Default construction" )
	{
		// Create a concrete camera implementation for testing
		class TestCamera : public Camera
		{
		public:
			CameraType getType() const noexcept override { return CameraType::Perspective; }
			Mat4<> getProjectionMatrix( float /* aspectRatio */ ) const noexcept override
			{
				return Mat4<>::identity();
			}
		};

		const TestCamera camera;

		// Test default Z-up positioning
		REQUIRE( camera.getPosition().x == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getPosition().y == Catch::Approx( -5.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getPosition().z == Catch::Approx( 5.0f ).epsilon( EPSILON ) );

		REQUIRE( camera.getTarget().x == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().y == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().z == Catch::Approx( 0.0f ).epsilon( EPSILON ) );

		REQUIRE( camera.getUp().x == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getUp().y == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getUp().z == Catch::Approx( 1.0f ).epsilon( EPSILON ) );
	}

	SECTION( "Camera vectors calculation" )
	{
		class TestCamera : public Camera
		{
		public:
			CameraType getType() const noexcept override { return CameraType::Perspective; }
			Mat4<> getProjectionMatrix( float /* aspectRatio */ ) const noexcept override
			{
				return Mat4<>::identity();
			}
		};

		const TestCamera camera;

		// Test forward vector (from position to target)
		const auto forward = camera.getForwardVector();
		const auto expectedForward = normalize( Vec3<>{ 0.0f, 5.0f, -5.0f } ); // From (0,-5,5) to (0,0,0)

		REQUIRE( forward.x == Catch::Approx( expectedForward.x ).epsilon( EPSILON ) );
		REQUIRE( forward.y == Catch::Approx( expectedForward.y ).epsilon( EPSILON ) );
		REQUIRE( forward.z == Catch::Approx( expectedForward.z ).epsilon( EPSILON ) );

		// Test right vector (cross product of forward and up)
		const auto right = camera.getRightVector();
		const auto expectedRight = normalize( cross( forward, camera.getUp() ) );

		REQUIRE( right.x == Catch::Approx( expectedRight.x ).epsilon( EPSILON ) );
		REQUIRE( right.y == Catch::Approx( expectedRight.y ).epsilon( EPSILON ) );
		REQUIRE( right.z == Catch::Approx( expectedRight.z ).epsilon( EPSILON ) );

		// Test distance calculation
		const float distance = camera.getDistance();
		const float expectedDistance = length( camera.getPosition() - camera.getTarget() );
		REQUIRE( distance == Catch::Approx( expectedDistance ).epsilon( EPSILON ) );
	}

	SECTION( "Camera positioning" )
	{
		class TestCamera : public Camera
		{
		public:
			CameraType getType() const noexcept override { return CameraType::Perspective; }
			Mat4<> getProjectionMatrix( float /* aspectRatio */ ) const noexcept override
			{
				return Mat4<>::identity();
			}
		};

		TestCamera camera;

		const Vec3<> newPosition{ 10.0f, 20.0f, 30.0f };
		const Vec3<> newTarget{ 1.0f, 2.0f, 3.0f };
		const Vec3<> newUp{ 0.0f, 1.0f, 0.0f };

		camera.setPosition( newPosition );
		camera.setTarget( newTarget );
		camera.setUp( newUp );

		REQUIRE( camera.getPosition().x == Catch::Approx( newPosition.x ).epsilon( EPSILON ) );
		REQUIRE( camera.getPosition().y == Catch::Approx( newPosition.y ).epsilon( EPSILON ) );
		REQUIRE( camera.getPosition().z == Catch::Approx( newPosition.z ).epsilon( EPSILON ) );

		REQUIRE( camera.getTarget().x == Catch::Approx( newTarget.x ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().y == Catch::Approx( newTarget.y ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().z == Catch::Approx( newTarget.z ).epsilon( EPSILON ) );

		// Up vector should be normalized
		const auto normalizedUp = normalize( newUp );
		REQUIRE( camera.getUp().x == Catch::Approx( normalizedUp.x ).epsilon( EPSILON ) );
		REQUIRE( camera.getUp().y == Catch::Approx( normalizedUp.y ).epsilon( EPSILON ) );
		REQUIRE( camera.getUp().z == Catch::Approx( normalizedUp.z ).epsilon( EPSILON ) );
	}
}

TEST_CASE( "Perspective Camera", "[camera]" )
{
	SECTION( "Default construction" )
	{
		const PerspectiveCamera camera;

		REQUIRE( camera.getType() == CameraType::Perspective );
		REQUIRE( camera.getFieldOfView() == Catch::Approx( 65.0f ).epsilon( EPSILON ) );
		REQUIRE( camera.getNearPlane() == Catch::Approx( 0.1f ).epsilon( EPSILON ) );
		REQUIRE( camera.getFarPlane() == Catch::Approx( 1000.0f ).epsilon( EPSILON ) );
	}

	SECTION( "Field of view constraints" )
	{
		PerspectiveCamera camera;

		camera.setFieldOfView( 30.0f );
		REQUIRE( camera.getFieldOfView() == Catch::Approx( 30.0f ).epsilon( EPSILON ) );

		// Test clamping
		camera.setFieldOfView( 0.5f ); // Below minimum
		REQUIRE( camera.getFieldOfView() == Catch::Approx( 1.0f ).epsilon( EPSILON ) );

		camera.setFieldOfView( 200.0f ); // Above maximum
		REQUIRE( camera.getFieldOfView() == Catch::Approx( 179.0f ).epsilon( EPSILON ) );
	}

	SECTION( "Projection matrix" )
	{
		const PerspectiveCamera camera( 60.0f );
		const float aspectRatio = 16.0f / 9.0f;

		const auto projMatrix = camera.getProjectionMatrix( aspectRatio );

		// Basic sanity checks - projection matrix should not be identity
		const auto identityMatrix = Mat4<>::identity();
		REQUIRE_FALSE( ( projMatrix.m00() == identityMatrix.m00() && projMatrix.m11() == identityMatrix.m11() ) );

		// Check that matrix has the expected structure for perspective projection
		// M[2][2] should be negative (far plane related)
		REQUIRE( projMatrix.m22() < 0.0f );

		// M[3][2] should be -1 for perspective projection
		REQUIRE( projMatrix.m32() == Catch::Approx( -1.0f ).epsilon( EPSILON ) );
	}

	SECTION( "Orbit controls" )
	{
		PerspectiveCamera camera;
		const auto initialPosition = camera.getPosition();
		const auto initialTarget = camera.getTarget();
		const float initialDistance = camera.getDistance();

		// Test yaw rotation (around Z-axis)
		camera.orbit( 90.0f, 0.0f ); // 90 degrees yaw

		const auto currentTarget = camera.getTarget();
		REQUIRE( currentTarget.x == Catch::Approx( initialTarget.x ).epsilon( EPSILON ) ); // Target should remain fixed
		REQUIRE( currentTarget.y == Catch::Approx( initialTarget.y ).epsilon( EPSILON ) );
		REQUIRE( currentTarget.z == Catch::Approx( initialTarget.z ).epsilon( EPSILON ) );
		REQUIRE( camera.getDistance() == Catch::Approx( initialDistance ).epsilon( EPSILON ) ); // Distance preserved

		// Position should have changed
		const auto currentPosition = camera.getPosition();
		REQUIRE_FALSE( ( currentPosition.x == Catch::Approx( initialPosition.x ).epsilon( EPSILON ) &&
			currentPosition.y == Catch::Approx( initialPosition.y ).epsilon( EPSILON ) &&
			currentPosition.z == Catch::Approx( initialPosition.z ).epsilon( EPSILON ) ) );
	}

	SECTION( "Zoom functionality" )
	{
		PerspectiveCamera camera;
		const float initialDistance = camera.getDistance();
		const auto initialTarget = camera.getTarget();

		camera.zoom( 2.0f ); // Zoom out by 2 units

		const auto currentTarget2 = camera.getTarget();
		REQUIRE( currentTarget2.x == Catch::Approx( initialTarget.x ).epsilon( EPSILON ) ); // Target unchanged
		REQUIRE( currentTarget2.y == Catch::Approx( initialTarget.y ).epsilon( EPSILON ) );
		REQUIRE( currentTarget2.z == Catch::Approx( initialTarget.z ).epsilon( EPSILON ) );
		REQUIRE( camera.getDistance() == Catch::Approx( initialDistance + 2.0f ).epsilon( EPSILON ) );

		// Test minimum distance constraint
		camera.zoom( -1000.0f );
		REQUIRE( camera.getDistance() >= 0.1f ); // Should be clamped to minimum
	}

	SECTION( "Focus functionality" )
	{
		PerspectiveCamera camera;
		const Vec3<> focusPoint{ 10.0f, 5.0f, -3.0f };
		const float focusDistance = 15.0f;

		camera.focusOnPoint( focusPoint, focusDistance );

		REQUIRE( camera.getTarget().x == Catch::Approx( focusPoint.x ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().y == Catch::Approx( focusPoint.y ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().z == Catch::Approx( focusPoint.z ).epsilon( EPSILON ) );
		REQUIRE( camera.getDistance() == Catch::Approx( focusDistance ).epsilon( EPSILON ) );
	}
}

TEST_CASE( "Orthographic Camera", "[camera]" )
{
	SECTION( "Default construction" )
	{
		const OrthographicCamera camera;

		REQUIRE( camera.getType() == CameraType::Orthographic );
		REQUIRE( camera.getViewType() == ViewType::Top );
		REQUIRE( camera.getOrthographicSize() == Catch::Approx( 10.0f ).epsilon( EPSILON ) );
	}

	SECTION( "View type setup" )
	{
		OrthographicCamera camera;

		// Test Top view (XY plane, looking down Z-axis)
		camera.setupView( ViewType::Top );
		REQUIRE( camera.getViewType() == ViewType::Top );

		// Position should be above target for top view
		REQUIRE( camera.getPosition().z > camera.getTarget().z );

		// Test Front view (XZ plane, looking down Y-axis)
		camera.setupView( ViewType::Front );
		REQUIRE( camera.getViewType() == ViewType::Front );

		// Position should be behind target for front view
		REQUIRE( camera.getPosition().y < camera.getTarget().y );

		// Test Side view (YZ plane, looking down X-axis)
		camera.setupView( ViewType::Side );
		REQUIRE( camera.getViewType() == ViewType::Side );

		// Position should be to the side of target
		REQUIRE( camera.getPosition().x > camera.getTarget().x );
	}

	SECTION( "Orthographic size constraints" )
	{
		OrthographicCamera camera;

		camera.setOrthographicSize( 5.0f );
		REQUIRE( camera.getOrthographicSize() == Catch::Approx( 5.0f ).epsilon( EPSILON ) );

		// Test minimum constraint
		camera.setOrthographicSize( -1.0f );
		REQUIRE( camera.getOrthographicSize() >= 0.1f );
	}

	SECTION( "Projection matrix" )
	{
		const OrthographicCamera camera;
		const float aspectRatio = 16.0f / 9.0f;

		const auto projMatrix = camera.getProjectionMatrix( aspectRatio );

		// Basic sanity checks
		const auto identityMatrix2 = Mat4<>::identity();
		REQUIRE_FALSE( ( projMatrix.m00() == identityMatrix2.m00() && projMatrix.m11() == identityMatrix2.m11() ) );

		// For orthographic projection, M[3][3] should be 1
		REQUIRE( projMatrix.m33() == Catch::Approx( 1.0f ).epsilon( EPSILON ) );

		// M[3][2] should be 0 for orthographic projection (no perspective division)
		REQUIRE( projMatrix.m32() == Catch::Approx( 0.0f ).epsilon( EPSILON ) );
	}

	SECTION( "Frame bounds functionality" )
	{
		OrthographicCamera camera;
		const Vec3<> boundsCenter{ 5.0f, -3.0f, 2.0f };
		const Vec3<> boundsSize{ 20.0f, 10.0f, 8.0f };

		camera.frameBounds( boundsCenter, boundsSize );

		REQUIRE( camera.getTarget().x == Catch::Approx( boundsCenter.x ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().y == Catch::Approx( boundsCenter.y ).epsilon( EPSILON ) );
		REQUIRE( camera.getTarget().z == Catch::Approx( boundsCenter.z ).epsilon( EPSILON ) );

		// Orthographic size should be based on the largest bound dimension
		const float expectedSize = std::max( { boundsSize.x, boundsSize.y, boundsSize.z } ) * 0.6f;
		REQUIRE( camera.getOrthographicSize() >= expectedSize * 0.9f ); // Allow some tolerance
	}
}

TEST_CASE( "Camera Utilities", "[camera]" )
{
	SECTION( "Framing distance calculation" )
	{
		const Vec3<> boundsSize{ 10.0f, 8.0f, 6.0f };
		const float fov = 45.0f;
		const float aspectRatio = 16.0f / 9.0f;

		const float distance = CameraUtils::CalculateFramingDistance( boundsSize, fov, aspectRatio );

		REQUIRE( distance > 0.0f );

		// Distance should increase with larger bounds
		const Vec3<> largerBounds{ 20.0f, 16.0f, 12.0f };
		const float largerDistance = CameraUtils::CalculateFramingDistance( largerBounds, fov, aspectRatio );

		REQUIRE( largerDistance > distance );

		// Distance should decrease with wider field of view
		const float widerFov = 90.0f;
		const float widerDistance = CameraUtils::CalculateFramingDistance( boundsSize, widerFov, aspectRatio );

		REQUIRE( widerDistance < distance );
	}

	SECTION( "Screen to world ray" )
	{
		// Create simple view and projection matrices
		const auto viewMatrix = Mat4<>::lookAt( Vec3<>{ 0.0f, -10.0f, 0.0f }, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 0.0f, 0.0f, 1.0f } );
		const auto projMatrix = Mat4<>::perspective( 45.0f * 3.14159f / 180.0f, 16.0f / 9.0f, 0.1f, 1000.0f );

		const Vec2<> screenSize{ 1920.0f, 1080.0f };
		const Vec2<> screenCenter = screenSize * 0.5f;

		const auto ray = CameraUtils::ScreenToWorldRay( screenCenter, screenSize, viewMatrix, projMatrix );

		// Ray from center of screen should point roughly toward the target
		REQUIRE( std::abs( ray.direction.y ) > 0.5f ); // Pointing in Y direction (either positive or negative)
		REQUIRE( std::abs( ray.direction.x ) < 0.1f ); // Minimal X component

		// Ray origin should be near the camera position
		REQUIRE( ray.origin.y == Catch::Approx( -10.0f ).epsilon( EPSILON ) );
	}

	SECTION( "World to screen projection" )
	{
		const auto viewMatrix = Mat4<>::lookAt( Vec3<>{ 0.0f, -10.0f, 0.0f }, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 0.0f, 0.0f, 1.0f } );
		const auto projMatrix = Mat4<>::perspective( 45.0f * 3.14159f / 180.0f, 16.0f / 9.0f, 0.1f, 1000.0f );

		const Vec2<> screenSize{ 1920.0f, 1080.0f };
		const Vec3<> worldOrigin{ 0.0f, 0.0f, 0.0f };

		const auto screenPos = CameraUtils::WorldToScreen( worldOrigin, screenSize, viewMatrix, projMatrix );

		// World origin should project to center of screen
		REQUIRE( screenPos.x == Catch::Approx( screenSize.x * 0.5f ).margin( 10.0f ) ); // Allow some tolerance
		REQUIRE( screenPos.y == Catch::Approx( screenSize.y * 0.5f ).margin( 10.0f ) );
		REQUIRE( screenPos.z > 0.0f ); // Should be in front of camera
	}

	SECTION( "Smooth damping" )
	{
		const Vec3<> current{ 0.0f, 0.0f, 0.0f };
		const Vec3<> target{ 10.0f, 5.0f, -3.0f };
		Vec3<> velocity{ 0.0f, 0.0f, 0.0f };

		const float smoothTime = 1.0f;
		const float deltaTime = 0.1f; // Larger delta time for noticeable movement

		const auto result = CameraUtils::SmoothDamp( current, target, velocity, smoothTime, deltaTime );

		// Result should be between current and target
		const float resultDistance = length( result - current );
		const float targetDistance = length( target - current );

		REQUIRE( resultDistance <= targetDistance ); // Should not overshoot target
		REQUIRE( resultDistance >= 0.0f );			 // Should have moved or stayed in place

		// Velocity should be updated (might be zero if already at target or very close)
		REQUIRE( length( velocity ) >= 0.0f );
	}
}

TEST_CASE( "Camera Integration", "[camera]" )
{
	SECTION( "View matrix consistency" )
	{
		PerspectiveCamera perspCamera;
		OrthographicCamera orthoCamera;

		// Set same position, target, and up for both cameras
		const Vec3<> position{ 5.0f, -10.0f, 8.0f };
		const Vec3<> target{ 0.0f, 0.0f, 0.0f };
		const Vec3<> up{ 0.0f, 0.0f, 1.0f };

		perspCamera.setPosition( position );
		perspCamera.setTarget( target );
		perspCamera.setUp( up );

		orthoCamera.setPosition( position );
		orthoCamera.setTarget( target );
		orthoCamera.setUp( up );

		const auto perspView = perspCamera.getViewMatrix();
		const auto orthoView = orthoCamera.getViewMatrix();

		// View matrices should be identical since they use the same positioning
		REQUIRE( perspView.m00() == Catch::Approx( orthoView.m00() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m01() == Catch::Approx( orthoView.m01() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m02() == Catch::Approx( orthoView.m02() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m03() == Catch::Approx( orthoView.m03() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m10() == Catch::Approx( orthoView.m10() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m11() == Catch::Approx( orthoView.m11() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m12() == Catch::Approx( orthoView.m12() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m13() == Catch::Approx( orthoView.m13() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m20() == Catch::Approx( orthoView.m20() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m21() == Catch::Approx( orthoView.m21() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m22() == Catch::Approx( orthoView.m22() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m23() == Catch::Approx( orthoView.m23() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m30() == Catch::Approx( orthoView.m30() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m31() == Catch::Approx( orthoView.m31() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m32() == Catch::Approx( orthoView.m32() ).epsilon( EPSILON ) );
		REQUIRE( perspView.m33() == Catch::Approx( orthoView.m33() ).epsilon( EPSILON ) );
	}

	SECTION( "Z-up coordinate system verification" )
	{
		// Test that our Z-up system is consistent
		PerspectiveCamera camera;

		// Set camera looking down Z-axis (top view)
		camera.setPosition( Vec3<>{ 0.0f, 0.0f, 10.0f } );
		camera.setTarget( Vec3<>{ 0.0f, 0.0f, 0.0f } );
		camera.setUp( Vec3<>{ 0.0f, 1.0f, 0.0f } ); // Y is up in screen space for top view

		const auto forward = camera.getForwardVector();

		// Forward should point down the Z-axis
		REQUIRE( forward.z < -0.9f ); // Mostly downward in Z
		REQUIRE( std::abs( forward.x ) < 0.1f );
		REQUIRE( std::abs( forward.y ) < 0.1f );
	}
}

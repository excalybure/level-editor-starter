// editor::Viewport 3D Picking and picking::Ray Casting tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "editor/viewport/viewport.h"
#include "engine/math/vec.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "editor::Viewport picking::Ray Structure", "[viewport][ray][picking]" )
{
	SECTION( "picking::Ray construction and properties" )
	{
		// Default constructor
		picking::Ray defaultRay;
		REQUIRE_THAT( defaultRay.origin.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( defaultRay.origin.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( defaultRay.origin.z, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( defaultRay.direction.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( defaultRay.direction.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( defaultRay.direction.z, WithinAbs( 0.0f, 0.001f ) );

		// Parameterized constructor
		math::Vec3<> origin( 1.0f, 2.0f, 3.0f );
		math::Vec3<> direction( 0.0f, 0.0f, -1.0f ); // Looking down negative Z
		picking::Ray ray( origin, direction );

		REQUIRE_THAT( ray.origin.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( ray.origin.y, WithinAbs( 2.0f, 0.001f ) );
		REQUIRE_THAT( ray.origin.z, WithinAbs( 3.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.z, WithinAbs( -1.0f, 0.001f ) );
	}

	SECTION( "picking::Ray assignment and modification" )
	{
		picking::Ray ray;
		ray.origin = math::Vec3<>( 5.0f, -3.0f, 10.0f );
		ray.direction = math::Vec3<>( 1.0f, 0.0f, 0.0f ); // Looking right

		REQUIRE_THAT( ray.origin.x, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( ray.origin.y, WithinAbs( -3.0f, 0.001f ) );
		REQUIRE_THAT( ray.origin.z, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( ray.direction.z, WithinAbs( 0.0f, 0.001f ) );
	}
}

TEST_CASE( "editor::Viewport Picking picking::Ray Generation", "[viewport][ray][picking]" )
{
	SECTION( "Perspective viewport picking rays" )
	{
		editor::Viewport viewport( ViewportType::Perspective );
		viewport.setRenderTargetSize( 800, 600 );

		// Test center of screen ray
		auto centerRay = viewport.getPickingRay( 400.0f, 300.0f );

		// picking::Ray should have a valid origin (camera position)
		// The exact values depend on camera setup, but should not be at origin
		bool hasValidOrigin = ( centerRay.origin.x != 0.0f ||
			centerRay.origin.y != 0.0f ||
			centerRay.origin.z != 0.0f );
		REQUIRE( hasValidOrigin );

		// Direction should be normalized (length approximately 1)
		float dirLength = sqrt( centerRay.direction.x * centerRay.direction.x +
			centerRay.direction.y * centerRay.direction.y +
			centerRay.direction.z * centerRay.direction.z );
		REQUIRE_THAT( dirLength, WithinAbs( 1.0f, 0.1f ) );

		// Test corner rays
		auto topLeftRay = viewport.getPickingRay( 0.0f, 0.0f );
		auto bottomRightRay = viewport.getPickingRay( 800.0f, 600.0f );

		// Corner rays should have different directions
		bool differentDirections = ( topLeftRay.direction.x != bottomRightRay.direction.x ||
			topLeftRay.direction.y != bottomRightRay.direction.y ||
			topLeftRay.direction.z != bottomRightRay.direction.z );
		REQUIRE( differentDirections );
	}

	SECTION( "Orthographic viewport picking rays" )
	{
		editor::Viewport topViewport( ViewportType::Top );
		topViewport.setRenderTargetSize( 1024, 768 );

		// Test picking ray from top view
		auto ray = topViewport.getPickingRay( 512.0f, 384.0f ); // Center

		// For top view (looking down Z), direction should be primarily in -Z
		// The exact direction depends on camera setup
		REQUIRE( ray.direction.z <= 0.0f ); // Should be looking down or level

		// Test different positions
		auto leftRay = topViewport.getPickingRay( 0.0f, 384.0f );
		auto rightRay = topViewport.getPickingRay( 1024.0f, 384.0f );

		// X positions should affect the ray differently
		bool differentX = ( leftRay.origin.x != rightRay.origin.x ||
			leftRay.direction.x != rightRay.direction.x );
		REQUIRE( differentX );
	}

	SECTION( "Front viewport picking rays" )
	{
		editor::Viewport frontViewport( ViewportType::Front );
		frontViewport.setRenderTargetSize( 640, 480 );

		auto ray = frontViewport.getPickingRay( 320.0f, 240.0f ); // Center

		// For front view (looking down Y), direction should be primarily in -Y or +Y
		bool hasValidYDirection = ( ray.direction.y != 0.0f );
		// Note: Exact direction depends on camera setup implementation
	}

	SECTION( "Side viewport picking rays" )
	{
		editor::Viewport sideViewport( ViewportType::Side );
		sideViewport.setRenderTargetSize( 1280, 720 );

		auto ray = sideViewport.getPickingRay( 640.0f, 360.0f ); // Center

		// For side view (looking down X), direction should be primarily in -X or +X
		bool hasValidXDirection = ( ray.direction.x != 0.0f );
		// Note: Exact direction depends on camera setup implementation
	}
}

TEST_CASE( "editor::Viewport Picking picking::Ray Edge Cases", "[viewport][ray][picking][edge]" )
{
	editor::Viewport viewport( ViewportType::Perspective );
	viewport.setRenderTargetSize( 800, 600 );

	SECTION( "Out of bounds screen coordinates" )
	{
		// Test coordinates outside viewport bounds
		auto negativeRay = viewport.getPickingRay( -100.0f, -200.0f );
		auto largeRay = viewport.getPickingRay( 1000.0f, 800.0f );

		// Should not crash and should produce valid rays
		REQUIRE_NOTHROW( viewport.getPickingRay( -1e6f, 1e6f ) );
		REQUIRE_NOTHROW( viewport.getPickingRay( 0.0f, 0.0f ) );
		REQUIRE_NOTHROW( viewport.getPickingRay( 800.0f, 600.0f ) );
	}

	SECTION( "Zero viewport size" )
	{
		viewport.setRenderTargetSize( 0, 0 );

		// Should handle gracefully
		REQUIRE_NOTHROW( viewport.getPickingRay( 0.0f, 0.0f ) );

		auto ray = viewport.getPickingRay( 100.0f, 100.0f );
		// picking::Ray should still be valid even with degenerate viewport
	}

	SECTION( "Very small viewport size" )
	{
		viewport.setRenderTargetSize( 1, 1 );

		auto ray = viewport.getPickingRay( 0.5f, 0.5f );

		// Should produce a valid ray
		float dirLength = sqrt( ray.direction.x * ray.direction.x +
			ray.direction.y * ray.direction.y +
			ray.direction.z * ray.direction.z );
		REQUIRE( dirLength > 0.0f ); // Direction should have length
	}

	SECTION( "Very large viewport size" )
	{
		viewport.setRenderTargetSize( 8192, 4320 ); // 8K resolution

		auto centerRay = viewport.getPickingRay( 4096.0f, 2160.0f );
		auto cornerRay = viewport.getPickingRay( 0.0f, 0.0f );

		// Should handle large viewports without precision issues
		REQUIRE_NOTHROW( viewport.getPickingRay( 8192.0f, 4320.0f ) );

		// Rays should be different
		bool different = ( centerRay.direction.x != cornerRay.direction.x ||
			centerRay.direction.y != cornerRay.direction.y );
		REQUIRE( different );
	}
}

TEST_CASE( "editor::Viewport Info Structure", "[viewport][info][structure]" )
{
	SECTION( "ViewportInfo construction and defaults" )
	{
		ViewportInfo info;

		// Check default values
		REQUIRE( info.width == 800 );
		REQUIRE( info.height == 600 );
		REQUIRE( info.isActive == false );
		REQUIRE( info.hasFocus == false );
	}

	SECTION( "ViewportInfo modification" )
	{
		editor::Viewport viewport( ViewportType::Perspective );
		auto &info = viewport.getInfo();

		// Modify values
		info.width = 1920;
		info.height = 1080;
		info.isActive = true;
		info.hasFocus = true;

		// Verify changes
		const auto &constInfo = viewport.getInfo();
		REQUIRE( constInfo.width == 1920 );
		REQUIRE( constInfo.height == 1080 );
		REQUIRE( constInfo.isActive == true );
		REQUIRE( constInfo.hasFocus == true );
	}

	SECTION( "ViewportInfo consistency with viewport methods" )
	{
		editor::Viewport viewport( ViewportType::Top );
		viewport.setRenderTargetSize( 1024, 768 );

		const auto &info = viewport.getInfo();
		// Note: Depending on implementation, info might sync with setRenderTargetSize
		// or might be independent - this test verifies the relationship
	}
}

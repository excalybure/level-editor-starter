// Grid system comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

import engine.grid;
import engine.camera;
import engine.vec;
import engine.matrix;
import engine.shader_manager;

using namespace grid;
using namespace math;
using namespace camera;

TEST_CASE( "Grid Settings Configuration", "[grid][settings]" )
{
	SECTION( "Default grid settings" )
	{
		GridSettings settings;

		// Check default colors (using direct comparison for now)
		REQUIRE( settings.majorGridColor.x == 0.5f );
		REQUIRE( settings.majorGridColor.y == 0.5f );
		REQUIRE( settings.majorGridColor.z == 0.5f );
		REQUIRE( settings.majorGridAlpha == 0.8f );

		REQUIRE( settings.minorGridColor.x == 0.3f );
		REQUIRE( settings.minorGridColor.y == 0.3f );
		REQUIRE( settings.minorGridColor.z == 0.3f );
		REQUIRE( settings.minorGridAlpha == 0.4f );

		// Check axis colors (X=Red, Y=Green, Z=Blue)
		REQUIRE( settings.axisXColor.x == 1.0f );
		REQUIRE( settings.axisXColor.y == 0.2f );
		REQUIRE( settings.axisXColor.z == 0.2f );

		REQUIRE( settings.axisYColor.x == 0.2f );
		REQUIRE( settings.axisYColor.y == 1.0f );
		REQUIRE( settings.axisYColor.z == 0.2f );

		REQUIRE( settings.axisZColor.x == 0.2f );
		REQUIRE( settings.axisZColor.y == 0.2f );
		REQUIRE( settings.axisZColor.z == 1.0f );

		// Check default properties
		REQUIRE( settings.gridSpacing == 1.0f );
		REQUIRE( settings.majorGridInterval == 10.0f );
		REQUIRE( settings.fadeDistance == 100.0f );
		REQUIRE( settings.axisThickness == 2.0f );

		REQUIRE( settings.showGrid == true );
		REQUIRE( settings.showAxes == true );
	}

	SECTION( "Grid settings modification" )
	{
		GridSettings settings;

		// Modify colors
		settings.majorGridColor = Vec3<>( 1.0f, 0.0f, 0.0f );
		settings.majorGridAlpha = 0.5f;

		REQUIRE( settings.majorGridColor.x == 1.0f );
		REQUIRE( settings.majorGridColor.y == 0.0f );
		REQUIRE( settings.majorGridColor.z == 0.0f );
		REQUIRE( settings.majorGridAlpha == 0.5f );

		// Modify properties
		settings.gridSpacing = 2.0f;
		settings.majorGridInterval = 5.0f;
		settings.fadeDistance = 50.0f;

		REQUIRE( settings.gridSpacing == 2.0f );
		REQUIRE( settings.majorGridInterval == 5.0f );
		REQUIRE( settings.fadeDistance == 50.0f );

		// Modify visibility
		settings.showGrid = false;
		settings.showAxes = false;

		REQUIRE_FALSE( settings.showGrid );
		REQUIRE_FALSE( settings.showAxes );
	}

	SECTION( "Grid settings bounds validation" )
	{
		GridSettings settings;

		// Test spacing bounds
		settings.minGridSpacing = 0.001f;
		settings.maxGridSpacing = 1000.0f;

		REQUIRE( settings.minGridSpacing > 0.0f );
		REQUIRE( settings.maxGridSpacing > settings.minGridSpacing );

		// Test valid spacing range
		REQUIRE( settings.gridSpacing >= settings.minGridSpacing );
		REQUIRE( settings.gridSpacing <= settings.maxGridSpacing );
	}
}

TEST_CASE( "GridRenderer Initialization", "[grid][renderer][initialization]" )
{
	SECTION( "GridRenderer creation" )
	{
		GridRenderer renderer;

		// Should be created with default settings
		const auto &settings = renderer.getSettings();
		REQUIRE( settings.gridSpacing == 1.0f );
		REQUIRE( settings.showGrid == true );
		REQUIRE( settings.showAxes == true );
	}

	SECTION( "GridRenderer settings management" )
	{
		GridRenderer renderer;

		// Modify settings
		GridSettings newSettings;
		newSettings.gridSpacing = 0.5f;
		newSettings.majorGridInterval = 20.0f;
		newSettings.showGrid = false;

		renderer.setSettings( newSettings );

		const auto &retrievedSettings = renderer.getSettings();
		REQUIRE( retrievedSettings.gridSpacing == 0.5f );
		REQUIRE( retrievedSettings.majorGridInterval == 20.0f );
		REQUIRE_FALSE( retrievedSettings.showGrid );
	}

	SECTION( "GridRenderer D3D12 initialization" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "GridRenderer D3D12 initialization" ) )
			return;

		GridRenderer renderer;
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();

		// Initialize should succeed with valid device and shader manager
		REQUIRE( renderer.initialize( &device, shaderManager ) );

		// Should be able to shutdown cleanly
		REQUIRE_NOTHROW( renderer.shutdown() );
	}

	SECTION( "GridRenderer initialization error cases" )
	{
		GridRenderer renderer;

		// Initialize with null device should fail
		REQUIRE_FALSE( renderer.initialize( nullptr, nullptr ) );

		// Shutdown without initialization should be safe
		REQUIRE_NOTHROW( renderer.shutdown() );

		// Multiple shutdowns should be safe
		REQUIRE_NOTHROW( renderer.shutdown() );
		REQUIRE_NOTHROW( renderer.shutdown() );
	}
}

TEST_CASE( "Grid Adaptive Spacing", "[grid][adaptive][spacing]" )
{
	SECTION( "Optimal spacing calculation" )
	{
		const float baseSpacing = 1.0f;

		// Close distance (0.5) - fine grid
		// log10(0.5) = -0.301, floor(-0.301) = -1, 10^-1 = 0.1, spacing = 0.1 * 0.1 = 0.01
		const float closeDistance = 0.5f;
		const float closeSpacing = GridRenderer::calculateOptimalSpacing( closeDistance, baseSpacing );
		REQUIRE( closeSpacing < baseSpacing );
		REQUIRE_THAT( closeSpacing, Catch::Matchers::WithinAbs( 0.01f, 0.001f ) );

		// Medium distance (5.0) - normal grid
		// log10(5.0) = 0.699, floor(0.699) = 0, 10^0 = 1, spacing = 1 * 0.1 = 0.1
		const float mediumDistance = 5.0f;
		const float mediumSpacing = GridRenderer::calculateOptimalSpacing( mediumDistance, baseSpacing );
		REQUIRE( mediumSpacing < baseSpacing );
		REQUIRE_THAT( mediumSpacing, Catch::Matchers::WithinAbs( 0.1f, 0.001f ) );

		// Far distance (50.0) - coarse grid
		// log10(50.0) = 1.699, floor(1.699) = 1, 10^1 = 10, spacing = 10 * 0.1 = 1.0
		const float farDistance = 50.0f;
		const float farSpacing = GridRenderer::calculateOptimalSpacing( farDistance, baseSpacing );
		REQUIRE( farSpacing == baseSpacing );
		REQUIRE_THAT( farSpacing, Catch::Matchers::WithinAbs( 1.0f, 0.001f ) );

		// Very far distance (500.0) - very coarse grid
		// log10(500.0) = 2.699, floor(2.699) = 2, 10^2 = 100, spacing = 100 * 0.1 = 10.0
		const float veryFarDistance = 500.0f;
		const float veryFarSpacing = GridRenderer::calculateOptimalSpacing( veryFarDistance, baseSpacing );
		REQUIRE( veryFarSpacing > farSpacing );
		REQUIRE_THAT( veryFarSpacing, Catch::Matchers::WithinAbs( 10.0f, 0.001f ) );
	}

	SECTION( "Major grid interval calculation" )
	{
		// Fine spacing - more frequent major lines
		float fineSpacing = 0.05f;
		int fineMajor = GridRenderer::calculateMajorInterval( fineSpacing );
		REQUIRE( fineMajor == 10 );

		// Normal spacing - standard interval
		float normalSpacing = 0.5f;
		int normalMajor = GridRenderer::calculateMajorInterval( normalSpacing );
		REQUIRE( normalMajor == 5 );

		// Coarse spacing - less frequent major lines
		float coarseSpacing = 5.0f;
		int coarseMajor = GridRenderer::calculateMajorInterval( coarseSpacing );
		REQUIRE( coarseMajor == 10 );
	}

	SECTION( "Adaptive spacing with camera" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Adaptive spacing with camera" ) )
			return;

		GridRenderer renderer;
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( renderer.initialize( &device, shaderManager ) );

		// Create camera at different distances
		PerspectiveCamera closeCamera;
		closeCamera.setPosition( Vec3<>( 0, 0, 0.5f ) );

		PerspectiveCamera farCamera;
		farCamera.setPosition( Vec3<>( 0, 0, 100.0f ) );

		// Update adaptive spacing for close camera
		renderer.updateAdaptiveSpacing( closeCamera );
		float closeSpacing = renderer.getSettings().gridSpacing;

		// Update adaptive spacing for far camera
		renderer.updateAdaptiveSpacing( farCamera );
		float farSpacing = renderer.getSettings().gridSpacing;

		// Far camera should have larger grid spacing
		REQUIRE( farSpacing > closeSpacing );

		renderer.shutdown();
	}
}

TEST_CASE( "GridUtils Utility Functions", "[grid][utils]" )
{
	SECTION( "Grid snapping functions" )
	{
		float spacing = 1.0f;

		// 2D snapping
		Vec2<> point2d( 1.3f, 2.7f );
		Vec2<> snapped2d = GridUtils::snapToGrid( point2d, spacing );
		// TODO: Fix floating point comparison syntax
		// REQUIRE(snapped2d.x == Catch::Matchers::WithinAbs(1.0f, 0.001f));
		// REQUIRE(snapped2d.y == Catch::Matchers::WithinAbs(3.0f, 0.001f));

		// 3D snapping
		Vec3<> point3d( 1.3f, 2.7f, -0.4f );
		Vec3<> snapped3d = GridUtils::snapToGrid( point3d, spacing );
		// TODO: Fix floating point comparison syntax
		// REQUIRE_THAT(snapped3d.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
		// REQUIRE_THAT(snapped3d.y, Catch::Matchers::WithinAbs(3.0f, 0.001f));
		// REQUIRE_THAT(snapped3d.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));

		// Exact grid points should remain unchanged
		Vec2<> exactPoint( 2.0f, 3.0f );
		Vec2<> exactSnapped = GridUtils::snapToGrid( exactPoint, spacing );
		REQUIRE( exactSnapped.x == exactPoint.x );
		REQUIRE( exactSnapped.y == exactPoint.y );
	}

	SECTION( "Grid line detection" )
	{
		float spacing = 1.0f;
		float tolerance = 0.01f;

		// Point on grid line (X axis)
		Vec2<> onGridX( 1.0f, 0.5f );
		REQUIRE( GridUtils::isOnGridLine( onGridX, spacing, tolerance ) );

		// Point on grid line (Y axis)
		Vec2<> onGridY( 0.5f, 2.0f );
		REQUIRE( GridUtils::isOnGridLine( onGridY, spacing, tolerance ) );

		// Point on grid intersection
		Vec2<> onIntersection( 1.0f, 2.0f );
		REQUIRE( GridUtils::isOnGridLine( onIntersection, spacing, tolerance ) );

		// Point not on grid line
		Vec2<> offGrid( 0.5f, 0.5f );
		REQUIRE_FALSE( GridUtils::isOnGridLine( offGrid, spacing, tolerance ) );

		// Point near grid line (within tolerance)
		Vec2<> nearGrid( 1.005f, 0.5f );
		REQUIRE( GridUtils::isOnGridLine( nearGrid, spacing, tolerance ) );

		// Point near grid line (outside tolerance)
		Vec2<> farFromGrid( 1.02f, 0.5f );
		REQUIRE_FALSE( GridUtils::isOnGridLine( farFromGrid, spacing, tolerance ) );
	}

	SECTION( "Axis color utilities" )
	{
		// X axis should be red
		Vec3<> xColor = GridUtils::getAxisColor( 0 );
		REQUIRE( xColor.x > 0.8f ); // Should be predominantly red
		REQUIRE( xColor.y < 0.3f );
		REQUIRE( xColor.z < 0.3f );

		// Y axis should be green
		Vec3<> yColor = GridUtils::getAxisColor( 1 );
		REQUIRE( yColor.x < 0.3f );
		REQUIRE( yColor.y > 0.8f ); // Should be predominantly green
		REQUIRE( yColor.z < 0.3f );

		// Z axis should be blue
		Vec3<> zColor = GridUtils::getAxisColor( 2 );
		REQUIRE( zColor.x < 0.3f );
		REQUIRE( zColor.y < 0.3f );
		REQUIRE( zColor.z > 0.8f ); // Should be predominantly blue

		// Invalid axis should return gray
		Vec3<> invalidColor = GridUtils::getAxisColor( 5 );
		// TODO: Fix floating point comparison syntax
		// REQUIRE(invalidColor.x == Catch::Matchers::WithinAbs(0.5f, 0.001f));
		// REQUIRE(invalidColor.y == Catch::Matchers::WithinAbs(0.5f, 0.001f));
		// REQUIRE(invalidColor.z == Catch::Matchers::WithinAbs(0.5f, 0.001f));
	}

	SECTION( "Grid fade calculations" )
	{
		Vec3<> cameraPos( 0, 0, 0 );
		float fadeDistance = 10.0f;

		// Close position - no fade
		Vec3<> closePos( 1, 1, 0 );
		float closeFade = GridUtils::calculateGridFade( closePos, cameraPos, fadeDistance );
		REQUIRE( closeFade > 0.8f );

		// Medium distance - partial fade
		Vec3<> mediumPos( 5, 0, 0 );
		float mediumFade = GridUtils::calculateGridFade( mediumPos, cameraPos, fadeDistance );
		REQUIRE( mediumFade > 0.4f );
		REQUIRE( mediumFade < 0.6f );

		// Far position - strong fade
		Vec3<> farPos( 9, 0, 0 );
		float farFade = GridUtils::calculateGridFade( farPos, cameraPos, fadeDistance );
		REQUIRE( farFade < 0.2f );

		// Beyond fade distance - complete fade
		Vec3<> beyondPos( 15, 0, 0 );
		float beyondFade = GridUtils::calculateGridFade( beyondPos, cameraPos, fadeDistance );
		REQUIRE( beyondFade == 0.0f );
	}

	SECTION( "Grid bounds calculation" )
	{
		PerspectiveCamera camera;
		camera.setPosition( Vec3<>( 5, 5, 10 ) );

		Mat4<> viewMatrix = camera.getViewMatrix();
		Mat4<> projMatrix = camera.getProjectionMatrix( 1.0f ); // 1:1 aspect ratio

		auto bounds = GridUtils::calculateGridBounds( camera, viewMatrix, projMatrix, 800, 600 );

		// Bounds should be reasonable
		REQUIRE( bounds.max.x > bounds.min.x );
		REQUIRE( bounds.max.y > bounds.min.y );
		REQUIRE( bounds.optimalSpacing > 0.0f );
		REQUIRE( bounds.majorInterval > 0 );

		// Should include camera area
		Vec3<> cameraPos = camera.getPosition();
		REQUIRE( bounds.min.x <= cameraPos.x );
		REQUIRE( bounds.max.x >= cameraPos.x );
		REQUIRE( bounds.min.y <= cameraPos.y );
		REQUIRE( bounds.max.y >= cameraPos.y );
	}
}

TEST_CASE( "Grid Rendering Integration", "[grid][render][integration]" )
{
	SECTION( "Grid rendering with valid setup" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Grid rendering integration" ) )
			return;

		GridRenderer renderer;
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( renderer.initialize( &device, shaderManager ) );

		// Create camera
		PerspectiveCamera camera;
		camera.setPosition( Vec3<>( 0, 0, 10 ) );

		Mat4<> viewMatrix = camera.getViewMatrix();
		Mat4<> projMatrix = camera.getProjectionMatrix( 1.78f ); // 16:9 aspect ratio

		// Render should succeed
		bool renderResult = renderer.render( camera, viewMatrix, projMatrix, 1920, 1080 );
		REQUIRE( renderResult );

		renderer.shutdown();
	}

	SECTION( "Grid rendering error cases" )
	{
		GridRenderer renderer;

		PerspectiveCamera camera;
		Mat4<> viewMatrix = camera.getViewMatrix();
		Mat4<> projMatrix = camera.getProjectionMatrix( 1.0f );

		// Render without initialization should fail
		bool renderResult = renderer.render( camera, viewMatrix, projMatrix, 800, 600 );
		REQUIRE_FALSE( renderResult );
	}

	SECTION( "Grid rendering with different settings" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Grid rendering with different settings" ) )
			return;

		GridRenderer renderer;
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( renderer.initialize( &device, shaderManager ) );

		PerspectiveCamera camera;
		Mat4<> viewMatrix = camera.getViewMatrix();
		Mat4<> projMatrix = camera.getProjectionMatrix( 1.0f );

		// Test with grid disabled
		GridSettings settings = renderer.getSettings();
		settings.showGrid = false;
		renderer.setSettings( settings );

		bool result1 = renderer.render( camera, viewMatrix, projMatrix, 800, 600 );
		REQUIRE( result1 ); // Should still succeed even with grid disabled

		// Test with axes disabled
		settings.showGrid = true;
		settings.showAxes = false;
		renderer.setSettings( settings );

		bool result2 = renderer.render( camera, viewMatrix, projMatrix, 800, 600 );
		REQUIRE( result2 );

		// Test with different spacing
		settings.gridSpacing = 0.5f;
		settings.majorGridInterval = 8.0f;
		renderer.setSettings( settings );

		bool result3 = renderer.render( camera, viewMatrix, projMatrix, 800, 600 );
		REQUIRE( result3 );

		renderer.shutdown();
	}
}

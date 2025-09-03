// Viewport Management tests
// Tests for multi-viewport system with cameras, input handling, and utility functions

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>
#include <vector>

import editor.viewport;
import platform.dx12;
import engine.shader_manager;

#include "test_dx12_helpers.h"

using namespace editor;
using Catch::Matchers::WithinAbs;

TEST_CASE( "Viewport Basic Properties", "[viewport]" )
{
	SECTION( "Perspective viewport creation" )
	{
		Viewport viewport( ViewportType::Perspective );

		REQUIRE( viewport.getType() == ViewportType::Perspective );
		REQUIRE_FALSE( viewport.isActive() );
		REQUIRE_FALSE( viewport.isFocused() );
		REQUIRE( viewport.isGridVisible() );
		REQUIRE( viewport.areGizmosVisible() );
		REQUIRE_FALSE( viewport.isViewSyncEnabled() );
	}

	SECTION( "Orthographic viewport creation" )
	{
		Viewport topViewport( ViewportType::Top );
		Viewport frontViewport( ViewportType::Front );
		Viewport sideViewport( ViewportType::Side );

		REQUIRE( topViewport.getType() == ViewportType::Top );
		REQUIRE( frontViewport.getType() == ViewportType::Front );
		REQUIRE( sideViewport.getType() == ViewportType::Side );

		// Check default states
		REQUIRE_FALSE( topViewport.isActive() );
		REQUIRE_FALSE( frontViewport.isActive() );
		REQUIRE_FALSE( sideViewport.isActive() );
	}

	SECTION( "Aspect ratio calculation" )
	{
		Viewport viewport( ViewportType::Perspective );

		viewport.setRenderTargetSize( 800, 600 );
		REQUIRE_THAT( viewport.getAspectRatio(), WithinAbs( 800.0f / 600.0f, 0.001f ) );

		viewport.setRenderTargetSize( 1920, 1080 );
		REQUIRE_THAT( viewport.getAspectRatio(), WithinAbs( 1920.0f / 1080.0f, 0.001f ) );

		// Test degenerate cases
		viewport.setRenderTargetSize( 100, 0 );
		REQUIRE_THAT( viewport.getAspectRatio(), WithinAbs( 1.0f, 0.001f ) ); // Should default to 1.0
	}
}

TEST_CASE( "Viewport State Management", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );

	SECTION( "Active state" )
	{
		REQUIRE_FALSE( viewport.isActive() );
		viewport.setActive( true );
		REQUIRE( viewport.isActive() );
		viewport.setActive( false );
		REQUIRE_FALSE( viewport.isActive() );
	}

	SECTION( "Focus state" )
	{
		REQUIRE_FALSE( viewport.isFocused() );
		viewport.setFocused( true );
		REQUIRE( viewport.isFocused() );
		viewport.setFocused( false );
		REQUIRE_FALSE( viewport.isFocused() );
	}

	SECTION( "Grid visibility" )
	{
		REQUIRE( viewport.isGridVisible() ); // Default true
		viewport.setGridVisible( false );
		REQUIRE_FALSE( viewport.isGridVisible() );
		viewport.setGridVisible( true );
		REQUIRE( viewport.isGridVisible() );
	}

	SECTION( "Gizmos visibility" )
	{
		REQUIRE( viewport.areGizmosVisible() ); // Default true
		viewport.setGizmosVisible( false );
		REQUIRE_FALSE( viewport.areGizmosVisible() );
		viewport.setGizmosVisible( true );
		REQUIRE( viewport.areGizmosVisible() );
	}

	SECTION( "View synchronization" )
	{
		REQUIRE_FALSE( viewport.isViewSyncEnabled() ); // Default false
		viewport.setViewSyncEnabled( true );
		REQUIRE( viewport.isViewSyncEnabled() );
		viewport.setViewSyncEnabled( false );
		REQUIRE_FALSE( viewport.isViewSyncEnabled() );
	}
}

TEST_CASE( "Viewport Input Handling", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );
	viewport.setFocused( true ); // Enable input handling

	SECTION( "Mouse input events" )
	{
		const auto mouseMove = ViewportUtils::createMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE( mouseMove.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( mouseMove.mouse.x == 100.0f );
		REQUIRE( mouseMove.mouse.y == 200.0f );
		REQUIRE( mouseMove.mouse.deltaX == 5.0f );
		REQUIRE( mouseMove.mouse.deltaY == -3.0f );

		// Should not crash when handling input
		REQUIRE_NOTHROW( viewport.handleInput( mouseMove ) );
	}

	SECTION( "Mouse button events" )
	{
		const auto leftClick = ViewportUtils::createMouseButtonEvent( 0, true, 150.0f, 250.0f );
		REQUIRE( leftClick.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( leftClick.mouse.button == 0 );
		REQUIRE( leftClick.mouse.pressed == true );
		REQUIRE( leftClick.mouse.x == 150.0f );
		REQUIRE( leftClick.mouse.y == 250.0f );

		REQUIRE_NOTHROW( viewport.handleInput( leftClick ) );
	}

	SECTION( "Mouse wheel events" )
	{
		const auto wheelEvent = ViewportUtils::createMouseWheelEvent( 120.0f, 300.0f, 400.0f );
		REQUIRE( wheelEvent.type == ViewportInputEvent::Type::MouseWheel );
		REQUIRE( wheelEvent.mouse.wheelDelta == 120.0f );
		REQUIRE( wheelEvent.mouse.x == 300.0f );
		REQUIRE( wheelEvent.mouse.y == 400.0f );

		REQUIRE_NOTHROW( viewport.handleInput( wheelEvent ) );
	}

	SECTION( "Keyboard events" )
	{
		const auto keyPress = ViewportUtils::createKeyEvent( 'W', true, false, true, false );
		REQUIRE( keyPress.type == ViewportInputEvent::Type::KeyPress );
		REQUIRE( keyPress.keyboard.keyCode == 'W' );
		REQUIRE( keyPress.keyboard.ctrl == true );
		REQUIRE_FALSE( keyPress.keyboard.shift );
		REQUIRE_FALSE( keyPress.keyboard.alt );

		REQUIRE_NOTHROW( viewport.handleInput( keyPress ) );

		const auto keyRelease = ViewportUtils::createKeyEvent( 'W', false, false, true, false );
		REQUIRE( keyRelease.type == ViewportInputEvent::Type::KeyRelease );
	}

	SECTION( "Input ignored when not focused" )
	{
		viewport.setFocused( false );

		auto mouseMove = ViewportUtils::createMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE_NOTHROW( viewport.handleInput( mouseMove ) ); // Should not crash but ignore input
	}
}

TEST_CASE( "Viewport View Operations", "[viewport]" )
{
	Viewport viewport( ViewportType::Perspective );

	SECTION( "Reset view operation" )
	{
		REQUIRE_NOTHROW( viewport.resetView() );
	}

	SECTION( "Frame all operation" )
	{
		REQUIRE_NOTHROW( viewport.frameAll() );
	}

	SECTION( "Update and render operations" )
	{
		REQUIRE_NOTHROW( viewport.update( 0.016f ) );  // 60 FPS
		REQUIRE_NOTHROW( viewport.render( nullptr ) ); // Test graceful handling of null device
	}
}

TEST_CASE( "ViewportManager Basic Operations", "[viewport][manager]" )
{
	SECTION( "ViewportManager requires D3D12 initialization" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "ViewportManager Basic Operations" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		// Basic state checks should work after successful initialization
		REQUIRE( manager.getViewports().empty() );
		REQUIRE( manager.getActiveViewport() == nullptr );
		REQUIRE( manager.getFocusedViewport() == nullptr );

		// Test viewport creation
		auto *viewport1 = manager.createViewport( ViewportType::Perspective );
		REQUIRE( viewport1 != nullptr );
		REQUIRE( manager.getViewports().size() == 1 );
		REQUIRE( manager.getActiveViewport() == viewport1 );
		REQUIRE( manager.getFocusedViewport() == viewport1 );

		auto *viewport2 = manager.createViewport( ViewportType::Top );
		REQUIRE( viewport2 != nullptr );
		REQUIRE( manager.getViewports().size() == 2 );
		REQUIRE( manager.getActiveViewport() == viewport1 ); // Still first viewport

		manager.destroyViewport( viewport1 );
		REQUIRE( manager.getViewports().size() == 1 );
		REQUIRE( manager.getActiveViewport() == viewport2 ); // Switched to remaining viewport

		manager.destroyAllViewports();
		REQUIRE( manager.getViewports().empty() );
		REQUIRE( manager.getActiveViewport() == nullptr );
	}

	SECTION( "ViewportManager handles null operations gracefully" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "ViewportManager null operations" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		// Should handle null operations without crashing
		REQUIRE_NOTHROW( manager.destroyViewport( nullptr ) );
		manager.setActiveViewport( nullptr );
		manager.setFocusedViewport( nullptr );
		REQUIRE( manager.getActiveViewport() == nullptr );
		REQUIRE( manager.getFocusedViewport() == nullptr );
	}

	SECTION( "Active and focused viewport management" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "Active and focused viewport management" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		auto *viewport1 = manager.createViewport( ViewportType::Perspective );
		auto *viewport2 = manager.createViewport( ViewportType::Top );

		REQUIRE( manager.getActiveViewport() == viewport1 );
		REQUIRE( viewport1->isActive() );
		REQUIRE_FALSE( viewport2->isActive() );

		manager.setActiveViewport( viewport2 );
		REQUIRE( manager.getActiveViewport() == viewport2 );
		REQUIRE_FALSE( viewport1->isActive() );
		REQUIRE( viewport2->isActive() );

		// Test focus management
		manager.setFocusedViewport( viewport1 );
		REQUIRE( manager.getFocusedViewport() == viewport1 );
		REQUIRE( viewport1->isFocused() );
		REQUIRE_FALSE( viewport2->isFocused() );
	}

	SECTION( "Update and render operations" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "Update and render operations" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		[[maybe_unused]] auto *viewport1 = manager.createViewport( ViewportType::Perspective );
		[[maybe_unused]] auto *viewport2 = manager.createViewport( ViewportType::Top );

		// Should not crash
		REQUIRE_NOTHROW( manager.update( 0.016f ) ); // 60 FPS
		REQUIRE_NOTHROW( manager.render() );
	}
}

TEST_CASE( "ViewportFactory Standard Layout", "[viewport][factory]" )
{
	ViewportManager manager;
	dx12::Device device;

	REQUIRE( requireHeadlessDevice( device, "ViewportFactory Standard Layout" ) );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	REQUIRE( manager.initialize( &device, shaderManager ) );

	SECTION( "Create standard 4-viewport layout" )
	{
		const auto layout = ViewportFactory::createStandardLayout( manager );

		REQUIRE( layout.perspective != nullptr );
		REQUIRE( layout.top != nullptr );
		REQUIRE( layout.front != nullptr );
		REQUIRE( layout.side != nullptr );

		REQUIRE( layout.perspective->getType() == ViewportType::Perspective );
		REQUIRE( layout.top->getType() == ViewportType::Top );
		REQUIRE( layout.front->getType() == ViewportType::Front );
		REQUIRE( layout.side->getType() == ViewportType::Side );

		REQUIRE( manager.getViewports().size() == 4 );
		REQUIRE( manager.getActiveViewport() == layout.perspective ); // Perspective is active by default
	}

	SECTION( "Create single viewport" )
	{
		const auto *viewport = ViewportFactory::createSingleViewport( manager, ViewportType::Side );

		REQUIRE( viewport != nullptr );
		REQUIRE( viewport->getType() == ViewportType::Side );
		REQUIRE( manager.getViewports().size() == 1 );
		REQUIRE( manager.getActiveViewport() == viewport );
	}
}

TEST_CASE( "ViewportUtils Utility Functions", "[viewport][utils]" )
{
	SECTION( "Viewport type names" )
	{
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Perspective ) ) == "Perspective" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Top ) ) == "Top" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Front ) ) == "Front" );
		REQUIRE( std::string( ViewportUtils::getViewportTypeName( ViewportType::Side ) ) == "Side" );
	}

	SECTION( "Orthographic type detection" )
	{
		REQUIRE_FALSE( ViewportUtils::isOrthographicType( ViewportType::Perspective ) );
		REQUIRE( ViewportUtils::isOrthographicType( ViewportType::Top ) );
		REQUIRE( ViewportUtils::isOrthographicType( ViewportType::Front ) );
		REQUIRE( ViewportUtils::isOrthographicType( ViewportType::Side ) );
	}
}

TEST_CASE( "Viewport Input Event Creation", "[viewport][input]" )
{
	SECTION( "Mouse events" )
	{
		const auto moveEvent = ViewportUtils::createMouseMoveEvent( 100.0f, 200.0f, 5.0f, -3.0f );
		REQUIRE( moveEvent.type == ViewportInputEvent::Type::MouseMove );
		REQUIRE( moveEvent.mouse.x == 100.0f );
		REQUIRE( moveEvent.mouse.y == 200.0f );
		REQUIRE( moveEvent.mouse.deltaX == 5.0f );
		REQUIRE( moveEvent.mouse.deltaY == -3.0f );

		const auto buttonEvent = ViewportUtils::createMouseButtonEvent( 1, true, 150.0f, 250.0f );
		REQUIRE( buttonEvent.type == ViewportInputEvent::Type::MouseButton );
		REQUIRE( buttonEvent.mouse.button == 1 );
		REQUIRE( buttonEvent.mouse.pressed == true );
		REQUIRE( buttonEvent.mouse.x == 150.0f );
		REQUIRE( buttonEvent.mouse.y == 250.0f );

		const auto wheelEvent = ViewportUtils::createMouseWheelEvent( -120.0f, 300.0f, 400.0f );
		REQUIRE( wheelEvent.type == ViewportInputEvent::Type::MouseWheel );
		REQUIRE( wheelEvent.mouse.wheelDelta == -120.0f );
		REQUIRE( wheelEvent.mouse.x == 300.0f );
		REQUIRE( wheelEvent.mouse.y == 400.0f );
	}

	SECTION( "Keyboard events" )
	{
		const auto keyPress = ViewportUtils::createKeyEvent( 'A', true, true, false, true );
		REQUIRE( keyPress.type == ViewportInputEvent::Type::KeyPress );
		REQUIRE( keyPress.keyboard.keyCode == 'A' );
		REQUIRE( keyPress.keyboard.shift == true );
		REQUIRE( keyPress.keyboard.ctrl == false );
		REQUIRE( keyPress.keyboard.alt == true );

		const auto keyRelease = ViewportUtils::createKeyEvent( 'B', false, false, true, false );
		REQUIRE( keyRelease.type == ViewportInputEvent::Type::KeyRelease );
		REQUIRE( keyRelease.keyboard.keyCode == 'B' );
		REQUIRE( keyRelease.keyboard.ctrl == true );
		REQUIRE_FALSE( keyRelease.keyboard.shift );
		REQUIRE_FALSE( keyRelease.keyboard.alt );
	}

	SECTION( "Resize events" )
	{
		const auto resizeEvent = ViewportUtils::createResizeEvent( 1280, 720 );
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
		viewport.setRenderTargetSize( 0, 0 );

		// Should handle gracefully
		REQUIRE_NOTHROW( viewport.update( 0.016f ) );
		REQUIRE_NOTHROW( viewport.render( nullptr ) ); // Test graceful handling of null device
	}

	SECTION( "Manager with no viewports" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "Manager with no viewports" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		// Should handle gracefully
		REQUIRE_NOTHROW( manager.update( 0.016f ) );
		REQUIRE_NOTHROW( manager.render() );

		// Should handle null operations
		manager.setActiveViewport( nullptr );
		manager.setFocusedViewport( nullptr );
		REQUIRE( manager.getActiveViewport() == nullptr );
		REQUIRE( manager.getFocusedViewport() == nullptr );
	}

	SECTION( "Destroy null viewport" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "Destroy null viewport" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		// Should not crash
		REQUIRE_NOTHROW( manager.destroyViewport( nullptr ) );
	}
}

TEST_CASE( "Viewport Types Coverage", "[viewport][types]" )
{
	SECTION( "All viewport types can be created" )
	{
		ViewportManager manager;
		dx12::Device device;

		REQUIRE( requireHeadlessDevice( device, "All viewport types can be created" ) );
		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		REQUIRE( manager.initialize( &device, shaderManager ) );

		const auto *perspective = manager.createViewport( ViewportType::Perspective );
		const auto *top = manager.createViewport( ViewportType::Top );
		const auto *front = manager.createViewport( ViewportType::Front );
		const auto *side = manager.createViewport( ViewportType::Side );

		REQUIRE( perspective->getType() == ViewportType::Perspective );
		REQUIRE( top->getType() == ViewportType::Top );
		REQUIRE( front->getType() == ViewportType::Front );
		REQUIRE( side->getType() == ViewportType::Side );

		REQUIRE( manager.getViewports().size() == 4 );
	}
}

TEST_CASE( "Viewport Camera Positioning Accuracy", "[viewport][camera]" )
{
	SECTION( "Perspective camera positioning" )
	{
		Viewport viewport( ViewportType::Perspective );
		const auto *camera = viewport.getCamera();

		const auto &position = camera->getPosition();
		const auto &target = camera->getTarget();
		const auto &up = camera->getUp();

		// Should be positioned for isometric-like view
		REQUIRE_THAT( position.x, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( position.y, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( position.z, WithinAbs( 5.0f, 0.001f ) );

		// Should look at origin
		REQUIRE_THAT( target.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.z, WithinAbs( 0.0f, 0.001f ) );

		// Should have Z-up orientation
		REQUIRE_THAT( up.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.z, WithinAbs( 1.0f, 0.001f ) );
	}

	SECTION( "Top view camera positioning" )
	{
		Viewport viewport( ViewportType::Top );
		const auto *camera = viewport.getCamera();

		const auto &position = camera->getPosition();
		const auto &target = camera->getTarget();
		const auto &up = camera->getUp();

		// Should look down Z-axis
		REQUIRE_THAT( position.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( position.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( position.z, WithinAbs( 10.0f, 0.001f ) );

		// Should look at origin
		REQUIRE_THAT( target.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.z, WithinAbs( 0.0f, 0.001f ) );

		// Should have Y-forward when looking down
		REQUIRE_THAT( up.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.y, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( up.z, WithinAbs( 0.0f, 0.001f ) );
	}

	SECTION( "Front view camera positioning" )
	{
		Viewport viewport( ViewportType::Front );
		const auto *camera = viewport.getCamera();

		const auto &position = camera->getPosition();
		const auto &target = camera->getTarget();
		const auto &up = camera->getUp();

		// Should look down Y-axis
		REQUIRE_THAT( position.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( position.y, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( position.z, WithinAbs( 0.0f, 0.001f ) );

		// Should look at origin
		REQUIRE_THAT( target.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.z, WithinAbs( 0.0f, 0.001f ) );

		// Should have Z-up
		REQUIRE_THAT( up.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.z, WithinAbs( 1.0f, 0.001f ) );
	}

	SECTION( "Side view camera positioning" )
	{
		Viewport viewport( ViewportType::Side );
		const auto *camera = viewport.getCamera();

		const auto &position = camera->getPosition();
		const auto &target = camera->getTarget();
		const auto &up = camera->getUp();

		// Should look down X-axis
		REQUIRE_THAT( position.x, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( position.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( position.z, WithinAbs( 0.0f, 0.001f ) );

		// Should look at origin
		REQUIRE_THAT( target.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( target.z, WithinAbs( 0.0f, 0.001f ) );

		// Should have Z-up
		REQUIRE_THAT( up.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( up.z, WithinAbs( 1.0f, 0.001f ) );
	}
}

TEST_CASE( "Viewport Picking Ray Generation", "[viewport][picking]" )
{
	SECTION( "Picking ray from center of viewport" )
	{
		Viewport viewport( ViewportType::Perspective );
		viewport.setRenderTargetSize( 800, 600 );

		// Get picking ray from center of screen
		const auto ray = viewport.getPickingRay( math::Vec2<>( 400.0f, 300.0f ) );

		// Ray should have valid origin and direction
		REQUIRE( length( ray.direction ) > 0.99f ); // Should be normalized
		REQUIRE( length( ray.direction ) < 1.01f );
	}

	SECTION( "Picking rays from different screen positions" )
	{
		Viewport viewport( ViewportType::Perspective );
		viewport.setRenderTargetSize( 800, 600 );

		// Get rays from corners
		const auto topLeftRay = viewport.getPickingRay( math::Vec2<>( 0.0f, 0.0f ) );
		const auto topRightRay = viewport.getPickingRay( math::Vec2<>( 800.0f, 0.0f ) );
		const auto bottomLeftRay = viewport.getPickingRay( math::Vec2<>( 0.0f, 600.0f ) );
		const auto bottomRightRay = viewport.getPickingRay( math::Vec2<>( 800.0f, 600.0f ) );

		// All rays should have normalized directions
		REQUIRE_THAT( length( topLeftRay.direction ), WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( length( topRightRay.direction ), WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( length( bottomLeftRay.direction ), WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( length( bottomRightRay.direction ), WithinAbs( 1.0f, 0.001f ) );

		// Rays from different positions should have different directions
		REQUIRE( !( topLeftRay.direction.x == topRightRay.direction.x &&
			topLeftRay.direction.y == topRightRay.direction.y &&
			topLeftRay.direction.z == topRightRay.direction.z ) );
		REQUIRE( !( topLeftRay.direction.x == bottomLeftRay.direction.x &&
			topLeftRay.direction.y == bottomLeftRay.direction.y &&
			topLeftRay.direction.z == bottomLeftRay.direction.z ) );
	}

	SECTION( "Orthographic viewport picking rays" )
	{
		Viewport viewport( ViewportType::Top );
		viewport.setRenderTargetSize( 800, 600 );

		const auto ray = viewport.getPickingRay( math::Vec2<>( 400.0f, 300.0f ) );

		// For top view, rays should generally point downward (negative Z direction)
		// (exact direction depends on camera setup, but should be consistent)
		REQUIRE_THAT( length( ray.direction ), WithinAbs( 1.0f, 0.001f ) );
	}
}

TEST_CASE( "Viewport Grid Settings Management", "[viewport][grid]" )
{
	SECTION( "Default grid settings values" )
	{
		Viewport viewport( ViewportType::Perspective );
		const auto &settings = viewport.getGridSettings();

		// Test default values match GridSettings defaults
		REQUIRE_THAT( settings.gridSpacing, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( settings.majorGridInterval, WithinAbs( 10.0f, 0.001f ) );
		REQUIRE_THAT( settings.fadeDistanceMultiplier, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( settings.axisThickness, WithinAbs( 2.0f, 0.001f ) );
		REQUIRE( settings.showGrid == true );
		REQUIRE( settings.showAxes == true );

		// Test default colors
		REQUIRE_THAT( settings.majorGridColor.x, WithinAbs( 0.5f, 0.001f ) );
		REQUIRE_THAT( settings.majorGridColor.y, WithinAbs( 0.5f, 0.001f ) );
		REQUIRE_THAT( settings.majorGridColor.z, WithinAbs( 0.5f, 0.001f ) );
		REQUIRE_THAT( settings.majorGridAlpha, WithinAbs( 0.8f, 0.001f ) );

		REQUIRE_THAT( settings.minorGridColor.x, WithinAbs( 0.3f, 0.001f ) );
		REQUIRE_THAT( settings.minorGridColor.y, WithinAbs( 0.3f, 0.001f ) );
		REQUIRE_THAT( settings.minorGridColor.z, WithinAbs( 0.3f, 0.001f ) );
		REQUIRE_THAT( settings.minorGridAlpha, WithinAbs( 0.4f, 0.001f ) );

		// Test default axis colors
		REQUIRE_THAT( settings.axisXColor.x, WithinAbs( 1.0f, 0.001f ) ); // Red X
		REQUIRE_THAT( settings.axisXColor.y, WithinAbs( 0.2f, 0.001f ) );
		REQUIRE_THAT( settings.axisXColor.z, WithinAbs( 0.2f, 0.001f ) );

		REQUIRE_THAT( settings.axisYColor.x, WithinAbs( 0.2f, 0.001f ) ); // Green Y
		REQUIRE_THAT( settings.axisYColor.y, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( settings.axisYColor.z, WithinAbs( 0.2f, 0.001f ) );

		REQUIRE_THAT( settings.axisZColor.x, WithinAbs( 0.2f, 0.001f ) ); // Blue Z
		REQUIRE_THAT( settings.axisZColor.y, WithinAbs( 0.2f, 0.001f ) );
		REQUIRE_THAT( settings.axisZColor.z, WithinAbs( 1.0f, 0.001f ) );
	}

	SECTION( "Grid settings modification" )
	{
		Viewport viewport( ViewportType::Perspective );

		// Get initial settings
		auto settings = viewport.getGridSettings();

		// Modify settings
		settings.gridSpacing = 2.5f;
		settings.majorGridInterval = 5.0f;
		settings.showGrid = false;
		settings.majorGridColor = { 1.0f, 0.0f, 0.0f }; // Red
		settings.majorGridAlpha = 0.6f;

		// Apply changes
		viewport.setGridSettings( settings );

		// Verify changes were applied
		const auto &updatedSettings = viewport.getGridSettings();
		REQUIRE_THAT( updatedSettings.gridSpacing, WithinAbs( 2.5f, 0.001f ) );
		REQUIRE_THAT( updatedSettings.majorGridInterval, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE( updatedSettings.showGrid == false );
		REQUIRE_THAT( updatedSettings.majorGridColor.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( updatedSettings.majorGridColor.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( updatedSettings.majorGridColor.z, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( updatedSettings.majorGridAlpha, WithinAbs( 0.6f, 0.001f ) );

		// Verify other settings remain unchanged
		REQUIRE_THAT( updatedSettings.fadeDistanceMultiplier, WithinAbs( 5.0f, 0.001f ) ); // Still default
		REQUIRE_THAT( updatedSettings.axisThickness, WithinAbs( 2.0f, 0.001f ) );			 // Still default
	}

	SECTION( "Grid settings persistence within viewport" )
	{
		Viewport viewport( ViewportType::Top );

		// Set custom settings
		auto settings = viewport.getGridSettings();
		settings.gridSpacing = 0.5f;
		settings.minorGridColor = { 0.8f, 0.8f, 0.2f }; // Yellow
		settings.minorGridAlpha = 0.7f;
		settings.showAxes = false;

		viewport.setGridSettings( settings );

		// Get settings again and verify persistence
		const auto &retrievedSettings = viewport.getGridSettings();
		REQUIRE_THAT( retrievedSettings.gridSpacing, WithinAbs( 0.5f, 0.001f ) );
		REQUIRE_THAT( retrievedSettings.minorGridColor.x, WithinAbs( 0.8f, 0.001f ) );
		REQUIRE_THAT( retrievedSettings.minorGridColor.y, WithinAbs( 0.8f, 0.001f ) );
		REQUIRE_THAT( retrievedSettings.minorGridColor.z, WithinAbs( 0.2f, 0.001f ) );
		REQUIRE_THAT( retrievedSettings.minorGridAlpha, WithinAbs( 0.7f, 0.001f ) );
		REQUIRE( retrievedSettings.showAxes == false );
	}

	SECTION( "Independent grid settings per viewport type" )
	{
		Viewport perspectiveViewport( ViewportType::Perspective );
		Viewport topViewport( ViewportType::Top );

		// Modify perspective viewport settings
		auto perspectiveSettings = perspectiveViewport.getGridSettings();
		perspectiveSettings.gridSpacing = 3.0f;
		perspectiveSettings.majorGridColor = { 1.0f, 0.5f, 0.0f }; // Orange
		perspectiveViewport.setGridSettings( perspectiveSettings );

		// Modify top viewport settings differently
		auto topSettings = topViewport.getGridSettings();
		topSettings.gridSpacing = 0.25f;
		topSettings.majorGridColor = { 0.0f, 1.0f, 0.5f }; // Green
		topViewport.setGridSettings( topSettings );

		// Verify independence - changes to one don't affect the other
		const auto &updatedPerspective = perspectiveViewport.getGridSettings();
		const auto &updatedTop = topViewport.getGridSettings();

		REQUIRE_THAT( updatedPerspective.gridSpacing, WithinAbs( 3.0f, 0.001f ) );
		REQUIRE_THAT( updatedTop.gridSpacing, WithinAbs( 0.25f, 0.001f ) );

		REQUIRE_THAT( updatedPerspective.majorGridColor.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( updatedTop.majorGridColor.x, WithinAbs( 0.0f, 0.001f ) );

		REQUIRE_THAT( updatedPerspective.majorGridColor.y, WithinAbs( 0.5f, 0.001f ) );
		REQUIRE_THAT( updatedTop.majorGridColor.y, WithinAbs( 1.0f, 0.001f ) );
	}
}

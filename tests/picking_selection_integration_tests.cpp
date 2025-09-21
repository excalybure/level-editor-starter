#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

// Integration test imports
#include "engine/picking.h"
#include "editor/selection.h"
#include "editor/viewport_input.h"
#include "editor/viewport/viewport.h"
#include "editor/selection_renderer.h"
#include "runtime/ecs.h"
#include "runtime/entity.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "engine/camera/camera.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/math/bounding_box_3d.h"

using namespace math;
using namespace picking;
using namespace editor;
using namespace components;
using namespace ecs;
using namespace systems;
using Catch::Matchers::WithinAbs;

// Mock viewport for complete integration testing
class IntegrationTestViewport : public Viewport
{
public:
	IntegrationTestViewport()
		: Viewport( ViewportType::Perspective ), m_viewMatrix( Mat4<>::identity() ), m_projMatrix( Mat4<>::perspective( 45.0f, 16.0f / 9.0f, 0.1f, 1000.0f ) )
	{
		setRenderTargetSize( 800, 600 );
	}

	ViewportRay getPickingRay( const Vec2<> &screenPos ) const noexcept override
	{
		ViewportRay ray;
		// Convert screen coordinates to normalized device coordinates
		const float ndcX = ( 2.0f * screenPos.x / 800.0f ) - 1.0f;
		const float ndcY = 1.0f - ( 2.0f * screenPos.y / 600.0f );

		// Simple perspective ray calculation
		ray.origin = Vec3<>{ 0.0f, -10.0f, 0.0f };
		ray.direction = normalize( Vec3<>{ ndcX * 5.0f, 10.0f, ndcY * 5.0f } );
		ray.length = 1000.0f;
		return ray;
	}

	Vec2<> worldToScreen( const Vec3<> &worldPos ) const noexcept override
	{
		// Simple orthographic projection for testing
		return Vec2<>{ ( worldPos.x + 5.0f ) * 80.0f, 300.0f - worldPos.z * 60.0f };
	}

	const Mat4<> &getViewMatrix() const { return m_viewMatrix; }
	const Mat4<> &getProjectionMatrix() const { return m_projMatrix; }

private:
	Mat4<> m_viewMatrix;
	Mat4<> m_projMatrix;
};

// Helper to create test entities with mesh renderers
Entity createTestCube( Scene &scene, const Vec3<> &position, const Vec3<> &size, const std::string &name )
{
	auto entity = scene.createEntity( name );

	Transform transform;
	transform.position = position;
	scene.addComponent( entity, transform );

	MeshRenderer meshRenderer;
	meshRenderer.bounds = BoundingBox3D<float>{
		-size * 0.5f,
		size * 0.5f
	};
	scene.addComponent( entity, meshRenderer );

	return entity;
}

TEST_CASE( "Picking System - Complete workflow integration", "[picking][integration][complete]" )
{
	// Setup complete scene with multiple objects
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	PickingSystem picker( systemManager );
	SelectionManager selectionManager( scene, systemManager );
	ViewportInputHandler inputHandler( selectionManager, picker, systemManager );

	// Create test scene with multiple objects at different depths
	const auto nearCube = createTestCube( scene, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 2.0f, 2.0f, 2.0f }, "NearCube" );
	const auto farCube = createTestCube( scene, Vec3<>{ 0.0f, 10.0f, 0.0f }, Vec3<>{ 2.0f, 2.0f, 2.0f }, "FarCube" );
	const auto sideObject = createTestCube( scene, Vec3<>{ 5.0f, 0.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "SideObject" );
	const auto smallObject = createTestCube( scene, Vec3<>{ -3.0f, 5.0f, 2.0f }, Vec3<>{ 0.5f, 0.5f, 0.5f }, "SmallObject" );

	IntegrationTestViewport viewport;

	SECTION( "Complete mouse picking workflow" )
	{
		// Update transform system to ensure world matrices are computed
		systemManager.update( scene, 0.016f );

		// Test 1: Click on near cube (center screen should hit it)
		Vec2<> centerScreen{ 400.0f, 300.0f };
		inputHandler.handleMouseClick( scene, viewport, centerScreen, true, false, false, false );

		REQUIRE( selectionManager.isSelected( nearCube ) );
		REQUIRE( selectionManager.getPrimarySelection() == nearCube );
		REQUIRE( selectionManager.getSelectionCount() == 1 );

		// Test 2: Ctrl+Click on side object (additive selection)
		Vec2<> sideScreen{ 800.0f, 300.0f }; // Right side of screen
		inputHandler.handleMouseClick( scene, viewport, sideScreen, true, false, true, false );

		REQUIRE( selectionManager.getSelectionCount() == 2 );
		REQUIRE( selectionManager.isSelected( nearCube ) );
		REQUIRE( selectionManager.isSelected( sideObject ) );
		REQUIRE( selectionManager.getPrimarySelection() == nearCube ); // Primary unchanged

		// Test 3: Shift+Click on already selected object (toggle/remove)
		inputHandler.handleMouseClick( scene, viewport, centerScreen, true, false, false, true );

		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE_FALSE( selectionManager.isSelected( nearCube ) );
		REQUIRE( selectionManager.isSelected( sideObject ) );
		REQUIRE( selectionManager.getPrimarySelection() == sideObject ); // Primary transferred

		// Test 4: Rectangle selection to get multiple objects
		inputHandler.handleMouseDrag( scene, viewport, Vec2<>{ 0.0f, 0.0f }, Vec2<>{ 800.0f, 600.0f }, false, false );
		inputHandler.handleMouseRelease( scene, viewport, Vec2<>{ 800.0f, 600.0f } );

		// Should select all visible objects in the rectangle
		REQUIRE( selectionManager.getSelectionCount() >= 3 );
		REQUIRE( selectionManager.isSelected( nearCube ) );
		REQUIRE( selectionManager.isSelected( farCube ) );
		REQUIRE( selectionManager.isSelected( sideObject ) );
	}

	SECTION( "Distance-based picking priority" )
	{
		systemManager.update( scene, 0.016f );

		// Ray that hits both near and far cube
		Vec3<> rayOrigin{ 0.0f, -5.0f, 0.0f };
		Vec3<> rayDirection{ 0.0f, 1.0f, 0.0f };

		const auto results = picker.raycastAll( scene, rayOrigin, rayDirection );

		REQUIRE( results.size() >= 2 );
		REQUIRE( results[0].entity == nearCube ); // Closer cube first
		REQUIRE( results[1].entity == farCube );  // Further cube second
		REQUIRE( results[0].distance < results[1].distance );

		// Verify hit positions are reasonable
		REQUIRE( results[0].distance == Catch::Approx( 4.0f ).margin( 1.0f ) );	 // Near cube at z=0, ray from z=-5
		REQUIRE( results[1].distance == Catch::Approx( 14.0f ).margin( 1.0f ) ); // Far cube at z=10
	}

	SECTION( "Selection bounds and spatial queries" )
	{
		selectionManager.select( { nearCube, farCube, sideObject } );
		systemManager.update( scene, 0.016f );

		auto bounds = selectionManager.getSelectionBounds();

		REQUIRE( bounds.isValid() );

		// Bounds should encompass all selected objects
		REQUIRE( bounds.min.x <= -1.0f ); // Near cube extends to -1
		REQUIRE( bounds.max.x >= 5.5f );  // Side object extends to 5.5
		REQUIRE( bounds.min.y <= -1.0f ); // Near cube extends to -1
		REQUIRE( bounds.max.y >= 11.0f ); // Far cube extends to 11

		// Test selection framing
		auto radius = selectionManager.getSelectionRadius();
		REQUIRE( radius > 0.0f );
		REQUIRE( radius >= 5.0f ); // Should be at least large enough to encompass spread
	}

	SECTION( "Hover detection and visual feedback" )
	{
		systemManager.update( scene, 0.016f );

		// Test hover detection
		Vec2<> hoverPos{ 400.0f, 300.0f }; // Center screen
		inputHandler.handleMouseMove( scene, viewport, hoverPos );

		auto hoveredEntity = inputHandler.getHoveredEntity();
		REQUIRE( hoveredEntity == nearCube ); // Should detect near cube at center

		// Move mouse away
		inputHandler.handleMouseMove( scene, viewport, Vec2<>{ 50.0f, 50.0f } );
		hoveredEntity = inputHandler.getHoveredEntity();
		// Hover state may or may not clear depending on what's at that position
	}
}

TEST_CASE( "Selection Event System - Complete integration", "[selection][events][integration]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	SelectionManager selectionManager( scene, systemManager );

	auto entity1 = createTestCube( scene, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "Entity1" );
	auto entity2 = createTestCube( scene, Vec3<>{ 3.0f, 0.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "Entity2" );
	auto entity3 = createTestCube( scene, Vec3<>{ 0.0f, 3.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "Entity3" );

	// Event tracking
	std::vector<SelectionChangedEvent> events;
	selectionManager.registerListener( [&]( const SelectionChangedEvent &event ) {
		events.push_back( event );
	} );

	SECTION( "Complex selection workflow events" )
	{
		// Step 1: Single selection
		selectionManager.select( entity1 );

		REQUIRE( events.size() == 1 );
		REQUIRE( events[0].currentSelection.size() == 1 );
		REQUIRE( events[0].currentSelection[0] == entity1 );
		REQUIRE( events[0].added.size() == 1 );
		REQUIRE( events[0].removed.empty() );
		REQUIRE( events[0].newPrimarySelection == entity1 );

		// Step 2: Additive selection
		selectionManager.select( entity2, true );

		REQUIRE( events.size() == 2 );
		REQUIRE( events[1].currentSelection.size() == 2 );
		REQUIRE( events[1].added.size() == 1 );
		REQUIRE( events[1].added[0] == entity2 );
		REQUIRE( events[1].removed.empty() );
		REQUIRE( events[1].newPrimarySelection == entity1 ); // Primary unchanged

		// Step 3: Batch selection (replace)
		selectionManager.select( { entity2, entity3 }, false );

		REQUIRE( events.size() == 3 );
		REQUIRE( events[2].currentSelection.size() == 2 );
		REQUIRE( events[2].added.size() == 1 );	  // entity3 added
		REQUIRE( events[2].removed.size() == 1 ); // entity1 removed
		REQUIRE( events[2].removed[0] == entity1 );
		REQUIRE( events[2].newPrimarySelection == entity2 );

		// Step 4: Primary selection change
		selectionManager.setPrimarySelection( entity3 );

		REQUIRE( events.size() == 4 );
		REQUIRE( events[3].currentSelection.size() == 2 ); // No change in selection
		REQUIRE( events[3].added.empty() );
		REQUIRE( events[3].removed.empty() );
		REQUIRE( events[3].previousPrimarySelection == entity2 );
		REQUIRE( events[3].newPrimarySelection == entity3 );

		// Step 5: Deselect all
		selectionManager.deselectAll();

		REQUIRE( events.size() == 5 );
		REQUIRE( events[4].currentSelection.empty() );
		REQUIRE( events[4].added.empty() );
		REQUIRE( events[4].removed.size() == 2 );
		REQUIRE( events[4].newPrimarySelection == Entity{} );
	}
}

TEST_CASE( "Rectangle Selection - Comprehensive integration", "[rectangle][selection][integration]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	PickingSystem picker( systemManager );
	SelectionManager selectionManager( scene, systemManager );
	ViewportInputHandler inputHandler( selectionManager, picker, systemManager );

	// Create grid of objects for rectangle selection testing
	std::vector<Entity> gridEntities;
	for ( int x = 0; x < 3; ++x )
	{
		for ( int y = 0; y < 3; ++y )
		{
			auto entity = createTestCube( scene,
				Vec3<>{ x * 2.0f - 2.0f, y * 2.0f - 2.0f, 0.0f },
				Vec3<>{ 0.8f, 0.8f, 0.8f },
				"Grid_" + std::to_string( x ) + "_" + std::to_string( y ) );
			gridEntities.push_back( entity );
		}
	}

	IntegrationTestViewport viewport;
	systemManager.update( scene, 0.016f );

	SECTION( "Rectangle selection behavior" )
	{
		// Test small rectangle selection
		Vec2<> smallStart{ 350.0f, 250.0f };
		Vec2<> smallEnd{ 450.0f, 350.0f };

		inputHandler.handleMouseDrag( scene, viewport, smallStart, smallEnd, false, false );

		REQUIRE( inputHandler.isRectSelectionActive() );

		auto &rectSelection = inputHandler.getRectSelection();
		REQUIRE( rectSelection.active );
		REQUIRE_THAT( rectSelection.startPos.x, WithinAbs( 350.0f, 0.1f ) );
		REQUIRE_THAT( rectSelection.startPos.y, WithinAbs( 250.0f, 0.1f ) );
		REQUIRE_THAT( rectSelection.endPos.x, WithinAbs( 450.0f, 0.1f ) );
		REQUIRE_THAT( rectSelection.endPos.y, WithinAbs( 350.0f, 0.1f ) );

		// Complete rectangle selection
		inputHandler.handleMouseRelease( scene, viewport, smallEnd );

		REQUIRE_FALSE( inputHandler.isRectSelectionActive() );

		// Should have selected center objects
		REQUIRE( selectionManager.getSelectionCount() > 0 );
	}

	SECTION( "Rectangle selection with modifiers" )
	{
		// First select some objects manually
		selectionManager.select( gridEntities[0] );

		// Rectangle select with Ctrl (additive)
		Vec2<> rectStart{ 200.0f, 200.0f };
		Vec2<> rectEnd{ 600.0f, 400.0f };

		inputHandler.handleMouseDrag( scene, viewport, rectStart, rectEnd, true, false );
		inputHandler.handleMouseRelease( scene, viewport, rectEnd );

		// Should have original selection plus rectangle selection
		REQUIRE( selectionManager.isSelected( gridEntities[0] ) );
		REQUIRE( selectionManager.getSelectionCount() > 1 );
	}

	SECTION( "Large rectangle selection" )
	{
		// Select entire viewport
		Vec2<> fullStart{ 0.0f, 0.0f };
		Vec2<> fullEnd{ 800.0f, 600.0f };

		inputHandler.handleMouseDrag( scene, viewport, fullStart, fullEnd, false, false );
		inputHandler.handleMouseRelease( scene, viewport, fullEnd );

		// Should select all visible objects
		REQUIRE( selectionManager.getSelectionCount() == gridEntities.size() );

		for ( auto entity : gridEntities )
		{
			REQUIRE( selectionManager.isSelected( entity ) );
		}
	}
}

TEST_CASE( "Performance - Large scene selection", "[performance][selection][integration]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	PickingSystem picker( systemManager );
	SelectionManager selectionManager( scene, systemManager );

	// Create large number of objects for performance testing
	std::vector<Entity> entities;
	const int objectCount = 1000;

	for ( int i = 0; i < objectCount; ++i )
	{
		float x = ( i % 100 ) * 2.0f - 100.0f;
		float z = ( i / 100 ) * 2.0f - 10.0f;
		auto entity = createTestCube( scene, Vec3<>{ x, 0.0f, z }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "Perf_" + std::to_string( i ) );
		entities.push_back( entity );
	}

	systemManager.update( scene, 0.016f );

	SECTION( "Ray casting performance with many objects" )
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		// Perform 100 ray casts
		for ( int i = 0; i < 100; ++i )
		{
			Vec3<> rayOrigin{ float( i % 10 ) * 2.0f, -20.0f, 0.0f };
			Vec3<> rayDirection{ 0.0f, 1.0f, 0.0f };

			auto result = picker.raycast( scene, rayOrigin, rayDirection );
			// Result validation not needed for performance test
		}

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( endTime - startTime );

		// Should complete in reasonable time (less than 200ms for 100 rays on 1000 objects)
		REQUIRE( duration.count() < 200 );
	}

	SECTION( "Selection performance with many objects" )
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		// Select many objects
		std::vector<Entity> toSelect;
		for ( int i = 0; i < 500; ++i )
		{
			toSelect.push_back( entities[i] );
		}

		selectionManager.select( toSelect, false );

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( endTime - startTime );

		// Should complete quickly (less than 50ms for 500 objects)
		REQUIRE( duration.count() < 50 );
		REQUIRE( selectionManager.getSelectionCount() == 500 );
	}

	SECTION( "Selection bounds calculation performance" )
	{
		// Select all objects
		selectionManager.select( entities, false );

		const auto startTime = std::chrono::high_resolution_clock::now();

		// Calculate bounds multiple times
		for ( int i = 0; i < 100; ++i )
		{
			const auto bounds = selectionManager.getSelectionBounds();
			REQUIRE( bounds.isValid() );
		}

		const auto endTime = std::chrono::high_resolution_clock::now();
		const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( endTime - startTime );

		// Should be fast (less than 300ms for 100 calculations on 1000 objects)
		REQUIRE( duration.count() < 300 );
	}
}

TEST_CASE( "Error Handling - Robustness integration", "[error][handling][integration]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	PickingSystem picker( systemManager );
	SelectionManager selectionManager( scene, systemManager );
	ViewportInputHandler inputHandler( selectionManager, picker, systemManager );

	SECTION( "Invalid entity handling" )
	{
		Entity invalidEntity{ 999999 };

		// Should not crash or affect valid state
		REQUIRE_NOTHROW( selectionManager.select( invalidEntity ) );
		REQUIRE( selectionManager.getSelectionCount() == 0 );

		REQUIRE_NOTHROW( selectionManager.deselect( invalidEntity ) );
		REQUIRE_NOTHROW( selectionManager.toggleSelection( invalidEntity ) );
		REQUIRE_NOTHROW( selectionManager.setPrimarySelection( invalidEntity ) );
	}

	SECTION( "Empty scene picking" )
	{
		IntegrationTestViewport viewport;

		// Ray cast in empty scene
		Vec3<> rayOrigin{ 0.0f, 0.0f, -5.0f };
		Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );
		REQUIRE_FALSE( result.hit );
		REQUIRE( result.entity == Entity{} );

		// Input handling in empty scene
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, Vec2<>{ 400.0f, 300.0f }, true, false, false, false ) );
		REQUIRE_NOTHROW( inputHandler.handleMouseMove( scene, viewport, Vec2<>{ 400.0f, 300.0f } ) );
	}

	SECTION( "Destroyed entity cleanup" )
	{
		auto entity = createTestCube( scene, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "TestCube" );

		selectionManager.select( entity );
		REQUIRE( selectionManager.isSelected( entity ) );

		// Destroy the entity
		scene.destroyEntity( entity );

		// Validate selection should clean up
		selectionManager.validateSelection();

		REQUIRE( selectionManager.getSelectionCount() == 0 );
		REQUIRE_FALSE( selectionManager.isSelected( entity ) );
	}

	SECTION( "Extreme values handling" )
	{
		auto entity = createTestCube( scene, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 1.0f, 1.0f, 1.0f }, "TestCube" );

		// Very large ray distances
		Vec3<> rayOrigin{ 0.0f, 0.0f, -1000000.0f };
		Vec3<> rayDirection{ 0.0f, 0.0f, 1.0f };

		REQUIRE_NOTHROW( picker.raycast( scene, rayOrigin, rayDirection, 2000000.0f ) );

		// Very small mouse movements
		IntegrationTestViewport viewport;
		REQUIRE_NOTHROW( inputHandler.handleMouseMove( scene, viewport, Vec2<>{ 0.001f, 0.001f } ) );

		// Mouse at screen edges
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, Vec2<>{ -100.0f, -100.0f }, true, false, false, false ) );
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, Vec2<>{ 1000.0f, 1000.0f }, true, false, false, false ) );
	}
}

TEST_CASE( "Integration Acceptance Criteria Validation", "[acceptance][criteria][integration]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );

	PickingSystem picker( systemManager );
	SelectionManager selectionManager( scene, systemManager );
	ViewportInputHandler inputHandler( selectionManager, picker, systemManager );

	// Create test objects
	auto testObject = createTestCube( scene, Vec3<>{ 0.0f, 0.0f, 0.0f }, Vec3<>{ 2.0f, 2.0f, 2.0f }, "TestObject" );
	auto secondObject = createTestCube( scene, Vec3<>{ 5.0f, 0.0f, 0.0f }, Vec3<>{ 2.0f, 2.0f, 2.0f }, "SecondObject" );

	IntegrationTestViewport viewport;
	systemManager.update( scene, 0.016f );

	SECTION( "✅ Ray-casting system: Accurate intersection testing" )
	{
		Vec3<> rayOrigin{ 0.0f, -5.0f, 0.0f };
		Vec3<> rayDirection{ 0.0f, 1.0f, 0.0f };

		auto result = picker.raycast( scene, rayOrigin, rayDirection );

		REQUIRE( result.hit );
		REQUIRE( result.entity == testObject );
		REQUIRE( result.distance > 0.0f );
		REQUIRE( result.distance < 10.0f );
	}

	SECTION( "✅ Single object selection: Mouse click selects nearest object" )
	{
		Vec2<> screenPos{ 400.0f, 300.0f }; // Center screen

		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, false, false );

		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE( selectionManager.isSelected( testObject ) );
	}

	SECTION( "✅ Multi-object selection: Ctrl/Shift modifier keys" )
	{
		// Initial selection
		selectionManager.select( testObject );

		// Additive selection with Ctrl
		Vec2<> sideScreen{ 800.0f, 300.0f };
		inputHandler.handleMouseClick( scene, viewport, sideScreen, true, false, true, false );

		REQUIRE( selectionManager.getSelectionCount() == 2 );
		REQUIRE( selectionManager.isSelected( testObject ) );
		REQUIRE( selectionManager.isSelected( secondObject ) );

		// Toggle with Shift
		inputHandler.handleMouseClick( scene, viewport, Vec2<>{ 400.0f, 300.0f }, true, false, false, true );

		REQUIRE( selectionManager.getSelectionCount() == 1 );
		REQUIRE_FALSE( selectionManager.isSelected( testObject ) );
		REQUIRE( selectionManager.isSelected( secondObject ) );
	}

	SECTION( "✅ Rectangle selection: Click and drag selects multiple objects" )
	{
		inputHandler.handleMouseDrag( scene, viewport, Vec2<>{ 0.0f, 0.0f }, Vec2<>{ 800.0f, 600.0f }, false, false );
		inputHandler.handleMouseRelease( scene, viewport, Vec2<>{ 800.0f, 600.0f } );

		REQUIRE( selectionManager.getSelectionCount() >= 2 );
		REQUIRE( selectionManager.isSelected( testObject ) );
		REQUIRE( selectionManager.isSelected( secondObject ) );
	}

	SECTION( "✅ Primary selection: Distinguished primary for gizmo operations" )
	{
		selectionManager.select( { testObject, secondObject } );

		REQUIRE( selectionManager.getPrimarySelection() == testObject );

		// Change primary
		selectionManager.setPrimarySelection( secondObject );
		REQUIRE( selectionManager.getPrimarySelection() == secondObject );

		// ECS components should reflect primary state
		REQUIRE_FALSE( scene.getComponent<Selected>( testObject )->isPrimary );
		REQUIRE( scene.getComponent<Selected>( secondObject )->isPrimary );
	}

	SECTION( "✅ Selection events: Notification system for selection changes" )
	{
		bool eventReceived = false;
		SelectionChangedEvent lastEvent;

		selectionManager.registerListener( [&]( const SelectionChangedEvent &event ) {
			eventReceived = true;
			lastEvent = event;
		} );

		selectionManager.select( testObject );

		REQUIRE( eventReceived );
		REQUIRE( lastEvent.currentSelection.size() == 1 );
		REQUIRE( lastEvent.added.size() == 1 );
		REQUIRE( lastEvent.removed.empty() );
	}

	SECTION( "✅ ECS integration: Selected component automatically managed" )
	{
		// Selection adds component
		selectionManager.select( testObject );
		REQUIRE( scene.hasComponent<Selected>( testObject ) );

		// Deselection removes component
		selectionManager.deselect( testObject );
		REQUIRE_FALSE( scene.hasComponent<Selected>( testObject ) );

		// Multi-selection manages components correctly
		selectionManager.select( { testObject, secondObject } );
		REQUIRE( scene.hasComponent<Selected>( testObject ) );
		REQUIRE( scene.hasComponent<Selected>( secondObject ) );

		auto *primaryComp = scene.getComponent<Selected>( testObject );
		auto *secondaryComp = scene.getComponent<Selected>( secondObject );
		REQUIRE( primaryComp->isPrimary );
		REQUIRE_FALSE( secondaryComp->isPrimary );
	}

	SECTION( "✅ Input responsiveness: Selection responds quickly" )
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		// Perform selection operation
		Vec2<> screenPos{ 400.0f, 300.0f };
		inputHandler.handleMouseClick( scene, viewport, screenPos, true, false, false, false );

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( endTime - startTime );

		// Should respond within 16ms (60fps)
		REQUIRE( duration.count() < 16000 ); // 16ms in microseconds
		REQUIRE( selectionManager.isSelected( testObject ) );
	}

	SECTION( "✅ Error handling: Graceful handling of edge cases" )
	{
		// Invalid input coordinates
		REQUIRE_NOTHROW( inputHandler.handleMouseClick( scene, viewport, Vec2<>{ -999.0f, -999.0f }, true, false, false, false ) );

		// Empty selection operations
		REQUIRE_NOTHROW( selectionManager.deselectAll() );
		REQUIRE_NOTHROW( selectionManager.validateSelection() );

		// Invalid entity operations
		Entity invalid{ 999999 };
		REQUIRE_NOTHROW( selectionManager.select( invalid ) );
		REQUIRE( selectionManager.getSelectionCount() == 0 );
	}
}
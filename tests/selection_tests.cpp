#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import editor.selection;
import runtime.ecs;
import runtime.entity;
import runtime.components;
import runtime.systems;
import engine.vec;
import engine.bounding_box_3d;

using namespace ecs;
using namespace components;
using namespace systems;
using namespace editor;

TEST_CASE( "SelectionManager - Basic operations", "[selection][basic]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	auto entity1 = scene.createEntity( "Object1" );
	auto entity2 = scene.createEntity( "Object2" );

	SECTION( "Single selection" )
	{
		selection.select( entity1 );

		REQUIRE( selection.getSelectionCount() == 1 );
		REQUIRE( selection.isSelected( entity1 ) );
		REQUIRE_FALSE( selection.isSelected( entity2 ) );
		REQUIRE( selection.getPrimarySelection() == entity1 );

		// ECS component should be added
		REQUIRE( scene.hasComponent<Selected>( entity1 ) );
		auto *selectedComp = scene.getComponent<Selected>( entity1 );
		REQUIRE( selectedComp->isPrimary == true );
	}

	SECTION( "Deselection" )
	{
		selection.select( entity1 );
		selection.deselect( entity1 );

		REQUIRE( selection.getSelectionCount() == 0 );
		REQUIRE_FALSE( selection.isSelected( entity1 ) );
		REQUIRE( selection.getPrimarySelection() == Entity{} );

		// ECS component should be removed
		REQUIRE_FALSE( scene.hasComponent<Selected>( entity1 ) );
	}

	SECTION( "Replace selection (non-additive)" )
	{
		selection.select( entity1 );
		selection.select( entity2, false ); // Replace, not additive

		REQUIRE( selection.getSelectionCount() == 1 );
		REQUIRE_FALSE( selection.isSelected( entity1 ) );
		REQUIRE( selection.isSelected( entity2 ) );
		REQUIRE( selection.getPrimarySelection() == entity2 );
	}

	SECTION( "Deselect all" )
	{
		selection.select( entity1 );
		selection.select( entity2, true );

		REQUIRE( selection.getSelectionCount() == 2 );

		selection.deselectAll();

		REQUIRE( selection.getSelectionCount() == 0 );
		REQUIRE_FALSE( selection.isSelected( entity1 ) );
		REQUIRE_FALSE( selection.isSelected( entity2 ) );
		REQUIRE( selection.getPrimarySelection() == Entity{} );

		// ECS components should be removed
		REQUIRE_FALSE( scene.hasComponent<Selected>( entity1 ) );
		REQUIRE_FALSE( scene.hasComponent<Selected>( entity2 ) );
	}
}

TEST_CASE( "SelectionManager - Multi-selection", "[selection][multi]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	auto entity1 = scene.createEntity( "Object1" );
	auto entity2 = scene.createEntity( "Object2" );
	auto entity3 = scene.createEntity( "Object3" );

	SECTION( "Additive selection" )
	{
		selection.select( entity1 );
		selection.select( entity2, true ); // Additive

		REQUIRE( selection.getSelectionCount() == 2 );
		REQUIRE( selection.isSelected( entity1 ) );
		REQUIRE( selection.isSelected( entity2 ) );

		// First selected remains primary
		REQUIRE( selection.getPrimarySelection() == entity1 );

		// Both have Selected components
		REQUIRE( scene.hasComponent<Selected>( entity1 ) );
		REQUIRE( scene.hasComponent<Selected>( entity2 ) );

		// Only primary is marked as primary
		REQUIRE( scene.getComponent<Selected>( entity1 )->isPrimary == true );
		REQUIRE( scene.getComponent<Selected>( entity2 )->isPrimary == false );
	}

	SECTION( "Batch selection" )
	{
		std::vector<Entity> entities = { entity1, entity2, entity3 };
		selection.select( entities, false );

		REQUIRE( selection.getSelectionCount() == 3 );
		for ( auto entity : entities )
		{
			REQUIRE( selection.isSelected( entity ) );
			REQUIRE( scene.hasComponent<Selected>( entity ) );
		}

		// First in list becomes primary
		REQUIRE( selection.getPrimarySelection() == entity1 );
	}

	SECTION( "Toggle selection" )
	{
		selection.select( entity1 );

		selection.toggleSelection( entity2 ); // Add
		REQUIRE( selection.isSelected( entity2 ) );
		REQUIRE( selection.getSelectionCount() == 2 );

		selection.toggleSelection( entity1 ); // Remove
		REQUIRE_FALSE( selection.isSelected( entity1 ) );
		REQUIRE( selection.getSelectionCount() == 1 );
		REQUIRE( selection.getPrimarySelection() == entity2 ); // Primary transferred
	}

	SECTION( "Primary selection management" )
	{
		selection.select( { entity1, entity2, entity3 } );

		REQUIRE( selection.getPrimarySelection() == entity1 );

		// Change primary
		selection.setPrimarySelection( entity2 );
		REQUIRE( selection.getPrimarySelection() == entity2 );

		// Check ECS components reflect primary change
		REQUIRE( scene.getComponent<Selected>( entity1 )->isPrimary == false );
		REQUIRE( scene.getComponent<Selected>( entity2 )->isPrimary == true );
		REQUIRE( scene.getComponent<Selected>( entity3 )->isPrimary == false );
	}
}

TEST_CASE( "SelectionManager - Events", "[selection][events]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	auto entity1 = scene.createEntity( "Object1" );
	auto entity2 = scene.createEntity( "Object2" );

	// Event capture
	SelectionChangedEvent lastEvent;
	bool eventReceived = false;

	selection.registerListener( [&]( const SelectionChangedEvent &event ) {
		lastEvent = event;
		eventReceived = true;
	} );

	SECTION( "Selection event" )
	{
		selection.select( entity1 );

		REQUIRE( eventReceived );
		REQUIRE( lastEvent.previousSelection.empty() );
		REQUIRE( lastEvent.currentSelection.size() == 1 );
		REQUIRE( lastEvent.currentSelection[0] == entity1 );
		REQUIRE( lastEvent.added.size() == 1 );
		REQUIRE( lastEvent.added[0] == entity1 );
		REQUIRE( lastEvent.removed.empty() );
		REQUIRE( lastEvent.newPrimarySelection == entity1 );
	}

	SECTION( "Deselection event" )
	{
		selection.select( entity1 );
		eventReceived = false; // Reset

		selection.deselect( entity1 );

		REQUIRE( eventReceived );
		REQUIRE( lastEvent.previousSelection.size() == 1 );
		REQUIRE( lastEvent.currentSelection.empty() );
		REQUIRE( lastEvent.removed.size() == 1 );
		REQUIRE( lastEvent.removed[0] == entity1 );
		REQUIRE( lastEvent.added.empty() );
		REQUIRE( lastEvent.previousPrimarySelection == entity1 );
		REQUIRE( lastEvent.newPrimarySelection == Entity{} );
	}

	SECTION( "Primary change event" )
	{
		selection.select( { entity1, entity2 } );
		eventReceived = false; // Reset

		selection.setPrimarySelection( entity2 );

		REQUIRE( eventReceived );
		REQUIRE( lastEvent.previousPrimarySelection == entity1 );
		REQUIRE( lastEvent.newPrimarySelection == entity2 );
		REQUIRE( lastEvent.currentSelection.size() == 2 );
		REQUIRE( lastEvent.added.empty() );
		REQUIRE( lastEvent.removed.empty() );
	}
}

TEST_CASE( "SelectionManager - Spatial queries", "[selection][spatial]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	// Create entities at known positions with bounds
	auto entity1 = scene.createEntity( "Cube1" );
	scene.addComponent( entity1, Transform{} );
	auto *transform1 = scene.getComponent<Transform>( entity1 );
	transform1->position = math::Vec3<>{ 0.0f, 0.0f, 0.0f };

	MeshRenderer mesh1;
	mesh1.bounds = math::BoundingBox3Df{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity1, mesh1 );

	auto entity2 = scene.createEntity( "Cube2" );
	scene.addComponent( entity2, Transform{} );
	auto *transform2 = scene.getComponent<Transform>( entity2 );
	transform2->position = math::Vec3<>{ 5.0f, 0.0f, 0.0f };

	MeshRenderer mesh2;
	mesh2.bounds = math::BoundingBox3Df{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( entity2, mesh2 );

	selection.select( { entity1, entity2 } );

	SECTION( "Selection bounds calculation" )
	{
		auto bounds = selection.getSelectionBounds();

		REQUIRE( bounds.isValid() );
		REQUIRE( bounds.min.x == Catch::Approx( -1.0f ) ); // entity1 left edge
		REQUIRE( bounds.max.x == Catch::Approx( 6.0f ) );  // entity2 right edge

		auto center = bounds.center();
		REQUIRE( center.x == Catch::Approx( 2.5f ) ); // Midpoint
	}

	SECTION( "Selection center" )
	{
		auto center = selection.getSelectionCenter();
		REQUIRE( center.x == Catch::Approx( 2.5f ) ); // Average of positions
	}

	SECTION( "Selection radius" )
	{
		auto radius = selection.getSelectionRadius();
		REQUIRE( radius > 0.0f );
		// Should encompass both cubes
		REQUIRE( radius >= 3.5f ); // At least half the distance between centers plus cube size
	}
}

TEST_CASE( "SelectionManager - Validation and cleanup", "[selection][validation]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	auto entity1 = scene.createEntity( "Object1" );
	auto entity2 = scene.createEntity( "Object2" );

	selection.select( { entity1, entity2 } );
	REQUIRE( selection.getSelectionCount() == 2 );

	SECTION( "Validate selection removes invalid entities" )
	{
		// Destroy one entity
		scene.destroyEntity( entity1 );

		// Validate should clean up selection
		selection.validateSelection();

		REQUIRE( selection.getSelectionCount() == 1 );
		REQUIRE_FALSE( selection.isSelected( entity1 ) );
		REQUIRE( selection.isSelected( entity2 ) );
		REQUIRE( selection.getPrimarySelection() == entity2 );
	}

	SECTION( "Refresh from ECS rebuilds selection" )
	{
		// Manually add Selected component
		auto entity3 = scene.createEntity( "Object3" );
		scene.addComponent( entity3, Selected{ false } );

		// Refresh should pick up the new selection
		selection.refreshFromECS();

		REQUIRE( selection.isSelected( entity3 ) );
		REQUIRE( selection.getSelectionCount() == 3 );
	}
}

TEST_CASE( "SelectionManager - Serialization", "[selection][serialization]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	auto entity1 = scene.createEntity( "Object1" );
	auto entity2 = scene.createEntity( "Object2" );
	auto entity3 = scene.createEntity( "Object3" );

	selection.select( { entity1, entity2, entity3 } );
	selection.setPrimarySelection( entity2 );

	SECTION( "Capture and restore selection" )
	{
		auto captured = selection.captureSelection();
		REQUIRE( captured.size() == 3 );

		// Clear selection
		selection.deselectAll();
		REQUIRE( selection.getSelectionCount() == 0 );

		// Restore
		selection.restoreSelection( captured, entity2 );

		REQUIRE( selection.getSelectionCount() == 3 );
		REQUIRE( selection.isSelected( entity1 ) );
		REQUIRE( selection.isSelected( entity2 ) );
		REQUIRE( selection.isSelected( entity3 ) );
		REQUIRE( selection.getPrimarySelection() == entity2 );
	}

	SECTION( "Restore with invalid entities filters them out" )
	{
		auto captured = selection.captureSelection();

		// Destroy one entity
		scene.destroyEntity( entity2 );

		// Restore should only restore valid entities
		selection.restoreSelection( captured, entity2 );

		REQUIRE( selection.getSelectionCount() == 2 );
		REQUIRE( selection.isSelected( entity1 ) );
		REQUIRE( selection.isSelected( entity3 ) );
		REQUIRE( selection.getPrimarySelection() == entity1 ); // Falls back to first valid
	}
}

TEST_CASE( "SelectionManager - Hierarchical transform bounds", "[selection][spatial][hierarchy]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	// Create parent entity at origin
	auto parent = scene.createEntity( "Parent" );
	scene.addComponent( parent, Transform{} );
	auto *parentTransform = scene.getComponent<Transform>( parent );
	parentTransform->position = math::Vec3<>{ 10.0f, 0.0f, 0.0f }; // Parent at (10,0,0)

	MeshRenderer parentMesh;
	parentMesh.bounds = math::BoundingBox3Df{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( parent, parentMesh );

	// Create child entity with local offset
	auto child = scene.createEntity( "Child" );
	scene.addComponent( child, Transform{} );
	auto *childTransform = scene.getComponent<Transform>( child );
	childTransform->position = math::Vec3<>{ 5.0f, 0.0f, 0.0f }; // Local offset (5,0,0)

	// Set up parent-child relationship
	scene.setParent( child, parent );

	MeshRenderer childMesh;
	childMesh.bounds = math::BoundingBox3Df{
		math::Vec3<>{ -1.0f, -1.0f, -1.0f },
		math::Vec3<>{ 1.0f, 1.0f, 1.0f }
	};
	scene.addComponent( child, childMesh );

	// Select only the child
	selection.select( child );

	// Update the transform system to ensure world matrices are computed
	systemManager.update( scene, 0.016f );

	SECTION( "Child bounds should use world transform (parent + local)" )
	{
		auto bounds = selection.getSelectionBounds();

		REQUIRE( bounds.isValid() );

		// Child should be at world position (15, 0, 0) = parent(10,0,0) + local(5,0,0)
		// With bounds extending from (14, -1, -1) to (16, 1, 1)
		REQUIRE( bounds.min.x == Catch::Approx( 14.0f ) ); // 15.0 - 1.0
		REQUIRE( bounds.max.x == Catch::Approx( 16.0f ) ); // 15.0 + 1.0

		auto center = bounds.center();
		REQUIRE( center.x == Catch::Approx( 15.0f ) ); // Should be at world position
	}
}

TEST_CASE( "SelectionManager - Edge cases", "[selection][edge-cases]" )
{
	Scene scene;
	SystemManager systemManager;
	systemManager.addSystem<TransformSystem>();
	systemManager.initialize( scene );
	SelectionManager selection( scene, systemManager );

	SECTION( "Select invalid entity does nothing" )
	{
		Entity invalidEntity{ 999 }; // Non-existent entity

		selection.select( invalidEntity );
		REQUIRE( selection.getSelectionCount() == 0 );
	}

	SECTION( "Deselect non-selected entity does nothing" )
	{
		auto entity = scene.createEntity( "Object" );

		selection.deselect( entity ); // Not selected
		REQUIRE( selection.getSelectionCount() == 0 );
	}

	SECTION( "Set primary on non-selected entity fails" )
	{
		auto entity = scene.createEntity( "Object" );

		selection.setPrimarySelection( entity ); // Not selected
		REQUIRE( selection.getPrimarySelection() == Entity{} );
	}

	SECTION( "Empty selection has no bounds" )
	{
		auto bounds = selection.getSelectionBounds();
		REQUIRE_FALSE( bounds.isValid() );

		auto center = selection.getSelectionCenter();
		REQUIRE( center.x == 0.0f );
		REQUIRE( center.y == 0.0f );
		REQUIRE( center.z == 0.0f );

		auto radius = selection.getSelectionRadius();
		REQUIRE( radius == 0.0f );
	}

	SECTION( "Multiple listeners all receive events" )
	{
		auto entity = scene.createEntity( "Object" );

		int listener1Called = 0;
		int listener2Called = 0;

		selection.registerListener( [&]( const SelectionChangedEvent & ) { listener1Called++; } );
		selection.registerListener( [&]( const SelectionChangedEvent & ) { listener2Called++; } );

		selection.select( entity );

		REQUIRE( listener1Called == 1 );
		REQUIRE( listener2Called == 1 );

		selection.deselect( entity );

		REQUIRE( listener1Called == 2 );
		REQUIRE( listener2Called == 2 );
	}
}
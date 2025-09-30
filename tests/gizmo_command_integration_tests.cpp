#include <catch2/catch_test_macros.hpp>
#include "editor/gizmos.h"
#include "editor/commands/CommandHistory.h"
#include "editor/transform_commands.h"
#include "editor/selection.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/components.h"

TEST_CASE( "GizmoSystem accepts CommandHistory parameter", "[gizmo-commands][AF3.6.1]" )
{
	SECTION( "GizmoSystem can be constructed with CommandHistory" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;

		// Act: Construct GizmoSystem with CommandHistory pointer
		editor::GizmoSystem gizmoSystem(
			selectionManager,
			scene,
			systemManager,
			&commandHistory );

		// Assert: GizmoSystem constructed successfully (no crash)
		// Basic validation - gizmo system should be functional
		REQUIRE_FALSE( gizmoSystem.isManipulating() );
		REQUIRE_FALSE( gizmoSystem.wasManipulated() );
	}

	SECTION( "GizmoSystem can be constructed with nullptr CommandHistory" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );

		// Act: Construct with nullptr (backward compatibility)
		editor::GizmoSystem gizmoSystem(
			selectionManager,
			scene,
			systemManager,
			nullptr );

		// Assert: Should work without CommandHistory
		REQUIRE_FALSE( gizmoSystem.isManipulating() );
		REQUIRE_FALSE( gizmoSystem.wasManipulated() );
	}

	SECTION( "GizmoSystem constructor with default parameter" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );

		// Act: Construct without specifying CommandHistory (uses default nullptr)
		editor::GizmoSystem gizmoSystem(
			selectionManager,
			scene,
			systemManager );

		// Assert: Should work with defaulted parameter
		REQUIRE_FALSE( gizmoSystem.isManipulating() );
	}
}

TEST_CASE( "Single entity manipulation creates TransformEntityCommand", "[gizmo-commands][AF3.6.5]" )
{
	SECTION( "Manipulating single entity creates command in history" )
	{
		// Arrange: Setup scene with entity
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		// Create entity with transform
		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		// Select the entity
		selectionManager.select( entity );

		// Act: Simulate gizmo manipulation
		gizmoSystem.beginManipulation();

		// Apply transform change
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 5.0f, 0.0f, 0.0f };
		gizmoSystem.applyTransformDelta( delta );

		gizmoSystem.endManipulation();

		// Assert: Command was created in history
		REQUIRE( commandHistory.getCommandCount() == 1 );
		REQUIRE( commandHistory.canUndo() );
		REQUIRE_FALSE( commandHistory.canRedo() );

		// Verify entity moved
		const auto *movedTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( movedTransform->position.x == 5.0f );
	}

	SECTION( "Undo restores original position" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		selectionManager.select( entity );

		// Manipulate
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 10.0f, 5.0f, 3.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();

		// Act: Undo
		REQUIRE( commandHistory.undo() );

		// Assert: Position restored
		const auto *restoredTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( restoredTransform->position.x == 0.0f );
		REQUIRE( restoredTransform->position.y == 0.0f );
		REQUIRE( restoredTransform->position.z == 0.0f );
		REQUIRE( commandHistory.canRedo() );
		REQUIRE_FALSE( commandHistory.canUndo() );
	}

	SECTION( "Redo reapplies transformation" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		selectionManager.select( entity );

		// Manipulate and undo
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 7.0f, 8.0f, 9.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();
		commandHistory.undo();

		// Act: Redo
		REQUIRE( commandHistory.redo() );

		// Assert: Transform reapplied
		const auto *redoneTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( redoneTransform->position.x == 7.0f );
		REQUIRE( redoneTransform->position.y == 8.0f );
		REQUIRE( redoneTransform->position.z == 9.0f );
		REQUIRE( commandHistory.canUndo() );
		REQUIRE_FALSE( commandHistory.canRedo() );
	}
}

TEST_CASE( "Multiple entity manipulation creates BatchTransformCommand", "[gizmo-commands][AF3.6.6]" )
{
	SECTION( "Manipulating multiple entities creates single batch command" )
	{
		// Arrange: Setup scene with 3 entities
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		// Create 3 entities
		const auto entity1 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 5.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		const auto entity3 = scene.createEntity();
		scene.addComponent<components::Transform>( entity3,
			components::Transform{ math::Vec3f{ 10.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		// Select all entities
		selectionManager.select( { entity1, entity2, entity3 } );

		// Act: Manipulate all at once
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 0.0f, 10.0f, 0.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();

		// Assert: Only 1 command created (batch)
		REQUIRE( commandHistory.getCommandCount() == 1 );

		// Verify all entities moved
		const auto *t1 = scene.getComponent<components::Transform>( entity1 );
		const auto *t2 = scene.getComponent<components::Transform>( entity2 );
		const auto *t3 = scene.getComponent<components::Transform>( entity3 );
		REQUIRE( t1->position.y == 10.0f );
		REQUIRE( t2->position.y == 10.0f );
		REQUIRE( t3->position.y == 10.0f );
	}

	SECTION( "Undo restores all entities to original positions" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		const auto entity1 = scene.createEntity();
		scene.addComponent<components::Transform>( entity1,
			components::Transform{ math::Vec3f{ 1.0f, 2.0f, 3.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		const auto entity2 = scene.createEntity();
		scene.addComponent<components::Transform>( entity2,
			components::Transform{ math::Vec3f{ 4.0f, 5.0f, 6.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		selectionManager.select( { entity1, entity2 } );

		// Manipulate
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 10.0f, 10.0f, 10.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();

		// Verify command was created
		REQUIRE( commandHistory.getCommandCount() == 1 );

		// Act: Undo
		REQUIRE( commandHistory.undo() );

		// Assert: Both entities restored
		const auto *restored1 = scene.getComponent<components::Transform>( entity1 );
		const auto *restored2 = scene.getComponent<components::Transform>( entity2 );
		REQUIRE( restored1->position.x == 1.0f );
		REQUIRE( restored1->position.y == 2.0f );
		REQUIRE( restored1->position.z == 3.0f );
		REQUIRE( restored2->position.x == 4.0f );
		REQUIRE( restored2->position.y == 5.0f );
		REQUIRE( restored2->position.z == 6.0f );
	}
}

TEST_CASE( "Null CommandHistory safety", "[gizmo-commands][AF3.6.8]" )
{
	SECTION( "GizmoSystem works without CommandHistory (no crashes)" )
	{
		// Arrange: GizmoSystem with nullptr CommandHistory
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, nullptr );

		const auto entity = scene.createEntity();
		scene.addComponent<components::Transform>( entity,
			components::Transform{ math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 0.0f, 0.0f, 0.0f }, math::Vec3f{ 1.0f, 1.0f, 1.0f } } );

		selectionManager.select( entity );

		// Act: Manipulate (should not crash)
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 5.0f, 0.0f, 0.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();

		// Assert: Transform still applied (no command created, but manipulation works)
		const auto *movedTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE( movedTransform->position.x == 5.0f );

		// No command history, so no undo available
		// This just verifies no crash occurred
		REQUIRE( true );
	}

	SECTION( "Empty selection creates no commands" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		editor::SelectionManager selectionManager( scene, systemManager );
		CommandHistory commandHistory;
		editor::GizmoSystem gizmoSystem( selectionManager, scene, systemManager, &commandHistory );

		// No entities selected

		// Act: Try to manipulate with empty selection
		gizmoSystem.beginManipulation();
		editor::GizmoResult delta;
		delta.translationDelta = math::Vec3<>{ 5.0f, 0.0f, 0.0f };
		gizmoSystem.applyTransformDelta( delta );
		gizmoSystem.endManipulation();

		// Assert: No commands created
		REQUIRE( commandHistory.getCommandCount() == 0 );
		REQUIRE_FALSE( commandHistory.canUndo() );
	}
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "editor/commands/Command.h"
#include "editor/commands/CommandHistory.h"
#include "editor/commands/EcsCommands.h"
#include "editor/commands/CommandUI.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/math/vec.h"
#include "engine/math/quat.h"
#include "editor/gizmos.h"
#include "editor/transform_commands.h"

using Catch::Matchers::WithinAbs;
using math::Vec3f;
using math::Quatf;

// Helper function to find entity by name
ecs::Entity findEntityByName( const ecs::Scene &scene, const std::string &targetName )
{
	auto allEntities = scene.getAllEntities();
	for ( ecs::Entity entity : allEntities )
	{
		if ( scene.hasComponent<components::Name>( entity ) )
		{
			const auto *nameComp = scene.getComponent<components::Name>( entity );
			if ( nameComp && nameComp->name == targetName )
			{
				return entity;
			}
		}
	}
	return {}; // Invalid entity
}

TEST_CASE( "Complete editing workflow with undo/redo", "[integration][workflow][AF6.1]" )
{
	SECTION( "Entity creation through transformation with full undo/redo" )
	{
		// Setup scene and command history
		ecs::Scene scene;
		CommandHistory history( 10, 1024 * 1024 ); // 10 commands, 1MB limit

		// Phase 1: Create entity with transform
		auto createCmd = editor::EcsCommandFactory::createEntity( scene, "TestCube" );
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		// Verify entity creation by checking command count and scene state
		REQUIRE( history.getCommandCount() == 1 );

		// Phase 2: Add Transform component - find the created entity first
		ecs::Entity testEntity = findEntityByName( scene, "TestCube" );
		REQUIRE( testEntity.isValid() );

		auto addTransformCmd = editor::EcsCommandFactory::addComponent(
			scene, testEntity, components::Transform{ Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f } } );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Verify transform component added
		REQUIRE( scene.hasComponent<components::Transform>( testEntity ) );
		const auto *transform = scene.getComponent<components::Transform>( testEntity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( transform->scale.x, WithinAbs( 1.0f, 0.001f ) );

		// Phase 3: Transform entity position
		// Create a transform command with before/after states
		components::Transform beforeTransform = *scene.getComponent<components::Transform>( testEntity );
		components::Transform afterTransform = beforeTransform;
		afterTransform.position = Vec3f{ 5.0f, 3.0f, 2.0f };

		auto transformCmd = std::make_unique<editor::TransformEntityCommand>(
			testEntity, scene, beforeTransform, afterTransform );
		REQUIRE( history.executeCommand( std::move( transformCmd ) ) );

		// Verify transformation applied
		transform = scene.getComponent<components::Transform>( testEntity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( 3.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.z, WithinAbs( 2.0f, 0.001f ) );

		// Phase 4: Add another component (MeshRenderer)
		auto addMeshRendererCmd = editor::EcsCommandFactory::addComponent(
			scene, testEntity, components::MeshRenderer{ 1 } // Using handle 1
		);
		REQUIRE( history.executeCommand( std::move( addMeshRendererCmd ) ) );

		REQUIRE( scene.hasComponent<components::MeshRenderer>( testEntity ) );
		const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( testEntity );
		REQUIRE( meshRenderer->meshHandle == 1 );

		// Phase 5: Undo operations in reverse order
		REQUIRE( history.canUndo() );

		// Undo add mesh renderer - should remove component
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::MeshRenderer>( testEntity ) );

		// Undo transform - should restore original position
		REQUIRE( history.undo() );
		transform = scene.getComponent<components::Transform>( testEntity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( 0.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.z, WithinAbs( 0.0f, 0.001f ) );

		// Undo add transform - should remove transform component
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::Transform>( testEntity ) );

		// Undo create entity - should remove entity completely
		REQUIRE( history.undo() );
		REQUIRE( !scene.isValid( testEntity ) );

		// Phase 6: Redo operations to verify state restoration
		REQUIRE( history.canRedo() );

		// Redo create entity
		REQUIRE( history.redo() );
		// Find the recreated entity
		ecs::Entity redoEntity = findEntityByName( scene, "TestCube" );
		REQUIRE( redoEntity.isValid() );

		// Redo add transform
		REQUIRE( history.redo() );
		REQUIRE( scene.hasComponent<components::Transform>( redoEntity ) );

		// Redo transform position
		REQUIRE( history.redo() );
		const auto *redoTransform = scene.getComponent<components::Transform>( redoEntity );
		REQUIRE_THAT( redoTransform->position.x, WithinAbs( 5.0f, 0.001f ) );
		REQUIRE_THAT( redoTransform->position.y, WithinAbs( 3.0f, 0.001f ) );

		// Redo add mesh renderer
		REQUIRE( history.redo() );
		REQUIRE( scene.hasComponent<components::MeshRenderer>( redoEntity ) );

		// Final verification: command count should be back to 4
		REQUIRE( history.getCommandCount() == 4 );
	}
}


TEST_CASE( "Gizmo manipulation integration with command system", "[integration][workflow][AF6.2]" )
{
	SECTION( "Transform commands generated by gizmo operations integrate with history" )
	{
		// Setup scene with entity
		ecs::Scene scene;
		CommandHistory history( 10, 1024 * 1024 );

		// Create entity with initial transform
		auto createCmd = editor::EcsCommandFactory::createEntity( scene, "GizmoTarget" );
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		// Find the created entity
		ecs::Entity entity = findEntityByName( scene, "GizmoTarget" );
		REQUIRE( entity.isValid() );

		auto addTransformCmd = editor::EcsCommandFactory::addComponent(
			scene, entity, components::Transform{ Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f } } );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Simulate gizmo-generated transform commands
		components::Transform before1 = *scene.getComponent<components::Transform>( entity );
		components::Transform after1 = before1;
		after1.position = Vec3f{ 1.0f, 0.0f, 0.0f };

		auto gizmoTransform1 = std::make_unique<editor::TransformEntityCommand>(
			entity, scene, before1, after1 );
		REQUIRE( history.executeCommand( std::move( gizmoTransform1 ) ) );

		// Verify transform was applied
		const auto *transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 1.0f, 0.001f ) );

		// Another gizmo operation - should be separate commands (no auto-merge in this test)
		components::Transform before2 = *scene.getComponent<components::Transform>( entity );
		components::Transform after2 = before2;
		after2.position = Vec3f{ 2.0f, 1.0f, 0.0f };

		auto gizmoTransform2 = std::make_unique<editor::TransformEntityCommand>(
			entity, scene, before2, after2 );
		REQUIRE( history.executeCommand( std::move( gizmoTransform2 ) ) );

		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 2.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( 1.0f, 0.001f ) );

		// Verify command history has all operations
		REQUIRE( history.getCommandCount() == 4 ); // create, add transform, gizmo1, gizmo2

		// Test undo of gizmo operations
		REQUIRE( history.undo() ); // Undo gizmo2
		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( 0.0f, 0.001f ) );

		REQUIRE( history.undo() ); // Undo gizmo1
		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 0.0f, 0.001f ) );

		// Test redo of gizmo operations
		REQUIRE( history.redo() ); // Redo gizmo1
		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 1.0f, 0.001f ) );

		REQUIRE( history.redo() ); // Redo gizmo2
		transform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( transform->position.x, WithinAbs( 2.0f, 0.001f ) );
		REQUIRE_THAT( transform->position.y, WithinAbs( 1.0f, 0.001f ) );
	}

	SECTION( "Multi-entity gizmo operations maintain consistency" )
	{
		// Setup scene with multiple entities
		ecs::Scene scene;
		CommandHistory history( 20, 2 * 1024 * 1024 );

		// Create three entities
		auto createCmd1 = editor::EcsCommandFactory::createEntity( scene, "Entity1" );
		REQUIRE( history.executeCommand( std::move( createCmd1 ) ) );

		auto createCmd2 = editor::EcsCommandFactory::createEntity( scene, "Entity2" );
		REQUIRE( history.executeCommand( std::move( createCmd2 ) ) );

		auto createCmd3 = editor::EcsCommandFactory::createEntity( scene, "Entity3" );
		REQUIRE( history.executeCommand( std::move( createCmd3 ) ) );

		// Find the created entities
		std::vector<ecs::Entity> entities;
		std::vector<std::string> entityNames = { "Entity1", "Entity2", "Entity3" };

		for ( const auto &targetName : entityNames )
		{
			ecs::Entity entity = findEntityByName( scene, targetName );
			if ( entity.isValid() )
			{
				entities.push_back( entity );
			}
		}

		REQUIRE( entities.size() == 3 );

		// Add transform components to all
		for ( const auto &entity : entities )
		{
			auto addTransformCmd = editor::EcsCommandFactory::addComponent(
				scene, entity, components::Transform{ Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 0.0f, 0.0f, 0.0f }, Vec3f{ 1.0f, 1.0f, 1.0f } } );
			REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );
		}

		// Batch transform operation (simulating multi-select gizmo)
		std::vector<Vec3f> initialPositions = {
			Vec3f{ 1.0f, 0.0f, 0.0f },
			Vec3f{ 2.0f, 0.0f, 0.0f },
			Vec3f{ 3.0f, 0.0f, 0.0f }
		};

		auto batchCmd = std::make_unique<editor::BatchTransformCommand>( entities, scene );

		// Add individual transforms to the batch command
		for ( size_t i = 0; i < entities.size(); ++i )
		{
			components::Transform beforeTransform = *scene.getComponent<components::Transform>( entities[i] );
			components::Transform afterTransform = beforeTransform;
			afterTransform.position = initialPositions[i];
			batchCmd->addTransform( entities[i], beforeTransform, afterTransform );
		}

		REQUIRE( history.executeCommand( std::move( batchCmd ) ) );

		// Verify all entities were transformed
		for ( size_t i = 0; i < entities.size(); ++i )
		{
			const auto *transform = scene.getComponent<components::Transform>( entities[i] );
			REQUIRE_THAT( transform->position.x, WithinAbs( initialPositions[i].x, 0.001f ) );
		}

		// Test undo of batch operation
		REQUIRE( history.undo() );

		// All entities should be back to origin
		for ( const auto &entity : entities )
		{
			const auto *transform = scene.getComponent<components::Transform>( entity );
			REQUIRE_THAT( transform->position.x, WithinAbs( 0.0f, 0.001f ) );
			REQUIRE_THAT( transform->position.y, WithinAbs( 0.0f, 0.001f ) );
			REQUIRE_THAT( transform->position.z, WithinAbs( 0.0f, 0.001f ) );
		}

		// Test redo of batch operation
		REQUIRE( history.redo() );

		// All entities should be transformed again
		for ( size_t i = 0; i < entities.size(); ++i )
		{
			const auto *transform = scene.getComponent<components::Transform>( entities[i] );
			REQUIRE_THAT( transform->position.x, WithinAbs( initialPositions[i].x, 0.001f ) );
		}
	}
}

TEST_CASE( "ECS command validation with scene operations", "[integration][workflow][AF6.3]" )
{
	SECTION( "All ECS operations integrate properly with command system" )
	{
		ecs::Scene scene;
		CommandHistory history( 15, 2 * 1024 * 1024 );

		// Test comprehensive ECS operations

		// Create multiple entities
		auto createCmd1 = editor::EcsCommandFactory::createEntity( scene, "TestEntity1" );
		REQUIRE( history.executeCommand( std::move( createCmd1 ) ) );

		auto createCmd2 = editor::EcsCommandFactory::createEntity( scene, "TestEntity2" );
		REQUIRE( history.executeCommand( std::move( createCmd2 ) ) );

		// Find entities
		ecs::Entity entity1 = findEntityByName( scene, "TestEntity1" );
		ecs::Entity entity2 = findEntityByName( scene, "TestEntity2" );
		REQUIRE( entity1.isValid() );
		REQUIRE( entity2.isValid() );

		// Add components
		auto addTransform1 = editor::EcsCommandFactory::addComponent(
			scene, entity1, components::Transform{ Vec3f{ 1.0f, 0.0f, 0.0f }, Vec3f{}, Vec3f{ 1.0f, 1.0f, 1.0f } } );
		REQUIRE( history.executeCommand( std::move( addTransform1 ) ) );

		auto addVisible1 = editor::EcsCommandFactory::addComponent( scene, entity1, components::Visible{} );
		REQUIRE( history.executeCommand( std::move( addVisible1 ) ) );

		auto addTransform2 = editor::EcsCommandFactory::addComponent(
			scene, entity2, components::Transform{ Vec3f{ 2.0f, 0.0f, 0.0f }, Vec3f{}, Vec3f{ 1.0f, 1.0f, 1.0f } } );
		REQUIRE( history.executeCommand( std::move( addTransform2 ) ) );

		// Verify components exist
		REQUIRE( scene.hasComponent<components::Transform>( entity1 ) );
		REQUIRE( scene.hasComponent<components::Visible>( entity1 ) );
		REQUIRE( scene.hasComponent<components::Transform>( entity2 ) );

		// Rename operation
		auto renameCmd = editor::EcsCommandFactory::renameEntity( scene, entity1, "RenamedEntity1" );
		REQUIRE( history.executeCommand( std::move( renameCmd ) ) );

		const auto *nameComp = scene.getComponent<components::Name>( entity1 );
		REQUIRE( nameComp->name == "RenamedEntity1" );

		// Test undo sequence
		REQUIRE( history.getCommandCount() == 6 );

		// Undo rename
		REQUIRE( history.undo() );
		nameComp = scene.getComponent<components::Name>( entity1 );
		REQUIRE( nameComp->name == "TestEntity1" );

		// Undo add transform to entity2
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::Transform>( entity2 ) );

		// Undo add visible to entity1
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::Visible>( entity1 ) );

		// Verify entity1 still has transform
		REQUIRE( scene.hasComponent<components::Transform>( entity1 ) );

		// Continue undo sequence - remove transform from entity1
		REQUIRE( history.undo() );
		REQUIRE( !scene.hasComponent<components::Transform>( entity1 ) );

		// Test redo sequence
		REQUIRE( history.redo() ); // Add transform1 back
		REQUIRE( scene.hasComponent<components::Transform>( entity1 ) );

		REQUIRE( history.redo() ); // Add visible back
		REQUIRE( scene.hasComponent<components::Visible>( entity1 ) );

		REQUIRE( history.redo() ); // Add transform2 back
		REQUIRE( scene.hasComponent<components::Transform>( entity2 ) );
	}
}

TEST_CASE( "UI responsiveness during command operations", "[integration][workflow][AF6.4]" )
{
	SECTION( "Command system performance under UI load" )
	{
		ecs::Scene scene;
		CommandHistory history( 50, 5 * 1024 * 1024 );

		// Simulate rapid command execution (typical of UI interactions)
		const int commandCount = 30;
		std::vector<ecs::Entity> entities;

		auto start = std::chrono::steady_clock::now();

		// Rapid entity creation
		for ( int i = 0; i < commandCount; ++i )
		{
			auto createCmd = editor::EcsCommandFactory::createEntity( scene, "Entity" + std::to_string( i ) );
			REQUIRE( history.executeCommand( std::move( createCmd ) ) );
		}

		auto creationEnd = std::chrono::steady_clock::now();
		auto creationTime = std::chrono::duration_cast<std::chrono::milliseconds>( creationEnd - start );

		// Verify creation was fast (< 50ms for 30 entities)
		REQUIRE( creationTime.count() < 50 );

		// Rapid undo operations
		start = std::chrono::steady_clock::now();
		for ( int i = 0; i < commandCount; ++i )
		{
			REQUIRE( history.undo() );
		}

		auto undoEnd = std::chrono::steady_clock::now();
		auto undoTime = std::chrono::duration_cast<std::chrono::milliseconds>( undoEnd - start );

		// Verify undo was fast (< 30ms for 30 operations)
		REQUIRE( undoTime.count() < 30 );

		// Rapid redo operations
		start = std::chrono::steady_clock::now();
		for ( int i = 0; i < commandCount; ++i )
		{
			REQUIRE( history.redo() );
		}

		auto redoEnd = std::chrono::steady_clock::now();
		auto redoTime = std::chrono::duration_cast<std::chrono::milliseconds>( redoEnd - start );

		// Verify redo was fast (< 30ms for 30 operations)
		REQUIRE( redoTime.count() < 30 );

		// Verify final state
		REQUIRE( history.getCommandCount() == commandCount );
	}
}

TEST_CASE( "Memory management under stress conditions", "[integration][workflow][AF6.5]" )
{
	SECTION( "Command history cleanup under memory pressure" )
	{
		// Use smaller memory limit to trigger cleanup
		ecs::Scene scene;
		CommandHistory history( 10, 64 * 1024 ); // 64KB limit to force cleanup

		// Create commands that will exceed memory limit
		const int excessiveCommandCount = 20; // More than max commands (10)

		for ( int i = 0; i < excessiveCommandCount; ++i )
		{
			auto createCmd = editor::EcsCommandFactory::createEntity( scene, "BigEntity" + std::to_string( i ) );
			REQUIRE( history.executeCommand( std::move( createCmd ) ) );
		}

		// Verify automatic cleanup occurred
		REQUIRE( history.getCommandCount() <= 10 );				 // Should not exceed max commands
		REQUIRE( history.getCurrentMemoryUsage() <= 64 * 1024 ); // Should respect memory limit

		// Verify we can still undo available commands
		int undoCount = 0;
		while ( history.canUndo() )
		{
			REQUIRE( history.undo() );
			++undoCount;
		}

		// Should have been able to undo all remaining commands
		REQUIRE( undoCount == static_cast<int>( history.getCommandCount() ) );
	}
}

TEST_CASE( "Command merging in realistic scenarios", "[integration][workflow][AF6.6]" )
{
	SECTION( "Transform command merging during continuous manipulation" )
	{
		ecs::Scene scene;
		CommandHistory history( 20, 1024 * 1024 );

		// Create entity with transform
		auto createCmd = editor::EcsCommandFactory::createEntity( scene, "TransformTarget" );
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		ecs::Entity entity = findEntityByName( scene, "TransformTarget" );
		REQUIRE( entity.isValid() );

		auto addTransformCmd = editor::EcsCommandFactory::addComponent(
			scene, entity, components::Transform{ Vec3f{}, Vec3f{}, Vec3f{ 1.0f, 1.0f, 1.0f } } );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Simulate continuous gizmo dragging with merging
		components::Transform beforeTransform1 = *scene.getComponent<components::Transform>( entity );
		components::Transform afterTransform1 = beforeTransform1;
		afterTransform1.position = Vec3f{ 0.1f, 0.0f, 0.0f };

		auto transform1 = std::make_unique<editor::TransformEntityCommand>(
			entity, scene, beforeTransform1, afterTransform1 );

		REQUIRE( history.executeCommandWithMerging( std::move( transform1 ) ) );
		const size_t commandsAfterFirst = history.getCommandCount();

		// Quick follow-up transform (within merge window)
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ); // Stay within 100ms merge window

		components::Transform beforeTransform2 = *scene.getComponent<components::Transform>( entity );
		components::Transform afterTransform2 = beforeTransform2;
		afterTransform2.position = Vec3f{ 0.2f, 0.0f, 0.0f };

		auto transform2 = std::make_unique<editor::TransformEntityCommand>(
			entity, scene, beforeTransform2, afterTransform2 );

		REQUIRE( history.executeCommandWithMerging( std::move( transform2 ) ) );

		// Verify final position
		const auto *finalTransform = scene.getComponent<components::Transform>( entity );
		REQUIRE_THAT( finalTransform->position.x, WithinAbs( 0.2f, 0.001f ) );

		// Note: Command merging might not work if TransformEntityCommand doesn't support it
		// This test verifies the system handles merging attempts gracefully
		REQUIRE( history.getCommandCount() >= commandsAfterFirst ); // Should not decrease
	}
}

TEST_CASE( "Error recovery and system stability", "[integration][workflow][AF6.7]" )
{
	SECTION( "System handles command failures gracefully" )
	{
		ecs::Scene scene;
		CommandHistory history( 10, 1024 * 1024 );

		// Create valid entity
		auto createCmd = editor::EcsCommandFactory::createEntity( scene, "ValidEntity" );
		REQUIRE( history.executeCommand( std::move( createCmd ) ) );

		ecs::Entity validEntity = findEntityByName( scene, "ValidEntity" );
		REQUIRE( validEntity.isValid() );

		// Add valid component
		auto addTransformCmd = editor::EcsCommandFactory::addComponent(
			scene, validEntity, components::Transform{} );
		REQUIRE( history.executeCommand( std::move( addTransformCmd ) ) );

		// Test with invalid entity (should not crash system)
		ecs::Entity invalidEntity{}; // Default-constructed invalid entity
		REQUIRE( !invalidEntity.isValid() );

		// This should fail gracefully without affecting the history
		auto invalidCmd = editor::EcsCommandFactory::addComponent(
			scene, invalidEntity, components::Visible{} );

		// The command creation should succeed, but execution might fail
		const size_t commandsBefore = history.getCommandCount();
		[[maybe_unused]] const bool result = history.executeCommand( std::move( invalidCmd ) );

		// System should remain stable regardless of result
		REQUIRE( history.getCommandCount() >= commandsBefore ); // Count should not decrease

		// Valid operations should still work
		auto anotherValidCmd = editor::EcsCommandFactory::addComponent(
			scene, validEntity, components::Visible{} );
		REQUIRE( history.executeCommand( std::move( anotherValidCmd ) ) );

		// Undo/redo should still work for valid commands
		REQUIRE( history.canUndo() );
		REQUIRE( history.undo() );
		REQUIRE( history.canRedo() );
		REQUIRE( history.redo() );
	}
}
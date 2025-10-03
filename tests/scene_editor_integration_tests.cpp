#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// Test complete scene editor workflows
#include "runtime/ecs.h"
#include "runtime/scene_serialization/SceneSerializer.h"
#include "editor/commands/EcsCommands.h"
#include "editor/commands/CommandHistory.h"
#include "editor/transform_commands.h"
#include "runtime/components.h"
#include "engine/math/vec.h"
#include "engine/math/quat.h"

#include <filesystem>
#include <memory>

using Catch::Matchers::WithinAbs;

namespace fs = std::filesystem;

TEST_CASE( "Complete editing workflow: Create → Transform → Save → Load", "[T7.1][integration][workflow][AF1]" )
{
	// Arrange: Setup scene and command history
	ecs::Scene scene;
	CommandHistory history;

	// Create a temporary file path for testing
	const auto testScenePath = fs::temp_directory_path() / "test_scene_workflow.scene";

	// Cleanup on exit
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup(); // Remove if exists from previous run

	SECTION( "Create entity with transform component" )
	{
		// Act: Create entity via command
		auto createCmd = std::make_unique<editor::CreateEntityCommand>( scene, "TestCube" );
		history.executeCommand( std::move( createCmd ) );

		// Get the entity from the scene
		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() == 1 );
		const auto entity = entities[0];

		// Assert: Entity exists
		REQUIRE( scene.isValid( entity ) );
		REQUIRE( scene.hasComponent<components::Name>( entity ) );
		const auto *name = scene.getComponent<components::Name>( entity );
		REQUIRE( name != nullptr );
		REQUIRE( name->name == "TestCube" );

		// Add Transform component
		components::Transform transform;
		transform.position = { 1.0f, 2.0f, 3.0f };
		auto addTransformCmd = std::make_unique<editor::AddComponentCommand<components::Transform>>( scene, entity, transform );
		history.executeCommand( std::move( addTransformCmd ) );

		REQUIRE( scene.hasComponent<components::Transform>( entity ) );
		const auto *transformComp = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformComp != nullptr );
		REQUIRE_THAT( transformComp->position.x, WithinAbs( 1.0f, 0.001f ) );
		REQUIRE_THAT( transformComp->position.y, WithinAbs( 2.0f, 0.001f ) );
		REQUIRE_THAT( transformComp->position.z, WithinAbs( 3.0f, 0.001f ) );
	}

	SECTION( "Save scene and verify file exists" )
	{
		// Arrange: Create entity with transform
		auto createCmd = std::make_unique<editor::CreateEntityCommand>( scene, "SavedCube" );
		history.executeCommand( std::move( createCmd ) );

		const auto entities = scene.getAllEntities();
		REQUIRE( entities.size() == 1 );
		const auto entity = entities[0];

		components::Transform transform;
		transform.position = { 5.0f, 10.0f, 15.0f };
		auto addTransformCmd = std::make_unique<editor::AddComponentCommand<components::Transform>>( scene, entity, transform );
		history.executeCommand( std::move( addTransformCmd ) );

		auto *transformComp = scene.getComponent<components::Transform>( entity );
		REQUIRE( transformComp != nullptr );
		REQUIRE( transformComp->position.x == 5.0f );
		REQUIRE( transformComp->position.y == 10.0f );
		REQUIRE( transformComp->position.z == 15.0f );

		// Act: Save scene
		const auto result = scene::SceneSerializer::saveScene( scene, testScenePath );

		// Assert: Save succeeded
		REQUIRE( result.has_value() );
		REQUIRE( fs::exists( testScenePath ) );
		REQUIRE( fs::file_size( testScenePath ) > 0 );

		cleanup();
	}

	SECTION( "Round-trip: Save and load preserves data" )
	{
		// Arrange: Create entity directly (not via commands)
		const auto entity = scene.createEntity( "RoundTripCube" );
		REQUIRE( scene.getAllEntities().size() == 1 );

		components::Transform transform;
		transform.position = { 7.5f, 12.5f, 22.5f };
		transform.scale = { 1.5f, 2.5f, 3.5f };
		scene.addComponent( entity, transform );

		// Add Visible component
		components::Visible visible;
		visible.visible = true;
		scene.addComponent( entity, visible );

		// Act: Save scene
		auto saveResult = scene::SceneSerializer::saveScene( scene, testScenePath );
		REQUIRE( saveResult.has_value() );

		// Destroy all entities (simulate new scene)
		// Copy entities list before destroying to avoid iterator invalidation
		const auto entitiesToDestroy = std::vector<ecs::Entity>( scene.getAllEntities().begin(), scene.getAllEntities().end() );
		for ( const auto e : entitiesToDestroy )
		{
			scene.destroyEntity( e );
		}
		REQUIRE( scene.getEntityCount() == 0 );

		// Load scene back
		auto loadResult = scene::SceneSerializer::loadScene( scene, testScenePath );
		REQUIRE( loadResult.has_value() );

		// Assert: Scene has one entity with correct data
		REQUIRE( scene.getEntityCount() == 1 );

		// Find entity by name
		const auto loadedEntities = scene.getAllEntities();
		REQUIRE( loadedEntities.size() == 1 );
		const auto loadedEntity = loadedEntities[0];

		REQUIRE( scene.isValid( loadedEntity ) );
		REQUIRE( scene.hasComponent<components::Name>( loadedEntity ) );
		const auto *loadedName = scene.getComponent<components::Name>( loadedEntity );
		REQUIRE( loadedName != nullptr );
		REQUIRE( loadedName->name == "RoundTripCube" );

		REQUIRE( scene.hasComponent<components::Transform>( loadedEntity ) );
		const auto *loadedTransform = scene.getComponent<components::Transform>( loadedEntity );
		REQUIRE( loadedTransform != nullptr );
		REQUIRE_THAT( loadedTransform->position.x, WithinAbs( 7.5f, 0.001f ) );
		REQUIRE_THAT( loadedTransform->position.y, WithinAbs( 12.5f, 0.001f ) );
		REQUIRE_THAT( loadedTransform->position.z, WithinAbs( 22.5f, 0.001f ) );
		REQUIRE_THAT( loadedTransform->scale.x, WithinAbs( 1.5f, 0.001f ) );
		REQUIRE_THAT( loadedTransform->scale.y, WithinAbs( 2.5f, 0.001f ) );
		REQUIRE_THAT( loadedTransform->scale.z, WithinAbs( 3.5f, 0.001f ) );

		REQUIRE( scene.hasComponent<components::Visible>( loadedEntity ) );

		cleanup();
	}

	cleanup();
}

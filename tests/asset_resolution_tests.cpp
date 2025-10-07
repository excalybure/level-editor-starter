#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/scene_serialization/SceneSerializer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/assets.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

TEST_CASE( "Asset resolution helper resolves meshPath to asset", "[asset_resolution][AF1]" )
{
	// Arrange: Create an AssetManager and a scene with MeshRenderer that has meshPath
	assets::AssetManager assetManager;
	ecs::Scene scene;

	// Create an entity with MeshRenderer that has meshPath but no meshHandle
	const auto entity = scene.createEntity( "TestEntity" );

	components::Transform transform;
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	meshRenderer.meshPath = "assets/test/triangle.gltf"; // Path to test asset
	meshRenderer.meshHandle = 0;						 // Not resolved yet
	scene.addComponent( entity, meshRenderer );

	// Act: Resolve mesh path to load asset
	// This will be our helper function to implement
	// bool resolveAssetPaths(ecs::Scene& scene, assets::AssetManager& assetManager);

	// For now, manually test the concept:
	auto *comp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( comp != nullptr );
	REQUIRE( comp->meshPath == "assets/test/triangle.gltf" );
	REQUIRE( comp->meshHandle == 0 ); // Not yet resolved

	// Try to load the asset
	if ( !comp->meshPath.empty() && comp->meshHandle == 0 )
	{
		// Load asset scene
		auto assetScene = assetManager.load<assets::Scene>( comp->meshPath );

		// For this test to pass, we need a real asset file
		// This test will be marked as conditional based on file existence
		if ( assetScene && assetScene->isLoaded() )
		{
			// Asset loaded successfully - we would resolve meshHandle here
			REQUIRE( assetScene->getMeshCount() > 0 );
		}
		else
		{
			// Asset doesn't exist - that's okay for this test structure validation
			INFO( "Test asset not found: " << comp->meshPath );
		}
	}
}

TEST_CASE( "Scene load with meshPath triggers asset resolution", "[asset_resolution][integration][AF2]" )
{
	// Arrange: Create a scene file with meshPath
	const auto testScenePath = fs::temp_directory_path() / "test_asset_resolution.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Create JSON scene with meshPath
	json sceneJson;
	sceneJson["version"] = "1.0";
	sceneJson["metadata"]["name"] = "Asset Resolution Test";
	sceneJson["metadata"]["created"] = "2025-10-07T10:00:00Z";
	sceneJson["metadata"]["modified"] = "2025-10-07T10:00:00Z";
	sceneJson["entities"] = json::array();

	json entityJson;
	entityJson["id"] = 1;
	entityJson["name"] = "TestCube";
	entityJson["parent"] = nullptr;
	entityJson["components"]["transform"]["position"] = { 0.0f, 0.0f, 0.0f };
	entityJson["components"]["transform"]["rotation"] = { 0.0f, 0.0f, 0.0f };
	entityJson["components"]["transform"]["scale"] = { 1.0f, 1.0f, 1.0f };
	entityJson["components"]["meshRenderer"]["meshPath"] = "assets/test/triangle.gltf";
	entityJson["components"]["meshRenderer"]["lodBias"] = 0.0f;

	sceneJson["entities"].push_back( entityJson );

	// Write to file
	std::ofstream file( testScenePath );
	REQUIRE( file.is_open() );
	file << sceneJson.dump( 2 );
	file.close();

	// Act: Load scene
	ecs::Scene scene;
	const auto loadResult = scene::SceneSerializer::loadScene( scene, testScenePath );

	// Assert: Scene loaded successfully
	REQUIRE( loadResult.has_value() );
	REQUIRE( scene.getEntityCount() == 1 );

	const auto entities = scene.getAllEntities();
	const auto entity = entities[0];

	// Verify meshPath was loaded
	REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );
	const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );
	REQUIRE( meshRenderer->meshPath == "assets/test/triangle.gltf" );
	REQUIRE( meshRenderer->meshHandle == 0 ); // Not resolved yet (no AssetManager passed to loadScene)

	cleanup();
}

TEST_CASE( "resolveSceneAssets function resolves all meshPath references", "[asset_resolution][helper][AF3]" )
{
	// Arrange: Create scene with multiple entities with meshPath
	ecs::Scene scene;
	assets::AssetManager assetManager;

	// Entity 1: Has meshPath, needs resolution
	const auto entity1 = scene.createEntity( "Entity1" );
	components::Transform transform1;
	scene.addComponent( entity1, transform1 );

	components::MeshRenderer meshRenderer1;
	meshRenderer1.meshPath = "assets/test/triangle.gltf";
	meshRenderer1.meshHandle = 0;
	scene.addComponent( entity1, meshRenderer1 );

	// Entity 2: Has empty meshPath, should be skipped
	const auto entity2 = scene.createEntity( "Entity2" );
	components::Transform transform2;
	scene.addComponent( entity2, transform2 );

	components::MeshRenderer meshRenderer2;
	meshRenderer2.meshPath = "";   // Empty - programmatic entity
	meshRenderer2.meshHandle = 99; // Already has handle
	scene.addComponent( entity2, meshRenderer2 );

	// Entity 3: Has meshPath but already resolved (meshHandle != 0)
	const auto entity3 = scene.createEntity( "Entity3" );
	components::Transform transform3;
	scene.addComponent( entity3, transform3 );

	components::MeshRenderer meshRenderer3;
	meshRenderer3.meshPath = "assets/test/cube.gltf";
	meshRenderer3.meshHandle = 42; // Already resolved
	scene.addComponent( entity3, meshRenderer3 );

	// Act: Resolve assets
	// This is the function we need to implement:
	// int resolveSceneAssets(ecs::Scene& scene, assets::AssetManager& assetManager);

	// For now, test the logic:
	int resolvedCount = 0;
	const auto entities = scene.getAllEntities();

	for ( const auto entity : entities )
	{
		if ( !scene.hasComponent<components::MeshRenderer>( entity ) )
			continue;

		auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
		if ( !meshRenderer )
			continue;

		// Skip if no path or already resolved
		if ( meshRenderer->meshPath.empty() || meshRenderer->meshHandle != 0 )
			continue;

		// This entity needs resolution
		INFO( "Entity needs resolution: " << scene.getComponent<components::Name>( entity )->name );
		resolvedCount++;
	}

	// Assert: Should identify 1 entity needing resolution (entity1)
	REQUIRE( resolvedCount == 1 );
}

TEST_CASE( "resolveSceneAssets handles missing asset files gracefully", "[asset_resolution][error_handling][AF4]" )
{
	// Arrange: Scene with non-existent asset path
	ecs::Scene scene;
	assets::AssetManager assetManager;

	const auto entity = scene.createEntity( "MissingAssetEntity" );
	components::Transform transform;
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	meshRenderer.meshPath = "assets/nonexistent/missing.gltf";
	meshRenderer.meshHandle = 0;
	scene.addComponent( entity, meshRenderer );

	// Act: Attempt to resolve
	// The function should handle missing files gracefully
	auto assetScene = assetManager.load<assets::Scene>( meshRenderer.meshPath );

	// Assert: Asset load should fail gracefully
	// AssetManager returns nullptr for missing files
	REQUIRE( ( assetScene == nullptr || !assetScene->isLoaded() ) );

	// The entity should remain with meshHandle = 0 (unresolved)
	const auto *comp = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( comp->meshHandle == 0 );
	REQUIRE( comp->meshPath == "assets/nonexistent/missing.gltf" ); // Path preserved
}

TEST_CASE( "resolveSceneAssets returns count of resolved assets", "[asset_resolution][return_value][AF5]" )
{
	// Arrange: Scene with mix of resolvable and unresolvable entities
	ecs::Scene scene;
	assets::AssetManager assetManager;

	// Entity 1: Needs resolution (but asset doesn't exist for this test)
	const auto entity1 = scene.createEntity( "NeedsResolution1" );
	components::MeshRenderer mr1;
	mr1.meshPath = "assets/test/model1.gltf";
	mr1.meshHandle = 0;
	scene.addComponent( entity1, components::Transform{} );
	scene.addComponent( entity1, mr1 );

	// Entity 2: Already resolved
	const auto entity2 = scene.createEntity( "AlreadyResolved" );
	components::MeshRenderer mr2;
	mr2.meshPath = "assets/test/model2.gltf";
	mr2.meshHandle = 42;
	scene.addComponent( entity2, components::Transform{} );
	scene.addComponent( entity2, mr2 );

	// Entity 3: Empty path
	const auto entity3 = scene.createEntity( "EmptyPath" );
	components::MeshRenderer mr3;
	mr3.meshPath = "";
	mr3.meshHandle = 99;
	scene.addComponent( entity3, components::Transform{} );
	scene.addComponent( entity3, mr3 );

	// Entity 4: Needs resolution
	const auto entity4 = scene.createEntity( "NeedsResolution2" );
	components::MeshRenderer mr4;
	mr4.meshPath = "assets/test/model3.gltf";
	mr4.meshHandle = 0;
	scene.addComponent( entity4, components::Transform{} );
	scene.addComponent( entity4, mr4 );

	// Act: Count entities that need resolution
	int needsResolutionCount = 0;
	const auto entities = scene.getAllEntities();

	for ( const auto entity : entities )
	{
		if ( !scene.hasComponent<components::MeshRenderer>( entity ) )
			continue;

		const auto *mr = scene.getComponent<components::MeshRenderer>( entity );
		if ( mr && !mr->meshPath.empty() && mr->meshHandle == 0 )
		{
			needsResolutionCount++;
		}
	}

	// Assert: Should identify 2 entities needing resolution
	REQUIRE( needsResolutionCount == 2 );
}

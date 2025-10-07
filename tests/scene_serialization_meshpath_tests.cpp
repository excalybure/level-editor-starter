#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/scene_serialization/SceneSerializer.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

TEST_CASE( "MeshRenderer serialization includes meshPath", "[scene_serialization][meshpath][AF1]" )
{
	// Arrange: Create scene with entity that has MeshRenderer with meshPath
	ecs::Scene scene;
	const auto entity = scene.createEntity( "CubeEntity" );

	// Add Transform component
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f };
	scene.addComponent( entity, transform );

	// Add MeshRenderer with meshPath
	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 42;
	meshRenderer.meshPath = "assets/models/cube.gltf";
	meshRenderer.lodBias = 0.5f;
	scene.addComponent( entity, meshRenderer );

	// Create temporary file path
	const auto testScenePath = fs::temp_directory_path() / "test_meshpath_save.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Act: Save scene
	const auto saveResult = scene::SceneSerializer::saveScene( scene, testScenePath );

	// Assert: Save succeeded
	REQUIRE( saveResult.has_value() );
	REQUIRE( fs::exists( testScenePath ) );

	// Read and parse JSON to verify meshPath is present
	std::ifstream file( testScenePath );
	REQUIRE( file.is_open() );

	json sceneJson;
	file >> sceneJson;
	file.close();

	REQUIRE( sceneJson.contains( "entities" ) );
	REQUIRE( sceneJson["entities"].is_array() );
	REQUIRE( sceneJson["entities"].size() == 1 );

	const auto &entityJson = sceneJson["entities"][0];
	REQUIRE( entityJson.contains( "components" ) );
	REQUIRE( entityJson["components"].contains( "meshRenderer" ) );

	const auto &meshRendererJson = entityJson["components"]["meshRenderer"];

	// Key assertion: meshPath should be present
	REQUIRE( meshRendererJson.contains( "meshPath" ) );
	REQUIRE( meshRendererJson["meshPath"] == "assets/models/cube.gltf" );

	// lodBias should also be present
	REQUIRE( meshRendererJson.contains( "lodBias" ) );
	REQUIRE( meshRendererJson["lodBias"] == 0.5f );

	// meshHandle should NOT be present (replaced by meshPath)
	REQUIRE_FALSE( meshRendererJson.contains( "meshHandle" ) );

	cleanup();
}

TEST_CASE( "MeshRenderer deserialization loads meshPath", "[scene_serialization][meshpath][AF2]" )
{
	// Arrange: Create scene file with meshPath
	const auto testScenePath = fs::temp_directory_path() / "test_meshpath_load.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Create JSON with meshPath
	json sceneJson;
	sceneJson["version"] = "1.0";
	sceneJson["metadata"]["name"] = "Test Scene";
	sceneJson["metadata"]["created"] = "2025-01-20T10:00:00Z";
	sceneJson["metadata"]["modified"] = "2025-01-20T10:00:00Z";
	sceneJson["entities"] = json::array();

	json entityJson;
	entityJson["id"] = 1;
	entityJson["name"] = "SphereEntity";
	entityJson["parent"] = nullptr;
	entityJson["components"]["transform"]["position"] = { 5.0f, 10.0f, 15.0f };
	entityJson["components"]["transform"]["rotation"] = { 0.0f, 0.0f, 0.0f };
	entityJson["components"]["transform"]["scale"] = { 1.0f, 1.0f, 1.0f };
	entityJson["components"]["meshRenderer"]["meshPath"] = "assets/models/sphere.gltf";
	entityJson["components"]["meshRenderer"]["lodBias"] = 1.5f;

	sceneJson["entities"].push_back( entityJson );

	// Write to file
	std::ofstream file( testScenePath );
	REQUIRE( file.is_open() );
	file << sceneJson.dump( 2 );
	file.close();

	// Act: Load scene
	ecs::Scene scene;
	const auto loadResult = scene::SceneSerializer::loadScene( scene, testScenePath );

	// Assert: Load succeeded
	REQUIRE( loadResult.has_value() );
	REQUIRE( scene.getEntityCount() == 1 );

	const auto entities = scene.getAllEntities();
	REQUIRE( entities.size() == 1 );
	const auto entity = entities[0];

	// Verify MeshRenderer component loaded correctly
	REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );
	const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );

	// Key assertion: meshPath should be loaded
	REQUIRE( meshRenderer->meshPath == "assets/models/sphere.gltf" );
	REQUIRE( meshRenderer->lodBias == 1.5f );

	// meshHandle should be 0 (placeholder until asset resolution)
	REQUIRE( meshRenderer->meshHandle == 0 );

	cleanup();
}

TEST_CASE( "MeshRenderer with empty meshPath is supported", "[scene_serialization][meshpath][AF3]" )
{
	// Arrange: Create scene with MeshRenderer without meshPath (programmatically created)
	ecs::Scene scene;
	const auto entity = scene.createEntity( "ProgrammaticEntity" );

	components::Transform transform;
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	meshRenderer.meshHandle = 99;
	meshRenderer.meshPath = ""; // Empty path
	meshRenderer.lodBias = 0.0f;
	scene.addComponent( entity, meshRenderer );

	const auto testScenePath = fs::temp_directory_path() / "test_meshpath_empty.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Act: Save scene
	const auto saveResult = scene::SceneSerializer::saveScene( scene, testScenePath );

	// Assert: Save succeeded
	REQUIRE( saveResult.has_value() );

	// Read JSON to verify behavior with empty meshPath
	std::ifstream file( testScenePath );
	REQUIRE( file.is_open() );
	json sceneJson;
	file >> sceneJson;
	file.close();

	const auto &meshRendererJson = sceneJson["entities"][0]["components"]["meshRenderer"];

	// Empty meshPath should NOT be serialized (omitted)
	REQUIRE_FALSE( meshRendererJson.contains( "meshPath" ) );

	// Should fall back to meshHandle for programmatic entities
	REQUIRE( meshRendererJson.contains( "meshHandle" ) );
	REQUIRE( meshRendererJson["meshHandle"] == 99 );

	cleanup();
}

TEST_CASE( "MeshRenderer backward compatibility with old meshHandle format", "[scene_serialization][meshpath][backward_compat][AF4]" )
{
	// Arrange: Create old-format scene file with meshHandle (no meshPath)
	const auto testScenePath = fs::temp_directory_path() / "test_meshpath_old_format.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Create JSON with OLD format (meshHandle only)
	json sceneJson;
	sceneJson["version"] = "1.0";
	sceneJson["metadata"]["name"] = "Old Format Scene";
	sceneJson["metadata"]["created"] = "2025-01-01T10:00:00Z";
	sceneJson["metadata"]["modified"] = "2025-01-01T10:00:00Z";
	sceneJson["entities"] = json::array();

	json entityJson;
	entityJson["id"] = 1;
	entityJson["name"] = "OldEntity";
	entityJson["parent"] = nullptr;
	entityJson["components"]["transform"]["position"] = { 0.0f, 0.0f, 0.0f };
	entityJson["components"]["transform"]["rotation"] = { 0.0f, 0.0f, 0.0f };
	entityJson["components"]["transform"]["scale"] = { 1.0f, 1.0f, 1.0f };
	entityJson["components"]["meshRenderer"]["meshHandle"] = 123; // Old format
	entityJson["components"]["meshRenderer"]["lodBias"] = 2.0f;

	sceneJson["entities"].push_back( entityJson );

	// Write to file
	std::ofstream file( testScenePath );
	REQUIRE( file.is_open() );
	file << sceneJson.dump( 2 );
	file.close();

	// Act: Load old format scene
	ecs::Scene scene;
	const auto loadResult = scene::SceneSerializer::loadScene( scene, testScenePath );

	// Assert: Load should succeed (backward compatible)
	REQUIRE( loadResult.has_value() );
	REQUIRE( scene.getEntityCount() == 1 );

	const auto entities = scene.getAllEntities();
	const auto entity = entities[0];

	REQUIRE( scene.hasComponent<components::MeshRenderer>( entity ) );
	const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );

	// Old format: meshHandle should be loaded
	REQUIRE( meshRenderer->meshHandle == 123 );
	REQUIRE( meshRenderer->lodBias == 2.0f );

	// meshPath should be empty (old format didn't have it)
	REQUIRE( meshRenderer->meshPath.empty() );

	cleanup();
}

TEST_CASE( "MeshRenderer round-trip preserves meshPath", "[scene_serialization][meshpath][roundtrip][AF5]" )
{
	// Arrange: Create scene with MeshRenderer with meshPath
	ecs::Scene originalScene;
	const auto entity = originalScene.createEntity( "RoundTripEntity" );

	components::Transform transform;
	transform.position = { 3.0f, 4.0f, 5.0f };
	originalScene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	meshRenderer.meshPath = "assets/models/teapot.gltf";
	meshRenderer.lodBias = 0.75f;
	originalScene.addComponent( entity, meshRenderer );

	const auto testScenePath = fs::temp_directory_path() / "test_meshpath_roundtrip.scene";
	const auto cleanup = [&]() {
		if ( fs::exists( testScenePath ) )
		{
			fs::remove( testScenePath );
		}
	};
	cleanup();

	// Act: Save
	const auto saveResult = scene::SceneSerializer::saveScene( originalScene, testScenePath );
	REQUIRE( saveResult.has_value() );

	// Load into new scene
	ecs::Scene loadedScene;
	const auto loadResult = scene::SceneSerializer::loadScene( loadedScene, testScenePath );
	REQUIRE( loadResult.has_value() );

	// Assert: Data preserved
	REQUIRE( loadedScene.getEntityCount() == 1 );
	const auto loadedEntities = loadedScene.getAllEntities();
	const auto loadedEntity = loadedEntities[0];

	REQUIRE( loadedScene.hasComponent<components::MeshRenderer>( loadedEntity ) );
	const auto *loadedMeshRenderer = loadedScene.getComponent<components::MeshRenderer>( loadedEntity );
	REQUIRE( loadedMeshRenderer != nullptr );

	// meshPath preserved
	REQUIRE( loadedMeshRenderer->meshPath == "assets/models/teapot.gltf" );
	REQUIRE( loadedMeshRenderer->lodBias == 0.75f );

	cleanup();
}

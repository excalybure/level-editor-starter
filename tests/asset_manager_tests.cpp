#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import engine.assets;
import engine.asset_manager;
import engine.gltf_loader;
import std;

using namespace assets;

TEST_CASE( "AssetManager can be instantiated", "[AssetManager][unit]" )
{
	AssetManager manager;
	REQUIRE( true ); // Basic instantiation test
}

TEST_CASE( "AssetManager caches assets correctly", "[AssetManager][unit]" )
{
	AssetManager manager;

	SECTION( "Loading same material twice returns same instance" )
	{
		// First load
		const auto material1 = manager.load<Material>( "test_material.mtl" );
		REQUIRE( material1 != nullptr );
		REQUIRE( material1->getPath() == "test_material.mtl" );
		REQUIRE( material1->isLoaded() );

		// Second load should return same instance from cache
		const auto material2 = manager.load<Material>( "test_material.mtl" );
		REQUIRE( material2 != nullptr );
		REQUIRE( material1.get() == material2.get() ); // Same object
		REQUIRE( material1.use_count() == 3 );		   // Both shared_ptrs + cache reference
	}

	SECTION( "Get retrieves cached assets" )
	{
		// Load first
		const auto scene1 = manager.load<Scene>( "test_scene.gltf" );
		REQUIRE( scene1 != nullptr );

		// Get from cache without loading
		const auto scene2 = manager.get<Scene>( "test_scene.gltf" );
		REQUIRE( scene2 != nullptr );
		REQUIRE( scene1.get() == scene2.get() );
	}

	SECTION( "Get returns nullptr for non-cached assets" )
	{
		auto scene = manager.get<Scene>( "non_existent.gltf" );
		REQUIRE( scene == nullptr );
	}
}

TEST_CASE( "AssetManager store functionality", "[AssetManager][unit]" )
{
	AssetManager manager;

	SECTION( "Store puts asset in cache" )
	{
		const auto material = std::make_shared<Material>();
		const std::string path = "stored_material.mtl";

		// Verify not in cache initially
		REQUIRE_FALSE( manager.isCached( path ) );

		// Store in cache
		manager.store( path, material );

		// Verify now in cache
		REQUIRE( manager.isCached( path ) );
		REQUIRE( material->getPath() == path );
		REQUIRE( material->isLoaded() );

		// Verify can retrieve same object
		const auto retrieved = manager.get<Material>( path );
		REQUIRE( retrieved.get() == material.get() );
	}
}

TEST_CASE( "AssetManager unload functionality", "[AssetManager][unit]" )
{
	AssetManager manager;

	SECTION( "Unload removes asset when no other references" )
	{
		auto material = manager.load<Material>( "unload_test.mtl" );
		REQUIRE( manager.isCached( "unload_test.mtl" ) );

		// Release our reference
		material.reset();

		// Now unload should succeed
		const bool unloaded = manager.unload( "unload_test.mtl" );
		REQUIRE( unloaded );
		REQUIRE_FALSE( manager.isCached( "unload_test.mtl" ) );
	}

	SECTION( "Unload fails when other references exist" )
	{
		const auto material = manager.load<Material>( "unload_test2.mtl" );
		REQUIRE( manager.isCached( "unload_test2.mtl" ) );

		// Keep reference alive
		const bool unloaded = manager.unload( "unload_test2.mtl" );
		REQUIRE_FALSE( unloaded );						   // Should fail because we still hold reference
		REQUIRE( manager.isCached( "unload_test2.mtl" ) ); // Still cached
	}

	SECTION( "Unload of non-existent asset returns false" )
	{
		const bool unloaded = manager.unload( "never_loaded.mtl" );
		REQUIRE_FALSE( unloaded );
	}
}

TEST_CASE( "AssetManager clearCache functionality", "[AssetManager][unit]" )
{
	AssetManager manager;

	// Load several assets
	const auto material = manager.load<Material>( "clear_test1.mtl" );
	const auto scene = manager.load<Scene>( "clear_test2.gltf" );

	REQUIRE( manager.isCached( "clear_test1.mtl" ) );
	REQUIRE( manager.isCached( "clear_test2.gltf" ) );

	// Clear cache
	manager.clearCache();

	// Verify cache is empty
	REQUIRE_FALSE( manager.isCached( "clear_test1.mtl" ) );
	REQUIRE_FALSE( manager.isCached( "clear_test2.gltf" ) );

	// Original references should still be valid
	REQUIRE( material != nullptr );
	REQUIRE( scene != nullptr );
}

TEST_CASE( "AssetManager integration with GLTFLoader", "[AssetManager][integration]" )
{
	AssetManager manager;
	gltf_loader::GLTFLoader gltfLoader;

	SECTION( "Store externally loaded scene" )
	{
		// Create a simple glTF scene using GLTFLoader
		const std::string gltfContent = R"({
            "asset": {"version": "2.0"},
            "scenes": [{"nodes": [0]}],
            "nodes": [{"name": "TestNode"}],
            "scene": 0
        })";

		// Load using GLTFLoader
		auto loadedScene = gltfLoader.loadFromString( gltfContent );

		// Store in AssetManager
		const std::string path = "test_integration.gltf";
		if ( loadedScene )
		{
			// Convert unique_ptr to shared_ptr for storage
			const auto sharedScene = std::shared_ptr<Scene>( loadedScene.release() );
			manager.store( path, sharedScene );

			// Verify it's cached
			REQUIRE( manager.isCached( path ) );

			// Verify we can retrieve it
			const auto retrievedScene = manager.get<Scene>( path );
			REQUIRE( retrievedScene != nullptr );
			REQUIRE( retrievedScene.get() == sharedScene.get() );
		}
	}
}

TEST_CASE( "AssetManager ECS import callback mechanism", "[AssetManager][ecs][callback]" )
{
	AssetManager manager;

	// Mock ECS scene - we'll just use a simple test double
	struct MockEcsScene
	{
		bool wasImported = false;
		std::shared_ptr<assets::Scene> importedScene;
	} mockEcsScene;

	// Set up the callback to capture ECS integration
	AssetManager::importSceneCallback = [&mockEcsScene]( std::shared_ptr<assets::Scene> scene, ecs::Scene & ) {
		mockEcsScene.wasImported = true;
		mockEcsScene.importedScene = scene;
	};

	SECTION( "importScene calls callback when set" )
	{
		// The static cast here is a bit of a hack for testing, but it demonstrates the callback mechanism
		const bool result = manager.importScene( "test_scene.gltf", reinterpret_cast<ecs::Scene &>( mockEcsScene ) );

		REQUIRE( result == true );
		REQUIRE( mockEcsScene.wasImported == true );
		REQUIRE( mockEcsScene.importedScene != nullptr );
		REQUIRE( mockEcsScene.importedScene->getPath() == "test_scene.gltf" );
		REQUIRE( mockEcsScene.importedScene->isLoaded() == true );
	}

	SECTION( "importScene returns false when no callback set" )
	{
		AssetManager::importSceneCallback = nullptr;

		const bool result = manager.importScene( "test_scene.gltf", reinterpret_cast<ecs::Scene &>( mockEcsScene ) );

		REQUIRE( result == false );
		REQUIRE( mockEcsScene.wasImported == false );
	}

	// Clean up callback after test
	AssetManager::importSceneCallback = nullptr;
}
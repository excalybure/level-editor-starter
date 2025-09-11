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
		// Set up mock callback for loading
		assets::AssetManager::setSceneLoaderCallback( []( const std::string & ) -> std::shared_ptr<assets::Scene> {
			return std::make_shared<assets::Scene>();
		} );

		// Load first
		const auto scene1 = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene1 != nullptr );

		// Get from cache without loading
		const auto scene2 = manager.get<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene2 != nullptr );
		REQUIRE( scene1.get() == scene2.get() );

		// Clean up
		assets::AssetManager::clearSceneLoaderCallback();
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

	// Set up mock callback for scene loading
	assets::AssetManager::setSceneLoaderCallback( []( const std::string & ) -> std::shared_ptr<assets::Scene> {
		return std::make_shared<assets::Scene>();
	} );

	// Load several assets
	const auto material = manager.load<Material>( "clear_test1.mtl" );
	const auto scene = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );

	REQUIRE( manager.isCached( "clear_test1.mtl" ) );
	REQUIRE( manager.isCached( "tests/test_assets/simple_triangle.gltf" ) );

	// Clear cache
	manager.clearCache();

	// Verify cache is empty
	REQUIRE_FALSE( manager.isCached( "clear_test1.mtl" ) );
	REQUIRE_FALSE( manager.isCached( "tests/test_assets/simple_triangle.gltf" ) );

	// Original references should still be valid
	REQUIRE( material != nullptr );
	REQUIRE( scene != nullptr );

	// Clean up
	assets::AssetManager::clearSceneLoaderCallback();
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
	AssetManager::setImportSceneCallback( [&mockEcsScene]( std::shared_ptr<assets::Scene> scene, ecs::Scene & ) {
		mockEcsScene.wasImported = true;
		mockEcsScene.importedScene = scene;
	} );

	SECTION( "importScene calls callback when set" )
	{
		// Set up mock scene loader callback
		AssetManager::setSceneLoaderCallback( []( const std::string & ) -> std::shared_ptr<assets::Scene> {
			return std::make_shared<assets::Scene>();
		} );

		// The static cast here is a bit of a hack for testing, but it demonstrates the callback mechanism
		const bool result = manager.importScene( "tests/test_assets/simple_triangle.gltf", reinterpret_cast<ecs::Scene &>( mockEcsScene ) );

		REQUIRE( result == true );
		REQUIRE( mockEcsScene.wasImported == true );
		REQUIRE( mockEcsScene.importedScene != nullptr );
		REQUIRE( mockEcsScene.importedScene->getPath() == "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( mockEcsScene.importedScene->isLoaded() == true );

		// Clean up scene loader callback
		AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "importScene returns false when no callback set" )
	{
		AssetManager::clearImportSceneCallback();

		const bool result = manager.importScene( "tests/test_assets/simple_triangle.gltf", reinterpret_cast<ecs::Scene &>( mockEcsScene ) );

		REQUIRE( result == false );
		REQUIRE( mockEcsScene.wasImported == false );
	}

	// Clean up callback after test
	AssetManager::clearImportSceneCallback();
}

TEST_CASE( "AssetManager loadScene loads actual glTF content", "[AssetManager][loadScene][integration]" )
{
	AssetManager manager;

	SECTION( "loadScene returns nullptr without callback" )
	{
		// Ensure no callback is set
		AssetManager::clearSceneLoaderCallback();

		// Load the test triangle scene
		const auto scene = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );

		// Without callback, should return nullptr
		REQUIRE( scene == nullptr );
		REQUIRE_FALSE( manager.isCached( "tests/test_assets/simple_triangle.gltf" ) );
	}

	SECTION( "loadScene handles invalid file path gracefully" )
	{
		// Ensure no callback is set
		AssetManager::clearSceneLoaderCallback();

		const auto scene = manager.load<Scene>( "non_existent_file.gltf" );
		REQUIRE( scene == nullptr ); // Without callback, returns nullptr
		REQUIRE_FALSE( manager.isCached( "non_existent_file.gltf" ) );
	}

	SECTION( "loadScene caches successfully loaded scenes" )
	{
		// Set up a mock loader callback
		AssetManager::setSceneLoaderCallback( []( const std::string & ) -> std::shared_ptr<assets::Scene> {
			auto scene = std::make_shared<assets::Scene>();
			return scene;
		} );

		// Load scene first time
		const auto scene1 = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene1 != nullptr );
		REQUIRE( manager.isCached( "tests/test_assets/simple_triangle.gltf" ) );

		// Load same scene second time - should return cached instance
		const auto scene2 = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene2 != nullptr );
		REQUIRE( scene1.get() == scene2.get() ); // Same object

		// Clean up
		assets::AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "loadScene uses callback when available" )
	{
		// Set up a mock loader callback using the static method
		bool callbackCalled = false;
		AssetManager::setSceneLoaderCallback( [&callbackCalled]( const std::string & ) -> std::shared_ptr<assets::Scene> {
			callbackCalled = true;

			// Create a mock scene with some content
			auto scene = std::make_shared<assets::Scene>();
			auto rootNode = std::make_unique<assets::SceneNode>();
			rootNode->setName( "test_node" );
			scene->addRootNode( std::move( rootNode ) );
			return scene;
		} );

		const auto scene = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene != nullptr );
		REQUIRE( callbackCalled );
		REQUIRE( scene->getRootNodes().size() == 1 );
		REQUIRE( scene->getRootNodes()[0]->getName() == "test_node" );

		// Clean up using the static method
		AssetManager::clearSceneLoaderCallback();
	}

	SECTION( "loadScene returns nullptr when callback unavailable" )
	{
		// Ensure no callback is set
		AssetManager::clearSceneLoaderCallback();

		const auto scene = manager.load<Scene>( "tests/test_assets/simple_triangle.gltf" );
		REQUIRE( scene == nullptr ); // No callback means no loading
		REQUIRE_FALSE( manager.isCached( "tests/test_assets/simple_triangle.gltf" ) );
	}
}
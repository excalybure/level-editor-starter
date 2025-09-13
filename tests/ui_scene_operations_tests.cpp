#include <catch2/catch_test_macros.hpp>

import editor.ui;
import runtime.ecs;
import runtime.systems;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;
import platform.dx12;
import std;

// Mock GPUResourceManager for testing
class MockGPUResourceManager : public engine::GPUResourceManager
{
public:
	MockGPUResourceManager() : engine::GPUResourceManager( getMockDevice() ) {}

private:
	static dx12::Device &getMockDevice()
	{
		static dx12::Device device;
		return device;
	}
};

TEST_CASE( "UI Scene Operations - Constructor and initialization", "[ui][scene][unit]" )
{
	SECTION( "UI provides scene operations interface without initialization" )
	{
		editor::UI ui;

		// Test that scene operations are available through UI
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( ui.getLastError().empty() );
	}

	SECTION( "UI scene operations with full initialization" )
	{
		// Setup dependencies
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;

		// Initialize UI with scene dependencies
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Test initial state after initialization
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( ui.getLastError().empty() );
	}
}

TEST_CASE( "UI Scene Operations - Scene loading functionality", "[ui][scene][loading][unit]" )
{
	SECTION( "Scene loading with nonexistent file produces error" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Test loading nonexistent file
		REQUIRE_FALSE( ui.loadScene( "nonexistent_file.gltf" ) );

		// Should have an error message
		REQUIRE_FALSE( ui.getLastError().empty() );

		// Scene path should remain empty
		REQUIRE( ui.getCurrentScenePath().empty() );

		// Entity count should remain 0
		REQUIRE( ui.getEntityCount() == 0 );
	}
}

TEST_CASE( "UI Scene Operations - Scene clearing", "[ui][scene][clear][unit]" )
{
	SECTION( "Scene clearing resets all state" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Add some entities to the scene
		scene.createEntity( "Entity1" );
		scene.createEntity( "Entity2" );
		REQUIRE( ui.getEntityCount() == 2 );

		// Clear the scene
		ui.clearScene();

		// All entities should be removed
		REQUIRE( ui.getEntityCount() == 0 );

		// Scene path should be empty
		REQUIRE( ui.getCurrentScenePath().empty() );
	}
}

TEST_CASE( "UI Scene Operations - File dialog functionality", "[ui][scene][dialog][unit]" )
{
	SECTION( "File dialog in test mode does not block" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Native Windows dialog is modal and would block tests
		// In test mode (no ImGui context), openFileDialog() should return immediately
		ui.openFileDialog(); // Should not block
	}
}
TEST_CASE( "UI Scene Operations - Entity counting", "[ui][scene][entities][unit]" )
{
	SECTION( "Entity count reflects scene state accurately" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Initially should have no entities
		REQUIRE( ui.getEntityCount() == 0 );

		// Add entities and verify count updates
		const auto entity1 = scene.createEntity( "Entity1" );
		REQUIRE( ui.getEntityCount() == 1 );

		const auto entity2 = scene.createEntity( "Entity2" );
		REQUIRE( ui.getEntityCount() == 2 );

		const auto entity3 = scene.createEntity( "Entity3" );
		REQUIRE( ui.getEntityCount() == 3 );

		// Remove an entity and verify count decreases
		scene.destroyEntity( entity2 );
		REQUIRE( ui.getEntityCount() == 2 );

		// Clear all and verify count is zero
		scene.destroyEntity( entity1 );
		scene.destroyEntity( entity3 );
		REQUIRE( ui.getEntityCount() == 0 );
	}
}

TEST_CASE( "UI Scene Operations Integration", "[ui][scene][integration]" )
{
	SECTION( "Error message management" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Initially no error
		REQUIRE( ui.getLastError().empty() );

		// Attempt to load invalid file (should produce error)
		ui.loadScene( "nonexistent.gltf" );

		// Should have error message
		REQUIRE_FALSE( ui.getLastError().empty() );

		// Clear scene should clear error
		ui.clearScene();

		// Error should be cleared
		REQUIRE( ui.getLastError().empty() );
	}

	SECTION( "Scene path management" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Initially no scene path
		REQUIRE( ui.getCurrentScenePath().empty() );

		// Clear should maintain empty path
		ui.clearScene();
		REQUIRE( ui.getCurrentScenePath().empty() );
	}

	SECTION( "Entity management during clear operations" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Create entities
		const auto entity = scene.createEntity( "TestEntity" );
		REQUIRE( ui.getEntityCount() == 1 );
		REQUIRE( scene.isValid( entity ) );

		// Clear scene
		ui.clearScene();

		// Entity should be destroyed
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( !scene.isValid( entity ) );
	}

	SECTION( "File dialog operations in integration context" )
	{
		editor::UI ui;

		// Test file dialog API for modal dialog behavior
		ui.openFileDialog(); // Should not block in test mode
	}
}

TEST_CASE( "UI Scene Operations - Integration test with native Windows dialog", "[ui][scene][integration][dialog]" )
{
	SECTION( "Native modal dialog system integration" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// In test mode (no ImGui context), openFileDialog() will skip the modal dialog
		ui.openFileDialog(); // Should return immediately without blocking

		// Since no actual file was selected in test mode, no scene should be loaded
		REQUIRE( ui.getCurrentScenePath().empty() );
	}
}
#include <catch2/catch_test_macros.hpp>

import editor.scene_editor;
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

TEST_CASE( "SceneEditor constructor initializes correctly", "[scene_editor][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;

	// Act
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Assert
	REQUIRE( sceneEditor.getCurrentScenePath().empty() );
	REQUIRE( sceneEditor.getEntityCount() == 0 );
}

TEST_CASE( "SceneEditor provides expected interface methods", "[scene_editor][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Act & Assert - verify methods exist and basic functionality works
	// Note: ImGui rendering methods omitted due to context requirements

	// Test clearScene (should work even with empty scene)
	sceneEditor.clearScene();
	REQUIRE( sceneEditor.getEntityCount() == 0 );
	REQUIRE( sceneEditor.getCurrentScenePath().empty() );

	// Test file dialog functionality
	REQUIRE_FALSE( sceneEditor.isFileDialogActive() );
	sceneEditor.openFileDialog();
	REQUIRE( sceneEditor.isFileDialogActive() );
}

TEST_CASE( "SceneEditor loadScene handles empty path correctly", "[scene_editor][load][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Act
	const bool result = sceneEditor.loadScene( "" );

	// Assert
	REQUIRE( result == false );
	REQUIRE( sceneEditor.getCurrentScenePath().empty() );
	REQUIRE( sceneEditor.getEntityCount() == 0 );
}

TEST_CASE( "SceneEditor clearScene removes all entities", "[scene_editor][clear][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Add some entities to the scene first
	const auto entity1 = scene.createEntity( "Entity1" );
	const auto entity2 = scene.createEntity( "Entity2" );
	REQUIRE( scene.isValid( entity1 ) );
	REQUIRE( scene.isValid( entity2 ) );
	REQUIRE( sceneEditor.getEntityCount() == 2 );

	// Act
	sceneEditor.clearScene();

	// Assert
	REQUIRE( sceneEditor.getEntityCount() == 0 );
	REQUIRE( !scene.isValid( entity1 ) );
	REQUIRE( !scene.isValid( entity2 ) );
	REQUIRE( sceneEditor.getCurrentScenePath().empty() );
}

TEST_CASE( "SceneEditor file dialog triggers file selection", "[dialog][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Initially no file should be pending
	REQUIRE_FALSE( sceneEditor.isFileDialogActive() );

	// After triggering dialog, it should be active
	sceneEditor.openFileDialog();
	REQUIRE( sceneEditor.isFileDialogActive() );

	// Process dialog directly (simulates cancel)
	sceneEditor.processFileDialog();
	REQUIRE_FALSE( sceneEditor.isFileDialogActive() );
}

TEST_CASE( "SceneEditor status bar shows correct information", "[status][unit]" )
{
	// Arrange
	ecs::Scene scene;
	systems::SystemManager systemManager;
	assets::AssetManager assetManager;
	MockGPUResourceManager gpuManager;
	editor::SceneEditor sceneEditor( scene, systemManager, assetManager, gpuManager );

	// Initially should show no scene loaded
	REQUIRE( sceneEditor.getCurrentScenePath().empty() );
	REQUIRE( sceneEditor.getEntityCount() == 0 );

	// Add some entities to the scene
	const auto entity1 = scene.createEntity( "Entity1" );
	const auto entity2 = scene.createEntity( "Entity2" );

	// Entity count should be updated
	REQUIRE( sceneEditor.getEntityCount() == 2 );

	// Note: ImGui rendering test omitted due to context requirements
	// The actual renderStatusBar() method is tested in integration tests
}
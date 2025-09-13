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
		REQUIRE_FALSE( ui.isFileDialogOpen() );
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

TEST_CASE( "UI Scene Operations - Load scene functionality", "[ui][scene][load][unit]" )
{
	SECTION( "Load scene handles empty path correctly" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Act
		const bool result = ui.loadScene( "" );

		// Assert
		REQUIRE( result == false );
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE_FALSE( ui.getLastError().empty() ); // Should have error message
	}

	SECTION( "Load scene without initialization fails gracefully" )
	{
		editor::UI ui;

		// Act
		const bool result = ui.loadScene( "test.gltf" );

		// Assert
		REQUIRE( result == false );
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE_FALSE( ui.getLastError().empty() ); // Should have error message about missing dependencies
	}
}

TEST_CASE( "UI Scene Operations - Clear scene functionality", "[ui][scene][clear][unit]" )
{
	SECTION( "Clear scene removes all entities" )
	{
		// Arrange
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Add some entities to the scene first
		const auto entity1 = scene.createEntity( "Entity1" );
		const auto entity2 = scene.createEntity( "Entity2" );
		REQUIRE( scene.isValid( entity1 ) );
		REQUIRE( scene.isValid( entity2 ) );
		REQUIRE( ui.getEntityCount() == 2 );

		// Act
		ui.clearScene();

		// Assert
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( !scene.isValid( entity1 ) );
		REQUIRE( !scene.isValid( entity2 ) );
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE( ui.getLastError().empty() ); // Clear should not produce errors
	}

	SECTION( "Clear scene without initialization works safely" )
	{
		editor::UI ui;

		// Act - should not crash
		REQUIRE_NOTHROW( ui.clearScene() );

		// Assert
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( ui.getCurrentScenePath().empty() );
	}
}

TEST_CASE( "UI Scene Operations - File dialog functionality", "[ui][scene][dialog][unit]" )
{
	SECTION( "File dialog triggers and processes correctly" )
	{
		// Arrange
		editor::UI ui;

		// Initially no file dialog should be active
		REQUIRE_FALSE( ui.isFileDialogOpen() );

		// After triggering dialog, it should be active
		ui.openFileDialog();
		REQUIRE( ui.isFileDialogOpen() );

		// Process dialog directly (simulates cancel/close)
		ui.processFileDialog();
		REQUIRE_FALSE( ui.isFileDialogOpen() );
	}

	SECTION( "Multiple file dialog operations" )
	{
		editor::UI ui;

		// Test multiple open/close cycles
		for ( int i = 0; i < 3; ++i )
		{
			REQUIRE_FALSE( ui.isFileDialogOpen() );
			ui.openFileDialog();
			REQUIRE( ui.isFileDialogOpen() );
			ui.processFileDialog();
			REQUIRE_FALSE( ui.isFileDialogOpen() );
		}
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

		// Clear scene and verify count is zero
		ui.clearScene();
		REQUIRE( ui.getEntityCount() == 0 );
	}

	SECTION( "Entity count without initialization returns zero" )
	{
		editor::UI ui;
		REQUIRE( ui.getEntityCount() == 0 );
	}
}

TEST_CASE( "UI Scene Operations - Error handling", "[ui][scene][error][unit]" )
{
	SECTION( "Error state is properly tracked and cleared" )
	{
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;

		editor::UI ui;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Initially no error
		REQUIRE( ui.getLastError().empty() );

		// Trigger an error with invalid load
		const bool result = ui.loadScene( "" );
		REQUIRE( result == false );
		REQUIRE_FALSE( ui.getLastError().empty() );

		// Clear scene should clear errors
		ui.clearScene();
		REQUIRE( ui.getLastError().empty() );
	}

	SECTION( "Error messages are informative" )
	{
		editor::UI ui;

		// Test error without initialization
		ui.loadScene( "test.gltf" );
		const std::string &error1 = ui.getLastError();
		REQUIRE_FALSE( error1.empty() );
		REQUIRE( error1.find( "dependencies" ) != std::string::npos );

		// Test error with empty path after initialization
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		MockGPUResourceManager gpuManager;
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		ui.loadScene( "" );
		const std::string &error2 = ui.getLastError();
		REQUIRE_FALSE( error2.empty() );
		REQUIRE( error2.find( "empty" ) != std::string::npos );
	}
}

// Integration test for backwards compatibility
TEST_CASE( "UI Scene Operations Integration", "[ui][scene][integration]" )
{
	SECTION( "Complete scene operations workflow" )
	{
		// Setup dependencies
		dx12::Device device;
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		engine::GPUResourceManager gpuManager( device );

		editor::UI ui;

		// Initialize UI with scene dependencies
		ui.initializeSceneOperations( scene, systemManager, assetManager, gpuManager );

		// Test complete workflow
		REQUIRE_FALSE( ui.loadScene( "" ) ); // Empty path should fail
		REQUIRE_NOTHROW( ui.clearScene() );	 // Should not crash
		REQUIRE( ui.getEntityCount() == 0 ); // Scene should be empty after clear

		// Add entities and verify operations
		const auto entity = scene.createEntity( "TestEntity" );
		REQUIRE( ui.getEntityCount() == 1 );

		// Clear and verify
		ui.clearScene();
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE( !scene.isValid( entity ) );
	}

	SECTION( "File dialog operations in integration context" )
	{
		editor::UI ui;

		// Test file dialog state management
		REQUIRE_FALSE( ui.isFileDialogOpen() );

		ui.openFileDialog();
		REQUIRE( ui.isFileDialogOpen() );

		ui.processFileDialog(); // Should close dialog
		REQUIRE_FALSE( ui.isFileDialogOpen() );
	}
}
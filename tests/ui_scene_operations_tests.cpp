#include <catch2/catch_test_macros.hpp>

import editor.ui;
import runtime.ecs;
import runtime.systems;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;
import platform.dx12;

TEST_CASE( "UI Scene Operations Integration", "[ui][scene][integration]" )
{
	SECTION( "UI provides scene operations interface" )
	{
		editor::UI ui;

		// Test that scene operations are available through UI
		REQUIRE( ui.getCurrentScenePath().empty() );
		REQUIRE( ui.getEntityCount() == 0 );
		REQUIRE_FALSE( ui.isFileDialogOpen() );
	}

	SECTION( "UI scene operations with dependencies" )
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

		// Test scene operations
		REQUIRE_FALSE( ui.loadScene( "" ) ); // Empty path should fail
		REQUIRE_NOTHROW( ui.clearScene() );	 // Should not crash
		REQUIRE( ui.getEntityCount() == 0 ); // Scene should be empty after clear
	}

	SECTION( "UI file dialog operations" )
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
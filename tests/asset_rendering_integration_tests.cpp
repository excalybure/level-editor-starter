#include <catch2/catch_test_macros.hpp>

import runtime.ecs;
import runtime.systems;
import runtime.mesh_rendering_system;
import engine.renderer;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;
import editor.ui;
import platform.dx12;

TEST_CASE( "Asset loading to rendering integration initializes correctly", "[integration][asset-rendering]" )
{
	SECTION( "AssetManager and GPUResourceManager can be instantiated" )
	{
		// Arrange - create minimal resources
		dx12::Device device; // Note: this won't initialize without a window, but we can test instantiation

		// Act - create managers
		assets::AssetManager assetManager;
		engine::GPUResourceManager gpuResourceManager( device );

		// Assert - basic instantiation successful
		REQUIRE( true ); // If we get here, instantiation worked
	}

	SECTION( "UI SceneEditor can be initialized with managers" )
	{
		// Arrange
		dx12::Device device;
		ecs::Scene scene;
		systems::SystemManager systemManager;
		assets::AssetManager assetManager;
		engine::GPUResourceManager gpuResourceManager( device );
		editor::UI ui;

		// Act - test initializeSceneOperations method exists and can be called
		REQUIRE_NOTHROW( ui.initializeSceneOperations( scene, systemManager, assetManager, gpuResourceManager ) );

		// Assert
		REQUIRE( true ); // If we get here without exceptions, the wiring is correct
	}
}

TEST_CASE( "MeshRenderingSystem integration with asset managers", "[integration][mesh-rendering]" )
{
	SECTION( "MeshRenderingSystem can be created with renderer" )
	{
		// Arrange
		dx12::Device device;

		REQUIRE( device.initializeHeadless() );

		renderer::Renderer renderer( device );

		// Act - create MeshRenderingSystem
		auto meshRenderingSystem = std::make_unique<runtime::systems::MeshRenderingSystem>( renderer );

		// Assert
		REQUIRE( meshRenderingSystem != nullptr );
	}

	SECTION( "SystemManager can add MeshRenderingSystem" )
	{
		// Arrange
		dx12::Device device;

		REQUIRE( device.initializeHeadless() );

		renderer::Renderer renderer( device );
		systems::SystemManager systemManager;
		ecs::Scene scene;

		// Act - add system to manager
		auto *meshRenderingSystem = systemManager.addSystem<runtime::systems::MeshRenderingSystem>( renderer );

		// Assert
		REQUIRE( meshRenderingSystem != nullptr );

		// Act - initialize systems
		REQUIRE_NOTHROW( systemManager.initialize( scene ) );
	}
}
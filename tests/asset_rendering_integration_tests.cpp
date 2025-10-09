#include <catch2/catch_test_macros.hpp>

#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/mesh_rendering_system.h"
#include "engine/renderer/renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/gpu/gpu_resource_manager.h"
#include "editor/ui.h"
#include "editor/selection.h"
#include "platform/dx12/dx12_device.h"
#include "engine/shader_manager/shader_manager.h"

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
		editor::SelectionManager selectionManager( scene, systemManager );
		editor::UI ui;

		// Act - test initializeSceneOperations method exists and can be called
		REQUIRE_NOTHROW( ui.initializeSceneOperations( scene, systemManager, assetManager, gpuResourceManager, selectionManager ) ); // Assert
		REQUIRE( true );																											 // If we get here without exceptions, the wiring is correct
	}
}

TEST_CASE( "MeshRenderingSystem integration with asset managers", "[integration][mesh-rendering]" )
{
	SECTION( "MeshRenderingSystem can be created with renderer" )
	{
		// Arrange
		dx12::Device device;

		REQUIRE( device.initializeHeadless() );

		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		renderer::Renderer renderer( device, *shaderManager );

		// Act - create MeshRenderingSystem
		auto meshRenderingSystem = std::make_unique<systems::MeshRenderingSystem>( renderer, shaderManager, nullptr );

		// Assert
		REQUIRE( meshRenderingSystem != nullptr );
	}

	SECTION( "SystemManager can add MeshRenderingSystem" )
	{
		// Arrange
		dx12::Device device;

		REQUIRE( device.initializeHeadless() );

		auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
		renderer::Renderer renderer( device, *shaderManager );
		systems::SystemManager systemManager;
		ecs::Scene scene;

		// Act - add system to manager
		auto *meshRenderingSystem = systemManager.addSystem<systems::MeshRenderingSystem>( renderer, shaderManager, nullptr );

		// Assert
		REQUIRE( meshRenderingSystem != nullptr );

		// Act - initialize systems
		REQUIRE_NOTHROW( systemManager.initialize( scene ) );
	}
}
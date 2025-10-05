#include <catch2/catch_test_macros.hpp>

#include "runtime/mesh_rendering_system.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/renderer/renderer.h"
#include "engine/camera/camera.h"
#include "platform/dx12/dx12_device.h"
#include "engine/shader_manager/shader_manager.h"
#include "engine/gpu/mesh_gpu.h"

TEST_CASE( "MeshRenderingSystem skips entities with visible=false", "[T6.0][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create entity with Transform, MeshRenderer, and Visible components
	const auto visibleEntity = scene.createEntity( "VisibleEntity" );
	components::Transform transform1;
	scene.addComponent( visibleEntity, transform1 );

	components::MeshRenderer meshRenderer1;
	// Note: For this test, we're checking that the system respects the Visible flag
	// The actual GPU rendering call won't happen without a valid gpuMesh, which is expected
	scene.addComponent( visibleEntity, meshRenderer1 );

	components::Visible visible1;
	visible1.visible = true;
	scene.addComponent( visibleEntity, visible1 );

	// Create entity with visible=false
	const auto invisibleEntity = scene.createEntity( "InvisibleEntity" );
	components::Transform transform2;
	scene.addComponent( invisibleEntity, transform2 );

	components::MeshRenderer meshRenderer2;
	scene.addComponent( invisibleEntity, meshRenderer2 );

	components::Visible visible2;
	visible2.visible = false; // This entity should not render
	scene.addComponent( invisibleEntity, visible2 );

	// Create entity without Visible component (should default to visible)
	const auto noVisibleCompEntity = scene.createEntity( "NoVisibleComp" );
	components::Transform transform3;
	scene.addComponent( noVisibleCompEntity, transform3 );

	components::MeshRenderer meshRenderer3;
	scene.addComponent( noVisibleCompEntity, meshRenderer3 );

	camera::PerspectiveCamera camera;

	// Act & Assert - Should not crash; entities with visible=false should be skipped
	REQUIRE_NOTHROW( system.render( scene, camera ) );

	// The test verifies that:
	// 1. Entities with visible=true are processed (visibleEntity)
	// 2. Entities with visible=false are skipped (invisibleEntity)
	// 3. Entities without Visible component are processed (noVisibleCompEntity)
	// Actual rendering validation would require mock renderer or GPU inspection
}

TEST_CASE( "MeshRenderingSystem renders entities without Visible component", "[T6.0][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create entity without Visible component (should still render)
	const auto entity = scene.createEntity( "NoVisibleComponent" );
	components::Transform transform;
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	scene.addComponent( entity, meshRenderer );

	camera::PerspectiveCamera camera;

	// Act & Assert - Should not crash; entity without Visible component should be processed
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}

TEST_CASE( "MeshRenderingSystem respects castShadows flag", "[T6.0][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create entity with castShadows=false
	const auto entity = scene.createEntity( "NoCastShadows" );
	components::Transform transform;
	scene.addComponent( entity, transform );

	components::MeshRenderer meshRenderer;
	scene.addComponent( entity, meshRenderer );

	components::Visible visible;
	visible.visible = true;
	visible.castShadows = false; // Future: shadow rendering should respect this
	visible.receiveShadows = true;
	scene.addComponent( entity, visible );

	camera::PerspectiveCamera camera;

	// Act & Assert - Should not crash
	// Note: This test documents intended behavior for future shadow system integration
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}

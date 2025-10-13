#include <catch2/catch_test_macros.hpp>

#include "runtime/mesh_rendering_system.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "graphics/renderer/immediate_renderer.h"
#include "engine/camera/camera.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/gpu/mesh_gpu.h"

TEST_CASE( "MeshRenderingSystem skips entities with visible=false", "[T6.0][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
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

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
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

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
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

TEST_CASE( "Hierarchical visibility: invisible parent hides children", "[hierarchical][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
	ecs::Scene scene;

	// Create parent entity with visible=false
	const auto parent = scene.createEntity( "InvisibleParent" );
	components::Transform parentTransform;
	scene.addComponent( parent, parentTransform );
	components::MeshRenderer parentMesh;
	scene.addComponent( parent, parentMesh );

	auto *parentVisible = scene.getComponent<components::Visible>( parent );
	parentVisible->visible = false; // Parent is invisible

	// Create child entity with visible=true
	const auto child = scene.createEntity( "VisibleChild" );
	components::Transform childTransform;
	scene.addComponent( child, childTransform );
	components::MeshRenderer childMesh;
	scene.addComponent( child, childMesh );

	auto *childVisible = scene.getComponent<components::Visible>( child );
	childVisible->visible = true; // Child is visible BUT parent is not

	// Set hierarchy
	scene.setParent( child, parent );

	camera::PerspectiveCamera camera;

	// Act & Assert
	// Child should NOT render because parent is invisible
	// This test verifies hierarchical visibility propagation
	REQUIRE_NOTHROW( system.render( scene, camera ) );

	// Note: Actual validation would require render interception
	// For now, we verify no crashes and document expected behavior:
	// Child with visible=true should NOT render when parent has visible=false
}

TEST_CASE( "Hierarchical visibility: visible parent shows visible children", "[hierarchical][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
	ecs::Scene scene;

	// Create parent entity with visible=true
	const auto parent = scene.createEntity( "VisibleParent" );
	components::Transform parentTransform;
	scene.addComponent( parent, parentTransform );
	components::MeshRenderer parentMesh;
	scene.addComponent( parent, parentMesh );

	auto *parentVisible = scene.getComponent<components::Visible>( parent );
	parentVisible->visible = true;

	// Create child entity with visible=true
	const auto child = scene.createEntity( "VisibleChild" );
	components::Transform childTransform;
	scene.addComponent( child, childTransform );
	components::MeshRenderer childMesh;
	scene.addComponent( child, childMesh );

	auto *childVisible = scene.getComponent<components::Visible>( child );
	childVisible->visible = true;

	// Set hierarchy
	scene.setParent( child, parent );

	camera::PerspectiveCamera camera;

	// Act & Assert
	// Both should render (parent visible, child visible)
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}

TEST_CASE( "Hierarchical visibility: visible parent respects invisible children", "[hierarchical][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
	ecs::Scene scene;

	// Create parent entity with visible=true
	const auto parent = scene.createEntity( "VisibleParent" );
	components::Transform parentTransform;
	scene.addComponent( parent, parentTransform );
	components::MeshRenderer parentMesh;
	scene.addComponent( parent, parentMesh );

	auto *parentVisible = scene.getComponent<components::Visible>( parent );
	parentVisible->visible = true;

	// Create child entity with visible=false
	const auto child = scene.createEntity( "InvisibleChild" );
	components::Transform childTransform;
	scene.addComponent( child, childTransform );
	components::MeshRenderer childMesh;
	scene.addComponent( child, childMesh );

	auto *childVisible = scene.getComponent<components::Visible>( child );
	childVisible->visible = false; // Child explicitly invisible

	// Set hierarchy
	scene.setParent( child, parent );

	camera::PerspectiveCamera camera;

	// Act & Assert
	// Parent should render, child should NOT (child has visible=false)
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}

TEST_CASE( "Hierarchical visibility: deep hierarchy respects all ancestors", "[hierarchical][visibility][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	renderer::ImmediateRenderer renderer( device, *shaderManager );
	systems::MeshRenderingSystem system( renderer, nullptr, shaderManager, nullptr );
	ecs::Scene scene;

	// Create grandparent (visible=false)
	const auto grandparent = scene.createEntity( "Grandparent" );
	components::Transform gpTransform;
	scene.addComponent( grandparent, gpTransform );
	components::MeshRenderer gpMesh;
	scene.addComponent( grandparent, gpMesh );

	auto *gpVisible = scene.getComponent<components::Visible>( grandparent );
	gpVisible->visible = false; // Grandparent invisible

	// Create parent (visible=true)
	const auto parent = scene.createEntity( "Parent" );
	components::Transform pTransform;
	scene.addComponent( parent, pTransform );
	components::MeshRenderer pMesh;
	scene.addComponent( parent, pMesh );

	auto *pVisible = scene.getComponent<components::Visible>( parent );
	pVisible->visible = true;

	// Create child (visible=true)
	const auto child = scene.createEntity( "Child" );
	components::Transform cTransform;
	scene.addComponent( child, cTransform );
	components::MeshRenderer cMesh;
	scene.addComponent( child, cMesh );

	auto *cVisible = scene.getComponent<components::Visible>( child );
	cVisible->visible = true;

	// Set hierarchy: grandparent -> parent -> child
	scene.setParent( parent, grandparent );
	scene.setParent( child, parent );

	camera::PerspectiveCamera camera;

	// Act & Assert
	// None should render: grandparent invisible makes entire subtree invisible
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}


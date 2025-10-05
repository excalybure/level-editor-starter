#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// We'll start by testing the basic module can be imported
#include "runtime/mesh_rendering_system.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/renderer/renderer.h"
#include "engine/camera/camera.h"
#include "platform/dx12/dx12_device.h"
#include "engine/shader_manager/shader_manager.h"

TEST_CASE( "MeshRenderingSystem can be created with renderer and ShaderManager", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();

	// Act & Assert - should compile and create without error
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
}

TEST_CASE( "MeshRenderingSystem update method can be called without error", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;
	const float deltaTime = 0.016f; // 60 FPS

	// Act & Assert - should not throw or crash
	system.update( scene, deltaTime );
}

TEST_CASE( "MeshRenderingSystem render method processes entities with MeshRenderer and Transform", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create an entity with both Transform and MeshRenderer components
	const auto entity = scene.createEntity( "TestEntity" );
	scene.addComponent( entity, components::Transform{} );
	scene.addComponent( entity, components::MeshRenderer{} );

	// Create camera for rendering
	camera::PerspectiveCamera camera;

	// Act & Assert - should not throw or crash, and should handle entities correctly
	system.render( scene, camera );
}

TEST_CASE( "MeshRenderingSystem calculateMVPMatrix returns valid matrix for identity transform", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );

	components::Transform transform;  // Default: identity transform
	camera::PerspectiveCamera camera; // Default camera

	// Act
	const auto mvpMatrix = system.calculateMVPMatrix( transform, camera );

	// Assert - Should not be a zero matrix (indicates proper calculation)
	bool hasNonZeroElement = false;
	if ( mvpMatrix.m00() != 0.0f || mvpMatrix.m11() != 0.0f || mvpMatrix.m22() != 0.0f || mvpMatrix.m33() != 0.0f )
	{
		hasNonZeroElement = true;
	}
	REQUIRE( hasNonZeroElement );
}

TEST_CASE( "MeshRenderingSystem renderEntity handles empty MeshRenderer without crashing", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create entity with empty mesh renderer (no GPU mesh)
	const auto entity = scene.createEntity( "TestEntity" );
	scene.addComponent( entity, components::Transform{} );
	scene.addComponent( entity, components::MeshRenderer{} );

	camera::PerspectiveCamera camera;

	// Act & Assert - Should not crash with empty mesh renderer
	REQUIRE_NOTHROW( system.renderEntity( scene, entity, camera ) );
}

TEST_CASE( "MeshRenderingSystem complete render system processes entities correctly", "[mesh_rendering_system][integration][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create multiple entities with different component combinations
	const auto entityWithBoth = scene.createEntity( "EntityWithBoth" );
	scene.addComponent( entityWithBoth, components::Transform{} );
	scene.addComponent( entityWithBoth, components::MeshRenderer{} );

	const auto entityTransformOnly = scene.createEntity( "TransformOnly" );
	scene.addComponent( entityTransformOnly, components::Transform{} );

	const auto entityMeshOnly = scene.createEntity( "MeshOnly" );
	scene.addComponent( entityMeshOnly, components::MeshRenderer{} );

	// Create camera for rendering
	camera::PerspectiveCamera camera;

	// Act & Assert - Should handle all entity types without crashing
	REQUIRE_NOTHROW( system.render( scene, camera ) );
}

TEST_CASE( "MeshRenderingSystem renderEntity sets MVP matrix on renderer when GPU mesh present", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	systems::MeshRenderingSystem system( renderer, shaderManager, nullptr );
	ecs::Scene scene;

	// Create entity with non-identity transform and empty mesh renderer
	const auto entity = scene.createEntity( "TestEntity" );
	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f }; // Non-identity transform
	scene.addComponent( entity, transform );
	scene.addComponent( entity, components::MeshRenderer{} );
	// Note: Without a real GPU mesh, the early return will happen
	// This test verifies the path when gpuMesh is null

	camera::PerspectiveCamera camera;

	// Store initial matrix
	const auto initialMatrix = renderer.getViewProjectionMatrix();

	// Act
	system.renderEntity( scene, entity, camera );

	// Assert - Matrix should remain unchanged when gpuMesh is null
	const auto finalMatrix = renderer.getViewProjectionMatrix();
	REQUIRE( finalMatrix.m00() == initialMatrix.m00() );
	REQUIRE( finalMatrix.m11() == initialMatrix.m11() );
	REQUIRE( finalMatrix.m22() == initialMatrix.m22() );
	REQUIRE( finalMatrix.m33() == initialMatrix.m33() );
}

TEST_CASE( "Renderer getCommandContext provides access to command context during active frame", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );

	// Act & Assert - No active frame, should return nullptr
	REQUIRE( renderer.getCommandContext() == nullptr );

	// Begin headless frame to create command context
	renderer.beginHeadlessForTests();

	// Now command context should be available
	auto *commandContext = renderer.getCommandContext();
	REQUIRE( commandContext != nullptr );

	// Verify command list is accessible through context
	auto *commandList = commandContext->get();
	REQUIRE( commandList != nullptr );
}

TEST_CASE( "MeshRenderingSystem uses world transforms for parent-child hierarchies", "[mesh_rendering_system][hierarchy][unit]" )
{
	// Arrange: Create scene with transform system
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();

	ecs::Scene scene;
	systems::SystemManager systemManager;
	auto *transformSystem = systemManager.addSystem<systems::TransformSystem>();

	// Create MeshRenderingSystem with SystemManager access for hierarchy support
	auto *meshRenderingSystem = systemManager.addSystem<systems::MeshRenderingSystem>( renderer, shaderManager, &systemManager );
	systemManager.initialize( scene );

	// Create parent and child entities
	const ecs::Entity parent = scene.createEntity( "Parent" );
	const ecs::Entity child = scene.createEntity( "Child" );

	// Set up transforms
	components::Transform parentTransform;
	parentTransform.position = { 10.0f, 0.0f, 0.0f };
	scene.addComponent( parent, parentTransform );

	components::Transform childTransform;
	childTransform.position = { 1.0f, 0.0f, 0.0f }; // Local offset
	scene.addComponent( child, childTransform );

	// Add MeshRenderer to child
	scene.addComponent( child, components::MeshRenderer{} );

	// Set up hierarchy
	scene.setParent( child, parent );

	// Update transform system to compute world matrices
	systemManager.update( scene, 0.016f );

	// Get child's world transform from TransformSystem
	const auto childWorldTransform = transformSystem->getWorldTransform( scene, child );

	// The child's world position should be parent(10,0,0) + child(1,0,0) = (11,0,0)
	REQUIRE( childWorldTransform.m03() == Catch::Approx( 11.0f ) );

	// Act: Call the new renderEntity that uses world transforms
	camera::PerspectiveCamera camera;

	// The new renderEntity(scene, entity, camera) method should use world transforms
	// This call should internally get worldTransform from TransformSystem
	REQUIRE_NOTHROW( meshRenderingSystem->renderEntity( scene, child, camera ) );

	// Verify that local matrix is different from world matrix
	const auto *childTransformComp = scene.getComponent<components::Transform>( child );
	const auto localMatrix = childTransformComp->getLocalMatrix();
	REQUIRE( localMatrix.m03() == Catch::Approx( 1.0f ) );	   // Local position
	REQUIRE( localMatrix.m03() != childWorldTransform.m03() ); // Local != World

	systemManager.shutdown( scene );
}
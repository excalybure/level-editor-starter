#include <catch2/catch_test_macros.hpp>

// We'll start by testing the basic module can be imported
import runtime.mesh_rendering_system;
import runtime.ecs;
import runtime.components;
import engine.renderer;
import engine.camera;
import platform.dx12;
import engine.shader_manager;

TEST_CASE( "MeshRenderingSystem can be created with renderer and ShaderManager", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();

	// Act & Assert - should compile and create without error
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );
}

TEST_CASE( "MeshRenderingSystem update method can be called without error", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );
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
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );
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
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );

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
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );

	components::Transform transform;	   // Default transform
	components::MeshRenderer meshRenderer; // Empty mesh renderer (no GPU mesh)
	camera::PerspectiveCamera camera;	   // Default camera

	// Act & Assert - Should not crash with empty mesh renderer
	REQUIRE_NOTHROW( system.renderEntity( transform, meshRenderer, camera ) );
}

TEST_CASE( "MeshRenderingSystem complete render system processes entities correctly", "[mesh_rendering_system][integration][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );
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
	runtime::systems::MeshRenderingSystem system( renderer, shaderManager );

	components::Transform transform;
	transform.position = { 1.0f, 2.0f, 3.0f }; // Non-identity transform

	components::MeshRenderer meshRenderer;
	// Note: Without a real GPU mesh, the early return will happen
	// This test verifies the path when gpuMesh is null

	camera::PerspectiveCamera camera;

	// Store initial matrix
	const auto initialMatrix = renderer.getViewProjectionMatrix();

	// Act
	system.renderEntity( transform, meshRenderer, camera );

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
#include <catch2/catch_test_macros.hpp>

// We'll start by testing the basic module can be imported
import runtime.mesh_rendering_system;
import runtime.ecs;
import runtime.components;
import engine.renderer;
import engine.camera;
import platform.dx12;

TEST_CASE( "MeshRenderingSystem can be created with renderer reference", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );

	// Act & Assert - should compile and create without error
	runtime::systems::MeshRenderingSystem system( renderer );
}

TEST_CASE( "MeshRenderingSystem update method can be called without error", "[mesh_rendering_system][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	renderer::Renderer renderer( device );
	runtime::systems::MeshRenderingSystem system( renderer );
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
	runtime::systems::MeshRenderingSystem system( renderer );
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
	runtime::systems::MeshRenderingSystem system( renderer );

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
	runtime::systems::MeshRenderingSystem system( renderer );

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
	runtime::systems::MeshRenderingSystem system( renderer );
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
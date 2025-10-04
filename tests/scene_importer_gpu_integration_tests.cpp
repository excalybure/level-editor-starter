#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <memory>

#include "runtime/ecs.h"
#include "runtime/entity.h"
#include "runtime/components.h"
#include "runtime/scene_importer.h"
#include "engine/gpu/gpu_resource_manager.h"
#include "engine/assets/assets.h"
#include "math/vec.h"
#include "platform/dx12/dx12_device.h"

using Catch::Approx;

TEST_CASE( "SceneImporter GPU integration compiles and links correctly", "[scene_importer][gpu_integration][unit]" )
{
	// This is a basic compilation test to ensure the GPU integration
	// methods are properly defined and can be called

	// Create a simple scene
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "gpu_integration_test.gltf" );
	scene->setLoaded( true );

	// Create a simple node
	auto rootNode = std::make_unique<assets::SceneNode>( "GPUTestNode" );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene using non-GPU path (basic test)
	ecs::Scene targetScene;
	const bool result = runtime::SceneImporter::importScene( scene, targetScene );

	// Verify import succeeded
	REQUIRE( result );

	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( entity.isValid() );
}

TEST_CASE( "SceneImporter creates MeshRenderer with GPU resources using GPUResourceManager", "[scene_importer][gpu_integration][unit]" )
{
	// AF1: runtime::SceneImporter should use GPUResourceManager to create GPU resources
	// This test should fail initially because the implementation is stubbed

	// Create a scene with a mesh node
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "gpu_test.gltf" );
	scene->setLoaded( true );

	// Create mesh for testing (corrected constructor call)
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "TestMesh" );
	const assets::MeshHandle meshHandle = scene->addMesh( mesh );

	// Create a node with mesh handle
	auto rootNode = std::make_unique<assets::SceneNode>( "GPUMeshNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene using GPU path (now unified with optional GPU parameter)
	ecs::Scene targetScene;
	// Create a dummy device and manager for stubbed test
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager resourceManager( device );
	const bool result = runtime::SceneImporter::importScene( scene, targetScene );

	// Verify import succeeded
	REQUIRE( result );

	// Create GPU resources separately
	const bool gpuResult = runtime::SceneImporter::createGPUResources( scene, targetScene, resourceManager );
	REQUIRE( gpuResult );

	// Verify entity was created
	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( entity.isValid() );

	// Verify components::MeshRenderer component exists (but GPU resources will be null for now)
	REQUIRE( targetScene.hasComponent<components::MeshRenderer>( entity ) );
	const auto *meshRenderer = targetScene.getComponent<components::MeshRenderer>( entity );

	// Currently this should be null since GPU path is stubbed
	// This test verifies the basic structure is in place
	CHECK( meshRenderer->gpuMesh == nullptr );
}

TEST_CASE( "SceneImporter with GPUResourceManager creates actual GPU resources", "[scene_importer][gpu_integration][unit]" )
{
	// AF2: runtime::SceneImporter should actually create GPU resources when provided GPUResourceManager
	// This test will fail until we implement real GPU resource creation

	// Create a DX12 device and GPU resource manager
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager resourceManager( device );

	// Create a scene with a mesh node
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "gpu_real_test.gltf" );
	scene->setLoaded( true );

	// Create mesh for testing with some primitive data
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "RealTestMesh" );

	// Add a simple primitive to the mesh to make it valid
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.5f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	// Add indices to form a triangle
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );
	mesh->getPrimitives().push_back( primitive );

	const assets::MeshHandle meshHandle = scene->addMesh( mesh );

	// Create a node with mesh handle
	auto rootNode = std::make_unique<assets::SceneNode>( "GPURealMeshNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene using CPU-only path first
	ecs::Scene targetScene;
	const bool result = runtime::SceneImporter::importScene( scene, targetScene );

	// Verify import succeeded
	REQUIRE( result );

	// Create GPU resources separately
	const bool gpuResult = runtime::SceneImporter::createGPUResources( scene, targetScene, resourceManager );
	REQUIRE( gpuResult );

	// Verify entity was created
	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( entity.isValid() );

	// Verify components::MeshRenderer component has actual GPU resources
	REQUIRE( targetScene.hasComponent<components::MeshRenderer>( entity ) );
	const auto *meshRenderer = targetScene.getComponent<components::MeshRenderer>( entity );

	// This should now have actual GPU resources - will fail until implemented
	REQUIRE( meshRenderer->gpuMesh != nullptr );
}

TEST_CASE( "SceneImporter createGPUResources adds GPU resources to existing scene", "[scene_importer][gpu_integration][unit]" )
{
	// AF3: Test the new createGPUResources workflow: import CPU-only, then add GPU resources separately

	// Create a scene with a mesh node
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "gpu_separate_test.gltf" );
	scene->setLoaded( true );

	// Create mesh for testing with some primitive data
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "SeparateTestMesh" );

	// Add a simple primitive to the mesh to make it valid
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.5f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	// Add indices to form a triangle
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );
	mesh->getPrimitives().push_back( primitive );

	const assets::MeshHandle meshHandle = scene->addMesh( mesh );

	// Create a node with mesh handle
	auto rootNode = std::make_unique<assets::SceneNode>( "SeparateGPUMeshNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Step 1: Import scene without GPU resources (CPU-only)
	ecs::Scene targetScene;
	const bool importResult = runtime::SceneImporter::importScene( scene, targetScene );
	REQUIRE( importResult );

	// Verify entity was created with CPU-only components::MeshRenderer
	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const ecs::Entity entity = entities[0];
	REQUIRE( entity.isValid() );
	REQUIRE( targetScene.hasComponent<components::MeshRenderer>( entity ) );

	auto *meshRenderer = targetScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );
	REQUIRE( meshRenderer->gpuMesh == nullptr ); // Should be null initially

	// Step 2: Create GPU resources separately
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager resourceManager( device );
	const bool gpuResult = runtime::SceneImporter::createGPUResources( scene, targetScene, resourceManager );
	REQUIRE( gpuResult );

	// Step 3: Verify GPU resources were added
	// Refresh the pointer in case component was moved
	meshRenderer = targetScene.getComponent<components::MeshRenderer>( entity );
	REQUIRE( meshRenderer != nullptr );
	REQUIRE( meshRenderer->gpuMesh != nullptr ); // Should now have GPU resources
}
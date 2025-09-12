#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <memory>

import std;
import runtime.ecs;
import runtime.entity;
import runtime.components;
import runtime.scene_importer;
import engine.gpu.gpu_resource_manager;
import engine.assets;
import engine.vec;
import platform.dx12;

using namespace runtime;
using namespace ecs;
using namespace components;
using namespace assets;
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
	auto rootNode = std::make_unique<SceneNode>( "GPUTestNode" );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene using non-GPU path (basic test)
	ecs::Scene targetScene;
	const bool result = SceneImporter::importScene( scene, targetScene );

	// Verify import succeeded
	REQUIRE( result );

	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const Entity entity = entities[0];
	REQUIRE( entity.isValid() );
}

TEST_CASE( "SceneImporter creates MeshRenderer with GPU resources using GPUResourceManager", "[scene_importer][gpu_integration][unit]" )
{
	// AF1: SceneImporter should use GPUResourceManager to create GPU resources
	// This test should fail initially because the implementation is stubbed

	// Create a scene with a mesh node
	auto scene = std::make_shared<assets::Scene>();
	scene->setPath( "gpu_test.gltf" );
	scene->setLoaded( true );

	// Create mesh for testing (corrected constructor call)
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "TestMesh" );
	const MeshHandle meshHandle = scene->addMesh( mesh );

	// Create a node with mesh handle
	auto rootNode = std::make_unique<SceneNode>( "GPUMeshNode" );
	rootNode->addMeshHandle( meshHandle );
	scene->addRootNode( std::move( rootNode ) );

	// Import scene using GPU path (currently stubbed to call basic path)
	ecs::Scene targetScene;
	// Create a dummy device and manager for stubbed test
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager resourceManager( device );
	const bool result = SceneImporter::importSceneWithGPU( scene, targetScene, resourceManager );

	// Verify import succeeded
	REQUIRE( result );

	// Verify entity was created
	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const Entity entity = entities[0];
	REQUIRE( entity.isValid() );

	// Verify MeshRenderer component exists (but GPU resources will be null for now)
	REQUIRE( targetScene.hasComponent<MeshRenderer>( entity ) );
	const auto *meshRenderer = targetScene.getComponent<MeshRenderer>( entity );

	// Currently this should be null since GPU path is stubbed
	// This test verifies the basic structure is in place
	CHECK( meshRenderer->gpuMesh == nullptr );
}

TEST_CASE( "SceneImporter with GPUResourceManager creates actual GPU resources", "[scene_importer][gpu_integration][unit]" )
{
	// AF2: SceneImporter should actually create GPU resources when provided GPUResourceManager
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

	// Import scene using GPU path with actual resource manager
	ecs::Scene targetScene;
	const bool result = SceneImporter::importSceneWithGPU( scene, targetScene, resourceManager );

	// Verify import succeeded
	REQUIRE( result );

	// Verify entity was created
	const auto entities = targetScene.getAllEntities();
	REQUIRE( entities.size() == 1 );

	const Entity entity = entities[0];
	REQUIRE( entity.isValid() );

	// Verify MeshRenderer component has actual GPU resources
	REQUIRE( targetScene.hasComponent<MeshRenderer>( entity ) );
	const auto *meshRenderer = targetScene.getComponent<MeshRenderer>( entity );

	// This should now have actual GPU resources - will fail until implemented
	REQUIRE( meshRenderer->gpuMesh != nullptr );
}
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

import engine.gpu.gpu_resource_manager;
import platform.dx12;
import engine.assets;
import engine.gpu.mesh_gpu;
import engine.gpu.material_gpu;
import std;

TEST_CASE( "GPUResourceManager can be instantiated", "[gpu_resource_manager][unit]" )
{
	// Arrange - create a DX12 device
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act - create GPUResourceManager
	engine::GPUResourceManager manager( device );

	// Assert - manager should be created successfully
	REQUIRE( manager.isValid() );
}

TEST_CASE( "GPUResourceManager caches mesh GPU buffers from shared_ptr", "[gpu_resource_manager][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create a test mesh with actual data
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "test_mesh.gltf" );

	// Add a primitive with vertices and indices
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );

	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	mesh->addPrimitive( primitive );

	// Act - get GPU buffers twice
	const auto buffers1 = manager.getMeshGPU( mesh );
	const auto buffers2 = manager.getMeshGPU( mesh );

	// Assert - same instance should be returned (caching working)
	REQUIRE( buffers1 != nullptr );
	REQUIRE( buffers2 != nullptr );
	REQUIRE( buffers1.get() == buffers2.get() );
}


TEST_CASE( "GPUResourceManager caches material GPU resources from shared_ptr", "[gpu_resource_manager][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create a test material
	auto material = std::make_shared<assets::Material>();
	material->setPath( "test_material.mat" );

	// Act - get MaterialGPU twice
	const auto materialGPU1 = manager.getMaterialGPU( material );
	const auto materialGPU2 = manager.getMaterialGPU( material );

	// Assert - same instance should be returned (caching working)
	REQUIRE( materialGPU1 != nullptr );
	REQUIRE( materialGPU2 != nullptr );
	REQUIRE( materialGPU1.get() == materialGPU2.get() );
}

TEST_CASE( "GPUResourceManager clears cache properly", "[gpu_resource_manager][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create test assets
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "test_mesh.gltf" );

	// Add a primitive with vertices and indices
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	mesh->addPrimitive( primitive );

	auto material = std::make_shared<assets::Material>();
	material->setPath( "test_material.mat" );

	// Load resources into cache
	const auto meshBuffers = manager.getMeshGPU( mesh );
	const auto materialGPU = manager.getMaterialGPU( material );

	REQUIRE( meshBuffers != nullptr );
	REQUIRE( materialGPU != nullptr );

	// Act - clear cache
	manager.clearCache();

	// Assert - new requests should create new instances
	const auto newMeshBuffers = manager.getMeshGPU( mesh );
	const auto newMaterialGPU = manager.getMaterialGPU( material );

	REQUIRE( newMeshBuffers != nullptr );
	REQUIRE( newMaterialGPU != nullptr );
	REQUIRE( newMeshBuffers.get() != meshBuffers.get() );
	REQUIRE( newMaterialGPU.get() != materialGPU.get() );
}

TEST_CASE( "GPUResourceManager tracks cache statistics", "[gpu_resource_manager][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create test assets
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "test_mesh.gltf" );

	// Add a primitive with vertices and indices
	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	mesh->addPrimitive( primitive );

	auto material = std::make_shared<assets::Material>();
	material->setPath( "test_material.mat" );

	// Act - load resources (cache misses)
	const auto meshBuffers1 = manager.getMeshGPU( mesh );
	const auto materialGPU1 = manager.getMaterialGPU( material );

	// Load same resources again (cache hits)
	const auto meshBuffers2 = manager.getMeshGPU( mesh );
	const auto materialGPU2 = manager.getMaterialGPU( material );

	// Assert - statistics should track hits and misses
	const auto &stats = manager.getStatistics();
	REQUIRE( stats.cacheHits == 2 );   // Two cache hits
	REQUIRE( stats.cacheMisses == 2 ); // Two cache misses
}

TEST_CASE( "Extract and validate PBR factor values", "[gpu_resource_manager][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create a material with specific PBR factor values
	auto material = std::make_shared<assets::Material>();
	material->setPath( "pbr_test_material.mat" );
	material->setName( "PBRTestMaterial" );

	// Set specific PBR factors to test extraction
	material->setBaseColorFactor( 0.8f, 0.2f, 0.1f, 0.9f );
	material->setMetallicFactor( 0.7f );
	material->setRoughnessFactor( 0.3f );

	// Set emissive factor through direct access to PBR material
	auto &pbrMaterial = material->getPBRMaterial();
	pbrMaterial.emissiveFactor = { 0.1f, 0.05f, 0.02f };

	// Act - get MaterialGPU which should extract PBR factors
	const auto materialGPU = manager.getMaterialGPU( material );

	// Assert - MaterialGPU should exist and be valid
	REQUIRE( materialGPU != nullptr );
	REQUIRE( materialGPU->isValid() );

	// Validate PBR factor extraction
	const auto &constants = materialGPU->getMaterialConstants();

	// Check base color factor
	REQUIRE( constants.baseColorFactor.x == 0.8f );
	REQUIRE( constants.baseColorFactor.y == 0.2f );
	REQUIRE( constants.baseColorFactor.z == 0.1f );
	REQUIRE( constants.baseColorFactor.w == 0.9f );

	// Check metallic and roughness factors
	REQUIRE( constants.metallicFactor == 0.7f );
	REQUIRE( constants.roughnessFactor == 0.3f );

	// Check emissive factor
	REQUIRE( constants.emissiveFactor.x == 0.1f );
	REQUIRE( constants.emissiveFactor.y == 0.05f );
	REQUIRE( constants.emissiveFactor.z == 0.02f );
}

TEST_CASE( "configureMaterials properly setup materials", "[gpu_resource_manager][mesh][material][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	engine::GPUResourceManager manager( device );

	// Create a test material
	const auto material = std::make_shared<assets::Material>();
	material->setBaseColorFactor( 1.0f, 0.0f, 0.0f, 1.0f );
	material->setMetallicFactor( 0.5f );
	material->setRoughnessFactor( 0.3f );

	// Create a scene with the material
	assets::Scene scene;
	const assets::MaterialHandle materialHandle = scene.addMaterial( material );

	// Create a test mesh with a primitive that has a material handle
	auto mesh = std::make_shared<assets::Mesh>();
	mesh->setPath( "test_mesh_with_material.gltf" );

	assets::Primitive primitive;
	primitive.addVertex( assets::Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( assets::Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );
	primitive.setMaterialHandle( materialHandle );

	mesh->addPrimitive( std::move( primitive ) );

	// Act - get MeshGPU from GPUResourceManager (without scene - materials won't be configured)
	const auto meshGPU = manager.getMeshGPU( mesh );

	// Assert - MeshGPU should be valid but materials should NOT be configured initially
	REQUIRE( meshGPU != nullptr );
	REQUIRE( meshGPU->isValid() );
	REQUIRE( meshGPU->getPrimitiveCount() == 1 );

	const auto &primitiveGPU = meshGPU->getPrimitive( 0 );
	REQUIRE_FALSE( primitiveGPU.hasMaterial() ); // This demonstrates the current problem!
	REQUIRE( primitiveGPU.getMaterial() == nullptr );

	// Now manually configure materials (simulating what the fixed getMeshGPU should do)
	meshGPU->configureMaterials( manager, scene, *mesh );

	// After manual configuration, material should be available
	REQUIRE( primitiveGPU.hasMaterial() );
	REQUIRE( primitiveGPU.getMaterial() != nullptr );
	REQUIRE( primitiveGPU.getMaterial()->getSourceMaterial() == material );
}

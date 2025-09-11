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

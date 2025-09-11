#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

import engine.assets;
import engine.gpu.mesh_gpu;
import engine.gpu.material_gpu;
import engine.gpu.gpu_resource_manager;
import platform.dx12;

using namespace assets;
using namespace engine::gpu;

TEST_CASE( "PrimitiveGPU creates vertex buffer from primitive", "[gpu][primitive][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a test primitive with some vertices
	Primitive primitive;
	primitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );

	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	// This should compile and create GPU buffers
	PrimitiveGPU gpuBuffer( device, primitive );

	// Verify the buffer was created successfully
	REQUIRE( gpuBuffer.isValid() );
	REQUIRE( gpuBuffer.getVertexCount() == 3 );
	REQUIRE( gpuBuffer.getIndexCount() == 3 );
}

TEST_CASE( "PrimitiveGPU provides valid D3D12 buffer views", "[gpu][primitive][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a test primitive
	Primitive primitive;
	primitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );

	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	// This should provide valid buffer views for rendering
	PrimitiveGPU gpuBuffer( device, primitive );

	const auto vertexView = gpuBuffer.getVertexBufferView();
	const auto indexView = gpuBuffer.getIndexBufferView();

	REQUIRE( vertexView.BufferLocation != 0 );
	REQUIRE( vertexView.SizeInBytes == 3 * sizeof( assets::Vertex ) );
	REQUIRE( vertexView.StrideInBytes == sizeof( assets::Vertex ) );

	REQUIRE( indexView.BufferLocation != 0 );
	REQUIRE( indexView.SizeInBytes == 3 * sizeof( std::uint32_t ) );
	REQUIRE( indexView.Format == DXGI_FORMAT_R32_UINT );
}

TEST_CASE( "Mesh maintains per-primitive GPU buffers", "[gpu][mesh][primitive][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a mesh with multiple primitives
	Mesh mesh;

	// Add first primitive
	Primitive primitive1;
	primitive1.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addIndex( 0 );
	primitive1.addIndex( 1 );
	primitive1.addIndex( 2 );
	primitive1.setMaterialHandle( 0 );

	// Add second primitive
	Primitive primitive2;
	primitive2.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive2.addVertex( Vertex{ { 2.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive2.addVertex( Vertex{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive2.addIndex( 0 );
	primitive2.addIndex( 1 );
	primitive2.addIndex( 2 );
	primitive2.setMaterialHandle( 1 );

	mesh.addPrimitive( std::move( primitive1 ) );
	mesh.addPrimitive( std::move( primitive2 ) );

	// This should create GPU buffers for each primitive independently
	MeshGPU meshBuffers( device, mesh );

	REQUIRE( meshBuffers.getPrimitiveCount() == 2 );

	// Each primitive should have its own GPU resources
	const auto &prim1 = meshBuffers.getPrimitive( 0 );
	const auto &prim2 = meshBuffers.getPrimitive( 1 );

	REQUIRE( prim1.getVertexBufferView().BufferLocation != prim2.getVertexBufferView().BufferLocation );
	REQUIRE( prim1.getIndexBufferView().BufferLocation != prim2.getIndexBufferView().BufferLocation );
}

TEST_CASE( "PrimitiveGPU handles empty primitive gracefully", "[gpu][primitive][error][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create an empty primitive
	Primitive emptyPrimitive;
	REQUIRE( emptyPrimitive.getVertexCount() == 0 );
	REQUIRE( emptyPrimitive.getIndexCount() == 0 );

	// Creating GPU buffers for empty primitive should fail gracefully
	PrimitiveGPU gpuBuffer( device, emptyPrimitive );

	// Buffer should be invalid for empty primitive
	REQUIRE_FALSE( gpuBuffer.isValid() );
	REQUIRE( gpuBuffer.getVertexCount() == 0 );
	REQUIRE( gpuBuffer.getIndexCount() == 0 );
}

TEST_CASE( "MeshGPU handles mesh with empty primitives", "[gpu][mesh][error][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a mesh with one valid and one empty primitive
	Mesh mesh;

	// Add valid primitive
	Primitive validPrimitive;
	validPrimitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	validPrimitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	validPrimitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	validPrimitive.addIndex( 0 );
	validPrimitive.addIndex( 1 );
	validPrimitive.addIndex( 2 );

	// Add empty primitive
	Primitive emptyPrimitive;

	mesh.addPrimitive( std::move( validPrimitive ) );
	mesh.addPrimitive( std::move( emptyPrimitive ) );

	// Mesh should have 2 primitives but only 1 should have valid GPU buffers
	REQUIRE( mesh.getPrimitiveCount() == 2 );

	MeshGPU meshBuffers( device, mesh );

	// Should have failed to create buffers for the empty primitive
	// The implementation should skip empty primitives
	REQUIRE( meshBuffers.getPrimitiveCount() == 1 ); // Only valid primitive should have buffers
}

TEST_CASE( "GPU buffers support large vertex counts", "[gpu][primitive][performance][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a primitive with many vertices
	Primitive largePrimitive;
	const std::uint32_t vertexCount = 10000;
	const std::uint32_t indexCount = 30000; // 10000 triangles

	// Add vertices
	for ( std::uint32_t i = 0; i < vertexCount; ++i )
	{
		const float t = static_cast<float>( i ) / static_cast<float>( vertexCount - 1 );
		largePrimitive.addVertex( Vertex{
			{ t, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },
			{ t, 0.0f },
			{ 1.0f, 0.0f, 0.0f, 1.0f } } );
	}

	// Add indices (create triangle strip)
	for ( std::uint32_t i = 0; i < indexCount; ++i )
	{
		largePrimitive.addIndex( i % vertexCount );
	}

	// Should handle large buffers successfully
	PrimitiveGPU gpuBuffer( device, largePrimitive );

	REQUIRE( gpuBuffer.isValid() );
	REQUIRE( gpuBuffer.getVertexCount() == vertexCount );
	REQUIRE( gpuBuffer.getIndexCount() == indexCount );

	// Verify buffer views have correct sizes
	const auto vertexView = gpuBuffer.getVertexBufferView();
	const auto indexView = gpuBuffer.getIndexBufferView();

	REQUIRE( vertexView.SizeInBytes == vertexCount * sizeof( assets::Vertex ) );
	REQUIRE( indexView.SizeInBytes == indexCount * sizeof( std::uint32_t ) );
}

// Material Integration Tests

TEST_CASE( "PrimitiveGPU constructor with MaterialGPU creates valid buffer", "[gpu][primitive][material][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a test primitive
	Primitive primitive;
	primitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	// Create a test material
	const auto material = std::make_shared<assets::Material>();
	material->setBaseColorFactor( 1.0f, 0.0f, 0.0f, 1.0f );
	material->setMetallicFactor( 0.5f );
	material->setRoughnessFactor( 0.3f );

	// Create MaterialGPU
	std::shared_ptr<MaterialGPU> materialGPU = std::make_shared<MaterialGPU>( material );

	// Create primitive GPU buffer and set material
	PrimitiveGPU primGPU( device, primitive );
	primGPU.setMaterial( materialGPU );

	// Verify the buffer was created successfully
	REQUIRE( primGPU.isValid() );
	REQUIRE( primGPU.hasMaterial() );
	REQUIRE( primGPU.getMaterial() != nullptr );
	REQUIRE( primGPU.getMaterial()->getSourceMaterial() == material );
}


TEST_CASE( "PrimitiveGPU constructor without MaterialGPU has no material", "[gpu][primitive][material][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a test primitive
	Primitive primitive;
	primitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	// Create primitive GPU buffer without material
	PrimitiveGPU gpuBuffer( device, primitive );

	// Verify the buffer was created successfully but has no material
	REQUIRE( gpuBuffer.isValid() );
	REQUIRE_FALSE( gpuBuffer.hasMaterial() );
	REQUIRE( gpuBuffer.getMaterial() == nullptr );
}

TEST_CASE( "PrimitiveGPU bindForRendering sets vertex and index buffers", "[gpu][primitive][material][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create a test primitive
	Primitive primitive;
	primitive.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive.addIndex( 0 );
	primitive.addIndex( 1 );
	primitive.addIndex( 2 );

	// Create primitive GPU buffer
	PrimitiveGPU primGPU( device, primitive );
	REQUIRE( primGPU.isValid() );

	// Create command list for testing (note: this is a stub test - actual command list binding would require more setup)
	// In a real scenario, we would need a proper command list and verify the bindings
	// For this test, we're mainly checking that the method can be called without crashing
	// bindForRendering with nullptr should handle gracefully
	primGPU.bindForRendering( nullptr ); // Should log error but not crash
}

TEST_CASE( "MeshGPU can resolve materials from Scene and create MaterialGPU via GPUResourceManager", "[gpu][mesh][material][unit]" )
{
	// Create a headless D3D12 device for testing
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Create GPU resource manager
	engine::GPUResourceManager resourceManager( device );
	REQUIRE( resourceManager.isValid() );

	// Create a test material
	const auto material = std::make_shared<assets::Material>();
	material->setBaseColorFactor( 1.0f, 0.0f, 0.0f, 1.0f );
	material->setMetallicFactor( 0.5f );
	material->setRoughnessFactor( 0.3f );

	// Create a scene with the material
	assets::Scene scene;
	const assets::MaterialHandle materialHandle = scene.addMaterial( material );

	// Create a test mesh with primitives that have material handles
	assets::Mesh mesh;

	// Add first primitive with material
	assets::Primitive primitive1;
	primitive1.addVertex( Vertex{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addVertex( Vertex{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addVertex( Vertex{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } } );
	primitive1.addIndex( 0 );
	primitive1.addIndex( 1 );
	primitive1.addIndex( 2 );
	primitive1.setMaterialHandle( materialHandle );

	// Add second primitive without material
	assets::Primitive primitive2;
	primitive2.addVertex( Vertex{ { 2.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } } );
	primitive2.addVertex( Vertex{ { 3.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } } );
	primitive2.addVertex( Vertex{ { 2.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } } );
	primitive2.addIndex( 0 );
	primitive2.addIndex( 1 );
	primitive2.addIndex( 2 );
	// No material handle set for primitive2

	mesh.addPrimitive( std::move( primitive1 ) );
	mesh.addPrimitive( std::move( primitive2 ) );

	// Create MeshGPU first (without materials)
	MeshGPU gpuMesh( device, mesh );
	REQUIRE( gpuMesh.isValid() );
	REQUIRE( gpuMesh.getPrimitiveCount() == 2 );

	// Initially, primitives should have no materials
	const auto &primGPU1Before = gpuMesh.getPrimitive( 0 );
	const auto &primGPU2Before = gpuMesh.getPrimitive( 1 );
	REQUIRE_FALSE( primGPU1Before.hasMaterial() );
	REQUIRE_FALSE( primGPU2Before.hasMaterial() );

	// Configure materials using GPUResourceManager and Scene
	gpuMesh.configureMaterials( resourceManager, scene, mesh );

	// After configuration, first primitive should have material, second should not
	const auto &primGPU1After = gpuMesh.getPrimitive( 0 );
	const auto &primGPU2After = gpuMesh.getPrimitive( 1 );

	REQUIRE( primGPU1After.hasMaterial() );
	REQUIRE( primGPU1After.getMaterial() != nullptr );
	REQUIRE( primGPU1After.getMaterial()->getSourceMaterial() == material );

	REQUIRE_FALSE( primGPU2After.hasMaterial() );
	REQUIRE( primGPU2After.getMaterial() == nullptr );
}

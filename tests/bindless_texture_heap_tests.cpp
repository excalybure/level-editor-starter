#include <catch2/catch_test_macros.hpp>
#include <graphics/texture/bindless_texture_heap.h>
#include <graphics/texture/texture_loader.h>
#include <platform/dx12/dx12_device.h>

using namespace graphics::texture;
using namespace dx12;

TEST_CASE( "BindlessTextureHeap initializes with 4096 slots", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	const bool result = heap.initialize( device.get(), 4096 );

	REQUIRE( result );
	REQUIRE( heap.getHeap() != nullptr );
	REQUIRE( heap.getMaxDescriptors() == 4096 );
	REQUIRE( heap.getAllocatedCount() == 0 );
	REQUIRE( heap.getAvailableCount() == 4096 );

	device.shutdown();
}

TEST_CASE( "BindlessTextureHeap validates device pointer", "[texture][bindless][unit]" )
{
	BindlessTextureHeap heap;
	const bool result = heap.initialize( nullptr, 4096 );

	REQUIRE_FALSE( result );
}

TEST_CASE( "BindlessTextureHeap allocates unique descriptor indices", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	REQUIRE( heap.initialize( device.get(), 100 ) );

	// Allocate 100 descriptors
	std::vector<uint32_t> indices;
	for ( int i = 0; i < 100; ++i )
	{
		const auto index = heap.allocate();
		REQUIRE( index.has_value() );
		indices.push_back( index.value() );
	}

	// All indices should be unique
	std::sort( indices.begin(), indices.end() );
	for ( size_t i = 0; i < indices.size(); ++i )
	{
		REQUIRE( indices[i] == i );
	}

	REQUIRE( heap.getAllocatedCount() == 100 );
	REQUIRE( heap.getAvailableCount() == 0 );

	device.shutdown();
}

TEST_CASE( "BindlessTextureHeap deallocates and reuses slots", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	REQUIRE( heap.initialize( device.get(), 100 ) );

	// Allocate some slots
	const auto index1 = heap.allocate();
	const auto index2 = heap.allocate();
	const auto index3 = heap.allocate();
	REQUIRE( index1.has_value() );
	REQUIRE( index2.has_value() );
	REQUIRE( index3.has_value() );

	// Deallocate middle slot
	heap.deallocate( index2.value() );
	REQUIRE( heap.getAllocatedCount() == 2 );
	REQUIRE( heap.getAvailableCount() == 98 );

	// Reallocate should reuse the freed slot
	const auto index4 = heap.allocate();
	REQUIRE( index4.has_value() );
	REQUIRE( index4.value() == index2.value() );

	device.shutdown();
}

TEST_CASE( "BindlessTextureHeap fails when heap is full", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	REQUIRE( heap.initialize( device.get(), 10 ) );

	// Allocate all 10 slots
	for ( int i = 0; i < 10; ++i )
	{
		const auto index = heap.allocate();
		REQUIRE( index.has_value() );
	}

	// 11th allocation should fail
	const auto failedIndex = heap.allocate();
	REQUIRE_FALSE( failedIndex.has_value() );

	device.shutdown();
}

TEST_CASE( "BindlessTextureHeap creates SRV at valid index", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	REQUIRE( heap.initialize( device.get(), 100 ) );

	// Create a simple texture
	Texture texture;
	ImageData imageData;
	imageData.width = 2;
	imageData.height = 2;
	imageData.channels = 4;
	imageData.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	imageData.pixels.resize( 2 * 2 * 4, 255 );
	REQUIRE( texture.createFromImageData( &device, imageData ) );

	// Allocate a slot
	const auto index = heap.allocate();
	REQUIRE( index.has_value() );

	// Create SRV at this index
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	// This should not crash
	heap.createSRV( index.value(), texture.getResource(), &srvDesc );

	// Verify we can get handles
	const auto cpuHandle = heap.getCpuHandle( index.value() );
	const auto gpuHandle = heap.getGpuHandle( index.value() );
	REQUIRE( cpuHandle.ptr != 0 );
	REQUIRE( gpuHandle.ptr != 0 );

	device.shutdown();
}

TEST_CASE( "BindlessTextureHeap provides correct CPU and GPU handles", "[texture][bindless][unit]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	BindlessTextureHeap heap;
	REQUIRE( heap.initialize( device.get(), 100 ) );

	// Allocate first and 10th slots (indices 0 and 9)
	const auto index0 = heap.allocate();
	for ( int i = 1; i < 9; ++i )
	{
		heap.allocate();
	}
	const auto index9 = heap.allocate();

	REQUIRE( index0.has_value() );
	REQUIRE( index9.has_value() );
	REQUIRE( index0.value() == 0 );
	REQUIRE( index9.value() == 9 );

	// Get handles
	const auto cpu0 = heap.getCpuHandle( 0 );
	const auto cpu9 = heap.getCpuHandle( 9 );
	const auto gpu0 = heap.getGpuHandle( 0 );
	const auto gpu9 = heap.getGpuHandle( 9 );

	// CPU handles should be offset correctly
	const auto heapStart = heap.getHeap()->GetCPUDescriptorHandleForHeapStart();
	const uint32_t descriptorSize = device.get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	REQUIRE( cpu0.ptr == heapStart.ptr );
	REQUIRE( cpu9.ptr == heapStart.ptr + 9 * descriptorSize );

	// GPU handles should also be offset correctly
	const auto gpuHeapStart = heap.getHeap()->GetGPUDescriptorHandleForHeapStart();
	REQUIRE( gpu0.ptr == gpuHeapStart.ptr );
	REQUIRE( gpu9.ptr == gpuHeapStart.ptr + 9 * descriptorSize );

	device.shutdown();
}

#include "bindless_texture_heap.h"
#include <core/console.h>
#include <algorithm>

namespace graphics::texture
{

bool BindlessTextureHeap::initialize( ID3D12Device *device, uint32_t maxDescriptors )
{
	if ( !device )
	{
		console::error( "BindlessTextureHeap::initialize: Device is null" );
		return false;
	}

	if ( maxDescriptors == 0 )
	{
		console::error( "BindlessTextureHeap::initialize: maxDescriptors must be > 0" );
		return false;
	}

	m_device = device;
	m_maxDescriptors = maxDescriptors;
	m_allocatedCount = 0;

	// Create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = maxDescriptors;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	const HRESULT hr = device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_heap ) );
	if ( FAILED( hr ) )
	{
		console::error( "BindlessTextureHeap::initialize: Failed to create descriptor heap" );
		return false;
	}

	// Get descriptor size
	m_descriptorSize = device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	// Initialize free list with all slots in reverse order
	// (so allocate() returns 0, 1, 2, ... by popping from the back)
	m_freeList.reserve( maxDescriptors );
	for ( uint32_t i = maxDescriptors; i > 0; --i )
	{
		m_freeList.push_back( i - 1 );
	}

	return true;
}

void BindlessTextureHeap::shutdown()
{
	m_heap.Reset();
	m_device = nullptr;
	m_descriptorSize = 0;
	m_maxDescriptors = 0;
	m_allocatedCount = 0;
	m_freeList.clear();
}

std::optional<uint32_t> BindlessTextureHeap::allocate()
{
	if ( m_freeList.empty() )
	{
		console::error( "BindlessTextureHeap::allocate: Heap is full ({} descriptors allocated)", m_allocatedCount );
		return std::nullopt;
	}

	// Take the last element from the free list (most efficient)
	const uint32_t index = m_freeList.back();
	m_freeList.pop_back();
	++m_allocatedCount;

	return index;
}

void BindlessTextureHeap::deallocate( uint32_t index )
{
	if ( index >= m_maxDescriptors )
	{
		console::error( "BindlessTextureHeap::deallocate: Invalid index {} (max {})", index, m_maxDescriptors );
		return;
	}

	// Add back to free list
	m_freeList.push_back( index );
	--m_allocatedCount;
}

void BindlessTextureHeap::createSRV( uint32_t index, ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc )
{
	if ( !m_device || !m_heap )
	{
		console::error( "BindlessTextureHeap::createSRV: Heap not initialized" );
		return;
	}

	if ( index >= m_maxDescriptors )
	{
		console::error( "BindlessTextureHeap::createSRV: Invalid index {} (max {})", index, m_maxDescriptors );
		return;
	}

	if ( !resource )
	{
		console::error( "BindlessTextureHeap::createSRV: Resource is null" );
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = getCpuHandle( index );
	m_device->CreateShaderResourceView( resource, desc, cpuHandle );
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessTextureHeap::getCpuHandle( uint32_t index ) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += index * m_descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessTextureHeap::getGpuHandle( uint32_t index ) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += index * m_descriptorSize;
	return handle;
}

} // namespace graphics::texture

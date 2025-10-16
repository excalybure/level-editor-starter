#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace graphics::texture
{

// Bindless descriptor heap for texture SRVs
// Manages a large descriptor heap with slot-based allocation
class BindlessTextureHeap
{
public:
	BindlessTextureHeap() = default;
	~BindlessTextureHeap() = default;

	// No copy/move for now
	BindlessTextureHeap( const BindlessTextureHeap & ) = delete;
	BindlessTextureHeap &operator=( const BindlessTextureHeap & ) = delete;

	// Initialize the descriptor heap
	bool initialize( ID3D12Device *device, uint32_t maxDescriptors = 4096 );
	void shutdown();

	// Allocate a descriptor slot, returns index
	std::optional<uint32_t> allocate();

	// Free a descriptor slot
	void deallocate( uint32_t index );

	// Create SRV at the given slot index
	void createSRV( uint32_t index, ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc );

	// Get heap for binding to command list
	ID3D12DescriptorHeap *getHeap() const { return m_heap.Get(); }

	// Get CPU descriptor handle for a given index
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle( uint32_t index ) const;

	// Get GPU descriptor handle for a given index
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle( uint32_t index ) const;

	// Query capacity and usage
	uint32_t getMaxDescriptors() const { return m_maxDescriptors; }
	uint32_t getAllocatedCount() const { return m_allocatedCount; }
	uint32_t getAvailableCount() const { return m_maxDescriptors - m_allocatedCount; }

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
	ID3D12Device *m_device = nullptr;
	uint32_t m_descriptorSize = 0;
	uint32_t m_maxDescriptors = 0;
	uint32_t m_allocatedCount = 0;
	std::vector<uint32_t> m_freeList; // Available slots
};

} // namespace graphics::texture

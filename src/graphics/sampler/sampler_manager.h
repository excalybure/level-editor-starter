#pragma once

#include "platform/dx12/dx12_device.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace graphics
{

// Predefined sampler types for common use cases
enum class SamplerType : uint32_t
{
	LinearWrap = 0,
	LinearClamp = 1,
	PointWrap = 2,
	PointClamp = 3,
	AnisotropicWrap = 4,
	AnisotropicClamp = 5,
	Count // Must be last
};

// Manages sampler descriptor heap and creation
// Creates common samplers during initialization for shader binding
class SamplerManager
{
public:
	SamplerManager() = default;
	~SamplerManager() = default;

	// Initialize sampler heap and create common samplers
	bool initialize( dx12::Device *device );

	// Cleanup resources
	void shutdown();

	// Get the sampler descriptor heap (for binding to command list)
	ID3D12DescriptorHeap *getHeap() const { return m_heap.Get(); }

	// Get descriptor handle for a specific sampler type
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle( SamplerType type ) const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle( SamplerType type ) const;

	// Get the starting GPU handle for the entire sampler table
	D3D12_GPU_DESCRIPTOR_HANDLE getTableStartGpuHandle() const;

	// Check if manager is initialized
	bool isInitialized() const { return m_heap != nullptr; }

private:
	// Create a specific sampler descriptor
	void createSampler( SamplerType type, const D3D12_SAMPLER_DESC &desc );

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
	dx12::Device *m_device = nullptr;
	uint32_t m_descriptorSize = 0;
};

} // namespace graphics

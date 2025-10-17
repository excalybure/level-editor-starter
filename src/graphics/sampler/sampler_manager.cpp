#include "graphics/sampler/sampler_manager.h"
#include "core/console.h"

namespace graphics
{

bool SamplerManager::initialize( dx12::Device *device )
{
	if ( !device || !device->get() )
	{
		console::error( "SamplerManager::initialize: invalid device" );
		return false;
	}

	m_device = device;

	// Create sampler descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	heapDesc.NumDescriptors = static_cast<UINT>( SamplerType::Count );
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	const HRESULT hr = m_device->get()->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_heap ) );
	if ( FAILED( hr ) )
	{
		console::error( "SamplerManager::initialize: failed to create sampler descriptor heap" );
		return false;
	}

	m_descriptorSize = m_device->get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );

	// Create common samplers
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	// Linear Wrap
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	createSampler( SamplerType::LinearWrap, samplerDesc );

	// Linear Clamp
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	createSampler( SamplerType::LinearClamp, samplerDesc );

	// Point Wrap
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	createSampler( SamplerType::PointWrap, samplerDesc );

	// Point Clamp
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	createSampler( SamplerType::PointClamp, samplerDesc );

	// Anisotropic Wrap
	samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	createSampler( SamplerType::AnisotropicWrap, samplerDesc );

	// Anisotropic Clamp
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	createSampler( SamplerType::AnisotropicClamp, samplerDesc );

	console::info( "SamplerManager: initialized with {} samplers", static_cast<uint32_t>( SamplerType::Count ) );
	return true;
}

void SamplerManager::shutdown()
{
	m_heap.Reset();
	m_device = nullptr;
	m_descriptorSize = 0;
}

void SamplerManager::beginFrame( ID3D12GraphicsCommandList *commandList )
{
	if ( !commandList || !m_heap )
	{
		console::error( "SamplerManager::beginFrame: invalid command list or heap not initialized" );
		return;
	}

	// Set the sampler descriptor heap on the command list
	ID3D12DescriptorHeap *const ppHeaps[] = { m_heap.Get() };
	commandList->SetDescriptorHeaps( 1, ppHeaps );
}

void SamplerManager::endFrame()
{
	// No cleanup needed per-frame for samplers
	// This is a placeholder for symmetry with beginFrame
}

D3D12_GPU_DESCRIPTOR_HANDLE SamplerManager::getGpuHandle( SamplerType type ) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<uint32_t>( type ) * m_descriptorSize;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SamplerManager::getCpuHandle( SamplerType type ) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<uint32_t>( type ) * m_descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE SamplerManager::getTableStartGpuHandle() const
{
	return m_heap->GetGPUDescriptorHandleForHeapStart();
}

void SamplerManager::createSampler( SamplerType type, const D3D12_SAMPLER_DESC &desc )
{
	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = getCpuHandle( type );
	m_device->get()->CreateSampler( &desc, cpuHandle );
}

} // namespace graphics

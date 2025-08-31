// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <windows.h>

module platform.dx12;

import std;

namespace dx12
{

bool Texture::createRenderTarget( Device *device, UINT width, UINT height, DXGI_FORMAT format )
{
	if ( !device || width == 0 || height == 0 )
		return false;

	m_device = device;
	m_width = width;
	m_height = height;
	m_format = format;

	// Create texture resource
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// Create heap properties
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	// Clear value for render target
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = 0.2f; // Background color - dark gray
	clearValue.Color[1] = 0.2f;
	clearValue.Color[2] = 0.2f;
	clearValue.Color[3] = 1.0f;

	try
	{
		throwIfFailed( device->get()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS( &m_resource ) ) );

		m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		return true;
	}
	catch ( const std::exception & )
	{
		return false;
	}
}

bool Texture::createShaderResourceView( Device *device, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle )
{
	if ( !device || !m_resource )
		return false;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	device->get()->CreateShaderResourceView( m_resource.Get(), &srvDesc, srvHandle );

	// Convert CPU handle to GPU handle for ImGui
	// Note: This requires the SRV heap to be shader visible
	ID3D12DescriptorHeap *srvHeap = device->getImguiDescriptorHeap();
	if ( srvHeap )
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart = srvHeap->GetGPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = srvHeap->GetCPUDescriptorHandleForHeapStart();

		UINT64 offset = srvHandle.ptr - cpuHandleStart.ptr;
		m_srvGpuHandle.ptr = gpuHandleStart.ptr + offset;
	}

	return true;
}

bool Texture::resize( Device *device, UINT width, UINT height )
{
	if ( width == m_width && height == m_height )
		return true; // No change needed

	if ( !device )
		device = m_device; // Use cached device

	if ( !device )
		return false;

	// Release old resource
	m_resource.Reset();

	// Create new resource with new dimensions
	return createRenderTarget( device, width, height, m_format );
}

void Texture::transitionTo( ID3D12GraphicsCommandList *commandList, D3D12_RESOURCE_STATES newState )
{
	if ( !commandList || !m_resource || m_currentState == newState )
		return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_resource.Get();
	barrier.Transition.StateBefore = m_currentState;
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier( 1, &barrier );
	m_currentState = newState;
}

// TextureManager implementation
bool TextureManager::initialize( Device *device )
{
	if ( !device )
		return false;

	m_device = device;

	// Create RTV descriptor heap for render targets
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = kMaxTextures;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	try
	{
		throwIfFailed( device->get()->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_rtvHeap ) ) );
		m_rtvDescriptorSize = device->get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

		// Create SRV descriptor heap for shader resource views (ImGui textures)
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = kMaxTextures;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		throwIfFailed( device->get()->CreateDescriptorHeap( &srvHeapDesc, IID_PPV_ARGS( &m_srvHeap ) ) );
		m_srvDescriptorSize = device->get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

		return true;
	}
	catch ( const std::exception & )
	{
		return false;
	}
}

void TextureManager::shutdown()
{
	m_srvHeap.Reset();
	m_rtvHeap.Reset();
	m_device = nullptr;
	m_currentRtvIndex = 0;
	m_currentSrvIndex = 0;
}

std::shared_ptr<Texture> TextureManager::createViewportRenderTarget( UINT width, UINT height )
{
	if ( !m_device || m_currentRtvIndex >= kMaxTextures || m_currentSrvIndex >= kMaxTextures )
		return nullptr;

	auto texture = std::make_shared<Texture>();

	// Create the render target
	if ( !texture->createRenderTarget( m_device, width, height ) )
		return nullptr;

	// Get RTV handle
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_currentRtvIndex * m_rtvDescriptorSize;

	// Create render target view
	m_device->get()->CreateRenderTargetView( texture->getResource(), nullptr, rtvHandle );
	texture->m_rtvHandle = rtvHandle;

	// Get SRV handle and create shader resource view
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += m_currentSrvIndex * m_srvDescriptorSize;

	if ( !texture->createShaderResourceView( m_device, srvHandle ) )
		return nullptr;

	++m_currentRtvIndex;
	++m_currentSrvIndex;

	return texture;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::getNextSrvHandle()
{
	if ( !m_srvHeap || m_currentSrvIndex >= kMaxTextures )
		return {};

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += m_currentSrvIndex * m_srvDescriptorSize;
	return handle;
}

} // namespace dx12

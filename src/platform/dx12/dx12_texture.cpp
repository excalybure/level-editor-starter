// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <windows.h>
#include <cassert>

module platform.dx12;

import std;
import runtime.console;

namespace dx12
{

bool Texture::createRenderTarget( Device *device, UINT width, UINT height, DXGI_FORMAT format )
{
	if ( !device )
	{
		console::error( "Texture::createRenderTarget: Device is null" );
		return false;
	}

	if ( width == 0 || height == 0 )
	{
		console::error( "Texture::createRenderTarget: Invalid dimensions {}x{}", width, height );
		return false;
	}

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

	// Clear value for render target - match the clear color used in clearRenderTarget
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = 0.1f; // Background color - dark gray to match viewport clear
	clearValue.Color[1] = 0.1f;
	clearValue.Color[2] = 0.1f;
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

bool Texture::createShaderResourceView( Device *device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle )
{
	if ( !device || !m_resource )
	{
		console::error( "Texture::createShaderResourceView: Invalid device ({}) or resource ({})",
			static_cast<void *>( device ),
			static_cast<void *>( m_resource.Get() ) );
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view
	device->get()->CreateShaderResourceView( m_resource.Get(), &srvDesc, srvCpuHandle );

	// GPU handle will be set by TextureManager after this call
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
	if ( !createRenderTarget( device, width, height, m_format ) )
		return false;

	// Update the RTV to point to the new resource
	if ( m_rtvHandle.ptr != 0 )
	{
		// Recreate the render target view for the new resource
		device->get()->CreateRenderTargetView( m_resource.Get(), nullptr, m_rtvHandle );
	}

	// Update the SRV to point to the new resource
	assert( m_srvCpuHandle.ptr != 0 );
	if ( m_srvCpuHandle.ptr != 0 )
	{
		if ( !createShaderResourceView( device, m_srvCpuHandle ) )
		{
			console::error( "Texture::resize: Failed to update SRV!" );
			return false;
		}
	}

	return true;
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

bool Texture::clearRenderTarget( Device *device, const float clearColor[4] )
{
	if ( !device || !m_resource || m_rtvHandle.ptr == 0 )
		return false;

	// Get command list
	ID3D12GraphicsCommandList *commandList = device->getCommandList();
	if ( !commandList )
		return false;

	// Transition to render target state if needed
	transitionTo( commandList, D3D12_RESOURCE_STATE_RENDER_TARGET );

	// Set render target (needed for clearing)
	commandList->OMSetRenderTargets( 1, &m_rtvHandle, FALSE, nullptr );

	// Clear the render target
	commandList->ClearRenderTargetView( m_rtvHandle, clearColor, 0, nullptr );

	return true;
}

// TextureManager implementation
bool TextureManager::initialize( Device *device )
{
	if ( !device )
	{
		console::error( "TextureManager::initialize: Invalid device (device == nullptr)" );
		return false;
	}

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

		// Use ImGui's descriptor heap but reserve indices safely
		// ImGui typically uses index 0 for font texture, we'll start from index 16 for safety
		m_srvHeap = device->getImguiDescriptorHeap();
		m_srvDescriptorSize = device->get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

		m_currentRtvIndex = 0;
		m_currentSrvIndex = 0;

		return true;
	}
	catch ( const std::exception &e )
	{
		console::error( "TextureManager::initialize: Failed to create RTV heap ({})", e.what() );
		return false;
	}
}

void TextureManager::shutdown()
{
	// Don't reset m_srvHeap since we don't own it (it's ImGui's heap)
	m_srvHeap = nullptr;
	m_rtvHeap.Reset();
	m_device = nullptr;
	m_currentRtvIndex = 0;
	m_currentSrvIndex = 0;
}

std::shared_ptr<Texture> TextureManager::createViewportRenderTarget( UINT width, UINT height )
{
	// Validate input parameters
	if ( !m_device )
	{
		console::error( "TextureManager::createViewportRenderTarget: Device is null" );
		return nullptr;
	}

	if ( width == 0 || height == 0 )
	{
		console::error( "TextureManager::createViewportRenderTarget: Invalid dimensions {}x{}", width, height );
		return nullptr;
	}

	if ( !m_srvHeap )
	{
		console::error( "TextureManager::createViewportRenderTarget: SRV heap is null" );
		return nullptr;
	}

	if ( m_currentRtvIndex >= kMaxTextures || m_currentSrvIndex >= kMaxTextures )
	{
		console::error( "TextureManager::createViewportRenderTarget: Descriptor heap full (RTV: {}/{}, SRV: {}/{})",
			m_currentRtvIndex,
			kMaxTextures,
			m_currentSrvIndex,
			kMaxTextures );
		return nullptr;
	}

	auto texture = std::make_shared<Texture>();

	// Create the render target
	if ( !texture->createRenderTarget( m_device, width, height ) )
	{
		console::error( "TextureManager::createViewportRenderTarget: Failed to create render target {}x{}", width, height );
		return nullptr;
	}

	// Get RTV handle
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_currentRtvIndex * m_rtvDescriptorSize;

	// Create render target view
	m_device->get()->CreateRenderTargetView( texture->getResource(), nullptr, rtvHandle );
	texture->m_rtvHandle = rtvHandle;

	// Get SRV handle and create shader resource view
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	srvCpuHandle.ptr += ( kSrvIndexOffset + m_currentSrvIndex ) * m_srvDescriptorSize;

	if ( !texture->createShaderResourceView( m_device, srvCpuHandle ) )
		return nullptr;

	// Store the SRV CPU handle for future updates during resize
	texture->m_srvCpuHandle = srvCpuHandle;

	// Calculate GPU handle for ImGui using ImGui's heap
	const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	const UINT64 offset = srvCpuHandle.ptr - cpuHandleStart.ptr;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	gpuHandle.ptr = gpuHandleStart.ptr + offset;

	// Store the GPU handle in the texture (need to add this to the texture class)
	texture->m_srvGpuHandle = gpuHandle;

	++m_currentRtvIndex;
	++m_currentSrvIndex;

	return texture;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::getNextSrvHandle()
{
	if ( !m_srvHeap || m_currentSrvIndex >= kMaxTextures )
		return {};

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += ( kSrvIndexOffset + m_currentSrvIndex ) * m_srvDescriptorSize;
	return handle;
}

} // namespace dx12

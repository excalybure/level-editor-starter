// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>
#include <comdef.h>

// ImGui integration
#include "imgui.h"
#include "imgui_impl_dx12.h"

module platform.dx12;

import std;

namespace dx12
{

// Device implementation
Device::Device() = default;

Device::~Device()
{
	shutdown();
}

bool Device::initialize( HWND window_handle )
{
	try
	{
		m_hwnd = window_handle;

#ifdef _DEBUG
		EnableDebugLayer();
#endif

		CreateFactory();
		FindAdapter();
		CreateDevice();
		CreateCommandObjects();
		CreateSwapChain( window_handle );
		CreateDescriptorHeaps();
		CreateRenderTargetViews();
		CreateSynchronizationObjects();

		return true;
	}
	catch ( const std::exception & )
	{
		return false;
	}
}

void Device::shutdown()
{
	// Wait for GPU to finish
	if ( m_commandQueue )
	{
		WaitForPreviousFrame();
	}

	// Close fence event
	if ( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
		m_fenceEvent = nullptr;
	}
}

void Device::endFrame()
{
	// Reset command allocator and list
	ThrowIfFailed( m_commandAllocator->Reset() );
	ThrowIfFailed( m_commandList->Reset( m_commandAllocator.Get(), nullptr ) );

	// Get current back buffer
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Transition to render target state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier( 1, &barrier );

	// Set render target
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
	m_commandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	// Clear render target
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	m_commandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

	// Set descriptor heaps for ImGui
	ID3D12DescriptorHeap *ppHeaps[] = { m_imguiDescriptorHeap.Get() };
	m_commandList->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
}

void Device::present()
{
	// Transition back to present state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier( 1, &barrier );

	// Execute command list
	ThrowIfFailed( m_commandList->Close() );
	ID3D12CommandList *ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );

	// Present
	ThrowIfFailed( m_swapChain->Present( 1, 0 ) );

	// Wait for frame to complete
	WaitForPreviousFrame();
}

void Device::EnableDebugLayer()
{
	// Enable the D3D12 debug layer
	if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &m_debugController ) ) ) )
	{
		m_debugController->EnableDebugLayer();
	}
}

void Device::CreateFactory()
{
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed( CreateDXGIFactory2( dxgiFactoryFlags, IID_PPV_ARGS( &m_factory ) ) );
}

void Device::FindAdapter()
{
	for ( UINT adapterIndex = 0; SUCCEEDED( m_factory->EnumAdapters1( adapterIndex, &m_adapter ) ); ++adapterIndex )
	{
		DXGI_ADAPTER_DESC1 desc;
		m_adapter->GetDesc1( &desc );

		// Skip software adapters
		if ( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
		{
			continue;
		}

		// Try to create device with this adapter
		if ( SUCCEEDED( D3D12CreateDevice( m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof( ID3D12Device ), nullptr ) ) )
		{
			break;
		}

		m_adapter.Reset();
	}

	if ( !m_adapter )
	{
		throw std::runtime_error( "No compatible D3D12 adapter found" );
	}
}

void Device::CreateDevice()
{
	ThrowIfFailed( D3D12CreateDevice(
		m_adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS( &m_device ) ) );
}

void Device::CreateCommandObjects()
{
	// Create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed( m_device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) );

	// Create command allocator
	ThrowIfFailed( m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS( &m_commandAllocator ) ) );

	// Create command list
	ThrowIfFailed( m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS( &m_commandList ) ) );

	// Close the command list initially
	ThrowIfFailed( m_commandList->Close() );
}

void Device::CreateSwapChain( HWND window_handle )
{
	// Get window dimensions
	RECT rect;
	GetClientRect( window_handle, &rect );
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed( m_factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		window_handle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain ) );

	// Disable Alt+Enter fullscreen toggle
	ThrowIfFailed( m_factory->MakeWindowAssociation( window_handle, DXGI_MWA_NO_ALT_ENTER ) );

	ThrowIfFailed( swapChain.As( &m_swapChain ) );
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Device::CreateDescriptorHeaps()
{
	// Create RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed( m_device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_rtvHeap ) ) );

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	// Create ImGui descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC imguiDesc = {};
	imguiDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiDesc.NumDescriptors = 1;
	imguiDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed( m_device->CreateDescriptorHeap( &imguiDesc, IID_PPV_ARGS( &m_imguiDescriptorHeap ) ) );
}

void Device::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// Create RTVs for each frame
	for ( UINT n = 0; n < FRAME_COUNT; n++ )
	{
		ThrowIfFailed( m_swapChain->GetBuffer( n, IID_PPV_ARGS( &m_renderTargets[n] ) ) );
		m_device->CreateRenderTargetView( m_renderTargets[n].Get(), nullptr, rtvHandle );
		rtvHandle.ptr += m_rtvDescriptorSize;
	}
}

void Device::CreateSynchronizationObjects()
{
	ThrowIfFailed( m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) );
	m_fenceValue = 1;

	// Create event handle for fence signaling
	m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( m_fenceEvent == nullptr )
	{
		ThrowIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
	}
}

void Device::WaitForPreviousFrame()
{
	// Signal and increment fence value
	const UINT64 fenceValueLocal = m_fenceValue;
	ThrowIfFailed( m_commandQueue->Signal( m_fence.Get(), fenceValueLocal ) );
	m_fenceValue++;

	// Wait until frame is finished
	if ( m_fence->GetCompletedValue() < fenceValueLocal )
	{
		ThrowIfFailed( m_fence->SetEventOnCompletion( fenceValueLocal, m_fenceEvent ) );
		WaitForSingleObject( m_fenceEvent, INFINITE );
	}
}

// CommandQueue implementation
CommandQueue::CommandQueue( Device &device, D3D12_COMMAND_LIST_TYPE type )
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed( device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) );
}

CommandQueue::~CommandQueue() = default;

void CommandQueue::ExecuteCommandLists( UINT numCommandLists, ID3D12CommandList *const *commandLists )
{
	m_commandQueue->ExecuteCommandLists( numCommandLists, commandLists );
}

void CommandQueue::Signal( ID3D12Fence *fence, UINT64 value )
{
	ThrowIfFailed( m_commandQueue->Signal( fence, value ) );
}

void CommandQueue::WaitForFence( ID3D12Fence *fence, UINT64 value )
{
	if ( fence->GetCompletedValue() < value )
	{
		HANDLE eventHandle = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		ThrowIfFailed( fence->SetEventOnCompletion( value, eventHandle ) );
		WaitForSingleObject( eventHandle, INFINITE );
		CloseHandle( eventHandle );
	}
}

// SwapChain implementation
SwapChain::SwapChain( Device &device, CommandQueue &commandQueue, HWND hwnd, UINT width, UINT height )
	: m_device( device ), m_commandQueue( commandQueue ), m_width( width ), m_height( height )
{
	// Create swap chain description
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed( device.GetFactory()->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain ) );

	// Disable Alt+Enter fullscreen transitions
	ThrowIfFailed( device.GetFactory()->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER ) );

	// Query for IDXGISwapChain3 interface
	ThrowIfFailed( swapChain.As( &m_swapChain ) );

	CreateBackBuffers();
}

SwapChain::~SwapChain() = default;

void SwapChain::CreateBackBuffers()
{
	for ( UINT i = 0; i < BufferCount; i++ )
	{
		ThrowIfFailed( m_swapChain->GetBuffer( i, IID_PPV_ARGS( &m_backBuffers[i] ) ) );
	}
}

void SwapChain::Present( UINT syncInterval )
{
	ThrowIfFailed( m_swapChain->Present( syncInterval, 0 ) );
}

UINT SwapChain::GetCurrentBackBufferIndex() const
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::Resize( UINT width, UINT height )
{
	if ( m_width == width && m_height == height )
		return;

	// Release back buffers
	for ( UINT i = 0; i < BufferCount; i++ )
	{
		m_backBuffers[i].Reset();
	}

	// Resize swap chain
	ThrowIfFailed( m_swapChain->ResizeBuffers( BufferCount, width, height, DXGI_FORMAT_UNKNOWN, 0 ) );

	m_width = width;
	m_height = height;

	CreateBackBuffers();
}

ID3D12Resource *SwapChain::GetCurrentBackBuffer() const
{
	return m_backBuffers[GetCurrentBackBufferIndex()].Get();
}

// CommandContext implementation
CommandContext::CommandContext( Device &device, D3D12_COMMAND_LIST_TYPE type )
	: m_device( device ), m_type( type )
{
	ThrowIfFailed( device->CreateCommandAllocator( type, IID_PPV_ARGS( &m_commandAllocator ) ) );
	ThrowIfFailed( device->CreateCommandList( 0, type, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS( &m_commandList ) ) );

	// Command lists are created in the recording state, close it for now
	ThrowIfFailed( m_commandList->Close() );
}

CommandContext::~CommandContext() = default;

void CommandContext::Reset()
{
	ThrowIfFailed( m_commandAllocator->Reset() );
	ThrowIfFailed( m_commandList->Reset( m_commandAllocator.Get(), nullptr ) );
}

void CommandContext::Close()
{
	ThrowIfFailed( m_commandList->Close() );
}

// Fence implementation
Fence::Fence( Device &device, UINT64 initialValue )
	: m_currentValue( initialValue )
{
	ThrowIfFailed( device->CreateFence( initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) );

	m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( !m_fenceEvent )
	{
		throw std::runtime_error( "Failed to create fence event" );
	}
}

Fence::~Fence()
{
	if ( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
	}
}

void Fence::Signal( CommandQueue &commandQueue )
{
	++m_currentValue;
	commandQueue.Signal( m_fence.Get(), m_currentValue );
}

void Fence::WaitForValue( UINT64 value )
{
	if ( m_fence->GetCompletedValue() < value )
	{
		ThrowIfFailed( m_fence->SetEventOnCompletion( value, m_fenceEvent ) );
		WaitForSingleObject( m_fenceEvent, INFINITE );
	}
}

void Fence::WaitForCurrentValue()
{
	WaitForValue( m_currentValue );
}

} // namespace dx12

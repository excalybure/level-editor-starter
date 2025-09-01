// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>
#include <comdef.h>
// ImGui headers (render integration moved here so we can issue draw calls on the active command list)

module platform.dx12;

import std;
import runtime.console;

namespace dx12
{

// Helper function to check HRESULT
void throwIfFailed( HRESULT hr, ID3D12Device *device )
{
	if ( FAILED( hr ) )
	{
		// Check if it's a device removed error and get more info
		if ( hr == DXGI_ERROR_DEVICE_REMOVED && device )
		{
			HRESULT removedReason = device->GetDeviceRemovedReason();
			console::fatal( "D3D12 DEVICE REMOVED! HRESULT: {:#x}, Removal Reason: {:#x}",
				static_cast<unsigned int>( hr ),
				static_cast<unsigned int>( removedReason ) );
		}
		else
		{
			console::fatal( "D3D12 operation failed with HRESULT: {:#x}",
				static_cast<unsigned int>( hr ) );
		}
	}
}

// Device implementation
Device::Device() = default;

Device::~Device()
{
	shutdown();
}

bool Device::initializeHeadless()
{
	try
	{
#ifdef _DEBUG
		// Guard against double initialization without prior shutdown
#endif
		if ( m_device )
		{
			return false; // already initialized
		}
#ifdef _DEBUG
		enableDebugLayer();
#endif
		createFactory();
		findAdapter();
		createDevice();
		createCommandObjects();
		createDescriptorHeaps(); // Add descriptor heaps for headless mode
		createSynchronizationObjects();

		// Initialize texture manager even in headless mode for viewport render targets
		if ( !m_textureManager.initialize( this ) )
		{
			return false;
		}

		return true;
	}
	catch ( const std::exception &e )
	{
		console::error( "Device::initializeHeadless: Failed because {}", e.what() );
		return false;
	}
}

bool Device::initialize( HWND window_handle )
{
	try
	{
		if ( m_device )
		{
			return false; // already initialized
		}
		m_hwnd = window_handle;

#ifdef _DEBUG
		enableDebugLayer();
#endif

		createFactory();
		findAdapter();
		createDevice();
		createCommandObjects();
		createSwapChain( window_handle );
		createDescriptorHeaps();
		createRenderTargetViews();
		createSynchronizationObjects();

		// Initialize texture manager for viewport render targets
		if ( !m_textureManager.initialize( this ) )
		{
			return false;
		}

		return true;
	}
	catch ( const std::exception &e )
	{
		console::error( "Device::initialize: Failed because {}", e.what() );
		return false;
	}
}

void Device::shutdown()
{
	// Shutdown texture manager first
	m_textureManager.shutdown();

	// Wait for GPU to finish
	if ( m_commandQueue )
	{
		waitForPreviousFrame();
	}

	// Close fence event
	if ( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
		m_fenceEvent = nullptr;
	}

	// Release COM resources explicitly (safe if already null)
	for ( UINT i = 0; i < kFrameCount; ++i )
	{
		m_renderTargets[i].Reset();
	}
	m_swapChain.Reset();
	m_rtvHeap.Reset();
	m_imguiDescriptorHeap.Reset();
	m_commandList.Reset();
	m_commandAllocator.Reset();
	m_commandQueue.Reset();
	m_fence.Reset();
	m_device.Reset();
	m_adapter.Reset();
	m_factory.Reset();
	m_debugController.Reset();
	m_frameIndex = 0;
	m_hwnd = nullptr;
}

void Device::beginFrame()
{
	// In headless mode (no swap chain/RTVs) or if not initialized, this is a no-op
	if ( !m_device || !m_commandAllocator || !m_commandList || !m_swapChain )
	{
		return;
	}
	// Reset command allocator and list
	throwIfFailed( m_commandAllocator->Reset() );
	throwIfFailed( m_commandList->Reset( m_commandAllocator.Get(), nullptr ) );

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

void Device::endFrame()
{
	if ( !m_device || !m_commandList || !m_swapChain )
	{
		return; // headless/uninitialized no-op
	}

	// Transition back to present state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier( 1, &barrier );

	// Close command list
	throwIfFailed( m_commandList->Close() );

	// Execute command list
	ID3D12CommandList *ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );
}

void Device::present()
{
	if ( !m_device || !m_commandList || !m_swapChain )
	{
		return; // headless/uninitialized no-op
	}
	// Command list already transitioned to PRESENT, closed and executed in endFrame().
	// Simply present the swap chain.
	throwIfFailed( m_swapChain->Present( 1, 0 ), m_device.Get() );

	// Wait for frame to complete
	waitForPreviousFrame();
}

void Device::enableDebugLayer()
{
	// Enable the D3D12 debug layer
	if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &m_debugController ) ) ) )
	{
		m_debugController->EnableDebugLayer();
	}
}

void Device::createFactory()
{
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	throwIfFailed( CreateDXGIFactory2( dxgiFactoryFlags, IID_PPV_ARGS( &m_factory ) ) );
}

void Device::findAdapter()
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
		console::errorAndThrow( "No compatible D3D12 adapter found" );
	}
}

void Device::createDevice()
{
	throwIfFailed( D3D12CreateDevice(
		m_adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS( &m_device ) ) );
}

void Device::createCommandObjects()
{
	// Create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	throwIfFailed( m_device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) );

	// Create command allocator
	throwIfFailed( m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS( &m_commandAllocator ) ) );

	// Create command list
	throwIfFailed( m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS( &m_commandList ) ) );

	// Close the command list initially
	throwIfFailed( m_commandList->Close() );
}

void Device::createSwapChain( HWND window_handle )
{
	// Get window dimensions
	RECT rect;
	GetClientRect( window_handle, &rect );
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = kFrameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	throwIfFailed( m_factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		window_handle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain ) );

	// Disable Alt+Enter fullscreen toggle
	throwIfFailed( m_factory->MakeWindowAssociation( window_handle, DXGI_MWA_NO_ALT_ENTER ) );

	throwIfFailed( swapChain.As( &m_swapChain ) );
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Device::createDescriptorHeaps()
{
	// Create RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = kFrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	throwIfFailed( m_device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_rtvHeap ) ) );

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	// Create ImGui descriptor heap - reserve indices 16+ for viewport textures
	// ImGui uses index 0 for font texture, we reserve 16-79 for 64 viewport textures
	D3D12_DESCRIPTOR_HEAP_DESC imguiDesc = {};
	imguiDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiDesc.NumDescriptors = 80; // 16 reserved for ImGui + 64 for viewport textures
	imguiDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	throwIfFailed( m_device->CreateDescriptorHeap( &imguiDesc, IID_PPV_ARGS( &m_imguiDescriptorHeap ) ) );
}

void Device::createRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// Create RTVs for each frame
	for ( UINT n = 0; n < kFrameCount; n++ )
	{
		throwIfFailed( m_swapChain->GetBuffer( n, IID_PPV_ARGS( &m_renderTargets[n] ) ) );
		m_device->CreateRenderTargetView( m_renderTargets[n].Get(), nullptr, rtvHandle );
		rtvHandle.ptr += m_rtvDescriptorSize;
	}
}

void Device::createSynchronizationObjects()
{
	throwIfFailed( m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) );
	m_fenceValue = 1;

	// Create event handle for fence signaling
	m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( m_fenceEvent == nullptr )
	{
		throwIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
	}
}

void Device::waitForPreviousFrame()
{
	// Signal and increment fence value
	const UINT64 fenceValueLocal = m_fenceValue;
	throwIfFailed( m_commandQueue->Signal( m_fence.Get(), fenceValueLocal ) );
	m_fenceValue++;

	// Wait until frame is finished
	if ( m_fence->GetCompletedValue() < fenceValueLocal )
	{
		throwIfFailed( m_fence->SetEventOnCompletion( fenceValueLocal, m_fenceEvent ) );
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

	throwIfFailed( device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_commandQueue ) ) );
}

CommandQueue::~CommandQueue() = default;

void CommandQueue::executeCommandLists( UINT numCommandLists, ID3D12CommandList *const *commandLists )
{
	m_commandQueue->ExecuteCommandLists( numCommandLists, commandLists );
}

void CommandQueue::signal( ID3D12Fence *fence, UINT64 value )
{
	throwIfFailed( m_commandQueue->Signal( fence, value ) );
}

void CommandQueue::waitForFence( ID3D12Fence *fence, UINT64 value )
{
	if ( fence->GetCompletedValue() < value )
	{
		HANDLE eventHandle = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		throwIfFailed( fence->SetEventOnCompletion( value, eventHandle ) );
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
	swapChainDesc.BufferCount = kBufferCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	throwIfFailed( device.getFactory()->CreateSwapChainForHwnd(
		commandQueue.get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain ) );

	// Disable Alt+Enter fullscreen transitions
	throwIfFailed( device.getFactory()->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER ) );

	// Query for IDXGISwapChain3 interface
	throwIfFailed( swapChain.As( &m_swapChain ) );

	createBackBuffers();
}

SwapChain::~SwapChain() = default;

void SwapChain::createBackBuffers()
{
	for ( UINT i = 0; i < kBufferCount; i++ )
	{
		throwIfFailed( m_swapChain->GetBuffer( i, IID_PPV_ARGS( &m_backBuffers[i] ) ) );
	}
}

void SwapChain::present( UINT syncInterval )
{
	throwIfFailed( m_swapChain->Present( syncInterval, 0 ) );
}

UINT SwapChain::getCurrentBackBufferIndex() const
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::resize( UINT width, UINT height )
{
	if ( m_width == width && m_height == height )
		return;

	// Release back buffers
	for ( UINT i = 0; i < kBufferCount; i++ )
	{
		m_backBuffers[i].Reset();
	}

	// Resize swap chain
	throwIfFailed( m_swapChain->ResizeBuffers( kBufferCount, width, height, DXGI_FORMAT_UNKNOWN, 0 ) );

	m_width = width;
	m_height = height;

	createBackBuffers();
}

ID3D12Resource *SwapChain::getCurrentBackBuffer() const
{
	return m_backBuffers[getCurrentBackBufferIndex()].Get();
}

// CommandContext implementation
CommandContext::CommandContext( Device &device, D3D12_COMMAND_LIST_TYPE type )
	: m_device( device ), m_type( type )
{
	throwIfFailed( device->CreateCommandAllocator( type, IID_PPV_ARGS( &m_commandAllocator ) ) );
	throwIfFailed( device->CreateCommandList( 0, type, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS( &m_commandList ) ) );

	// Command lists are created in the recording state, close it for now
	throwIfFailed( m_commandList->Close() );
}

CommandContext::~CommandContext() = default;

void CommandContext::reset()
{
	throwIfFailed( m_commandAllocator->Reset() );
	throwIfFailed( m_commandList->Reset( m_commandAllocator.Get(), nullptr ) );
}

void CommandContext::close()
{
	throwIfFailed( m_commandList->Close() );
}

// Fence implementation
Fence::Fence( Device &device, UINT64 initialValue )
	: m_currentValue( initialValue )
{
	throwIfFailed( device->CreateFence( initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_fence ) ) );

	m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( !m_fenceEvent )
	{
		console::errorAndThrow( "Failed to create fence event" );
	}
}

Fence::~Fence()
{
	if ( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
	}
}

void Fence::signal( CommandQueue &commandQueue )
{
	++m_currentValue;
	commandQueue.signal( m_fence.Get(), m_currentValue );
}

void Fence::waitForValue( UINT64 value )
{
	if ( m_fence->GetCompletedValue() < value )
	{
		throwIfFailed( m_fence->SetEventOnCompletion( value, m_fenceEvent ) );
		WaitForSingleObject( m_fenceEvent, INFINITE );
	}
}

void Fence::waitForCurrentValue()
{
	waitForValue( m_currentValue );
}

} // namespace dx12

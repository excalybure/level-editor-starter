#include "dx12_device.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>
#ifdef _DEBUG
#include <d3d12sdklayers.h> // For ID3D12InfoQueue
#endif
#include "core/console.h"

#ifdef _DEBUG
// Debug message callback for real-time message processing
static void CALLBACK DebugMessageCallback(
	D3D12_MESSAGE_CATEGORY Category,
	D3D12_MESSAGE_SEVERITY Severity,
	D3D12_MESSAGE_ID ID,
	LPCSTR pDescription,
	void *pContext )
{
	// Format and route to console based on severity
	const std::string message = std::format( "[D3D12] {}", pDescription );

	switch ( Severity )
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
	case D3D12_MESSAGE_SEVERITY_ERROR:
		console::error( message );
		break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		console::warning( message );
		break;
	case D3D12_MESSAGE_SEVERITY_INFO:
		console::info( message );
		break;
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		console::debug( message );
		break;
	}
}
#endif

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
			const HRESULT removedReason = device->GetDeviceRemovedReason();
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

bool Device::initialize( HWND windowHandle )
{
	try
	{
		if ( m_device )
		{
			return false; // already initialized
		}
		m_hwnd = windowHandle;

#ifdef _DEBUG
		enableDebugLayer();
#endif

		createFactory();
		findAdapter();
		createDevice();
		createCommandObjects();
		createDescriptorHeaps();
		createSwapChain( windowHandle );
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

#ifdef _DEBUG
	// Cleanup debug message callback
	cleanupDebugMessageCallback();
#endif

	// Close fence event
	if ( m_fenceEvent )
	{
		CloseHandle( m_fenceEvent );
		m_fenceEvent = nullptr;
	}

	// Release wrappers first (they hold references to Device resources)
	m_swapChain.reset();
	m_commandQueue.reset();
	m_commandContext.reset();

	// Release COM resources explicitly (safe if already null)
	m_rtvHeap.Reset();
	m_imguiDescriptorHeap.Reset();
	m_fence.Reset();
	m_device.Reset();
	m_adapter.Reset();
	m_factory.Reset();
	m_debugController.Reset();
	m_hwnd = nullptr;
}

void Device::beginFrame()
{
	// Validate that beginFrame hasn't already been called
	if ( m_inFrame )
	{
		console::error( "Device::beginFrame called when already in frame. Call endFrame() first." );
		return;
	}

	// Always reset command context if available
	if ( !m_device || !m_commandContext )
	{
		return;
	}

	// Mark that we're now in a frame
	m_inFrame = true;

	// Reset command context for new frame
	m_commandContext->reset();

	// In headless mode (no swap chain), we only need command context reset
	if ( !m_swapChain )
	{
		return;
	}

	// Windowed mode: full frame setup with swap chain
	// Get current back buffer from SwapChain wrapper
	const UINT frameIndex = m_swapChain->getCurrentBackBufferIndex();
	ID3D12Resource *const currentBackBuffer = m_swapChain->getCurrentBackBuffer();

	// Transition to render target state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentBackBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandContext->get()->ResourceBarrier( 1, &barrier );

	// Set render target and depth stencil view
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * m_rtvDescriptorSize;
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap ? m_dsvHeap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandContext->get()->OMSetRenderTargets( 1, &rtvHandle, FALSE, m_dsvHeap ? &dsvHandle : nullptr );

	// Clear render target
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	m_commandContext->get()->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

	// Clear depth buffer if available
	if ( m_dsvHeap && m_depthBuffer )
	{
		m_commandContext->get()->ClearDepthStencilView( dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );
	}

	// Set descriptor heaps for ImGui
	ID3D12DescriptorHeap *const ppHeaps[] = { m_imguiDescriptorHeap.Get() };
	m_commandContext->get()->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
}

void Device::endFrame()
{
	// Validate that we're actually in a frame
	if ( !m_inFrame )
	{
		console::error( "Device::endFrame called when not in frame. Call beginFrame() first." );
		return;
	}

	// Always close command context if available
	if ( !m_device || !m_commandContext )
	{
		console::error( "Device::endFrame called but device or command context not available." );
		m_inFrame = false;
		return;
	}

	// For headless mode, just close the command context and execute
	if ( !m_swapChain )
	{
		m_commandContext->close();
		ID3D12CommandList *ppCommandLists[] = { m_commandContext->get() };
		m_commandQueue->executeCommandLists( _countof( ppCommandLists ), ppCommandLists );
		m_inFrame = false;
		return;
	}

	// Windowed mode: transition back buffer to present state
	ID3D12Resource *const currentBackBuffer = m_swapChain->getCurrentBackBuffer();

	// Transition back to present state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentBackBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandContext->get()->ResourceBarrier( 1, &barrier );

	// Close command list
	m_commandContext->close();

	// Execute command list
	ID3D12CommandList *const ppCommandLists[] = { m_commandContext->get() };
	m_commandQueue->executeCommandLists( _countof( ppCommandLists ), ppCommandLists );

	// Mark that we're no longer in a frame
	m_inFrame = false;
}

void Device::clear( const math::Color &clearColor ) noexcept
{
	// Validate we're in a frame
	if ( !m_inFrame || !m_commandContext )
	{
		return; // Not in frame or no command context - safe no-op
	}

	// In headless mode, there's no render target to clear
	if ( !m_swapChain || !m_rtvHeap )
	{
		return; // Headless mode - no render targets
	}

	// Get current back buffer RTV handle
	const UINT frameIndex = m_swapChain->getCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * m_rtvDescriptorSize;

	// Clear render target with specified color
	const float color[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
	m_commandContext->get()->ClearRenderTargetView( rtvHandle, color, 0, nullptr );
}

void Device::clearDepth( float depth ) noexcept
{
	// Validate we're in a frame
	if ( !m_inFrame || !m_commandContext )
	{
		return; // Not in frame or no command context - safe no-op
	}

	// Check if depth buffer is available
	if ( !m_swapChain || !m_dsvHeap || !m_depthBuffer )
	{
		return; // No depth buffer available - safe no-op
	}

	// Get depth stencil view handle
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear depth buffer with specified depth value
	m_commandContext->get()->ClearDepthStencilView( dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr );
}

void Device::present()
{
	if ( !m_device || !m_commandContext || !m_swapChain )
	{
		return; // headless/uninitialized no-op
	}
	// Command list already transitioned to PRESENT, closed and executed in endFrame().
	// Simply present the swap chain using the wrapper.
	m_swapChain->present( 1 );

	// Wait for frame to complete
	waitForPreviousFrame();
}

void Device::setBackbufferRenderTarget()
{
	if ( !m_device || !m_commandContext || !m_swapChain )
	{
		return; // headless/uninitialized no-op
	}

	// Set backbuffer render target for ImGui rendering
	const UINT frameIndex = m_swapChain->getCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * m_rtvDescriptorSize;
	m_commandContext->get()->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	// Set descriptor heap for ImGui (should already be set, but ensure consistency)
	ID3D12DescriptorHeap *const ppHeaps[] = { m_imguiDescriptorHeap.Get() };
	m_commandContext->get()->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
}

ID3D12GraphicsCommandList *Device::getCommandList() const
{
	return m_commandContext ? m_commandContext->get() : nullptr;
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

#ifdef _DEBUG
	// Configure debug layer to break on errors and warnings
	configureDebugBreaks();
	// Setup real-time debug message callback
	setupDebugMessageCallback();
#endif
}

void Device::configureDebugBreaks()
{
#ifdef _DEBUG
	// Try to get ID3D12InfoQueue1 first (Windows 10 1809+), fallback to ID3D12InfoQueue
	if ( SUCCEEDED( m_device->QueryInterface( IID_PPV_ARGS( m_infoQueue.GetAddressOf() ) ) ) )
	{
		// Break on D3D12 errors
		m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );

		// Break on D3D12 warnings (optional - comment out if too noisy)
		m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );

		// Optionally break on info messages (usually too noisy)
		// m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, TRUE);

		console::info( "D3D12 debug layer configured with console output integration" );
	}
#endif
}

void Device::setupDebugMessageCallback()
{
#ifdef _DEBUG
	// m_infoQueue was already queried in configureDebugBreaks()
	if ( m_infoQueue )
	{
		// Register callback for real-time message processing
		const HRESULT hr = m_infoQueue->RegisterMessageCallback(
			DebugMessageCallback,
			D3D12_MESSAGE_CALLBACK_FLAG_NONE,
			this,
			&m_callbackCookie );

		if ( FAILED( hr ) )
		{
			console::warning( "Failed to register D3D12 debug message callback" );
		}
	}
	else
	{
		console::warning( "ID3D12InfoQueue1 not available, debug messages will not be captured" );
	}
#endif
}

void Device::cleanupDebugMessageCallback()
{
#ifdef _DEBUG
	if ( m_infoQueue && m_callbackCookie != 0 )
	{
		m_infoQueue->UnregisterMessageCallback( m_callbackCookie );
		m_callbackCookie = 0;
	}
	m_infoQueue.Reset();
#endif
}

void Device::createCommandObjects()
{
	// Create command queue wrapper
	m_commandQueue = std::make_unique<CommandQueue>( *this );

	// Create command context wrapper
	m_commandContext = std::make_unique<CommandContext>( *this );
}

void Device::createSwapChain( HWND windowHandle )
{
	// Get window dimensions
	RECT rect;
	GetClientRect( windowHandle, &rect );
	const UINT width = rect.right - rect.left;
	const UINT height = rect.bottom - rect.top;

	// Create swap chain wrapper using the existing command queue wrapper
	m_swapChain = std::make_unique<SwapChain>( *this, *m_commandQueue, windowHandle, width, height );

	// Create depth buffer to match swap chain size
	createDepthBuffer( width, height );

	// Create render target views for the swap chain buffers
	createRenderTargetViews();
}

void Device::createDescriptorHeaps()
{
	// Create RTV descriptor heap for swap chain buffers (2 buffers for double buffering)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2; // SwapChain uses 2 buffers
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	throwIfFailed( m_device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_rtvHeap ) ) );

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	// Create DSV descriptor heap for depth buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	throwIfFailed( m_device->CreateDescriptorHeap( &dsvHeapDesc, IID_PPV_ARGS( &m_dsvHeap ) ) );

	m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_DSV );

	// Create ImGui descriptor heap - reserve indices 16+ for viewport textures
	// ImGui uses index 0 for font texture, we reserve 16-79 for 64 viewport textures
	D3D12_DESCRIPTOR_HEAP_DESC imguiDesc = {};
	imguiDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imguiDesc.NumDescriptors = 80; // 16 reserved for ImGui + 64 for viewport textures
	imguiDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	throwIfFailed( m_device->CreateDescriptorHeap( &imguiDesc, IID_PPV_ARGS( &m_imguiDescriptorHeap ) ) );
}

void Device::createDepthBuffer( UINT width, UINT height )
{
	// Create depth buffer resource
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;

	throwIfFailed( m_device->CreateCommittedResource(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS( &m_depthBuffer ) ) );

	// Create depth stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView( m_depthBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart() );
}

void Device::createRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// Create RTVs for each frame using SwapChain wrapper (2 buffers)
	for ( UINT n = 0; n < 2; n++ ) // SwapChain uses 2 buffers
	{
		// Get buffer from swap chain using the native interface
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		throwIfFailed( m_swapChain->get()->GetBuffer( n, IID_PPV_ARGS( &backBuffer ) ) );
		m_device->CreateRenderTargetView( backBuffer.Get(), nullptr, rtvHandle );
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
	m_commandQueue->signal( m_fence.Get(), fenceValueLocal );
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

void Device::resize( UINT width, UINT height )
{
	if ( !m_swapChain )
	{
		// No swap chain to resize (headless mode)
		return;
	}

	// Wait for all GPU work to complete before resizing resources
	waitForPreviousFrame();

	// Release current depth buffer and render target views
	m_depthBuffer.Reset();

	// Resize the swap chain (this recreates the back buffers)
	m_swapChain->resize( width, height );

	// Recreate depth buffer with new dimensions
	createDepthBuffer( width, height );

	// Recreate render target views for the new swap chain buffers
	createRenderTargetViews();
}

} // namespace dx12

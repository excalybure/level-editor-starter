// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>
#include <cstdio>
#include <stdexcept>

export module platform.dx12;

import std;

// Link required libraries
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

export namespace dx12
{

// Forward declarations
class Device;
class CommandQueue;
class SwapChain;
class Texture;
class TextureManager;

// Helper function to check HRESULT
void throwIfFailed( HRESULT hr, ID3D12Device *device = nullptr );

// D3D12 Texture class for viewport render targets
export class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	// No copy/move for now
	Texture( const Texture & ) = delete;
	Texture &operator=( const Texture & ) = delete;

	// Create a render target texture
	bool createRenderTarget(
		Device *device,
		UINT width,
		UINT height,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM );

	// Create shader resource view for ImGui integration
	bool createShaderResourceView( Device *device, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle );

	// Resize the texture (recreates the resource)
	bool resize( Device *device, UINT width, UINT height );

	// Clear the render target texture with a solid color
	bool clearRenderTarget( Device *device, const float clearColor[4] );

	// Resource access
	ID3D12Resource *getResource() const { return m_resource.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE getRtvHandle() const { return m_rtvHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE getSrvGpuHandle() const { return m_srvGpuHandle; }
	void *getImGuiTextureId() const { return (void *)m_srvGpuHandle.ptr; }

	// Properties
	UINT getWidth() const { return m_width; }
	UINT getHeight() const { return m_height; }
	DXGI_FORMAT getFormat() const { return m_format; }

	// Resource state management
	void transitionTo( ID3D12GraphicsCommandList *commandList, D3D12_RESOURCE_STATES newState );

	// Allow TextureManager to access private members for GPU handle management
	friend class TextureManager;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle = {};

	UINT m_width = 0;
	UINT m_height = 0;
	DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

	Device *m_device = nullptr;
};

// Texture manager for viewport render targets
export class TextureManager
{
public:
	TextureManager() = default;
	~TextureManager() = default;

	// Initialize with device and descriptor heaps
	bool initialize( Device *device );
	void shutdown();

	// Create a new viewport render target
	std::shared_ptr<Texture> createViewportRenderTarget( UINT width, UINT height );

	// Get next available SRV descriptor handle
	D3D12_CPU_DESCRIPTOR_HANDLE getNextSrvHandle();

	// Constants
	static const UINT kMaxTextures = 64;	// Support up to 64 viewport render targets
	static const UINT kSrvIndexOffset = 16; // Start our textures at index 16 to avoid ImGui conflicts

private:
	Device *m_device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	UINT m_rtvDescriptorSize = 0;
	UINT m_srvDescriptorSize = 0;
	UINT m_currentRtvIndex = 0;
	UINT m_currentSrvIndex = 0;
};

// D3D12 Device wrapper
export class Device
{
public:
	Device();
	~Device();

	// No copy/move for now
	Device( const Device & ) = delete;
	Device &operator=( const Device & ) = delete;

	// Initialize the device with window handle for swap chain and ImGui
	bool initialize( HWND window_handle );
	// Headless initialization (no window / swap chain) for tests or compute-only scenarios
	bool initializeHeadless();
	void shutdown();

	// Frame operations
	void beginFrame();
	void endFrame();
	void present();

	// Render target management - restore backbuffer for ImGui after viewport rendering
	void setBackbufferRenderTarget();

	ID3D12Device *get() const { return m_device.Get(); }
	ID3D12Device *operator->() const { return m_device.Get(); }

	// ImGui integration
	ID3D12Device *getDevice() const { return m_device.Get(); }
	ID3D12DescriptorHeap *getImguiDescriptorHeap() const { return m_imguiDescriptorHeap.Get(); }

	// Command list for ImGui rendering
	ID3D12GraphicsCommandList *getCommandList() const { return m_commandList.Get(); }

	// Factory for creating other D3D12 objects
	IDXGIFactory4 *getFactory() const { return m_factory.Get(); }

	// Texture management for viewport render targets
	TextureManager *getTextureManager() { return &m_textureManager; }

private:
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;

	// Command objects
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

	// Swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	static const UINT kFrameCount = 3;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kFrameCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize = 0;
	UINT m_frameIndex = 0;

	// ImGui descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiDescriptorHeap;

	// Synchronization
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	// Window handle
	HWND m_hwnd = nullptr;

	// Texture manager for viewport render targets
	TextureManager m_textureManager;

	// Initialization methods
	void enableDebugLayer();
	void createFactory();
	void findAdapter();
	void createDevice();
	void configureDebugBreaks();
	void createCommandObjects();
	void createSwapChain( HWND window_handle );
	void createDescriptorHeaps();
	void createRenderTargetViews();
	void createSynchronizationObjects();

	// Frame methods
	void waitForPreviousFrame();
};

// Command Queue wrapper
export class CommandQueue
{
public:
	explicit CommandQueue( Device &device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT );
	~CommandQueue();

	// No copy/move for now
	CommandQueue( const CommandQueue & ) = delete;
	CommandQueue &operator=( const CommandQueue & ) = delete;

	ID3D12CommandQueue *get() const { return m_commandQueue.Get(); }
	ID3D12CommandQueue *operator->() const { return m_commandQueue.Get(); }

	// Execute command lists
	void executeCommandLists( UINT numCommandLists, ID3D12CommandList *const *commandLists );

	// Signal and wait for fence
	void signal( ID3D12Fence *fence, UINT64 value );
	void waitForFence( ID3D12Fence *fence, UINT64 value );

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
};

// Swap Chain wrapper
export class SwapChain
{
public:
	SwapChain( Device &device, CommandQueue &commandQueue, HWND hwnd, UINT width, UINT height );
	~SwapChain();

	// No copy/move for now
	SwapChain( const SwapChain & ) = delete;
	SwapChain &operator=( const SwapChain & ) = delete;

	IDXGISwapChain3 *get() const { return m_swapChain.Get(); }
	IDXGISwapChain3 *operator->() const { return m_swapChain.Get(); }

	// Present the current back buffer
	void present( UINT syncInterval = 1 );

	// Get current back buffer index
	UINT getCurrentBackBufferIndex() const;

	// Resize the swap chain
	void resize( UINT width, UINT height );

	// Get back buffer render target view
	ID3D12Resource *getCurrentBackBuffer() const;

	static constexpr UINT kBufferCount = 2;

private:
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[kBufferCount];

	Device &m_device;
	CommandQueue &m_commandQueue;
	UINT m_width, m_height;

	void createBackBuffers();
};

// Basic command allocator and list management
export class CommandContext
{
public:
	CommandContext( Device &device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT );
	~CommandContext();

	// No copy/move for now
	CommandContext( const CommandContext & ) = delete;
	CommandContext &operator=( const CommandContext & ) = delete;

	ID3D12GraphicsCommandList *get() const { return m_commandList.Get(); }
	ID3D12GraphicsCommandList *operator->() const { return m_commandList.Get(); }

	// Reset for new frame
	void reset();

	// Close command list for execution
	void close();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Device &m_device;
	D3D12_COMMAND_LIST_TYPE m_type;
};

// Simple fence wrapper for synchronization
export class Fence
{
public:
	explicit Fence( Device &device, UINT64 initialValue = 0 );
	~Fence();

	// No copy/move for now
	Fence( const Fence & ) = delete;
	Fence &operator=( const Fence & ) = delete;

	ID3D12Fence *get() const { return m_fence.Get(); }
	ID3D12Fence *operator->() const { return m_fence.Get(); }

	// Get current fence value
	UINT64 getCurrentValue() const { return m_currentValue; }

	// Signal and wait
	void signal( CommandQueue &commandQueue );
	void waitForValue( UINT64 value );
	void waitForCurrentValue();

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_currentValue;
	HANDLE m_fenceEvent;
};

} // namespace dx12

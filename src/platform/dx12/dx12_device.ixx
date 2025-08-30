// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>

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

// Helper function to check HRESULT
inline void throwIfFailed( HRESULT hr )
{
	if ( FAILED( hr ) )
	{
		throw std::runtime_error( "D3D12 operation failed" );
	}
}

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

	ID3D12Device *get() const { return m_device.Get(); }
	ID3D12Device *operator->() const { return m_device.Get(); }

	// ImGui integration
	ID3D12Device *getDevice() const { return m_device.Get(); }
	ID3D12DescriptorHeap *getImguiDescriptorHeap() const { return m_imguiDescriptorHeap.Get(); }

	// Command list for ImGui rendering
	ID3D12GraphicsCommandList *getCommandList() const { return m_commandList.Get(); }

	// Factory for creating other D3D12 objects
	IDXGIFactory4 *getFactory() const { return m_factory.Get(); }

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

	// Initialization methods
	void enableDebugLayer();
	void createFactory();
	void findAdapter();
	void createDevice();
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

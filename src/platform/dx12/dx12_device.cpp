// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>
#include <comdef.h>

module platform.dx12;

import std;

namespace dx12
{

// Device implementation
Device::Device()
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif
	
	CreateFactory();
	FindAdapter();
	CreateDevice();
}

Device::~Device() = default;

void Device::EnableDebugLayer()
{
	// Enable the D3D12 debug layer
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController))))
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

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));
}

void Device::FindAdapter()
{
	for (UINT adapterIndex = 0; SUCCEEDED(m_factory->EnumAdapters1(adapterIndex, &m_adapter)); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		m_adapter->GetDesc1(&desc);

		// Skip software adapters
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		// Try to create device with this adapter
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		
		m_adapter.Reset();
	}

	if (!m_adapter)
	{
		throw std::runtime_error("No compatible D3D12 adapter found");
	}
}

void Device::CreateDevice()
{
	ThrowIfFailed(D3D12CreateDevice(
		m_adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));
}

// CommandQueue implementation
CommandQueue::CommandQueue(Device& device, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
}

CommandQueue::~CommandQueue() = default;

void CommandQueue::ExecuteCommandLists(UINT numCommandLists, ID3D12CommandList* const* commandLists)
{
	m_commandQueue->ExecuteCommandLists(numCommandLists, commandLists);
}

void CommandQueue::Signal(ID3D12Fence* fence, UINT64 value)
{
	ThrowIfFailed(m_commandQueue->Signal(fence, value));
}

void CommandQueue::WaitForFence(ID3D12Fence* fence, UINT64 value)
{
	if (fence->GetCompletedValue() < value)
	{
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ThrowIfFailed(fence->SetEventOnCompletion(value, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

// SwapChain implementation
SwapChain::SwapChain(Device& device, CommandQueue& commandQueue, HWND hwnd, UINT width, UINT height)
	: m_device(device), m_commandQueue(commandQueue), m_width(width), m_height(height)
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
	ThrowIfFailed(device.GetFactory()->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// Disable Alt+Enter fullscreen transitions
	ThrowIfFailed(device.GetFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	// Query for IDXGISwapChain3 interface
	ThrowIfFailed(swapChain.As(&m_swapChain));

	CreateBackBuffers();
}

SwapChain::~SwapChain() = default;

void SwapChain::CreateBackBuffers()
{
	for (UINT i = 0; i < BufferCount; i++)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));
	}
}

void SwapChain::Present(UINT syncInterval)
{
	ThrowIfFailed(m_swapChain->Present(syncInterval, 0));
}

UINT SwapChain::GetCurrentBackBufferIndex() const
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::Resize(UINT width, UINT height)
{
	if (m_width == width && m_height == height)
		return;

	// Release back buffers
	for (UINT i = 0; i < BufferCount; i++)
	{
		m_backBuffers[i].Reset();
	}

	// Resize swap chain
	ThrowIfFailed(m_swapChain->ResizeBuffers(BufferCount, width, height, DXGI_FORMAT_UNKNOWN, 0));

	m_width = width;
	m_height = height;

	CreateBackBuffers();
}

ID3D12Resource* SwapChain::GetCurrentBackBuffer() const
{
	return m_backBuffers[GetCurrentBackBufferIndex()].Get();
}

// CommandContext implementation
CommandContext::CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE type)
	: m_device(device), m_type(type)
{
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_commandAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, type, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	
	// Command lists are created in the recording state, close it for now
	ThrowIfFailed(m_commandList->Close());
}

CommandContext::~CommandContext() = default;

void CommandContext::Reset()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}

void CommandContext::Close()
{
	ThrowIfFailed(m_commandList->Close());
}

// Fence implementation
Fence::Fence(Device& device, UINT64 initialValue)
	: m_currentValue(initialValue)
{
	ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
	{
		throw std::runtime_error("Failed to create fence event");
	}
}

Fence::~Fence()
{
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
	}
}

void Fence::Signal(CommandQueue& commandQueue)
{
	++m_currentValue;
	commandQueue.Signal(m_fence.Get(), m_currentValue);
}

void Fence::WaitForValue(UINT64 value)
{
	if (m_fence->GetCompletedValue() < value)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(value, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void Fence::WaitForCurrentValue()
{
	WaitForValue(m_currentValue);
}

} // namespace dx12

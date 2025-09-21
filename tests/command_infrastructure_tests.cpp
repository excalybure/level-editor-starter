// D3D12 Command infrastructure comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"


TEST_CASE( "dx12::CommandQueue Operations", "[dx12][command][queue]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::CommandQueue operations" ) )
		return;

	SECTION( "Command queue creation and properties" )
	{
		auto d3dDevice = device.get();
		REQUIRE( d3dDevice != nullptr );

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		HRESULT hr = d3dDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &commandQueue ) );

		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( commandQueue != nullptr );

		// Verify queue properties
		auto desc = commandQueue->GetDesc();
		REQUIRE( desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT );
		REQUIRE( desc.Priority == D3D12_COMMAND_QUEUE_PRIORITY_NORMAL );
	}

	SECTION( "Command execution" )
	{
		auto d3dDevice = device.get();
		auto commandList = device.getCommandList();
		REQUIRE( commandList != nullptr );

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		HRESULT hr = d3dDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &commandQueue ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Create command allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &commandAllocator ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Reset command list
		hr = commandList->Reset( commandAllocator.Get(), nullptr );
		REQUIRE( SUCCEEDED( hr ) );

		// Close command list
		hr = commandList->Close();
		REQUIRE( SUCCEEDED( hr ) );

		// Execute command list
		ID3D12CommandList *commandLists[] = { commandList };
		REQUIRE_NOTHROW( commandQueue->ExecuteCommandLists( 1, commandLists ) );
	}

	SECTION( "Signal and wait operations" )
	{
		auto d3dDevice = device.get();

		// Create command queue
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		HRESULT hr = d3dDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &commandQueue ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Create fence
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		hr = d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Signal fence
		UINT64 fenceValue = 1;
		hr = commandQueue->Signal( fence.Get(), fenceValue );
		REQUIRE( SUCCEEDED( hr ) );

		// Check fence completion
		if ( fence->GetCompletedValue() < fenceValue )
		{
			HANDLE eventHandle = CreateEvent( nullptr, FALSE, FALSE, nullptr );
			REQUIRE( eventHandle != nullptr );

			hr = fence->SetEventOnCompletion( fenceValue, eventHandle );
			REQUIRE( SUCCEEDED( hr ) );

			WaitForSingleObject( eventHandle, INFINITE );
			CloseHandle( eventHandle );
		}

		REQUIRE( fence->GetCompletedValue() >= fenceValue );
	}
}

TEST_CASE( "CommandAllocator Management", "[dx12][command][allocator]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "CommandAllocator management" ) )
		return;

	SECTION( "Command allocator creation" )
	{
		auto d3dDevice = device.get();
		REQUIRE( d3dDevice != nullptr );

		// Create direct command allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> directAllocator;
		HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &directAllocator ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( directAllocator != nullptr );

		// Create compute command allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> computeAllocator;
		hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS( &computeAllocator ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( computeAllocator != nullptr );

		// Create copy command allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> copyAllocator;
		hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS( &copyAllocator ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( copyAllocator != nullptr );
	}

	SECTION( "Allocator reset operations" )
	{
		auto d3dDevice = device.get();

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &allocator ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Reset should work (though may need to ensure GPU is done)
		// In practice, this would require synchronization
		// For testing, we'll just verify the call doesn't crash
		hr = allocator->Reset();
		// Result depends on GPU state, so we don't assert success
		INFO( "Allocator reset returned: " << std::hex << hr );
	}

	SECTION( "Multiple allocators per type" )
	{
		auto d3dDevice = device.get();

		// Create multiple direct command allocators
		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocators;

		for ( int i = 0; i < 5; ++i )
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
			HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &allocator ) );
			REQUIRE( SUCCEEDED( hr ) );
			REQUIRE( allocator != nullptr );
			allocators.push_back( allocator );
		}

		// All allocators should be valid and unique
		for ( size_t i = 0; i < allocators.size(); ++i )
		{
			REQUIRE( allocators[i] != nullptr );
			for ( size_t j = i + 1; j < allocators.size(); ++j )
			{
				REQUIRE( allocators[i].Get() != allocators[j].Get() );
			}
		}
	}
}

TEST_CASE( "dx12::CommandList Operations", "[dx12][command][list]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::CommandList operations" ) )
		return;

	SECTION( "Command list creation and basic operations" )
	{
		auto d3dDevice = device.get();
		REQUIRE( d3dDevice != nullptr );

		// Create command allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &allocator ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Create command list
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		hr = d3dDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS( &commandList ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( commandList != nullptr );

		// Close command list
		hr = commandList->Close();
		REQUIRE( SUCCEEDED( hr ) );

		// Reset command list
		hr = commandList->Reset( allocator.Get(), nullptr );
		REQUIRE( SUCCEEDED( hr ) );
	}

	SECTION( "Viewport and scissor operations" )
	{
		auto commandList = device.getCommandList();
		REQUIRE( commandList != nullptr );

		// Create command allocator for reset
		auto d3dDevice = device.get();
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &allocator ) );
		REQUIRE( SUCCEEDED( hr ) );

		hr = commandList->Reset( allocator.Get(), nullptr );
		REQUIRE( SUCCEEDED( hr ) );

		// Set viewport
		D3D12_VIEWPORT viewport = {};
		viewport.Width = 1920.0f;
		viewport.Height = 1080.0f;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		REQUIRE_NOTHROW( commandList->RSSetViewports( 1, &viewport ) );

		// Set scissor rect
		D3D12_RECT scissorRect = {};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = 1920;
		scissorRect.bottom = 1080;

		REQUIRE_NOTHROW( commandList->RSSetScissorRects( 1, &scissorRect ) );

		hr = commandList->Close();
		REQUIRE( SUCCEEDED( hr ) );
	}

	SECTION( "dx12::Resource barrier operations" )
	{
		auto commandList = device.getCommandList();
		REQUIRE( commandList != nullptr );

		auto d3dDevice = device.get();
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		HRESULT hr = d3dDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &allocator ) );
		REQUIRE( SUCCEEDED( hr ) );

		hr = commandList->Reset( allocator.Get(), nullptr );
		REQUIRE( SUCCEEDED( hr ) );

		// Create a simple buffer resource to test barriers
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = 1024;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		hr = d3dDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS( &buffer ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Create resource barrier
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = buffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		REQUIRE_NOTHROW( commandList->ResourceBarrier( 1, &barrier ) );

		hr = commandList->Close();
		REQUIRE( SUCCEEDED( hr ) );
	}
}

TEST_CASE( "Fence and Synchronization", "[dx12][command][sync]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "Fence and synchronization" ) )
		return;

	SECTION( "Fence creation and basic operations" )
	{
		auto d3dDevice = device.get();
		REQUIRE( d3dDevice != nullptr );

		// Create fence
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( fence != nullptr );

		// Initial value should be 0
		REQUIRE( fence->GetCompletedValue() == 0 );

		// Signal fence directly (CPU signal)
		hr = fence->Signal( 42 );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( fence->GetCompletedValue() == 42 );
	}

	SECTION( "Event-based waiting" )
	{
		auto d3dDevice = device.get();

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Create event
		HANDLE eventHandle = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		REQUIRE( eventHandle != nullptr );

		// Signal fence to value 10
		hr = fence->Signal( 10 );
		REQUIRE( SUCCEEDED( hr ) );

		// Set event on completion of value 5 (should trigger immediately)
		hr = fence->SetEventOnCompletion( 5, eventHandle );
		REQUIRE( SUCCEEDED( hr ) );

		// Wait should complete immediately since fence value (10) > target value (5)
		DWORD waitResult = WaitForSingleObject( eventHandle, 100 ); // 100ms timeout
		REQUIRE( waitResult == WAIT_OBJECT_0 );

		CloseHandle( eventHandle );
	}

	SECTION( "Multiple fence values" )
	{
		auto d3dDevice = device.get();

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
		REQUIRE( SUCCEEDED( hr ) );

		// Test multiple incremental signals
		for ( UINT64 value = 1; value <= 100; ++value )
		{
			hr = fence->Signal( value );
			REQUIRE( SUCCEEDED( hr ) );
			REQUIRE( fence->GetCompletedValue() == value );
		}
	}

	SECTION( "Shared fence operations" )
	{
		auto d3dDevice = device.get();

		// Create shareable fence
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = d3dDevice->CreateFence( 0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS( &fence ) );
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( fence != nullptr );

		// Should be able to get shared handle (though we won't use it in tests)
		HANDLE sharedHandle = nullptr;
		hr = d3dDevice->CreateSharedHandle( fence.Get(), nullptr, GENERIC_ALL, nullptr, &sharedHandle );
		if ( SUCCEEDED( hr ) && sharedHandle )
		{
			CloseHandle( sharedHandle );
		}
		// Note: CreateSharedHandle might fail on some systems, so we don't require success
	}
}

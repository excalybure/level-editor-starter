// D3D12 dx12::Device comprehensive initialization and state tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"
#include "math/color.h"


TEST_CASE( "Device Initialization Methods", "[dx12][device][initialization]" )
{
	SECTION( "Headless initialization" )
	{
		dx12::Device device;

		// Headless initialization should succeed on D3D12-capable systems
		bool result = device.initializeHeadless();

		if ( result )
		{
			REQUIRE( device.get() != nullptr );
			REQUIRE( device.getDevice() != nullptr );
			REQUIRE( device.getFactory() != nullptr );
			REQUIRE( device.getImguiDescriptorHeap() != nullptr );
			REQUIRE( device.getCommandList() != nullptr );
			REQUIRE( device.getTextureManager() != nullptr );

			// Shutdown should be safe
			REQUIRE_NOTHROW( device.shutdown() );
		}
		else
		{
			// On systems without D3D12, this is acceptable
			WARN( "Headless D3D12 initialization failed (possibly unsupported hardware)" );
		}
	}

	SECTION( "Multiple headless initializations" )
	{
		dx12::Device device;

		const bool result = device.initializeHeadless();
		if ( result )
		{
			// Second initialization - behavior may vary (re-init or fail)
			device.initializeHeadless();

			// dx12::Device should remain functional regardless
			REQUIRE( device.get() != nullptr );
			REQUIRE( device.getDevice() != nullptr );

			device.shutdown();
		}
	}

	SECTION( "Windowed initialization without window" )
	{
		dx12::Device device;

		// Should fail gracefully with null window handle
		REQUIRE_FALSE( device.initialize( nullptr ) );

		// dx12::Device should remain in safe state
		REQUIRE( device.get() == nullptr );
	}

	SECTION( "Shutdown before initialization" )
	{
		dx12::Device device;

		// Should be safe to shutdown uninitialized device
		REQUIRE_NOTHROW( device.shutdown() );
		REQUIRE_NOTHROW( device.shutdown() ); // Multiple shutdowns should be safe
	}
}

TEST_CASE( "Device Component Access", "[dx12][device][components]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Device component access" ) )
		return;

	SECTION( "Core D3D12 components" )
	{
		// All core components should be available
		REQUIRE( device.get() != nullptr );
		REQUIRE( device.getDevice() != nullptr );
		REQUIRE( device.getFactory() != nullptr );

		// Operator-> should work
		REQUIRE( device->GetNodeCount() > 0 );
	}

	SECTION( "ImGui integration components" )
	{
		auto imguiHeap = device.getImguiDescriptorHeap();
		REQUIRE( imguiHeap != nullptr );

		// ImGui heap should have valid descriptor count
		auto desc = imguiHeap->GetDesc();
		REQUIRE( desc.NumDescriptors > 0 );
		REQUIRE( desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
	}

	SECTION( "Command objects" )
	{
		auto commandList = device.getCommandList();
		REQUIRE( commandList != nullptr );

		// Command list should be in a valid state
		// (Exact state depends on implementation)
	}

	SECTION( "Texture manager integration" )
	{
		auto textureManager = device.getTextureManager();
		REQUIRE( textureManager != nullptr );

		// Should be able to initialize texture manager
		REQUIRE( textureManager->initialize( &device ) );
	}
}

TEST_CASE( "Device Frame Operations", "[dx12][device][frame]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Device frame operations" ) )
		return;

	SECTION( "Frame lifecycle without window" )
	{
		// In headless mode, frame operations should be safe even without swap chain
		REQUIRE_NOTHROW( device.beginFrame() );
		REQUIRE_NOTHROW( device.endFrame() );

		// Present should handle gracefully in headless mode
		REQUIRE_NOTHROW( device.present() );
	}

	SECTION( "Multiple frame cycles" )
	{
		// Multiple frame cycles should work
		for ( int i = 0; i < 5; ++i )
		{
			REQUIRE_NOTHROW( device.beginFrame() );
			REQUIRE_NOTHROW( device.endFrame() );
			REQUIRE_NOTHROW( device.present() );
		}
	}

	SECTION( "Mismatched begin/end calls" )
	{
		// Should handle mismatched calls gracefully
		REQUIRE_NOTHROW( device.beginFrame() );
		REQUIRE_NOTHROW( device.beginFrame() ); // Double begin
		REQUIRE_NOTHROW( device.endFrame() );
		REQUIRE_NOTHROW( device.endFrame() ); // Double end
	}

	SECTION( "End without begin" )
	{
		// Should handle gracefully
		REQUIRE_NOTHROW( device.endFrame() );
		REQUIRE_NOTHROW( device.present() );
	}
}

TEST_CASE( "Device Error Handling and Edge Cases", "[dx12][device][error]" )
{
	SECTION( "Device creation on unsupported systems" )
	{
		dx12::Device device;

		// On systems without D3D12, initialization should fail gracefully
		bool result = device.initializeHeadless();

		if ( !result )
		{
			// Should remain in safe state
			REQUIRE( device.get() == nullptr );
			REQUIRE( device.getDevice() == nullptr );
			REQUIRE( device.getFactory() == nullptr );
			REQUIRE( device.getImguiDescriptorHeap() == nullptr );
			REQUIRE( device.getCommandList() == nullptr );

			// Operations should be safe even in failed state
			REQUIRE_NOTHROW( device.beginFrame() );
			REQUIRE_NOTHROW( device.endFrame() );
			REQUIRE_NOTHROW( device.present() );
			REQUIRE_NOTHROW( device.shutdown() );
		}
	}

	SECTION( "Operations after shutdown" )
	{
		dx12::Device device;
		if ( device.initializeHeadless() )
		{
			device.shutdown();

			// All operations should be safe after shutdown
			REQUIRE_NOTHROW( device.beginFrame() );
			REQUIRE_NOTHROW( device.endFrame() );
			REQUIRE_NOTHROW( device.present() );

			// Component access should return null safely
			REQUIRE( device.get() == nullptr );
			REQUIRE( device.getDevice() == nullptr );
			REQUIRE( device.getCommandList() == nullptr );
		}
	}

	SECTION( "Destructor safety" )
	{
		// Test that destructor handles all states safely
		{
			dx12::Device device; // Uninitialized
								 // Should destruct safely
		}

		{
			dx12::Device device;
			if ( device.initializeHeadless() )
			{
				// Should destruct safely when initialized
			}
		}

		{
			dx12::Device device;
			if ( device.initializeHeadless() )
			{
				device.shutdown();
				// Should destruct safely after shutdown
			}
		}
	}
}

TEST_CASE( "Device Helper Functions", "[dx12][device][helpers]" )
{
	SECTION( "throwIfFailed with success" )
	{
		// Should not throw for S_OK
		REQUIRE_NOTHROW( dx12::throwIfFailed( S_OK ) );
		REQUIRE_NOTHROW( dx12::throwIfFailed( S_FALSE ) );
	}

	SECTION( "throwIfFailed with failure" )
	{
		// Should throw for failure codes
		REQUIRE_THROWS( dx12::throwIfFailed( E_FAIL ) );
		REQUIRE_THROWS( dx12::throwIfFailed( E_INVALIDARG ) );
		REQUIRE_THROWS( dx12::throwIfFailed( E_OUTOFMEMORY ) );
	}

	SECTION( "throwIfFailed with device context" )
	{
		dx12::Device device;
		if ( device.initializeHeadless() )
		{
			// Should not throw for success with device context
			REQUIRE_NOTHROW( dx12::throwIfFailed( S_OK, device.get() ) );

			// Should throw for failure with device context
			REQUIRE_THROWS( dx12::throwIfFailed( E_FAIL, device.get() ) );
		}
	}
}

TEST_CASE( "Device Resource Management", "[dx12][device][resources]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Device resource management" ) )
		return;

	SECTION( "Factory object properties" )
	{
		auto factory = device.getFactory();
		REQUIRE( factory != nullptr );

		// Should be able to enumerate adapters
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		HRESULT hr = factory->EnumAdapters1( 0, &adapter );

		// Should have at least one adapter
		REQUIRE( SUCCEEDED( hr ) );
		REQUIRE( adapter != nullptr );
	}

	SECTION( "Device capabilities" )
	{
		auto d3dDevice = device.get();
		REQUIRE( d3dDevice != nullptr );

		// Check device node count
		UINT nodeCount = d3dDevice->GetNodeCount();
		REQUIRE( nodeCount > 0 );

		// Check feature level support
		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
		D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_12_1 };
		featureLevels.pFeatureLevelsRequested = levels;
		featureLevels.NumFeatureLevels = ARRAYSIZE( levels );

		HRESULT hr = d3dDevice->CheckFeatureSupport( D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof( featureLevels ) );
		if ( SUCCEEDED( hr ) )
		{
			REQUIRE( featureLevels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_0 );
		}
	}

	SECTION( "Descriptor heap properties" )
	{
		auto imguiHeap = device.getImguiDescriptorHeap();
		REQUIRE( imguiHeap != nullptr );

		auto desc = imguiHeap->GetDesc();
		REQUIRE( desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
		REQUIRE( desc.NumDescriptors > 0 );
		REQUIRE( ( desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ) != 0 );
	}
}

TEST_CASE( "Device Clear Operations", "[dx12][device][clear]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Device clear operations" ) )
		return;

	SECTION( "Clear with custom color during frame" )
	{
		device.beginFrame();

		// Should be callable without crash
		const math::Color customColor{ 1.0f, 0.0f, 0.0f, 1.0f };
		REQUIRE_NOTHROW( device.clear( customColor ) );

		device.endFrame();
	}

	SECTION( "Clear with default color during frame" )
	{
		device.beginFrame();
		REQUIRE_NOTHROW( device.clear() );
		device.endFrame();
	}

	SECTION( "Clear outside frame should be safe" )
	{
		// Should handle gracefully when not in frame
		REQUIRE_NOTHROW( device.clear() );
	}

	SECTION( "ClearDepth with custom depth during frame" )
	{
		device.beginFrame();
		REQUIRE_NOTHROW( device.clearDepth( 0.5f ) );
		device.endFrame();
	}

	SECTION( "ClearDepth with default depth during frame" )
	{
		device.beginFrame();
		REQUIRE_NOTHROW( device.clearDepth() );
		device.endFrame();
	}

	SECTION( "ClearDepth outside frame should be safe" )
	{
		REQUIRE_NOTHROW( device.clearDepth() );
	}
}

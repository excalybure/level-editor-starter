// D3D12 dx12::TextureManager comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "dx12::TextureManager Initialization and Lifecycle", "[dx12][texture][manager]" )
{
	SECTION( "dx12::TextureManager initialization with valid device" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "dx12::TextureManager initialization" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager != nullptr );

		// Should be able to initialize successfully
		REQUIRE( manager->initialize( &device ) );
	}

	SECTION( "dx12::TextureManager initialization with null device" )
	{
		dx12::TextureManager manager;

		// Should fail with null device
		REQUIRE_FALSE( manager.initialize( nullptr ) );
	}

	SECTION( "dx12::TextureManager shutdown safety" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "dx12::TextureManager shutdown" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		// Should be safe to shut down multiple times
		REQUIRE_NOTHROW( manager->shutdown() );
		REQUIRE_NOTHROW( manager->shutdown() );
	}
}

TEST_CASE( "dx12::TextureManager Viewport Render Target Creation", "[dx12][texture][viewport]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::TextureManager viewport render targets" ) )
		return;

	dx12::TextureManager *manager = device.getTextureManager();
	REQUIRE( manager->initialize( &device ) );

	SECTION( "Valid render target creation" )
	{
		// Test standard resolutions
		auto texture1 = manager->createViewportRenderTarget( 800, 600 );
		REQUIRE( texture1 != nullptr );
		REQUIRE( texture1->getWidth() == 800 );
		REQUIRE( texture1->getHeight() == 600 );

		auto texture2 = manager->createViewportRenderTarget( 1920, 1080 );
		REQUIRE( texture2 != nullptr );
		REQUIRE( texture2->getWidth() == 1920 );
		REQUIRE( texture2->getHeight() == 1080 );

		// Different instances should be unique
		REQUIRE( texture1.get() != texture2.get() );
	}

	SECTION( "Multiple render targets" )
	{
		// Create multiple render targets
		std::vector<std::shared_ptr<dx12::Texture>> textures;

		for ( int i = 0; i < 8; ++i )
		{
			auto texture = manager->createViewportRenderTarget( 640 + i * 64, 480 + i * 48 );
			REQUIRE( texture != nullptr );
			textures.push_back( texture );
		}

		// All should be unique
		for ( size_t i = 0; i < textures.size(); ++i )
		{
			for ( size_t j = i + 1; j < textures.size(); ++j )
			{
				REQUIRE( textures[i].get() != textures[j].get() );
			}
		}
	}

	SECTION( "Invalid dimensions" )
	{
		// Test zero dimensions
		auto texture1 = manager->createViewportRenderTarget( 0, 600 );
		REQUIRE( texture1 == nullptr );

		auto texture2 = manager->createViewportRenderTarget( 800, 0 );
		REQUIRE( texture2 == nullptr );

		auto texture3 = manager->createViewportRenderTarget( 0, 0 );
		REQUIRE( texture3 == nullptr );
	}

	SECTION( "Extreme dimensions" )
	{
		// Test very large dimensions (should handle gracefully)
		auto largeTexture = manager->createViewportRenderTarget( 8192, 8192 );
		// Note: This might succeed or fail depending on system capabilities
		// The key is it shouldn't crash

		// Test very small but valid dimensions
		auto smallTexture = manager->createViewportRenderTarget( 1, 1 );
		REQUIRE( smallTexture != nullptr );
		REQUIRE( smallTexture->getWidth() == 1 );
		REQUIRE( smallTexture->getHeight() == 1 );
	}
}

TEST_CASE( "dx12::TextureManager SRV Handle Management", "[dx12][texture][srv]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::TextureManager SRV handles" ) )
		return;

	dx12::TextureManager *manager = device.getTextureManager();
	REQUIRE( manager->initialize( &device ) );

	SECTION( "SRV handle allocation" )
	{
		// Get multiple SRV handles
		auto handle1 = manager->getNextSrvHandle();
		auto handle2 = manager->getNextSrvHandle();
		auto handle3 = manager->getNextSrvHandle();

		// Handles should have different pointer values
		REQUIRE( handle1.ptr != handle2.ptr );
		REQUIRE( handle2.ptr != handle3.ptr );
		REQUIRE( handle1.ptr != handle3.ptr );
	}

	SECTION( "SRV handle consistency" )
	{
		// Sequential handles should be incremental
		auto handle1 = manager->getNextSrvHandle();
		auto handle2 = manager->getNextSrvHandle();

		// The difference should be consistent with descriptor size
		// (exact value depends on implementation)
		REQUIRE( handle2.ptr > handle1.ptr );
	}

	SECTION( "SRV handle exhaustion resilience" )
	{
		// Create many handles to test bounds checking
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;

		for ( int i = 0; i < TextureManager::kMaxTextures + 10; ++i )
		{
			auto handle = manager->getNextSrvHandle();
			handles.push_back( handle );
		}

		// Should handle gracefully without crashing
		REQUIRE( handles.size() == TextureManager::kMaxTextures + 10 );
	}
}

TEST_CASE( "dx12::TextureManager Constants and Limits", "[dx12][texture][constants]" )
{
	SECTION( "Compile-time constants" )
	{
		// Verify expected constants
		REQUIRE( TextureManager::kMaxTextures == 64 );
		REQUIRE( TextureManager::kSrvIndexOffset == 16 );

		// Constants should be reasonable
		REQUIRE( TextureManager::kMaxTextures > 0 );
		REQUIRE( TextureManager::kSrvIndexOffset >= 0 );
		REQUIRE( TextureManager::kMaxTextures + TextureManager::kSrvIndexOffset < 1024 ); // Reasonable upper bound
	}

	SECTION( "Maximum texture creation" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "Maximum texture creation" ) )
			return;

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		// Create up to the maximum number of textures
		std::vector<std::shared_ptr<dx12::Texture>> textures;

		for ( UINT i = 0; i < TextureManager::kMaxTextures && i < 16; ++i ) // Limit to 16 for test performance
		{
			auto texture = manager->createViewportRenderTarget( 256, 256 );
			if ( texture )
			{
				textures.push_back( texture );
			}
		}

		// Should have created at least some textures
		REQUIRE( textures.size() > 0 );
		REQUIRE( textures.size() <= TextureManager::kMaxTextures );
	}
}

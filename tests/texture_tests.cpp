// D3D12 dx12::Texture class comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"

using Catch::Matchers::WithinAbs;

TEST_CASE( "dx12::Texture Creation and Properties", "[dx12][texture][creation]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture creation" ) )
		return;

	SECTION( "Default texture state" )
	{
		dx12::Texture texture;

		// Default values
		REQUIRE( texture.getWidth() == 0 );
		REQUIRE( texture.getHeight() == 0 );
		REQUIRE( texture.getResource() == nullptr );
		REQUIRE( texture.getImGuiTextureId() == nullptr );
	}

	SECTION( "Valid render target creation" )
	{
		dx12::Texture texture;

		// Test standard resolution
		REQUIRE( texture.createRenderTarget( &device, 800, 600 ) );
		REQUIRE( texture.getWidth() == 800 );
		REQUIRE( texture.getHeight() == 600 );
		REQUIRE( texture.getResource() != nullptr );

		// Test different resolution
		dx12::Texture texture2;
		REQUIRE( texture2.createRenderTarget( &device, 1024, 768 ) );
		REQUIRE( texture2.getWidth() == 1024 );
		REQUIRE( texture2.getHeight() == 768 );
		REQUIRE( texture2.getResource() != nullptr );

		// Resources should be different
		REQUIRE( texture.getResource() != texture2.getResource() );
	}

	SECTION( "Format specification" )
	{
		dx12::Texture texture;

		// Test with specific format
		REQUIRE( texture.createRenderTarget( &device, 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM ) );
		REQUIRE( texture.getFormat() == DXGI_FORMAT_R8G8B8A8_UNORM );
		REQUIRE( texture.getWidth() == 512 );
		REQUIRE( texture.getHeight() == 512 );
	}
}

TEST_CASE( "dx12::Texture Invalid Creation Parameters", "[dx12][texture][invalid]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture invalid parameters" ) )
		return;

	SECTION( "Null device" )
	{
		dx12::Texture texture;

		// Should fail with null device
		REQUIRE_FALSE( texture.createRenderTarget( nullptr, 800, 600 ) );
		REQUIRE( texture.getWidth() == 0 );
		REQUIRE( texture.getHeight() == 0 );
		REQUIRE( texture.getResource() == nullptr );
	}

	SECTION( "Zero dimensions" )
	{
		dx12::Texture texture;

		// Should fail with zero width
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 600 ) );

		// Should fail with zero height
		REQUIRE_FALSE( texture.createRenderTarget( &device, 800, 0 ) );

		// Should fail with both zero
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 0 ) );
	}

	SECTION( "Extreme dimensions" )
	{
		dx12::Texture texture;

		// Very large dimensions - may succeed or fail based on system limits
		bool result = texture.createRenderTarget( &device, 16384, 16384 );
		// Key is that it shouldn't crash regardless of result

		if ( result )
		{
			REQUIRE( texture.getWidth() == 16384 );
			REQUIRE( texture.getHeight() == 16384 );
		}
		else
		{
			REQUIRE( texture.getWidth() == 0 );
			REQUIRE( texture.getHeight() == 0 );
		}
	}
}

TEST_CASE( "dx12::Texture Resize Operations", "[dx12][texture][resize]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture resize" ) )
		return;

	SECTION( "Valid resize operations" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 800, 600 ) );

		auto originalResource = texture.getResource();
		REQUIRE( originalResource != nullptr );

		// Resize to different dimensions
		REQUIRE( texture.resize( &device, 1024, 768 ) );
		REQUIRE( texture.getWidth() == 1024 );
		REQUIRE( texture.getHeight() == 768 );

		// dx12::Resource should have changed (recreated)
		auto newResource = texture.getResource();
		REQUIRE( newResource != nullptr );
		// Note: dx12::Resource might be the same or different depending on implementation
	}

	SECTION( "Resize to same dimensions" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 640, 480 ) );

		// Resize to same dimensions
		REQUIRE( texture.resize( &device, 640, 480 ) );
		REQUIRE( texture.getWidth() == 640 );
		REQUIRE( texture.getHeight() == 480 );
		REQUIRE( texture.getResource() != nullptr );
	}

	SECTION( "Invalid resize parameters" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 800, 600 ) );

		// Resize with null device
		REQUIRE_FALSE( texture.resize( nullptr, 1024, 768 ) );
		// Original dimensions should be preserved
		REQUIRE( texture.getWidth() == 800 );
		REQUIRE( texture.getHeight() == 600 );

		// Resize with zero dimensions
		REQUIRE_FALSE( texture.resize( &device, 0, 768 ) );
		REQUIRE_FALSE( texture.resize( &device, 1024, 0 ) );

		// Dimensions should remain unchanged
		REQUIRE( texture.getWidth() == 800 );
		REQUIRE( texture.getHeight() == 600 );
	}

	SECTION( "Multiple consecutive resizes" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 100, 100 ) );

		// Chain of resizes
		REQUIRE( texture.resize( &device, 200, 150 ) );
		REQUIRE( texture.getWidth() == 200 );
		REQUIRE( texture.getHeight() == 150 );

		REQUIRE( texture.resize( &device, 300, 225 ) );
		REQUIRE( texture.getWidth() == 300 );
		REQUIRE( texture.getHeight() == 225 );

		REQUIRE( texture.resize( &device, 50, 50 ) );
		REQUIRE( texture.getWidth() == 50 );
		REQUIRE( texture.getHeight() == 50 );
	}
}

TEST_CASE( "dx12::Texture Shader dx12::Resource View", "[dx12][texture][srv]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture SRV creation" ) )
		return;

	SECTION( "SRV creation with valid texture" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 512, 512 ) );

		// Get SRV handle from dx12::TextureManager
		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		auto srvHandle = manager->getNextSrvHandle();
		REQUIRE( texture.createShaderResourceView( &device, srvHandle ) );

		// ImGui texture ID should be valid after SRV creation
		auto textureId = texture.getImGuiTextureId();
		REQUIRE( textureId != nullptr );
	}

	SECTION( "SRV creation with invalid parameters" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Create SRV with null device
		D3D12_CPU_DESCRIPTOR_HANDLE dummyHandle = {};
		REQUIRE_FALSE( texture.createShaderResourceView( nullptr, dummyHandle ) );
	}

	SECTION( "SRV creation without render target" )
	{
		dx12::Texture texture; // No render target created

		dx12::TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		auto srvHandle = manager->getNextSrvHandle();
		REQUIRE_FALSE( texture.createShaderResourceView( &device, srvHandle ) );
	}
}

TEST_CASE( "dx12::Texture Clear Operations", "[dx12][texture][clear]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture clear operations" ) )
		return;

	SECTION( "Clear with standard colors" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Clear with red
		float redColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		REQUIRE( texture.clearRenderTarget( &device, redColor ) );

		// Clear with blue
		float blueColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		REQUIRE( texture.clearRenderTarget( &device, blueColor ) );

		// Clear with transparent
		float transparentColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		REQUIRE( texture.clearRenderTarget( &device, transparentColor ) );
	}

	SECTION( "Clear with invalid parameters" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Clear with null device
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		REQUIRE_FALSE( texture.clearRenderTarget( nullptr, color ) );

		// Clear with null color array
		REQUIRE_FALSE( texture.clearRenderTarget( &device, nullptr ) );
	}

	SECTION( "Clear without render target" )
	{
		dx12::Texture texture; // No render target created

		float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		REQUIRE_FALSE( texture.clearRenderTarget( &device, color ) );
	}
}

TEST_CASE( "dx12::Texture dx12::Resource State Management", "[dx12][texture][state]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "dx12::Texture state management" ) )
		return;

	SECTION( "dx12::Resource state transitions" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Get command list for state transitions
		auto commandList = device.getCommandList();
		REQUIRE( commandList != nullptr );

		// Should not crash during state transitions
		REQUIRE_NOTHROW( texture.transitionTo( commandList, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
		REQUIRE_NOTHROW( texture.transitionTo( commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );
		REQUIRE_NOTHROW( texture.transitionTo( commandList, D3D12_RESOURCE_STATE_COMMON ) );
	}

	SECTION( "State transition with null command list" )
	{
		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Should handle null command list gracefully
		REQUIRE_NOTHROW( texture.transitionTo( nullptr, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
	}

	SECTION( "State transition without resource" )
	{
		dx12::Texture texture; // No resource created

		auto commandList = device.getCommandList();

		// Should handle gracefully when no resource exists
		REQUIRE_NOTHROW( texture.transitionTo( commandList, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
	}
}

// D3D12 Texture class comprehensive tests
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "test_dx12_helpers.h"

#include "platform/dx12/dx12_device.h"

using namespace dx12;
using Catch::Matchers::WithinAbs;

TEST_CASE( "Texture Creation and Properties", "[dx12][texture][creation]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture creation" ) )
		return;

	SECTION( "Default texture state" )
	{
		Texture texture;

		// Default values
		REQUIRE( texture.getWidth() == 0 );
		REQUIRE( texture.getHeight() == 0 );
		REQUIRE( texture.getResource() == nullptr );
		REQUIRE( texture.getImGuiTextureId() == nullptr );
	}

	SECTION( "Valid render target creation" )
	{
		Texture texture;

		// Test standard resolution
		REQUIRE( texture.createRenderTarget( &device, 800, 600 ) );
		REQUIRE( texture.getWidth() == 800 );
		REQUIRE( texture.getHeight() == 600 );
		REQUIRE( texture.getResource() != nullptr );

		// Test different resolution
		Texture texture2;
		REQUIRE( texture2.createRenderTarget( &device, 1024, 768 ) );
		REQUIRE( texture2.getWidth() == 1024 );
		REQUIRE( texture2.getHeight() == 768 );
		REQUIRE( texture2.getResource() != nullptr );

		// Resources should be different
		REQUIRE( texture.getResource() != texture2.getResource() );
	}

	SECTION( "Format specification" )
	{
		Texture texture;

		// Test with specific format
		REQUIRE( texture.createRenderTarget( &device, 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM ) );
		REQUIRE( texture.getFormat() == DXGI_FORMAT_R8G8B8A8_UNORM );
		REQUIRE( texture.getWidth() == 512 );
		REQUIRE( texture.getHeight() == 512 );
	}
}

TEST_CASE( "Texture Invalid Creation Parameters", "[dx12][texture][invalid]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture invalid parameters" ) )
		return;

	SECTION( "Null device" )
	{
		Texture texture;

		// Should fail with null device
		REQUIRE_FALSE( texture.createRenderTarget( nullptr, 800, 600 ) );
		REQUIRE( texture.getWidth() == 0 );
		REQUIRE( texture.getHeight() == 0 );
		REQUIRE( texture.getResource() == nullptr );
	}

	SECTION( "Zero dimensions" )
	{
		Texture texture;

		// Should fail with zero width
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 600 ) );

		// Should fail with zero height
		REQUIRE_FALSE( texture.createRenderTarget( &device, 800, 0 ) );

		// Should fail with both zero
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 0 ) );
	}

	SECTION( "Extreme dimensions" )
	{
		Texture texture;

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

TEST_CASE( "Texture Resize Operations", "[dx12][texture][resize]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture resize" ) )
		return;

	SECTION( "Valid resize operations" )
	{
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 800, 600 ) );

		auto originalResource = texture.getResource();
		REQUIRE( originalResource != nullptr );

		// Resize to different dimensions
		REQUIRE( texture.resize( &device, 1024, 768 ) );
		REQUIRE( texture.getWidth() == 1024 );
		REQUIRE( texture.getHeight() == 768 );

		// Resource should have changed (recreated)
		auto newResource = texture.getResource();
		REQUIRE( newResource != nullptr );
		// Note: Resource might be the same or different depending on implementation
	}

	SECTION( "Resize to same dimensions" )
	{
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 640, 480 ) );

		// Resize to same dimensions
		REQUIRE( texture.resize( &device, 640, 480 ) );
		REQUIRE( texture.getWidth() == 640 );
		REQUIRE( texture.getHeight() == 480 );
		REQUIRE( texture.getResource() != nullptr );
	}

	SECTION( "Invalid resize parameters" )
	{
		Texture texture;
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
		Texture texture;
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

TEST_CASE( "Texture Shader Resource View", "[dx12][texture][srv]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture SRV creation" ) )
		return;

	SECTION( "SRV creation with valid texture" )
	{
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 512, 512 ) );

		// Get SRV handle from TextureManager
		TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		auto srvHandle = manager->getNextSrvHandle();
		REQUIRE( texture.createShaderResourceView( &device, srvHandle ) );

		// ImGui texture ID should be valid after SRV creation
		auto textureId = texture.getImGuiTextureId();
		REQUIRE( textureId != nullptr );
	}

	SECTION( "SRV creation with invalid parameters" )
	{
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Create SRV with null device
		D3D12_CPU_DESCRIPTOR_HANDLE dummyHandle = {};
		REQUIRE_FALSE( texture.createShaderResourceView( nullptr, dummyHandle ) );
	}

	SECTION( "SRV creation without render target" )
	{
		Texture texture; // No render target created

		TextureManager *manager = device.getTextureManager();
		REQUIRE( manager->initialize( &device ) );

		auto srvHandle = manager->getNextSrvHandle();
		REQUIRE_FALSE( texture.createShaderResourceView( &device, srvHandle ) );
	}
}

TEST_CASE( "Texture Clear Operations", "[dx12][texture][clear]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture clear operations" ) )
		return;

	SECTION( "Clear with standard colors" )
	{
		Texture texture;
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
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Clear with null device
		float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		REQUIRE_FALSE( texture.clearRenderTarget( nullptr, color ) );

		// Clear with null color array
		REQUIRE_FALSE( texture.clearRenderTarget( &device, nullptr ) );
	}

	SECTION( "Clear without render target" )
	{
		Texture texture; // No render target created

		float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		REQUIRE_FALSE( texture.clearRenderTarget( &device, color ) );
	}
}

TEST_CASE( "Texture Resource State Management", "[dx12][texture][state]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "Texture state management" ) )
		return;

	SECTION( "Resource state transitions" )
	{
		Texture texture;
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
		Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Should handle null command list gracefully
		REQUIRE_NOTHROW( texture.transitionTo( nullptr, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
	}

	SECTION( "State transition without resource" )
	{
		Texture texture; // No resource created

		auto commandList = device.getCommandList();

		// Should handle gracefully when no resource exists
		REQUIRE_NOTHROW( texture.transitionTo( commandList, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
	}
}

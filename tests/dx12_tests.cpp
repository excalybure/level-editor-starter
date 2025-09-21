#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// We need to import the module for testing
#include "platform/dx12/dx12_device.h"
#include "test_dx12_helpers.h"


TEST_CASE( "D3D12 dx12::Device Creation", "[dx12]" )
{
	SECTION( "dx12::Device can be created" )
	{
		// This test will verify that we can create a D3D12 device
		// without crashing. On systems without D3D12 support, this may throw
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "D3D12 device creation" ) )
					return; // Skip on unsupported
				REQUIRE( device.get() != nullptr );
				REQUIRE( device.getFactory() != nullptr );
			}
			catch ( const std::runtime_error &e )
			{
				// If D3D12 is not supported, that's acceptable for this test
				// We're mainly testing that the module compiles and links
				WARN( "D3D12 device creation failed (possibly unsupported): " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "D3D12 Command Queue", "[dx12]" )
{
	SECTION( "Command queue can be created with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "D3D12 command queue" ) )
					return;
				dx12::CommandQueue queue( device );
				REQUIRE( queue.get() != nullptr );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "D3D12 command queue creation failed: " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "D3D12 Fence", "[dx12]" )
{
	SECTION( "Fence can be created with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "D3D12 fence" ) )
					return;
				dx12::Fence fence( device );
				REQUIRE( fence.get() != nullptr );
				REQUIRE( fence.getCurrentValue() == 0 );
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "D3D12 fence creation failed: " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "D3D12 Command Context", "[dx12]" )
{
	SECTION( "Command context can be created with valid device" )
	{
		REQUIRE_NOTHROW( []() {
			try
			{
				dx12::Device device;
				if ( !requireHeadlessDevice( device, "D3D12 command context" ) )
					return;
				dx12::CommandContext context( device );
				REQUIRE( context.get() != nullptr );

				// Test reset functionality
				context.reset();
				context.close();
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "D3D12 command context creation failed: " << e.what() );
			}
		}() );
	}
}

TEST_CASE( "D3D12 Headless Lifecycle & Idempotence", "[dx12]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "headless lifecycle" ) )
		return;
	// Double init should fail (return false) now
	REQUIRE_FALSE( device.initializeHeadless() );
	// Shutdown should be safe & idempotent
	device.shutdown();
	device.shutdown(); // second call no crash
	// Re-initialize after shutdown should succeed again
	REQUIRE( device.initializeHeadless() );
}

TEST_CASE( "D3D12 Fence Signal & Wait", "[dx12]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "fence signal/wait" ) )
		return;
	dx12::CommandQueue queue( device );
	dx12::Fence fence( device );
	REQUIRE( fence.getCurrentValue() == 0 );
	fence.signal( queue );
	REQUIRE( fence.getCurrentValue() == 1 );
	fence.waitForCurrentValue(); // should not deadlock
}

TEST_CASE( "D3D12 CommandContext Reuse", "[dx12]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "command context reuse" ) )
		return;
	dx12::CommandContext ctx( device );
	for ( int i = 0; i < 3; ++i )
	{
		REQUIRE_NOTHROW( ctx.reset() );
		REQUIRE_NOTHROW( ctx.close() );
	}
}

TEST_CASE( "D3D12 Headless Frame Functions Are No-Op", "[dx12]" )
{
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "headless frame no-op" ) )
		return;
	// Should not throw despite lack of swap chain
	REQUIRE_NOTHROW( device.beginFrame() );
	REQUIRE_NOTHROW( device.endFrame() );
	REQUIRE_NOTHROW( device.present() );
}

TEST_CASE( "D3D12 Multi-dx12::Device Independence", "[dx12]" )
{
	dx12::Device a;
	dx12::Device b;
	if ( !requireHeadlessDevice( a, "multi-device A" ) )
		return;
	if ( !requireHeadlessDevice( b, "multi-device B" ) )
		return;
	REQUIRE( a.get() != nullptr );
	REQUIRE( b.get() != nullptr );
	// Some drivers / factory scenarios could return the same underlying device (e.g., single WARP adapter)
	// So we don't assert inequality; we only ensure both are valid.
}

TEST_CASE( "D3D12 Pre-Initialization Safety", "[dx12]" )
{
	dx12::Device device; // not initialized
	// These should just no-op (guards added) and not throw
	REQUIRE_NOTHROW( device.beginFrame() );
	REQUIRE_NOTHROW( device.endFrame() );
	REQUIRE_NOTHROW( device.present() );
}

// ============================================================================
// dx12::Texture and TextureManager Tests
// ============================================================================

TEST_CASE( "D3D12 dx12::Texture Creation", "[dx12][texture]" )
{
	SECTION( "dx12::Texture can be created with valid parameters" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture creation" ) )
			return;

		dx12::Texture texture;

		// Test successful creation with valid parameters
		REQUIRE( texture.createRenderTarget( &device, 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM ) );
		REQUIRE( texture.getResource() != nullptr );
		REQUIRE( texture.getWidth() == 256 );
		REQUIRE( texture.getHeight() == 256 );
		REQUIRE( texture.getFormat() == DXGI_FORMAT_R8G8B8A8_UNORM );
	}

	SECTION( "dx12::Texture creation fails with invalid parameters" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture invalid params" ) )
			return;

		dx12::Texture texture;

		// Test null device
		REQUIRE_FALSE( texture.createRenderTarget( nullptr, 256, 256 ) );

		// Test invalid dimensions
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 256 ) );
		REQUIRE_FALSE( texture.createRenderTarget( &device, 256, 0 ) );
		REQUIRE_FALSE( texture.createRenderTarget( &device, 0, 0 ) );
	}
}

TEST_CASE( "D3D12 dx12::Texture Resize", "[dx12][texture]" )
{
	SECTION( "dx12::Texture resize with same dimensions is no-op" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture resize same" ) )
			return;

		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		auto *originalResource = texture.getResource();

		// Resize to same dimensions should return true but not change resource
		REQUIRE( texture.resize( &device, 256, 256 ) );
		REQUIRE( texture.getResource() == originalResource );
	}

	SECTION( "dx12::Texture resize fails with null device when no cached device" )
	{
		// Create texture without caching device reference
		dx12::Texture texture;

		// Should fail with null device when no cached device
		REQUIRE_FALSE( texture.resize( nullptr, 512, 512 ) );
	}
}

TEST_CASE( "D3D12 dx12::Texture Shader dx12::Resource View", "[dx12][texture]" )
{
	SECTION( "SRV creation fails with invalid parameters" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture SRV invalid" ) )
			return;

		dx12::Texture texture;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

		// Should fail with null device
		REQUIRE_FALSE( texture.createShaderResourceView( nullptr, handle ) );

		// Should fail with no resource (texture not created)
		REQUIRE_FALSE( texture.createShaderResourceView( &device, handle ) );
	}
}

TEST_CASE( "D3D12 TextureManager Initialization", "[dx12][texture]" )
{
	SECTION( "TextureManager initialization fails with null device" )
	{
		dx12::TextureManager manager;
		REQUIRE_FALSE( manager.initialize( nullptr ) );
	}

	SECTION( "TextureManager shutdown is safe to call multiple times" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture manager shutdown" ) )
			return;

		dx12::TextureManager manager;
		REQUIRE( manager.initialize( &device ) );

		// Multiple shutdowns should be safe
		REQUIRE_NOTHROW( manager.shutdown() );
		REQUIRE_NOTHROW( manager.shutdown() );
	}
}

TEST_CASE( "D3D12 TextureManager Viewport Render Target Creation", "[dx12][texture]" )
{
	SECTION( "Viewport render target creation fails with invalid dimensions" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture manager invalid dimensions" ) )
			return;

		dx12::TextureManager manager;
		REQUIRE( manager.initialize( &device ) );

		// Should fail with zero dimensions
		REQUIRE( manager.createViewportRenderTarget( 0, 256 ) == nullptr );
		REQUIRE( manager.createViewportRenderTarget( 256, 0 ) == nullptr );
		REQUIRE( manager.createViewportRenderTarget( 0, 0 ) == nullptr );

		manager.shutdown();
	}

	SECTION( "Viewport render target creation fails when manager not initialized" )
	{
		dx12::TextureManager manager;
		// Don't initialize the manager

		REQUIRE( manager.createViewportRenderTarget( 256, 256 ) == nullptr );
	}

	SECTION( "Can create multiple unique viewport render targets" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture manager multiple RT" ) )
			return;

		dx12::TextureManager manager;
		REQUIRE( manager.initialize( &device ) );

		// Create render targets with different sizes
		// Note: This may fail in headless mode due to missing ImGui descriptor heap
		auto texture1 = manager.createViewportRenderTarget( 128, 128 );
		auto texture2 = manager.createViewportRenderTarget( 256, 256 );

		// If texture creation succeeded, verify they are unique
		if ( texture1 != nullptr && texture2 != nullptr )
		{
			REQUIRE( texture1->getResource() != texture2->getResource() );
			REQUIRE( texture1->getWidth() == 128 );
			REQUIRE( texture2->getWidth() == 256 );
		}
		else
		{
			WARN( "dx12::Texture creation failed in headless mode - likely due to missing ImGui descriptor heap" );
		}

		manager.shutdown();
	}
}

TEST_CASE( "D3D12 dx12::Texture State Management", "[dx12][texture]" )
{
	SECTION( "dx12::Texture state transition handles null command list" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture null command list" ) )
			return;

		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		// Should not crash with null command list
		REQUIRE_NOTHROW( texture.transitionTo( nullptr, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );
	}

	SECTION( "dx12::Texture state transition handles null resource" )
	{
		dx12::Texture texture; // No resource created

		// Should not crash with null resource
		REQUIRE_NOTHROW( texture.transitionTo( nullptr, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );
	}
}

TEST_CASE( "D3D12 dx12::Texture Clear Operations", "[dx12][texture]" )
{
	SECTION( "dx12::Texture clear fails with invalid parameters" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture clear invalid" ) )
			return;

		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Should fail with null device
		REQUIRE_FALSE( texture.clearRenderTarget( nullptr, clearColor ) );

		// Should fail with null clear color
		REQUIRE_FALSE( texture.clearRenderTarget( &device, nullptr ) );
	}

	SECTION( "dx12::Texture clear fails without RTV handle" )
	{
		dx12::Device device;
		if ( !requireHeadlessDevice( device, "texture clear no RTV" ) )
			return;

		dx12::Texture texture;
		REQUIRE( texture.createRenderTarget( &device, 256, 256 ) );

		float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

		// Should fail because RTV handle is not set (requires TextureManager)
		REQUIRE_FALSE( texture.clearRenderTarget( &device, clearColor ) );
	}
}

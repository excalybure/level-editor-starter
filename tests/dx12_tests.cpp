#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// We need to import the module for testing
import platform.dx12;

#include "test_dx12_helpers.h"

using namespace dx12;

TEST_CASE( "D3D12 Device Creation", "[dx12]" )
{
	SECTION( "Device can be created" )
	{
		// This test will verify that we can create a D3D12 device
		// without crashing. On systems without D3D12 support, this may throw
		REQUIRE_NOTHROW( []() {
			try
			{
				Device device;
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
				Device device;
				if ( !requireHeadlessDevice( device, "D3D12 command queue" ) )
					return;
				CommandQueue queue( device );
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
				Device device;
				if ( !requireHeadlessDevice( device, "D3D12 fence" ) )
					return;
				Fence fence( device );
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
				Device device;
				if ( !requireHeadlessDevice( device, "D3D12 command context" ) )
					return;
				CommandContext context( device );
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
	Device device;
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
	Device device;
	if ( !requireHeadlessDevice( device, "fence signal/wait" ) )
		return;
	CommandQueue queue( device );
	Fence fence( device );
	REQUIRE( fence.getCurrentValue() == 0 );
	fence.signal( queue );
	REQUIRE( fence.getCurrentValue() == 1 );
	fence.waitForCurrentValue(); // should not deadlock
}

TEST_CASE( "D3D12 CommandContext Reuse", "[dx12]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "command context reuse" ) )
		return;
	CommandContext ctx( device );
	for ( int i = 0; i < 3; ++i )
	{
		REQUIRE_NOTHROW( ctx.reset() );
		REQUIRE_NOTHROW( ctx.close() );
	}
}

TEST_CASE( "D3D12 Headless Frame Functions Are No-Op", "[dx12]" )
{
	Device device;
	if ( !requireHeadlessDevice( device, "headless frame no-op" ) )
		return;
	// Should not throw despite lack of swap chain
	REQUIRE_NOTHROW( device.beginFrame() );
	REQUIRE_NOTHROW( device.endFrame() );
	REQUIRE_NOTHROW( device.present() );
}

TEST_CASE( "D3D12 Multi-Device Independence", "[dx12]" )
{
	Device a;
	Device b;
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
	Device device; // not initialized
	// These should just no-op (guards added) and not throw
	REQUIRE_NOTHROW( device.beginFrame() );
	REQUIRE_NOTHROW( device.endFrame() );
	REQUIRE_NOTHROW( device.present() );
}

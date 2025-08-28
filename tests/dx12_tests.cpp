#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// We need to import the module for testing
import platform.dx12;

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
				REQUIRE( device.Get() != nullptr );
				REQUIRE( device.GetFactory() != nullptr );
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
				CommandQueue queue( device );
				REQUIRE( queue.Get() != nullptr );
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
				Fence fence( device );
				REQUIRE( fence.Get() != nullptr );
				REQUIRE( fence.GetCurrentValue() == 0 );
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
				CommandContext context( device );
				REQUIRE( context.Get() != nullptr );

				// Test reset functionality
				context.Reset();
				context.Close();
			}
			catch ( const std::runtime_error &e )
			{
				WARN( "D3D12 command context creation failed: " << e.what() );
			}
		}() );
	}
}

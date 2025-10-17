// SamplerManager tests
#include <catch2/catch_test_macros.hpp>
#include "graphics/sampler/sampler_manager.h"
#include "platform/dx12/dx12_device.h"
#include "test_dx12_helpers.h"

using namespace graphics;

TEST_CASE( "SamplerManager initializes successfully", "[sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "SamplerManager initialization" ) )
		return;

	SamplerManager samplerManager;

	// Act
	const bool result = samplerManager.initialize( &device );

	// Assert
	REQUIRE( result );
	REQUIRE( samplerManager.isInitialized() );
	REQUIRE( samplerManager.getHeap() != nullptr );
}

TEST_CASE( "SamplerManager fails with null device", "[sampler][unit]" )
{
	// Arrange
	SamplerManager samplerManager;

	// Act
	const bool result = samplerManager.initialize( nullptr );

	// Assert
	REQUIRE_FALSE( result );
	REQUIRE_FALSE( samplerManager.isInitialized() );
}

TEST_CASE( "SamplerManager creates all predefined samplers", "[sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "SamplerManager sampler creation" ) )
		return;

	SamplerManager samplerManager;
	samplerManager.initialize( &device );

	// Act & Assert - Verify we can get handles for all sampler types
	REQUIRE( samplerManager.getGpuHandle( SamplerType::LinearWrap ).ptr != 0 );
	REQUIRE( samplerManager.getGpuHandle( SamplerType::LinearClamp ).ptr != 0 );
	REQUIRE( samplerManager.getGpuHandle( SamplerType::PointWrap ).ptr != 0 );
	REQUIRE( samplerManager.getGpuHandle( SamplerType::PointClamp ).ptr != 0 );
	REQUIRE( samplerManager.getGpuHandle( SamplerType::AnisotropicWrap ).ptr != 0 );
	REQUIRE( samplerManager.getGpuHandle( SamplerType::AnisotropicClamp ).ptr != 0 );
}

TEST_CASE( "SamplerManager GPU handles are in correct order", "[sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "SamplerManager handle order" ) )
		return;

	SamplerManager samplerManager;
	samplerManager.initialize( &device );

	// Act
	const auto handle0 = samplerManager.getGpuHandle( SamplerType::LinearWrap );
	const auto handle1 = samplerManager.getGpuHandle( SamplerType::LinearClamp );
	const auto tableStart = samplerManager.getTableStartGpuHandle();

	// Assert - Handles should be contiguous and start at table start
	REQUIRE( tableStart.ptr == handle0.ptr );
	REQUIRE( handle1.ptr > handle0.ptr );
}

TEST_CASE( "SamplerManager shutdown clears resources", "[sampler][unit]" )
{
	// Arrange
	dx12::Device device;
	if ( !requireHeadlessDevice( device, "SamplerManager shutdown" ) )
		return;

	SamplerManager samplerManager;
	samplerManager.initialize( &device );

	// Act
	samplerManager.shutdown();

	// Assert
	REQUIRE_FALSE( samplerManager.isInitialized() );
	REQUIRE( samplerManager.getHeap() == nullptr );
}

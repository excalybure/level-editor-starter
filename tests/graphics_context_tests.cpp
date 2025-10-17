#include <catch2/catch_test_macros.hpp>
#include "graphics/graphics_context.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/material_system/material_system.h"
#include "graphics/sampler/sampler_manager.h"
#include "platform/dx12/dx12_device.h"

TEST_CASE( "GraphicsContext construction fails with null device", "[graphics_context][unit]" )
{
	// Act & Assert
	REQUIRE_THROWS_AS( graphics::GraphicsContext( nullptr ), std::invalid_argument );
}

TEST_CASE( "GraphicsContext construction succeeds with valid device", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert - all systems should be accessible
	REQUIRE( context.getDevice() == &device );
	REQUIRE( context.getShaderManager() != nullptr );
	REQUIRE( context.getMaterialSystem() != nullptr );
	REQUIRE( context.getGPUResourceManager() != nullptr );
	REQUIRE( context.getImmediateRenderer() != nullptr );
	REQUIRE( context.getSamplerManager() != nullptr );
}

TEST_CASE( "GraphicsContext initializes shader manager", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert
	auto *shaderManager = context.getShaderManager();
	REQUIRE( shaderManager != nullptr );
	// Note: ShaderManager may have shaders registered by ImmediateRenderer during construction
}

TEST_CASE( "GraphicsContext initializes material system with shader manager", "[graphics_context][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert
	auto *materialSystem = context.getMaterialSystem();
	REQUIRE( materialSystem != nullptr );
	REQUIRE( materialSystem->getShaderManager() == context.getShaderManager() );
}

TEST_CASE( "GraphicsContext initializes GPU resource manager with device", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert
	auto *gpuResourceManager = context.getGPUResourceManager();
	REQUIRE( gpuResourceManager != nullptr );
}

TEST_CASE( "GraphicsContext initializes immediate renderer with device and shader manager", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert
	auto *renderer = context.getImmediateRenderer();
	REQUIRE( renderer != nullptr );
}

TEST_CASE( "GraphicsContext initializes sampler manager with device", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act
	graphics::GraphicsContext context( &device );

	// Assert
	auto *samplerManager = context.getSamplerManager();
	REQUIRE( samplerManager != nullptr );
	REQUIRE( samplerManager->isInitialized() );
}

TEST_CASE( "GraphicsContext can be moved", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	graphics::GraphicsContext context1( &device );

	// Act - move constructor
	graphics::GraphicsContext context2( std::move( context1 ) );

	// Assert
	REQUIRE( context2.getDevice() == &device );
	REQUIRE( context2.getShaderManager() != nullptr );
	REQUIRE( context2.getMaterialSystem() != nullptr );
	REQUIRE( context2.getGPUResourceManager() != nullptr );
	REQUIRE( context2.getImmediateRenderer() != nullptr );
	REQUIRE( context2.getSamplerManager() != nullptr );
}

TEST_CASE( "GraphicsContext move assignment works correctly", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device1;
	dx12::Device device2;
	REQUIRE( device1.initializeHeadless() );
	REQUIRE( device2.initializeHeadless() );

	graphics::GraphicsContext context1( &device1 );
	graphics::GraphicsContext context2( &device2 );

	// Act - move assignment
	context2 = std::move( context1 );

	// Assert
	REQUIRE( context2.getDevice() == &device1 );
	REQUIRE( context2.getShaderManager() != nullptr );
}

TEST_CASE( "GraphicsContext with material system JSON loading", "[graphics_context][integration]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );
	const std::string materialsPath = "materials.json";

	// Act
	graphics::GraphicsContext context( &device, materialsPath );

	// Assert
	auto *materialSystem = context.getMaterialSystem();
	REQUIRE( materialSystem != nullptr );
	// MaterialSystem should have attempted to load the file (success depends on file existence)
}

TEST_CASE( "GraphicsContext without material system JSON uses default initialization", "[graphics_context][unit]" )
{
	// Arrange
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	// Act - no materials path provided
	graphics::GraphicsContext context( &device );

	// Assert
	auto *materialSystem = context.getMaterialSystem();
	REQUIRE( materialSystem != nullptr );
	REQUIRE( materialSystem->getShaderManager() != nullptr );
}

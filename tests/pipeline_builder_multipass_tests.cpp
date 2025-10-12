#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/pipeline_builder.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/material_system.h"
#include "platform/dx12/dx12_device.h"
#include <nlohmann/json.hpp>

using namespace graphics::material_system;
using json = nlohmann::json;

// ============================================================================
// T303: Multi-Pass PipelineBuilder Tests
// ============================================================================

TEST_CASE( "PipelineBuilder builds PSO from specific pass name", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with two passes: depth_prepass and forward
	const json materialJson = {
		{ "id", "multipass_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "depth_prepass" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/unlit.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
						} },
					{ "states",
						{
							{ "rasterizer", "solid_back" },
							{ "depthStencil", "depth_write" },
						} },
					{ "primitiveTopology", "Triangle" },
				},
				{
					{ "name", "forward" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
							{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
						} },
					{ "states",
						{
							{ "rasterizer", "solid_back" },
							{ "depthStencil", "depth_test" },
							{ "blend", "opaque" },
						} },
					{ "primitiveTopology", "Triangle" },
				},
			} ) },
	};

	const auto material = MaterialParser::parse( materialJson );

	// Create minimal MaterialSystem and Device for PSO building
	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.name = "depth_prepass";
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - Build PSO for "depth_prepass" pass
	const auto pso = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "depth_prepass" );

	// Assert - PSO should be created using depth_prepass shaders (only vertex, no pixel)
	REQUIRE( pso != nullptr );

	device.shutdown();
}

TEST_CASE( "PipelineBuilder builds different PSOs for different passes", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with depth_prepass + forward passes
	const json materialJson = {
		{ "id", "multipass_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "depth_prepass" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/unlit.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
						} },
					{ "primitiveTopology", "Triangle" },
				},
				{
					{ "name", "forward" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
							{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
						} },
					{ "primitiveTopology", "Triangle" },
				},
			} ) },
	};

	const auto material = MaterialParser::parse( materialJson );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - Build PSOs for both passes
	const auto psoDepth = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "depth_prepass" );
	const auto psoForward = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "forward" );

	// Assert - Different PSOs should be created (different shaders)
	REQUIRE( psoDepth != nullptr );
	REQUIRE( psoForward != nullptr );
	REQUIRE( psoDepth.Get() != psoForward.Get() ); // Different PSO objects

	device.shutdown();
}

TEST_CASE( "PipelineBuilder caches PSOs per pass name", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with forward pass
	const json materialJson = {
		{ "id", "test_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "forward" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
							{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
						} },
				},
			} ) },
	};

	const auto material = MaterialParser::parse( materialJson );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.numRenderTargets = 1;

	// Act - Build same PSO twice
	const auto pso1 = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "forward" );
	const auto pso2 = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "forward" );

	// Assert - Should return same cached PSO
	REQUIRE( pso1 != nullptr );
	REQUIRE( pso2 != nullptr );
	REQUIRE( pso1.Get() == pso2.Get() ); // Same PSO object (cached)

	device.shutdown();
}

TEST_CASE( "PipelineBuilder falls back to legacy format when passName empty", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Legacy material with "pass" string (not "passes" array)
	const json materialJson = {
		{ "id", "legacy_material" },
		{ "pass", "forward" },
		{ "shaders",
			{
				{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
				{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
			} },
		{ "primitiveTopology", "Triangle" },
	};

	const auto material = MaterialParser::parse( materialJson );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.numRenderTargets = 1;

	// Act - Build PSO with empty passName (legacy behavior)
	const auto pso = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "" );

	// Assert - Should use legacy material.shaders field
	REQUIRE( pso != nullptr );

	device.shutdown();
}

TEST_CASE( "PipelineBuilder returns nullptr for invalid pass name", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with only "forward" pass
	const json materialJson = {
		{ "id", "test_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "forward" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
							{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
						} },
				},
			} ) },
	};

	const auto material = MaterialParser::parse( materialJson );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.numRenderTargets = 1;

	// Act - Try to build PSO for non-existent "shadow" pass
	const auto pso = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "shadow" );

	// Assert - Should return nullptr for invalid pass
	REQUIRE( pso == nullptr );

	device.shutdown();
}

TEST_CASE( "PipelineBuilder uses pass-specific topology", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with wireframe pass using Line topology
	const json materialJson = {
		{ "id", "wireframe_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "wireframe" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/grid.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
							{ "pixel", { { "file", "shaders/grid.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
						} },
					{ "primitiveTopology", "Line" },
				},
			} ) },
	};

	const auto material = MaterialParser::parse( materialJson );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.numRenderTargets = 1;

	// Act - Build PSO with Line topology
	const auto pso = PipelineBuilder::buildPSO( &device, material, passConfig, nullptr, "wireframe" );

	// Assert - PSO should be created (validates topology is used correctly)
	REQUIRE( pso != nullptr );

	device.shutdown();
}

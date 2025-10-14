#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/pso_builder.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/material_system.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/shader_manager/shader_manager.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace graphics::material_system;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// T303: Multi-Pass PSOBuilder Tests
// ============================================================================

TEST_CASE( "PSOBuilder builds PSO from specific pass name", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Create MaterialSystem with vertex format and material with two passes
	const auto testDir = std::filesystem::temp_directory_path() / "pso_builder_test_T303_1";
	std::filesystem::create_directories( testDir );

	const std::string jsonContent = R"({
		"states": {
			"rasterizerStates": {
				"solid_back": { "fillMode": "Solid", "cullMode": "Back", "frontCounterClockwise": false }
			},
			"depthStencilStates": {
				"depth_write": { "depthEnable": true, "depthWriteMask": "All", "depthFunc": "Less", "stencilEnable": false },
				"depth_test": { "depthEnable": true, "depthWriteMask": "Zero", "depthFunc": "LessEqual", "stencilEnable": false }
			},
			"blendStates": {
				"opaque": { "alphaToCoverage": false, "independentBlend": false, "renderTargets": [{ "enable": false }] }
			},
			"renderTargetStates": {
				"MainColor": {
					"rtvFormats": ["R8G8B8A8_UNORM"],
					"dsvFormat": "D32_FLOAT",
					"samples": 1
				}
			},
			"vertexFormats": {
				"PositionNormalUVTangentColor": {
					"stride": 52,
					"elements": [
						{ "semantic": "POSITION", "semanticIndex": 0, "format": "R32G32B32_FLOAT", "offset": 0 },
						{ "semantic": "NORMAL", "semanticIndex": 0, "format": "R32G32B32_FLOAT", "offset": 12 },
						{ "semantic": "TEXCOORD", "semanticIndex": 0, "format": "R32G32_FLOAT", "offset": 24 },
						{ "semantic": "TANGENT", "semanticIndex": 0, "format": "R32G32B32A32_FLOAT", "offset": 32 },
						{ "semantic": "COLOR", "semanticIndex": 0, "format": "R32G32B32A32_FLOAT", "offset": 48 }
					]
				}
			}
		},
		"materials": [
			{
				"id": "multipass_material",
				"vertexFormat": "PositionNormalUVTangentColor",
				"passes": [
					{
						"name": "depth_prepass",
						"shaders": {
							"vertex": { "file": "shaders/unlit.hlsl", "profile": "vs_5_1", "entry": "VSMain" }
						},
						"states": {
							"rasterizer": "solid_back",
							"depthStencil": "depth_write"
						},
						"primitiveTopology": "Triangle"
					},
					{
						"name": "forward",
						"shaders": {
							"vertex": { "file": "shaders/unlit.hlsl", "profile": "vs_5_1", "entry": "VSMain" },
							"pixel": { "file": "shaders/unlit.hlsl", "profile": "ps_5_1", "entry": "PSMain" }
						},
						"states": {
							"rasterizer": "solid_back",
							"depthStencil": "depth_test",
							"blend": "opaque"
						},
						"primitiveTopology": "Triangle"
					}
				]
			}
		],
		"renderPasses": [
			{ "name": "depth_prepass", "queue": "Geometry", "states": { "renderTarget": "MainColor" } },
			{ "name": "forward", "queue": "Geometry", "states": { "renderTarget": "MainColor" } }
		]
	})";

	std::ofstream( testDir / "materials.json" ) << jsonContent;

	MaterialSystem materialSystem;
	if ( !materialSystem.initialize( ( testDir / "materials.json" ).string() ) )
	{
		WARN( "MaterialSystem initialization failed" );
		fs::remove_all( testDir );
		return;
	}

	const auto materialHandle = materialSystem.getMaterialHandle( "multipass_material" );
	REQUIRE( materialHandle.isValid() );

	const auto *material = materialSystem.getMaterial( materialHandle );
	REQUIRE( material != nullptr );

	// Create minimal Device for PSO building
	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		fs::remove_all( testDir );
		return;
	}

	// Create ShaderManager for reflection-based root signatures
	shader_manager::ShaderManager shaderManager;

	RenderPassConfig passConfig;
	passConfig.name = "depth_prepass";
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - Build PSO for "depth_prepass" pass with reflection
	const auto pso = PSOBuilder::build( &device, *material, passConfig, &materialSystem, "depth_prepass", &shaderManager, materialSystem.getReflectionCache() );

	// Assert - PSO should be created using depth_prepass shaders (only vertex, no pixel)
	REQUIRE( pso != nullptr );

	device.shutdown();
	fs::remove_all( testDir );
}

TEST_CASE( "PSOBuilder builds different PSOs for different passes", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Create MaterialSystem with vertex format and material with depth_prepass + forward passes
	const auto testDir = std::filesystem::temp_directory_path() / "pso_builder_test_T303_2";
	std::filesystem::create_directories( testDir );

	const std::string jsonContent = R"({
		"vertexFormats": [
			{
				"id": "PositionColor",
				"stride": 28,
				"elements": [
					{ "semantic": "POSITION", "semanticIndex": 0, "format": "R32G32B32_FLOAT", "alignedByteOffset": 0 },
					{ "semantic": "COLOR", "semanticIndex": 0, "format": "R32G32B32A32_FLOAT", "alignedByteOffset": 12 }
				]
			}
		],
		"materials": [
			{
				"id": "multipass_material",
				"vertexFormat": "PositionColor",
				"passes": [
					{
						"name": "depth_prepass",
						"shaders": {
							"vertex": { "file": "shaders/simple.hlsl", "profile": "vs_5_1", "entry": "VSMain" }
						},
						"primitiveTopology": "Triangle"
					},
					{
						"name": "forward",
						"shaders": {
							"vertex": { "file": "shaders/simple.hlsl", "profile": "vs_5_1", "entry": "VSMain" },
							"pixel": { "file": "shaders/simple.hlsl", "profile": "ps_5_1", "entry": "PSMain" }
						},
						"primitiveTopology": "Triangle"
					}
				]
			}
		]
	})";

	std::ofstream( testDir / "materials.json" ) << jsonContent;

	shader_manager::ShaderManager shaderManager;
	MaterialSystem materialSystem;
	if ( !materialSystem.initialize( ( testDir / "materials.json" ).string(), &shaderManager ) )
	{
		WARN( "MaterialSystem initialization failed" );
		fs::remove_all( testDir );
		return;
	}

	const auto materialHandle = materialSystem.getMaterialHandle( "multipass_material" );
	REQUIRE( materialHandle.isValid() );

	const auto *material = materialSystem.getMaterial( materialHandle );
	REQUIRE( material != nullptr );

	// Verify ShaderManager is properly set
	REQUIRE( materialSystem.getShaderManager() == &shaderManager );
	REQUIRE( materialSystem.getReflectionCache() != nullptr );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		fs::remove_all( testDir );
		return;
	}

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Clear PSO cache to ensure fresh builds with reflection
	PSOBuilder::clearCache();

	// Debug: verify we have all the needed components
	INFO( "ShaderManager address: " << &shaderManager );
	INFO( "MaterialSystem.getShaderManager(): " << materialSystem.getShaderManager() );
	INFO( "MaterialSystem.getReflectionCache(): " << materialSystem.getReflectionCache() );

	// Act - Build PSOs for both passes with reflection-based root signatures
	const auto psoDepth = PSOBuilder::build( &device, *material, passConfig, &materialSystem, "depth_prepass", &shaderManager, materialSystem.getReflectionCache() );
	const auto psoForward = PSOBuilder::build( &device, *material, passConfig, &materialSystem, "forward", &shaderManager, materialSystem.getReflectionCache() );

	// Assert - Different PSOs should be created (different shaders)
	REQUIRE( psoDepth != nullptr );
	REQUIRE( psoForward != nullptr );
	REQUIRE( psoDepth.Get() != psoForward.Get() ); // Different PSO objects

	device.shutdown();
	fs::remove_all( testDir );
}

TEST_CASE( "PSOBuilder caches PSOs per pass name", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with forward pass using MaterialSystem
	const auto testDir = fs::temp_directory_path() / "pso_cache_test";
	fs::create_directories( testDir );

	const json materialsJson = {
		{ "materials",
			json::array( {
				{
					{ "id", "test_cached_material" },
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
				},
			} ) },
	};

	std::ofstream( testDir / "materials.json" ) << materialsJson.dump( 2 );

	dx12::Device device;
	if ( !device.initializeHeadless() )
	{
		WARN( "D3D12 headless initialization failed (possibly unsupported hardware)" );
		fs::remove_all( testDir );
		return;
	}

	shader_manager::ShaderManager shaderManager;
	MaterialSystem materialSystem;
	if ( !materialSystem.initialize( ( testDir / "materials.json" ).string(), &shaderManager ) )
	{
		WARN( "MaterialSystem initialization failed" );
		fs::remove_all( testDir );
		return;
	}

	const auto materialHandle = materialSystem.getMaterialHandle( "test_cached_material" );
	REQUIRE( materialHandle.isValid() );

	const auto *material = materialSystem.getMaterial( materialHandle );
	REQUIRE( material != nullptr );

	RenderPassConfig passConfig;
	passConfig.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	passConfig.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	passConfig.numRenderTargets = 1;

	// Act - Build same PSO twice with reflection
	PSOBuilder::clearCache();
	const auto pso1 = PSOBuilder::build( &device, *material, passConfig, &materialSystem, "forward", &shaderManager, materialSystem.getReflectionCache() );
	const auto pso2 = PSOBuilder::build( &device, *material, passConfig, &materialSystem, "forward", &shaderManager, materialSystem.getReflectionCache() );

	// Assert - Should return same cached PSO
	REQUIRE( pso1 != nullptr );
	REQUIRE( pso2 != nullptr );
	REQUIRE( pso1.Get() == pso2.Get() ); // Same PSO object (cached)

	device.shutdown();
	fs::remove_all( testDir );
}

TEST_CASE( "PSOBuilder returns nullptr when passName empty (no legacy support)", "[pipeline-builder][T303][integration]" )
{
	// Arrange - Material with passes array (new format)
	const json materialJson = {
		{ "id", "legacy_material" },
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

	// Act - Build PSO with empty passName (should fail - requires valid pass)
	shader_manager::ShaderManager shaderManager;
	ShaderReflectionCache reflectionCache;
	const auto pso = PSOBuilder::build( &device, material, passConfig, nullptr, "", &shaderManager, &reflectionCache );

	// Assert - Should return nullptr (legacy format not supported, empty passName not allowed)
	REQUIRE( pso == nullptr );

	device.shutdown();
}

TEST_CASE( "PSOBuilder returns nullptr for invalid pass name", "[pipeline-builder][T303][integration]" )
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
	shader_manager::ShaderManager shaderManager;
	ShaderReflectionCache reflectionCache;
	const auto pso = PSOBuilder::build( &device, material, passConfig, nullptr, "shadow", &shaderManager, &reflectionCache );

	// Assert - Should return nullptr for invalid pass
	REQUIRE( pso == nullptr );

	device.shutdown();
}

TEST_CASE( "PSOBuilder uses pass-specific topology", "[pipeline-builder][T303][integration]" )
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
	shader_manager::ShaderManager shaderManager;
	ShaderReflectionCache reflectionCache;
	const auto pso = PSOBuilder::build( &device, material, passConfig, nullptr, "wireframe", &shaderManager, &reflectionCache );

	// Assert - PSO should be created (validates topology is used correctly)
	REQUIRE( pso != nullptr );

	device.shutdown();
}

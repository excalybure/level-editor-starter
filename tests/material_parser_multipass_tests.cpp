#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/parser.h"
#include <nlohmann/json.hpp>

using namespace graphics::material_system;
using json = nlohmann::json;

// ============================================================================
// T302: Multi-Pass Parser Tests
// ============================================================================

TEST_CASE( "MaterialParser parses passes array with single pass", "[material-parser][T302][unit]" )
{
	// Arrange - JSON with passes array containing one pass
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
					{ "states",
						{
							{ "rasterizer", "solid_back" },
							{ "depthStencil", "depth_test" },
							{ "blend", "opaque" },
						} },
				},
			} ) },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.id == "test_material" );
	REQUIRE( material.passes.size() == 1 );
	REQUIRE( material.passes[0].passName == "forward" );
	REQUIRE( material.passes[0].shaders.size() == 2 );
	REQUIRE( material.passes[0].states.rasterizer == "solid_back" );
	REQUIRE( material.passes[0].states.depthStencil == "depth_test" );
	REQUIRE( material.passes[0].states.blend == "opaque" );
}

TEST_CASE( "MaterialParser parses passes array with multiple passes", "[material-parser][T302][unit]" )
{
	// Arrange - JSON with passes array containing depth prepass + forward
	const json materialJson = {
		{ "id", "pbr_material" },
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
							{ "depthStencil", "depth_write" },
							{ "rasterizer", "solid_back" },
						} },
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
				},
			} ) },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.id == "pbr_material" );
	REQUIRE( material.passes.size() == 2 );
	REQUIRE( material.passes[0].passName == "depth_prepass" );
	REQUIRE( material.passes[0].shaders.size() == 1 );
	REQUIRE( material.passes[1].passName == "forward" );
	REQUIRE( material.passes[1].shaders.size() == 2 );
}

TEST_CASE( "MaterialParser parses pass-specific parameters", "[material-parser][T302][unit]" )
{
	// Arrange - JSON with pass containing parameters
	const json materialJson = {
		{ "id", "shadow_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "shadow_cast" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/unlit.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
						} },
					{ "parameters",
						json::array( {
							{ { "name", "shadowBias" }, { "type", "float" }, { "defaultValue", 0.001 } },
						} ) },
				},
			} ) },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.passes.size() == 1 );
	REQUIRE( material.passes[0].passName == "shadow_cast" );
	REQUIRE( material.passes[0].parameters.size() == 1 );
	REQUIRE( material.passes[0].parameters[0].name == "shadowBias" );
	REQUIRE( material.passes[0].parameters[0].type == ParameterType::Float );
}

TEST_CASE( "MaterialParser parses pass-specific topology", "[material-parser][T302][unit]" )
{
	// Arrange - JSON with wireframe pass using Line topology
	const json materialJson = {
		{ "id", "debug_material" },
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

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert
	REQUIRE( material.passes.size() == 1 );
	REQUIRE( material.passes[0].passName == "wireframe" );
	REQUIRE( material.passes[0].topology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE );
}

TEST_CASE( "MaterialParser falls back to legacy single-pass format", "[material-parser][T302][unit]" )
{
	// Arrange - JSON without passes array (legacy format)
	const json materialJson = {
		{ "id", "legacy_material" },
		{ "pass", "forward" },
		{ "shaders",
			{
				{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
				{ "pixel", { { "file", "shaders/simple.hlsl" }, { "profile", "ps_5_1" }, { "entry", "PSMain" } } },
			} },
		{ "states",
			{
				{ "rasterizer", "solid_back" },
			} },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert - legacy format converted to multi-pass with single pass
	REQUIRE( material.id == "legacy_material" );
	REQUIRE( material.passes.size() == 1 );
	REQUIRE( material.passes[0].passName == "forward" );
	REQUIRE( material.passes[0].shaders.size() == 2 );
	REQUIRE( material.passes[0].states.rasterizer == "solid_back" );
}

TEST_CASE( "MaterialParser handles missing pass name gracefully", "[material-parser][T302][unit]" )
{
	// Arrange - JSON with pass missing name field
	const json materialJson = {
		{ "id", "invalid_material" },
		{ "passes",
			json::array( {
				{
					// Missing "name" field
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
						} },
				},
			} ) },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert - parser should skip invalid pass or return empty material
	REQUIRE( material.id == "invalid_material" );
	// Either passes is empty (skipped invalid pass) or material is empty (fatal error)
	// Implementation can choose either behavior
}

TEST_CASE( "MaterialParser parses pass with all optional fields omitted", "[material-parser][T302][unit]" )
{
	// Arrange - minimal pass with only name and shaders
	const json materialJson = {
		{ "id", "minimal_material" },
		{ "passes",
			json::array( {
				{
					{ "name", "forward" },
					{ "shaders",
						{
							{ "vertex", { { "file", "shaders/simple.hlsl" }, { "profile", "vs_5_1" }, { "entry", "VSMain" } } },
						} },
				},
			} ) },
	};

	// Act
	const auto material = MaterialParser::parse( materialJson );

	// Assert - should use defaults
	REQUIRE( material.passes.size() == 1 );
	REQUIRE( material.passes[0].passName == "forward" );
	REQUIRE( material.passes[0].shaders.size() == 1 );
	REQUIRE( material.passes[0].parameters.empty() );
	REQUIRE( material.passes[0].states.rasterizer.empty() );
	REQUIRE( material.passes[0].topology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE ); // Default
}

#include <catch2/catch_test_macros.hpp>
#include "graphics/material_system/parser.h"

using namespace graphics::material_system;

// ============================================================================
// T301: MaterialPass Structure Tests
// ============================================================================

TEST_CASE( "MaterialPass has required fields", "[material-pass][T301][unit]" )
{
	// Arrange - create a MaterialPass
	MaterialPass pass;
	pass.passName = "forward";

	// Add a shader
	ShaderReference vsShader;
	vsShader.stage = ShaderStage::Vertex;
	vsShader.file = "shaders/test.hlsl";
	vsShader.entryPoint = "VSMain";
	vsShader.profile = "vs_5_1";
	pass.shaders.push_back( vsShader );

	// Add states
	pass.states.rasterizer = "solid_back";
	pass.states.depthStencil = "depth_test";
	pass.states.blend = "opaque";

	// Add topology
	pass.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Assert - verify all fields are accessible
	REQUIRE( pass.passName == "forward" );
	REQUIRE( pass.shaders.size() == 1 );
	REQUIRE( pass.shaders[0].stage == ShaderStage::Vertex );
	REQUIRE( pass.states.rasterizer == "solid_back" );
	REQUIRE( pass.topology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE );
}

TEST_CASE( "MaterialDefinition supports multiple passes", "[material-pass][T301][unit]" )
{
	// Arrange - create a MaterialDefinition with multiple passes
	MaterialDefinition material;
	material.id = "test_material";
	material.vertexFormat = "PositionNormalUV";

	// Add depth prepass
	MaterialPass depthPass;
	depthPass.passName = "depth_prepass";
	ShaderReference depthVS;
	depthVS.stage = ShaderStage::Vertex;
	depthVS.file = "shaders/depth.hlsl";
	depthVS.entryPoint = "VSMain";
	depthVS.profile = "vs_5_1";
	depthPass.shaders.push_back( depthVS );
	depthPass.states.depthStencil = "depth_write";
	material.passes.push_back( depthPass );

	// Add forward pass
	MaterialPass forwardPass;
	forwardPass.passName = "forward";
	ShaderReference forwardVS;
	forwardVS.stage = ShaderStage::Vertex;
	forwardVS.file = "shaders/pbr.hlsl";
	forwardVS.entryPoint = "VSMain";
	forwardVS.profile = "vs_5_1";
	forwardPass.shaders.push_back( forwardVS );
	forwardPass.states.depthStencil = "depth_test";
	material.passes.push_back( forwardPass );

	// Assert - verify passes are stored correctly
	REQUIRE( material.passes.size() == 2 );
	REQUIRE( material.passes[0].passName == "depth_prepass" );
	REQUIRE( material.passes[1].passName == "forward" );
}

TEST_CASE( "MaterialDefinition::getPass returns correct pass by name", "[material-pass][T301][unit]" )
{
	// Arrange - material with multiple passes
	MaterialDefinition material;
	material.id = "test_material";

	MaterialPass pass1;
	pass1.passName = "shadow_cast";
	material.passes.push_back( pass1 );

	MaterialPass pass2;
	pass2.passName = "forward";
	material.passes.push_back( pass2 );

	// Act - query passes by name
	const auto *shadowPass = material.getPass( "shadow_cast" );
	const auto *forwardPass = material.getPass( "forward" );
	const auto *missingPass = material.getPass( "nonexistent" );

	// Assert - verify correct passes returned
	REQUIRE( shadowPass != nullptr );
	REQUIRE( shadowPass->passName == "shadow_cast" );
	REQUIRE( forwardPass != nullptr );
	REQUIRE( forwardPass->passName == "forward" );
	REQUIRE( missingPass == nullptr );
}

TEST_CASE( "MaterialDefinition::hasPass checks pass existence", "[material-pass][T301][unit]" )
{
	// Arrange - material with passes
	MaterialDefinition material;
	material.id = "test_material";

	MaterialPass pass;
	pass.passName = "forward";
	material.passes.push_back( pass );

	// Act & Assert - verify hasPass works correctly
	REQUIRE( material.hasPass( "forward" ) == true );
	REQUIRE( material.hasPass( "shadow_cast" ) == false );
	REQUIRE( material.hasPass( "" ) == false );
}

TEST_CASE( "MaterialPass supports pass-specific parameters", "[material-pass][T301][unit]" )
{
	// Arrange - pass with specific parameters
	MaterialPass pass;
	pass.passName = "forward";

	// Add pass-specific parameter
	Parameter param;
	param.name = "shadowBias";
	param.type = ParameterType::Float;
	param.defaultValue = 0.001f;
	pass.parameters.push_back( param );

	// Assert - verify parameters stored
	REQUIRE( pass.parameters.size() == 1 );
	REQUIRE( pass.parameters[0].name == "shadowBias" );
	REQUIRE( pass.parameters[0].type == ParameterType::Float );
}

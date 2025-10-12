#include "parser.h"
#include "core/console.h"
#include <stdexcept>
#include <regex>
#include <filesystem>
#include <unordered_set>

namespace graphics::material_system
{

std::string shaderStageToString( ShaderStage stage )
{
	switch ( stage )
	{
	case ShaderStage::Vertex:
		return "vertex";
	case ShaderStage::Pixel:
		return "pixel";
	case ShaderStage::Domain:
		return "domain";
	case ShaderStage::Hull:
		return "hull";
	case ShaderStage::Geometry:
		return "geometry";
	case ShaderStage::Compute:
		return "compute";
	default:
		return "unknown";
	}
}

MaterialDefinition MaterialParser::parse( const nlohmann::json &jsonMaterial )
{
	MaterialDefinition material;

	// Parse required fields
	if ( !jsonMaterial.contains( "id" ) || !jsonMaterial["id"].is_string() )
	{
		console::error( "MaterialParser: Missing or invalid 'id' field" );
		return material; // Return empty material
	}
	material.id = jsonMaterial["id"].get<std::string>();

	if ( !jsonMaterial.contains( "pass" ) || !jsonMaterial["pass"].is_string() )
	{
		console::error( "MaterialParser: Missing or invalid 'pass' field in material '{}'", material.id );
		return material;
	}
	material.pass = jsonMaterial["pass"].get<std::string>();

	// Parse vertexFormat (optional)
	if ( jsonMaterial.contains( "vertexFormat" ) && jsonMaterial["vertexFormat"].is_string() )
	{
		material.vertexFormat = jsonMaterial["vertexFormat"].get<std::string>();
	}

	// Parse primitiveTopology (optional, defaults to TRIANGLE)
	if ( jsonMaterial.contains( "primitiveTopology" ) && jsonMaterial["primitiveTopology"].is_string() )
	{
		const std::string topologyStr = jsonMaterial["primitiveTopology"].get<std::string>();
		if ( topologyStr == "Triangle" )
			material.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		else if ( topologyStr == "Line" )
			material.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		else if ( topologyStr == "Point" )
			material.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		else if ( topologyStr == "Patch" )
			material.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		else
		{
			console::error( "MaterialParser: Unknown primitiveTopology '{}', defaulting to Triangle", topologyStr );
			material.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
	}

	// Parse shaders (required)
	if ( !jsonMaterial.contains( "shaders" ) || !jsonMaterial["shaders"].is_object() )
	{
		console::error( "MaterialParser: Missing or invalid 'shaders' field in material '{}'", material.id );
		return material;
	}

	const auto &shadersObj = jsonMaterial["shaders"];
	std::unordered_set<ShaderStage> seenStages; // Track for duplicate detection

	for ( auto it = shadersObj.begin(); it != shadersObj.end(); ++it )
	{
		ShaderReference shaderRef;
		shaderRef.stage = parseShaderStage( it.key() );

		// Check for duplicate shader stages
		if ( seenStages.find( shaderRef.stage ) != seenStages.end() )
		{
			console::errorAndThrow( "MaterialParser: Duplicate shader stage '{}' in material '{}'",
				it.key(),
				material.id );
		}
		seenStages.insert( shaderRef.stage );

		// Only accept object mode (legacy string mode no longer supported)
		if ( it.value().is_string() )
		{
			console::errorAndThrow( "MaterialParser: Legacy string shader references no longer supported. Shader '{}' in material '{}' must be an object with 'file', 'profile', etc.",
				it.key(),
				material.id );
		}
		else if ( !it.value().is_object() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in material '{}' must be an object",
				it.key(),
				material.id );
		}

		// Parse inline shader object
		const auto &shaderObj = it.value();

		// Required: file
		if ( !shaderObj.contains( "file" ) || !shaderObj["file"].is_string() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in material '{}' missing required 'file' field",
				it.key(),
				material.id );
		}
		shaderRef.file = shaderObj["file"].get<std::string>();

		// Validate file path exists (relative to current working directory)
		if ( !std::filesystem::exists( shaderRef.file ) )
		{
			console::errorAndThrow( "MaterialParser: Shader file '{}' for shader '{}' in material '{}' does not exist",
				shaderRef.file,
				it.key(),
				material.id );
		}

		// Required: profile
		if ( !shaderObj.contains( "profile" ) || !shaderObj["profile"].is_string() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in material '{}' missing required 'profile' field",
				it.key(),
				material.id );
		}
		shaderRef.profile = shaderObj["profile"].get<std::string>();

		// Validate profile format (vs_X_Y, ps_X_Y, etc.)
		const std::string &profile = shaderRef.profile;
		const std::regex profileRegex( R"((vs|ps|ds|hs|gs|cs)_\d+_\d+)" );
		if ( !std::regex_match( profile, profileRegex ) )
		{
			console::errorAndThrow( "MaterialParser: Invalid profile '{}' for shader '{}' in material '{}'. Expected format: (vs|ps|ds|hs|gs|cs)_X_Y",
				profile,
				it.key(),
				material.id );
		}

		// Optional: entry (default "main")
		if ( shaderObj.contains( "entry" ) && shaderObj["entry"].is_string() )
		{
			shaderRef.entryPoint = shaderObj["entry"].get<std::string>();
		}
		else
		{
			shaderRef.entryPoint = "main";
		}

		// Optional: defines
		if ( shaderObj.contains( "defines" ) && shaderObj["defines"].is_array() )
		{
			for ( const auto &defineJson : shaderObj["defines"] )
			{
				if ( defineJson.is_string() )
				{
					shaderRef.defines.push_back( defineJson.get<std::string>() );
				}
			}
		}

		material.shaders.push_back( shaderRef );
	}

	// Parse optional fields
	if ( jsonMaterial.contains( "enabled" ) && jsonMaterial["enabled"].is_boolean() )
	{
		material.enabled = jsonMaterial["enabled"].get<bool>();
	}

	if ( jsonMaterial.contains( "versionHash" ) && jsonMaterial["versionHash"].is_string() )
	{
		material.versionHash = jsonMaterial["versionHash"].get<std::string>();
	}

	// Parse parameters (optional)
	if ( jsonMaterial.contains( "parameters" ) && jsonMaterial["parameters"].is_array() )
	{
		for ( const auto &paramJson : jsonMaterial["parameters"] )
		{
			Parameter param;

			if ( !paramJson.contains( "name" ) || !paramJson["name"].is_string() )
			{
				console::error( "MaterialParser: Invalid parameter in material '{}' - missing 'name'", material.id );
				continue;
			}
			param.name = paramJson["name"].get<std::string>();

			if ( !paramJson.contains( "type" ) || !paramJson["type"].is_string() )
			{
				console::error( "MaterialParser: Invalid parameter '{}' in material '{}' - missing 'type'", param.name, material.id );
				continue;
			}
			param.type = parseParameterType( paramJson["type"].get<std::string>() );

			if ( paramJson.contains( "defaultValue" ) )
			{
				param.defaultValue = paramJson["defaultValue"];
			}

			material.parameters.push_back( param );
		}
	}

	// Parse states (optional)
	if ( jsonMaterial.contains( "states" ) && jsonMaterial["states"].is_object() )
	{
		const auto &statesObj = jsonMaterial["states"];

		if ( statesObj.contains( "rasterizer" ) && statesObj["rasterizer"].is_string() )
		{
			material.states.rasterizer = statesObj["rasterizer"].get<std::string>();
		}

		if ( statesObj.contains( "depthStencil" ) && statesObj["depthStencil"].is_string() )
		{
			material.states.depthStencil = statesObj["depthStencil"].get<std::string>();
		}

		if ( statesObj.contains( "blend" ) && statesObj["blend"].is_string() )
		{
			material.states.blend = statesObj["blend"].get<std::string>();
		}

		if ( statesObj.contains( "renderTarget" ) && statesObj["renderTarget"].is_string() )
		{
			material.states.renderTarget = statesObj["renderTarget"].get<std::string>();
		}
	}

	return material;
}

ParameterType MaterialParser::parseParameterType( const std::string &typeStr )
{
	if ( typeStr == "float" )
		return ParameterType::Float;
	if ( typeStr == "int" )
		return ParameterType::Int;
	if ( typeStr == "bool" )
		return ParameterType::Bool;
	if ( typeStr == "float4" )
		return ParameterType::Float4;

	console::error( "MaterialParser: Unknown parameter type '{}'", typeStr );
	return ParameterType::Float; // Default fallback
}

ShaderStage parseShaderStage( const std::string &stageStr )
{
	if ( stageStr == "vertex" || stageStr == "vs" )
		return ShaderStage::Vertex;
	if ( stageStr == "pixel" || stageStr == "ps" )
		return ShaderStage::Pixel;
	if ( stageStr == "domain" || stageStr == "ds" )
		return ShaderStage::Domain;
	if ( stageStr == "hull" || stageStr == "hs" )
		return ShaderStage::Hull;
	if ( stageStr == "geometry" || stageStr == "gs" )
		return ShaderStage::Geometry;
	if ( stageStr == "compute" || stageStr == "cs" )
		return ShaderStage::Compute;

	console::errorAndThrow( "MaterialParser: Unknown shader stage '{}'", stageStr );
	return ShaderStage::Vertex; // Won't reach here due to fatal
}

RenderPassDefinition MaterialParser::parseRenderPass( const nlohmann::json &jsonRenderPass )
{
	RenderPassDefinition renderPass;

	// Parse required fields
	if ( !jsonRenderPass.contains( "name" ) || !jsonRenderPass["name"].is_string() )
	{
		console::error( "MaterialParser: Missing or invalid 'name' field in render pass" );
		return renderPass;
	}
	renderPass.name = jsonRenderPass["name"].get<std::string>();

	if ( !jsonRenderPass.contains( "queue" ) || !jsonRenderPass["queue"].is_string() )
	{
		console::error( "MaterialParser: Missing or invalid 'queue' field in render pass '{}'", renderPass.name );
		return renderPass;
	}
	renderPass.queue = jsonRenderPass["queue"].get<std::string>();

	// Parse states (optional)
	if ( jsonRenderPass.contains( "states" ) && jsonRenderPass["states"].is_object() )
	{
		const auto &states = jsonRenderPass["states"];

		if ( states.contains( "rasterizer" ) && states["rasterizer"].is_string() )
			renderPass.states.rasterizer = states["rasterizer"].get<std::string>();

		if ( states.contains( "depthStencil" ) && states["depthStencil"].is_string() )
			renderPass.states.depthStencil = states["depthStencil"].get<std::string>();

		if ( states.contains( "blend" ) && states["blend"].is_string() )
			renderPass.states.blend = states["blend"].get<std::string>();

		if ( states.contains( "renderTarget" ) && states["renderTarget"].is_string() )
			renderPass.states.renderTarget = states["renderTarget"].get<std::string>();
	}

	return renderPass;
}

} // namespace graphics::material_system

#include "parser.h"
#include "core/console.h"
#include <stdexcept>

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

	// Parse shaders (required)
	if ( !jsonMaterial.contains( "shaders" ) || !jsonMaterial["shaders"].is_object() )
	{
		console::error( "MaterialParser: Missing or invalid 'shaders' field in material '{}'", material.id );
		return material;
	}

	const auto &shadersObj = jsonMaterial["shaders"];
	for ( auto it = shadersObj.begin(); it != shadersObj.end(); ++it )
	{
		ShaderReference shaderRef;
		shaderRef.stage = parseShaderStage( it.key() );
		shaderRef.shaderId = it.value().get<std::string>();
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

	console::fatal( "MaterialParser: Unknown shader stage '{}'", stageStr );
	return ShaderStage::Vertex; // Won't reach here due to fatal
}

} // namespace graphics::material_system

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

	// Check for multi-pass format ("passes" array)
	const bool hasPassesArray = jsonMaterial.contains( "passes" ) && jsonMaterial["passes"].is_array();

	if ( hasPassesArray )
	{
		// Parse multi-pass format
		const auto &passesArray = jsonMaterial["passes"];
		for ( const auto &passJson : passesArray )
		{
			MaterialPass pass = parseMaterialPass( passJson, material.id );
			if ( !pass.passName.empty() ) // Only add valid passes
			{
				material.passes.push_back( std::move( pass ) );
			}
		}
	}
	else
	{
		console::error( "MaterialParser: Material '{}' missing 'passes' array. Only multi-pass format is supported.", material.id );
		return material;
	}

	// Parse vertexFormat (optional - applies to all passes)
	if ( jsonMaterial.contains( "vertexFormat" ) && jsonMaterial["vertexFormat"].is_string() )
	{
		material.vertexFormat = jsonMaterial["vertexFormat"].get<std::string>();
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

// Helper: Parse MaterialPass from JSON
MaterialPass MaterialParser::parseMaterialPass( const nlohmann::json &jsonPass, const std::string &materialId )
{
	MaterialPass pass;

	// Parse required name field
	if ( !jsonPass.contains( "name" ) || !jsonPass["name"].is_string() )
	{
		console::error( "MaterialParser: Pass in material '{}' missing required 'name' field", materialId );
		return pass; // Return empty pass (will be skipped by caller)
	}
	pass.passName = jsonPass["name"].get<std::string>();

	// Parse shaders (required)
	if ( jsonPass.contains( "shaders" ) && jsonPass["shaders"].is_object() )
	{
		parseShaders( pass.shaders, jsonPass["shaders"], materialId + "::" + pass.passName );
	}
	else
	{
		console::error( "MaterialParser: Pass '{}' in material '{}' missing 'shaders' field", pass.passName, materialId );
		pass.passName.clear(); // Mark as invalid
		return pass;
	}

	// Parse optional states
	if ( jsonPass.contains( "states" ) && jsonPass["states"].is_object() )
	{
		parseStates( pass.states, jsonPass["states"] );
	}

	// Parse optional parameters
	if ( jsonPass.contains( "parameters" ) && jsonPass["parameters"].is_array() )
	{
		parseParameters( pass.parameters, jsonPass["parameters"], materialId + "::" + pass.passName );
	}

	// Parse optional primitiveTopology
	if ( jsonPass.contains( "primitiveTopology" ) && jsonPass["primitiveTopology"].is_string() )
	{
		pass.topology = parseTopology( jsonPass["primitiveTopology"].get<std::string>() );
	}

	return pass;
}

// Helper: Parse shaders object into vector
void MaterialParser::parseShaders( std::vector<ShaderReference> &outShaders, const nlohmann::json &shadersObj, const std::string &contextId )
{
	std::unordered_set<ShaderStage> seenStages; // Track for duplicate detection

	for ( auto it = shadersObj.begin(); it != shadersObj.end(); ++it )
	{
		ShaderReference shaderRef;
		shaderRef.stage = parseShaderStage( it.key() );

		// Check for duplicate shader stages
		if ( seenStages.find( shaderRef.stage ) != seenStages.end() )
		{
			console::errorAndThrow( "MaterialParser: Duplicate shader stage '{}' in '{}'",
				it.key(),
				contextId );
		}
		seenStages.insert( shaderRef.stage );

		// Only accept object mode
		if ( it.value().is_string() )
		{
			console::errorAndThrow( "MaterialParser: Legacy string shader references no longer supported. Shader '{}' in '{}' must be an object with 'file', 'profile', etc.",
				it.key(),
				contextId );
		}
		else if ( !it.value().is_object() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in '{}' must be an object",
				it.key(),
				contextId );
		}

		// Parse inline shader object
		const auto &shaderObj = it.value();

		// Required: file
		if ( !shaderObj.contains( "file" ) || !shaderObj["file"].is_string() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in '{}' missing required 'file' field",
				it.key(),
				contextId );
		}
		shaderRef.file = shaderObj["file"].get<std::string>();

		// Validate file path exists
		if ( !std::filesystem::exists( shaderRef.file ) )
		{
			console::errorAndThrow( "MaterialParser: Shader file '{}' for shader '{}' in '{}' does not exist",
				shaderRef.file,
				it.key(),
				contextId );
		}

		// Required: profile
		if ( !shaderObj.contains( "profile" ) || !shaderObj["profile"].is_string() )
		{
			console::errorAndThrow( "MaterialParser: Shader '{}' in '{}' missing required 'profile' field",
				it.key(),
				contextId );
		}
		shaderRef.profile = shaderObj["profile"].get<std::string>();

		// Validate profile format
		const std::string &profile = shaderRef.profile;
		const std::regex profileRegex( R"((vs|ps|ds|hs|gs|cs)_\d+_\d+)" );
		if ( !std::regex_match( profile, profileRegex ) )
		{
			console::errorAndThrow( "MaterialParser: Invalid profile '{}' for shader '{}' in '{}'. Expected format: (vs|ps|ds|hs|gs|cs)_X_Y",
				profile,
				it.key(),
				contextId );
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

		outShaders.push_back( shaderRef );
	}
}

// Helper: Parse parameters array
void MaterialParser::parseParameters( std::vector<Parameter> &outParameters, const nlohmann::json &parametersArray, const std::string &contextId )
{
	for ( const auto &paramJson : parametersArray )
	{
		Parameter param;

		if ( !paramJson.contains( "name" ) || !paramJson["name"].is_string() )
		{
			console::error( "MaterialParser: Invalid parameter in '{}' - missing 'name'", contextId );
			continue;
		}
		param.name = paramJson["name"].get<std::string>();

		if ( !paramJson.contains( "type" ) || !paramJson["type"].is_string() )
		{
			console::error( "MaterialParser: Invalid parameter '{}' in '{}' - missing 'type'", param.name, contextId );
			continue;
		}
		param.type = parseParameterType( paramJson["type"].get<std::string>() );

		if ( paramJson.contains( "defaultValue" ) )
		{
			param.defaultValue = paramJson["defaultValue"];
		}

		outParameters.push_back( param );
	}
}

// Helper: Parse states object
void MaterialParser::parseStates( StateReferences &outStates, const nlohmann::json &statesObj )
{
	if ( statesObj.contains( "rasterizer" ) && statesObj["rasterizer"].is_string() )
	{
		outStates.rasterizer = statesObj["rasterizer"].get<std::string>();
	}

	if ( statesObj.contains( "depthStencil" ) && statesObj["depthStencil"].is_string() )
	{
		outStates.depthStencil = statesObj["depthStencil"].get<std::string>();
	}

	if ( statesObj.contains( "blend" ) && statesObj["blend"].is_string() )
	{
		outStates.blend = statesObj["blend"].get<std::string>();
	}

	if ( statesObj.contains( "renderTarget" ) && statesObj["renderTarget"].is_string() )
	{
		outStates.renderTarget = statesObj["renderTarget"].get<std::string>();
	}
}

// Helper: Parse primitiveTopology string
D3D12_PRIMITIVE_TOPOLOGY_TYPE MaterialParser::parseTopology( const std::string &topologyStr )
{
	if ( topologyStr == "Triangle" )
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	else if ( topologyStr == "Line" )
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	else if ( topologyStr == "Point" )
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	else if ( topologyStr == "Patch" )
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	else
	{
		console::error( "MaterialParser: Unknown primitiveTopology '{}', defaulting to Triangle", topologyStr );
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

// MaterialDefinition query methods
const MaterialPass *MaterialDefinition::getPass( const std::string &passName ) const
{
	for ( const auto &materialPass : passes )
	{
		if ( materialPass.passName == passName )
			return &materialPass;
	}
	return nullptr;
}

bool MaterialDefinition::hasPass( const std::string &passName ) const
{
	return getPass( passName ) != nullptr;
}

} // namespace graphics::material_system

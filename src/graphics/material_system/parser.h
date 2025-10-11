#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace graphics::material_system
{

// Parameter type enumeration
enum class ParameterType
{
	Float,
	Int,
	Bool,
	Float4
};

// Shader stage enumeration
enum class ShaderStage
{
	Vertex,
	Pixel,
	Domain,
	Hull,
	Geometry,
	Compute
};

// Parameter definition
struct Parameter
{
	std::string name;
	ParameterType type;
	nlohmann::json defaultValue; // Stores scalar or array depending on type
};

// Shader entry reference (within a material)
struct ShaderReference
{
	ShaderStage stage;				  // Vertex, Pixel, Compute, etc.
	std::string shaderId;			  // References a ShaderEntry.id (legacy mode)
	std::string file;				  // Path to .hlsl file
	std::string entryPoint;			  // Function name (default "main")
	std::string profile;			  // "vs_6_7", "ps_6_7", etc.
	std::vector<std::string> defines; // Per-shader defines
};

// State block references
struct StateReferences
{
	std::string rasterizer;
	std::string depthStencil;
	std::string blend;
	std::string renderTarget;
};

// Material definition structure
struct MaterialDefinition
{
	std::string id;
	std::string pass;
	std::string vertexFormat; // References VertexFormat.id from MaterialSystem
	std::vector<ShaderReference> shaders;
	std::vector<Parameter> parameters;
	StateReferences states;
	bool enabled = true;
	std::string versionHash;
};

// Helper function to convert ShaderStage to string (for hashing and lookups)
std::string shaderStageToString( ShaderStage stage );

// Helper function to parse string into ShaderStage enum
ShaderStage parseShaderStage( const std::string &stageStr );

// Material parser class
class MaterialParser
{
public:
	// Parse a single material definition from JSON
	// Returns MaterialDefinition if successful, throws or logs fatal error otherwise
	static MaterialDefinition parse( const nlohmann::json &jsonMaterial );

private:
	static ParameterType parseParameterType( const std::string &typeStr );
};

} // namespace graphics::material_system

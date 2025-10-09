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
	std::string stage;	  // "vertex", "pixel", "compute"
	std::string shaderId; // References a ShaderEntry.id
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
	std::vector<ShaderReference> shaders;
	std::vector<Parameter> parameters;
	StateReferences states;
	bool enabled = true;
	std::string versionHash;
};

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

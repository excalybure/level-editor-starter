#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <d3d12.h>

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

// Render pass definition structure
struct RenderPassDefinition
{
	std::string name;
	std::string queue;
	StateReferences states; // Reuse StateReferences for render pass states
};

// Material pass structure (single rendering pass within a material)
struct MaterialPass
{
	std::string passName;															 // Pass identifier (e.g., "forward", "depth_prepass")
	std::vector<ShaderReference> shaders;											 // Shaders specific to this pass
	StateReferences states;															 // State blocks for this pass
	std::vector<Parameter> parameters;												 // Pass-specific parameters
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Topology for this pass
};

// Material definition structure
struct MaterialDefinition
{
	std::string id;
	std::vector<MaterialPass> passes; // Multi-pass support
	std::string vertexFormat;		  // References VertexFormat.id from MaterialSystem
	bool enabled = true;
	std::string versionHash;

	// Query methods
	const MaterialPass *getPass( const std::string &passName ) const;
	bool hasPass( const std::string &passName ) const;
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

	// Parse a single render pass definition from JSON
	static RenderPassDefinition parseRenderPass( const nlohmann::json &jsonRenderPass );

private:
	static ParameterType parseParameterType( const std::string &typeStr );
	static MaterialPass parseMaterialPass( const nlohmann::json &jsonPass, const std::string &materialId );
	static void parseShaders( std::vector<ShaderReference> &outShaders, const nlohmann::json &shadersObj, const std::string &contextId );
	static void parseParameters( std::vector<Parameter> &outParameters, const nlohmann::json &parametersArray, const std::string &contextId );
	static void parseStates( StateReferences &outStates, const nlohmann::json &statesObj );
	static D3D12_PRIMITIVE_TOPOLOGY_TYPE parseTopology( const std::string &topologyStr );
};

} // namespace graphics::material_system

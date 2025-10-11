#pragma once

#include "parser.h"
#include "state_blocks.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace graphics::material_system
{

// Forward declarations
class JsonLoader;

// Opaque handle for material access
struct MaterialHandle
{
	uint32_t index = UINT32_MAX;

	bool isValid() const { return index != UINT32_MAX; }
};

// Main material system API for renderer integration
class MaterialSystem
{
public:
	MaterialSystem() = default;

	// Initialize system from JSON file
	// Returns true on success, false if file not found or validation fails
	bool initialize( const std::string &jsonPath );

	// Query material handle by ID
	// Returns invalid handle if material not found
	MaterialHandle getMaterialHandle( const std::string &materialId ) const;

	// Get material definition by handle
	// Returns nullptr if handle invalid
	const MaterialDefinition *getMaterial( MaterialHandle handle ) const;

	// Query state blocks by ID
	// Returns nullptr if state not found
	const RasterizerStateBlock *getRasterizerState( const std::string &id ) const;
	const DepthStencilStateBlock *getDepthStencilState( const std::string &id ) const;
	const BlendStateBlock *getBlendState( const std::string &id ) const;
	const RenderTargetStateBlock *getRenderTargetState( const std::string &id ) const;

private:
	std::unordered_map<std::string, uint32_t> m_materialIdToIndex;
	std::vector<MaterialDefinition> m_materials;

	// State block storage
	std::unordered_map<std::string, RasterizerStateBlock> m_rasterizerStates;
	std::unordered_map<std::string, DepthStencilStateBlock> m_depthStencilStates;
	std::unordered_map<std::string, BlendStateBlock> m_blendStates;
	std::unordered_map<std::string, RenderTargetStateBlock> m_renderTargetStates;
};

} // namespace graphics::material_system

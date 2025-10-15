#pragma once

#include "parser.h"
#include "state_blocks.h"
#include "shader_reflection.h"
#include "graphics/shader_manager/shader_manager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace graphics::material_system
{

// Forward declarations
class JsonLoader;
struct RenderPassConfig;

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
	~MaterialSystem();

	// Initialize system from JSON file with ShaderManager for reflection-based root signatures
	// Pass nullptr for shaderManager if reflection-based features are not needed
	// Returns true on success, false if file not found or validation fails
	bool initialize( const std::string &jsonPath, shader_manager::ShaderManager *shaderManager );

	// Access shader manager (may be nullptr if not provided during init)
	shader_manager::ShaderManager *getShaderManager() const { return m_shaderManager; }

	// Access shader reflection cache
	ShaderReflectionCache *getReflectionCache() const { return const_cast<ShaderReflectionCache *>( &m_reflectionCache ); }

	// Query material handle by ID
	// Returns invalid handle if material not found
	MaterialHandle getMaterialHandle( const std::string &materialId ) const;

	// Get material definition by handle
	// Returns nullptr if handle invalid
	const MaterialDefinition *getMaterial( MaterialHandle handle ) const;

	// Query specific pass from material by handle and pass name
	// Returns nullptr if handle invalid or pass not found
	const MaterialPass *getMaterialPass( MaterialHandle handle, const std::string &passName ) const;

	// Check if material has specific pass
	// Returns false if handle invalid or pass not found
	bool hasMaterialPass( MaterialHandle handle, const std::string &passName ) const;

	// Query state blocks by ID
	// Returns nullptr if state not found
	const RasterizerStateBlock *getRasterizerState( const std::string &id ) const;
	const DepthStencilStateBlock *getDepthStencilState( const std::string &id ) const;
	const BlendStateBlock *getBlendState( const std::string &id ) const;
	const RenderTargetStateBlock *getRenderTargetState( const std::string &id ) const;
	const VertexFormat *getVertexFormat( const std::string &id ) const;

	// Query render pass by name
	// Returns nullptr if render pass not found
	const RenderPassDefinition *getRenderPass( const std::string &name ) const;

	// Generate RenderPassConfig from render pass definition
	// Queries render pass by name, then queries referenced render target state block
	// Populates RenderPassConfig with rtvFormats, dsvFormat, numRenderTargets from state block
	// Logs console::fatal if render pass or render target state not found
	RenderPassConfig getRenderPassConfig( const std::string &passName ) const;

private:
	// Shader reflection support (optional, may be nullptr)
	shader_manager::ShaderManager *m_shaderManager = nullptr;
	ShaderReflectionCache m_reflectionCache;
	shader_manager::CallbackHandle m_hotReloadCallbackHandle = shader_manager::INVALID_CALLBACK_HANDLE;

	std::unordered_map<std::string, uint32_t> m_materialIdToIndex;
	std::vector<MaterialDefinition> m_materials;

	// Track which materials use which shaders for PSO invalidation on hot-reload
	std::unordered_map<std::string, std::vector<std::string>> m_shaderToMaterials; // shader file path -> materialIds

	// State block storage
	std::unordered_map<std::string, RasterizerStateBlock> m_rasterizerStates;
	std::unordered_map<std::string, DepthStencilStateBlock> m_depthStencilStates;
	std::unordered_map<std::string, BlendStateBlock> m_blendStates;
	std::unordered_map<std::string, RenderTargetStateBlock> m_renderTargetStates;
	std::unordered_map<std::string, VertexFormat> m_vertexFormats;

	// Render pass storage
	std::unordered_map<std::string, RenderPassDefinition> m_renderPasses;
};

} // namespace graphics::material_system

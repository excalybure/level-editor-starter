#pragma once

#include "graphics/material_system/material_system.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <d3d12.h>
#include <wrl/client.h>

// Forward declarations
namespace dx12
{
class Device;
}

namespace shader_manager
{
class ShaderManager;
}

namespace graphics::material_system
{

// Represents a runtime instance of a material with cached GPU resources for all passes
// Handles PSO/root signature lifecycle, hot-reloading, and multi-pass management
// Note: Does NOT manage constant buffers - that's the caller's responsibility
class MaterialInstance
{
public:
	// Create material instance from material ID
	// device: DX12 device for PSO/root signature creation
	// materialSystem: Material system for querying definitions
	// shaderManager: Shader manager for hot-reload (can be nullptr)
	// materialId: Material ID to look up (e.g., "grid_material", "pbr_material")
	MaterialInstance(
		dx12::Device *device,
		MaterialSystem *materialSystem,
		shader_manager::ShaderManager *shaderManager,
		const std::string &materialId );

	~MaterialInstance();

	// No copy (manages GPU resources and callbacks)
	MaterialInstance( const MaterialInstance & ) = delete;
	MaterialInstance &operator=( const MaterialInstance & ) = delete;

	// Check if material instance is valid (material found with at least one pass)
	bool isValid() const;

	// Check if material has specific pass
	bool hasPass( const std::string &passName ) const;

	// Query material definition (nullptr if invalid)
	const MaterialDefinition *getMaterial() const;

	// Query specific material pass (nullptr if not found)
	const MaterialPass *getPass( const std::string &passName ) const;

	// Get material handle
	MaterialHandle getHandle() const { return m_materialHandle; }

	// Get root signature (shared across all passes)
	// Returns nullptr if root signature not created or material invalid
	ID3D12RootSignature *getRootSignature() const;

	// Get or create pipeline state for specific pass (lazy creation with caching)
	// Returns nullptr if pass doesn't exist or PSO creation fails
	ID3D12PipelineState *getPipelineState( const std::string &passName );

	// Setup command list with PSO and root signature for specific pass
	// Returns true on success, false if command list nullptr, pass invalid, or resources unavailable
	bool setupCommandList( ID3D12GraphicsCommandList *commandList, const std::string &passName );

private:
	// Create pipeline state for specific pass using PipelineBuilder
	// Returns true on success, false on failure
	bool createPipelineStateForPass( const std::string &passName );

	// Hot-reload callback - marks all passes dirty and clears PSO cache
	void onShaderReloaded();

	dx12::Device *m_device = nullptr;
	MaterialSystem *m_materialSystem = nullptr;
	shader_manager::ShaderManager *m_shaderManager = nullptr;
	MaterialHandle m_materialHandle;
	size_t m_hotReloadCallbackHandle = 0;

	// Single root signature shared by all passes
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	// Per-pass PSO cache (lazy creation)
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pipelineStates;

	// Dirty flags for PSO recreation (e.g., after hot-reload)
	std::unordered_set<std::string> m_dirtyPasses;
};

} // namespace graphics::material_system

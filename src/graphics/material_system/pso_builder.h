#pragma once

#include "graphics/material_system/parser.h"
#include "graphics/material_system/cache.h"
#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

// Forward declaration
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

// Forward declarations
class MaterialSystem;
class ShaderReflectionCache;

// Render pass configuration for PSO creation
struct RenderPassConfig
{
	std::string name;
	DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	UINT numRenderTargets = 0;
};

// Pipeline State Object builder
class PSOBuilder
{
public:
	// Build PSO from material definition and render pass configuration using shader reflection
	// Returns ID3D12PipelineState on success, nullptr on failure
	// Automatically caches PSOs and reuses them for identical requests
	// @param device - D3D12 device
	// @param material - Material definition with passes
	// @param passConfig - Render pass configuration (formats, sample count)
	// @param materialSystem - Material system for querying state blocks (required)
	// @param passName - Name of the pass to build PSO for (required for multi-pass materials)
	// @param shaderManager - ShaderManager for retrieving compiled shader blobs (required)
	// @param reflectionCache - Cache for shader reflection results (required)
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> build(
		dx12::Device *device,
		const MaterialDefinition &material,
		const RenderPassConfig &passConfig,
		const MaterialSystem *materialSystem,
		const std::string &passName,
		shader_manager::ShaderManager *shaderManager,
		ShaderReflectionCache *reflectionCache );

	// Get or create root signature for a material pass using shader reflection
	// Uses shared cache for efficient reuse across materials
	// @param device - D3D12 device
	// @param material - Material definition with passes
	// @param shaderManager - ShaderManager for retrieving compiled shader blobs (required)
	// @param reflectionCache - Cache for shader reflection results (required)
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> getRootSignature(
		dx12::Device *device,
		const MaterialDefinition &material,
		shader_manager::ShaderManager *shaderManager,
		ShaderReflectionCache *reflectionCache );

	// Clear the PSO cache (useful for hot-reloading)
	static void clearCache();

private:
	static PipelineCache s_cache;
};

} // namespace graphics::material_system

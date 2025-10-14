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
	// Build PSO from material definition and render pass configuration
	// Returns ID3D12PipelineState on success, nullptr on failure
	// Automatically caches PSOs and reuses them for identical requests
	// MaterialSystem pointer is optional - if provided, state blocks will be queried; if nullptr, uses D3D12 defaults
	// passName: Specific pass to build PSO for (multi-pass materials); empty string uses legacy format
	// shaderManager, reflectionCache: Optional for reflection-based root signatures; if nullptr, uses legacy parameter-based root signature generation
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> build(
		dx12::Device *device,
		const MaterialDefinition &material,
		const RenderPassConfig &passConfig,
		const MaterialSystem *materialSystem = nullptr,
		const std::string &passName = "",
		shader_manager::ShaderManager *shaderManager = nullptr,
		ShaderReflectionCache *reflectionCache = nullptr );

	// Get or create root signature for a material
	// Uses shared cache for efficient reuse across materials
	// shaderManager, reflectionCache: Optional for reflection-based root signatures; if nullptr, uses legacy parameter-based generation
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> getRootSignature(
		dx12::Device *device,
		const MaterialDefinition &material,
		shader_manager::ShaderManager *shaderManager = nullptr,
		ShaderReflectionCache *reflectionCache = nullptr );

	// Clear the PSO cache (useful for hot-reloading)
	static void clearCache();

private:
	static PipelineCache s_cache;
};

} // namespace graphics::material_system

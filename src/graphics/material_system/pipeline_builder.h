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

namespace graphics::material_system
{

// Render pass configuration for PSO creation
struct RenderPassConfig
{
	std::string name;
	DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	UINT numRenderTargets = 0;
};

// Pipeline State Object builder
class PipelineBuilder
{
public:
	// Build PSO from material definition and render pass configuration
	// Returns ID3D12PipelineState on success, nullptr on failure
	// Automatically caches PSOs and reuses them for identical requests
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> buildPSO(
		dx12::Device *device,
		const MaterialDefinition &material,
		const RenderPassConfig &passConfig );

private:
	static PipelineCache s_cache;
};

} // namespace graphics::material_system

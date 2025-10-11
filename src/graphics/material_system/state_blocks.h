#pragma once

#include <string>
#include <vector>
#include <array>
#include <d3d12.h>
#include <dxgiformat.h>

namespace graphics::material_system
{

// Rasterizer state block
struct RasterizerStateBlock
{
	std::string id;
	std::string base; // Optional inheritance

	D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
	BOOL frontCounterClockwise = FALSE;
	INT depthBias = D3D12_DEFAULT_DEPTH_BIAS;
	FLOAT depthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	FLOAT slopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	BOOL depthClipEnable = TRUE;
	BOOL multisampleEnable = FALSE;
	BOOL antialiasedLineEnable = FALSE;
	UINT forcedSampleCount = 0;
	D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
};

// Depth stencil operation descriptor
struct DepthStencilOpDesc
{
	D3D12_STENCIL_OP stencilFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP stencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP stencilPassOp = D3D12_STENCIL_OP_KEEP;
	D3D12_COMPARISON_FUNC stencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Conversion to D3D12 descriptor
	D3D12_DEPTH_STENCILOP_DESC toD3D12() const
	{
		return { stencilFailOp, stencilDepthFailOp, stencilPassOp, stencilFunc };
	}
};

// Depth stencil state block
struct DepthStencilStateBlock
{
	std::string id;
	std::string base; // Optional inheritance

	BOOL depthEnable = TRUE;
	D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
	BOOL stencilEnable = FALSE;
	UINT8 stencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	UINT8 stencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	DepthStencilOpDesc frontFace;
	DepthStencilOpDesc backFace;
};

// Blend render target state
struct BlendRenderTargetState
{
	BOOL blendEnable = FALSE;
	BOOL logicOpEnable = FALSE;
	D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
	D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
	D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;
	D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE;
	D3D12_BLEND destBlendAlpha = D3D12_BLEND_ZERO;
	D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD;
	D3D12_LOGIC_OP logicOp = D3D12_LOGIC_OP_NOOP;
	UINT8 renderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Conversion to D3D12 descriptor
	D3D12_RENDER_TARGET_BLEND_DESC toD3D12() const
	{
		D3D12_RENDER_TARGET_BLEND_DESC desc = {};
		desc.BlendEnable = blendEnable;
		desc.LogicOpEnable = logicOpEnable;
		desc.SrcBlend = srcBlend;
		desc.DestBlend = destBlend;
		desc.BlendOp = blendOp;
		desc.SrcBlendAlpha = srcBlendAlpha;
		desc.DestBlendAlpha = destBlendAlpha;
		desc.BlendOpAlpha = blendOpAlpha;
		desc.LogicOp = logicOp;
		desc.RenderTargetWriteMask = renderTargetWriteMask;
		return desc;
	}
};

// Blend state block
struct BlendStateBlock
{
	std::string id;
	std::string base; // Optional inheritance

	BOOL alphaToCoverageEnable = FALSE;
	BOOL independentBlendEnable = FALSE;
	std::array<BlendRenderTargetState, 8> renderTargets;
};

// Render target state block
struct RenderTargetStateBlock
{
	std::string id;
	std::vector<DXGI_FORMAT> rtvFormats;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	UINT sampleCount = 1;
	UINT sampleQuality = 0;
};

// Vertex element (input layout element)
struct VertexElement
{
	std::string semantic; // "POSITION", "NORMAL", "TEXCOORD", etc.
	UINT semanticIndex = 0;
	DXGI_FORMAT format;
	UINT inputSlot = 0;
	UINT alignedByteOffset;
	D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	UINT instanceDataStepRate = 0;
};

// Vertex format (input layout specification)
struct VertexFormat
{
	std::string id;
	std::vector<VertexElement> elements;
	UINT stride;
};

} // namespace graphics::material_system

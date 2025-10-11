#pragma once

#include "state_blocks.h"
#include <nlohmann/json.hpp>
#include <string>

namespace graphics::material_system
{

class StateBlockParser
{
public:
	// Parse individual state blocks
	static RasterizerStateBlock parseRasterizer( const nlohmann::json &j );
	static DepthStencilStateBlock parseDepthStencil( const nlohmann::json &j );
	static BlendStateBlock parseBlend( const nlohmann::json &j );
	static RenderTargetStateBlock parseRenderTarget( const nlohmann::json &j );
	static VertexFormat parseVertexFormat( const nlohmann::json &j );

	// Parse enum strings to D3D12 enums
	static D3D12_FILL_MODE parseFillMode( const std::string &str );
	static D3D12_CULL_MODE parseCullMode( const std::string &str );
	static D3D12_COMPARISON_FUNC parseComparisonFunc( const std::string &str );
	static D3D12_BLEND parseBlendFactor( const std::string &str );
	static D3D12_BLEND_OP parseBlendOp( const std::string &str );
	static D3D12_LOGIC_OP parseLogicOp( const std::string &str );
	static D3D12_STENCIL_OP parseStencilOp( const std::string &str );
	static D3D12_DEPTH_WRITE_MASK parseDepthWriteMask( const std::string &str );
	static UINT8 parseColorWriteMask( const std::string &str );
	static DXGI_FORMAT parseFormat( const std::string &str );
};

} // namespace graphics::material_system

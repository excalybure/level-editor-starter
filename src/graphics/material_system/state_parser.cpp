#include "state_parser.h"
#include "core/console.h"
#include <unordered_map>

namespace graphics::material_system
{

D3D12_FILL_MODE StateBlockParser::parseFillMode( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_FILL_MODE> mapping = {
		{ "Solid", D3D12_FILL_MODE_SOLID },
		{ "Wireframe", D3D12_FILL_MODE_WIREFRAME }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid FillMode value: '{}'. Must be 'Solid' or 'Wireframe'.", str );
		return D3D12_FILL_MODE_SOLID; // Unreachable, but silences compiler warning
	}

	return it->second;
}

D3D12_CULL_MODE StateBlockParser::parseCullMode( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_CULL_MODE> mapping = {
		{ "None", D3D12_CULL_MODE_NONE },
		{ "Front", D3D12_CULL_MODE_FRONT },
		{ "Back", D3D12_CULL_MODE_BACK }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid CullMode value: '{}'. Must be 'None', 'Front', or 'Back'.", str );
		return D3D12_CULL_MODE_NONE;
	}

	return it->second;
}

D3D12_COMPARISON_FUNC StateBlockParser::parseComparisonFunc( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_COMPARISON_FUNC> mapping = {
		{ "Never", D3D12_COMPARISON_FUNC_NEVER },
		{ "Less", D3D12_COMPARISON_FUNC_LESS },
		{ "Equal", D3D12_COMPARISON_FUNC_EQUAL },
		{ "LessEqual", D3D12_COMPARISON_FUNC_LESS_EQUAL },
		{ "Greater", D3D12_COMPARISON_FUNC_GREATER },
		{ "NotEqual", D3D12_COMPARISON_FUNC_NOT_EQUAL },
		{ "GreaterEqual", D3D12_COMPARISON_FUNC_GREATER_EQUAL },
		{ "Always", D3D12_COMPARISON_FUNC_ALWAYS }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid ComparisonFunc value: '{}'. Must be one of: Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always.", str );
		return D3D12_COMPARISON_FUNC_NEVER;
	}

	return it->second;
}

D3D12_BLEND StateBlockParser::parseBlendFactor( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_BLEND> mapping = {
		{ "Zero", D3D12_BLEND_ZERO },
		{ "One", D3D12_BLEND_ONE },
		{ "SrcColor", D3D12_BLEND_SRC_COLOR },
		{ "InvSrcColor", D3D12_BLEND_INV_SRC_COLOR },
		{ "SrcAlpha", D3D12_BLEND_SRC_ALPHA },
		{ "InvSrcAlpha", D3D12_BLEND_INV_SRC_ALPHA },
		{ "DestAlpha", D3D12_BLEND_DEST_ALPHA },
		{ "InvDestAlpha", D3D12_BLEND_INV_DEST_ALPHA },
		{ "DestColor", D3D12_BLEND_DEST_COLOR },
		{ "InvDestColor", D3D12_BLEND_INV_DEST_COLOR },
		{ "SrcAlphaSat", D3D12_BLEND_SRC_ALPHA_SAT },
		{ "BlendFactor", D3D12_BLEND_BLEND_FACTOR },
		{ "InvBlendFactor", D3D12_BLEND_INV_BLEND_FACTOR },
		{ "Src1Color", D3D12_BLEND_SRC1_COLOR },
		{ "InvSrc1Color", D3D12_BLEND_INV_SRC1_COLOR },
		{ "Src1Alpha", D3D12_BLEND_SRC1_ALPHA },
		{ "InvSrc1Alpha", D3D12_BLEND_INV_SRC1_ALPHA }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid Blend factor value: '{}'. Must be a valid D3D12_BLEND value.", str );
		return D3D12_BLEND_ZERO;
	}

	return it->second;
}

D3D12_BLEND_OP StateBlockParser::parseBlendOp( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_BLEND_OP> mapping = {
		{ "Add", D3D12_BLEND_OP_ADD },
		{ "Subtract", D3D12_BLEND_OP_SUBTRACT },
		{ "RevSubtract", D3D12_BLEND_OP_REV_SUBTRACT },
		{ "Min", D3D12_BLEND_OP_MIN },
		{ "Max", D3D12_BLEND_OP_MAX }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid BlendOp value: '{}'. Must be one of: Add, Subtract, RevSubtract, Min, Max.", str );
		return D3D12_BLEND_OP_ADD;
	}

	return it->second;
}

D3D12_LOGIC_OP StateBlockParser::parseLogicOp( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_LOGIC_OP> mapping = {
		{ "Clear", D3D12_LOGIC_OP_CLEAR },
		{ "Set", D3D12_LOGIC_OP_SET },
		{ "Copy", D3D12_LOGIC_OP_COPY },
		{ "CopyInverted", D3D12_LOGIC_OP_COPY_INVERTED },
		{ "Noop", D3D12_LOGIC_OP_NOOP },
		{ "Invert", D3D12_LOGIC_OP_INVERT },
		{ "And", D3D12_LOGIC_OP_AND },
		{ "Nand", D3D12_LOGIC_OP_NAND },
		{ "Or", D3D12_LOGIC_OP_OR },
		{ "Nor", D3D12_LOGIC_OP_NOR },
		{ "Xor", D3D12_LOGIC_OP_XOR },
		{ "Equiv", D3D12_LOGIC_OP_EQUIV },
		{ "AndReverse", D3D12_LOGIC_OP_AND_REVERSE },
		{ "AndInverted", D3D12_LOGIC_OP_AND_INVERTED },
		{ "OrReverse", D3D12_LOGIC_OP_OR_REVERSE },
		{ "OrInverted", D3D12_LOGIC_OP_OR_INVERTED }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid LogicOp value: '{}'.", str );
		return D3D12_LOGIC_OP_NOOP;
	}

	return it->second;
}

D3D12_STENCIL_OP StateBlockParser::parseStencilOp( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_STENCIL_OP> mapping = {
		{ "Keep", D3D12_STENCIL_OP_KEEP },
		{ "Zero", D3D12_STENCIL_OP_ZERO },
		{ "Replace", D3D12_STENCIL_OP_REPLACE },
		{ "IncrSat", D3D12_STENCIL_OP_INCR_SAT },
		{ "DecrSat", D3D12_STENCIL_OP_DECR_SAT },
		{ "Invert", D3D12_STENCIL_OP_INVERT },
		{ "Incr", D3D12_STENCIL_OP_INCR },
		{ "Decr", D3D12_STENCIL_OP_DECR }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid StencilOp value: '{}'. Must be one of: Keep, Zero, Replace, IncrSat, DecrSat, Invert, Incr, Decr.", str );
		return D3D12_STENCIL_OP_KEEP;
	}

	return it->second;
}

D3D12_DEPTH_WRITE_MASK StateBlockParser::parseDepthWriteMask( const std::string &str )
{
	static const std::unordered_map<std::string, D3D12_DEPTH_WRITE_MASK> mapping = {
		{ "Zero", D3D12_DEPTH_WRITE_MASK_ZERO },
		{ "All", D3D12_DEPTH_WRITE_MASK_ALL }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid DepthWriteMask value: '{}'. Must be 'Zero' or 'All'.", str );
		return D3D12_DEPTH_WRITE_MASK_ZERO;
	}

	return it->second;
}

UINT8 StateBlockParser::parseColorWriteMask( const std::string &str )
{
	static const std::unordered_map<std::string, UINT8> mapping = {
		{ "Red", D3D12_COLOR_WRITE_ENABLE_RED },
		{ "Green", D3D12_COLOR_WRITE_ENABLE_GREEN },
		{ "Blue", D3D12_COLOR_WRITE_ENABLE_BLUE },
		{ "Alpha", D3D12_COLOR_WRITE_ENABLE_ALPHA },
		{ "All", D3D12_COLOR_WRITE_ENABLE_ALL }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid ColorWriteMask value: '{}'. Must be one of: Red, Green, Blue, Alpha, All.", str );
		return D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return it->second;
}

DXGI_FORMAT StateBlockParser::parseFormat( const std::string &str )
{
	// Common render target, depth, and vertex formats
	static const std::unordered_map<std::string, DXGI_FORMAT> mapping = {
		// Common RT formats
		{ "R8G8B8A8_UNORM", DXGI_FORMAT_R8G8B8A8_UNORM },
		{ "R8G8B8A8_UNORM_SRGB", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
		{ "R16G16B16A16_FLOAT", DXGI_FORMAT_R16G16B16A16_FLOAT },
		{ "R32G32B32A32_FLOAT", DXGI_FORMAT_R32G32B32A32_FLOAT },
		{ "R10G10B10A2_UNORM", DXGI_FORMAT_R10G10B10A2_UNORM },
		{ "R11G11B10_FLOAT", DXGI_FORMAT_R11G11B10_FLOAT },

		// Depth formats
		{ "D32_FLOAT", DXGI_FORMAT_D32_FLOAT },
		{ "D24_UNORM_S8_UINT", DXGI_FORMAT_D24_UNORM_S8_UINT },
		{ "D16_UNORM", DXGI_FORMAT_D16_UNORM },

		// Vertex formats (POSITION, NORMAL, TEXCOORD, etc.)
		{ "R32G32B32_FLOAT", DXGI_FORMAT_R32G32B32_FLOAT },
		{ "R32G32_FLOAT", DXGI_FORMAT_R32G32_FLOAT },
		{ "R32_FLOAT", DXGI_FORMAT_R32_FLOAT },

		// Special
		{ "UNKNOWN", DXGI_FORMAT_UNKNOWN }
	};

	const auto it = mapping.find( str );
	if ( it == mapping.end() )
	{
		console::errorAndThrow( "Invalid DXGI_FORMAT value: '{}'. Format not supported in parser.", str );
		return DXGI_FORMAT_UNKNOWN;
	}

	return it->second;
}

RasterizerStateBlock StateBlockParser::parseRasterizer( const nlohmann::json &j )
{
	RasterizerStateBlock state;

	// Extract id (required)
	if ( j.contains( "id" ) && j["id"].is_string() )
	{
		state.id = j["id"].get<std::string>();
	}

	// Extract base (optional)
	if ( j.contains( "base" ) && j["base"].is_string() )
	{
		state.base = j["base"].get<std::string>();
	}

	// Extract fillMode (optional, defaults set in struct)
	if ( j.contains( "fillMode" ) && j["fillMode"].is_string() )
	{
		state.fillMode = parseFillMode( j["fillMode"].get<std::string>() );
	}

	// Extract cullMode (optional)
	if ( j.contains( "cullMode" ) && j["cullMode"].is_string() )
	{
		state.cullMode = parseCullMode( j["cullMode"].get<std::string>() );
	}

	// Extract frontCounterClockwise (optional)
	if ( j.contains( "frontCounterClockwise" ) && j["frontCounterClockwise"].is_boolean() )
	{
		state.frontCounterClockwise = j["frontCounterClockwise"].get<bool>() ? TRUE : FALSE;
	}

	// Extract depthBias (optional)
	if ( j.contains( "depthBias" ) && j["depthBias"].is_number_integer() )
	{
		state.depthBias = j["depthBias"].get<INT>();
	}

	// Extract depthBiasClamp (optional)
	if ( j.contains( "depthBiasClamp" ) && j["depthBiasClamp"].is_number() )
	{
		state.depthBiasClamp = j["depthBiasClamp"].get<FLOAT>();
	}

	// Extract slopeScaledDepthBias (optional)
	if ( j.contains( "slopeScaledDepthBias" ) && j["slopeScaledDepthBias"].is_number() )
	{
		state.slopeScaledDepthBias = j["slopeScaledDepthBias"].get<FLOAT>();
	}

	// Extract depthClipEnable (optional)
	if ( j.contains( "depthClipEnable" ) && j["depthClipEnable"].is_boolean() )
	{
		state.depthClipEnable = j["depthClipEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract multisampleEnable (optional)
	if ( j.contains( "multisampleEnable" ) && j["multisampleEnable"].is_boolean() )
	{
		state.multisampleEnable = j["multisampleEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract antialiasedLineEnable (optional)
	if ( j.contains( "antialiasedLineEnable" ) && j["antialiasedLineEnable"].is_boolean() )
	{
		state.antialiasedLineEnable = j["antialiasedLineEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract forcedSampleCount (optional)
	if ( j.contains( "forcedSampleCount" ) && j["forcedSampleCount"].is_number_unsigned() )
	{
		state.forcedSampleCount = j["forcedSampleCount"].get<UINT>();
	}

	// Extract conservativeRaster (optional)
	if ( j.contains( "conservativeRaster" ) && j["conservativeRaster"].is_boolean() )
	{
		const bool enabled = j["conservativeRaster"].get<bool>();
		state.conservativeRaster = enabled ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	return state;
}

DepthStencilStateBlock StateBlockParser::parseDepthStencil( const nlohmann::json &j )
{
	DepthStencilStateBlock state;

	// Extract id (required)
	if ( j.contains( "id" ) && j["id"].is_string() )
	{
		state.id = j["id"].get<std::string>();
	}

	// Extract base (optional)
	if ( j.contains( "base" ) && j["base"].is_string() )
	{
		state.base = j["base"].get<std::string>();
	}

	// Extract depthEnable (optional)
	if ( j.contains( "depthEnable" ) && j["depthEnable"].is_boolean() )
	{
		state.depthEnable = j["depthEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract depthWriteMask (optional)
	if ( j.contains( "depthWriteMask" ) && j["depthWriteMask"].is_string() )
	{
		state.depthWriteMask = parseDepthWriteMask( j["depthWriteMask"].get<std::string>() );
	}

	// Extract depthFunc (optional)
	if ( j.contains( "depthFunc" ) && j["depthFunc"].is_string() )
	{
		state.depthFunc = parseComparisonFunc( j["depthFunc"].get<std::string>() );
	}

	// Extract stencilEnable (optional)
	if ( j.contains( "stencilEnable" ) && j["stencilEnable"].is_boolean() )
	{
		state.stencilEnable = j["stencilEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract stencilReadMask (optional)
	if ( j.contains( "stencilReadMask" ) && j["stencilReadMask"].is_number_unsigned() )
	{
		state.stencilReadMask = static_cast<UINT8>( j["stencilReadMask"].get<unsigned int>() );
	}

	// Extract stencilWriteMask (optional)
	if ( j.contains( "stencilWriteMask" ) && j["stencilWriteMask"].is_number_unsigned() )
	{
		state.stencilWriteMask = static_cast<UINT8>( j["stencilWriteMask"].get<unsigned int>() );
	}

	// Extract frontFace stencil ops (optional)
	if ( j.contains( "frontFace" ) && j["frontFace"].is_object() )
	{
		const auto &frontFace = j["frontFace"];
		if ( frontFace.contains( "stencilFailOp" ) && frontFace["stencilFailOp"].is_string() )
		{
			state.frontFace.stencilFailOp = parseStencilOp( frontFace["stencilFailOp"].get<std::string>() );
		}
		if ( frontFace.contains( "stencilDepthFailOp" ) && frontFace["stencilDepthFailOp"].is_string() )
		{
			state.frontFace.stencilDepthFailOp = parseStencilOp( frontFace["stencilDepthFailOp"].get<std::string>() );
		}
		if ( frontFace.contains( "stencilPassOp" ) && frontFace["stencilPassOp"].is_string() )
		{
			state.frontFace.stencilPassOp = parseStencilOp( frontFace["stencilPassOp"].get<std::string>() );
		}
		if ( frontFace.contains( "stencilFunc" ) && frontFace["stencilFunc"].is_string() )
		{
			state.frontFace.stencilFunc = parseComparisonFunc( frontFace["stencilFunc"].get<std::string>() );
		}
	}

	// Extract backFace stencil ops (optional)
	if ( j.contains( "backFace" ) && j["backFace"].is_object() )
	{
		const auto &backFace = j["backFace"];
		if ( backFace.contains( "stencilFailOp" ) && backFace["stencilFailOp"].is_string() )
		{
			state.backFace.stencilFailOp = parseStencilOp( backFace["stencilFailOp"].get<std::string>() );
		}
		if ( backFace.contains( "stencilDepthFailOp" ) && backFace["stencilDepthFailOp"].is_string() )
		{
			state.backFace.stencilDepthFailOp = parseStencilOp( backFace["stencilDepthFailOp"].get<std::string>() );
		}
		if ( backFace.contains( "stencilPassOp" ) && backFace["stencilPassOp"].is_string() )
		{
			state.backFace.stencilPassOp = parseStencilOp( backFace["stencilPassOp"].get<std::string>() );
		}
		if ( backFace.contains( "stencilFunc" ) && backFace["stencilFunc"].is_string() )
		{
			state.backFace.stencilFunc = parseComparisonFunc( backFace["stencilFunc"].get<std::string>() );
		}
	}

	return state;
}

BlendStateBlock StateBlockParser::parseBlend( const nlohmann::json &j )
{
	BlendStateBlock state;

	// Extract id (required)
	if ( j.contains( "id" ) && j["id"].is_string() )
	{
		state.id = j["id"].get<std::string>();
	}

	// Extract base (optional)
	if ( j.contains( "base" ) && j["base"].is_string() )
	{
		state.base = j["base"].get<std::string>();
	}

	// Extract alphaToCoverageEnable (optional)
	if ( j.contains( "alphaToCoverageEnable" ) && j["alphaToCoverageEnable"].is_boolean() )
	{
		state.alphaToCoverageEnable = j["alphaToCoverageEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract independentBlendEnable (optional)
	if ( j.contains( "independentBlendEnable" ) && j["independentBlendEnable"].is_boolean() )
	{
		state.independentBlendEnable = j["independentBlendEnable"].get<bool>() ? TRUE : FALSE;
	}

	// Extract renderTargets array (optional)
	if ( j.contains( "renderTargets" ) && j["renderTargets"].is_array() )
	{
		const auto &rtArray = j["renderTargets"];
		const size_t numRTs = std::min( rtArray.size(), state.renderTargets.size() );

		for ( size_t i = 0; i < numRTs; ++i )
		{
			const auto &rtJson = rtArray[i];
			auto &rt = state.renderTargets[i];

			// Extract blendEnable (optional)
			if ( rtJson.contains( "blendEnable" ) && rtJson["blendEnable"].is_boolean() )
			{
				rt.blendEnable = rtJson["blendEnable"].get<bool>() ? TRUE : FALSE;
			}

			// Extract logicOpEnable (optional)
			if ( rtJson.contains( "logicOpEnable" ) && rtJson["logicOpEnable"].is_boolean() )
			{
				rt.logicOpEnable = rtJson["logicOpEnable"].get<bool>() ? TRUE : FALSE;
			}

			// Extract srcBlend (optional)
			if ( rtJson.contains( "srcBlend" ) && rtJson["srcBlend"].is_string() )
			{
				rt.srcBlend = parseBlendFactor( rtJson["srcBlend"].get<std::string>() );
			}

			// Extract destBlend (optional)
			if ( rtJson.contains( "destBlend" ) && rtJson["destBlend"].is_string() )
			{
				rt.destBlend = parseBlendFactor( rtJson["destBlend"].get<std::string>() );
			}

			// Extract blendOp (optional)
			if ( rtJson.contains( "blendOp" ) && rtJson["blendOp"].is_string() )
			{
				rt.blendOp = parseBlendOp( rtJson["blendOp"].get<std::string>() );
			}

			// Extract srcBlendAlpha (optional)
			if ( rtJson.contains( "srcBlendAlpha" ) && rtJson["srcBlendAlpha"].is_string() )
			{
				rt.srcBlendAlpha = parseBlendFactor( rtJson["srcBlendAlpha"].get<std::string>() );
			}

			// Extract destBlendAlpha (optional)
			if ( rtJson.contains( "destBlendAlpha" ) && rtJson["destBlendAlpha"].is_string() )
			{
				rt.destBlendAlpha = parseBlendFactor( rtJson["destBlendAlpha"].get<std::string>() );
			}

			// Extract blendOpAlpha (optional)
			if ( rtJson.contains( "blendOpAlpha" ) && rtJson["blendOpAlpha"].is_string() )
			{
				rt.blendOpAlpha = parseBlendOp( rtJson["blendOpAlpha"].get<std::string>() );
			}

			// Extract logicOp (optional)
			if ( rtJson.contains( "logicOp" ) && rtJson["logicOp"].is_string() )
			{
				rt.logicOp = parseLogicOp( rtJson["logicOp"].get<std::string>() );
			}

			// Extract renderTargetWriteMask (optional)
			if ( rtJson.contains( "renderTargetWriteMask" ) && rtJson["renderTargetWriteMask"].is_string() )
			{
				rt.renderTargetWriteMask = parseColorWriteMask( rtJson["renderTargetWriteMask"].get<std::string>() );
			}
		}
	}

	return state;
}

RenderTargetStateBlock StateBlockParser::parseRenderTarget( const nlohmann::json &j )
{
	RenderTargetStateBlock state;

	// Extract id (required)
	if ( j.contains( "id" ) && j["id"].is_string() )
	{
		state.id = j["id"].get<std::string>();
	}

	// Extract rtvFormats array (optional)
	if ( j.contains( "rtvFormats" ) && j["rtvFormats"].is_array() )
	{
		const auto &formatsArray = j["rtvFormats"];
		for ( const auto &formatJson : formatsArray )
		{
			if ( formatJson.is_string() )
			{
				state.rtvFormats.push_back( parseFormat( formatJson.get<std::string>() ) );
			}
		}
	}

	// Extract dsvFormat (optional)
	if ( j.contains( "dsvFormat" ) && j["dsvFormat"].is_string() )
	{
		state.dsvFormat = parseFormat( j["dsvFormat"].get<std::string>() );
	}

	// Extract sampleCount (optional)
	if ( j.contains( "sampleCount" ) && j["sampleCount"].is_number_unsigned() )
	{
		state.sampleCount = j["sampleCount"].get<UINT>();
	}

	// Extract sampleQuality (optional)
	if ( j.contains( "sampleQuality" ) && j["sampleQuality"].is_number_unsigned() )
	{
		state.sampleQuality = j["sampleQuality"].get<UINT>();
	}

	return state;
}

VertexFormat StateBlockParser::parseVertexFormat( const nlohmann::json &j )
{
	VertexFormat format;

	// Extract id (required)
	if ( j.contains( "id" ) && j["id"].is_string() )
	{
		format.id = j["id"].get<std::string>();
	}

	// Extract stride (required)
	if ( j.contains( "stride" ) && j["stride"].is_number_unsigned() )
	{
		format.stride = j["stride"].get<UINT>();
	}

	// Extract elements array (required)
	if ( j.contains( "elements" ) && j["elements"].is_array() )
	{
		const auto &elementsArray = j["elements"];
		for ( const auto &elemJson : elementsArray )
		{
			VertexElement element;

			// semantic (required)
			if ( elemJson.contains( "semantic" ) && elemJson["semantic"].is_string() )
			{
				element.semantic = elemJson["semantic"].get<std::string>();
			}

			// format (required)
			if ( elemJson.contains( "format" ) && elemJson["format"].is_string() )
			{
				element.format = parseFormat( elemJson["format"].get<std::string>() );
			}

			// offset (required) - stored in alignedByteOffset
			if ( elemJson.contains( "offset" ) && elemJson["offset"].is_number_unsigned() )
			{
				element.alignedByteOffset = elemJson["offset"].get<UINT>();
			}

			// Optional fields - semanticIndex, inputSlot, inputSlotClass, instanceDataStepRate use struct defaults

			format.elements.push_back( element );
		}
	}

	return format;
}

} // namespace graphics::material_system

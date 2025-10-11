#include "pipeline_builder.h"
#include "material_system.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/material_system/shader_compiler.h"
#include "graphics/material_system/root_signature_builder.h"
#include "graphics/material_system/root_signature_cache.h"
#include "core/console.h"
#include <d3d12.h>
#include <filesystem>

namespace graphics::material_system
{

// Static cache instances
PipelineCache PipelineBuilder::s_cache;
static RootSignatureCache s_rootSignatureCache;

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineBuilder::buildPSO(
	dx12::Device *device,
	const MaterialDefinition &material,
	const RenderPassConfig &passConfig,
	const MaterialSystem *materialSystem )
{
	if ( !device || !device->get() )
	{
		console::error( "PipelineBuilder::buildPSO: invalid device" );
		return nullptr;
	}

	// Check cache first
	const PSOHash hash = computePSOHash( material, passConfig );
	auto cachedPSO = s_cache.get( hash );
	if ( cachedPSO )
	{
		return cachedPSO;
	}

	// Compile shaders using MaterialShaderCompiler (T012)
	// T203: Use shader info from material.shaders instead of hardcoded simple.hlsl

	// Storage for compiled shader blobs (must persist until PSO creation)
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> dsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> hsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> gsBlob;

	// Compile each shader from material definition
	for ( const auto &shaderRef : material.shaders )
	{
		// Convert shader defines vector to map (T203-AF4)
		// Shader defines are simple flags (no values), so use empty string as value
		std::unordered_map<std::string, std::string> shaderDefines;
		for ( const auto &define : shaderRef.defines )
		{
			shaderDefines[define] = ""; // Flag-style define (e.g., #define IS_PREPASS)
		}

		const auto compiledBlob = MaterialShaderCompiler::CompileWithDefines(
			shaderRef.file, shaderRef.entryPoint, shaderRef.profile, shaderDefines );

		if ( !compiledBlob.isValid() || !compiledBlob.blob )
		{
			console::error( "Failed to compile {} shader from '{}' for material '{}'",
				shaderStageToString( shaderRef.stage ),
				shaderRef.file,
				material.id );
			return nullptr;
		}

		// Store compiled blob based on shader stage
		switch ( shaderRef.stage )
		{
		case ShaderStage::Vertex:
			vsBlob = compiledBlob.blob;
			break;
		case ShaderStage::Pixel:
			psBlob = compiledBlob.blob;
			break;
		case ShaderStage::Domain:
			dsBlob = compiledBlob.blob;
			break;
		case ShaderStage::Hull:
			hsBlob = compiledBlob.blob;
			break;
		case ShaderStage::Geometry:
			gsBlob = compiledBlob.blob;
			break;
		case ShaderStage::Compute:
			console::error( "Compute shaders not supported in graphics pipeline for material '{}'", material.id );
			return nullptr;
		}
	}

	// Validate required shader stages for graphics PSO (T203-AF5)
	if ( !vsBlob )
	{
		console::fatal( "Material '{}' missing required Vertex shader for graphics pipeline", material.id );
	}

	// Build pipeline state descriptor
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// Shaders - populate from compiled bytecode (only if present)
	if ( vsBlob )
	{
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	}
	if ( psBlob )
	{
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	}
	if ( dsBlob )
	{
		psoDesc.DS = { dsBlob->GetBufferPointer(), dsBlob->GetBufferSize() };
	}
	if ( hsBlob )
	{
		psoDesc.HS = { hsBlob->GetBufferPointer(), hsBlob->GetBufferSize() };
	}
	if ( gsBlob )
	{
		psoDesc.GS = { gsBlob->GetBufferPointer(), gsBlob->GetBufferSize() };
	}

	// Input layout - use vertex format from material if specified
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
	if ( !material.vertexFormat.empty() && materialSystem )
	{
		const auto *vertexFormat = materialSystem->getVertexFormat( material.vertexFormat );
		if ( vertexFormat )
		{
			inputLayout.reserve( vertexFormat->elements.size() );
			for ( const auto &elem : vertexFormat->elements )
			{
				inputLayout.push_back( { elem.semantic.c_str(),
					elem.semanticIndex,
					elem.format,
					elem.inputSlot,
					elem.alignedByteOffset,
					elem.inputSlotClass,
					elem.instanceDataStepRate } );
			}
		}
	}

	// Fallback to hardcoded layout if no vertex format specified (backward compatibility)
	if ( inputLayout.empty() )
	{
		inputLayout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}

	psoDesc.InputLayout = { inputLayout.data(), static_cast<UINT>( inputLayout.size() ) };

	// Root signature - use RootSignatureBuilder + RootSignatureCache (T215)
	const auto rootSigSpec = RootSignatureBuilder::Build( material );
	auto rootSignature = s_rootSignatureCache.getOrCreate( device, rootSigSpec );
	if ( !rootSignature )
	{
		console::fatal( "Failed to create root signature for material: {}", material.id );
		return nullptr;
	}

	psoDesc.pRootSignature = rootSignature.Get();

	// Rasterizer state - query from MaterialSystem or use D3D12 defaults
	const RasterizerStateBlock *rasterizerState = nullptr;
	if ( materialSystem && !material.states.rasterizer.empty() )
	{
		rasterizerState = materialSystem->getRasterizerState( material.states.rasterizer );
	}

	if ( rasterizerState )
	{
		// Apply state block from MaterialSystem
		psoDesc.RasterizerState.FillMode = rasterizerState->fillMode;
		psoDesc.RasterizerState.CullMode = rasterizerState->cullMode;
		psoDesc.RasterizerState.FrontCounterClockwise = rasterizerState->frontCounterClockwise;
		psoDesc.RasterizerState.DepthBias = rasterizerState->depthBias;
		psoDesc.RasterizerState.DepthBiasClamp = rasterizerState->depthBiasClamp;
		psoDesc.RasterizerState.SlopeScaledDepthBias = rasterizerState->slopeScaledDepthBias;
		psoDesc.RasterizerState.DepthClipEnable = rasterizerState->depthClipEnable;
		psoDesc.RasterizerState.MultisampleEnable = rasterizerState->multisampleEnable;
		psoDesc.RasterizerState.AntialiasedLineEnable = rasterizerState->antialiasedLineEnable;
		psoDesc.RasterizerState.ForcedSampleCount = rasterizerState->forcedSampleCount;
		psoDesc.RasterizerState.ConservativeRaster = rasterizerState->conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	else
	{
		// Use D3D12 defaults (backward compatible)
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.RasterizerState.MultisampleEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	// Blend state - query from MaterialSystem or use D3D12 defaults
	const BlendStateBlock *blendState = nullptr;
	if ( materialSystem && !material.states.blend.empty() )
	{
		blendState = materialSystem->getBlendState( material.states.blend );
	}

	if ( blendState )
	{
		// Apply state block from MaterialSystem
		psoDesc.BlendState.AlphaToCoverageEnable = blendState->alphaToCoverageEnable;
		psoDesc.BlendState.IndependentBlendEnable = blendState->independentBlendEnable;
		for ( UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
		{
			const auto &rtBlend = blendState->renderTargets[i];
			psoDesc.BlendState.RenderTarget[i] = rtBlend.toD3D12();
		}
	}
	else
	{
		// Use D3D12 defaults (backward compatible)
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		for ( UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
		{
			psoDesc.BlendState.RenderTarget[i].BlendEnable = FALSE;
			psoDesc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
			psoDesc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
			psoDesc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
			psoDesc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			psoDesc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			psoDesc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
			psoDesc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			psoDesc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
			psoDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
	}

	// Depth/stencil state - query from MaterialSystem or use D3D12 defaults
	const DepthStencilStateBlock *depthStencilState = nullptr;
	if ( materialSystem && !material.states.depthStencil.empty() )
	{
		depthStencilState = materialSystem->getDepthStencilState( material.states.depthStencil );
	}

	if ( depthStencilState )
	{
		// Apply state block from MaterialSystem
		psoDesc.DepthStencilState.DepthEnable = depthStencilState->depthEnable;
		psoDesc.DepthStencilState.DepthWriteMask = depthStencilState->depthWriteMask;
		psoDesc.DepthStencilState.DepthFunc = depthStencilState->depthFunc;
		psoDesc.DepthStencilState.StencilEnable = depthStencilState->stencilEnable;
		psoDesc.DepthStencilState.StencilReadMask = depthStencilState->stencilReadMask;
		psoDesc.DepthStencilState.StencilWriteMask = depthStencilState->stencilWriteMask;
		psoDesc.DepthStencilState.FrontFace = depthStencilState->frontFace.toD3D12();
		psoDesc.DepthStencilState.BackFace = depthStencilState->backFace.toD3D12();
	}
	else
	{
		// Use D3D12 defaults (backward compatible)
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP,
			D3D12_COMPARISON_FUNC_ALWAYS
		};
		psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
		psoDesc.DepthStencilState.BackFace = defaultStencilOp;
	}

	// Sample mask
	psoDesc.SampleMask = UINT_MAX;

	// Primitive topology - use material's specified topology
	psoDesc.PrimitiveTopologyType = material.primitiveTopology;

	// Render target formats - query from MaterialSystem or use passConfig
	const RenderTargetStateBlock *renderTargetState = nullptr;
	if ( materialSystem && !material.states.renderTarget.empty() )
	{
		renderTargetState = materialSystem->getRenderTargetState( material.states.renderTarget );
	}

	if ( renderTargetState )
	{
		// Apply render target state from MaterialSystem
		psoDesc.NumRenderTargets = static_cast<UINT>( renderTargetState->rtvFormats.size() );
		for ( UINT i = 0; i < renderTargetState->rtvFormats.size() && i < 8; ++i )
		{
			psoDesc.RTVFormats[i] = renderTargetState->rtvFormats[i];
		}
		psoDesc.DSVFormat = renderTargetState->dsvFormat;
		psoDesc.SampleDesc.Count = renderTargetState->sampleCount;
		psoDesc.SampleDesc.Quality = renderTargetState->sampleQuality;
	}
	else
	{
		// Use passConfig (backward compatible)
		psoDesc.NumRenderTargets = passConfig.numRenderTargets;
		for ( UINT i = 0; i < passConfig.numRenderTargets && i < 8; ++i )
		{
			psoDesc.RTVFormats[i] = passConfig.rtvFormats[i];
		}
		psoDesc.DSVFormat = passConfig.dsvFormat;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
	}

	// Create PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = device->get()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pso ) );
	if ( FAILED( hr ) )
	{
		console::error( "Failed to create pipeline state for material '{}', HRESULT={:#x}", material.id, static_cast<unsigned int>( hr ) );
		return nullptr;
	}

	// Store in cache for future reuse
	s_cache.store( hash, pso, material.id, passConfig.name );

	return pso;
}

} // namespace graphics::material_system

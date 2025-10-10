#include "pipeline_builder.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/material_system/shader_compiler.h"
#include "core/console.h"
#include <d3d12.h>
#include <filesystem>

namespace graphics::material_system
{

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineBuilder::buildPSO(
	dx12::Device *device,
	const MaterialDefinition &material,
	const RenderPassConfig &passConfig )
{
	if ( !device || !device->get() )
	{
		console::error( "PipelineBuilder::buildPSO: invalid device" );
		return nullptr;
	}

	// Compile shaders using MaterialShaderCompiler (T012)
	// For now, using simple.hlsl as placeholder for all materials
	const std::filesystem::path shaderPath = "shaders/simple.hlsl";
	const std::unordered_map<std::string, std::string> emptyDefines;

	const auto vsBlob = MaterialShaderCompiler::CompileWithDefines(
		shaderPath, "VSMain", "vs_5_0", emptyDefines );

	if ( !vsBlob.isValid() || !vsBlob.blob )
	{
		console::error( "Failed to compile vertex shader for material '{}'", material.id );
		return nullptr;
	}

	const auto psBlob = MaterialShaderCompiler::CompileWithDefines(
		shaderPath, "PSMain", "ps_5_0", emptyDefines );

	if ( !psBlob.isValid() || !psBlob.blob )
	{
		console::error( "Failed to compile pixel shader for material '{}'", material.id );
		return nullptr;
	}

	// Build pipeline state descriptor
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// Shaders - using compiled bytecode
	psoDesc.VS = { vsBlob.blob->GetBufferPointer(), vsBlob.blob->GetBufferSize() };
	psoDesc.PS = { psBlob.blob->GetBufferPointer(), psBlob.blob->GetBufferSize() };

	// Input layout - must match simple.hlsl (POSITION + COLOR)
	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };

	// Root signature - create minimal one with CBV for simple.hlsl
	// TODO: Use RootSignatureBuilder (T013) to generate root signature from material parameters
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam.Descriptor.ShaderRegister = 0;
	rootParam.Descriptor.RegisterSpace = 0;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParam;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature( &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob );
	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			console::error( "Root signature serialization failed: {}", static_cast<char *>( errorBlob->GetBufferPointer() ) );
		}
		return nullptr;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = device->get()->CreateRootSignature( 0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS( &rootSignature ) );
	if ( FAILED( hr ) )
	{
		console::error( "Root signature creation failed" );
		return nullptr;
	}

	psoDesc.pRootSignature = rootSignature.Get();

	// Rasterizer state - using defaults
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

	// Blend state - using defaults
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

	// Depth/stencil state - using defaults
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

	// Sample mask
	psoDesc.SampleMask = UINT_MAX;

	// Primitive topology
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Render target formats from passConfig
	psoDesc.NumRenderTargets = passConfig.numRenderTargets;
	for ( UINT i = 0; i < passConfig.numRenderTargets && i < 8; ++i )
	{
		psoDesc.RTVFormats[i] = passConfig.rtvFormats[i];
	}
	psoDesc.DSVFormat = passConfig.dsvFormat;

	// Sample description
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	// Create PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	hr = device->get()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pso ) );
	if ( FAILED( hr ) )
	{
		console::error( "Failed to create pipeline state for material '{}', HRESULT={:#x}", material.id, static_cast<unsigned int>( hr ) );
		return nullptr;
	}

	return pso;
}

} // namespace graphics::material_system

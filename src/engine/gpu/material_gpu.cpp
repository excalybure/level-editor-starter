// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>
#include <cstring>

module engine.gpu.material_gpu;

import runtime.console;

namespace engine::gpu
{

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material )
	: m_material( material ), m_device( nullptr )
{
	if ( !material )
	{
		console::error( "MaterialGPU: Cannot create from null material" );
		return;
	}

	updateMaterialConstants();
	// Note: GPU resources not created without device
	console::info( "MaterialGPU: Created material-only instance (no GPU resources)" );
	m_isValid = true;
}

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material, dx12::Device &device )
	: m_material( material ), m_device( &device )
{
	if ( !material )
	{
		console::error( "MaterialGPU: Cannot create from null material" );
		return;
	}

	updateMaterialConstants();
	createConstantBuffer();
	createPipelineState();
	loadTextures();

	m_isValid = true;
}

MaterialGPU::MaterialGPU( MaterialGPU &&other ) noexcept
	: m_material( std::move( other.m_material ) ), m_materialConstants( other.m_materialConstants ), m_device( other.m_device ), m_pipelineState( std::move( other.m_pipelineState ) ), m_constantBuffer( std::move( other.m_constantBuffer ) ), m_isValid( other.m_isValid )
{
	other.m_isValid = false;
	other.m_device = nullptr;
}

MaterialGPU &MaterialGPU::operator=( MaterialGPU &&other ) noexcept
{
	if ( this != &other )
	{
		m_material = std::move( other.m_material );
		m_materialConstants = other.m_materialConstants;
		m_device = other.m_device;
		m_pipelineState = std::move( other.m_pipelineState );
		m_constantBuffer = std::move( other.m_constantBuffer );
		m_isValid = other.m_isValid;

		other.m_isValid = false;
		other.m_device = nullptr;
	}
	return *this;
}

MaterialGPU::~MaterialGPU() = default;

void MaterialGPU::bindToCommandList( ID3D12GraphicsCommandList *commandList ) const
{
	if ( !isValid() || !commandList )
	{
		console::error( "MaterialGPU::bindToCommandList: Invalid state or null command list" );
		return;
	}

	if ( !m_device )
	{
		console::info( "MaterialGPU: Binding material resources to command list (stub - no device)" );
		return;
	}

	// Set the pipeline state if available
	if ( m_pipelineState )
	{
		commandList->SetPipelineState( m_pipelineState.Get() );
	}

	// Bind constant buffer if available
	if ( m_constantBuffer )
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = m_constantBuffer->GetGPUVirtualAddress();
		commandList->SetGraphicsRootConstantBufferView( 0, cbvAddress ); // Assuming root parameter 0 is CBV
	}

	// TODO: Bind textures when texture loading is implemented
	// This would involve setting descriptor tables or root descriptors for textures

	console::info( "MaterialGPU: Material resources bound to command list" );
}

void MaterialGPU::createPipelineState()
{
	if ( !m_device )
	{
		console::info( "MaterialGPU: Creating pipeline state (stub - no device)" );
		return;
	}

	// For now, create a basic pipeline state for material rendering
	// TODO: Implement proper material-specific pipeline state with shaders

	// Create a simple root signature for materials
	// This is a placeholder - should be shared or more sophisticated
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam.Descriptor.ShaderRegister = 0; // b0
	rootParam.Descriptor.RegisterSpace = 0;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = &rootParam;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature( &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob );
	if ( FAILED( hr ) )
	{
		console::error( "MaterialGPU: Failed to serialize root signature" );
		return;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	hr = m_device->get()->CreateRootSignature( 0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS( &rootSignature ) );
	if ( FAILED( hr ) )
	{
		console::error( "MaterialGPU: Failed to create root signature" );
		return;
	}

	// Create basic shaders (placeholders for now)
	// TODO: Use proper material shaders from shader manager
	const char *vsSource = R"(
		struct VSInput {
			float3 position : POSITION;
			float3 normal : NORMAL;
			float2 texcoord : TEXCOORD0;
		};
		struct VSOutput {
			float4 position : SV_POSITION;
			float3 normal : NORMAL;
			float2 texcoord : TEXCOORD0;
		};
		VSOutput main(VSInput input) {
			VSOutput output;
			output.position = float4(input.position, 1.0);
			output.normal = input.normal;
			output.texcoord = input.texcoord;
			return output;
		}
	)";

	const char *psSource = R"(
		struct PSInput {
			float4 position : SV_POSITION;
			float3 normal : NORMAL;
			float2 texcoord : TEXCOORD0;
		};
		float4 main(PSInput input) : SV_TARGET {
			return float4(1.0, 0.0, 1.0, 1.0); // Magenta placeholder
		}
	)";

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob;

	hr = D3DCompile( vsSource, strlen( vsSource ), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob );
	if ( FAILED( hr ) )
	{
		console::error( "MaterialGPU: Failed to compile vertex shader" );
		return;
	}

	hr = D3DCompile( psSource, strlen( psSource ), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob );
	if ( FAILED( hr ) )
	{
		console::error( "MaterialGPU: Failed to compile pixel shader" );
		return;
	}

	// Create pipeline state
	const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	hr = m_device->get()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_pipelineState ) );
	if ( SUCCEEDED( hr ) )
	{
		console::info( "MaterialGPU: Pipeline state created successfully" );
	}
	else
	{
		console::error( "MaterialGPU: Failed to create pipeline state" );
	}
}

void MaterialGPU::createConstantBuffer()
{
	if ( !m_device )
	{
		console::info( "MaterialGPU: Creating constant buffer (stub - no device)" );
		return;
	}

	// Create constant buffer resource for MaterialConstants
	const UINT constantBufferSize = ( sizeof( MaterialConstants ) + 255 ) & ~255; // Align to 256 bytes

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = constantBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = m_device->get()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &m_constantBuffer ) );

	if ( FAILED( hr ) )
	{
		console::error( "MaterialGPU: Failed to create constant buffer" );
		return;
	}

	// Map and copy the material constants
	void *mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	hr = m_constantBuffer->Map( 0, &readRange, &mappedData );
	if ( SUCCEEDED( hr ) )
	{
		memcpy( mappedData, &m_materialConstants, sizeof( MaterialConstants ) );
		m_constantBuffer->Unmap( 0, nullptr );
		console::info( "MaterialGPU: Constant buffer created and mapped successfully" );
	}
	else
	{
		console::error( "MaterialGPU: Failed to map constant buffer" );
	}
}

void MaterialGPU::updateMaterialConstants()
{
	if ( !m_material )
	{
		return;
	}

	const auto &pbr = m_material->getPBRMaterial();

	m_materialConstants.baseColorFactor = pbr.baseColorFactor;

	m_materialConstants.metallicFactor = pbr.metallicFactor;
	m_materialConstants.roughnessFactor = pbr.roughnessFactor;

	m_materialConstants.emissiveFactor = pbr.emissiveFactor;

	// Set texture flags based on available textures
	m_materialConstants.textureFlags = 0;
	if ( !pbr.baseColorTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kBaseColorTextureBit;
	}
	if ( !pbr.metallicRoughnessTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kMetallicRoughnessTextureBit;
	}
	if ( !pbr.normalTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kNormalTextureBit;
	}
	if ( !pbr.emissiveTexture.empty() )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kEmissiveTextureBit;
	}
}

void MaterialGPU::loadTextures()
{
	if ( !m_device )
	{
		console::info( "MaterialGPU: Loading textures (stub - no device)" );
		return;
	}

	if ( !m_material )
	{
		console::error( "MaterialGPU: Cannot load textures without material" );
		return;
	}

	const auto &pbr = m_material->getPBRMaterial();

	// Log which textures need to be loaded
	if ( !pbr.baseColorTexture.empty() )
	{
		console::info( "MaterialGPU: Loading base color texture: " + pbr.baseColorTexture );
		// TODO: Implement actual texture loading using texture manager
	}

	if ( !pbr.metallicRoughnessTexture.empty() )
	{
		console::info( "MaterialGPU: Loading metallic roughness texture: " + pbr.metallicRoughnessTexture );
		// TODO: Implement actual texture loading
	}

	if ( !pbr.normalTexture.empty() )
	{
		console::info( "MaterialGPU: Loading normal texture: " + pbr.normalTexture );
		// TODO: Implement actual texture loading
	}

	if ( !pbr.emissiveTexture.empty() )
	{
		console::info( "MaterialGPU: Loading emissive texture: " + pbr.emissiveTexture );
		// TODO: Implement actual texture loading
	}

	console::info( "MaterialGPU: Texture loading preparation completed" );
}

} // namespace engine::gpu
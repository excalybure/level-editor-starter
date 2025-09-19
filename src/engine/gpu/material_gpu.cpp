// Global module fragment - headers go here
module;

#include <d3d12.h>
#include <wrl.h>
#include <cstring>

module engine.gpu.material_gpu;

import runtime.console;
import engine.matrix;
import engine.vec;

namespace engine::gpu
{

// ObjectConstants definition to match MeshRenderingSystem's root signature
struct ObjectConstants
{
	math::Mat4<> worldMatrix;
	math::Mat4<> normalMatrix;

	ObjectConstants() = default;
};

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
	loadTextures();
	m_isValid = true;
}

MaterialGPU::MaterialGPU( MaterialGPU &&other ) noexcept
	: m_material( std::move( other.m_material ) ), m_materialConstants( other.m_materialConstants ), m_device( other.m_device ), m_constantBuffer( std::move( other.m_constantBuffer ) ), m_isValid( other.m_isValid )
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

	// Bind material constant buffer if available to root parameter 2 (b2)
	if ( m_constantBuffer )
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = m_constantBuffer->GetGPUVirtualAddress();
		commandList->SetGraphicsRootConstantBufferView( 2, cbvAddress );
	}

	// TODO: Bind textures when texture loading is implemented
	// This would involve setting descriptor tables or root descriptors for textures
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
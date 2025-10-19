#include "material_gpu.h"

#include <d3d12.h>
#include <wrl.h>
#include <cstring>

#include "engine/assets/assets.h"
#include "math/matrix.h"
#include "math/vec.h"
#include "platform/dx12/dx12_device.h"
#include "core/console.h"
#include "graphics/texture/texture_manager.h"

namespace graphics::gpu
{

// ObjectConstants definition to match MeshRenderingSystem's root signature
struct ObjectConstants
{
	math::Mat4<> worldMatrix;
	math::Mat4<> normalMatrix;

	ObjectConstants() = default;
};

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material, dx12::Device &device, graphics::texture::TextureManager &textureManager )
	: m_material( material ), m_device( &device ), m_textureManager( &textureManager )
{
	if ( !material )
	{
		console::error( "MaterialGPU: Cannot create from null material" );
		return;
	}

	initializeGPUResources();
}

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material, dx12::Device &device, const assets::MaterialInstance *instance, graphics::texture::TextureManager &textureManager )
	: m_material( material ), m_device( &device ), m_textureManager( &textureManager ), m_materialInstance( instance )
{
	if ( !material )
	{
		console::error( "MaterialGPU: Cannot create from null material" );
		return;
	}

	initializeGPUResources();
}

MaterialGPU::MaterialGPU( MaterialGPU &&other ) noexcept
	: m_material( std::move( other.m_material ) ), m_materialConstants( other.m_materialConstants ), m_device( other.m_device ), m_textureManager( other.m_textureManager ), m_materialInstance( other.m_materialInstance ), m_constantBuffer( std::move( other.m_constantBuffer ) ), m_baseColorTexture( other.m_baseColorTexture ), m_metallicRoughnessTexture( other.m_metallicRoughnessTexture ), m_normalTexture( other.m_normalTexture ), m_emissiveTexture( other.m_emissiveTexture ), m_isValid( other.m_isValid )
{
	other.m_isValid = false;
	other.m_device = nullptr;
	other.m_textureManager = nullptr;
	other.m_materialInstance = nullptr;
	other.m_baseColorTexture = graphics::texture::kInvalidTextureHandle;
	other.m_metallicRoughnessTexture = graphics::texture::kInvalidTextureHandle;
	other.m_normalTexture = graphics::texture::kInvalidTextureHandle;
	other.m_emissiveTexture = graphics::texture::kInvalidTextureHandle;
}

MaterialGPU &MaterialGPU::operator=( MaterialGPU &&other ) noexcept
{
	if ( this != &other )
	{
		m_material = std::move( other.m_material );
		m_materialConstants = other.m_materialConstants;
		m_device = other.m_device;
		m_textureManager = other.m_textureManager;
		m_materialInstance = other.m_materialInstance;
		m_constantBuffer = std::move( other.m_constantBuffer );
		m_baseColorTexture = other.m_baseColorTexture;
		m_metallicRoughnessTexture = other.m_metallicRoughnessTexture;
		m_normalTexture = other.m_normalTexture;
		m_emissiveTexture = other.m_emissiveTexture;
		m_isValid = other.m_isValid;

		other.m_isValid = false;
		other.m_device = nullptr;
		other.m_textureManager = nullptr;
		other.m_materialInstance = nullptr;
		other.m_baseColorTexture = graphics::texture::kInvalidTextureHandle;
		other.m_metallicRoughnessTexture = graphics::texture::kInvalidTextureHandle;
		other.m_normalTexture = graphics::texture::kInvalidTextureHandle;
		other.m_emissiveTexture = graphics::texture::kInvalidTextureHandle;
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

	// Apply overrides if MaterialInstance is provided, otherwise use base material
	if ( m_materialInstance )
	{
		m_materialConstants.baseColorFactor = m_materialInstance->getEffectiveBaseColor( m_material.get() );
		m_materialConstants.metallicFactor = m_materialInstance->getEffectiveMetallic( m_material.get() );
		m_materialConstants.roughnessFactor = m_materialInstance->getEffectiveRoughness( m_material.get() );
		m_materialConstants.emissiveFactor = m_materialInstance->getEffectiveEmissive( m_material.get() );
	}
	else
	{
		m_materialConstants.baseColorFactor = pbr.baseColorFactor;
		m_materialConstants.metallicFactor = pbr.metallicFactor;
		m_materialConstants.roughnessFactor = pbr.roughnessFactor;
		m_materialConstants.emissiveFactor = pbr.emissiveFactor;
	}

	// Set texture flags based on available textures (considering overrides)
	m_materialConstants.textureFlags = 0;

	// Helper lambda to check if texture exists (override or base)
	auto hasTexture = [&]( const std::optional<std::string> &override, const std::string &base ) -> bool {
		if ( m_materialInstance && override.has_value() )
			return !override.value().empty();
		return !base.empty();
	};

	if ( hasTexture( m_materialInstance ? m_materialInstance->baseColorTextureOverride : std::nullopt, pbr.baseColorTexture ) )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kBaseColorTextureBit;
	}
	if ( hasTexture( m_materialInstance ? m_materialInstance->metallicRoughnessTextureOverride : std::nullopt, pbr.metallicRoughnessTexture ) )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kMetallicRoughnessTextureBit;
	}
	if ( hasTexture( m_materialInstance ? m_materialInstance->normalTextureOverride : std::nullopt, pbr.normalTexture ) )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kNormalTextureBit;
	}
	if ( hasTexture( m_materialInstance ? m_materialInstance->emissiveTextureOverride : std::nullopt, pbr.emissiveTexture ) )
	{
		m_materialConstants.textureFlags |= MaterialConstants::kEmissiveTextureBit;
	}

	// Populate texture indices from TextureManager if available
	// This must be called after loadTextures() to have valid texture handles
	m_materialConstants.textureIndices[0] = m_textureManager->getSrvIndex( m_baseColorTexture );
	m_materialConstants.textureIndices[1] = m_textureManager->getSrvIndex( m_normalTexture );
	m_materialConstants.textureIndices[2] = m_textureManager->getSrvIndex( m_metallicRoughnessTexture );
	m_materialConstants.textureIndices[3] = m_textureManager->getSrvIndex( m_emissiveTexture );
}

void MaterialGPU::loadTextures()
{
	if ( !m_material )
	{
		console::error( "MaterialGPU: Cannot load textures without material" );
		return;
	}

	const auto &pbr = m_material->getPBRMaterial();

	// Helper that returns the texture handle: load override via TextureManager when an override path
	// is present and non-empty; otherwise return the base material's pre-resolved handle.
	const auto pickTextureHandle = [&]( const std::optional<std::string> &overridePath, graphics::texture::TextureHandle baseHandle ) -> graphics::texture::TextureHandle {
		if ( overridePath.has_value() && !overridePath->empty() )
			return m_textureManager->loadTexture( overridePath.value() );
		return baseHandle;
	};

	// Use the helper for each texture; if there is no material instance, pass std::nullopt
	const std::optional<std::string> noOverride;
	const auto &inst = m_materialInstance;

	m_baseColorTexture = pickTextureHandle( inst ? inst->baseColorTextureOverride : noOverride, pbr.baseColorTextureHandle );
	m_metallicRoughnessTexture = pickTextureHandle( inst ? inst->metallicRoughnessTextureOverride : noOverride, pbr.metallicRoughnessTextureHandle );
	m_normalTexture = pickTextureHandle( inst ? inst->normalTextureOverride : noOverride, pbr.normalTextureHandle );
	m_emissiveTexture = pickTextureHandle( inst ? inst->emissiveTextureOverride : noOverride, pbr.emissiveTextureHandle );
}

void MaterialGPU::initializeGPUResources()
{
	loadTextures();
	updateMaterialConstants();
	createConstantBuffer();
	m_isValid = true;
}

void MaterialGPU::updateFromInstance( const assets::MaterialInstance *instance )
{
	m_materialInstance = instance;

	// Reload textures to apply texture overrides
	loadTextures();

	// Update material constants (includes texture flags and indices)
	updateMaterialConstants();

	// Update the GPU constant buffer with new values
	if ( m_constantBuffer )
	{
		void *mappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };
		HRESULT hr = m_constantBuffer->Map( 0, &readRange, &mappedData );
		if ( SUCCEEDED( hr ) )
		{
			memcpy( mappedData, &m_materialConstants, sizeof( MaterialConstants ) );
			m_constantBuffer->Unmap( 0, nullptr );
		}
		else
		{
			console::error( "MaterialGPU::updateFromInstance: Failed to map constant buffer" );
		}
	}
}

} // namespace graphics::gpu
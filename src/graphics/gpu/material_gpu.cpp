#include "material_gpu.h"

#include <d3d12.h>
#include <wrl.h>
#include <cstring>
#include <filesystem>

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

MaterialGPU::MaterialGPU( const std::shared_ptr<assets::Material> &material, dx12::Device &device, graphics::texture::TextureManager *textureManager )
	: m_material( material ), m_device( &device ), m_textureManager( textureManager )
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
	: m_material( std::move( other.m_material ) ), m_materialConstants( other.m_materialConstants ), m_device( other.m_device ), m_textureManager( other.m_textureManager ), m_constantBuffer( std::move( other.m_constantBuffer ) ), m_baseColorTexture( other.m_baseColorTexture ), m_metallicRoughnessTexture( other.m_metallicRoughnessTexture ), m_normalTexture( other.m_normalTexture ), m_emissiveTexture( other.m_emissiveTexture ), m_isValid( other.m_isValid )
{
	other.m_isValid = false;
	other.m_device = nullptr;
	other.m_textureManager = nullptr;
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
		m_constantBuffer = std::move( other.m_constantBuffer );
		m_baseColorTexture = other.m_baseColorTexture;
		m_metallicRoughnessTexture = other.m_metallicRoughnessTexture;
		m_normalTexture = other.m_normalTexture;
		m_emissiveTexture = other.m_emissiveTexture;
		m_isValid = other.m_isValid;

		other.m_isValid = false;
		other.m_device = nullptr;
		other.m_textureManager = nullptr;
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

	// Bind textures if texture manager is available
	bindTextures( commandList );
}

void MaterialGPU::bindTextures( ID3D12GraphicsCommandList *commandList ) const
{
	if ( !commandList )
	{
		console::error( "MaterialGPU::bindTextures: Null command list" );
		return;
	}

	if ( !m_textureManager )
	{
		// No texture manager - textures not supported yet
		return;
	}

	// Set descriptor heap
	ID3D12DescriptorHeap *heaps[] = { m_textureManager->getSrvHeap() };
	if ( !heaps[0] )
	{
		console::error( "MaterialGPU::bindTextures: Texture manager has null SRV heap" );
		return;
	}

	commandList->SetDescriptorHeaps( 1, heaps );

	// Get SRV index for base color texture (or use first valid texture handle)
	// For now, we'll use the base color texture as the starting point
	const uint32_t baseIndex = m_textureManager->getSrvIndex( m_baseColorTexture );
	if ( baseIndex == UINT32_MAX )
	{
		// No valid textures to bind
		return;
	}

	// Get GPU handle for this material's textures
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heaps[0]->GetGPUDescriptorHandleForHeapStart();
	const UINT descriptorSize = m_textureManager->getDevice()->get()->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
	gpuHandle.ptr += baseIndex * descriptorSize;

	// Bind descriptor table to root parameter 2 (will be changed based on actual root signature)
	// TODO: Update this to use correct root parameter index once root signature is finalized
	commandList->SetGraphicsRootDescriptorTable( 2, gpuHandle );
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

	// If no texture manager, just log what would be loaded
	if ( !m_textureManager )
	{
		if ( !pbr.baseColorTexture.empty() )
		{
			console::info( "MaterialGPU: Loading base color texture: " + pbr.baseColorTexture );
		}
		if ( !pbr.metallicRoughnessTexture.empty() )
		{
			console::info( "MaterialGPU: Loading metallic roughness texture: " + pbr.metallicRoughnessTexture );
		}
		if ( !pbr.normalTexture.empty() )
		{
			console::info( "MaterialGPU: Loading normal texture: " + pbr.normalTexture );
		}
		if ( !pbr.emissiveTexture.empty() )
		{
			console::info( "MaterialGPU: Loading emissive texture: " + pbr.emissiveTexture );
		}
		return;
	}

	// Load textures using texture manager
	// Get base path from material if available
	const std::string basePath = m_material->getPath().empty() ? "" : std::filesystem::path( m_material->getPath() ).parent_path().string();

	if ( !pbr.baseColorTexture.empty() )
	{
		m_baseColorTexture = m_textureManager->loadTexture( pbr.baseColorTexture, basePath );
		if ( m_baseColorTexture == graphics::texture::kInvalidTextureHandle )
		{
			console::error( "MaterialGPU: Failed to load base color texture: " + pbr.baseColorTexture );
		}
	}

	if ( !pbr.metallicRoughnessTexture.empty() )
	{
		m_metallicRoughnessTexture = m_textureManager->loadTexture( pbr.metallicRoughnessTexture, basePath );
		if ( m_metallicRoughnessTexture == graphics::texture::kInvalidTextureHandle )
		{
			console::error( "MaterialGPU: Failed to load metallic roughness texture: " + pbr.metallicRoughnessTexture );
		}
	}

	if ( !pbr.normalTexture.empty() )
	{
		m_normalTexture = m_textureManager->loadTexture( pbr.normalTexture, basePath );
		if ( m_normalTexture == graphics::texture::kInvalidTextureHandle )
		{
			console::error( "MaterialGPU: Failed to load normal texture: " + pbr.normalTexture );
		}
	}

	if ( !pbr.emissiveTexture.empty() )
	{
		m_emissiveTexture = m_textureManager->loadTexture( pbr.emissiveTexture, basePath );
		if ( m_emissiveTexture == graphics::texture::kInvalidTextureHandle )
		{
			console::error( "MaterialGPU: Failed to load emissive texture: " + pbr.emissiveTexture );
		}
	}
}

} // namespace graphics::gpu
#include "texture_manager.h"
#include "bindless_texture_heap.h"
#include "texture_loader.h"
#include <platform/dx12/dx12_device.h>
#include <core/console.h>
#include <filesystem>

namespace graphics::texture
{

// Destructor must be in cpp where BindlessTextureHeap is complete
TextureManager::~TextureManager() = default;

bool TextureManager::initialize( dx12::Device *device, uint32_t maxTextures )
{
	if ( !device )
	{
		console::error( "TextureManager::initialize: Device is null" );
		return false;
	}

	m_device = device;

	// Create bindless heap
	m_bindlessHeap = std::make_unique<BindlessTextureHeap>();
	if ( !m_bindlessHeap->initialize( device->get(), maxTextures ) )
	{
		console::error( "TextureManager::initialize: Failed to create bindless heap" );
		return false;
	}

	// Reserve slot 0 as invalid
	m_textures.resize( 1 );
	m_textures[0].isValid = false;

	return true;
}

void TextureManager::shutdown()
{
	// Release all textures
	m_textures.clear();
	m_pathCache.clear();
	m_freeHandles.clear();

	if ( m_bindlessHeap )
	{
		m_bindlessHeap->shutdown();
		m_bindlessHeap.reset();
	}

	m_device = nullptr;
}

TextureHandle TextureManager::loadTexture( const std::string &path, const std::string &basePath )
{
	if ( path.empty() )
	{
		console::error( "TextureManager::loadTexture: Empty path" );
		return kInvalidTextureHandle;
	}

	// Resolve full path
	std::filesystem::path fullPath;
	if ( basePath.empty() || std::filesystem::path( path ).is_absolute() )
	{
		fullPath = path;
	}
	else
	{
		fullPath = std::filesystem::path( basePath ) / path;
	}

	// Normalize to canonical path for consistent caching (case-insensitive on Windows)
	std::error_code ec;
	const auto canonicalPath = std::filesystem::weakly_canonical( fullPath, ec );
	const std::string fullPathStr = ec ? fullPath.string() : canonicalPath.string();

	// Check cache
	const auto cacheIt = m_pathCache.find( fullPathStr );
	if ( cacheIt != m_pathCache.end() )
	{
		const TextureHandle handle = cacheIt->second;
		if ( handle < m_textures.size() && m_textures[handle].isValid )
		{
			// Increment refcount
			++m_textures[handle].refCount;
			return handle;
		}
		else
		{
			// Invalid cached handle, remove from cache
			m_pathCache.erase( cacheIt );
		}
	}

	// Load image data
	const auto imageData = TextureLoader::loadFromFile( fullPathStr );
	if ( !imageData.has_value() )
	{
		console::error( "TextureManager::loadTexture: Failed to load image from '{}'", fullPathStr );
		return kInvalidTextureHandle;
	}

	// Create GPU texture and allocate handle
	const TextureHandle handle = createTextureFromImageData( imageData.value(), fullPathStr );
	if ( handle != kInvalidTextureHandle )
	{
		// Cache by path for file-based loads
		m_pathCache[fullPathStr] = handle;
	}

	return handle;
}

TextureHandle TextureManager::loadTextureFromMemory( const uint8_t *data, size_t size, const std::string &debugName )
{
	if ( !data || size == 0 )
	{
		console::error( "TextureManager::loadTextureFromMemory: Invalid data" );
		return kInvalidTextureHandle;
	}

	// Load image data
	const auto imageData = TextureLoader::loadFromMemory( data, size );
	if ( !imageData.has_value() )
	{
		console::error( "TextureManager::loadTextureFromMemory: Failed to load image" );
		return kInvalidTextureHandle;
	}

	// Create GPU texture and allocate handle (no path caching for memory loads)
	return createTextureFromImageData( imageData.value(), debugName );
}

void TextureManager::releaseTexture( TextureHandle handle )
{
	if ( handle >= m_textures.size() || !m_textures[handle].isValid )
	{
		console::error( "TextureManager::releaseTexture: Invalid handle {}", handle );
		return;
	}

	TextureEntry &entry = m_textures[handle];

	// Decrement refcount
	if ( entry.refCount > 0 )
	{
		--entry.refCount;
	}

	// Free texture when refcount reaches 0
	if ( entry.refCount == 0 )
	{
		// Deallocate descriptor
		m_bindlessHeap->deallocate( entry.info.srvIndex );

		// Remove from path cache
		if ( !entry.path.empty() )
		{
			m_pathCache.erase( entry.path );
		}

		// Mark entry as invalid
		entry.isValid = false;
		entry.texture.reset();

		// Add handle to free list
		m_freeHandles.push_back( handle );
	}
}

const TextureInfo *TextureManager::getTextureInfo( TextureHandle handle ) const
{
	if ( handle >= m_textures.size() || !m_textures[handle].isValid )
	{
		return nullptr;
	}

	return &m_textures[handle].info;
}

uint32_t TextureManager::getSrvIndex( TextureHandle handle ) const
{
	if ( handle >= m_textures.size() || !m_textures[handle].isValid )
	{
		return UINT32_MAX;
	}

	return m_textures[handle].info.srvIndex;
}

ID3D12DescriptorHeap *TextureManager::getSrvHeap() const
{
	return m_bindlessHeap ? m_bindlessHeap->getHeap() : nullptr;
}

TextureHandle TextureManager::createTextureFromImageData( const ImageData &imageData, const std::string &pathForCache )
{
	// Allocate descriptor slot
	const auto srvIndex = m_bindlessHeap->allocate();
	if ( !srvIndex.has_value() )
	{
		console::error( "TextureManager: Bindless heap is full" );
		return kInvalidTextureHandle;
	}

	// Create GPU texture
	auto texture = std::make_shared<dx12::Texture>();
	if ( !texture->createFromImageData( m_device, imageData, D3D12_RESOURCE_FLAG_NONE ) )
	{
		console::error( "TextureManager: Failed to create GPU texture" );
		m_bindlessHeap->deallocate( srvIndex.value() );
		return kInvalidTextureHandle;
	}

	// Create SRV in bindless heap
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = imageData.format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	m_bindlessHeap->createSRV( srvIndex.value(), texture->getResource(), &srvDesc );

	// Allocate handle
	TextureHandle handle;
	if ( !m_freeHandles.empty() )
	{
		handle = m_freeHandles.back();
		m_freeHandles.pop_back();
	}
	else
	{
		handle = static_cast<TextureHandle>( m_textures.size() );
		m_textures.resize( handle + 1 );
	}

	// Store texture entry
	TextureEntry &entry = m_textures[handle];
	entry.texture = texture;
	entry.info.width = imageData.width;
	entry.info.height = imageData.height;
	entry.info.format = imageData.format;
	entry.info.srvIndex = srvIndex.value();
	entry.path = pathForCache;
	entry.refCount = 1;
	entry.isValid = true;

	return handle;
}

} // namespace graphics::texture

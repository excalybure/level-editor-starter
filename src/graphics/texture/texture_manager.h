#pragma once

#include <d3d12.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <dxgiformat.h>

// Forward declarations
namespace dx12
{
class Device;
class Texture;
} // namespace dx12

namespace graphics::texture
{

struct ImageData; // From texture_loader.h
class BindlessTextureHeap;

using TextureHandle = uint32_t;
constexpr TextureHandle kInvalidTextureHandle = 0;

struct TextureInfo
{
	uint32_t width = 0;
	uint32_t height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint32_t srvIndex = 0; // Index in bindless heap
};

class TextureManager
{
public:
	TextureManager() = default;
	~TextureManager(); // Needs to be in cpp for unique_ptr<BindlessTextureHeap>

	// No copy/move for now
	TextureManager( const TextureManager & ) = delete;
	TextureManager &operator=( const TextureManager & ) = delete;

	// Initialize with device and max textures
	bool initialize( dx12::Device *device, uint32_t maxTextures = 4096 );
	void shutdown();

	// Load texture from file path (caches by path)
	// Returns INVALID_TEXTURE_HANDLE on failure
	TextureHandle loadTexture( const std::string &path, const std::string &basePath = "" );

	// Load texture from memory (no caching)
	TextureHandle loadTextureFromMemory( const uint8_t *data, size_t size, const std::string &debugName = "" );

	// Release texture (decrements refcount, frees when reaches 0)
	void releaseTexture( TextureHandle handle );

	// Query texture info
	const TextureInfo *getTextureInfo( TextureHandle handle ) const;
	uint32_t getSrvIndex( TextureHandle handle ) const;

	// Get bindless heap (for binding to command list)
	ID3D12DescriptorHeap *getSrvHeap() const;

	// Get device
	dx12::Device *getDevice() const { return m_device; }

private:
	struct TextureEntry
	{
		std::shared_ptr<dx12::Texture> texture;
		TextureInfo info;
		std::string path; // For caching
		uint32_t refCount = 0;
		bool isValid = false;
	};

	// Helper to create GPU texture from image data and allocate handle
	TextureHandle createTextureFromImageData( const ImageData &imageData, const std::string &pathForCache );

	dx12::Device *m_device = nullptr;
	std::unique_ptr<BindlessTextureHeap> m_bindlessHeap;
	std::vector<TextureEntry> m_textures; // Indexed by handle
	std::unordered_map<std::string, TextureHandle> m_pathCache;
	std::vector<TextureHandle> m_freeHandles; // Recycled handles
};

} // namespace graphics::texture

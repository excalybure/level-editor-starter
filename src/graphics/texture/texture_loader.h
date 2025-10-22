#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <dxgiformat.h>

// Forward declarations
namespace graphics::image
{
enum class MipmapFilter;
}

namespace graphics::texture
{

struct ImageData
{
	std::vector<uint8_t> pixels;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channels = 0; // Actual channel count (1, 2, 3, or 4)
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	std::vector<ImageData> mipLevels; // Additional mip levels (mip 1, 2, 3...). Base is mip 0.
};

class TextureLoader
{
public:
	// Load from file path (relative or absolute)
	static std::optional<ImageData> loadFromFile( const std::string &path );

	// Load from memory buffer (for embedded data)
	static std::optional<ImageData> loadFromMemory( const uint8_t *data, size_t size );

	// Load from glTF data URI (handles base64 decoding)
	static std::optional<ImageData> loadFromDataURI( const std::string &uri );

	// Load from file path with automatic mipmap generation
	// @param path File path to load
	// @param maxLevels Maximum mip levels (0 = full chain to 1x1)
	// @param filter Downsampling filter for mipmap generation
	// @return ImageData with mipLevels populated, or nullopt on failure
	static std::optional<ImageData> loadWithMipmaps(
		const std::string &path,
		uint32_t maxLevels,
		graphics::image::MipmapFilter filter );

private:
	static std::optional<std::vector<uint8_t>> decodeBase64( const std::string &encoded );
};

} // namespace graphics::texture

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <dxgiformat.h>

namespace graphics::texture
{

struct ImageData
{
	std::vector<uint8_t> pixels;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t channels = 4; // Always RGBA
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

private:
	static std::optional<std::vector<uint8_t>> decodeBase64( const std::string &encoded );
};

} // namespace graphics::texture

#include "texture_loader.h"
#include <core/console.h>
#include <array>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace graphics::texture
{

std::optional<ImageData> TextureLoader::loadFromFile( const std::string &path )
{
	int width, height, channels;

	// Force 4 channels (RGBA)
	stbi_uc *pixels = stbi_load( path.c_str(), &width, &height, &channels, 4 );

	if ( !pixels )
	{
		console::error( "Failed to load image: {}", path );
		return std::nullopt;
	}

	ImageData imageData;
	imageData.width = static_cast<uint32_t>( width );
	imageData.height = static_cast<uint32_t>( height );
	imageData.channels = 4;
	imageData.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	const size_t dataSize = width * height * 4;
	imageData.pixels.resize( dataSize );
	std::memcpy( imageData.pixels.data(), pixels, dataSize );

	stbi_image_free( pixels );

	return imageData;
}

std::optional<ImageData> TextureLoader::loadFromMemory( const uint8_t *data, size_t size )
{
	if ( !data || size == 0 )
	{
		console::error( "Invalid memory buffer for image loading" );
		return std::nullopt;
	}

	int width, height, channels;

	// Force 4 channels (RGBA)
	stbi_uc *pixels = stbi_load_from_memory( data, static_cast<int>( size ), &width, &height, &channels, 4 );

	if ( !pixels )
	{
		console::error( "Failed to load image from memory" );
		return std::nullopt;
	}

	ImageData imageData;
	imageData.width = static_cast<uint32_t>( width );
	imageData.height = static_cast<uint32_t>( height );
	imageData.channels = 4;
	imageData.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	const size_t dataSize = width * height * 4;
	imageData.pixels.resize( dataSize );
	std::memcpy( imageData.pixels.data(), pixels, dataSize );

	stbi_image_free( pixels );

	return imageData;
}

std::optional<ImageData> TextureLoader::loadFromDataURI( const std::string &uri )
{
	// Parse data URI format: data:[<mediatype>][;base64],<data>
	constexpr const char *dataPrefix = "data:";
	if ( uri.substr( 0, 5 ) != dataPrefix )
	{
		console::error( "Invalid data URI: missing 'data:' prefix" );
		return std::nullopt;
	}

	// Find the comma that separates metadata from data
	const size_t commaPos = uri.find( ',' );
	if ( commaPos == std::string::npos )
	{
		console::error( "Invalid data URI: missing comma separator" );
		return std::nullopt;
	}

	const std::string metadata = uri.substr( 5, commaPos - 5 ); // Skip "data:"
	const std::string encodedData = uri.substr( commaPos + 1 );

	// Check if it's base64 encoded
	if ( metadata.find( "base64" ) == std::string::npos )
	{
		console::error( "Data URI must be base64 encoded" );
		return std::nullopt;
	}

	// Check if it's an image
	if ( metadata.find( "image/" ) == std::string::npos )
	{
		console::error( "Data URI must be an image type" );
		return std::nullopt;
	}

	// Decode base64
	const auto decodedData = decodeBase64( encodedData );
	if ( !decodedData.has_value() )
	{
		console::error( "Failed to decode base64 data URI" );
		return std::nullopt;
	}

	// Load from decoded memory
	return loadFromMemory( decodedData->data(), decodedData->size() );
}

std::optional<std::vector<uint8_t>> TextureLoader::decodeBase64( const std::string &encoded )
{
	static constexpr const char *base64Chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	// Create lookup table
	std::array<int, 256> lookup{};
	lookup.fill( -1 );
	for ( int i = 0; i < 64; ++i )
	{
		lookup[static_cast<unsigned char>( base64Chars[i] )] = i;
	}

	std::vector<uint8_t> decoded;
	decoded.reserve( ( encoded.size() * 3 ) / 4 );

	uint32_t buffer = 0;
	int bitsCollected = 0;

	for ( const unsigned char c : encoded )
	{
		if ( c == '=' )
		{
			break; // Padding character
		}

		if ( c == ' ' || c == '\n' || c == '\r' || c == '\t' )
		{
			continue; // Skip whitespace
		}

		const int value = lookup[c];
		if ( value == -1 )
		{
			console::error( "Invalid base64 character: {}", static_cast<char>( c ) );
			return std::nullopt;
		}

		buffer = ( buffer << 6 ) | value;
		bitsCollected += 6;

		if ( bitsCollected >= 8 )
		{
			bitsCollected -= 8;
			decoded.push_back( static_cast<uint8_t>( ( buffer >> bitsCollected ) & 0xFF ) );
		}
	}

	return decoded;
}

} // namespace graphics::texture

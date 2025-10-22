#include <catch2/catch_test_macros.hpp>
#include <graphics/texture/texture_loader.h>
#include <graphics/image/image_processor.h>
#include <fstream>
#include <filesystem>

using namespace graphics::texture;
using namespace graphics::image;

TEST_CASE( "TextureLoader loads valid PNG file", "[texture][unit]" )
{
	const std::string testFile = "assets/test/test_red_2x2.png";

	// Verify test file exists
	REQUIRE( std::filesystem::exists( testFile ) );

	const auto result = TextureLoader::loadFromFile( testFile );

	REQUIRE( result.has_value() );
	const auto &imageData = result.value();

	REQUIRE( imageData.width == 2 );
	REQUIRE( imageData.height == 2 );
	REQUIRE( imageData.channels == 4 );
	REQUIRE( imageData.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ); // Actual format from image
	REQUIRE( imageData.pixels.size() == 2 * 2 * 4 );				// 2x2 RGBA

	// First pixel should be red (255, 0, 0, 255)
	REQUIRE( imageData.pixels[0] == 255 ); // R
	REQUIRE( imageData.pixels[1] == 0 );   // G
	REQUIRE( imageData.pixels[2] == 0 );   // B
	REQUIRE( imageData.pixels[3] == 255 ); // A
}

TEST_CASE( "TextureLoader returns nullopt for missing file", "[texture][unit]" )
{
	const auto result = TextureLoader::loadFromFile( "nonexistent_file.png" );

	REQUIRE_FALSE( result.has_value() );
}

TEST_CASE( "TextureLoader returns nullopt for corrupt image", "[texture][unit]" )
{
	// Create a temporary corrupt file
	const std::string corruptFile = "assets/test/corrupt.png";
	{
		std::ofstream out( corruptFile, std::ios::binary );
		out << "This is not a valid PNG file";
	}

	const auto result = TextureLoader::loadFromFile( corruptFile );

	REQUIRE_FALSE( result.has_value() );

	// Cleanup
	std::filesystem::remove( corruptFile );
}

TEST_CASE( "TextureLoader loads from memory buffer", "[texture][unit]" )
{
	// First, load the test PNG to get its raw bytes
	const std::string testFile = "assets/test/test_red_2x2.png";
	std::ifstream file( testFile, std::ios::binary | std::ios::ate );
	REQUIRE( file.is_open() );

	const size_t fileSize = file.tellg();
	file.seekg( 0 );
	std::vector<uint8_t> fileData( fileSize );
	file.read( reinterpret_cast<char *>( fileData.data() ), fileSize );

	// Load from memory
	const auto result = TextureLoader::loadFromMemory( fileData.data(), fileData.size() );

	REQUIRE( result.has_value() );
	const auto &imageData = result.value();

	REQUIRE( imageData.width == 2 );
	REQUIRE( imageData.height == 2 );
	REQUIRE( imageData.channels == 4 );
}

TEST_CASE( "TextureLoader returns nullopt for invalid memory data", "[texture][unit]" )
{
	const std::string invalidData = "Not an image";
	const auto result = TextureLoader::loadFromMemory(
		reinterpret_cast<const uint8_t *>( invalidData.data() ),
		invalidData.size() );

	REQUIRE_FALSE( result.has_value() );
}

TEST_CASE( "TextureLoader loads from data URI", "[texture][unit]" )
{
	// Create a minimal base64-encoded PNG data URI
	// This is a 1x1 red pixel PNG
	const std::string dataUri =
		"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==";

	const auto result = TextureLoader::loadFromDataURI( dataUri );

	REQUIRE( result.has_value() );
	const auto &imageData = result.value();

	REQUIRE( imageData.width == 1 );
	REQUIRE( imageData.height == 1 );
	REQUIRE( imageData.channels == 4 );
}

TEST_CASE( "TextureLoader returns nullopt for invalid data URI", "[texture][unit]" )
{
	const auto result = TextureLoader::loadFromDataURI( "data:image/png;base64,invalid-base64!" );

	REQUIRE_FALSE( result.has_value() );
}

TEST_CASE( "TextureLoader returns nullopt for non-image data URI", "[texture][unit]" )
{
	const auto result = TextureLoader::loadFromDataURI( "data:text/plain;base64,SGVsbG8=" );

	REQUIRE_FALSE( result.has_value() );
}

TEST_CASE( "TextureLoader::loadWithMipmaps generates mipmap chain", "[texture][mipmap]" )
{
	const std::string testFile = "assets/test/test_red_2x2.png";
	REQUIRE( std::filesystem::exists( testFile ) );

	SECTION( "Load with full mipmap chain" )
	{
		const auto result = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );

		REQUIRE( result.has_value() );
		const auto &imageData = result.value();

		// Base level
		REQUIRE( imageData.width == 2 );
		REQUIRE( imageData.height == 2 );
		REQUIRE( imageData.channels == 4 );

		// Should have 1 additional mip level (1x1)
		REQUIRE( imageData.mipLevels.size() == 1 );
		REQUIRE( imageData.mipLevels[0].width == 1 );
		REQUIRE( imageData.mipLevels[0].height == 1 );
		REQUIRE( imageData.mipLevels[0].channels == 4 );
	}

	SECTION( "Load with limited mip levels" )
	{
		const auto result = TextureLoader::loadWithMipmaps( testFile, 1, MipmapFilter::Triangle );

		REQUIRE( result.has_value() );
		const auto &imageData = result.value();

		// Should only have base level (maxLevels=1)
		REQUIRE( imageData.width == 2 );
		REQUIRE( imageData.height == 2 );
		REQUIRE( imageData.mipLevels.empty() );
	}

	SECTION( "Compare with manual mipmap generation" )
	{
		// Load without mipmaps
		const auto baseImage = TextureLoader::loadFromFile( testFile );
		REQUIRE( baseImage.has_value() );

		// Generate mipmaps manually
		const auto manualMips = ImageProcessor::generateMipmaps( baseImage.value(), 0, MipmapFilter::Kaiser );

		// Load with mipmaps
		const auto withMips = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Kaiser );
		REQUIRE( withMips.has_value() );

		// Should match
		REQUIRE( manualMips.size() == ( 1 + withMips->mipLevels.size() ) );
		REQUIRE( withMips->width == manualMips[0].width );
		REQUIRE( withMips->height == manualMips[0].height );

		for ( size_t i = 0; i < withMips->mipLevels.size(); ++i )
		{
			REQUIRE( withMips->mipLevels[i].width == manualMips[i + 1].width );
			REQUIRE( withMips->mipLevels[i].height == manualMips[i + 1].height );
		}
	}
}

TEST_CASE( "TextureLoader::loadWithMipmaps handles invalid file", "[texture][mipmap]" )
{
	const auto result = TextureLoader::loadWithMipmaps( "nonexistent.png", 0, MipmapFilter::Box );
	REQUIRE_FALSE( result.has_value() );
}

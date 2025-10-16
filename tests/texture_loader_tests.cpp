#include <catch2/catch_test_macros.hpp>
#include <graphics/texture/texture_loader.h>
#include <fstream>
#include <filesystem>

using namespace graphics::texture;

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
	REQUIRE( imageData.format == DXGI_FORMAT_R8G8B8A8_UNORM );
	REQUIRE( imageData.pixels.size() == 2 * 2 * 4 ); // 2x2 RGBA

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

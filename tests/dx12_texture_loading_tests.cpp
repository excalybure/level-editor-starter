#include <catch2/catch_test_macros.hpp>
#include <platform/dx12/dx12_device.h>
#include <graphics/texture/texture_loader.h>
#include <filesystem>

using namespace dx12;
using namespace graphics::texture;

TEST_CASE( "Texture creates from ImageData", "[dx12][texture][loading]" )
{
	// Load test image
	const std::string testFile = "assets/test/test_red_2x2.png";
	REQUIRE( std::filesystem::exists( testFile ) );

	const auto imageData = TextureLoader::loadFromFile( testFile );
	REQUIRE( imageData.has_value() );

	// Create device
	Device device;
	REQUIRE( device.initializeHeadless() );

	// Create texture from image data
	Texture texture;
	const bool result = texture.createFromImageData( &device, imageData.value() );

	REQUIRE( result );
	REQUIRE( texture.getWidth() == 2 );
	REQUIRE( texture.getHeight() == 2 );
	REQUIRE( texture.getFormat() == DXGI_FORMAT_R8G8B8A8_UNORM );
	REQUIRE( texture.getResource() != nullptr );

	device.shutdown();
}

TEST_CASE( "Texture createFromImageData validates inputs", "[dx12][texture][loading]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	Texture texture;
	ImageData emptyData;

	// Test with empty image data (width=0)
	const bool result = texture.createFromImageData( &device, emptyData );
	REQUIRE_FALSE( result );

	device.shutdown();
}

TEST_CASE( "Texture createFromImageData requires valid device", "[dx12][texture][loading]" )
{
	// Load test image
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadFromFile( testFile );
	REQUIRE( imageData.has_value() );

	Texture texture;
	const bool result = texture.createFromImageData( nullptr, imageData.value() );

	REQUIRE_FALSE( result );
}

TEST_CASE( "Texture uploads data with staging buffer", "[dx12][texture][loading]" )
{
	// Load test image
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadFromFile( testFile );
	REQUIRE( imageData.has_value() );

	Device device;
	REQUIRE( device.initializeHeadless() );

	// Create texture
	Texture texture;
	REQUIRE( texture.createFromImageData( &device, imageData.value() ) );

	// Upload data
	device.beginFrame();
	const bool uploadResult = texture.uploadTextureData(
		device.getCommandList(),
		imageData->pixels.data(),
		imageData->width * 4,					 // rowPitch (RGBA = 4 bytes per pixel)
		imageData->width * imageData->height * 4 // slicePitch
	);
	device.endFrame();

	REQUIRE( uploadResult );

	device.shutdown();
}

TEST_CASE( "Texture upload validates command list", "[dx12][texture][loading]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	// Load test image
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadFromFile( testFile );
	REQUIRE( imageData.has_value() );

	Texture texture;
	REQUIRE( texture.createFromImageData( &device, imageData.value() ) );

	// Try to upload without valid command list
	const bool result = texture.uploadTextureData(
		nullptr,
		imageData->pixels.data(),
		imageData->width * 4,
		imageData->width * imageData->height * 4 );

	REQUIRE_FALSE( result );

	device.shutdown();
}

TEST_CASE( "Integration: Load PNG and create GPU texture", "[dx12][texture][integration]" )
{
	Device device;
	REQUIRE( device.initializeHeadless() );

	// Load image from file
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadFromFile( testFile );
	REQUIRE( imageData.has_value() );

	// Create GPU texture
	Texture texture;
	REQUIRE( texture.createFromImageData( &device, imageData.value() ) );

	// Upload texture data
	device.beginFrame();
	REQUIRE( texture.uploadTextureData(
		device.getCommandList(),
		imageData->pixels.data(),
		imageData->width * 4,
		imageData->width * imageData->height * 4 ) );
	device.endFrame();

	// Verify final state
	REQUIRE( texture.getResource() != nullptr );
	REQUIRE( texture.getWidth() == 2 );
	REQUIRE( texture.getHeight() == 2 );

	device.shutdown();
}

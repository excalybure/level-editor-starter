#include <catch2/catch_test_macros.hpp>
#include <platform/dx12/dx12_device.h>
#include <graphics/texture/texture_loader.h>
#include <graphics/image/image_processor.h>
#include <filesystem>

using namespace dx12;
using namespace graphics::texture;
using namespace graphics::image;

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
	REQUIRE( texture.getFormat() == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ); // sRGB format for loaded images
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
	const uint32_t bytesPerPixel = imageData->channels;
	const bool uploadResult = texture.uploadTextureData(
		device.getCommandList(),
		imageData->pixels.data(),
		imageData->width * bytesPerPixel,					 // rowPitch
		imageData->width * imageData->height * bytesPerPixel // slicePitch
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
	const uint32_t bytesPerPixel = imageData->channels;
	const bool result = texture.uploadTextureData(
		nullptr,
		imageData->pixels.data(),
		imageData->width * bytesPerPixel,
		imageData->width * imageData->height * bytesPerPixel );

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
	const uint32_t bytesPerPixel = imageData->channels;
	REQUIRE( texture.uploadTextureData(
		device.getCommandList(),
		imageData->pixels.data(),
		imageData->width * bytesPerPixel,
		imageData->width * imageData->height * bytesPerPixel ) );
	device.endFrame();

	// Verify final state
	REQUIRE( texture.getResource() != nullptr );
	REQUIRE( texture.getWidth() == 2 );
	REQUIRE( texture.getHeight() == 2 );

	device.shutdown();
}

TEST_CASE( "ImageData with mipmaps has correct structure", "[dx12][texture][mipmap]" )
{
	// Load image with mipmaps
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );
	REQUIRE( imageData.has_value() );

	// Verify base level
	REQUIRE( imageData->width == 2 );
	REQUIRE( imageData->height == 2 );
	REQUIRE( imageData->channels == 4 );
	REQUIRE( imageData->pixels.size() == 2 * 2 * 4 );

	// Should have 1 additional mip level (1x1)
	REQUIRE( imageData->mipLevels.size() == 1 );
	REQUIRE( imageData->mipLevels[0].width == 1 );
	REQUIRE( imageData->mipLevels[0].height == 1 );
	REQUIRE( imageData->mipLevels[0].channels == 4 );
	REQUIRE( imageData->mipLevels[0].pixels.size() == 1 * 1 * 4 );
}

TEST_CASE( "Texture creates resource with mip levels", "[dx12][texture][mipmap][phase2.3]" )
{
	// Load test image with mipmaps
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );
	REQUIRE( imageData.has_value() );

	// Create device
	Device device;
	REQUIRE( device.initializeHeadless() );

	// Create texture from mipmapped image data
	Texture texture;
	const bool result = texture.createFromImageData( &device, imageData.value() );

	// Verify texture created with correct mip level count (base + 1 additional = 2 total)
	REQUIRE( result );
	REQUIRE( texture.getWidth() == 2 );
	REQUIRE( texture.getHeight() == 2 );
	REQUIRE( texture.getMipLevels() == 2 ); // This will fail - getMipLevels() doesn't exist yet

	device.shutdown();
}

TEST_CASE( "Texture uploads all mip levels", "[dx12][texture][mipmap][phase2.3]" )
{
	// Load test image with mipmaps
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );
	REQUIRE( imageData.has_value() );

	// Create device
	Device device;
	REQUIRE( device.initializeHeadless() );

	// Create texture from mipmapped image data
	Texture texture;
	REQUIRE( texture.createFromImageData( &device, imageData.value() ) );

	// Upload all mip levels (base + additional)
	device.beginFrame();
	const bool result = texture.uploadAllMipLevels( device.getCommandList(), imageData.value() );
	device.endFrame();

	REQUIRE( result ); // This will fail - uploadAllMipLevels() doesn't exist yet

	device.shutdown();
}

TEST_CASE( "Texture SRV exposes all mip levels", "[dx12][texture][mipmap][phase2.3]" )
{
	// Load test image with mipmaps
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );
	REQUIRE( imageData.has_value() );

	// Create device and texture manager
	Device device;
	REQUIRE( device.initializeHeadless() );

	TextureManager textureManager;
	REQUIRE( textureManager.initialize( &device ) );

	// Create texture
	auto texture = std::make_shared<Texture>();
	REQUIRE( texture->createFromImageData( &device, imageData.value() ) );

	// Upload all mip levels
	device.beginFrame();
	REQUIRE( texture->uploadAllMipLevels( device.getCommandList(), imageData.value() ) );
	device.endFrame();

	// Create SRV (this should respect mip levels)
	const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = textureManager.getNextSrvHandle();
	REQUIRE( texture->createShaderResourceView( &device, srvHandle ) );

	// Verify texture has proper mip levels exposed
	REQUIRE( texture->getMipLevels() == 2 ); // Base + 1 additional
	REQUIRE( srvHandle.ptr != 0 );			 // SRV handle was allocated

	textureManager.shutdown();
	device.shutdown();
}

TEST_CASE( "Integration: TextureManager creates mipmapped texture from file", "[dx12][texture][mipmap][phase2.3][integration]" )
{
	// Create device and texture manager
	Device device;
	REQUIRE( device.initializeHeadless() );

	TextureManager textureManager;
	REQUIRE( textureManager.initialize( &device ) );

	// Load test image with mipmaps (2x2 -> 1x1 = 2 mip levels)
	const std::string testFile = "assets/test/test_red_2x2.png";
	const auto imageData = TextureLoader::loadWithMipmaps( testFile, 0, MipmapFilter::Box );
	REQUIRE( imageData.has_value() );
	REQUIRE( imageData->mipLevels.size() == 1 ); // 1 additional mip beyond base

	// Create texture via the standard path
	auto texture = std::make_shared<Texture>();
	REQUIRE( texture->createFromImageData( &device, imageData.value() ) );

	// Upload via new method
	device.beginFrame();
	const bool uploadResult = texture->uploadAllMipLevels( device.getCommandList(), imageData.value() );
	device.endFrame();
	REQUIRE( uploadResult );

	// Create SRV
	const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = textureManager.getNextSrvHandle();
	REQUIRE( texture->createShaderResourceView( &device, srvHandle ) );

	// Verify final texture state
	REQUIRE( texture->getWidth() == 2 );
	REQUIRE( texture->getHeight() == 2 );
	REQUIRE( texture->getMipLevels() == 2 );
	REQUIRE( texture->getResource() != nullptr );

	textureManager.shutdown();
	device.shutdown();
}

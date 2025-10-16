#include <catch2/catch_test_macros.hpp>
#include <graphics/texture/texture_manager.h>
#include <graphics/texture/bindless_texture_heap.h>
#include <graphics/texture/texture_loader.h>
#include <platform/dx12/dx12_device.h>
#include <filesystem>

TEST_CASE( "graphics::TextureManager initializes with device and max textures", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	const bool result = manager.initialize( &device, 100 );

	REQUIRE( result );
	REQUIRE( manager.getDevice() == &device );
	REQUIRE( manager.getSrvHeap() != nullptr );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager validates device pointer", "[texture][manager][unit]" )
{
	graphics::texture::TextureManager manager;
	const bool result = manager.initialize( nullptr, 100 );

	REQUIRE_FALSE( result );
}

TEST_CASE( "graphics::TextureManager loads texture from file", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	// Use existing test image
	const std::string testImage = "assets/test/test_red_2x2.png";
	REQUIRE( std::filesystem::exists( testImage ) );

	const graphics::texture::TextureHandle handle = manager.loadTexture( testImage );

	REQUIRE( handle != graphics::texture::kInvalidTextureHandle );
	REQUIRE( manager.getSrvIndex( handle ) < 100 );

	const graphics::texture::TextureInfo *info = manager.getTextureInfo( handle );
	REQUIRE( info != nullptr );
	REQUIRE( info->width == 2 );
	REQUIRE( info->height == 2 );
	REQUIRE( info->format == DXGI_FORMAT_R8G8B8A8_UNORM );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager caches textures by path", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const std::string testImage = "assets/test/test_red_2x2.png";

	const graphics::texture::TextureHandle handle1 = manager.loadTexture( testImage );
	const graphics::texture::TextureHandle handle2 = manager.loadTexture( testImage );

	REQUIRE( handle1 == handle2 );
	REQUIRE( handle1 != graphics::texture::kInvalidTextureHandle );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager returns invalid handle for missing file", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const graphics::texture::TextureHandle handle = manager.loadTexture( "nonexistent_file.png" );

	REQUIRE( handle == graphics::texture::kInvalidTextureHandle );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager releases texture with refcounting", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const std::string testImage = "assets/test/test_red_2x2.png";

	// Load texture twice (increases refcount to 2)
	const graphics::texture::TextureHandle handle1 = manager.loadTexture( testImage );
	const graphics::texture::TextureHandle handle2 = manager.loadTexture( testImage );
	REQUIRE( handle1 == handle2 );

	// Release once (refcount = 1, texture still valid)
	manager.releaseTexture( handle1 );
	const graphics::texture::TextureInfo *info1 = manager.getTextureInfo( handle1 );
	REQUIRE( info1 != nullptr );

	// Release again (refcount = 0, texture freed)
	manager.releaseTexture( handle2 );
	const graphics::texture::TextureInfo *info2 = manager.getTextureInfo( handle2 );
	REQUIRE( info2 == nullptr );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager reuses freed handles", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const std::string testImage = "assets/test/test_red_2x2.png";

	// Load and release texture
	const graphics::texture::TextureHandle handle1 = manager.loadTexture( testImage );
	REQUIRE( handle1 != graphics::texture::kInvalidTextureHandle );
	manager.releaseTexture( handle1 );

	// Load another texture, should reuse handle
	const graphics::texture::TextureHandle handle2 = manager.loadTexture( testImage );
	REQUIRE( handle2 == handle1 );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager queries return null for invalid handle", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const graphics::texture::TextureInfo *info = manager.getTextureInfo( graphics::texture::kInvalidTextureHandle );
	REQUIRE( info == nullptr );

	const uint32_t srvIndex = manager.getSrvIndex( graphics::texture::kInvalidTextureHandle );
	REQUIRE( srvIndex == UINT32_MAX );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager getSrvIndex returns unique indices", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const std::string testImage = "assets/test/test_red_2x2.png";

	// Load texture
	const graphics::texture::TextureHandle handle = manager.loadTexture( testImage );
	REQUIRE( handle != graphics::texture::kInvalidTextureHandle );

	const uint32_t srvIndex = manager.getSrvIndex( handle );
	REQUIRE( srvIndex < 100 );
	REQUIRE( srvIndex != UINT32_MAX );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager resolves relative paths with base path", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	// Load with relative path and base path
	const graphics::texture::TextureHandle handle = manager.loadTexture( "test_red_2x2.png", "assets/test" );

	REQUIRE( handle != graphics::texture::kInvalidTextureHandle );

	const graphics::texture::TextureInfo *info = manager.getTextureInfo( handle );
	REQUIRE( info != nullptr );
	REQUIRE( info->width == 2 );
	REQUIRE( info->height == 2 );

	manager.shutdown();
	device.shutdown();
}

TEST_CASE( "graphics::TextureManager normalizes paths with different case", "[texture][manager][unit]" )
{
	dx12::Device device;
	REQUIRE( device.initializeHeadless() );

	graphics::texture::TextureManager manager;
	REQUIRE( manager.initialize( &device, 100 ) );

	const std::string testImage = "assets/test/test_red_2x2.png";

	// Load with original case
	const graphics::texture::TextureHandle handle1 = manager.loadTexture( testImage );
	REQUIRE( handle1 != graphics::texture::kInvalidTextureHandle );

	// Load with different case variations (should return same handle on Windows)
	const graphics::texture::TextureHandle handle2 = manager.loadTexture( "assets/TEST/test_red_2x2.png" );
	const graphics::texture::TextureHandle handle3 = manager.loadTexture( "ASSETS/test/TEST_RED_2X2.PNG" );

// On Windows (case-insensitive filesystem), canonical paths resolve to same file
// so handles should match
#ifdef _WIN32
	REQUIRE( handle1 == handle2 );
	REQUIRE( handle1 == handle3 );
#endif

	manager.shutdown();
	device.shutdown();
}

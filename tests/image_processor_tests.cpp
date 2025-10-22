#include <catch2/catch_test_macros.hpp>
#include "graphics/image/image_processor.h"
#include "graphics/texture/texture_loader.h"

using namespace graphics::image;
using namespace graphics::texture;

// Helper function to create test image data
static ImageData createTestImage( uint32_t width, uint32_t height, uint32_t channels )
{
	ImageData img;
	img.width = width;
	img.height = height;
	img.channels = channels;
	img.format = DXGI_FORMAT_R8G8B8A8_UNORM; // Assume RGBA for tests
	img.pixels.resize( width * height * channels );

	// Fill with a simple pattern (gradient)
	for ( uint32_t y = 0; y < height; ++y )
	{
		for ( uint32_t x = 0; x < width; ++x )
		{
			const uint32_t idx = ( y * width + x ) * channels;
			img.pixels[idx + 0] = static_cast<uint8_t>( ( x * 255 ) / width ); // R
			if ( channels > 1 )
				img.pixels[idx + 1] = static_cast<uint8_t>( ( y * 255 ) / height ); // G
			if ( channels > 2 )
				img.pixels[idx + 2] = 128; // B
			if ( channels > 3 )
				img.pixels[idx + 3] = 255; // A
		}
	}

	return img;
}

TEST_CASE( "ImageProcessor::resize downscales correctly", "[image][resize]" )
{
	// Arrange: Create 256x256 4-channel test image
	const auto source = createTestImage( 256, 256, 4 );

	// Act: Resize to 64x64
	const auto result = ImageProcessor::resize( source, 64, 64, ResizeFilter::Bilinear );

	// Assert
	REQUIRE( result.has_value() );
	REQUIRE( result->width == 64 );
	REQUIRE( result->height == 64 );
	REQUIRE( result->channels == 4 );
	REQUIRE( result->pixels.size() == 64 * 64 * 4 );
}

TEST_CASE( "ImageProcessor::resize upscales correctly", "[image][resize]" )
{
	// Arrange: Create 64x64 4-channel test image
	const auto source = createTestImage( 64, 64, 4 );

	// Act: Resize to 128x128
	const auto result = ImageProcessor::resize( source, 128, 128, ResizeFilter::Bilinear );

	// Assert
	REQUIRE( result.has_value() );
	REQUIRE( result->width == 128 );
	REQUIRE( result->height == 128 );
	REQUIRE( result->channels == 4 );
	REQUIRE( result->pixels.size() == 128 * 128 * 4 );
}

TEST_CASE( "ImageProcessor::resize handles different channel counts", "[image][resize]" )
{
	SECTION( "1 channel (grayscale)" )
	{
		const auto source = createTestImage( 100, 100, 1 );
		const auto result = ImageProcessor::resize( source, 50, 50, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 50 );
		REQUIRE( result->height == 50 );
		REQUIRE( result->channels == 1 );
		REQUIRE( result->pixels.size() == 50 * 50 * 1 );
	}

	SECTION( "3 channels (RGB)" )
	{
		const auto source = createTestImage( 100, 100, 3 );
		const auto result = ImageProcessor::resize( source, 50, 50, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 50 );
		REQUIRE( result->height == 50 );
		REQUIRE( result->channels == 3 );
		REQUIRE( result->pixels.size() == 50 * 50 * 3 );
	}
}

TEST_CASE( "ImageProcessor::resize handles invalid inputs", "[image][resize]" )
{
	SECTION( "Empty source image" )
	{
		ImageData empty;
		empty.width = 0;
		empty.height = 0;
		empty.channels = 4;

		const auto result = ImageProcessor::resize( empty, 64, 64, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result.has_value() );
	}

	SECTION( "Zero target dimensions" )
	{
		const auto source = createTestImage( 100, 100, 4 );

		const auto result1 = ImageProcessor::resize( source, 0, 64, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result1.has_value() );

		const auto result2 = ImageProcessor::resize( source, 64, 0, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result2.has_value() );
	}

	SECTION( "Invalid channel count" )
	{
		ImageData invalid;
		invalid.width = 100;
		invalid.height = 100;
		invalid.channels = 0;
		invalid.pixels.resize( 100 * 100 * 4 );

		const auto result = ImageProcessor::resize( invalid, 50, 50, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result.has_value() );
	}
}

TEST_CASE( "ImageProcessor::resizeAspect maintains aspect ratio", "[image][resize]" )
{
	SECTION( "Wide image fits within box" )
	{
		// 200x100 image (2:1 aspect) -> max 100x100 box -> should be 100x50
		const auto source = createTestImage( 200, 100, 4 );
		const auto result = ImageProcessor::resizeAspect( source, 100, 100, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 100 );
		REQUIRE( result->height == 50 );
		REQUIRE( result->channels == 4 );
	}

	SECTION( "Tall image fits within box" )
	{
		// 100x200 image (1:2 aspect) -> max 100x100 box -> should be 50x100
		const auto source = createTestImage( 100, 200, 4 );
		const auto result = ImageProcessor::resizeAspect( source, 100, 100, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 50 );
		REQUIRE( result->height == 100 );
		REQUIRE( result->channels == 4 );
	}

	SECTION( "Square image fits exactly" )
	{
		// 128x128 image -> max 64x64 box -> should be 64x64
		const auto source = createTestImage( 128, 128, 4 );
		const auto result = ImageProcessor::resizeAspect( source, 64, 64, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 64 );
		REQUIRE( result->height == 64 );
		REQUIRE( result->channels == 4 );
	}

	SECTION( "Image smaller than box is not upscaled" )
	{
		// 50x50 image -> max 100x100 box -> stays 50x50 (no upscale)
		const auto source = createTestImage( 50, 50, 4 );
		const auto result = ImageProcessor::resizeAspect( source, 100, 100, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 50 );
		REQUIRE( result->height == 50 );
	}

	SECTION( "Very wide image" )
	{
		// 800x100 image (8:1 aspect) -> max 200x200 box -> should be 200x25
		const auto source = createTestImage( 800, 100, 3 );
		const auto result = ImageProcessor::resizeAspect( source, 200, 200, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 200 );
		REQUIRE( result->height == 25 );
		REQUIRE( result->channels == 3 );
	}
}

TEST_CASE( "ImageProcessor::createThumbnail generates square output", "[image][thumbnail]" )
{
	SECTION( "Wide image crops to square" )
	{
		// 200x100 image -> 64x64 thumbnail (crops width)
		const auto source = createTestImage( 200, 100, 4 );
		const auto result = ImageProcessor::createThumbnail( source, 64, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 64 );
		REQUIRE( result->height == 64 );
		REQUIRE( result->channels == 4 );
	}

	SECTION( "Tall image crops to square" )
	{
		// 100x200 image -> 64x64 thumbnail (crops height)
		const auto source = createTestImage( 100, 200, 4 );
		const auto result = ImageProcessor::createThumbnail( source, 64, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 64 );
		REQUIRE( result->height == 64 );
		REQUIRE( result->channels == 4 );
	}

	SECTION( "Square image resizes to thumbnail" )
	{
		// 256x256 image -> 100x100 thumbnail
		const auto source = createTestImage( 256, 256, 3 );
		const auto result = ImageProcessor::createThumbnail( source, 100, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 100 );
		REQUIRE( result->height == 100 );
		REQUIRE( result->channels == 3 );
	}

	SECTION( "Small image upscales to thumbnail" )
	{
		// 32x32 image -> 64x64 thumbnail
		const auto source = createTestImage( 32, 32, 4 );
		const auto result = ImageProcessor::createThumbnail( source, 64, ResizeFilter::Bilinear );

		REQUIRE( result.has_value() );
		REQUIRE( result->width == 64 );
		REQUIRE( result->height == 64 );
	}

	SECTION( "Invalid inputs return nullopt" )
	{
		const auto source = createTestImage( 100, 100, 4 );

		// Zero size
		const auto result1 = ImageProcessor::createThumbnail( source, 0, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result1.has_value() );

		// Empty source
		ImageData empty;
		empty.width = 0;
		empty.height = 0;
		empty.channels = 4;
		const auto result2 = ImageProcessor::createThumbnail( empty, 64, ResizeFilter::Bilinear );
		REQUIRE_FALSE( result2.has_value() );
	}
}

TEST_CASE( "ImageProcessor::calculateMipLevels computes correct level count", "[image][mipmap]" )
{
	SECTION( "Square power-of-two textures" )
	{
		REQUIRE( ImageProcessor::calculateMipLevels( 1, 1 ) == 1 );
		REQUIRE( ImageProcessor::calculateMipLevels( 2, 2 ) == 2 );
		REQUIRE( ImageProcessor::calculateMipLevels( 4, 4 ) == 3 );
		REQUIRE( ImageProcessor::calculateMipLevels( 8, 8 ) == 4 );
		REQUIRE( ImageProcessor::calculateMipLevels( 256, 256 ) == 9 );
		REQUIRE( ImageProcessor::calculateMipLevels( 512, 512 ) == 10 );
	}

	SECTION( "Non-square textures use max dimension" )
	{
		REQUIRE( ImageProcessor::calculateMipLevels( 4, 2 ) == 3 );		// max=4 -> 3 levels
		REQUIRE( ImageProcessor::calculateMipLevels( 2, 4 ) == 3 );		// max=4 -> 3 levels
		REQUIRE( ImageProcessor::calculateMipLevels( 256, 128 ) == 9 ); // max=256 -> 9 levels
		REQUIRE( ImageProcessor::calculateMipLevels( 128, 256 ) == 9 ); // max=256 -> 9 levels
	}

	SECTION( "Non-power-of-two dimensions" )
	{
		REQUIRE( ImageProcessor::calculateMipLevels( 3, 3 ) == 2 );		// floor(log2(3))+1 = 1+1=2
		REQUIRE( ImageProcessor::calculateMipLevels( 5, 5 ) == 3 );		// floor(log2(5))+1 ≈2+1=3
		REQUIRE( ImageProcessor::calculateMipLevels( 100, 100 ) == 7 ); // floor(log2(100))+1 ≈6+1=7
	}
}

TEST_CASE( "ImageProcessor::generateMipmaps creates correct chain", "[image][mipmap]" )
{
	SECTION( "Full mipmap chain for 4x4 image" )
	{
		const auto source = createTestImage( 4, 4, 4 );
		const auto mipmaps = ImageProcessor::generateMipmaps( source, 0, MipmapFilter::Box );

		REQUIRE( mipmaps.size() == 3 ); // 4x4, 2x2, 1x1
		REQUIRE( mipmaps[0].width == 4 );
		REQUIRE( mipmaps[0].height == 4 );
		REQUIRE( mipmaps[1].width == 2 );
		REQUIRE( mipmaps[1].height == 2 );
		REQUIRE( mipmaps[2].width == 1 );
		REQUIRE( mipmaps[2].height == 1 );
		REQUIRE( mipmaps[0].channels == 4 );
		REQUIRE( mipmaps[1].channels == 4 );
		REQUIRE( mipmaps[2].channels == 4 );
	}

	SECTION( "Limited mipmap levels" )
	{
		const auto source = createTestImage( 8, 8, 3 );
		const auto mipmaps = ImageProcessor::generateMipmaps( source, 2, MipmapFilter::Triangle );

		REQUIRE( mipmaps.size() == 2 ); // Only 8x8 and 4x4
		REQUIRE( mipmaps[0].width == 8 );
		REQUIRE( mipmaps[0].height == 8 );
		REQUIRE( mipmaps[1].width == 4 );
		REQUIRE( mipmaps[1].height == 4 );
	}

	SECTION( "Non-square image uses max dimension for levels" )
	{
		const auto source = createTestImage( 8, 4, 4 ); // max=8, levels=4 (8,4,2,1)
		const auto mipmaps = ImageProcessor::generateMipmaps( source, 0, MipmapFilter::Box );

		REQUIRE( mipmaps.size() == 4 );
		REQUIRE( mipmaps[0].width == 8 );
		REQUIRE( mipmaps[0].height == 4 );
		REQUIRE( mipmaps[1].width == 4 );
		REQUIRE( mipmaps[1].height == 2 );
		REQUIRE( mipmaps[2].width == 2 );
		REQUIRE( mipmaps[2].height == 1 );
		REQUIRE( mipmaps[3].width == 1 );
		REQUIRE( mipmaps[3].height == 1 );
	}

	SECTION( "Invalid inputs return empty vector" )
	{
		ImageData empty;
		empty.width = 0;
		empty.height = 0;
		empty.channels = 4;

		const auto mipmaps = ImageProcessor::generateMipmaps( empty, 0, MipmapFilter::Box );
		REQUIRE( mipmaps.empty() );
	}
}

TEST_CASE( "ImageData can store mipmap chain", "[image][mipmap][imagedata]" )
{
	SECTION( "ImageData with mipLevels stores additional mips" )
	{
		ImageData baseImage;
		baseImage.width = 4;
		baseImage.height = 4;
		baseImage.channels = 4;
		baseImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		baseImage.pixels.resize( 4 * 4 * 4 );

		// Add mip level 1 (2x2)
		ImageData mip1;
		mip1.width = 2;
		mip1.height = 2;
		mip1.channels = 4;
		mip1.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		mip1.pixels.resize( 2 * 2 * 4 );

		// Add mip level 2 (1x1)
		ImageData mip2;
		mip2.width = 1;
		mip2.height = 1;
		mip2.channels = 4;
		mip2.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		mip2.pixels.resize( 1 * 1 * 4 );

		baseImage.mipLevels.push_back( mip1 );
		baseImage.mipLevels.push_back( mip2 );

		REQUIRE( baseImage.mipLevels.size() == 2 );
		REQUIRE( baseImage.mipLevels[0].width == 2 );
		REQUIRE( baseImage.mipLevels[0].height == 2 );
		REQUIRE( baseImage.mipLevels[1].width == 1 );
		REQUIRE( baseImage.mipLevels[1].height == 1 );
	}

	SECTION( "Empty mipLevels indicates single-level texture" )
	{
		ImageData singleLevel;
		singleLevel.width = 8;
		singleLevel.height = 8;
		singleLevel.channels = 3;
		singleLevel.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		singleLevel.pixels.resize( 8 * 8 * 3 );

		REQUIRE( singleLevel.mipLevels.empty() );
	}
}

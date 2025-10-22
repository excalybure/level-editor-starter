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

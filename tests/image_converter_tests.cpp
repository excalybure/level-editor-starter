#include <catch2/catch_test_macros.hpp>
#include "graphics/image/image_converter.h"
#include "graphics/texture/texture_loader.h"

using namespace graphics::image;
using namespace graphics::texture;

TEST_CASE( "ImageConverter::extractChannel extracts R channel", "[image][converter][channel]" )
{
	// Arrange: Create RGBA image with known values
	ImageData source;
	source.width = 2;
	source.height = 2;
	source.channels = 4;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		255, 0, 0, 255, // R=255, G=0, B=0, A=255
		128,
		64,
		32,
		200, // R=128, G=64, B=32, A=200
		0,
		255,
		0,
		100, // R=0, G=255, B=0, A=100
		50,
		100,
		150,
		50 // R=50, G=100, B=150, A=50
	};

	// Act: Extract R channel
	const auto result = ImageConverter::extractChannel( source, Channel::R );

	// Assert: Should succeed and contain R channel values
	REQUIRE( result.has_value() );
	REQUIRE( result->width == 2 );
	REQUIRE( result->height == 2 );
	REQUIRE( result->channels == 1 );
	REQUIRE( result->format == DXGI_FORMAT_R8_UNORM );
	REQUIRE( result->pixels.size() == 4 );
	REQUIRE( result->pixels[0] == 255 );
	REQUIRE( result->pixels[1] == 128 );
	REQUIRE( result->pixels[2] == 0 );
	REQUIRE( result->pixels[3] == 50 );
}

TEST_CASE( "ImageConverter::extractChannel extracts G, B, A channels", "[image][converter][channel]" )
{
	// Arrange: Create RGBA image with distinct channel values
	ImageData source;
	source.width = 2;
	source.height = 1;
	source.channels = 4;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		10, 20, 30, 40, // R=10, G=20, B=30, A=40
		50,
		60,
		70,
		80 // R=50, G=60, B=70, A=80
	};

	SECTION( "Extract G channel" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::G );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 1 );
		REQUIRE( result->pixels[0] == 20 );
		REQUIRE( result->pixels[1] == 60 );
	}

	SECTION( "Extract B channel" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::B );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 1 );
		REQUIRE( result->pixels[0] == 30 );
		REQUIRE( result->pixels[1] == 70 );
	}

	SECTION( "Extract A channel" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::A );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 1 );
		REQUIRE( result->pixels[0] == 40 );
		REQUIRE( result->pixels[1] == 80 );
	}
}

TEST_CASE( "ImageConverter::extractChannel handles RGB images", "[image][converter][channel]" )
{
	// Arrange: RGB image (no alpha)
	ImageData source;
	source.width = 2;
	source.height = 1;
	source.channels = 3;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM; // Format doesn't affect extraction
	source.pixels = {
		100, 110, 120, // R=100, G=110, B=120
		200,
		210,
		220 // R=200, G=210, B=220
	};

	SECTION( "Extract R from RGB" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::R );
		REQUIRE( result.has_value() );
		REQUIRE( result->pixels[0] == 100 );
		REQUIRE( result->pixels[1] == 200 );
	}

	SECTION( "Extract B from RGB" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::B );
		REQUIRE( result.has_value() );
		REQUIRE( result->pixels[0] == 120 );
		REQUIRE( result->pixels[1] == 220 );
	}

	SECTION( "Requesting A from RGB fails" )
	{
		const auto result = ImageConverter::extractChannel( source, Channel::A );
		REQUIRE_FALSE( result.has_value() );
	}
}

TEST_CASE( "ImageConverter::extractChannel validates inputs", "[image][converter][channel][error]" )
{
	SECTION( "Empty image" )
	{
		ImageData source;
		const auto result = ImageConverter::extractChannel( source, Channel::R );
		REQUIRE_FALSE( result.has_value() );
	}

	SECTION( "Zero dimensions" )
	{
		ImageData source;
		source.channels = 4;
		source.pixels.resize( 16 );
		const auto result = ImageConverter::extractChannel( source, Channel::R );
		REQUIRE_FALSE( result.has_value() );
	}

	SECTION( "Channel out of range" )
	{
		ImageData source;
		source.width = 1;
		source.height = 1;
		source.channels = 3;
		source.pixels = { 100, 110, 120 };

		const auto result = ImageConverter::extractChannel( source, Channel::A );
		REQUIRE_FALSE( result.has_value() );
	}
}

TEST_CASE( "ImageConverter::packChannels packs single R channel", "[image][converter][pack]" )
{
	// Arrange: Single-channel R image
	ImageData r;
	r.width = 2;
	r.height = 1;
	r.channels = 1;
	r.format = DXGI_FORMAT_R8_UNORM;
	r.pixels = { 100, 200 };

	// Act: Pack R only
	const auto result = ImageConverter::packChannels( &r, nullptr, nullptr, nullptr );

	// Assert: Should create R8 image
	REQUIRE( result.has_value() );
	REQUIRE( result->width == 2 );
	REQUIRE( result->height == 1 );
	REQUIRE( result->channels == 1 );
	REQUIRE( result->format == DXGI_FORMAT_R8_UNORM );
	REQUIRE( result->pixels.size() == 2 );
	REQUIRE( result->pixels[0] == 100 );
	REQUIRE( result->pixels[1] == 200 );
}

TEST_CASE( "ImageConverter::packChannels packs multiple channels", "[image][converter][pack]" )
{
	// Arrange: Create separate R, G, B, A channels
	ImageData r, g, b, a;
	r.width = g.width = b.width = a.width = 2;
	r.height = g.height = b.height = a.height = 1;
	r.channels = g.channels = b.channels = a.channels = 1;
	r.format = g.format = b.format = a.format = DXGI_FORMAT_R8_UNORM;
	r.pixels = { 10, 20 };
	g.pixels = { 30, 40 };
	b.pixels = { 50, 60 };
	a.pixels = { 70, 80 };

	SECTION( "Pack RG" )
	{
		const auto result = ImageConverter::packChannels( &r, &g, nullptr, nullptr );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 2 );
		REQUIRE( result->format == DXGI_FORMAT_R8G8_UNORM );
		REQUIRE( result->pixels.size() == 4 ); // 2 pixels * 2 channels
		REQUIRE( result->pixels[0] == 10 );	   // R[0]
		REQUIRE( result->pixels[1] == 30 );	   // G[0]
		REQUIRE( result->pixels[2] == 20 );	   // R[1]
		REQUIRE( result->pixels[3] == 40 );	   // G[1]
	}

	SECTION( "Pack RGB" )
	{
		const auto result = ImageConverter::packChannels( &r, &g, &b, nullptr );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 3 );
		REQUIRE( result->format == DXGI_FORMAT_R8G8B8A8_UNORM ); // No RGB8 format
		REQUIRE( result->pixels.size() == 6 );					 // 2 pixels * 3 channels
		REQUIRE( result->pixels[0] == 10 );						 // R[0]
		REQUIRE( result->pixels[1] == 30 );						 // G[0]
		REQUIRE( result->pixels[2] == 50 );						 // B[0]
		REQUIRE( result->pixels[3] == 20 );						 // R[1]
		REQUIRE( result->pixels[4] == 40 );						 // G[1]
		REQUIRE( result->pixels[5] == 60 );						 // B[1]
	}

	SECTION( "Pack RGBA" )
	{
		const auto result = ImageConverter::packChannels( &r, &g, &b, &a );
		REQUIRE( result.has_value() );
		REQUIRE( result->channels == 4 );
		REQUIRE( result->format == DXGI_FORMAT_R8G8B8A8_UNORM );
		REQUIRE( result->pixels.size() == 8 ); // 2 pixels * 4 channels
		REQUIRE( result->pixels[0] == 10 );	   // R[0]
		REQUIRE( result->pixels[1] == 30 );	   // G[0]
		REQUIRE( result->pixels[2] == 50 );	   // B[0]
		REQUIRE( result->pixels[3] == 70 );	   // A[0]
		REQUIRE( result->pixels[4] == 20 );	   // R[1]
		REQUIRE( result->pixels[5] == 40 );	   // G[1]
		REQUIRE( result->pixels[6] == 60 );	   // B[1]
		REQUIRE( result->pixels[7] == 80 );	   // A[1]
	}
}

TEST_CASE( "ImageConverter::packChannels validates inputs", "[image][converter][pack][error]" )
{
	ImageData r;
	r.width = 2;
	r.height = 1;
	r.channels = 1;
	r.pixels = { 100, 200 };

	SECTION( "Null R channel fails" )
	{
		const auto result = ImageConverter::packChannels( nullptr, nullptr, nullptr, nullptr );
		REQUIRE_FALSE( result.has_value() );
	}

	SECTION( "Mismatched dimensions fail" )
	{
		ImageData g;
		g.width = 3; // Different width
		g.height = 1;
		g.channels = 1;
		g.pixels = { 1, 2, 3 };

		const auto result = ImageConverter::packChannels( &r, &g, nullptr, nullptr );
		REQUIRE_FALSE( result.has_value() );
	}

	SECTION( "Multi-channel input fails" )
	{
		ImageData rgb;
		rgb.width = 2;
		rgb.height = 1;
		rgb.channels = 3; // Not single-channel
		rgb.pixels = { 1, 2, 3, 4, 5, 6 };

		const auto result = ImageConverter::packChannels( &rgb, nullptr, nullptr, nullptr );
		REQUIRE_FALSE( result.has_value() );
	}
}

TEST_CASE( "ImageConverter::swizzle rearranges RGBA channels", "[image][converter][swizzle]" )
{
	// Arrange: RGBA image with distinct channel values
	ImageData source;
	source.width = 2;
	source.height = 1;
	source.channels = 4;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		10, 20, 30, 40, // R=10, G=20, B=30, A=40
		50,
		60,
		70,
		80 // R=50, G=60, B=70, A=80
	};

	// Act: Swizzle to BGRA
	const auto result = ImageConverter::swizzle( source, Channel::B, Channel::G, Channel::R, Channel::A );

	// Assert: Channels swapped
	REQUIRE( result.has_value() );
	REQUIRE( result->width == 2 );
	REQUIRE( result->height == 1 );
	REQUIRE( result->channels == 4 );
	REQUIRE( result->pixels[0] == 30 ); // B from source
	REQUIRE( result->pixels[1] == 20 ); // G from source
	REQUIRE( result->pixels[2] == 10 ); // R from source
	REQUIRE( result->pixels[3] == 40 ); // A from source
	REQUIRE( result->pixels[4] == 70 ); // B from source
	REQUIRE( result->pixels[5] == 60 ); // G from source
	REQUIRE( result->pixels[6] == 50 ); // R from source
	REQUIRE( result->pixels[7] == 80 ); // A from source
}

TEST_CASE( "ImageConverter::addAlpha converts RGB to RGBA", "[image][converter][format]" )
{
	// Arrange: RGB image
	ImageData source;
	source.width = 2;
	source.height = 1;
	source.channels = 3;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		10, 20, 30, // RGB
		40,
		50,
		60 // RGB
	};

	// Act: Add alpha=200
	const auto result = ImageConverter::addAlpha( source, 200 );

	// Assert: RGBA with alpha=200
	REQUIRE( result.has_value() );
	REQUIRE( result->channels == 4 );
	REQUIRE( result->pixels.size() == 8 );
	REQUIRE( result->pixels[0] == 10 );
	REQUIRE( result->pixels[1] == 20 );
	REQUIRE( result->pixels[2] == 30 );
	REQUIRE( result->pixels[3] == 200 ); // Added alpha
	REQUIRE( result->pixels[7] == 200 ); // Added alpha
}

TEST_CASE( "ImageConverter::discardAlpha converts RGBA to RGB", "[image][converter][format]" )
{
	// Arrange: RGBA image
	ImageData source;
	source.width = 2;
	source.height = 1;
	source.channels = 4;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		10, 20, 30, 255, // RGBA
		40,
		50,
		60,
		128 // RGBA
	};

	// Act: Discard alpha
	const auto result = ImageConverter::discardAlpha( source );

	// Assert: RGB only
	REQUIRE( result.has_value() );
	REQUIRE( result->channels == 3 );
	REQUIRE( result->pixels.size() == 6 );
	REQUIRE( result->pixels[0] == 10 );
	REQUIRE( result->pixels[1] == 20 );
	REQUIRE( result->pixels[2] == 30 );
	REQUIRE( result->pixels[3] == 40 );
	REQUIRE( result->pixels[4] == 50 );
	REQUIRE( result->pixels[5] == 60 );
}

TEST_CASE( "ImageConverter::toGrayscale converts RGB to luminance", "[image][converter][format]" )
{
	// Arrange: RGB image with known luminance values
	// Luminance = 0.299*R + 0.587*G + 0.114*B
	// Pure red (255,0,0) = 76
	// Pure green (0,255,0) = 150
	// Pure blue (0,0,255) = 29
	ImageData source;
	source.width = 3;
	source.height = 1;
	source.channels = 3;
	source.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	source.pixels = {
		255, 0, 0, // Pure red
		0,
		255,
		0, // Pure green
		0,
		0,
		255 // Pure blue
	};

	// Act: Convert to grayscale
	const auto result = ImageConverter::toGrayscale( source );

	// Assert: Single-channel with luminance values
	REQUIRE( result.has_value() );
	REQUIRE( result->channels == 1 );
	REQUIRE( result->pixels.size() == 3 );
	REQUIRE( result->pixels[0] == 76 );	 // Red luminance
	REQUIRE( result->pixels[1] == 150 ); // Green luminance
	REQUIRE( result->pixels[2] == 29 );	 // Blue luminance
}

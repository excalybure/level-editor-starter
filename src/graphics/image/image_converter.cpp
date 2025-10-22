#include "image_converter.h"
#include "graphics/texture/texture_loader.h"
#include "core/console.h"

namespace graphics::image
{

std::optional<graphics::texture::ImageData> ImageConverter::extractChannel(
	const graphics::texture::ImageData &source,
	Channel channel )
{
	// Validate input
	const uint32_t channelIndex = static_cast<uint32_t>( channel );
	if ( channelIndex >= source.channels )
	{
		console::error( "ImageConverter::extractChannel: Channel index {} exceeds source channels {}",
			channelIndex,
			source.channels );
		return std::nullopt;
	}

	if ( source.pixels.empty() || source.width == 0 || source.height == 0 )
	{
		console::error( "ImageConverter::extractChannel: Invalid source image" );
		return std::nullopt;
	}

	// Create output image (single channel, R8_UNORM)
	graphics::texture::ImageData result;
	result.width = source.width;
	result.height = source.height;
	result.channels = 1;
	result.format = DXGI_FORMAT_R8_UNORM;

	const size_t pixelCount = source.width * source.height;
	result.pixels.resize( pixelCount );

	// Extract the target channel from each pixel
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t srcOffset = i * source.channels + channelIndex;
		result.pixels[i] = source.pixels[srcOffset];
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageConverter::packChannels(
	const graphics::texture::ImageData *r,
	const graphics::texture::ImageData *g,
	const graphics::texture::ImageData *b,
	const graphics::texture::ImageData *a )
{
	// R channel is required
	if ( !r || r->pixels.empty() || r->width == 0 || r->height == 0 )
	{
		console::error( "ImageConverter::packChannels: R channel is required and must be valid" );
		return std::nullopt;
	}

	// Ensure R is single-channel
	if ( r->channels != 1 )
	{
		console::error( "ImageConverter::packChannels: R channel must be single-channel (channels={})", r->channels );
		return std::nullopt;
	}

	// Determine output channel count
	uint32_t outputChannels = 1;
	if ( g )
		++outputChannels;
	if ( b )
		++outputChannels;
	if ( a )
		++outputChannels;

	// Validate all provided channels have matching dimensions and are single-channel
	const graphics::texture::ImageData *channels[] = { r, g, b, a };
	for ( uint32_t i = 1; i < 4; ++i )
	{
		if ( channels[i] )
		{
			if ( channels[i]->width != r->width || channels[i]->height != r->height )
			{
				console::error( "ImageConverter::packChannels: All channels must have same dimensions" );
				return std::nullopt;
			}
			if ( channels[i]->channels != 1 )
			{
				console::error( "ImageConverter::packChannels: All input channels must be single-channel" );
				return std::nullopt;
			}
		}
	}

	// Create output image
	graphics::texture::ImageData result;
	result.width = r->width;
	result.height = r->height;
	result.channels = outputChannels;

	// Set format based on channel count
	switch ( outputChannels )
	{
	case 1:
		result.format = DXGI_FORMAT_R8_UNORM;
		break;
	case 2:
		result.format = DXGI_FORMAT_R8G8_UNORM;
		break;
	case 3:
		result.format = DXGI_FORMAT_R8G8B8A8_UNORM; // No RGB8, use RGBA8
		break;
	case 4:
		result.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	default:
		console::error( "ImageConverter::packChannels: Invalid channel count {}", outputChannels );
		return std::nullopt;
	}

	const size_t pixelCount = r->width * r->height;
	result.pixels.resize( pixelCount * outputChannels );

	// Pack channels
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t outOffset = i * outputChannels;
		result.pixels[outOffset + 0] = r->pixels[i];
		if ( g )
			result.pixels[outOffset + 1] = g->pixels[i];
		if ( b )
			result.pixels[outOffset + 2] = b->pixels[i];
		if ( a )
			result.pixels[outOffset + 3] = a->pixels[i];
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageConverter::swizzle(
	const graphics::texture::ImageData &source,
	Channel r,
	Channel g,
	Channel b,
	Channel a )
{
	// Validate input
	if ( source.pixels.empty() || source.width == 0 || source.height == 0 )
	{
		console::error( "ImageConverter::swizzle: Invalid source image" );
		return std::nullopt;
	}

	if ( source.channels != 4 )
	{
		console::error( "ImageConverter::swizzle: Only RGBA images supported (got {} channels)", source.channels );
		return std::nullopt;
	}

	// Validate channel indices
	const uint32_t channelIndices[] = {
		static_cast<uint32_t>( r ),
		static_cast<uint32_t>( g ),
		static_cast<uint32_t>( b ),
		static_cast<uint32_t>( a )
	};

	for ( const uint32_t idx : channelIndices )
	{
		if ( idx >= source.channels )
		{
			console::error( "ImageConverter::swizzle: Channel index {} exceeds source channels {}", idx, source.channels );
			return std::nullopt;
		}
	}

	// Create output image (same dimensions and format)
	graphics::texture::ImageData result;
	result.width = source.width;
	result.height = source.height;
	result.channels = 4;
	result.format = source.format;

	const size_t pixelCount = source.width * source.height;
	result.pixels.resize( pixelCount * 4 );

	// Swizzle pixels
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t srcOffset = i * 4;
		const size_t dstOffset = i * 4;
		result.pixels[dstOffset + 0] = source.pixels[srcOffset + channelIndices[0]];
		result.pixels[dstOffset + 1] = source.pixels[srcOffset + channelIndices[1]];
		result.pixels[dstOffset + 2] = source.pixels[srcOffset + channelIndices[2]];
		result.pixels[dstOffset + 3] = source.pixels[srcOffset + channelIndices[3]];
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageConverter::addAlpha(
	const graphics::texture::ImageData &source,
	uint8_t alpha )
{
	// Validate input
	if ( source.pixels.empty() || source.width == 0 || source.height == 0 )
	{
		console::error( "ImageConverter::addAlpha: Invalid source image" );
		return std::nullopt;
	}

	if ( source.channels != 3 )
	{
		console::error( "ImageConverter::addAlpha: Source must be RGB (3 channels, got {})", source.channels );
		return std::nullopt;
	}

	// Create RGBA output
	graphics::texture::ImageData result;
	result.width = source.width;
	result.height = source.height;
	result.channels = 4;
	result.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	const size_t pixelCount = source.width * source.height;
	result.pixels.resize( pixelCount * 4 );

	// Copy RGB and add alpha
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t srcOffset = i * 3;
		const size_t dstOffset = i * 4;
		result.pixels[dstOffset + 0] = source.pixels[srcOffset + 0];
		result.pixels[dstOffset + 1] = source.pixels[srcOffset + 1];
		result.pixels[dstOffset + 2] = source.pixels[srcOffset + 2];
		result.pixels[dstOffset + 3] = alpha;
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageConverter::discardAlpha(
	const graphics::texture::ImageData &source )
{
	// Validate input
	if ( source.pixels.empty() || source.width == 0 || source.height == 0 )
	{
		console::error( "ImageConverter::discardAlpha: Invalid source image" );
		return std::nullopt;
	}

	if ( source.channels != 4 )
	{
		console::error( "ImageConverter::discardAlpha: Source must be RGBA (4 channels, got {})", source.channels );
		return std::nullopt;
	}

	// Create RGB output
	graphics::texture::ImageData result;
	result.width = source.width;
	result.height = source.height;
	result.channels = 3;
	result.format = DXGI_FORMAT_R8G8B8A8_UNORM; // Still RGBA format, but 3 channels

	const size_t pixelCount = source.width * source.height;
	result.pixels.resize( pixelCount * 3 );

	// Copy RGB, discard alpha
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t srcOffset = i * 4;
		const size_t dstOffset = i * 3;
		result.pixels[dstOffset + 0] = source.pixels[srcOffset + 0];
		result.pixels[dstOffset + 1] = source.pixels[srcOffset + 1];
		result.pixels[dstOffset + 2] = source.pixels[srcOffset + 2];
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageConverter::toGrayscale(
	const graphics::texture::ImageData &source )
{
	// Validate input
	if ( source.pixels.empty() || source.width == 0 || source.height == 0 )
	{
		console::error( "ImageConverter::toGrayscale: Invalid source image" );
		return std::nullopt;
	}

	if ( source.channels < 3 )
	{
		console::error( "ImageConverter::toGrayscale: Source must have at least 3 channels (got {})", source.channels );
		return std::nullopt;
	}

	// Create grayscale output
	graphics::texture::ImageData result;
	result.width = source.width;
	result.height = source.height;
	result.channels = 1;
	result.format = DXGI_FORMAT_R8_UNORM;

	const size_t pixelCount = source.width * source.height;
	result.pixels.resize( pixelCount );

	// Convert to grayscale using luminance formula
	// Luminance = 0.299*R + 0.587*G + 0.114*B
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t srcOffset = i * source.channels;
		const float r = static_cast<float>( source.pixels[srcOffset + 0] );
		const float g = static_cast<float>( source.pixels[srcOffset + 1] );
		const float b = static_cast<float>( source.pixels[srcOffset + 2] );
		const float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
		result.pixels[i] = static_cast<uint8_t>( luminance + 0.5f ); // Round to nearest
	}

	return result;
}

} // namespace graphics::image

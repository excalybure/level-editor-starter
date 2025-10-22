#include "graphics/image/image_processor.h"
#include "graphics/texture/texture_loader.h"
#include "core/console.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

namespace graphics::image
{

std::optional<graphics::texture::ImageData> ImageProcessor::resize(
	const graphics::texture::ImageData &source,
	uint32_t targetWidth,
	uint32_t targetHeight,
	ResizeFilter filter )
{
	// Validate inputs
	if ( source.width == 0 || source.height == 0 || targetWidth == 0 || targetHeight == 0 )
	{
		console::error( "ImageProcessor::resize: Invalid dimensions (source: {}x{}, target: {}x{})",
			source.width,
			source.height,
			targetWidth,
			targetHeight );
		return std::nullopt;
	}

	if ( source.channels == 0 || source.channels > 4 )
	{
		console::error( "ImageProcessor::resize: Invalid channel count ({})", source.channels );
		return std::nullopt;
	}

	if ( source.pixels.empty() )
	{
		console::error( "ImageProcessor::resize: Empty pixel data" );
		return std::nullopt;
	}

	// Verify pixel buffer size matches dimensions
	const size_t expectedSize = static_cast<size_t>( source.width ) * source.height * source.channels;
	if ( source.pixels.size() < expectedSize )
	{
		console::error( "ImageProcessor::resize: Pixel buffer too small ({} < {})",
			source.pixels.size(),
			expectedSize );
		return std::nullopt;
	}

	// Map ResizeFilter to stb filter
	stbir_filter stbFilter = STBIR_FILTER_DEFAULT;
	switch ( filter )
	{
	case ResizeFilter::NearestNeighbor:
		stbFilter = STBIR_FILTER_POINT_SAMPLE;
		break;
	case ResizeFilter::Bilinear:
		stbFilter = STBIR_FILTER_TRIANGLE; // Triangle produces bilinear-like results
		break;
	case ResizeFilter::Bicubic:
		stbFilter = STBIR_FILTER_CUBICBSPLINE;
		break;
	case ResizeFilter::Lanczos:
		stbFilter = STBIR_FILTER_MITCHELL; // Mitchell is closest to Lanczos
		break;
	}

	// Map channel count to stb pixel layout
	stbir_pixel_layout pixelLayout = STBIR_RGBA;
	switch ( source.channels )
	{
	case 1:
		pixelLayout = STBIR_1CHANNEL;
		break;
	case 2:
		pixelLayout = STBIR_2CHANNEL;
		break;
	case 3:
		pixelLayout = STBIR_RGB;
		break;
	case 4:
		pixelLayout = STBIR_RGBA;
		break;
	}

	// Allocate output buffer
	graphics::texture::ImageData result;
	result.width = targetWidth;
	result.height = targetHeight;
	result.channels = source.channels;
	result.format = source.format;
	result.pixels.resize( static_cast<size_t>( targetWidth ) * targetHeight * source.channels );

	// Perform resize
	const int srcStride = static_cast<int>( source.width * source.channels );
	const int dstStride = static_cast<int>( targetWidth * source.channels );

	void *resizeResult = stbir_resize(
		source.pixels.data(),
		static_cast<int>( source.width ),
		static_cast<int>( source.height ),
		srcStride,
		result.pixels.data(),
		static_cast<int>( targetWidth ),
		static_cast<int>( targetHeight ),
		dstStride,
		pixelLayout,
		STBIR_TYPE_UINT8,
		STBIR_EDGE_CLAMP,
		stbFilter );

	if ( resizeResult == nullptr )
	{
		console::error( "ImageProcessor::resize: stb_image_resize2 failed" );
		return std::nullopt;
	}

	return result;
}

std::optional<graphics::texture::ImageData> ImageProcessor::resizeAspect(
	const graphics::texture::ImageData &source,
	uint32_t maxWidth,
	uint32_t maxHeight,
	ResizeFilter filter )
{
	// Validate inputs
	if ( source.width == 0 || source.height == 0 )
	{
		console::error( "ImageProcessor::resizeAspect: Invalid source dimensions ({}x{})",
			source.width,
			source.height );
		return std::nullopt;
	}

	if ( maxWidth == 0 || maxHeight == 0 )
	{
		console::error( "ImageProcessor::resizeAspect: Invalid max dimensions ({}x{})",
			maxWidth,
			maxHeight );
		return std::nullopt;
	}

	// If image already fits, return as-is (no upscaling)
	if ( source.width <= maxWidth && source.height <= maxHeight )
	{
		return source;
	}

	// Calculate aspect ratio
	const float sourceAspect = static_cast<float>( source.width ) / static_cast<float>( source.height );
	const float boxAspect = static_cast<float>( maxWidth ) / static_cast<float>( maxHeight );

	uint32_t targetWidth = 0;
	uint32_t targetHeight = 0;

	// Determine which dimension constrains the fit
	if ( sourceAspect > boxAspect )
	{
		// Width is the limiting dimension
		targetWidth = maxWidth;
		targetHeight = static_cast<uint32_t>( maxWidth / sourceAspect );
	}
	else
	{
		// Height is the limiting dimension
		targetHeight = maxHeight;
		targetWidth = static_cast<uint32_t>( maxHeight * sourceAspect );
	}

	// Ensure dimensions are at least 1
	if ( targetWidth == 0 )
		targetWidth = 1;
	if ( targetHeight == 0 )
		targetHeight = 1;

	return resize( source, targetWidth, targetHeight, filter );
}

std::optional<graphics::texture::ImageData> ImageProcessor::createThumbnail(
	const graphics::texture::ImageData &source,
	uint32_t size,
	ResizeFilter filter )
{
	// Validate inputs
	if ( source.width == 0 || source.height == 0 )
	{
		console::error( "ImageProcessor::createThumbnail: Invalid source dimensions ({}x{})",
			source.width,
			source.height );
		return std::nullopt;
	}

	if ( size == 0 )
	{
		console::error( "ImageProcessor::createThumbnail: Invalid thumbnail size (0)" );
		return std::nullopt;
	}

	if ( source.channels == 0 || source.channels > 4 )
	{
		console::error( "ImageProcessor::createThumbnail: Invalid channel count ({})", source.channels );
		return std::nullopt;
	}

	if ( source.pixels.empty() )
	{
		console::error( "ImageProcessor::createThumbnail: Empty pixel data" );
		return std::nullopt;
	}

	// Determine crop region (center square)
	const uint32_t cropSize = ( source.width < source.height ) ? source.width : source.height;
	const uint32_t cropX = ( source.width - cropSize ) / 2;
	const uint32_t cropY = ( source.height - cropSize ) / 2;

	// Extract center square
	graphics::texture::ImageData cropped;
	cropped.width = cropSize;
	cropped.height = cropSize;
	cropped.channels = source.channels;
	cropped.format = source.format;
	cropped.pixels.resize( static_cast<size_t>( cropSize ) * cropSize * source.channels );

	// Copy cropped region row by row
	for ( uint32_t y = 0; y < cropSize; ++y )
	{
		const size_t srcOffset = ( ( cropY + y ) * source.width + cropX ) * source.channels;
		const size_t dstOffset = y * cropSize * source.channels;
		const size_t rowBytes = cropSize * source.channels;

		std::memcpy( cropped.pixels.data() + dstOffset, source.pixels.data() + srcOffset, rowBytes );
	}

	// Resize the cropped square to target thumbnail size
	return resize( cropped, size, size, filter );
}

std::vector<graphics::texture::ImageData> ImageProcessor::generateMipmaps(
	const graphics::texture::ImageData &source,
	uint32_t maxLevels,
	MipmapFilter filter )
{
	// TODO: Future implementation
	console::error( "ImageProcessor::generateMipmaps not implemented yet" );
	return {};
}

uint32_t ImageProcessor::calculateMipLevels( uint32_t width, uint32_t height )
{
	// TODO: Future implementation
	console::error( "ImageProcessor::calculateMipLevels not implemented yet" );
	return 0;
}

} // namespace graphics::image

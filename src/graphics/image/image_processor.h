#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace graphics::texture
{
struct ImageData;
}

namespace graphics::image
{

enum class ResizeFilter
{
	NearestNeighbor, // Fast, blocky
	Bilinear,		 // Good quality/speed balance
	Bicubic,		 // Higher quality, slower
	Lanczos			 // Best quality, slowest
};

enum class MipmapFilter
{
	Box,	  // Simple averaging
	Triangle, // Linear
	Kaiser	  // High quality
};

class ImageProcessor
{
public:
	// ===== Resizing =====

	/// Resize image to exact dimensions
	/// @param source Source image data
	/// @param targetWidth Target width in pixels
	/// @param targetHeight Target height in pixels
	/// @param filter Resize filter algorithm
	/// @return Resized image, or nullopt on failure
	static std::optional<graphics::texture::ImageData> resize(
		const graphics::texture::ImageData &source,
		uint32_t targetWidth,
		uint32_t targetHeight,
		ResizeFilter filter = ResizeFilter::Bilinear );

	/// Resize image maintaining aspect ratio (fit within box)
	/// @param source Source image data
	/// @param maxWidth Maximum width
	/// @param maxHeight Maximum height
	/// @param filter Resize filter algorithm
	/// @return Resized image, or nullopt on failure
	static std::optional<graphics::texture::ImageData> resizeAspect(
		const graphics::texture::ImageData &source,
		uint32_t maxWidth,
		uint32_t maxHeight,
		ResizeFilter filter = ResizeFilter::Bilinear );

	/// Create thumbnail (square crop + resize)
	/// @param source Source image data
	/// @param size Target size (width and height)
	/// @param filter Resize filter algorithm
	/// @return Square thumbnail, or nullopt on failure
	static std::optional<graphics::texture::ImageData> createThumbnail(
		const graphics::texture::ImageData &source,
		uint32_t size,
		ResizeFilter filter = ResizeFilter::Bilinear );

	// ===== Mipmaps (Future) =====

	/// Generate full mipmap chain
	/// @param source Source image (mip level 0)
	/// @param maxLevels Maximum mip levels (0 = full chain to 1x1)
	/// @param filter Downsampling filter
	/// @return Vector of mip levels [0..N], or empty on failure
	static std::vector<graphics::texture::ImageData> generateMipmaps(
		const graphics::texture::ImageData &source,
		uint32_t maxLevels = 0,
		MipmapFilter filter = MipmapFilter::Kaiser );

	/// Calculate mip level count for given dimensions
	/// @param width Image width
	/// @param height Image height
	/// @return Number of mip levels in full chain
	static uint32_t calculateMipLevels( uint32_t width, uint32_t height );
};

} // namespace graphics::image

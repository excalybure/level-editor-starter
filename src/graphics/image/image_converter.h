#pragma once

#include <cstdint>
#include <optional>

namespace graphics::texture
{
struct ImageData;
}

namespace graphics::image
{

enum class Channel
{
	R = 0,
	G = 1,
	B = 2,
	A = 3
};

class ImageConverter
{
public:
	// ===== Channel Operations =====

	/// Extract single channel as grayscale image
	/// @param source Source image (must have target channel)
	/// @param channel Channel to extract
	/// @return Single-channel image (R8_UNORM), or nullopt on failure
	static std::optional<graphics::texture::ImageData> extractChannel(
		const graphics::texture::ImageData &source,
		Channel channel );

	/// Pack up to 4 single-channel images into multi-channel image
	/// @param r Red channel (required)
	/// @param g Green channel (optional)
	/// @param b Blue channel (optional)
	/// @param a Alpha channel (optional)
	/// @return Packed image (R8/RG8/RGB8/RGBA8), or nullopt on failure
	static std::optional<graphics::texture::ImageData> packChannels(
		const graphics::texture::ImageData *r,
		const graphics::texture::ImageData *g = nullptr,
		const graphics::texture::ImageData *b = nullptr,
		const graphics::texture::ImageData *a = nullptr );

	/// Swizzle channels (e.g., RGBA -> BGRA)
	/// @param source Source image
	/// @param r Source channel for output red
	/// @param g Source channel for output green
	/// @param b Source channel for output blue
	/// @param a Source channel for output alpha
	/// @return Swizzled image, or nullopt on failure
	static std::optional<graphics::texture::ImageData> swizzle(
		const graphics::texture::ImageData &source,
		Channel r,
		Channel g,
		Channel b,
		Channel a );

	// ===== Format Conversion =====

	/// Convert RGB to RGBA (add alpha channel)
	/// @param source RGB image
	/// @param alpha Alpha value to add (0-255)
	/// @return RGBA image, or nullopt on failure
	static std::optional<graphics::texture::ImageData> addAlpha(
		const graphics::texture::ImageData &source,
		uint8_t alpha = 255 );

	/// Convert RGBA to RGB (discard alpha)
	/// @param source RGBA image
	/// @return RGB image, or nullopt on failure
	static std::optional<graphics::texture::ImageData> discardAlpha(
		const graphics::texture::ImageData &source );

	/// Convert to grayscale (luminance)
	/// @param source Source image
	/// @return Grayscale image (R8_UNORM), or nullopt on failure
	static std::optional<graphics::texture::ImageData> toGrayscale(
		const graphics::texture::ImageData &source );
};

} // namespace graphics::image

export module engine.color;

import std;
import engine.math; // For lerp, clamp, pow, log, abs, mod functions

export namespace math
{

// ================================
// Color Math Functions
// ================================

// Convert HSV to RGB color space
// Hue: [0, 360], Saturation: [0, 1], Value: [0, 1]
// Returns RGB: [0, 1] for each component
template <typename T>
constexpr auto hsvToRgb( T hue, T saturation, T value )
{
	// Normalize hue to [0, 360) range
	hue = mod( hue, static_cast<T>( 360 ) );
	if ( hue < static_cast<T>( 0 ) )
		hue += static_cast<T>( 360 );

	T c = value * saturation; // Chroma
	T x = c * ( static_cast<T>( 1 ) - abs( mod( hue / static_cast<T>( 60 ), static_cast<T>( 2 ) ) - static_cast<T>( 1 ) ) );
	T m = value - c;

	T r, g, b;

	if ( hue < static_cast<T>( 60 ) )
	{
		r = c;
		g = x;
		b = static_cast<T>( 0 );
	}
	else if ( hue < static_cast<T>( 120 ) )
	{
		r = x;
		g = c;
		b = static_cast<T>( 0 );
	}
	else if ( hue < static_cast<T>( 180 ) )
	{
		r = static_cast<T>( 0 );
		g = c;
		b = x;
	}
	else if ( hue < static_cast<T>( 240 ) )
	{
		r = static_cast<T>( 0 );
		g = x;
		b = c;
	}
	else if ( hue < static_cast<T>( 300 ) )
	{
		r = x;
		g = static_cast<T>( 0 );
		b = c;
	}
	else
	{
		r = c;
		g = static_cast<T>( 0 );
		b = x;
	}

	// Return as a simple struct (we'll use Vec3 when calling this)
	struct RgbColor
	{
		T r, g, b;
	};
	return RgbColor{ r + m, g + m, b + m };
}

// Convert RGB to HSV color space
// RGB: [0, 1] for each component
// Returns HSV: Hue [0, 360], Saturation [0, 1], Value [0, 1]
template <typename T>
constexpr auto rgbToHsv( T red, T green, T blue )
{
	T maxVal = std::max( { red, green, blue } );
	T minVal = std::min( { red, green, blue } );
	T delta = maxVal - minVal;

	T hue = static_cast<T>( 0 );
	T saturation = ( maxVal == static_cast<T>( 0 ) ) ? static_cast<T>( 0 ) : delta / maxVal;
	T value = maxVal;

	if ( delta != static_cast<T>( 0 ) )
	{
		if ( maxVal == red )
		{
			hue = static_cast<T>( 60 ) * mod( ( green - blue ) / delta, static_cast<T>( 6 ) );
		}
		else if ( maxVal == green )
		{
			hue = static_cast<T>( 60 ) * ( ( blue - red ) / delta + static_cast<T>( 2 ) );
		}
		else // maxVal == blue
		{
			hue = static_cast<T>( 60 ) * ( ( red - green ) / delta + static_cast<T>( 4 ) );
		}

		if ( hue < static_cast<T>( 0 ) )
			hue += static_cast<T>( 360 );
	}

	struct HsvColor
	{
		T h, s, v;
	};
	return HsvColor{ hue, saturation, value };
}

// Linear interpolation between two colors in RGB space
template <typename T>
constexpr auto lerpRgb( T r1, T g1, T b1, T r2, T g2, T b2, T t )
{
	struct RgbColor
	{
		T r, g, b;
	};
	return RgbColor{
		lerp( r1, r2, t ),
		lerp( g1, g2, t ),
		lerp( b1, b2, t )
	};
}

// Linear interpolation between two colors in HSV space
// This provides more intuitive color blending for UI applications
template <typename T>
constexpr auto lerpHsv( T h1, T s1, T v1, T h2, T s2, T v2, T t )
{
	// Handle hue wrapping - take the shorter path around the color wheel
	T hueDiff = h2 - h1;
	if ( hueDiff > static_cast<T>( 180 ) )
	{
		hueDiff -= static_cast<T>( 360 );
	}
	else if ( hueDiff < static_cast<T>( -180 ) )
	{
		hueDiff += static_cast<T>( 360 );
	}

	T hueResult = h1 + hueDiff * t;
	if ( hueResult < static_cast<T>( 0 ) )
		hueResult += static_cast<T>( 360 );
	else if ( hueResult >= static_cast<T>( 360 ) )
		hueResult -= static_cast<T>( 360 );

	struct HsvColor
	{
		T h, s, v;
	};
	return HsvColor{
		hueResult,
		lerp( s1, s2, t ),
		lerp( v1, v2, t )
	};
}

// Convert linear color value to gamma-corrected (sRGB)
template <typename T>
constexpr T linearToGamma( T linear )
{
	if ( linear <= static_cast<T>( 0.0031308 ) )
	{
		return static_cast<T>( 12.92 ) * linear;
	}
	else
	{
		return static_cast<T>( 1.055 ) * pow( linear, static_cast<T>( 1.0 / 2.4 ) ) - static_cast<T>( 0.055 );
	}
}

// Convert gamma-corrected (sRGB) color value to linear
template <typename T>
constexpr T gammaToLinear( T gamma )
{
	if ( gamma <= static_cast<T>( 0.04045 ) )
	{
		return gamma / static_cast<T>( 12.92 );
	}
	else
	{
		return pow( ( gamma + static_cast<T>( 0.055 ) ) / static_cast<T>( 1.055 ), static_cast<T>( 2.4 ) );
	}
}

// Calculate luminance of RGB color using standard weights
template <typename T>
constexpr T luminance( T red, T green, T blue )
{
	// Using ITU-R BT.709 luma coefficients
	return static_cast<T>( 0.2126 ) * red + static_cast<T>( 0.7152 ) * green + static_cast<T>( 0.0722 ) * blue;
}

// Adjust color saturation
template <typename T>
constexpr auto adjustSaturation( T red, T green, T blue, T saturationFactor )
{
	T luma = luminance( red, green, blue );

	struct RgbColor
	{
		T r, g, b;
	};
	return RgbColor{
		lerp( luma, red, saturationFactor ),
		lerp( luma, green, saturationFactor ),
		lerp( luma, blue, saturationFactor )
	};
}

// Convert RGB color temperature (Kelvin) to RGB
// Useful range: 1000K - 40000K, typical: 2700K (warm) to 6500K (cool daylight)
template <typename T>
constexpr auto temperatureToRgb( T kelvin )
{
	// Clamp to reasonable range
	kelvin = clamp( kelvin, static_cast<T>( 1000 ), static_cast<T>( 40000 ) );
	kelvin /= static_cast<T>( 100 );

	T red, green, blue;

	// Calculate red
	if ( kelvin <= static_cast<T>( 66 ) )
	{
		red = static_cast<T>( 255 );
	}
	else
	{
		red = kelvin - static_cast<T>( 60 );
		red = static_cast<T>( 329.698727446 ) * pow( red, static_cast<T>( -0.1332047592 ) );
		red = clamp( red, static_cast<T>( 0 ), static_cast<T>( 255 ) );
	}

	// Calculate green
	if ( kelvin <= static_cast<T>( 66 ) )
	{
		green = kelvin;
		green = static_cast<T>( 99.4708025861 ) * log( green ) - static_cast<T>( 161.1195681661 );
		green = clamp( green, static_cast<T>( 0 ), static_cast<T>( 255 ) );
	}
	else
	{
		green = kelvin - static_cast<T>( 60 );
		green = static_cast<T>( 288.1221695283 ) * pow( green, static_cast<T>( -0.0755148492 ) );
		green = clamp( green, static_cast<T>( 0 ), static_cast<T>( 255 ) );
	}

	// Calculate blue
	if ( kelvin >= static_cast<T>( 66 ) )
	{
		blue = static_cast<T>( 255 );
	}
	else if ( kelvin <= static_cast<T>( 19 ) )
	{
		blue = static_cast<T>( 0 );
	}
	else
	{
		blue = kelvin - static_cast<T>( 10 );
		blue = static_cast<T>( 138.5177312231 ) * log( blue ) - static_cast<T>( 305.0447927307 );
		blue = clamp( blue, static_cast<T>( 0 ), static_cast<T>( 255 ) );
	}

	// Convert from [0,255] to [0,1]
	struct RgbColor
	{
		T r, g, b;
	};
	return RgbColor{
		red / static_cast<T>( 255 ),
		green / static_cast<T>( 255 ),
		blue / static_cast<T>( 255 )
	};
}

} // namespace math

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
#include "engine/math/color.h"

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

// Test color math functions
TEST_CASE( "Color space conversions", "[math][color]" )
{
	SECTION( "HSV to RGB conversion" )
	{
		// Pure red (Hue=0, Sat=1, Val=1)
		auto red = math::hsvToRgb( 0.0f, 1.0f, 1.0f );
		REQUIRE_THAT( red.r, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( red.g, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( red.b, WithinAbs( 0.0f, 1e-6f ) );

		// Pure green (Hue=120, Sat=1, Val=1)
		auto green = math::hsvToRgb( 120.0f, 1.0f, 1.0f );
		REQUIRE_THAT( green.r, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( green.g, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( green.b, WithinAbs( 0.0f, 1e-6f ) );

		// Pure blue (Hue=240, Sat=1, Val=1)
		auto blue = math::hsvToRgb( 240.0f, 1.0f, 1.0f );
		REQUIRE_THAT( blue.r, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( blue.g, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( blue.b, WithinRel( 1.0f, 1e-5f ) );

		// White (any hue, Sat=0, Val=1)
		auto white = math::hsvToRgb( 0.0f, 0.0f, 1.0f );
		REQUIRE_THAT( white.r, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( white.g, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( white.b, WithinRel( 1.0f, 1e-5f ) );

		// Black (any hue, any sat, Val=0)
		auto black = math::hsvToRgb( 180.0f, 1.0f, 0.0f );
		REQUIRE_THAT( black.r, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( black.g, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( black.b, WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "RGB to HSV conversion" )
	{
		// Pure red -> HSV(0, 1, 1)
		auto redHsv = math::rgbToHsv( 1.0f, 0.0f, 0.0f );
		REQUIRE_THAT( redHsv.h, WithinAbs( 0.0f, 1e-5f ) );
		REQUIRE_THAT( redHsv.s, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( redHsv.v, WithinRel( 1.0f, 1e-5f ) );

		// Pure green -> HSV(120, 1, 1)
		auto greenHsv = math::rgbToHsv( 0.0f, 1.0f, 0.0f );
		REQUIRE_THAT( greenHsv.h, WithinRel( 120.0f, 1e-4f ) );
		REQUIRE_THAT( greenHsv.s, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( greenHsv.v, WithinRel( 1.0f, 1e-5f ) );

		// Pure blue -> HSV(240, 1, 1)
		auto blueHsv = math::rgbToHsv( 0.0f, 0.0f, 1.0f );
		REQUIRE_THAT( blueHsv.h, WithinRel( 240.0f, 1e-4f ) );
		REQUIRE_THAT( blueHsv.s, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( blueHsv.v, WithinRel( 1.0f, 1e-5f ) );

		// White -> HSV(0, 0, 1)
		auto whiteHsv = math::rgbToHsv( 1.0f, 1.0f, 1.0f );
		REQUIRE_THAT( whiteHsv.h, WithinAbs( 0.0f, 1e-5f ) );
		REQUIRE_THAT( whiteHsv.s, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( whiteHsv.v, WithinRel( 1.0f, 1e-5f ) );

		// Black -> HSV(0, 0, 0)
		auto blackHsv = math::rgbToHsv( 0.0f, 0.0f, 0.0f );
		REQUIRE_THAT( blackHsv.h, WithinAbs( 0.0f, 1e-5f ) );
		REQUIRE_THAT( blackHsv.s, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( blackHsv.v, WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "round trip HSV<->RGB conversion" )
	{
		// Test several colors for round-trip accuracy
		float testColors[][3] = {
			{ 30.0f, 0.8f, 0.9f },	// Orange-ish
			{ 160.0f, 0.6f, 0.7f }, // Green-ish
			{ 270.0f, 0.9f, 0.5f }, // Purple-ish
			{ 45.0f, 0.3f, 0.8f }	// Muted yellow-ish
		};

		for ( auto &[h, s, v] : testColors )
		{
			auto rgb = math::hsvToRgb( h, s, v );
			auto hsvBack = math::rgbToHsv( rgb.r, rgb.g, rgb.b );

			REQUIRE_THAT( hsvBack.h, WithinRel( h, 1e-3f ) );
			REQUIRE_THAT( hsvBack.s, WithinRel( s, 1e-4f ) );
			REQUIRE_THAT( hsvBack.v, WithinRel( v, 1e-5f ) );
		}
	}
}

TEST_CASE( "Color interpolation", "[math][color]" )
{
	SECTION( "RGB interpolation" )
	{
		// Lerp from red to blue
		auto midColor = math::lerpRgb( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f );
		REQUIRE_THAT( midColor.r, WithinRel( 0.5f, 1e-5f ) );
		REQUIRE_THAT( midColor.g, WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( midColor.b, WithinRel( 0.5f, 1e-5f ) );

		// Lerp from black to white
		auto gray = math::lerpRgb( 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.3f );
		REQUIRE_THAT( gray.r, WithinRel( 0.3f, 1e-5f ) );
		REQUIRE_THAT( gray.g, WithinRel( 0.3f, 1e-5f ) );
		REQUIRE_THAT( gray.b, WithinRel( 0.3f, 1e-5f ) );
	}

	SECTION( "HSV interpolation" )
	{
		// Lerp from red (0°) to green (120°)
		auto midHsv = math::lerpHsv( 0.0f, 1.0f, 1.0f, 120.0f, 1.0f, 1.0f, 0.5f );
		REQUIRE_THAT( midHsv.h, WithinRel( 60.0f, 1e-4f ) ); // Should be yellow
		REQUIRE_THAT( midHsv.s, WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( midHsv.v, WithinRel( 1.0f, 1e-5f ) );

		// Test hue wrapping (red 0° to red 350° should go through purple, not green)
		auto shortPath = math::lerpHsv( 0.0f, 1.0f, 1.0f, 350.0f, 1.0f, 1.0f, 0.5f );
		REQUIRE_THAT( shortPath.h, WithinRel( 355.0f, 1e-3f ) ); // Should wrap around
	}
}

TEST_CASE( "Gamma correction", "[math][color]" )
{
	SECTION( "linear to gamma conversion" )
	{
		REQUIRE_THAT( math::linearToGamma( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::linearToGamma( 1.0f ), WithinRel( 1.0f, 1e-5f ) );

		// Test the breakpoint region
		float lowValue = 0.001f;
		REQUIRE_THAT( math::linearToGamma( lowValue ), WithinRel( 12.92f * lowValue, 1e-4f ) );
	}

	SECTION( "gamma to linear conversion" )
	{
		REQUIRE_THAT( math::gammaToLinear( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::gammaToLinear( 1.0f ), WithinRel( 1.0f, 1e-5f ) );
	}

	SECTION( "round trip gamma correction" )
	{
		float testValues[] = { 0.0f, 0.1f, 0.3f, 0.5f, 0.7f, 0.9f, 1.0f };

		for ( float value : testValues )
		{
			float gammaConverted = math::linearToGamma( value );
			float backToLinear = math::gammaToLinear( gammaConverted );
			REQUIRE_THAT( backToLinear, WithinRel( value, 1e-4f ) );
		}
	}
}

TEST_CASE( "Color utilities", "[math][color]" )
{
	SECTION( "luminance calculation" )
	{
		// Pure white should have luminance 1.0
		REQUIRE_THAT( math::luminance( 1.0f, 1.0f, 1.0f ), WithinRel( 1.0f, 1e-5f ) );

		// Pure black should have luminance 0.0
		REQUIRE_THAT( math::luminance( 0.0f, 0.0f, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );

		// Pure green should be brighter than pure red (due to human perception)
		float redLuma = math::luminance( 1.0f, 0.0f, 0.0f );
		float greenLuma = math::luminance( 0.0f, 1.0f, 0.0f );
		REQUIRE( greenLuma > redLuma );

		// Blue should be darkest
		float blueLuma = math::luminance( 0.0f, 0.0f, 1.0f );
		REQUIRE( blueLuma < redLuma );
		REQUIRE( blueLuma < greenLuma );
	}

	SECTION( "saturation adjustment" )
	{
		// Desaturate red should move toward gray
		auto desaturated = math::adjustSaturation( 1.0f, 0.0f, 0.0f, 0.5f );
		REQUIRE( desaturated.r < 1.0f );
		REQUIRE( desaturated.g > 0.0f );
		REQUIRE( desaturated.b > 0.0f );

		// Over-saturate should increase color intensity
		auto oversaturated = math::adjustSaturation( 0.8f, 0.2f, 0.2f, 1.5f );
		REQUIRE( oversaturated.r > 0.8f );
		REQUIRE( oversaturated.g < 0.2f );
		REQUIRE( oversaturated.b < 0.2f );

		// Complete desaturation should create gray
		auto gray = math::adjustSaturation( 1.0f, 0.0f, 0.0f, 0.0f );
		REQUIRE_THAT( gray.r, WithinAbs( gray.g, 1e-5f ) );
		REQUIRE_THAT( gray.g, WithinAbs( gray.b, 1e-5f ) );
	}
}

TEST_CASE( "Color temperature", "[math][color]" )
{
	SECTION( "temperature to RGB conversion" )
	{
		// Very warm temperature (candlelight ~1900K) should be reddish
		auto warm = math::temperatureToRgb( 1900.0f );
		REQUIRE( warm.r > warm.g );
		REQUIRE( warm.g > warm.b );

		// Neutral daylight (~5500K) should be fairly balanced
		auto neutral = math::temperatureToRgb( 5500.0f );
		REQUIRE_THAT( neutral.r, WithinRel( 1.0f, 0.2f ) );
		REQUIRE_THAT( neutral.g, WithinRel( 1.0f, 0.2f ) );
		REQUIRE_THAT( neutral.b, WithinRel( 1.0f, 0.3f ) );

		// Cool temperature (~8000K) should be bluish
		auto cool = math::temperatureToRgb( 8000.0f );
		REQUIRE( cool.b > cool.g );
		REQUIRE( cool.b > cool.r );
	}

	SECTION( "temperature clamping" )
	{
		// Should handle extreme values gracefully
		auto tooLow = math::temperatureToRgb( 500.0f );	   // Should clamp to 1000K
		auto tooHigh = math::temperatureToRgb( 50000.0f ); // Should clamp to 40000K

		// Results should be within valid RGB range [0,1]
		REQUIRE( tooLow.r >= 0.0f );
		REQUIRE( tooLow.r <= 1.0f );
		REQUIRE( tooLow.g >= 0.0f );
		REQUIRE( tooLow.g <= 1.0f );
		REQUIRE( tooLow.b >= 0.0f );
		REQUIRE( tooLow.b <= 1.0f );

		REQUIRE( tooHigh.r >= 0.0f );
		REQUIRE( tooHigh.r <= 1.0f );
		REQUIRE( tooHigh.g >= 0.0f );
		REQUIRE( tooHigh.g <= 1.0f );
		REQUIRE( tooHigh.b >= 0.0f );
		REQUIRE( tooHigh.b <= 1.0f );
	}
}

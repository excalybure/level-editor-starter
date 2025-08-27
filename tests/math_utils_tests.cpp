#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
import engine.math;

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

// Test mathematical constants
TEST_CASE( "Math constants", "[math][constants]" )
{
	// Test pi constant
	REQUIRE_THAT( math::pi<float>, WithinRel( 3.14159265f, 1e-6f ) );
	REQUIRE_THAT( math::pi<double>, WithinRel( 3.141592653589793, 1e-12 ) );

	// Test e constant
	REQUIRE_THAT( math::e<float>, WithinRel( 2.71828183f, 1e-6f ) );
	REQUIRE_THAT( math::e<double>, WithinRel( 2.718281828459045, 1e-12 ) );

	// Test sqrt2 constant
	REQUIRE_THAT( math::sqrt2<float>, WithinRel( 1.41421356f, 1e-6f ) );
	REQUIRE_THAT( math::sqrt2<double>, WithinRel( 1.414213562373095, 1e-12 ) );

	// Test sqrt3 constant
	REQUIRE_THAT( math::sqrt3<float>, WithinRel( 1.73205081f, 1e-6f ) );
	REQUIRE_THAT( math::sqrt3<double>, WithinRel( 1.732050807568877, 1e-12 ) );
}

// Test angle conversion functions
TEST_CASE( "Angle conversions", "[math][angles]" )
{
	SECTION( "radians to degrees" )
	{
		REQUIRE_THAT( math::degrees( math::pi<float> ), WithinRel( 180.0f, 1e-5f ) );
		REQUIRE_THAT( math::degrees( math::pi<double> / 2.0 ), WithinRel( 90.0, 1e-10 ) );
		REQUIRE_THAT( math::degrees( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "degrees to radians" )
	{
		REQUIRE_THAT( math::radians( 180.0f ), WithinRel( math::pi<float>, 1e-5f ) );
		REQUIRE_THAT( math::radians( 90.0 ), WithinRel( math::pi<double> / 2.0, 1e-10 ) );
		REQUIRE_THAT( math::radians( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "round trip conversions" )
	{
		float angle_deg = 45.0f;
		REQUIRE_THAT( math::degrees( math::radians( angle_deg ) ), WithinRel( angle_deg, 1e-5f ) );

		double angle_rad = math::pi<double> / 3.0;
		REQUIRE_THAT( math::radians( math::degrees( angle_rad ) ), WithinRel( angle_rad, 1e-10 ) );
	}
}

// Test interpolation functions
TEST_CASE( "Linear interpolation", "[math][lerp]" )
{
	SECTION( "basic lerp" )
	{
		REQUIRE_THAT( math::lerp( 0.0f, 10.0f, 0.5f ), WithinAbs( 5.0f, 1e-6f ) );
		REQUIRE_THAT( math::lerp( -5.0, 5.0, 0.0 ), WithinAbs( -5.0, 1e-10 ) );
		REQUIRE_THAT( math::lerp( -5.0, 5.0, 1.0 ), WithinAbs( 5.0, 1e-10 ) );
		REQUIRE_THAT( math::lerp( 100.0f, 200.0f, 0.25f ), WithinRel( 125.0f, 1e-5f ) );
	}

	SECTION( "edge cases" )
	{
		REQUIRE_THAT( math::lerp( 5.0f, 5.0f, 0.5f ), WithinAbs( 5.0f, 1e-6f ) );
		REQUIRE_THAT( math::lerp( 0.0f, 1.0f, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::lerp( 0.0f, 1.0f, 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}
}

// Test clamping functions
TEST_CASE( "Clamp function", "[math][clamp]" )
{
	SECTION( "basic clamping" )
	{
		REQUIRE( math::clamp( 5.0f, 0.0f, 10.0f ) == 5.0f );
		REQUIRE( math::clamp( -1.0f, 0.0f, 10.0f ) == 0.0f );
		REQUIRE( math::clamp( 15.0f, 0.0f, 10.0f ) == 10.0f );
	}

	SECTION( "integer clamping" )
	{
		REQUIRE( math::clamp( 7, 1, 10 ) == 7 );
		REQUIRE( math::clamp( 0, 1, 10 ) == 1 );
		REQUIRE( math::clamp( 11, 1, 10 ) == 10 );
	}

	SECTION( "edge cases" )
	{
		REQUIRE( math::clamp( 5.0f, 5.0f, 5.0f ) == 5.0f );
		REQUIRE( math::clamp( 3.0f, 5.0f, 10.0f ) == 5.0f );
		REQUIRE( math::clamp( 12.0f, 5.0f, 10.0f ) == 10.0f );
	}
}

// Test sign function
TEST_CASE( "Sign function", "[math][sign]" )
{
	REQUIRE( math::sign( 5.0f ) == 1.0f );
	REQUIRE( math::sign( -3.0f ) == -1.0f );
	REQUIRE( math::sign( 0.0f ) == 0.0f );
	REQUIRE( math::sign( -0.0f ) == 0.0f );

	REQUIRE( math::sign( 100 ) == 1 );
	REQUIRE( math::sign( -50 ) == -1 );
	REQUIRE( math::sign( 0 ) == 0 );
}

// Test absolute value function
TEST_CASE( "Absolute value", "[math][abs]" )
{
	REQUIRE( math::abs( 5.0f ) == 5.0f );
	REQUIRE( math::abs( -5.0f ) == 5.0f );
	REQUIRE( math::abs( 0.0f ) == 0.0f );

	REQUIRE( math::abs( 42 ) == 42 );
	REQUIRE( math::abs( -42 ) == 42 );
	REQUIRE( math::abs( 0 ) == 0 );
}

// Test square function
TEST_CASE( "Square function", "[math][square]" )
{
	REQUIRE( math::square( 5.0f ) == 25.0f );
	REQUIRE( math::square( -3.0f ) == 9.0f );
	REQUIRE( math::square( 0.0f ) == 0.0f );
	REQUIRE_THAT( math::square( 2.5 ), WithinRel( 6.25, 1e-10 ) );
}

// Test power and root functions
TEST_CASE( "Power and root functions", "[math][power]" )
{
	SECTION( "power function" )
	{
		REQUIRE_THAT( math::pow( 2.0f, 3.0f ), WithinRel( 8.0f, 1e-5f ) );
		REQUIRE_THAT( math::pow( 5.0, 0.0 ), WithinRel( 1.0, 1e-10 ) );
		REQUIRE_THAT( math::pow( 9.0f, 0.5f ), WithinRel( 3.0f, 1e-5f ) );
	}

	SECTION( "square root" )
	{
		REQUIRE_THAT( math::sqrt( 4.0f ), WithinRel( 2.0f, 1e-5f ) );
		REQUIRE_THAT( math::sqrt( 9.0 ), WithinRel( 3.0, 1e-10 ) );
		REQUIRE_THAT( math::sqrt( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::sqrt( 2.0 ), WithinRel( math::sqrt2<double>, 1e-10 ) );
	}
}

// Test trigonometric functions
TEST_CASE( "Trigonometric functions", "[math][trig]" )
{
	SECTION( "basic trig functions" )
	{
		REQUIRE_THAT( math::sin( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::sin( math::pi<float> / 2.0f ), WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( math::cos( 0.0f ), WithinRel( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::cos( math::pi<float> ), WithinRel( -1.0f, 1e-5f ) );
		REQUIRE_THAT( math::tan( math::pi<double> / 4.0 ), WithinRel( 1.0, 1e-10 ) );
	}

	SECTION( "inverse trig functions" )
	{
		REQUIRE_THAT( math::asin( 1.0f ), WithinRel( math::pi<float> / 2.0f, 1e-5f ) );
		REQUIRE_THAT( math::acos( 1.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::atan( 1.0 ), WithinRel( math::pi<double> / 4.0, 1e-10 ) );
	}

	SECTION( "atan2 function" )
	{
		REQUIRE_THAT( math::atan2( 1.0f, 1.0f ), WithinRel( math::pi<float> / 4.0f, 1e-5f ) );
		REQUIRE_THAT( math::atan2( 1.0, 0.0 ), WithinRel( math::pi<double> / 2.0, 1e-10 ) );
		REQUIRE_THAT( math::atan2( 0.0f, 1.0f ), WithinAbs( 0.0f, 1e-6f ) );
	}
}

// Test smoothing functions
TEST_CASE( "Smoothing functions", "[math][smooth]" )
{
	SECTION( "smoothstep" )
	{
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 1.0f ), WithinRel( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 0.5f ), WithinRel( 0.5f, 1e-5f ) );

		// Test that smoothstep clamps values outside [0,1]
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, -1.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 2.0f ), WithinRel( 1.0f, 1e-6f ) );
	}

	SECTION( "smootherstep" )
	{
		REQUIRE_THAT( math::smootherstep( 0.0f, 1.0f, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::smootherstep( 0.0f, 1.0f, 1.0f ), WithinRel( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::smootherstep( 0.0f, 1.0f, 0.5f ), WithinRel( 0.5f, 1e-5f ) );

		// Test that smootherstep clamps values outside [0,1]
		REQUIRE_THAT( math::smootherstep( 0.0f, 1.0f, -1.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::smootherstep( 0.0f, 1.0f, 2.0f ), WithinRel( 1.0f, 1e-6f ) );
	}

	SECTION( "smoothstep vs smootherstep comparison" )
	{
		// Both should give same results at endpoints
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 0.0f ), WithinAbs( math::smootherstep( 0.0f, 1.0f, 0.0f ), 1e-6f ) );
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 1.0f ), WithinAbs( math::smootherstep( 0.0f, 1.0f, 1.0f ), 1e-6f ) );
		REQUIRE_THAT( math::smoothstep( 0.0f, 1.0f, 0.5f ), WithinAbs( math::smootherstep( 0.0f, 1.0f, 0.5f ), 1e-6f ) );
	}
}

// Test rounding functions
TEST_CASE( "Rounding functions", "[math][rounding]" )
{
	SECTION( "floor function" )
	{
		REQUIRE_THAT( math::floor( 3.7f ), WithinAbs( 3.0f, 1e-6f ) );
		REQUIRE_THAT( math::floor( -2.3 ), WithinAbs( -3.0, 1e-10 ) );
		REQUIRE_THAT( math::floor( 5.0f ), WithinAbs( 5.0f, 1e-6f ) );
	}

	SECTION( "ceil function" )
	{
		REQUIRE_THAT( math::ceil( 3.2f ), WithinAbs( 4.0f, 1e-6f ) );
		REQUIRE_THAT( math::ceil( -2.7 ), WithinAbs( -2.0, 1e-10 ) );
		REQUIRE_THAT( math::ceil( 5.0f ), WithinAbs( 5.0f, 1e-6f ) );
	}

	SECTION( "round function" )
	{
		REQUIRE_THAT( math::round( 3.7f ), WithinAbs( 4.0f, 1e-6f ) );
		REQUIRE_THAT( math::round( 3.2f ), WithinAbs( 3.0f, 1e-6f ) );
		REQUIRE_THAT( math::round( -2.7 ), WithinAbs( -3.0, 1e-10 ) );
		REQUIRE_THAT( math::round( -2.2 ), WithinAbs( -2.0, 1e-10 ) );
	}

	SECTION( "frac function" )
	{
		REQUIRE_THAT( math::frac( 3.7f ), WithinRel( 0.7f, 1e-5f ) );
		REQUIRE_THAT( math::frac( -2.3f ), WithinRel( 0.7f, 1e-5f ) ); // frac(-2.3) = -2.3 - floor(-2.3) = -2.3 - (-3) = 0.7
		REQUIRE_THAT( math::frac( 5.0 ), WithinAbs( 0.0, 1e-10 ) );
	}
}

// Test modulo and wrap functions
TEST_CASE( "Modulo and wrap functions", "[math][mod]" )
{
	SECTION( "modulo function" )
	{
		REQUIRE_THAT( math::mod( 7.0f, 3.0f ), WithinRel( 1.0f, 1e-5f ) );
		REQUIRE_THAT( math::mod( -7.0f, 3.0f ), WithinRel( 2.0f, 1e-5f ) );
		REQUIRE_THAT( math::mod( 6.0, 3.0 ), WithinAbs( 0.0, 1e-10 ) );
	}

	SECTION( "wrap function" )
	{
		REQUIRE_THAT( math::wrap( 7.0f, 5.0f ), WithinRel( 2.0f, 1e-5f ) );
		REQUIRE_THAT( math::wrap( 5.0f, 5.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::wrap( 12.5, 10.0 ), WithinRel( 2.5, 1e-10 ) );
	}
}

// Test step function
TEST_CASE( "Step function", "[math][step]" )
{
	REQUIRE( math::step( 5.0f, 3.0f ) == 0.0f );
	REQUIRE( math::step( 5.0f, 7.0f ) == 1.0f );
	REQUIRE( math::step( 5.0f, 5.0f ) == 1.0f ); // x >= edge returns 1

	REQUIRE( math::step( 0.0, -1.0 ) == 0.0 );
	REQUIRE( math::step( 0.0, 1.0 ) == 1.0 );
	REQUIRE( math::step( 0.0, 0.0 ) == 1.0 );
}

// Test power-of-two functions
TEST_CASE( "Power of two functions", "[math][power2]" )
{
	SECTION( "isPowerOfTwo function" )
	{
		REQUIRE( math::isPowerOfTwo( 1u ) );
		REQUIRE( math::isPowerOfTwo( 2u ) );
		REQUIRE( math::isPowerOfTwo( 4u ) );
		REQUIRE( math::isPowerOfTwo( 8u ) );
		REQUIRE( math::isPowerOfTwo( 16u ) );
		REQUIRE( math::isPowerOfTwo( 1024u ) );

		REQUIRE_FALSE( math::isPowerOfTwo( 0u ) );
		REQUIRE_FALSE( math::isPowerOfTwo( 3u ) );
		REQUIRE_FALSE( math::isPowerOfTwo( 5u ) );
		REQUIRE_FALSE( math::isPowerOfTwo( 6u ) );
		REQUIRE_FALSE( math::isPowerOfTwo( 7u ) );
		REQUIRE_FALSE( math::isPowerOfTwo( 9u ) );
	}

	SECTION( "nextPowerOfTwo function" )
	{
		REQUIRE( math::nextPowerOfTwo( 1u ) == 2u );
		REQUIRE( math::nextPowerOfTwo( 2u ) == 4u );
		REQUIRE( math::nextPowerOfTwo( 3u ) == 4u );
		REQUIRE( math::nextPowerOfTwo( 5u ) == 8u );
		REQUIRE( math::nextPowerOfTwo( 9u ) == 16u );
		REQUIRE( math::nextPowerOfTwo( 17u ) == 32u );
		REQUIRE( math::nextPowerOfTwo( 1000u ) == 1024u );
	}
}

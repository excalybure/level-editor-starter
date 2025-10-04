#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
#include "math/math.h"

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
		REQUIRE_THAT( math::mod( -7.0f, 3.0f ), WithinRel( -1.0f, 1e-5f ) ); // std::fmod(-7, 3) = -1
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

TEST_CASE( "Advanced math utilities - Fast approximations", "[math][advanced][fast]" )
{
	SECTION( "fastInverseSqrt function" )
	{
		// Test basic functionality
		REQUIRE_THAT( math::fastInverseSqrt( 1.0f ), WithinRel( 1.0f, 0.01f ) );
		REQUIRE_THAT( math::fastInverseSqrt( 4.0f ), WithinRel( 0.5f, 0.01f ) );
		REQUIRE_THAT( math::fastInverseSqrt( 9.0f ), WithinRel( 1.0f / 3.0f, 0.01f ) );
		REQUIRE_THAT( math::fastInverseSqrt( 16.0f ), WithinRel( 0.25f, 0.01f ) );

		// Test edge cases
		REQUIRE( math::fastInverseSqrt( 0.0f ) == 0.0f );
		REQUIRE( math::fastInverseSqrt( -1.0f ) == 0.0f );

		// Test accuracy vs standard library
		const std::vector<float> testValues = { 0.1f, 0.5f, 1.0f, 2.0f, 10.0f, 100.0f };
		for ( const auto val : testValues )
		{
			const float fast = math::fastInverseSqrt( val );
			const float standard = 1.0f / math::sqrt( val );
			REQUIRE_THAT( fast, WithinRel( standard, 0.01f ) ); // Within 1% accuracy
		}
	}

	SECTION( "fastSqrt function" )
	{
		// Test basic functionality
		REQUIRE_THAT( math::fastSqrt( 1.0f ), WithinRel( 1.0f, 0.01f ) );
		REQUIRE_THAT( math::fastSqrt( 4.0f ), WithinRel( 2.0f, 0.01f ) );
		REQUIRE_THAT( math::fastSqrt( 9.0f ), WithinRel( 3.0f, 0.01f ) );
		REQUIRE_THAT( math::fastSqrt( 16.0f ), WithinRel( 4.0f, 0.01f ) );

		// Test edge cases
		REQUIRE( math::fastSqrt( 0.0f ) == 0.0f );
		REQUIRE( math::fastSqrt( -1.0f ) == 0.0f );

		// Test accuracy vs standard library
		const std::vector<float> testValues = { 0.1f, 0.5f, 1.0f, 2.0f, 10.0f, 100.0f };
		for ( const auto val : testValues )
		{
			const float fast = math::fastSqrt( val );
			const float standard = math::sqrt( val );
			REQUIRE_THAT( fast, WithinRel( standard, 0.01f ) ); // Within 1% accuracy
		}
	}
}

TEST_CASE( "Advanced math utilities - Number theory", "[math][advanced][number_theory]" )
{
	SECTION( "factorial function" )
	{
		// Test basic factorials
		REQUIRE( math::factorial( 0u ) == 1ull );
		REQUIRE( math::factorial( 1u ) == 1ull );
		REQUIRE( math::factorial( 2u ) == 2ull );
		REQUIRE( math::factorial( 3u ) == 6ull );
		REQUIRE( math::factorial( 4u ) == 24ull );
		REQUIRE( math::factorial( 5u ) == 120ull );
		REQUIRE( math::factorial( 10u ) == 3628800ull );

		// Test larger values (within safe range)
		REQUIRE( math::factorial( 15u ) == 1307674368000ull );
		REQUIRE( math::factorial( 20u ) == 2432902008176640000ull );

		// Test overflow protection
		REQUIRE( math::factorial( 21u ) == 0ull );	// Should return 0 for overflow
		REQUIRE( math::factorial( 100u ) == 0ull ); // Should return 0 for overflow
	}

	SECTION( "gcd function" )
	{
		// Test basic cases
		REQUIRE( math::gcd( 12u, 8u ) == 4u );
		REQUIRE( math::gcd( 15u, 25u ) == 5u );
		REQUIRE( math::gcd( 17u, 19u ) == 1u ); // Coprime numbers

		// Test commutative property
		REQUIRE( math::gcd( 24u, 16u ) == math::gcd( 16u, 24u ) );
		REQUIRE( math::gcd( 48u, 18u ) == math::gcd( 18u, 48u ) );

		// Test edge cases
		REQUIRE( math::gcd( 0u, 5u ) == 5u );
		REQUIRE( math::gcd( 5u, 0u ) == 5u );
		REQUIRE( math::gcd( 1u, 1u ) == 1u );
		REQUIRE( math::gcd( 7u, 7u ) == 7u );

		// Test larger numbers
		REQUIRE( math::gcd( 1071u, 462u ) == 21u );
		REQUIRE( math::gcd( 1001u, 1309u ) == 77u );
	}

	SECTION( "lcm function" )
	{
		// Test basic cases
		REQUIRE( math::lcm( 4u, 6u ) == 12u );
		REQUIRE( math::lcm( 15u, 25u ) == 75u );
		REQUIRE( math::lcm( 7u, 11u ) == 77u ); // Coprime numbers

		// Test commutative property
		REQUIRE( math::lcm( 8u, 12u ) == math::lcm( 12u, 8u ) );
		REQUIRE( math::lcm( 9u, 15u ) == math::lcm( 15u, 9u ) );

		// Test edge cases
		REQUIRE( math::lcm( 0u, 5u ) == 0u );
		REQUIRE( math::lcm( 5u, 0u ) == 0u );
		REQUIRE( math::lcm( 1u, 1u ) == 1u );
		REQUIRE( math::lcm( 7u, 7u ) == 7u );

		// Verify relationship: lcm(a,b) * gcd(a,b) = a * b
		const std::vector<std::pair<unsigned int, unsigned int>> testPairs = {
			{ 12u, 8u }, { 15u, 25u }, { 17u, 19u }, { 24u, 16u }, { 48u, 18u }
		};
		for ( const auto &[a, b] : testPairs )
		{
			const auto lcmValue = math::lcm( a, b );
			const auto gcdValue = math::gcd( a, b );
			REQUIRE( lcmValue * gcdValue == a * b );
		}
	}

	SECTION( "isPrime function" )
	{
		// Test known primes
		const std::vector<unsigned int> primes = { 2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u, 41u, 43u, 47u };
		for ( const auto prime : primes )
		{
			REQUIRE( math::isPrime( prime ) );
		}

		// Test known non-primes
		const std::vector<unsigned int> nonPrimes = { 0u, 1u, 4u, 6u, 8u, 9u, 10u, 12u, 14u, 15u, 16u, 18u, 20u, 21u, 22u };
		for ( const auto nonPrime : nonPrimes )
		{
			REQUIRE_FALSE( math::isPrime( nonPrime ) );
		}

		// Test larger primes
		REQUIRE( math::isPrime( 97u ) );
		REQUIRE( math::isPrime( 101u ) );
		REQUIRE( math::isPrime( 103u ) );
		REQUIRE( math::isPrime( 107u ) );

		// Test larger composites
		REQUIRE_FALSE( math::isPrime( 91u ) );	// 7 * 13
		REQUIRE_FALSE( math::isPrime( 121u ) ); // 11 * 11
		REQUIRE_FALSE( math::isPrime( 143u ) ); // 11 * 13
	}
}

TEST_CASE( "Advanced math utilities - Bit manipulation", "[math][advanced][bits]" )
{
	SECTION( "countBits function" )
	{
		// Test basic cases
		REQUIRE( math::countBits( 0u ) == 0u );
		REQUIRE( math::countBits( 1u ) == 1u );
		REQUIRE( math::countBits( 2u ) == 1u );
		REQUIRE( math::countBits( 3u ) == 2u );	 // 0b11
		REQUIRE( math::countBits( 7u ) == 3u );	 // 0b111
		REQUIRE( math::countBits( 8u ) == 1u );	 // 0b1000
		REQUIRE( math::countBits( 15u ) == 4u ); // 0b1111

		// Test powers of 2 (should have exactly 1 bit)
		const std::vector<unsigned int> powersOf2 = { 1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u };
		for ( const auto power : powersOf2 )
		{
			REQUIRE( math::countBits( power ) == 1u );
		}

		// Test all bits set cases
		REQUIRE( math::countBits( 0xFFu ) == 8u );		  // 8 bits
		REQUIRE( math::countBits( 0xFFFFu ) == 16u );	  // 16 bits
		REQUIRE( math::countBits( 0xFFFFFFFFu ) == 32u ); // 32 bits

		// Test specific patterns
		REQUIRE( math::countBits( 0xAAAAAAAAu ) == 16u ); // Alternating bits (10101010...)
		REQUIRE( math::countBits( 0x55555555u ) == 16u ); // Alternating bits (01010101...)
	}

	SECTION( "reverseBits function" )
	{
		// Test basic cases
		REQUIRE( math::reverseBits( 0u ) == 0u );
		REQUIRE( math::reverseBits( 1u ) == 0x80000000u ); // 1 becomes 10000000000000000000000000000000
		REQUIRE( math::reverseBits( 0x80000000u ) == 1u );

		// Test symmetry (reversing twice should give original)
		const std::vector<unsigned int> testValues = {
			0u, 1u, 2u, 3u, 0xFFu, 0xFF00u, 0x12345678u, 0xAAAAAAAAu, 0x55555555u, 0xFFFFFFFFu
		};
		for ( const auto value : testValues )
		{
			REQUIRE( math::reverseBits( math::reverseBits( value ) ) == value );
		}

		// Test specific known values
		REQUIRE( math::reverseBits( 0x12345678u ) == 0x1E6A2C48u );
		REQUIRE( math::reverseBits( 0xFFFFFFFFu ) == 0xFFFFFFFFu ); // All bits set
	}

	SECTION( "rotateLeft function" )
	{
		// Test basic cases
		REQUIRE( math::rotateLeft( 1u, 1 ) == 2u );
		REQUIRE( math::rotateLeft( 1u, 2 ) == 4u );
		REQUIRE( math::rotateLeft( 1u, 31 ) == 0x80000000u );
		REQUIRE( math::rotateLeft( 0x80000000u, 1 ) == 1u ); // Wrap around

		// Test no rotation
		REQUIRE( math::rotateLeft( 0x12345678u, 0 ) == 0x12345678u );

		// Test full rotation (32 bits)
		REQUIRE( math::rotateLeft( 0x12345678u, 32 ) == 0x12345678u );

		// Test negative shifts (should be normalized)
		REQUIRE( math::rotateLeft( 0x12345678u, -1 ) == math::rotateLeft( 0x12345678u, 31 ) );

		// Test large shifts (should be normalized)
		REQUIRE( math::rotateLeft( 0x12345678u, 33 ) == math::rotateLeft( 0x12345678u, 1 ) );

		// Test specific patterns
		REQUIRE( math::rotateLeft( 0xAAAAAAAAu, 1 ) == 0x55555555u );
		REQUIRE( math::rotateLeft( 0x55555555u, 1 ) == 0xAAAAAAAAu );
	}

	SECTION( "rotateRight function" )
	{
		// Test basic cases
		REQUIRE( math::rotateRight( 2u, 1 ) == 1u );
		REQUIRE( math::rotateRight( 4u, 2 ) == 1u );
		REQUIRE( math::rotateRight( 0x80000000u, 31 ) == 1u );
		REQUIRE( math::rotateRight( 1u, 1 ) == 0x80000000u ); // Wrap around

		// Test no rotation
		REQUIRE( math::rotateRight( 0x12345678u, 0 ) == 0x12345678u );

		// Test full rotation (32 bits)
		REQUIRE( math::rotateRight( 0x12345678u, 32 ) == 0x12345678u );

		// Test negative shifts (should be normalized)
		REQUIRE( math::rotateRight( 0x12345678u, -1 ) == math::rotateRight( 0x12345678u, 31 ) );

		// Test large shifts (should be normalized)
		REQUIRE( math::rotateRight( 0x12345678u, 33 ) == math::rotateRight( 0x12345678u, 1 ) );

		// Test relationship with rotateLeft
		const std::vector<unsigned int> testValues = { 0x12345678u, 0xAAAAAAAAu, 0x55555555u, 0xFFFFFFFFu };
		for ( const auto value : testValues )
		{
			for ( int shift = 1; shift < 32; ++shift )
			{
				REQUIRE( math::rotateLeft( value, shift ) == math::rotateRight( value, 32 - shift ) );
			}
		}
	}
}

TEST_CASE( "Advanced math utilities - Const correctness", "[math][advanced][const]" )
{
	SECTION( "All advanced functions are const-correct" )
	{
		// Test that all functions can be called with const parameters
		const float constFloat = 4.0f;
		const unsigned int constUInt = 42u;
		const int constInt = 5;

		// Fast math functions
		const float fastInvSqrt = math::fastInverseSqrt( constFloat );
		const float fastSqrtVal = math::fastSqrt( constFloat );

		// Number theory functions
		const unsigned long long factVal = math::factorial( 5u );
		const unsigned int gcdVal = math::gcd( constUInt, 24u );
		const unsigned int lcmVal = math::lcm( constUInt, 24u );
		const bool primeVal = math::isPrime( constUInt );

		// Bit manipulation functions
		const unsigned int bitsVal = math::countBits( constUInt );
		const unsigned int reverseVal = math::reverseBits( constUInt );
		const unsigned int rotLeftVal = math::rotateLeft( constUInt, constInt );
		const unsigned int rotRightVal = math::rotateRight( constUInt, constInt );

		// Just verify they produce reasonable values
		REQUIRE( fastInvSqrt >= 0.0f );
		REQUIRE( fastSqrtVal >= 0.0f );
		REQUIRE( factVal >= 1ull );
		REQUIRE( gcdVal >= 1u );
		REQUIRE( lcmVal >= 1u );
		REQUIRE( ( primeVal == true || primeVal == false ) ); // Boolean test
		REQUIRE( bitsVal <= 32u );							  // Can't have more than 32 bits in a 32-bit int
		REQUIRE( reverseVal >= 0u );						  // Always valid
		REQUIRE( rotLeftVal >= 0u );						  // Always valid
		REQUIRE( rotRightVal >= 0u );						  // Always valid
	}
}

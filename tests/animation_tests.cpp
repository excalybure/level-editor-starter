#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <cmath>
#include "engine/math/animation.h"

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

// Test easing functions
TEST_CASE( "Quadratic easing functions", "[math][animation]" )
{
	SECTION( "easeInQuad boundary conditions" )
	{
		REQUIRE_THAT( math::easeInQuad( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInQuad( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInQuad monotonic increasing" )
	{
		const std::vector<float> testValues = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
		for ( size_t i = 0; i < testValues.size() - 1; ++i )
		{
			const float current = math::easeInQuad( testValues[i] );
			const float next = math::easeInQuad( testValues[i + 1] );
			REQUIRE( next > current );
		}
	}

	SECTION( "easeInQuad mathematical correctness" )
	{
		REQUIRE_THAT( math::easeInQuad( 0.5f ), WithinAbs( 0.25f, 1e-6f ) );
		REQUIRE_THAT( math::easeInQuad( 0.8f ), WithinAbs( 0.64f, 1e-6f ) );
	}

	SECTION( "easeOutQuad boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutQuad( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutQuad( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeOutQuad monotonic increasing" )
	{
		const std::vector<float> testValues = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
		for ( size_t i = 0; i < testValues.size() - 1; ++i )
		{
			const float current = math::easeOutQuad( testValues[i] );
			const float next = math::easeOutQuad( testValues[i + 1] );
			REQUIRE( next > current );
		}
	}

	SECTION( "easeInOutQuad boundary conditions" )
	{
		REQUIRE_THAT( math::easeInOutQuad( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutQuad( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutQuad( 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "easeInOutQuad symmetry" )
	{
		// InOut functions should be symmetric around 0.5
		const std::vector<float> testValues = { 0.1f, 0.2f, 0.3f, 0.4f };
		for ( const auto t : testValues )
		{
			const float left = math::easeInOutQuad( t );
			const float right = math::easeInOutQuad( 1.0f - t );
			REQUIRE_THAT( left, WithinAbs( 1.0f - right, 1e-5f ) );
		}
	}
}

TEST_CASE( "Cubic easing functions", "[math][animation]" )
{
	SECTION( "easeInCubic boundary conditions" )
	{
		REQUIRE_THAT( math::easeInCubic( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInCubic( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInCubic mathematical correctness" )
	{
		REQUIRE_THAT( math::easeInCubic( 0.5f ), WithinAbs( 0.125f, 1e-6f ) );
		REQUIRE_THAT( math::easeInCubic( 0.8f ), WithinAbs( 0.512f, 1e-6f ) );
	}

	SECTION( "easeOutCubic boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutCubic( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutCubic( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutCubic boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutCubic( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutCubic( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutCubic( 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
	}
}

TEST_CASE( "Quartic easing functions", "[math][animation]" )
{
	SECTION( "easeInQuart boundary conditions" )
	{
		REQUIRE_THAT( math::easeInQuart( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInQuart( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInQuart mathematical correctness" )
	{
		REQUIRE_THAT( math::easeInQuart( 0.5f ), WithinAbs( 0.0625f, 1e-6f ) ); // 0.5^4
		REQUIRE_THAT( math::easeInQuart( 0.8f ), WithinAbs( 0.4096f, 1e-5f ) ); // 0.8^4
	}

	SECTION( "easeOutQuart boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutQuart( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutQuart( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutQuart boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutQuart( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutQuart( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutQuart( 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
	}
}

TEST_CASE( "Sine easing functions", "[math][animation]" )
{
	SECTION( "easeInSine boundary conditions" )
	{
		REQUIRE_THAT( math::easeInSine( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInSine( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeOutSine boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutSine( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutSine( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutSine boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutSine( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutSine( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutSine( 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "sine easing monotonicity" )
	{
		const std::vector<float> testValues = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

		// Test easeInSine monotonicity
		for ( size_t i = 0; i < testValues.size() - 1; ++i )
		{
			const float current = math::easeInSine( testValues[i] );
			const float next = math::easeInSine( testValues[i + 1] );
			REQUIRE( next > current );
		}

		// Test easeOutSine monotonicity
		for ( size_t i = 0; i < testValues.size() - 1; ++i )
		{
			const float current = math::easeOutSine( testValues[i] );
			const float next = math::easeOutSine( testValues[i + 1] );
			REQUIRE( next > current );
		}
	}
}

TEST_CASE( "Bounce easing functions", "[math][animation]" )
{
	SECTION( "easeInBounce boundary conditions" )
	{
		REQUIRE_THAT( math::easeInBounce( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInBounce( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeOutBounce boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutBounce( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutBounce( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutBounce boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutBounce( 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutBounce( 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutBounce( 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "bounce functions are well-behaved" )
	{
		// Bounce functions should stay within [0,1] range
		const std::vector<float> testValues = { 0.1f, 0.2f, 0.3f, 0.4f, 0.6f, 0.7f, 0.8f, 0.9f };
		for ( const auto t : testValues )
		{
			const float inBounce = math::easeInBounce( t );
			const float outBounce = math::easeOutBounce( t );
			const float inOutBounce = math::easeInOutBounce( t );

			REQUIRE( inBounce >= 0.0f );
			REQUIRE( inBounce <= 1.0f );
			REQUIRE( outBounce >= 0.0f );
			REQUIRE( outBounce <= 1.0f );
			REQUIRE( inOutBounce >= 0.0f );
			REQUIRE( inOutBounce <= 1.0f );
		}
	}
}

TEST_CASE( "Elastic easing functions", "[math][animation]" )
{
	SECTION( "easeInElastic boundary conditions" )
	{
		REQUIRE_THAT( math::easeInElastic( 0.0f, 1.0f, 0.3f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInElastic( 1.0f, 1.0f, 0.3f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeOutElastic boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutElastic( 0.0f, 1.0f, 0.3f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutElastic( 1.0f, 1.0f, 0.3f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutElastic boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutElastic( 0.0f, 1.0f, 0.45f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutElastic( 1.0f, 1.0f, 0.45f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutElastic( 0.5f, 1.0f, 0.45f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "elastic functions with custom parameters" )
	{
		const float customAmplitude = 2.0f;
		const float customPeriod = 0.5f;

		// Test with custom parameters
		REQUIRE_THAT( math::easeInElastic( 0.0f, customAmplitude, customPeriod ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInElastic( 1.0f, customAmplitude, customPeriod ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutElastic( 0.0f, customAmplitude, customPeriod ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutElastic( 1.0f, customAmplitude, customPeriod ), WithinAbs( 1.0f, 1e-6f ) );
	}
}

TEST_CASE( "Back easing functions", "[math][animation]" )
{
	SECTION( "easeInBack boundary conditions" )
	{
		REQUIRE_THAT( math::easeInBack( 0.0f, 1.70158f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInBack( 1.0f, 1.70158f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeOutBack boundary conditions" )
	{
		REQUIRE_THAT( math::easeOutBack( 0.0f, 1.70158f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutBack( 1.0f, 1.70158f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInOutBack boundary and midpoint conditions" )
	{
		REQUIRE_THAT( math::easeInOutBack( 0.0f, 1.70158f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutBack( 1.0f, 1.70158f ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInOutBack( 0.5f, 1.70158f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "back functions with custom overshoot" )
	{
		const float customOvershoot = 3.0f;

		// Test with custom overshoot
		REQUIRE_THAT( math::easeInBack( 0.0f, customOvershoot ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeInBack( 1.0f, customOvershoot ), WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutBack( 0.0f, customOvershoot ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::easeOutBack( 1.0f, customOvershoot ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "easeInBack overshoot behavior" )
	{
		// Back easing should go below 0 at some point (overshoot)
		bool foundNegative = false;
		const std::vector<float> testValues = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
		for ( const auto t : testValues )
		{
			if ( math::easeInBack( t, 1.70158f ) < 0.0f )
			{
				foundNegative = true;
				break;
			}
		}
		REQUIRE( foundNegative );
	}

	SECTION( "easeOutBack overshoot behavior" )
	{
		// Back easing should go above 1 at some point (overshoot)
		bool foundOvershoot = false;
		const std::vector<float> testValues = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
		for ( const auto t : testValues )
		{
			if ( math::easeOutBack( t, 1.70158f ) > 1.0f )
			{
				foundOvershoot = true;
				break;
			}
		}
		REQUIRE( foundOvershoot );
	}
}

TEST_CASE( "Utility functions", "[math][animation]" )
{
	SECTION( "inverseLerp basic functionality" )
	{
		REQUIRE_THAT( math::inverseLerp( 0.0f, 10.0f, 5.0f ), WithinAbs( 0.5f, 1e-6f ) );
		REQUIRE_THAT( math::inverseLerp( -5.0f, 5.0f, 0.0f ), WithinAbs( 0.5f, 1e-6f ) );
		REQUIRE_THAT( math::inverseLerp( 10.0f, 20.0f, 15.0f ), WithinAbs( 0.5f, 1e-6f ) );
	}

	SECTION( "inverseLerp boundary conditions" )
	{
		REQUIRE_THAT( math::inverseLerp( 0.0f, 10.0f, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::inverseLerp( 0.0f, 10.0f, 10.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "inverseLerp handles zero range" )
	{
		// When a == b, should return 0
		REQUIRE_THAT( math::inverseLerp( 5.0f, 5.0f, 5.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::inverseLerp( 5.0f, 5.0f, 10.0f ), WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "remap basic functionality" )
	{
		// Remap 5 from [0,10] to [0,100] should give 50
		REQUIRE_THAT( math::remap( 5.0f, 0.0f, 10.0f, 0.0f, 100.0f ), WithinAbs( 50.0f, 1e-5f ) );

		// Remap 0 from [-1,1] to [10,20] should give 15
		REQUIRE_THAT( math::remap( 0.0f, -1.0f, 1.0f, 10.0f, 20.0f ), WithinAbs( 15.0f, 1e-5f ) );
	}

	SECTION( "remap boundary preservation" )
	{
		// Remapping boundary values should give boundary outputs
		REQUIRE_THAT( math::remap( 0.0f, 0.0f, 10.0f, 20.0f, 30.0f ), WithinAbs( 20.0f, 1e-5f ) );
		REQUIRE_THAT( math::remap( 10.0f, 0.0f, 10.0f, 20.0f, 30.0f ), WithinAbs( 30.0f, 1e-5f ) );
	}

	SECTION( "remap identity transformation" )
	{
		// Remapping to the same range should give the same value
		REQUIRE_THAT( math::remap( 7.5f, 0.0f, 10.0f, 0.0f, 10.0f ), WithinAbs( 7.5f, 1e-5f ) );
	}
}

TEST_CASE( "EaseType enumeration and dispatcher", "[math][animation]" )
{
	SECTION( "ease() function with Linear type" )
	{
		REQUIRE_THAT( math::ease( math::EaseType::Linear, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::Linear, 0.5f ), WithinAbs( 0.5f, 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::Linear, 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
	}

	SECTION( "ease() function dispatches correctly" )
	{
		const float t = 0.5f;

		// Test that dispatcher matches direct function calls
		REQUIRE_THAT( math::ease( math::EaseType::InQuad, t ), WithinAbs( math::easeInQuad( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::OutQuad, t ), WithinAbs( math::easeOutQuad( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::InOutQuad, t ), WithinAbs( math::easeInOutQuad( t ), 1e-6f ) );

		REQUIRE_THAT( math::ease( math::EaseType::InCubic, t ), WithinAbs( math::easeInCubic( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::OutCubic, t ), WithinAbs( math::easeOutCubic( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::InOutCubic, t ), WithinAbs( math::easeInOutCubic( t ), 1e-6f ) );

		REQUIRE_THAT( math::ease( math::EaseType::InQuart, t ), WithinAbs( math::easeInQuart( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::OutQuart, t ), WithinAbs( math::easeOutQuart( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::InOutQuart, t ), WithinAbs( math::easeInOutQuart( t ), 1e-6f ) );

		REQUIRE_THAT( math::ease( math::EaseType::InSine, t ), WithinAbs( math::easeInSine( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::OutSine, t ), WithinAbs( math::easeOutSine( t ), 1e-6f ) );
		REQUIRE_THAT( math::ease( math::EaseType::InOutSine, t ), WithinAbs( math::easeInOutSine( t ), 1e-6f ) );
	}

	SECTION( "all ease types maintain boundary conditions" )
	{
		const std::vector<math::EaseType> allTypes = {
			math::EaseType::Linear,
			math::EaseType::InQuad,
			math::EaseType::OutQuad,
			math::EaseType::InOutQuad,
			math::EaseType::InCubic,
			math::EaseType::OutCubic,
			math::EaseType::InOutCubic,
			math::EaseType::InQuart,
			math::EaseType::OutQuart,
			math::EaseType::InOutQuart,
			math::EaseType::InSine,
			math::EaseType::OutSine,
			math::EaseType::InOutSine,
			math::EaseType::InBounce,
			math::EaseType::OutBounce,
			math::EaseType::InOutBounce,
			math::EaseType::InElastic,
			math::EaseType::OutElastic,
			math::EaseType::InOutElastic,
			math::EaseType::InBack,
			math::EaseType::OutBack,
			math::EaseType::InOutBack
		};

		for ( const auto type : allTypes )
		{
			REQUIRE_THAT( math::ease( type, 0.0f ), WithinAbs( 0.0f, 1e-6f ) );
			REQUIRE_THAT( math::ease( type, 1.0f ), WithinAbs( 1.0f, 1e-6f ) );
		}
	}
}

TEST_CASE( "Const-correctness validation", "[math][animation]" )
{
	SECTION( "all easing functions are const-correct" )
	{
		// Test that all functions can be called with const parameters
		const float t = 0.5f;
		const float amplitude = 1.0f;
		const float period = 0.3f;
		const float overshoot = 1.70158f;

		// These should all compile without issues (const-correctness test)
		const float quad = math::easeInQuad( t );
		const float cubic = math::easeInCubic( t );
		const float quart = math::easeInQuart( t );
		const float sine = math::easeInSine( t );
		const float bounce = math::easeInBounce( t );
		const float elastic = math::easeInElastic( t, amplitude, period );
		const float back = math::easeInBack( t, overshoot );

		// Test utility functions
		const float invLerp = math::inverseLerp( 0.0f, 10.0f, 5.0f );
		const float remapped = math::remap( 5.0f, 0.0f, 10.0f, 0.0f, 100.0f );
		const float eased = math::ease( math::EaseType::Linear, t );

		// Just verify they produce reasonable values
		REQUIRE( quad >= 0.0f );
		REQUIRE( cubic >= 0.0f );
		REQUIRE( quart >= 0.0f );
		REQUIRE( sine >= 0.0f );
		REQUIRE( bounce >= 0.0f );
		REQUIRE( elastic >= -1.0f ); // Elastic can oscillate below 0 due to its nature
		REQUIRE( back >= -1.0f );	 // Back can go negative due to overshoot
		REQUIRE( invLerp >= 0.0f );
		REQUIRE( remapped >= 0.0f );
		REQUIRE( eased >= 0.0f );
	}
}

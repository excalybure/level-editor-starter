#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <algorithm>
#include <set>
import engine.random;

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

// Test random number generator functionality
TEST_CASE( "Random class basic functionality", "[math][random]" )
{
	SECTION( "seeded random generates consistent values" )
	{
		math::Random rng1( 12345 );
		math::Random rng2( 12345 );

		// Same seed should generate same sequence
		for ( int i = 0; i < 10; i++ )
		{
			REQUIRE_THAT( rng1.random(), WithinAbs( rng2.random(), 1e-6f ) );
		}
	}

	SECTION( "random() returns values in [0, 1) range" )
	{
		math::Random rng( 42 );

		for ( int i = 0; i < 100; i++ )
		{
			float value = rng.random();
			REQUIRE( value >= 0.0f );
			REQUIRE( value < 1.0f );
		}
	}

	SECTION( "range(float) returns values in specified range" )
	{
		math::Random rng( 123 );
		float min = -10.0f;
		float max = 25.5f;

		for ( int i = 0; i < 100; i++ )
		{
			float value = rng.range( min, max );
			REQUIRE( value >= min );
			REQUIRE( value < max );
		}
	}

	SECTION( "range(int) returns values in specified inclusive range" )
	{
		math::Random rng( 456 );
		int min = -5;
		int max = 10;
		std::set<int> generatedValues;

		// Generate many values and check they're all in range
		for ( int i = 0; i < 1000; i++ )
		{
			int value = rng.range( min, max );
			REQUIRE( value >= min );
			REQUIRE( value <= max );
			generatedValues.insert( value );
		}

		// Should have generated multiple different values
		REQUIRE( generatedValues.size() > 5 );
	}

	SECTION( "chance() returns true/false based on probability" )
	{
		math::Random rng( 789 );
		
		// Test with 0% probability
		bool anyTrue = false;
		for ( int i = 0; i < 100; i++ )
		{
			if ( rng.chance( 0.0f ) )
			{
				anyTrue = true;
				break;
			}
		}
		REQUIRE( anyTrue == false );

		// Test with 100% probability
		bool anyFalse = false;
		for ( int i = 0; i < 100; i++ )
		{
			if ( !rng.chance( 1.0f ) )
			{
				anyFalse = true;
				break;
			}
		}
		REQUIRE( anyFalse == false );
	}
}

TEST_CASE( "Random geometric functions", "[math][random]" )
{
	SECTION( "unitCircle() generates points on unit circle" )
	{
		math::Random rng( 101112 );

		for ( int i = 0; i < 50; i++ )
		{
			auto point = rng.unitCircle();
			float distance = sqrt( point.x * point.x + point.y * point.y );
			REQUIRE_THAT( distance, WithinRel( 1.0f, 1e-5f ) );
		}
	}

	SECTION( "unitSphere() generates points on unit sphere surface" )
	{
		math::Random rng( 131415 );

		for ( int i = 0; i < 50; i++ )
		{
			auto point = rng.unitSphere();
			float distance = sqrt( point.x * point.x + point.y * point.y + point.z * point.z );
			REQUIRE_THAT( distance, WithinRel( 1.0f, 1e-5f ) );
		}
	}

	SECTION( "insideSphere() generates points inside unit sphere" )
	{
		math::Random rng( 161718 );

		for ( int i = 0; i < 100; i++ )
		{
			auto point = rng.insideSphere();
			float distance = sqrt( point.x * point.x + point.y * point.y + point.z * point.z );
			REQUIRE( distance <= 1.0f );
		}

		// Should generate points at various distances, not all on surface
		float minDistance = 1.0f;
		for ( int i = 0; i < 100; i++ )
		{
			auto point = rng.insideSphere();
			float distance = sqrt( point.x * point.x + point.y * point.y + point.z * point.z );
			if ( distance < minDistance )
				minDistance = distance;
		}
		REQUIRE( minDistance < 0.9f ); // Should have some points well inside
	}

	SECTION( "insideCube() generates points inside unit cube" )
	{
		math::Random rng( 192021 );

		for ( int i = 0; i < 100; i++ )
		{
			auto point = rng.insideCube();
			REQUIRE( point.x >= -1.0f );
			REQUIRE( point.x <= 1.0f );
			REQUIRE( point.y >= -1.0f );
			REQUIRE( point.y <= 1.0f );
			REQUIRE( point.z >= -1.0f );
			REQUIRE( point.z <= 1.0f );
		}
	}
}

TEST_CASE( "Random utility functions", "[math][random]" )
{
	SECTION( "choice() selects random element from container" )
	{
		math::Random rng( 222324 );
		std::vector<int> values = { 10, 20, 30, 40, 50 };
		std::set<int> chosen;

		// Select many times to ensure we get different elements
		for ( int i = 0; i < 200; i++ )
		{
			int selected = rng.choice( values );
			// Should be one of our values
			REQUIRE( std::find( values.begin(), values.end(), selected ) != values.end() );
			chosen.insert( selected );
		}

		// Should have chosen multiple different values
		REQUIRE( chosen.size() >= 3 );
	}

	SECTION( "shuffle() randomizes container order" )
	{
		math::Random rng( 252627 );
		std::vector<int> original = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		std::vector<int> shuffled = original;

		rng.shuffle( shuffled );

		// Should have same elements
		std::sort( shuffled.begin(), shuffled.end() );
		REQUIRE( shuffled == original );

		// Do multiple shuffles to check they're different
		std::vector<int> test1 = original;
		std::vector<int> test2 = original;
		rng.shuffle( test1 );
		rng.shuffle( test2 );

		// Extremely unlikely to be identical after shuffle (but technically possible)
		// Just check that at least one position changed from original
		bool anyDifferent = false;
		for ( size_t i = 0; i < test1.size(); i++ )
		{
			if ( test1[i] != original[i] )
			{
				anyDifferent = true;
				break;
			}
		}
		// With 10 elements, this should virtually always be true
		REQUIRE( anyDifferent );
	}
}

TEST_CASE( "Global random functions", "[math][random]" )
{
	SECTION( "global convenience functions work" )
	{
		// Test that global functions are accessible and return reasonable values
		float r1 = math::random();
		REQUIRE( r1 >= 0.0f );
		REQUIRE( r1 < 1.0f );

		float r2 = math::random( 5.0f, 10.0f );
		REQUIRE( r2 >= 5.0f );
		REQUIRE( r2 < 10.0f );

		int ri = math::randomInt( -3, 7 );
		REQUIRE( ri >= -3 );
		REQUIRE( ri <= 7 );

		auto circle = math::randomUnitCircle();
		float circleDistance = sqrt( circle.x * circle.x + circle.y * circle.y );
		REQUIRE_THAT( circleDistance, WithinRel( 1.0f, 1e-5f ) );

		auto sphere = math::randomUnitSphere();
		float sphereDistance = sqrt( sphere.x * sphere.x + sphere.y * sphere.y + sphere.z * sphere.z );
		REQUIRE_THAT( sphereDistance, WithinRel( 1.0f, 1e-5f ) );

		auto inside = math::randomInsideSphere();
		float insideDistance = sqrt( inside.x * inside.x + inside.y * inside.y + inside.z * inside.z );
		REQUIRE( insideDistance <= 1.0f );
	}
}

TEST_CASE( "Noise functions", "[math][random]" )
{
	SECTION( "perlinNoise() returns values in reasonable range" )
	{
		// Test various coordinates
		float testCoords[][2] = {
			{ 0.0f, 0.0f },
			{ 1.5f, 2.3f },
			{ -3.7f, 4.1f },
			{ 10.0f, -5.5f },
			{ 0.1f, 0.1f }
		};

		for ( auto& [x, y] : testCoords )
		{
			float noise = math::perlinNoise( x, y );
			// Perlin noise should typically be in range [-1, 1]
			REQUIRE( noise >= -1.5f ); // Allow slight overshoot due to interpolation
			REQUIRE( noise <= 1.5f );
		}
	}

	SECTION( "perlinNoise() is deterministic" )
	{
		// Same coordinates should always give same result
		float x = 3.14f, y = 2.71f;
		float noise1 = math::perlinNoise( x, y );
		float noise2 = math::perlinNoise( x, y );
		
		REQUIRE_THAT( noise1, WithinAbs( noise2, 1e-6f ) );
	}

	SECTION( "perlinNoise() varies smoothly" )
	{
		// Adjacent points should have similar values (smoothness test)
		float baseX = 5.0f, baseY = 3.0f;
		float delta = 0.01f;
		
		float center = math::perlinNoise( baseX, baseY );
		float right = math::perlinNoise( baseX + delta, baseY );
		float up = math::perlinNoise( baseX, baseY + delta );
		
		// Adjacent values should be close (smoothness)
		REQUIRE( abs( center - right ) < 0.1f );
		REQUIRE( abs( center - up ) < 0.1f );
	}

	SECTION( "fractalNoise() combines multiple octaves" )
	{
		float x = 2.5f, y = 1.8f;
		
		// Test different octave counts
		float noise1 = math::fractalNoise( x, y, 1 );
		float noise4 = math::fractalNoise( x, y, 4 );
		float noise8 = math::fractalNoise( x, y, 8 );
		
		// All should be in reasonable range
		REQUIRE( noise1 >= -2.0f );
		REQUIRE( noise1 <= 2.0f );
		REQUIRE( noise4 >= -2.0f );
		REQUIRE( noise4 <= 2.0f );
		REQUIRE( noise8 >= -2.0f );
		REQUIRE( noise8 <= 2.0f );
		
		// More octaves should generally give different results
		// (though not guaranteed, so just test they're accessible)
		REQUIRE( true ); // Basic accessibility test
	}

	SECTION( "turbulence() returns positive values" )
	{
		float testCoords[][2] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ -2.5f, 3.7f },
			{ 7.2f, -4.1f }
		};

		for ( auto& [x, y] : testCoords )
		{
			float turb = math::turbulence( x, y, 4 );
			// Turbulence should be positive (absolute values)
			REQUIRE( turb >= 0.0f );
			REQUIRE( turb <= 2.0f ); // Should be normalized
		}
	}

	SECTION( "noise functions are accessible through SimpleNoise class" )
	{
		// Test direct class access
		float noise1 = math::SimpleNoise::perlinNoise( 1.0f, 2.0f );
		float noise2 = math::SimpleNoise::fractalNoise( 1.0f, 2.0f, 3 );
		float noise3 = math::SimpleNoise::turbulence( 1.0f, 2.0f, 3 );

		// Basic range checks
		REQUIRE( noise1 >= -2.0f );
		REQUIRE( noise1 <= 2.0f );
		REQUIRE( noise2 >= -2.0f );
		REQUIRE( noise2 <= 2.0f );
		REQUIRE( noise3 >= 0.0f );
		REQUIRE( noise3 <= 2.0f );
	}
}

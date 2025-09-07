export module engine.random;
import engine.math;
import engine.vec;
import <random>;
import <cstdint>;

export namespace math
{
// Seeded random number generator class
class Random
{
public:
	// Helper struct for disc coordinates - public for external use
	struct DiscPoint
	{
		float x, y, lengthSquared;
	};

private:
	std::mt19937 m_generator;
	std::uniform_real_distribution<float> m_uniformFloat{ 0.0f, 1.0f };

	// Helper function to project disc coordinates onto sphere with given radius
	Vec3f projectOntoSphereSurface( const DiscPoint &disc, const float radius )
	{
		const float scale = radius * 2.0f * math::sqrt( 1.0f - disc.lengthSquared );
		return {
			disc.x * scale,
			disc.y * scale,
			radius * ( 1.0f - 2.0f * disc.lengthSquared )
		};
	}

public:
	// Constructor with optional seed
	Random( unsigned int seed = std::random_device{}() )
		: m_generator( seed )
	{
	}

	// Seed the generator
	void seed( const unsigned int newSeed )
	{
		m_generator.seed( newSeed );
	}

	// Get the current seed (approximation - returns a state-based value)
	// Note: This method modifies internal state and cannot be const
	unsigned int getSeed()
	{
		// Note: std::mt19937 doesn't provide direct seed access
		// This returns a hash of the current state for testing purposes
		return static_cast<unsigned int>( m_generator() );
	}

	// Random float in range [0, 1)
	float random()
	{
		return m_uniformFloat( m_generator );
	}

	// Random float in range [min, max)
	float range( const float min, const float max )
	{
		return min + random() * ( max - min );
	}

	// Random integer in range [min, max] (inclusive)
	int range( const int min, const int max )
	{
		std::uniform_int_distribution<int> dist( min, max );
		return dist( m_generator );
	}

	// Random boolean with given probability of true
	bool chance( const float probability = 0.5f )
	{
		return random() < probability;
	}

	// Random point on unit circle
	Vec2f unitCircle()
	{
		float angle = range( 0.0f, 2.0f * math::pi<float> );
		return { math::cos( angle ), math::sin( angle ) };
	}

	// Generate uniform random point inside unit disc
	// Returns the point coordinates and squared length for reuse
	DiscPoint insideDisc()
	{
		// Use Marsaglia's method for uniform distribution on sphere
		// This avoids clustering at poles that occurs with naive spherical coordinate sampling
		float x, y, lengthSquared;
		do
		{
			x = range( -1.0f, 1.0f );
			y = range( -1.0f, 1.0f );
			lengthSquared = x * x + y * y;
		} while ( lengthSquared >= 1.0f || lengthSquared == 0.0f );

		return { x, y, lengthSquared };
	}

	// Random point on unit sphere surface
	Vec3f unitSphere()
	{
		const auto disc = insideDisc();
		return projectOntoSphereSurface( disc, 1.0f );
	}

	// Random point inside unit sphere
	Vec3f insideSphere()
	{
		const auto disc = insideDisc();

		// Generate uniform radius for volume distribution (cube root for uniform 3D density)
		const float r = math::pow( random(), 1.0f / 3.0f );

		return projectOntoSphereSurface( disc, r );
	}

	// Random point inside unit cube
	Vec3f insideCube()
	{
		return { range( -1.0f, 1.0f ), range( -1.0f, 1.0f ), range( -1.0f, 1.0f ) };
	}

	// Random element from array/container
	template <typename Container>
	auto &choice( Container &container )
	{
		const auto size = container.size();
		const auto index = range( 0, static_cast<int>( size ) - 1 );
		return container[index];
	}

	template <typename Container>
	const auto &choice( const Container &container )
	{
		const auto size = container.size();
		const auto index = range( 0, static_cast<int>( size ) - 1 );
		return container[index];
	}

	// Shuffle array/container using Fisher-Yates algorithm
	template <typename Container>
	void shuffle( Container &container )
	{
		const auto size = container.size();
		for ( size_t i = size - 1; i > 0; --i )
		{
			const size_t j = range( 0, static_cast<int>( i ) );
			std::swap( container[i], container[j] );
		}
	}
};

// Global random instance for convenience
inline Random globalRandom;

// Convenience functions using global random instance
inline float random()
{
	return globalRandom.random();
}
inline float random( const float min, const float max )
{
	return globalRandom.range( min, max );
}
inline int randomInt( const int min, const int max )
{
	return globalRandom.range( min, max );
}
inline bool randomBool( const float probability = 0.5f )
{
	return globalRandom.chance( probability );
}
inline Vec2f randomUnitCircle()
{
	return globalRandom.unitCircle();
}
inline Vec3f randomUnitSphere()
{
	return globalRandom.unitSphere();
}
inline Vec3f randomInsideSphere()
{
	return globalRandom.insideSphere();
}

// Simple noise functions for procedural generation
class SimpleNoise
{
private:
	// Permutation table for noise generation
	static constexpr int kNoiseSize = 256;
	static constexpr int kNoiseMask = kNoiseSize - 1;

	// Pre-computed permutation table (based on Ken Perlin's reference implementation)
	// clang-format off
	static constexpr int permutation[kNoiseSize] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
	// clang-format on

	static float fade( const float t )
	{
		// Fade function: 6t^5 - 15t^4 + 10t^3
		return t * t * t * ( t * ( t * 6.0f - 15.0f ) + 10.0f );
	}

	static float grad( const int hash, const float x, const float y )
	{
		// Convert hash to gradient vector
		const int h = hash & 3;
		const float u = h < 2 ? x : y;
		const float v = h < 2 ? y : x;
		return ( ( h & 1 ) == 0 ? u : -u ) + ( ( h & 2 ) == 0 ? v : -v );
	}

public:
	// 2D Perlin-style noise function
	static float perlinNoise( const float x, const float y )
	{
		// Find unit square containing point
		const int xi = static_cast<int>( math::floor( x ) ) & kNoiseMask;
		const int yi = static_cast<int>( math::floor( y ) ) & kNoiseMask;

		// Find relative position in square
		const float xf = x - math::floor( x );
		const float yf = y - math::floor( y );

		// Compute fade curves
		const float u = fade( xf );
		const float v = fade( yf );

		// Hash coordinates of square corners
		const int aa = permutation[permutation[xi] + yi];
		const int ab = permutation[permutation[xi] + yi + 1];
		const int ba = permutation[permutation[xi + 1] + yi];
		const int bb = permutation[permutation[xi + 1] + yi + 1];

		// Blend the results from the corners
		const float x1 = math::lerp( grad( aa, xf, yf ), grad( ba, xf - 1.0f, yf ), u );
		const float x2 = math::lerp( grad( ab, xf, yf - 1.0f ), grad( bb, xf - 1.0f, yf - 1.0f ), u );

		return math::lerp( x1, x2, v );
	}

	// Fractal noise (multiple octaves of Perlin noise)
	static float fractalNoise( const float x, const float y, const int octaves = 4, const float frequency = 1.0f, const float amplitude = 1.0f, const float persistence = 0.5f )
	{
		float total = 0.0f;
		float maxValue = 0.0f;
		float currentAmplitude = amplitude;
		float currentFrequency = frequency;

		for ( int i = 0; i < octaves; i++ )
		{
			total += perlinNoise( x * currentFrequency, y * currentFrequency ) * currentAmplitude;
			maxValue += currentAmplitude;

			currentAmplitude *= persistence;
			currentFrequency *= 2.0f;
		}

		return total / maxValue; // Normalize to [-1, 1]
	}

	// Turbulence (absolute value of fractal noise)
	static float turbulence( const float x, const float y, const int octaves = 4, const float frequency = 1.0f, const float amplitude = 1.0f, const float persistence = 0.5f )
	{
		float total = 0.0f;
		float maxValue = 0.0f;
		float currentAmplitude = amplitude;
		float currentFrequency = frequency;

		for ( int i = 0; i < octaves; i++ )
		{
			total += math::abs( perlinNoise( x * currentFrequency, y * currentFrequency ) ) * currentAmplitude;
			maxValue += currentAmplitude;

			currentAmplitude *= persistence;
			currentFrequency *= 2.0f;
		}

		return total / maxValue; // Normalize to [0, 1]
	}
};

// Convenience noise functions
inline float perlinNoise( const float x, const float y )
{
	return SimpleNoise::perlinNoise( x, y );
}
inline float fractalNoise( const float x, const float y, const int octaves = 4 )
{
	return SimpleNoise::fractalNoise( x, y, octaves );
}
inline float turbulence( const float x, const float y, const int octaves = 4 )
{
	return SimpleNoise::turbulence( x, y, octaves );
}
} // namespace math

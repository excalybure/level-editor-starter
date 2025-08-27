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
private:
	std::mt19937 m_generator;
	std::uniform_real_distribution<float> m_uniformFloat{ 0.0f, 1.0f };

public:
	// Constructor with optional seed
	Random( unsigned int seed = std::random_device{}() )
		: m_generator( seed )
	{
	}

	// Seed the generator
	void seed( unsigned int newSeed )
	{
		m_generator.seed( newSeed );
	}

	// Get the current seed (approximation - returns a state-based value)
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
	float range( float min, float max )
	{
		return min + random() * ( max - min );
	}

	// Random integer in range [min, max] (inclusive)
	int range( int min, int max )
	{
		std::uniform_int_distribution<int> dist( min, max );
		return dist( m_generator );
	}

	// Random boolean with given probability of true
	bool chance( float probability = 0.5f )
	{
		return random() < probability;
	}

	// Random point on unit circle
	Vec2<float> unitCircle()
	{
		float angle = range( 0.0f, 2.0f * math::pi<float> );
		return { math::cos( angle ), math::sin( angle ) };
	}

	// Random point on unit sphere surface
	Vec3<float> unitSphere()
	{
		// Use rejection sampling for uniform distribution
		float z = range( -1.0f, 1.0f );
		float angle = range( 0.0f, 2.0f * math::pi<float> );
		float radius = math::sqrt( 1.0f - z * z );
		return { radius * math::cos( angle ), radius * math::sin( angle ), z };
	}

	// Random point inside unit sphere
	Vec3<float> insideSphere()
	{
		// Generate random point on sphere and scale by random radius
		Vec3<float> point = unitSphere();
		float radius = math::pow( random(), 1.0f / 3.0f ); // Cube root for uniform volume distribution
		return point * radius;
	}

	// Random point inside unit cube
	Vec3<float> insideCube()
	{
		return { range( -1.0f, 1.0f ), range( -1.0f, 1.0f ), range( -1.0f, 1.0f ) };
	}

	// Random element from array/container
	template <typename Container>
	auto &choice( Container &container )
	{
		auto size = container.size();
		auto index = range( 0, static_cast<int>( size ) - 1 );
		return container[index];
	}

	template <typename Container>
	const auto &choice( const Container &container )
	{
		auto size = container.size();
		auto index = range( 0, static_cast<int>( size ) - 1 );
		return container[index];
	}

	// Shuffle array/container using Fisher-Yates algorithm
	template <typename Container>
	void shuffle( Container &container )
	{
		auto size = container.size();
		for ( size_t i = size - 1; i > 0; --i )
		{
			size_t j = range( 0, static_cast<int>( i ) );
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
inline float random( float min, float max )
{
	return globalRandom.range( min, max );
}
inline int randomInt( int min, int max )
{
	return globalRandom.range( min, max );
}
inline bool randomBool( float probability = 0.5f )
{
	return globalRandom.chance( probability );
}
inline Vec2<float> randomUnitCircle()
{
	return globalRandom.unitCircle();
}
inline Vec3<float> randomUnitSphere()
{
	return globalRandom.unitSphere();
}
inline Vec3<float> randomInsideSphere()
{
	return globalRandom.insideSphere();
}

// Simple noise functions for procedural generation
class SimpleNoise
{
private:
	// Permutation table for noise generation
	static constexpr int NOISE_SIZE = 256;
	static constexpr int NOISE_MASK = NOISE_SIZE - 1;

	// Pre-computed permutation table (based on Ken Perlin's reference implementation)
	static constexpr int permutation[NOISE_SIZE] = {
		151,
		160,
		137,
		91,
		90,
		15,
		131,
		13,
		201,
		95,
		96,
		53,
		194,
		233,
		7,
		225,
		140,
		36,
		103,
		30,
		69,
		142,
		8,
		99,
		37,
		240,
		21,
		10,
		23,
		190,
		6,
		148,
		247,
		120,
		234,
		75,
		0,
		26,
		197,
		62,
		94,
		252,
		219,
		203,
		117,
		35,
		11,
		32,
		57,
		177,
		33,
		88,
		237,
		149,
		56,
		87,
		174,
		20,
		125,
		136,
		171,
		168,
		68,
		175,
		74,
		165,
		71,
		134,
		139,
		48,
		27,
		166,
		77,
		146,
		158,
		231,
		83,
		111,
		229,
		122,
		60,
		211,
		133,
		230,
		220,
		105,
		92,
		41,
		55,
		46,
		245,
		40,
		244,
		102,
		143,
		54,
		65,
		25,
		63,
		161,
		1,
		216,
		80,
		73,
		209,
		76,
		132,
		187,
		208,
		89,
		18,
		169,
		200,
		196,
		135,
		130,
		116,
		188,
		159,
		86,
		164,
		100,
		109,
		198,
		173,
		186,
		3,
		64,
		52,
		217,
		226,
		250,
		124,
		123,
		5,
		202,
		38,
		147,
		118,
		126,
		255,
		82,
		85,
		212,
		207,
		206,
		59,
		227,
		47,
		16,
		58,
		17,
		182,
		189,
		28,
		42,
		223,
		183,
		170,
		213,
		119,
		248,
		152,
		2,
		44,
		154,
		163,
		70,
		221,
		153,
		101,
		155,
		167,
		43,
		172,
		9,
		129,
		22,
		39,
		253,
		19,
		98,
		108,
		110,
		79,
		113,
		224,
		232,
		178,
		185,
		112,
		104,
		218,
		246,
		97,
		228,
		251,
		34,
		242,
		193,
		238,
		210,
		144,
		12,
		191,
		179,
		162,
		241,
		81,
		51,
		145,
		235,
		249,
		14,
		239,
		107,
		49,
		192,
		214,
		31,
		181,
		199,
		106,
		157,
		184,
		84,
		204,
		176,
		115,
		121,
		50,
		45,
		127,
		4,
		150,
		254,
		138,
		236,
		205,
		93,
		222,
		114,
		67,
		29,
		24,
		72,
		243,
		141,
		128,
		195,
		78,
		66,
		215,
		61,
		156,
		180
	};

	static float fade( float t )
	{
		// Fade function: 6t^5 - 15t^4 + 10t^3
		return t * t * t * ( t * ( t * 6.0f - 15.0f ) + 10.0f );
	}

	static float grad( int hash, float x, float y )
	{
		// Convert hash to gradient vector
		int h = hash & 3;
		float u = h < 2 ? x : y;
		float v = h < 2 ? y : x;
		return ( ( h & 1 ) == 0 ? u : -u ) + ( ( h & 2 ) == 0 ? v : -v );
	}

public:
	// 2D Perlin-style noise function
	static float perlinNoise( float x, float y )
	{
		// Find unit square containing point
		int xi = static_cast<int>( floor( x ) ) & NOISE_MASK;
		int yi = static_cast<int>( floor( y ) ) & NOISE_MASK;

		// Find relative position in square
		float xf = x - floor( x );
		float yf = y - floor( y );

		// Compute fade curves
		float u = fade( xf );
		float v = fade( yf );

		// Hash coordinates of square corners
		int aa = permutation[permutation[xi] + yi];
		int ab = permutation[permutation[xi] + yi + 1];
		int ba = permutation[permutation[xi + 1] + yi];
		int bb = permutation[permutation[xi + 1] + yi + 1];

		// Blend the results from the corners
		float x1 = lerp( grad( aa, xf, yf ), grad( ba, xf - 1.0f, yf ), u );
		float x2 = lerp( grad( ab, xf, yf - 1.0f ), grad( bb, xf - 1.0f, yf - 1.0f ), u );

		return lerp( x1, x2, v );
	}

	// Fractal noise (multiple octaves of Perlin noise)
	static float fractalNoise( float x, float y, int octaves = 4, float frequency = 1.0f, float amplitude = 1.0f, float persistence = 0.5f )
	{
		float total = 0.0f;
		float maxValue = 0.0f;

		for ( int i = 0; i < octaves; i++ )
		{
			total += perlinNoise( x * frequency, y * frequency ) * amplitude;
			maxValue += amplitude;

			amplitude *= persistence;
			frequency *= 2.0f;
		}

		return total / maxValue; // Normalize to [-1, 1]
	}

	// Turbulence (absolute value of fractal noise)
	static float turbulence( float x, float y, int octaves = 4, float frequency = 1.0f, float amplitude = 1.0f, float persistence = 0.5f )
	{
		float total = 0.0f;
		float maxValue = 0.0f;

		for ( int i = 0; i < octaves; i++ )
		{
			total += math::abs( perlinNoise( x * frequency, y * frequency ) ) * amplitude;
			maxValue += amplitude;

			amplitude *= persistence;
			frequency *= 2.0f;
		}

		return total / maxValue; // Normalize to [0, 1]
	}
};

// Convenience noise functions
inline float perlinNoise( float x, float y )
{
	return SimpleNoise::perlinNoise( x, y );
}
inline float fractalNoise( float x, float y, int octaves = 4 )
{
	return SimpleNoise::fractalNoise( x, y, octaves );
}
inline float turbulence( float x, float y, int octaves = 4 )
{
	return SimpleNoise::turbulence( x, y, octaves );
}
} // namespace math

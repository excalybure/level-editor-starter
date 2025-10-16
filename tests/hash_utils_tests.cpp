#include <catch2/catch_test_macros.hpp>
#include "core/hash_utils.h"
#include <string>

TEST_CASE( "hash_combine produces non-zero hash for single value", "[hash_utils][unit]" )
{
	std::size_t hash = 0;
	core::hash_combine( hash, 42 );

	REQUIRE( hash != 0 );
}

TEST_CASE( "hash_combine is deterministic", "[hash_utils][unit]" )
{
	std::size_t hash1 = 0;
	std::size_t hash2 = 0;

	core::hash_combine( hash1, 123 );
	core::hash_combine( hash2, 123 );

	REQUIRE( hash1 == hash2 );
}

TEST_CASE( "hash_combine produces different hashes for different values", "[hash_utils][unit]" )
{
	std::size_t hash1 = 0;
	std::size_t hash2 = 0;

	core::hash_combine( hash1, 123 );
	core::hash_combine( hash2, 456 );

	REQUIRE( hash1 != hash2 );
}

TEST_CASE( "hash_combine order matters", "[hash_utils][unit]" )
{
	std::size_t hash1 = 0;
	std::size_t hash2 = 0;

	core::hash_combine( hash1, 123 );
	core::hash_combine( hash1, 456 );

	core::hash_combine( hash2, 456 );
	core::hash_combine( hash2, 123 );

	REQUIRE( hash1 != hash2 );
}

TEST_CASE( "hash_combine works with strings", "[hash_utils][unit]" )
{
	std::size_t hash = 0;
	core::hash_combine( hash, std::string( "test" ) );

	REQUIRE( hash != 0 );
}

TEST_CASE( "hash_combine variadic template works", "[hash_utils][unit]" )
{
	std::size_t hash1 = 0;
	std::size_t hash2 = 0;

	// Combine values one at a time
	core::hash_combine( hash1, 1 );
	core::hash_combine( hash1, 2 );
	core::hash_combine( hash1, 3 );

	// Combine all at once using variadic template
	core::hash_combine( hash2, 1, 2, 3 );

	REQUIRE( hash1 == hash2 );
}

TEST_CASE( "hash_combine matches boost algorithm", "[hash_utils][unit]" )
{
	// Verify that our implementation matches the expected boost-style pattern
	std::size_t hash = 0;
	const int value = 42;

	core::hash_combine( hash, value );

	// Manually compute expected hash using boost formula
	std::hash<int> hasher;
	std::size_t expected = 0;
	expected ^= hasher( value ) + 0x9e3779b9 + ( expected << 6 ) + ( expected >> 2 );

	REQUIRE( hash == expected );
}

TEST_CASE( "hash_combine with multiple types", "[hash_utils][unit]" )
{
	std::size_t hash = 0;

	core::hash_combine( hash, 42, 3.14f, std::string( "test" ), true );

	REQUIRE( hash != 0 );
}

#include <catch2/catch_test_macros.hpp>

// Test that stb_image.h can be included without errors
#include <stb_image.h>

TEST_CASE( "stb_image header can be included", "[stb][integration]" )
{
	// Simple test to verify the header compiles and basic functions exist
	REQUIRE( true );
}

TEST_CASE( "stb_image version macros are defined", "[stb][integration]" )
{
// Verify that stb_image macros are available
#ifdef STBI_VERSION
	REQUIRE( STBI_VERSION == 1 );
#else
	FAIL( "STBI_VERSION macro not defined" );
#endif
}

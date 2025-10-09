#include <catch2/catch_test_macros.hpp>

#include "core/console.h"
#include <sstream>
#include <iostream>
#include <stdexcept>

TEST_CASE( "Console module functionality", "[console]" )
{
	SECTION( "Console functions compile and don't crash" )
	{
		// These tests just verify the functions exist and can be called
		// We can't easily test the colored output in unit tests

		REQUIRE_NOTHROW( console::info( "Test info message" ) );
		REQUIRE_NOTHROW( console::debug( "Test debug message" ) );
		REQUIRE_NOTHROW( console::warning( "Test warning message" ) );
		REQUIRE_NOTHROW( console::error( "Test error message" ) );

		// Test C-string overloads
		REQUIRE_NOTHROW( console::info( "C-string info" ) );
		REQUIRE_NOTHROW( console::debug( "C-string debug" ) );
		REQUIRE_NOTHROW( console::warning( "C-string warning" ) );
		REQUIRE_NOTHROW( console::error( "C-string error" ) );
	}

	SECTION( "Console functions accept string" )
	{
		std::string testMessage = "Test string message";
		REQUIRE_NOTHROW( console::info( testMessage ) );
		REQUIRE_NOTHROW( console::debug( testMessage ) );
		REQUIRE_NOTHROW( console::warning( testMessage ) );
		REQUIRE_NOTHROW( console::error( testMessage ) );
	}

	SECTION( "Variadic template functions with format" )
	{
		int value = 42;
		float floatValue = 3.14f;
		const char *name = "Test";

		// Test format strings with various argument types
		REQUIRE_NOTHROW( console::info( "Integer value: {}", value ) );
		REQUIRE_NOTHROW( console::debug( "Float value: {:.2f}", floatValue ) );
		REQUIRE_NOTHROW( console::warning( "String value: {}", name ) );
		REQUIRE_NOTHROW( console::error( "Multiple values: {} {} {}", value, floatValue, name ) );

		// Test with no arguments (just format string)
		REQUIRE_NOTHROW( console::info( "No arguments" ) );

		// Test with complex formatting
		REQUIRE_NOTHROW( console::debug( "Hex value: {:#x}, Binary: {:#b}", value, value ) );
	}

	SECTION( "errorAndThrow function throws runtime_error" )
	{
		// Test that errorAndThrow properly throws std::runtime_error
		std::string testMessage = "Test error message";

		REQUIRE_THROWS_AS( console::errorAndThrow( testMessage ), std::runtime_error );

		// Test that the exception contains the correct message
		try
		{
			console::errorAndThrow( "Test exception message" );
			FAIL( "Expected exception was not thrown" );
		}
		catch ( const std::runtime_error &e )
		{
			REQUIRE( std::string( e.what() ) == "Test exception message" );
		}

		// Test C-string overload
		REQUIRE_THROWS_AS( console::errorAndThrow( "C-string error" ), std::runtime_error );

		// Test template overload with formatting
		int errorCode = 404;
		REQUIRE_THROWS_AS( console::errorAndThrow( "Error {}: Resource not found", errorCode ), std::runtime_error );
	}

	// Note: We don't test console::fatal() because it calls std::exit()
	// which would terminate the test runner
}

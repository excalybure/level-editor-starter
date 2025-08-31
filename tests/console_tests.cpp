#include <catch2/catch_test_macros.hpp>

import runtime.console;
#include <sstream>
#include <iostream>

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

	SECTION( "Console functions accept std::string" )
	{
		std::string testMessage = "Test string message";
		REQUIRE_NOTHROW( console::info( testMessage ) );
		REQUIRE_NOTHROW( console::debug( testMessage ) );
		REQUIRE_NOTHROW( console::warning( testMessage ) );
		REQUIRE_NOTHROW( console::error( testMessage ) );
	}

	SECTION( "Variadic template functions with std::format" )
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

	// Note: We don't test console::fatal() because it calls std::exit()
	// which would terminate the test runner
}

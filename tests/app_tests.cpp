#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "runtime/app.h"


TEST_CASE( "App tick approximate timing", "[app]" )
{
	app::App a;
	const auto start = std::chrono::high_resolution_clock::now();
	a.tick();
	const auto end = std::chrono::high_resolution_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
	// Expect roughly 16ms; allow broad window (5..40) to avoid flakiness on CI
	REQUIRE( ms >= 5 );
	REQUIRE( ms <= 40 );
}

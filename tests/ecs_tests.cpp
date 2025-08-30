#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import runtime.ecs;

using namespace ecs;

struct Position
{
	float x{ 0 }, y{ 0 };
};
static_assert( ecs::Component<Position> );

TEST_CASE( "ECS Storage basic create/has/get", "[ecs]" )
{
	Storage<Position> storage;
	REQUIRE( storage.dense.empty() );
	REQUIRE( storage.sparse.empty() );

	// Create default entity
	Entity e0 = storage.create();
	REQUIRE( e0.id == 0u );
	REQUIRE( storage.has( e0 ) );
	REQUIRE( storage.dense.size() == 1 );
	REQUIRE( storage.sparse.size() == 1 );

	// Create with value
	Position p{ 3.5f, -2.0f };
	Entity e1 = storage.create( p );
	REQUIRE( e1.id == 1u );
	REQUIRE( storage.has( e1 ) );
	REQUIRE( storage.dense.size() == 2 );
	REQUIRE( storage.get( e1 ).x == Catch::Approx( 3.5f ) );
	REQUIRE( storage.get( e1 ).y == Catch::Approx( -2.0f ) );

	// Modify component via reference
	auto &posRef = storage.get( e0 );
	posRef.x = 10.f;
	posRef.y = 5.f;
	REQUIRE( storage.get( e0 ).x == Catch::Approx( 10.f ) );
	REQUIRE( storage.get( e0 ).y == Catch::Approx( 5.f ) );

	// has() should be false for non-existent entity id
	Entity invalid{ 100 };
	REQUIRE_FALSE( storage.has( invalid ) );
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
import engine.math;

using Catch::Matchers::WithinRel;

TEST_CASE( "Vec2 basic arithmetic", "[math][vec2]" )
{
	math::Vec2<> a{ 1.0f, 2.0f }, b{ 3.0f, 4.0f };
	a += b;
	REQUIRE( a.x == 4.0f );
	REQUIRE( a.y == 6.0f );
	auto c = a - b; // should get original a
	REQUIRE( c.x == 1.0f );
	REQUIRE( c.y == 2.0f );
	auto d = b * 2.0f;
	REQUIRE( d.x == 6.0f );
	REQUIRE( d.y == 8.0f );
}

TEST_CASE( "Vec3 dot and cross", "[math][vec3]" )
{
	math::Vec3<> x{ 1, 0, 0 }, y{ 0, 1, 0 };
	REQUIRE_THAT( math::dot( x, x ), WithinRel( 1.0f ) );
	auto c = math::cross( x, y );
	REQUIRE_THAT( c.z, WithinRel( 1.0f ) );
}

TEST_CASE( "Vec3 normalize", "[math][vec3]" )
{
	math::Vec3<> v{ 3.0f, 0.0f, 4.0f };
	auto n = math::normalize( v );
	REQUIRE_THAT( math::dot( n, n ), WithinRel( 1.0f ) );
}

TEST_CASE( "Vec4 arithmetic and dot", "[math][vec4]" )
{
	math::Vec4<> a{ 1, 2, 3, 4 }, b{ 2, 3, 4, 5 };
	auto s = a + b;
	REQUIRE( s.x == 3 );
	REQUIRE( s.y == 5 );
	REQUIRE( s.z == 7 );
	REQUIRE( s.w == 9 );
	auto d = b - a;
	REQUIRE( d.x == 1 );
	REQUIRE( d.y == 1 );
	REQUIRE( d.z == 1 );
	REQUIRE( d.w == 1 );
	REQUIRE( math::dot( a, a ) == 1 + 4 + 9 + 16 );
}

TEST_CASE( "Normalize zero vector returns zero", "[math][normalize]" )
{
	math::Vec2<> z{};
	auto n = math::normalize( z );
	REQUIRE( n.x == 0.0f );
	REQUIRE( n.y == 0.0f );
}

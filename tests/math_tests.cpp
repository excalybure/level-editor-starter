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
	const auto c = a - b; // should get original a
	REQUIRE( c.x == 1.0f );
	REQUIRE( c.y == 2.0f );
	const auto d = b * 2.0f;
	REQUIRE( d.x == 6.0f );
	REQUIRE( d.y == 8.0f );
}

TEST_CASE( "Vec3 dot and cross", "[math][vec3]" )
{
	const math::Vec3<> x{ 1, 0, 0 }, y{ 0, 1, 0 };
	REQUIRE_THAT( math::dot( x, x ), WithinRel( 1.0f ) );
	const auto c = math::cross( x, y );
	REQUIRE_THAT( c.z, WithinRel( 1.0f ) );
}

TEST_CASE( "Vec3 normalize", "[math][vec3]" )
{
	const math::Vec3<> v{ 3.0f, 0.0f, 4.0f };
	const auto n = math::normalize( v );
	REQUIRE_THAT( math::dot( n, n ), WithinRel( 1.0f ) );
}

TEST_CASE( "Vec4 arithmetic and dot", "[math][vec4]" )
{
	const math::Vec4<> a{ 1, 2, 3, 4 }, b{ 2, 3, 4, 5 };
	const auto s = a + b;
	REQUIRE( s.x == 3 );
	REQUIRE( s.y == 5 );
	REQUIRE( s.z == 7 );
	REQUIRE( s.w == 9 );
	const auto d = b - a;
	REQUIRE( d.x == 1 );
	REQUIRE( d.y == 1 );
	REQUIRE( d.z == 1 );
	REQUIRE( d.w == 1 );
	REQUIRE( math::dot( a, a ) == 1 + 4 + 9 + 16 );
}

TEST_CASE( "Normalize zero vector returns zero", "[math][normalize]" )
{
	const math::Vec2<> z{};
	const auto n = math::normalize( z );
	REQUIRE( n.x == 0.0f );
	REQUIRE( n.y == 0.0f );
}

TEST_CASE( "Distance and distanceSquared", "[math][distance]" )
{
	const math::Vec3<> a{ 0, 0, 0 }, b{ 3, 4, 0 };
	REQUIRE( math::distanceSquared( a, b ) == 25.0f );
	REQUIRE_THAT( static_cast<float>( math::distance( a, b ) ), WithinRel( 5.0f ) );
}

TEST_CASE( "Lerp", "[math][lerp]" )
{
	const math::Vec2<> a{ 0, 0 }, b{ 10, 20 };
	const auto mid = math::lerp( a, b, 0.5f );
	REQUIRE( mid.x == 5.0f );
	REQUIRE( mid.y == 10.0f );
}

TEST_CASE( "Reflect", "[math][reflect]" )
{
	const math::Vec2<> I{ 1, -1 };
	math::Vec2<> N{ 0, 1 }; // reflect off horizontal surface
	N = math::normalize( N );
	const auto R = math::reflect( I, N );
	REQUIRE( R.x == 1 );
	REQUIRE( R.y == 1 );
}

TEST_CASE( "Project", "[math][project]" )
{
	const math::Vec3<> A{ 2, 2, 0 }, B{ 1, 0, 0 };
	const auto P = math::project( A, B );
	REQUIRE( P.x == 2 );
	REQUIRE( P.y == 0 );
}

TEST_CASE( "Min/Max and nearEqual", "[math][minmax]" )
{
	const math::Vec4<> a{ 1, 5, -2, 4 }, b{ 2, 3, 0, 5 };
	auto mn = math::min( a, b );
	auto mx = math::max( a, b );
	REQUIRE( mn.x == 1 );
	REQUIRE( mn.y == 3 );
	REQUIRE( mn.z == -2 );
	REQUIRE( mn.w == 4 );
	REQUIRE( mx.x == 2 );
	REQUIRE( mx.y == 5 );
	REQUIRE( mx.z == 0 );
	REQUIRE( mx.w == 5 );
	const math::Vec4<> c = a + math::Vec4<>{ 0.000001f, 0.0f, 0.0f, 0.0f };
	REQUIRE( math::nearEqual( a, c ) );
}

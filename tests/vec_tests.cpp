#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
#include "engine/math/math.h"
#include "engine/math/vec.h"

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

TEST_CASE( "Clamp and saturate", "[math][clamp]" )
{
	const math::Vec3<> v{ -1.0f, 0.5f, 2.0f };
	const auto cl = math::clamp( v, 0.0f, 1.0f );
	REQUIRE( cl.x == 0.0f );
	REQUIRE( cl.y == 0.5f );
	REQUIRE( cl.z == 1.0f );
	auto sat = math::saturate( v );
	REQUIRE( sat.x == 0.0f );
	REQUIRE( sat.y == 0.5f );
	REQUIRE( sat.z == 1.0f );
}

TEST_CASE( "Angle between vectors", "[math][angle]" )
{
	const math::Vec3<> x{ 1, 0, 0 }, y{ 0, 1, 0 };
	const float a = math::angle( x, y );
	REQUIRE_THAT( a, WithinRel( static_cast<float>( std::numbers::pi / 2.0 ), 1e-5f ) );
}

TEST_CASE( "Slerp basics", "[math][slerp]" )
{
	const math::Vec3<> x{ 1, 0, 0 }, y{ 0, 1, 0 };
	const auto mid = math::slerp( x, y, 0.5f );
	// Midpoint direction should be roughly normalized (0.707,0.707,0)
	REQUIRE_THAT( math::dot( math::normalize( mid ), math::Vec3<>{ std::numbers::sqrt2_v<float> / 2.0f, std::numbers::sqrt2_v<float> / 2.0f, 0.0f } ), WithinRel( 1.0f, 1e-4f ) );
}

TEST_CASE( "Slerp varying magnitudes", "[math][slerp]" )
{
	// a length 2, b length 4, 90 deg apart.
	const math::Vec3<> a{ 2, 0, 0 }; // |a| = 2
	const math::Vec3<> b{ 0, 4, 0 }; // |b| = 4
	const float t = 0.25f;			 // quarter of the way
	const auto r = math::slerp( a, b, t );
	// Expected blended magnitude = 2*(1-t)+4*t = 2.5
	const auto mag = math::length( r );
	REQUIRE_THAT( static_cast<float>( mag ), WithinRel( 2.5f, 1e-4f ) );
	// Direction should be rotated 22.5 degrees from a toward b (pi/8)
	const auto dir = math::normalize( r );
	const float expectedAngle = static_cast<float>( std::numbers::pi / 8.0 );
	// Compute angle between dir and x-axis (1,0,0)
	const float ang = math::angle( dir, math::Vec3<>{ 1, 0, 0 } );
	REQUIRE_THAT( ang, WithinRel( expectedAngle, 1e-3f ) );
}

TEST_CASE( "Vec3 swizzle accessors", "[math][vec3][swizzle]" )
{
	const math::Vec3<> v{ 1.0f, 2.0f, 3.0f };

	SECTION( "2D swizzles" )
	{
		auto xy = v.xy();
		REQUIRE( xy.x == 1.0f );
		REQUIRE( xy.y == 2.0f );

		auto xz = v.xz();
		REQUIRE( xz.x == 1.0f );
		REQUIRE( xz.y == 3.0f );

		auto yz = v.yz();
		REQUIRE( yz.x == 2.0f );
		REQUIRE( yz.y == 3.0f );
	}
}

TEST_CASE( "Vec4 swizzle accessors", "[math][vec4][swizzle]" )
{
	const math::Vec4<> v{ 1.0f, 2.0f, 3.0f, 4.0f };

	SECTION( "2D swizzles" )
	{
		auto xy = v.xy();
		REQUIRE( xy.x == 1.0f );
		REQUIRE( xy.y == 2.0f );

		auto xz = v.xz();
		REQUIRE( xz.x == 1.0f );
		REQUIRE( xz.y == 3.0f );

		auto xw = v.xw();
		REQUIRE( xw.x == 1.0f );
		REQUIRE( xw.y == 4.0f );

		auto yz = v.yz();
		REQUIRE( yz.x == 2.0f );
		REQUIRE( yz.y == 3.0f );

		auto yw = v.yw();
		REQUIRE( yw.x == 2.0f );
		REQUIRE( yw.y == 4.0f );

		auto zw = v.zw();
		REQUIRE( zw.x == 3.0f );
		REQUIRE( zw.y == 4.0f );
	}

	SECTION( "3D swizzles" )
	{
		auto xyz = v.xyz();
		REQUIRE( xyz.x == 1.0f );
		REQUIRE( xyz.y == 2.0f );
		REQUIRE( xyz.z == 3.0f );

		auto xzw = v.xzw();
		REQUIRE( xzw.x == 1.0f );
		REQUIRE( xzw.y == 3.0f );
		REQUIRE( xzw.z == 4.0f );

		auto yzw = v.yzw();
		REQUIRE( yzw.x == 2.0f );
		REQUIRE( yzw.y == 3.0f );
		REQUIRE( yzw.z == 4.0f );

		auto xyw = v.xyw();
		REQUIRE( xyw.x == 1.0f );
		REQUIRE( xyw.y == 2.0f );
		REQUIRE( xyw.z == 4.0f );
	}
}

TEST_CASE( "Vec swizzle type consistency", "[math][vec][swizzle][types]" )
{
	SECTION( "Vec3 swizzles return correct types" )
	{
		const math::Vec3f vf{ 1.0f, 2.0f, 3.0f };
		const math::Vec3d vd{ 1.0, 2.0, 3.0 };
		const math::Vec3i vi{ 1, 2, 3 };

		// Test that swizzles preserve the template type
		static_assert( std::is_same_v<decltype( vf.xy() ), math::Vec2f> );
		static_assert( std::is_same_v<decltype( vd.xz() ), math::Vec2d> );
		static_assert( std::is_same_v<decltype( vi.yz() ), math::Vec2i> );
	}

	SECTION( "Vec4 swizzles return correct types" )
	{
		const math::Vec4f vf{ 1.0f, 2.0f, 3.0f, 4.0f };
		const math::Vec4d vd{ 1.0, 2.0, 3.0, 4.0 };
		const math::Vec4i vi{ 1, 2, 3, 4 };

		// Test 2D swizzles
		static_assert( std::is_same_v<decltype( vf.xy() ), math::Vec2f> );
		static_assert( std::is_same_v<decltype( vd.zw() ), math::Vec2d> );
		static_assert( std::is_same_v<decltype( vi.yw() ), math::Vec2i> );

		// Test 3D swizzles
		static_assert( std::is_same_v<decltype( vf.xyz() ), math::Vec3f> );
		static_assert( std::is_same_v<decltype( vd.xzw() ), math::Vec3d> );
		static_assert( std::is_same_v<decltype( vi.yzw() ), math::Vec3i> );
	}
}

TEST_CASE( "Vec swizzles with different numeric types", "[math][vec][swizzle][numeric]" )
{
	SECTION( "Integer vectors" )
	{
		const math::Vec3i v3i{ 10, 20, 30 };
		const math::Vec4i v4i{ 10, 20, 30, 40 };

		auto xy = v3i.xy();
		REQUIRE( xy.x == 10 );
		REQUIRE( xy.y == 20 );

		auto xzw = v4i.xzw();
		REQUIRE( xzw.x == 10 );
		REQUIRE( xzw.y == 30 );
		REQUIRE( xzw.z == 40 );
	}

	SECTION( "Double vectors" )
	{
		const math::Vec3d v3d{ 1.1, 2.2, 3.3 };
		const math::Vec4d v4d{ 1.1, 2.2, 3.3, 4.4 };

		auto xz = v3d.xz();
		REQUIRE( xz.x == 1.1 );
		REQUIRE( xz.y == 3.3 );

		auto yzw = v4d.yzw();
		REQUIRE( yzw.x == 2.2 );
		REQUIRE( yzw.y == 3.3 );
		REQUIRE( yzw.z == 4.4 );
	}
}

TEST_CASE( "Vec swizzles are constexpr", "[math][vec][swizzle][constexpr]" )
{
	SECTION( "Compile-time Vec3 swizzles" )
	{
		constexpr math::Vec3<float> v{ 1.0f, 2.0f, 3.0f };

		constexpr auto xy = v.xy();
		static_assert( xy.x == 1.0f );
		static_assert( xy.y == 2.0f );

		constexpr auto xz = v.xz();
		static_assert( xz.x == 1.0f );
		static_assert( xz.y == 3.0f );

		constexpr auto yz = v.yz();
		static_assert( yz.x == 2.0f );
		static_assert( yz.y == 3.0f );
	}

	SECTION( "Compile-time Vec4 swizzles" )
	{
		constexpr math::Vec4<float> v{ 1.0f, 2.0f, 3.0f, 4.0f };

		// Test 2D swizzles
		constexpr auto xy = v.xy();
		static_assert( xy.x == 1.0f );
		static_assert( xy.y == 2.0f );

		constexpr auto zw = v.zw();
		static_assert( zw.x == 3.0f );
		static_assert( zw.y == 4.0f );

		// Test 3D swizzles
		constexpr auto xyz = v.xyz();
		static_assert( xyz.x == 1.0f );
		static_assert( xyz.y == 2.0f );
		static_assert( xyz.z == 3.0f );

		constexpr auto xzw = v.xzw();
		static_assert( xzw.x == 1.0f );
		static_assert( xzw.y == 3.0f );
		static_assert( xzw.z == 4.0f );

		constexpr auto yzw = v.yzw();
		static_assert( yzw.x == 2.0f );
		static_assert( yzw.y == 3.0f );
		static_assert( yzw.z == 4.0f );

		constexpr auto xyw = v.xyw();
		static_assert( xyw.x == 1.0f );
		static_assert( xyw.y == 2.0f );
		static_assert( xyw.z == 4.0f );
	}
}

TEST_CASE( "Vec3 angle conversion (radians/degrees)", "[math][vec3][angles]" )
{
	// Test Vec3 angle conversion functions
	SECTION( "radians to degrees conversion" )
	{
		const math::Vec3f radiansVec{ math::pi<float>, math::pi<float> / 2.0f, math::pi<float> / 4.0f };
		const math::Vec3f degreesVec = math::degrees( radiansVec );

		REQUIRE_THAT( degreesVec.x, WithinRel( 180.0f, 0.01f ) );
		REQUIRE_THAT( degreesVec.y, WithinRel( 90.0f, 0.01f ) );
		REQUIRE_THAT( degreesVec.z, WithinRel( 45.0f, 0.01f ) );
	}

	SECTION( "degrees to radians conversion" )
	{
		const math::Vec3f degreesVec{ 180.0f, 90.0f, 45.0f };
		const math::Vec3f radiansVec = math::radians( degreesVec );

		REQUIRE_THAT( radiansVec.x, WithinRel( math::pi<float>, 0.01f ) );
		REQUIRE_THAT( radiansVec.y, WithinRel( math::pi<float> / 2.0f, 0.01f ) );
		REQUIRE_THAT( radiansVec.z, WithinRel( math::pi<float> / 4.0f, 0.01f ) );
	}

	SECTION( "round-trip conversion" )
	{
		const math::Vec3f original{ 1.2f, 2.3f, 3.4f };
		const math::Vec3f converted = math::radians( math::degrees( original ) );

		REQUIRE_THAT( converted.x, WithinRel( original.x, 0.0001f ) );
		REQUIRE_THAT( converted.y, WithinRel( original.y, 0.0001f ) );
		REQUIRE_THAT( converted.z, WithinRel( original.z, 0.0001f ) );
	}
}

TEST_CASE( "Vec2 angle conversion (radians/degrees)", "[math][vec2][angles]" )
{
	// Test Vec2 angle conversion functions
	const math::Vec2f radiansVec{ math::pi<float>, math::pi<float> / 2.0f };
	const math::Vec2f degreesVec = math::degrees( radiansVec );

	REQUIRE_THAT( degreesVec.x, WithinRel( 180.0f, 0.01f ) );
	REQUIRE_THAT( degreesVec.y, WithinRel( 90.0f, 0.01f ) );

	const math::Vec2f backToRadians = math::radians( degreesVec );
	REQUIRE_THAT( backToRadians.x, WithinRel( math::pi<float>, 0.001f ) );
	REQUIRE_THAT( backToRadians.y, WithinRel( math::pi<float> / 2.0f, 0.001f ) );
}

TEST_CASE( "Vec4 angle conversion (radians/degrees)", "[math][vec4][angles]" )
{
	// Test Vec4 angle conversion functions
	const math::Vec4f radiansVec{ math::pi<float>, math::pi<float> / 2.0f, math::pi<float> / 4.0f, math::pi<float> / 6.0f };
	const math::Vec4f degreesVec = math::degrees( radiansVec );

	REQUIRE_THAT( degreesVec.x, WithinRel( 180.0f, 0.01f ) );
	REQUIRE_THAT( degreesVec.y, WithinRel( 90.0f, 0.01f ) );
	REQUIRE_THAT( degreesVec.z, WithinRel( 45.0f, 0.01f ) );
	REQUIRE_THAT( degreesVec.w, WithinRel( 30.0f, 0.01f ) );

	const math::Vec4f backToRadians = math::radians( degreesVec );
	REQUIRE_THAT( backToRadians.x, WithinRel( math::pi<float>, 0.001f ) );
	REQUIRE_THAT( backToRadians.y, WithinRel( math::pi<float> / 2.0f, 0.001f ) );
	REQUIRE_THAT( backToRadians.z, WithinRel( math::pi<float> / 4.0f, 0.001f ) );
	REQUIRE_THAT( backToRadians.w, WithinRel( math::pi<float> / 6.0f, 0.001f ) );
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import engine.math;
import engine.quat;
import std;

using namespace math;

TEST_CASE( "Quaternion basic construction and properties", "[quaternion]" )
{
	SECTION( "Default constructor creates identity quaternion" )
	{
		const Quat<float> q;
		REQUIRE( q.w == 1.0f );
		REQUIRE( q.x == 0.0f );
		REQUIRE( q.y == 0.0f );
		REQUIRE( q.z == 0.0f );
	}

	SECTION( "Component constructor" )
	{
		const Quat<float> q{ 0.5f, 0.5f, 0.5f, 0.5f };
		REQUIRE( q.w == 0.5f );
		REQUIRE( q.x == 0.5f );
		REQUIRE( q.y == 0.5f );
		REQUIRE( q.z == 0.5f );
	}

	SECTION( "Magnitude calculation" )
	{
		const Quat<float> q{ 1.0f, 2.0f, 3.0f, 4.0f };
		REQUIRE( q.magnitudeSquared() == Catch::Approx( 30.0f ) );
		REQUIRE( q.magnitude() == Catch::Approx( std::sqrt( 30.0f ) ) );
	}

	SECTION( "Normalization" )
	{
		Quat<float> q{ 1.0f, 2.0f, 3.0f, 4.0f };
		Quat<float> normalized = q.normalized();
		REQUIRE( normalized.magnitude() == Catch::Approx( 1.0f ) );

		q.normalize();
		REQUIRE( q.magnitude() == Catch::Approx( 1.0f ) );
	}
}

TEST_CASE( "Quaternion arithmetic operations", "[quaternion]" )
{
	const Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
	const Quat<float> q2{ 0.5f, 1.0f, 1.5f, 2.0f };

	SECTION( "Addition" )
	{
		const Quat<float> result = q1 + q2;
		REQUIRE( result.w == 1.5f );
		REQUIRE( result.x == 3.0f );
		REQUIRE( result.y == 4.5f );
		REQUIRE( result.z == 6.0f );
	}

	SECTION( "Subtraction" )
	{
		const Quat<float> result = q1 - q2;
		REQUIRE( result.w == 0.5f );
		REQUIRE( result.x == 1.0f );
		REQUIRE( result.y == 1.5f );
		REQUIRE( result.z == 2.0f );
	}

	SECTION( "Scalar multiplication" )
	{
		const Quat<float> result = q1 * 2.0f;
		REQUIRE( result.w == 2.0f );
		REQUIRE( result.x == 4.0f );
		REQUIRE( result.y == 6.0f );
		REQUIRE( result.z == 8.0f );

		// Commutative property
		const Quat<float> result2 = 2.0f * q1;
		REQUIRE( result == result2 );
	}

	SECTION( "Scalar division" )
	{
		const Quat<float> result = q1 / 2.0f;
		REQUIRE( result.w == 0.5f );
		REQUIRE( result.x == 1.0f );
		REQUIRE( result.y == 1.5f );
		REQUIRE( result.z == 2.0f );
	}

	SECTION( "Quaternion multiplication" )
	{
		const Quat<float> i{ 0.0f, 1.0f, 0.0f, 0.0f };
		const Quat<float> j{ 0.0f, 0.0f, 1.0f, 0.0f };
		const Quat<float> k{ 0.0f, 0.0f, 0.0f, 1.0f };

		// Test quaternion multiplication properties: i*j = k, j*k = i, k*i = j
		const Quat<float> ij = i * j;
		REQUIRE( ij.w == Catch::Approx( k.w ) );
		REQUIRE( ij.x == Catch::Approx( k.x ) );
		REQUIRE( ij.y == Catch::Approx( k.y ) );
		REQUIRE( ij.z == Catch::Approx( k.z ) );
	}

	SECTION( "Dot product" )
	{
		const float dot = q1.dot( q2 );
		REQUIRE( dot == Catch::Approx( 15.0f ) ); // 1*0.5 + 2*1 + 3*1.5 + 4*2 = 0.5 + 2 + 4.5 + 8
	}

	SECTION( "Conjugate" )
	{
		const Quat<float> conj = q1.conjugate();
		REQUIRE( conj.w == q1.w );
		REQUIRE( conj.x == -q1.x );
		REQUIRE( conj.y == -q1.y );
		REQUIRE( conj.z == -q1.z );
	}

	SECTION( "Inverse" )
	{
		const Quat<float> identity = quatIdentity<float>();
		const Quat<float> q = q1.normalized(); // Use normalized quaternion for inverse
		const Quat<float> inv = q.inverse();
		const Quat<float> result = q * inv;

		REQUIRE( result.w == Catch::Approx( identity.w ).margin( 0.001f ) );
		REQUIRE( result.x == Catch::Approx( identity.x ).margin( 0.001f ) );
		REQUIRE( result.y == Catch::Approx( identity.y ).margin( 0.001f ) );
		REQUIRE( result.z == Catch::Approx( identity.z ).margin( 0.001f ) );
	}
}

TEST_CASE( "Quaternion rotation operations", "[quaternion]" )
{
	SECTION( "Axis-angle construction" )
	{
		const Vec3<float> axis{ 0.0f, 0.0f, 1.0f }; // Z-axis
		const float angle = radians( 90.0f );
		const Quat<float> q{ axis, angle };

		// For 90 degree rotation around Z-axis: w = cos(45°), z = sin(45°)
		const float expected = std::cos( radians( 45.0f ) );
		REQUIRE( q.w == Catch::Approx( expected ) );
		REQUIRE( q.x == Catch::Approx( 0.0f ) );
		REQUIRE( q.y == Catch::Approx( 0.0f ) );
		REQUIRE( q.z == Catch::Approx( expected ) );
	}

	SECTION( "Vector rotation" )
	{
		// 90 degree rotation around Z-axis
		const Vec3<float> axis{ 0.0f, 0.0f, 1.0f };
		const float angle = radians( 90.0f );
		const Quat<float> q{ axis, angle };

		const Vec3<float> v{ 1.0f, 0.0f, 0.0f }; // X-axis vector
		const Vec3<float> rotated = q.rotate( v );

		// After 90° rotation around Z, X should become Y
		REQUIRE( rotated.x == Catch::Approx( 0.0f ).margin( 0.001f ) );
		REQUIRE( rotated.y == Catch::Approx( 1.0f ).margin( 0.001f ) );
		REQUIRE( rotated.z == Catch::Approx( 0.0f ).margin( 0.001f ) );
	}

	SECTION( "Axis-angle conversion" )
	{
		Vec3<float> originalAxis{ 1.0f, 2.0f, 3.0f };
		originalAxis = normalize( originalAxis );
		float originalAngle = radians( 60.0f );

		Quat<float> q{ originalAxis, originalAngle };
		auto [convertedAxis, convertedAngle] = q.toAxisAngle();

		REQUIRE( convertedAxis.x == Catch::Approx( originalAxis.x ).margin( 0.001f ) );
		REQUIRE( convertedAxis.y == Catch::Approx( originalAxis.y ).margin( 0.001f ) );
		REQUIRE( convertedAxis.z == Catch::Approx( originalAxis.z ).margin( 0.001f ) );
		REQUIRE( convertedAngle == Catch::Approx( originalAngle ).margin( 0.001f ) );
	}
}

TEST_CASE( "Quaternion Euler angle conversions", "[quaternion]" )
{
	SECTION( "Euler angles construction and conversion" )
	{
		const float yaw = radians( 45.0f );
		const float pitch = radians( 30.0f );
		const float roll = radians( 60.0f );

		const Quat<float> q{ yaw, pitch, roll };
		const Vec3<float> converted = q.toEulerAngles();

		REQUIRE( converted.z == Catch::Approx( yaw ).margin( 0.001f ) );   // yaw
		REQUIRE( converted.y == Catch::Approx( pitch ).margin( 0.001f ) ); // pitch
		REQUIRE( converted.x == Catch::Approx( roll ).margin( 0.001f ) );  // roll
	}
}

TEST_CASE( "Quaternion interpolation", "[quaternion]" )
{
	SECTION( "SLERP interpolation" )
	{
		const Quat<float> q1 = quatIdentity<float>();
		const Vec3<float> axis{ 0.0f, 0.0f, 1.0f };
		const Quat<float> q2{ axis, radians( 90.0f ) };

		// Interpolate halfway
		const Quat<float> half = q1.slerp( q2, 0.5f );

		// The result should be approximately a 45-degree rotation
		const float expectedW = std::cos( radians( 22.5f ) );
		const float expectedZ = std::sin( radians( 22.5f ) );
		REQUIRE( half.w == Catch::Approx( expectedW ).margin( 0.001f ) );
		REQUIRE( half.z == Catch::Approx( expectedZ ).margin( 0.001f ) );

		// Test endpoints
		const Quat<float> start = q1.slerp( q2, 0.0f );
		const Quat<float> end = q1.slerp( q2, 1.0f );

		REQUIRE( start.w == Catch::Approx( q1.w ).margin( 0.001f ) );
		REQUIRE( end.w == Catch::Approx( q2.w ).margin( 0.001f ) );
		REQUIRE( end.z == Catch::Approx( q2.z ).margin( 0.001f ) );
	}
}

TEST_CASE( "Quaternion factory functions", "[quaternion]" )
{
	SECTION( "Identity quaternion factory" )
	{
		const Quat<float> identity = quatIdentity<float>();
		REQUIRE( identity.w == 1.0f );
		REQUIRE( identity.x == 0.0f );
		REQUIRE( identity.y == 0.0f );
		REQUIRE( identity.z == 0.0f );
	}

	SECTION( "Axis-angle factory" )
	{
		const Vec3<float> axis{ 0.0f, 1.0f, 0.0f };
		const float angle = radians( 45.0f );
		const Quat<float> q = quatFromAxisAngle( axis, angle );

		const float expectedW = std::cos( radians( 22.5f ) );
		const float expectedY = std::sin( radians( 22.5f ) );
		REQUIRE( q.w == Catch::Approx( expectedW ) );
		REQUIRE( q.y == Catch::Approx( expectedY ) );
	}

	SECTION( "Euler angles factory" )
	{
		const float yaw = radians( 30.0f );
		const float pitch = radians( 45.0f );
		const float roll = radians( 60.0f );

		const Quat<float> q = quatFromEulerAngles( yaw, pitch, roll );
		const Vec3<float> converted = q.toEulerAngles();

		REQUIRE( converted.z == Catch::Approx( yaw ).margin( 0.001f ) );
		REQUIRE( converted.y == Catch::Approx( pitch ).margin( 0.001f ) );
		REQUIRE( converted.x == Catch::Approx( roll ).margin( 0.001f ) );
	}
}

TEST_CASE( "Quaternion comparison operators", "[quaternion]" )
{
	SECTION( "Equality comparison" )
	{
		const Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
		const Quat<float> q2{ 1.0f, 2.0f, 3.0f, 4.0f };
		const Quat<float> q3{ 1.1f, 2.0f, 3.0f, 4.0f };

		REQUIRE( q1 == q2 );
		REQUIRE( q1 != q3 );
	}
}

TEST_CASE( "Quaternion assignment operators", "[quaternion]" )
{
	const Quat<float> q2{ 0.5f, 1.0f, 1.5f, 2.0f };

	SECTION( "Addition assignment" )
	{
		Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
		q1 += q2;
		REQUIRE( q1.w == 1.5f );
		REQUIRE( q1.x == 3.0f );
		REQUIRE( q1.y == 4.5f );
		REQUIRE( q1.z == 6.0f );
	}

	SECTION( "Subtraction assignment" )
	{
		Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
		q1 -= q2;
		REQUIRE( q1.w == 0.5f );
		REQUIRE( q1.x == 1.0f );
		REQUIRE( q1.y == 1.5f );
		REQUIRE( q1.z == 2.0f );
	}

	SECTION( "Scalar multiplication assignment" )
	{
		Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
		q1 *= 2.0f;
		REQUIRE( q1.w == 2.0f );
		REQUIRE( q1.x == 4.0f );
		REQUIRE( q1.y == 6.0f );
		REQUIRE( q1.z == 8.0f );
	}

	SECTION( "Scalar division assignment" )
	{
		Quat<float> q1{ 1.0f, 2.0f, 3.0f, 4.0f };
		q1 /= 2.0f;
		REQUIRE( q1.w == 0.5f );
		REQUIRE( q1.x == 1.0f );
		REQUIRE( q1.y == 1.5f );
		REQUIRE( q1.z == 2.0f );
	}
}

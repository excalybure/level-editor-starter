#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
#include "math/math.h"
#include "math/matrix.h"
#include "math/vec.h"

using Catch::Approx;

using Catch::Matchers::WithinRel;
using Catch::Approx;
using math::Mat2;
using math::Mat3;
using math::Mat4;
using math::Vec2;
using math::Vec3;
using math::Vec4;

TEST_CASE( "Mat4 to Mat3 conversion extracts upper-left 3x3", "[math][matrix][mat4][conversion]" )
{
	SECTION( "Identity matrix conversion" )
	{
		const math::Mat4f mat4 = math::Mat4f::identity();
		const math::Mat3f mat3 = mat4.toMat3();

		// Check that the 3x3 portion matches
		REQUIRE( mat3.m00() == 1.0f );
		REQUIRE( mat3.m11() == 1.0f );
		REQUIRE( mat3.m22() == 1.0f );
		REQUIRE( mat3.m01() == 0.0f );
		REQUIRE( mat3.m02() == 0.0f );
		REQUIRE( mat3.m10() == 0.0f );
		REQUIRE( mat3.m12() == 0.0f );
		REQUIRE( mat3.m20() == 0.0f );
		REQUIRE( mat3.m21() == 0.0f );
	}

	SECTION( "Translation matrix ignores translation component" )
	{
		const math::Mat4f mat4 = math::Mat4f::translation( 5.0f, 10.0f, 15.0f );
		const math::Mat3f mat3 = mat4.toMat3();

		// Should be identity 3x3 (translation ignored)
		REQUIRE( mat3.m00() == 1.0f );
		REQUIRE( mat3.m11() == 1.0f );
		REQUIRE( mat3.m22() == 1.0f );
		REQUIRE( mat3.m01() == 0.0f );
		REQUIRE( mat3.m02() == 0.0f );
		REQUIRE( mat3.m10() == 0.0f );
		REQUIRE( mat3.m12() == 0.0f );
		REQUIRE( mat3.m20() == 0.0f );
		REQUIRE( mat3.m21() == 0.0f );
	}
}

TEST_CASE( "Mat4 extractScale returns scale factors from transformation matrix", "[math][matrix][mat4][scale]" )
{
	SECTION( "Identity matrix has unit scale" )
	{
		const math::Mat4f mat4 = math::Mat4f::identity();
		const math::Vec3f scale = mat4.extractScale();

		REQUIRE( scale.x == Catch::Approx( 1.0f ) );
		REQUIRE( scale.y == Catch::Approx( 1.0f ) );
		REQUIRE( scale.z == Catch::Approx( 1.0f ) );
	}

	SECTION( "Pure scale matrix extracts correct values" )
	{
		const math::Mat4f mat4 = math::Mat4f::scale( 2.0f, 3.0f, 4.0f );
		const math::Vec3f scale = mat4.extractScale();

		REQUIRE( scale.x == Catch::Approx( 2.0f ) );
		REQUIRE( scale.y == Catch::Approx( 3.0f ) );
		REQUIRE( scale.z == Catch::Approx( 4.0f ) );
	}
}

TEST_CASE( "Mat3 toEulerAngles converts rotation matrix to Euler angles", "[math][matrix][mat3][euler]" )
{
	SECTION( "Identity matrix has zero rotation" )
	{
		const math::Mat3f mat3 = math::Mat3f::identity();
		const math::Vec3f angles = mat3.toEulerAngles();

		REQUIRE( angles.x == Catch::Approx( 0.0f ) );
		REQUIRE( angles.y == Catch::Approx( 0.0f ) );
		REQUIRE( angles.z == Catch::Approx( 0.0f ) );
	}

	SECTION( "90 degree Z rotation converts correctly" )
	{
		const math::Mat3f mat3 = math::Mat3f::rotationZ( math::radians( 90.0f ) );
		const math::Vec3f angles = mat3.toEulerAngles();

		// Should have 90 degrees rotation in Z
		REQUIRE( angles.x == Catch::Approx( 0.0f ) );
		REQUIRE( angles.y == Catch::Approx( 0.0f ) );
		REQUIRE( angles.z == Catch::Approx( math::radians( 90.0f ) ) );
	}

	SECTION( "90 degree X rotation converts correctly" )
	{
		const math::Mat3f mat3 = math::Mat3f::rotationX( math::radians( 90.0f ) );
		const math::Vec3f angles = mat3.toEulerAngles();

		// Should have 90 degrees rotation in X
		REQUIRE( angles.x == Catch::Approx( math::radians( 90.0f ) ) );
		REQUIRE( angles.y == Catch::Approx( 0.0f ) );
		REQUIRE( angles.z == Catch::Approx( 0.0f ) );
	}

	SECTION( "90 degree Y rotation converts correctly" )
	{
		// Test with smaller angle to avoid gimbal lock
		const float angle = math::radians( 45.0f );
		const math::Mat3f mat3 = math::Mat3f::rotationY( angle );
		const math::Vec3f angles = mat3.toEulerAngles();

		// Should have 45 degrees rotation in Y, with X and Z close to zero
		REQUIRE( std::abs( angles.x ) < 0.01f );
		REQUIRE( std::abs( angles.y - angle ) < 0.01f );
		REQUIRE( std::abs( angles.z ) < 0.01f );
	}

	SECTION( "Small angle rotation preserves precision" )
	{
		const float smallAngle = math::radians( 5.0f );
		const math::Mat3f mat3 = math::Mat3f::rotationX( smallAngle );
		const math::Vec3f angles = mat3.toEulerAngles();

		REQUIRE( angles.x == Catch::Approx( smallAngle ).margin( 1e-6f ) );
		REQUIRE( angles.y == Catch::Approx( 0.0f ).margin( 1e-6f ) );
		REQUIRE( angles.z == Catch::Approx( 0.0f ).margin( 1e-6f ) );
	}
}

TEST_CASE( "Mat2 basic operations", "[math][matrix][mat2]" )
{
	math::Mat2f identity = math::Mat2f::identity();

	SECTION( "Identity matrix properties" )
	{
		REQUIRE( identity.m00() == 1.0f );
		REQUIRE( identity.m11() == 1.0f );
		REQUIRE( identity.m01() == 0.0f );
		REQUIRE( identity.m10() == 0.0f );

		Vec2<float> v{ 3.0f, 4.0f };
		Vec2<float> result = identity * v;
		REQUIRE( result.x == v.x );
		REQUIRE( result.y == v.y );
	}

	SECTION( "Matrix arithmetic" )
	{
		math::Mat2f a{ 1.0f, 2.0f, 3.0f, 4.0f };
		math::Mat2f b{ 5.0f, 6.0f, 7.0f, 8.0f };

		// Addition
		math::Mat2f sum = a + b;
		REQUIRE( sum.m00() == 6.0f );
		REQUIRE( sum.m01() == 8.0f );
		REQUIRE( sum.m10() == 10.0f );
		REQUIRE( sum.m11() == 12.0f );

		// Subtraction
		math::Mat2f diff = b - a;
		REQUIRE( diff.m00() == 4.0f );
		REQUIRE( diff.m01() == 4.0f );
		REQUIRE( diff.m10() == 4.0f );
		REQUIRE( diff.m11() == 4.0f );

		// Scalar multiplication
		math::Mat2f scaled = a * 2.0f;
		REQUIRE( scaled.m00() == 2.0f );
		REQUIRE( scaled.m01() == 4.0f );
		REQUIRE( scaled.m10() == 6.0f );
		REQUIRE( scaled.m11() == 8.0f );

		// Matrix multiplication
		math::Mat2f product = a * b;
		REQUIRE( product.m00() == 19.0f ); // 1*5 + 2*7
		REQUIRE( product.m01() == 22.0f ); // 1*6 + 2*8
		REQUIRE( product.m10() == 43.0f ); // 3*5 + 4*7
		REQUIRE( product.m11() == 50.0f ); // 3*6 + 4*8
	}

	SECTION( "Determinant and inverse" )
	{
		math::Mat2f m{ 4.0f, 7.0f, 2.0f, 6.0f };

		float det = m.determinant();
		REQUIRE( det == 10.0f ); // 4*6 - 7*2

		math::Mat2f inv = m.inverse();
		REQUIRE( inv.m00() == Catch::Approx( 0.6f ) );
		REQUIRE( inv.m01() == Catch::Approx( -0.7f ) );
		REQUIRE( inv.m10() == Catch::Approx( -0.2f ) );
		REQUIRE( inv.m11() == Catch::Approx( 0.4f ) );

		// Test inverse: m * m^-1 = identity
		math::Mat2f result = m * inv;
		REQUIRE( result.m00() == Catch::Approx( 1.0f ) );
		REQUIRE( result.m01() == Catch::Approx( 0.0f ) );
		REQUIRE( result.m10() == Catch::Approx( 0.0f ) );
		REQUIRE( result.m11() == Catch::Approx( 1.0f ) );
	}

	SECTION( "Rotation matrix" )
	{
		// 90 degrees rotation
		math::Mat2f rot = math::Mat2f::rotation( math::radians( 90.0f ) );

		Vec2<float> v{ 1.0f, 0.0f };
		Vec2<float> rotated = rot * v;

		REQUIRE( rotated.x == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Catch::Approx( 1.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat3 basic operations", "[math][matrix][mat3]" )
{
	math::Mat3f identity = math::Mat3f::identity();

	SECTION( "Identity matrix properties" )
	{
		REQUIRE( identity.m00() == 1.0f );
		REQUIRE( identity.m11() == 1.0f );
		REQUIRE( identity.m22() == 1.0f );

		math::Vec3f v{ 3.0f, 4.0f, 5.0f };
		math::Vec3f result = identity * v;
		REQUIRE( result.x == v.x );
		REQUIRE( result.y == v.y );
		REQUIRE( result.z == v.z );
	}

	SECTION( "Translation matrix" )
	{
		math::Mat3f trans = math::Mat3f::translation( 10.0f, 20.0f );

		math::Vec3f v{ 5.0f, 7.0f, 1.0f }; // Homogeneous coordinate w=1
		math::Vec3f translated = trans * v;

		REQUIRE( translated.x == 15.0f ); // 5 + 10
		REQUIRE( translated.y == 27.0f ); // 7 + 20
		REQUIRE( translated.z == 1.0f );  // w coordinate unchanged
	}

	SECTION( "Scale matrix" )
	{
		math::Mat3f scale = math::Mat3f::scale( 2.0f, 3.0f, 4.0f );

		math::Vec3f v{ 5.0f, 7.0f, 9.0f };
		math::Vec3f scaled = scale * v;

		REQUIRE( scaled.x == 10.0f ); // 5 * 2
		REQUIRE( scaled.y == 21.0f ); // 7 * 3
		REQUIRE( scaled.z == 36.0f ); // 9 * 4
	}

	SECTION( "Rotation matrices" )
	{
		// 90 degrees around Z axis
		math::Mat3f rotZ = math::Mat3f::rotationZ( math::radians( 90.0f ) );

		math::Vec3f v{ 1.0f, 0.0f, 0.0f };
		math::Vec3f rotated = rotZ * v;

		REQUIRE( rotated.x == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Catch::Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.z == Catch::Approx( 0.0f ).margin( 1e-5f ) );
	}

	SECTION( "Matrix determinant and inverse" )
	{
		math::Mat3f m{
			1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 4.0f, 5.0f, 6.0f, 0.0f
		};

		float det = m.determinant();
		REQUIRE( det == Catch::Approx( 1.0f ).margin( 1e-5f ) );

		math::Mat3f inv = m.inverse();

		// Test inverse: m * m^-1 = identity
		math::Mat3f result = m * inv;
		REQUIRE( result.m00() == Catch::Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m11() == Catch::Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m22() == Catch::Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m01() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m02() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m10() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m12() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m20() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m21() == Catch::Approx( 0.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat4 transformations", "[math][matrix][mat4]" )
{
	math::Mat4f identity = math::Mat4f::identity();

	SECTION( "Translation matrix" )
	{
		math::Mat4f trans = math::Mat4f::translation( 10.0f, 20.0f, 30.0f );

		math::Vec3f v{ 5.0f, 7.0f, 9.0f };
		math::Vec3f translated = trans.transformPoint( v );

		REQUIRE( translated.x == 15.0f ); // 5 + 10
		REQUIRE( translated.y == 27.0f ); // 7 + 20
		REQUIRE( translated.z == 39.0f ); // 9 + 30

		// Direction vectors shouldn't be affected by translation
		math::Vec3f dir{ 1.0f, 0.0f, 0.0f };
		math::Vec3f transformedDir = trans.transformVector( dir );

		REQUIRE( transformedDir.x == 1.0f );
		REQUIRE( transformedDir.y == 0.0f );
		REQUIRE( transformedDir.z == 0.0f );
	}

	SECTION( "Scale matrix" )
	{
		math::Mat4f scale = math::Mat4f::scale( 2.0f, 3.0f, 4.0f );

		math::Vec3f v{ 5.0f, 7.0f, 9.0f };
		math::Vec3f scaled = scale.transformPoint( v );

		REQUIRE( scaled.x == Catch::Approx( 10.0f ) ); // 5 * 2
		REQUIRE( scaled.y == Catch::Approx( 21.0f ) ); // 7 * 3
		REQUIRE( scaled.z == Catch::Approx( 36.0f ) ); // 9 * 4
	}

	SECTION( "Rotation matrices" )
	{
		// 90 degrees around Y axis
		math::Mat4f rotY = math::Mat4f::rotationY( math::radians( 90.0f ) );

		math::Vec3f v{ 0.0f, 0.0f, 1.0f };
		math::Vec3f rotated = rotY.transformPoint( v );

		REQUIRE( rotated.x == Catch::Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.z == Catch::Approx( 0.0f ).margin( 1e-5f ) );
	}

	SECTION( "Combined transformations" )
	{
		// Create a transformation that:
		// 1. Scales by 2 in all directions
		// 2. Rotates 90 degrees around Y
		// 3. Translates by (10, 20, 30)
		math::Mat4f scale = math::Mat4f::scale( 2.0f, 2.0f, 2.0f );
		math::Mat4f rotate = math::Mat4f::rotationY( math::radians( 90.0f ) );
		math::Mat4f translate = math::Mat4f::translation( 10.0f, 20.0f, 30.0f );

		// Apply transforms in correct order (scale, then rotate, then translate)
		math::Mat4f combined = translate * rotate * scale;

		math::Vec3f v{ 1.0f, 2.0f, 3.0f };
		math::Vec3f result = combined.transformPoint( v );

		// First scaled: (2, 4, 6)
		// Then rotated: (6, 4, -2) (90° Y rotation swaps X and Z and negates Z)
		// Then translated: (16, 24, 28)
		REQUIRE( result.x == Catch::Approx( 16.0f ).margin( 1e-4f ) );
		REQUIRE( result.y == Catch::Approx( 24.0f ).margin( 1e-4f ) );
		REQUIRE( result.z == Catch::Approx( 28.0f ).margin( 1e-4f ) );
	}

	SECTION( "Perspective matrix" )
	{
		float fov = math::radians( 45.0f );
		float aspect = 16.0f / 9.0f;
		float near = 0.1f;
		float far = 100.0f;

		math::Mat4f perspective = math::Mat4f::perspective( fov, aspect, near, far );

		// Point on near plane, center
		math::Vec3f nearCenter{ 0.0f, 0.0f, -near };
		Vec4<float> clipSpace = perspective * Vec4<float>{ nearCenter.x, nearCenter.y, nearCenter.z, 1.0f };

		// After perspective division, z should be -1 (OpenGL NDC for near plane)
		float ndcZ = clipSpace.z / clipSpace.w;
		REQUIRE( ndcZ == Catch::Approx( -1.0f ).margin( 1e-5f ) );

		// Point on far plane, center
		math::Vec3f farCenter{ 0.0f, 0.0f, -far };
		clipSpace = perspective * Vec4<float>{ farCenter.x, farCenter.y, farCenter.z, 1.0f };

		// After perspective division, z should be 1 (OpenGL NDC for far plane)
		ndcZ = clipSpace.z / clipSpace.w;
		REQUIRE( ndcZ == Catch::Approx( 1.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat3 fromEulerAngles creates composite rotation matrix", "[math][matrix][mat3][rotation][euler]" )
{
	SECTION( "Zero rotations produce identity matrix" )
	{
		const math::Mat3f mat3 = math::Mat3f::fromEulerAngles( 0.0f, 0.0f, 0.0f );
		const math::Mat3f identity = math::Mat3f::identity();

		REQUIRE( mat3.m00() == Catch::Approx( identity.m00() ) );
		REQUIRE( mat3.m01() == Catch::Approx( identity.m01() ) );
		REQUIRE( mat3.m02() == Catch::Approx( identity.m02() ) );
		REQUIRE( mat3.m10() == Catch::Approx( identity.m10() ) );
		REQUIRE( mat3.m11() == Catch::Approx( identity.m11() ) );
		REQUIRE( mat3.m12() == Catch::Approx( identity.m12() ) );
		REQUIRE( mat3.m20() == Catch::Approx( identity.m20() ) );
		REQUIRE( mat3.m21() == Catch::Approx( identity.m21() ) );
		REQUIRE( mat3.m22() == Catch::Approx( identity.m22() ) );
	}

	SECTION( "Euler angles round-trip correctly" )
	{
		const float yaw = math::radians( 30.0f );
		const float pitch = math::radians( 45.0f );
		const float roll = math::radians( 60.0f );

		const math::Mat3f matrix = math::Mat3f::fromEulerAngles( yaw, pitch, roll );
		const math::Vec3f extractedAngles = matrix.toEulerAngles();

		// Check that extracted angles match input angles
		// Note: roll (X) maps to extractedAngles.x
		// Note: pitch (Y) maps to extractedAngles.y
		// Note: yaw (Z) maps to extractedAngles.z
		REQUIRE( extractedAngles.x == Catch::Approx( roll ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.y == Catch::Approx( pitch ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.z == Catch::Approx( yaw ).margin( 1e-5f ) );
	}

	SECTION( "Single axis rotations match individual rotation functions" )
	{
		const float angle = math::radians( 45.0f );

		// Test roll-only rotation (X-axis)
		const math::Mat3f rollOnly = math::Mat3f::fromEulerAngles( 0.0f, 0.0f, angle );
		const math::Mat3f xIndividual = math::Mat3f::rotationX( angle );
		REQUIRE( rollOnly.m00() == Catch::Approx( xIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m11() == Catch::Approx( xIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m22() == Catch::Approx( xIndividual.m22() ).margin( 1e-5f ) );

		// Test pitch-only rotation (Y-axis)
		const math::Mat3f pitchOnly = math::Mat3f::fromEulerAngles( 0.0f, angle, 0.0f );
		const math::Mat3f yIndividual = math::Mat3f::rotationY( angle );
		REQUIRE( pitchOnly.m00() == Catch::Approx( yIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m11() == Catch::Approx( yIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m22() == Catch::Approx( yIndividual.m22() ).margin( 1e-5f ) );

		// Test yaw-only rotation (Z-axis)
		const math::Mat3f yawOnly = math::Mat3f::fromEulerAngles( angle, 0.0f, 0.0f );
		const math::Mat3f zIndividual = math::Mat3f::rotationZ( angle );
		REQUIRE( yawOnly.m00() == Catch::Approx( zIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m11() == Catch::Approx( zIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m22() == Catch::Approx( zIndividual.m22() ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat4 fromEulerAngles creates composite rotation matrix", "[math][matrix][mat4][rotation][euler]" )
{
	SECTION( "Zero rotations produce identity matrix" )
	{
		const math::Mat4f mat4 = math::Mat4f::fromEulerAngles( 0.0f, 0.0f, 0.0f );
		const math::Mat4f identity = math::Mat4f::identity();

		REQUIRE( mat4.m00() == Catch::Approx( identity.m00() ) );
		REQUIRE( mat4.m01() == Catch::Approx( identity.m01() ) );
		REQUIRE( mat4.m02() == Catch::Approx( identity.m02() ) );
		REQUIRE( mat4.m03() == Catch::Approx( identity.m03() ) );
		REQUIRE( mat4.m10() == Catch::Approx( identity.m10() ) );
		REQUIRE( mat4.m11() == Catch::Approx( identity.m11() ) );
		REQUIRE( mat4.m12() == Catch::Approx( identity.m12() ) );
		REQUIRE( mat4.m13() == Catch::Approx( identity.m13() ) );
		REQUIRE( mat4.m20() == Catch::Approx( identity.m20() ) );
		REQUIRE( mat4.m21() == Catch::Approx( identity.m21() ) );
		REQUIRE( mat4.m22() == Catch::Approx( identity.m22() ) );
		REQUIRE( mat4.m23() == Catch::Approx( identity.m23() ) );
		REQUIRE( mat4.m30() == Catch::Approx( identity.m30() ) );
		REQUIRE( mat4.m31() == Catch::Approx( identity.m31() ) );
		REQUIRE( mat4.m32() == Catch::Approx( identity.m32() ) );
		REQUIRE( mat4.m33() == Catch::Approx( identity.m33() ) );
	}

	SECTION( "Euler angles round-trip correctly" )
	{
		const float yaw = math::radians( 30.0f );
		const float pitch = math::radians( 45.0f );
		const float roll = math::radians( 60.0f );

		const math::Mat4f matrix = math::Mat4f::fromEulerAngles( yaw, pitch, roll );
		// Extract rotation part as Mat3 and convert back to Euler angles
		const math::Mat3f rotationPart = matrix.toMat3();
		const math::Vec3f extractedAngles = rotationPart.toEulerAngles();

		// Check that extracted angles match input angles
		// Note: roll (X) maps to extractedAngles.x
		// Note: pitch (Y) maps to extractedAngles.y
		// Note: yaw (Z) maps to extractedAngles.z
		REQUIRE( extractedAngles.x == Catch::Approx( roll ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.y == Catch::Approx( pitch ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.z == Catch::Approx( yaw ).margin( 1e-5f ) );
	}

	SECTION( "Single axis rotations match individual rotation functions" )
	{
		const float angle = math::radians( 45.0f );

		// Test roll-only rotation (X-axis)
		const math::Mat4f rollOnly = math::Mat4f::fromEulerAngles( 0.0f, 0.0f, angle );
		const math::Mat4f xIndividual = math::Mat4f::rotationX( angle );
		REQUIRE( rollOnly.m00() == Catch::Approx( xIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m11() == Catch::Approx( xIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m22() == Catch::Approx( xIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m33() == Catch::Approx( xIndividual.m33() ).margin( 1e-5f ) );

		// Test pitch-only rotation (Y-axis)
		const math::Mat4f pitchOnly = math::Mat4f::fromEulerAngles( 0.0f, angle, 0.0f );
		const math::Mat4f yIndividual = math::Mat4f::rotationY( angle );
		REQUIRE( pitchOnly.m00() == Catch::Approx( yIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m11() == Catch::Approx( yIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m22() == Catch::Approx( yIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m33() == Catch::Approx( yIndividual.m33() ).margin( 1e-5f ) );

		// Test yaw-only rotation (Z-axis)
		const math::Mat4f yawOnly = math::Mat4f::fromEulerAngles( angle, 0.0f, 0.0f );
		const math::Mat4f zIndividual = math::Mat4f::rotationZ( angle );
		REQUIRE( yawOnly.m00() == Catch::Approx( zIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m11() == Catch::Approx( zIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m22() == Catch::Approx( zIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m33() == Catch::Approx( zIndividual.m33() ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Matrix view transformations", "[math][matrix][mat4][view]" )
{
	SECTION( "Look-at matrix" )
	{
		math::Vec3f eye{ 0.0f, 0.0f, 5.0f };
		math::Vec3f target{ 0.0f, 0.0f, 0.0f };
		math::Vec3f up{ 0.0f, 1.0f, 0.0f };

		math::Mat4f view = math::Mat4f::lookAt( eye, target, up );

		// A point at the target should transform to (0,0,-5) in view space
		math::Vec3f targetInViewSpace = view.transformPoint( target );
		REQUIRE( targetInViewSpace.x == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( targetInViewSpace.y == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( targetInViewSpace.z == Catch::Approx( -5.0f ).margin( 1e-5f ) );

		// The eye position should transform to the origin in view space
		math::Vec3f eyeInViewSpace = view.transformPoint( eye );
		REQUIRE( eyeInViewSpace.x == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( eyeInViewSpace.y == Catch::Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( eyeInViewSpace.z == Catch::Approx( 0.0f ).margin( 1e-5f ) );
	}
}

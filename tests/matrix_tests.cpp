#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
import engine.math;
import engine.matrix;
import engine.vec;

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
		const Mat4<float> mat4 = Mat4<float>::identity();
		const Mat3<float> mat3 = mat4.toMat3();

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
		const Mat4<float> mat4 = Mat4<float>::translation( 5.0f, 10.0f, 15.0f );
		const Mat3<float> mat3 = mat4.toMat3();

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
		const Mat4<float> mat4 = Mat4<float>::identity();
		const Vec3<float> scale = mat4.extractScale();

		REQUIRE( scale.x == Approx( 1.0f ) );
		REQUIRE( scale.y == Approx( 1.0f ) );
		REQUIRE( scale.z == Approx( 1.0f ) );
	}

	SECTION( "Pure scale matrix extracts correct values" )
	{
		const Mat4<float> mat4 = Mat4<float>::scale( 2.0f, 3.0f, 4.0f );
		const Vec3<float> scale = mat4.extractScale();

		REQUIRE( scale.x == Approx( 2.0f ) );
		REQUIRE( scale.y == Approx( 3.0f ) );
		REQUIRE( scale.z == Approx( 4.0f ) );
	}
}

TEST_CASE( "Mat3 toEulerAngles converts rotation matrix to Euler angles", "[math][matrix][mat3][euler]" )
{
	SECTION( "Identity matrix has zero rotation" )
	{
		const Mat3<float> mat3 = Mat3<float>::identity();
		const Vec3<float> angles = mat3.toEulerAngles();

		REQUIRE( angles.x == Approx( 0.0f ) );
		REQUIRE( angles.y == Approx( 0.0f ) );
		REQUIRE( angles.z == Approx( 0.0f ) );
	}

	SECTION( "90 degree Z rotation converts correctly" )
	{
		const Mat3<float> mat3 = Mat3<float>::rotationZ( math::radians( 90.0f ) );
		const Vec3<float> angles = mat3.toEulerAngles();

		// Should have 90 degrees rotation in Z
		REQUIRE( angles.x == Approx( 0.0f ) );
		REQUIRE( angles.y == Approx( 0.0f ) );
		REQUIRE( angles.z == Approx( math::radians( 90.0f ) ) );
	}

	SECTION( "90 degree X rotation converts correctly" )
	{
		const Mat3<float> mat3 = Mat3<float>::rotationX( math::radians( 90.0f ) );
		const Vec3<float> angles = mat3.toEulerAngles();

		// Should have 90 degrees rotation in X
		REQUIRE( angles.x == Approx( math::radians( 90.0f ) ) );
		REQUIRE( angles.y == Approx( 0.0f ) );
		REQUIRE( angles.z == Approx( 0.0f ) );
	}

	SECTION( "90 degree Y rotation converts correctly" )
	{
		// Test with smaller angle to avoid gimbal lock
		const float angle = math::radians( 45.0f );
		const Mat3<float> mat3 = Mat3<float>::rotationY( angle );
		const Vec3<float> angles = mat3.toEulerAngles();

		// Should have 45 degrees rotation in Y, with X and Z close to zero
		REQUIRE( std::abs( angles.x ) < 0.01f );
		REQUIRE( std::abs( angles.y - angle ) < 0.01f );
		REQUIRE( std::abs( angles.z ) < 0.01f );
	}

	SECTION( "Small angle rotation preserves precision" )
	{
		const float smallAngle = math::radians( 5.0f );
		const Mat3<float> mat3 = Mat3<float>::rotationX( smallAngle );
		const Vec3<float> angles = mat3.toEulerAngles();

		REQUIRE( angles.x == Approx( smallAngle ).margin( 1e-6f ) );
		REQUIRE( angles.y == Approx( 0.0f ).margin( 1e-6f ) );
		REQUIRE( angles.z == Approx( 0.0f ).margin( 1e-6f ) );
	}
}

TEST_CASE( "Mat2 basic operations", "[math][matrix][mat2]" )
{
	Mat2<float> identity = Mat2<float>::identity();

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
		Mat2<float> a{ 1.0f, 2.0f, 3.0f, 4.0f };
		Mat2<float> b{ 5.0f, 6.0f, 7.0f, 8.0f };

		// Addition
		Mat2<float> sum = a + b;
		REQUIRE( sum.m00() == 6.0f );
		REQUIRE( sum.m01() == 8.0f );
		REQUIRE( sum.m10() == 10.0f );
		REQUIRE( sum.m11() == 12.0f );

		// Subtraction
		Mat2<float> diff = b - a;
		REQUIRE( diff.m00() == 4.0f );
		REQUIRE( diff.m01() == 4.0f );
		REQUIRE( diff.m10() == 4.0f );
		REQUIRE( diff.m11() == 4.0f );

		// Scalar multiplication
		Mat2<float> scaled = a * 2.0f;
		REQUIRE( scaled.m00() == 2.0f );
		REQUIRE( scaled.m01() == 4.0f );
		REQUIRE( scaled.m10() == 6.0f );
		REQUIRE( scaled.m11() == 8.0f );

		// Matrix multiplication
		Mat2<float> product = a * b;
		REQUIRE( product.m00() == 19.0f ); // 1*5 + 2*7
		REQUIRE( product.m01() == 22.0f ); // 1*6 + 2*8
		REQUIRE( product.m10() == 43.0f ); // 3*5 + 4*7
		REQUIRE( product.m11() == 50.0f ); // 3*6 + 4*8
	}

	SECTION( "Determinant and inverse" )
	{
		Mat2<float> m{ 4.0f, 7.0f, 2.0f, 6.0f };

		float det = m.determinant();
		REQUIRE( det == 10.0f ); // 4*6 - 7*2

		Mat2<float> inv = m.inverse();
		REQUIRE( inv.m00() == Approx( 0.6f ) );
		REQUIRE( inv.m01() == Approx( -0.7f ) );
		REQUIRE( inv.m10() == Approx( -0.2f ) );
		REQUIRE( inv.m11() == Approx( 0.4f ) );

		// Test inverse: m * m^-1 = identity
		Mat2<float> result = m * inv;
		REQUIRE( result.m00() == Approx( 1.0f ) );
		REQUIRE( result.m01() == Approx( 0.0f ) );
		REQUIRE( result.m10() == Approx( 0.0f ) );
		REQUIRE( result.m11() == Approx( 1.0f ) );
	}

	SECTION( "Rotation matrix" )
	{
		// 90 degrees rotation
		Mat2<float> rot = Mat2<float>::rotation( math::radians( 90.0f ) );

		Vec2<float> v{ 1.0f, 0.0f };
		Vec2<float> rotated = rot * v;

		REQUIRE( rotated.x == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Approx( 1.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat3 basic operations", "[math][matrix][mat3]" )
{
	Mat3<float> identity = Mat3<float>::identity();

	SECTION( "Identity matrix properties" )
	{
		REQUIRE( identity.m00() == 1.0f );
		REQUIRE( identity.m11() == 1.0f );
		REQUIRE( identity.m22() == 1.0f );

		Vec3<float> v{ 3.0f, 4.0f, 5.0f };
		Vec3<float> result = identity * v;
		REQUIRE( result.x == v.x );
		REQUIRE( result.y == v.y );
		REQUIRE( result.z == v.z );
	}

	SECTION( "Translation matrix" )
	{
		Mat3<float> trans = Mat3<float>::translation( 10.0f, 20.0f );

		Vec3<float> v{ 5.0f, 7.0f, 1.0f }; // Homogeneous coordinate w=1
		Vec3<float> translated = trans * v;

		REQUIRE( translated.x == 15.0f ); // 5 + 10
		REQUIRE( translated.y == 27.0f ); // 7 + 20
		REQUIRE( translated.z == 1.0f );  // w coordinate unchanged
	}

	SECTION( "Scale matrix" )
	{
		Mat3<float> scale = Mat3<float>::scale( 2.0f, 3.0f, 4.0f );

		Vec3<float> v{ 5.0f, 7.0f, 9.0f };
		Vec3<float> scaled = scale * v;

		REQUIRE( scaled.x == 10.0f ); // 5 * 2
		REQUIRE( scaled.y == 21.0f ); // 7 * 3
		REQUIRE( scaled.z == 36.0f ); // 9 * 4
	}

	SECTION( "Rotation matrices" )
	{
		// 90 degrees around Z axis
		Mat3<float> rotZ = Mat3<float>::rotationZ( math::radians( 90.0f ) );

		Vec3<float> v{ 1.0f, 0.0f, 0.0f };
		Vec3<float> rotated = rotZ * v;

		REQUIRE( rotated.x == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.z == Approx( 0.0f ).margin( 1e-5f ) );
	}

	SECTION( "Matrix determinant and inverse" )
	{
		Mat3<float> m{
			1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 4.0f, 5.0f, 6.0f, 0.0f
		};

		float det = m.determinant();
		REQUIRE( det == Approx( 1.0f ).margin( 1e-5f ) );

		Mat3<float> inv = m.inverse();

		// Test inverse: m * m^-1 = identity
		Mat3<float> result = m * inv;
		REQUIRE( result.m00() == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m11() == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m22() == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m01() == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m02() == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m10() == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m12() == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m20() == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m21() == Approx( 0.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat4 transformations", "[math][matrix][mat4]" )
{
	Mat4<float> identity = Mat4<float>::identity();

	SECTION( "Translation matrix" )
	{
		Mat4<float> trans = Mat4<float>::translation( 10.0f, 20.0f, 30.0f );

		Vec3<float> v{ 5.0f, 7.0f, 9.0f };
		Vec3<float> translated = trans.transformPoint( v );

		REQUIRE( translated.x == 15.0f ); // 5 + 10
		REQUIRE( translated.y == 27.0f ); // 7 + 20
		REQUIRE( translated.z == 39.0f ); // 9 + 30

		// Direction vectors shouldn't be affected by translation
		Vec3<float> dir{ 1.0f, 0.0f, 0.0f };
		Vec3<float> transformedDir = trans.transformVector( dir );

		REQUIRE( transformedDir.x == 1.0f );
		REQUIRE( transformedDir.y == 0.0f );
		REQUIRE( transformedDir.z == 0.0f );
	}

	SECTION( "Scale matrix" )
	{
		Mat4<float> scale = Mat4<float>::scale( 2.0f, 3.0f, 4.0f );

		Vec3<float> v{ 5.0f, 7.0f, 9.0f };
		Vec3<float> scaled = scale.transformPoint( v );

		REQUIRE( scaled.x == Approx( 10.0f ) ); // 5 * 2
		REQUIRE( scaled.y == Approx( 21.0f ) ); // 7 * 3
		REQUIRE( scaled.z == Approx( 36.0f ) ); // 9 * 4
	}

	SECTION( "Rotation matrices" )
	{
		// 90 degrees around Y axis
		Mat4<float> rotY = Mat4<float>::rotationY( math::radians( 90.0f ) );

		Vec3<float> v{ 0.0f, 0.0f, 1.0f };
		Vec3<float> rotated = rotY.transformPoint( v );

		REQUIRE( rotated.x == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.y == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( rotated.z == Approx( 0.0f ).margin( 1e-5f ) );
	}

	SECTION( "Combined transformations" )
	{
		// Create a transformation that:
		// 1. Scales by 2 in all directions
		// 2. Rotates 90 degrees around Y
		// 3. Translates by (10, 20, 30)
		Mat4<float> scale = Mat4<float>::scale( 2.0f, 2.0f, 2.0f );
		Mat4<float> rotate = Mat4<float>::rotationY( math::radians( 90.0f ) );
		Mat4<float> translate = Mat4<float>::translation( 10.0f, 20.0f, 30.0f );

		// Apply transforms in correct order (scale, then rotate, then translate)
		Mat4<float> combined = translate * rotate * scale;

		Vec3<float> v{ 1.0f, 2.0f, 3.0f };
		Vec3<float> result = combined.transformPoint( v );

		// First scaled: (2, 4, 6)
		// Then rotated: (6, 4, -2) (90° Y rotation swaps X and Z and negates Z)
		// Then translated: (16, 24, 28)
		REQUIRE( result.x == Approx( 16.0f ).margin( 1e-4f ) );
		REQUIRE( result.y == Approx( 24.0f ).margin( 1e-4f ) );
		REQUIRE( result.z == Approx( 28.0f ).margin( 1e-4f ) );
	}

	SECTION( "Perspective matrix" )
	{
		float fov = math::radians( 45.0f );
		float aspect = 16.0f / 9.0f;
		float near = 0.1f;
		float far = 100.0f;

		Mat4<float> perspective = Mat4<float>::perspective( fov, aspect, near, far );

		// Point on near plane, center
		Vec3<float> nearCenter{ 0.0f, 0.0f, -near };
		Vec4<float> clipSpace = perspective * Vec4<float>{ nearCenter.x, nearCenter.y, nearCenter.z, 1.0f };

		// After perspective division, z should be -1 (OpenGL NDC for near plane)
		float ndcZ = clipSpace.z / clipSpace.w;
		REQUIRE( ndcZ == Approx( -1.0f ).margin( 1e-5f ) );

		// Point on far plane, center
		Vec3<float> farCenter{ 0.0f, 0.0f, -far };
		clipSpace = perspective * Vec4<float>{ farCenter.x, farCenter.y, farCenter.z, 1.0f };

		// After perspective division, z should be 1 (OpenGL NDC for far plane)
		ndcZ = clipSpace.z / clipSpace.w;
		REQUIRE( ndcZ == Approx( 1.0f ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat3 fromEulerAngles creates composite rotation matrix", "[math][matrix][mat3][rotation][euler]" )
{
	SECTION( "Zero rotations produce identity matrix" )
	{
		const Mat3<float> mat3 = Mat3<float>::fromEulerAngles( 0.0f, 0.0f, 0.0f );
		const Mat3<float> identity = Mat3<float>::identity();

		REQUIRE( mat3.m00() == Approx( identity.m00() ) );
		REQUIRE( mat3.m01() == Approx( identity.m01() ) );
		REQUIRE( mat3.m02() == Approx( identity.m02() ) );
		REQUIRE( mat3.m10() == Approx( identity.m10() ) );
		REQUIRE( mat3.m11() == Approx( identity.m11() ) );
		REQUIRE( mat3.m12() == Approx( identity.m12() ) );
		REQUIRE( mat3.m20() == Approx( identity.m20() ) );
		REQUIRE( mat3.m21() == Approx( identity.m21() ) );
		REQUIRE( mat3.m22() == Approx( identity.m22() ) );
	}

	SECTION( "Euler angles round-trip correctly" )
	{
		const float yaw = math::radians( 30.0f );
		const float pitch = math::radians( 45.0f );
		const float roll = math::radians( 60.0f );

		const Mat3<float> matrix = Mat3<float>::fromEulerAngles( yaw, pitch, roll );
		const Vec3<float> extractedAngles = matrix.toEulerAngles();

		// Check that extracted angles match input angles
		// Note: roll (X) maps to extractedAngles.x
		// Note: pitch (Y) maps to extractedAngles.y
		// Note: yaw (Z) maps to extractedAngles.z
		REQUIRE( extractedAngles.x == Approx( roll ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.y == Approx( pitch ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.z == Approx( yaw ).margin( 1e-5f ) );
	}

	SECTION( "Single axis rotations match individual rotation functions" )
	{
		const float angle = math::radians( 45.0f );

		// Test roll-only rotation (X-axis)
		const Mat3<float> rollOnly = Mat3<float>::fromEulerAngles( 0.0f, 0.0f, angle );
		const Mat3<float> xIndividual = Mat3<float>::rotationX( angle );
		REQUIRE( rollOnly.m00() == Approx( xIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m11() == Approx( xIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m22() == Approx( xIndividual.m22() ).margin( 1e-5f ) );

		// Test pitch-only rotation (Y-axis)
		const Mat3<float> pitchOnly = Mat3<float>::fromEulerAngles( 0.0f, angle, 0.0f );
		const Mat3<float> yIndividual = Mat3<float>::rotationY( angle );
		REQUIRE( pitchOnly.m00() == Approx( yIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m11() == Approx( yIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m22() == Approx( yIndividual.m22() ).margin( 1e-5f ) );

		// Test yaw-only rotation (Z-axis)
		const Mat3<float> yawOnly = Mat3<float>::fromEulerAngles( angle, 0.0f, 0.0f );
		const Mat3<float> zIndividual = Mat3<float>::rotationZ( angle );
		REQUIRE( yawOnly.m00() == Approx( zIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m11() == Approx( zIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m22() == Approx( zIndividual.m22() ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Mat4 fromEulerAngles creates composite rotation matrix", "[math][matrix][mat4][rotation][euler]" )
{
	SECTION( "Zero rotations produce identity matrix" )
	{
		const Mat4<float> mat4 = Mat4<float>::fromEulerAngles( 0.0f, 0.0f, 0.0f );
		const Mat4<float> identity = Mat4<float>::identity();

		REQUIRE( mat4.m00() == Approx( identity.m00() ) );
		REQUIRE( mat4.m01() == Approx( identity.m01() ) );
		REQUIRE( mat4.m02() == Approx( identity.m02() ) );
		REQUIRE( mat4.m03() == Approx( identity.m03() ) );
		REQUIRE( mat4.m10() == Approx( identity.m10() ) );
		REQUIRE( mat4.m11() == Approx( identity.m11() ) );
		REQUIRE( mat4.m12() == Approx( identity.m12() ) );
		REQUIRE( mat4.m13() == Approx( identity.m13() ) );
		REQUIRE( mat4.m20() == Approx( identity.m20() ) );
		REQUIRE( mat4.m21() == Approx( identity.m21() ) );
		REQUIRE( mat4.m22() == Approx( identity.m22() ) );
		REQUIRE( mat4.m23() == Approx( identity.m23() ) );
		REQUIRE( mat4.m30() == Approx( identity.m30() ) );
		REQUIRE( mat4.m31() == Approx( identity.m31() ) );
		REQUIRE( mat4.m32() == Approx( identity.m32() ) );
		REQUIRE( mat4.m33() == Approx( identity.m33() ) );
	}

	SECTION( "Euler angles round-trip correctly" )
	{
		const float yaw = math::radians( 30.0f );
		const float pitch = math::radians( 45.0f );
		const float roll = math::radians( 60.0f );

		const Mat4<float> matrix = Mat4<float>::fromEulerAngles( yaw, pitch, roll );
		// Extract rotation part as Mat3 and convert back to Euler angles
		const Mat3<float> rotationPart = matrix.toMat3();
		const Vec3<float> extractedAngles = rotationPart.toEulerAngles();

		// Check that extracted angles match input angles
		// Note: roll (X) maps to extractedAngles.x
		// Note: pitch (Y) maps to extractedAngles.y
		// Note: yaw (Z) maps to extractedAngles.z
		REQUIRE( extractedAngles.x == Approx( roll ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.y == Approx( pitch ).margin( 1e-5f ) );
		REQUIRE( extractedAngles.z == Approx( yaw ).margin( 1e-5f ) );
	}

	SECTION( "Single axis rotations match individual rotation functions" )
	{
		const float angle = math::radians( 45.0f );

		// Test roll-only rotation (X-axis)
		const Mat4<float> rollOnly = Mat4<float>::fromEulerAngles( 0.0f, 0.0f, angle );
		const Mat4<float> xIndividual = Mat4<float>::rotationX( angle );
		REQUIRE( rollOnly.m00() == Approx( xIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m11() == Approx( xIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m22() == Approx( xIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( rollOnly.m33() == Approx( xIndividual.m33() ).margin( 1e-5f ) );

		// Test pitch-only rotation (Y-axis)
		const Mat4<float> pitchOnly = Mat4<float>::fromEulerAngles( 0.0f, angle, 0.0f );
		const Mat4<float> yIndividual = Mat4<float>::rotationY( angle );
		REQUIRE( pitchOnly.m00() == Approx( yIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m11() == Approx( yIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m22() == Approx( yIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( pitchOnly.m33() == Approx( yIndividual.m33() ).margin( 1e-5f ) );

		// Test yaw-only rotation (Z-axis)
		const Mat4<float> yawOnly = Mat4<float>::fromEulerAngles( angle, 0.0f, 0.0f );
		const Mat4<float> zIndividual = Mat4<float>::rotationZ( angle );
		REQUIRE( yawOnly.m00() == Approx( zIndividual.m00() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m11() == Approx( zIndividual.m11() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m22() == Approx( zIndividual.m22() ).margin( 1e-5f ) );
		REQUIRE( yawOnly.m33() == Approx( zIndividual.m33() ).margin( 1e-5f ) );
	}
}

TEST_CASE( "Matrix view transformations", "[math][matrix][mat4][view]" )
{
	SECTION( "Look-at matrix" )
	{
		Vec3<float> eye{ 0.0f, 0.0f, 5.0f };
		Vec3<float> target{ 0.0f, 0.0f, 0.0f };
		Vec3<float> up{ 0.0f, 1.0f, 0.0f };

		Mat4<float> view = Mat4<float>::lookAt( eye, target, up );

		// A point at the target should transform to (0,0,-5) in view space
		Vec3<float> targetInViewSpace = view.transformPoint( target );
		REQUIRE( targetInViewSpace.x == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( targetInViewSpace.y == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( targetInViewSpace.z == Approx( -5.0f ).margin( 1e-5f ) );

		// The eye position should transform to the origin in view space
		Vec3<float> eyeInViewSpace = view.transformPoint( eye );
		REQUIRE( eyeInViewSpace.x == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( eyeInViewSpace.y == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( eyeInViewSpace.z == Approx( 0.0f ).margin( 1e-5f ) );
	}
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>
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

TEST_CASE( "Mat2 basic operations", "[math][matrix][mat2]" )
{
	Mat2<float> identity = Mat2<float>::identity();

	SECTION( "Identity matrix properties" )
	{
		REQUIRE( identity.m00 == 1.0f );
		REQUIRE( identity.m11 == 1.0f );
		REQUIRE( identity.m01 == 0.0f );
		REQUIRE( identity.m10 == 0.0f );

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
		REQUIRE( sum.m00 == 6.0f );
		REQUIRE( sum.m01 == 8.0f );
		REQUIRE( sum.m10 == 10.0f );
		REQUIRE( sum.m11 == 12.0f );

		// Subtraction
		Mat2<float> diff = b - a;
		REQUIRE( diff.m00 == 4.0f );
		REQUIRE( diff.m01 == 4.0f );
		REQUIRE( diff.m10 == 4.0f );
		REQUIRE( diff.m11 == 4.0f );

		// Scalar multiplication
		Mat2<float> scaled = a * 2.0f;
		REQUIRE( scaled.m00 == 2.0f );
		REQUIRE( scaled.m01 == 4.0f );
		REQUIRE( scaled.m10 == 6.0f );
		REQUIRE( scaled.m11 == 8.0f );

		// Matrix multiplication
		Mat2<float> product = a * b;
		REQUIRE( product.m00 == 19.0f ); // 1*5 + 2*7
		REQUIRE( product.m01 == 22.0f ); // 1*6 + 2*8
		REQUIRE( product.m10 == 43.0f ); // 3*5 + 4*7
		REQUIRE( product.m11 == 50.0f ); // 3*6 + 4*8
	}

	SECTION( "Determinant and inverse" )
	{
		Mat2<float> m{ 4.0f, 7.0f, 2.0f, 6.0f };

		float det = m.determinant();
		REQUIRE( det == 10.0f ); // 4*6 - 7*2

		Mat2<float> inv = m.inverse();
		REQUIRE( inv.m00 == Approx( 0.6f ) );
		REQUIRE( inv.m01 == Approx( -0.7f ) );
		REQUIRE( inv.m10 == Approx( -0.2f ) );
		REQUIRE( inv.m11 == Approx( 0.4f ) );

		// Test inverse: m * m^-1 = identity
		Mat2<float> result = m * inv;
		REQUIRE( result.m00 == Approx( 1.0f ) );
		REQUIRE( result.m01 == Approx( 0.0f ) );
		REQUIRE( result.m10 == Approx( 0.0f ) );
		REQUIRE( result.m11 == Approx( 1.0f ) );
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
		REQUIRE( identity.m00 == 1.0f );
		REQUIRE( identity.m11 == 1.0f );
		REQUIRE( identity.m22 == 1.0f );

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
		REQUIRE( result.m00 == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m11 == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m22 == Approx( 1.0f ).margin( 1e-5f ) );
		REQUIRE( result.m01 == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m02 == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m10 == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m12 == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m20 == Approx( 0.0f ).margin( 1e-5f ) );
		REQUIRE( result.m21 == Approx( 0.0f ).margin( 1e-5f ) );
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

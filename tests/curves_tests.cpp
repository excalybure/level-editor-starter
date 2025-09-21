#include <catch2/catch_all.hpp>

#include "engine/math/curves.h"
#include "engine/math/vec.h"
#include "engine/math/math.h"


// Helper function for floating point comparison
template <typename T>
bool approxEqual( T a, T b, T eps = static_cast<T>( 1e-6 ) )
{
	return std::abs( a - b ) <= eps;
}

TEST_CASE( "Curve and Spline Functions", "[curves]" )
{
	SECTION( "Linear Bezier Curves" )
	{
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 10.0f, 5.0f };

		// Test endpoints
		const auto start = linearBezier( p0, p1, 0.0f );
		REQUIRE( approxEqual( start.x, 0.0f ) );
		REQUIRE( approxEqual( start.y, 0.0f ) );

		const auto end = linearBezier( p0, p1, 1.0f );
		REQUIRE( approxEqual( end.x, 10.0f ) );
		REQUIRE( approxEqual( end.y, 5.0f ) );

		// Test midpoint
		const auto mid = linearBezier( p0, p1, 0.5f );
		REQUIRE( approxEqual( mid.x, 5.0f ) );
		REQUIRE( approxEqual( mid.y, 2.5f ) );

		// Test 3D version
		const math::Vec3<> p0_3d{ 0.0f, 0.0f, 0.0f };
		const math::Vec3<> p1_3d{ 6.0f, 3.0f, 9.0f };

		const auto mid_3d = linearBezier( p0_3d, p1_3d, 0.5f );
		REQUIRE( approxEqual( mid_3d.x, 3.0f ) );
		REQUIRE( approxEqual( mid_3d.y, 1.5f ) );
		REQUIRE( approxEqual( mid_3d.z, 4.5f ) );
	}

	SECTION( "Quadratic Bezier Curves" )
	{
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 5.0f, 10.0f }; // Control point
		const math::Vec2<> p2{ 10.0f, 0.0f };

		// Test endpoints
		const auto start = quadraticBezier( p0, p1, p2, 0.0f );
		REQUIRE( approxEqual( start.x, 0.0f ) );
		REQUIRE( approxEqual( start.y, 0.0f ) );

		const auto end = quadraticBezier( p0, p1, p2, 1.0f );
		REQUIRE( approxEqual( end.x, 10.0f ) );
		REQUIRE( approxEqual( end.y, 0.0f ) );

		// Test midpoint (should be influenced by control point)
		const auto mid = quadraticBezier( p0, p1, p2, 0.5f );
		REQUIRE( approxEqual( mid.x, 5.0f ) );
		REQUIRE( approxEqual( mid.y, 5.0f ) ); // Pulled up by control point

		// Test curve is smooth (derivative should be continuous)
		const auto deriv_start = quadraticBezierDerivative( p0, p1, p2, 0.0f );
		const auto deriv_end = quadraticBezierDerivative( p0, p1, p2, 1.0f );

		// At t=0, derivative should point towards p1
		REQUIRE( deriv_start.x > 0.0f );
		REQUIRE( deriv_start.y > 0.0f );

		// At t=1, derivative should point from p1 to p2
		REQUIRE( deriv_end.x > 0.0f );
		REQUIRE( deriv_end.y < 0.0f );
	}

	SECTION( "Cubic Bezier Curves" )
	{
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 0.0f, 10.0f };	 // First control point
		const math::Vec2<> p2{ 10.0f, 10.0f }; // Second control point
		const math::Vec2<> p3{ 10.0f, 0.0f };

		// Test endpoints
		const auto start = cubicBezier( p0, p1, p2, p3, 0.0f );
		REQUIRE( approxEqual( start.x, 0.0f ) );
		REQUIRE( approxEqual( start.y, 0.0f ) );

		const auto end = cubicBezier( p0, p1, p2, p3, 1.0f );
		REQUIRE( approxEqual( end.x, 10.0f ) );
		REQUIRE( approxEqual( end.y, 0.0f ) );

		// Test smoothness with derivatives
		const auto deriv_0 = cubicBezierDerivative( p0, p1, p2, p3, 0.0f );
		const auto deriv_1 = cubicBezierDerivative( p0, p1, p2, p3, 1.0f );

		// Initial tangent should point towards first control point
		REQUIRE( approxEqual( deriv_0.x, 0.0f ) );
		REQUIRE( deriv_0.y > 0.0f );

		// Final tangent should be from p2 to p3
		// At t=1: derivative = 3 * (p3 - p2) = 3 * ((10,0) - (10,10)) = 3 * (0,-10) = (0,-30)
		REQUIRE( approxEqual( deriv_1.x, 0.0f, 1e-5f ) ); // Should be 0, not > 0
		REQUIRE( approxEqual( deriv_1.y, -30.0f, 1e-5f ) );

		// Test 3D cubic Bezier
		const math::Vec3<> p0_3d{ 0.0f, 0.0f, 0.0f };
		const math::Vec3<> p1_3d{ 1.0f, 1.0f, 1.0f };
		const math::Vec3<> p2_3d{ 2.0f, 2.0f, 2.0f };
		const math::Vec3<> p3_3d{ 3.0f, 0.0f, 3.0f };

		const auto mid_3d = cubicBezier( p0_3d, p1_3d, p2_3d, p3_3d, 0.5f );
		REQUIRE( mid_3d.x >= 0.0f );
		REQUIRE( mid_3d.x <= 3.0f );
		REQUIRE( mid_3d.z >= 0.0f );
		REQUIRE( mid_3d.z <= 3.0f );
	}

	SECTION( "Catmull-Rom Splines" )
	{
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 1.0f, 1.0f };	// Start point
		const math::Vec2<> p2{ 2.0f, -1.0f }; // End point (changed to create curvature)
		const math::Vec2<> p3{ 3.0f, 0.0f };

		// Test that spline passes through control points
		const auto start = catmullRom( p0, p1, p2, p3, 0.0f );
		REQUIRE( approxEqual( start.x, p1.x ) );
		REQUIRE( approxEqual( start.y, p1.y ) );

		const auto end = catmullRom( p0, p1, p2, p3, 1.0f );
		REQUIRE( approxEqual( end.x, p2.x ) );
		REQUIRE( approxEqual( end.y, p2.y ) );

		// Test smoothness
		const auto deriv_mid = catmullRomDerivative( p0, p1, p2, p3, 0.5f );
		REQUIRE( std::abs( deriv_mid.x ) > 0.0f ); // Should have some horizontal movement

		// Test with tension parameter - use more dramatic difference
		const math::Vec2<> q0{ 0.0f, 0.0f };
		const math::Vec2<> q1{ 1.0f, 0.0f };
		const math::Vec2<> q2{ 2.0f, 0.0f };
		const math::Vec2<> q3{ 3.0f, 1.0f }; // Non-collinear point to create curvature

		const auto with_high_tension = catmullRomWithTension( q0, q1, q2, q3, 0.5f, 2.0f ); // Very high tension
		const auto with_low_tension = catmullRomWithTension( q0, q1, q2, q3, 0.5f, 0.1f );	// Low tension

		// Different tensions should produce different results
		REQUIRE( std::abs( with_high_tension.y - with_low_tension.y ) > 1e-3f );

		// Test 3D Catmull-Rom
		const math::Vec3<> p0_3d{ 0.0f, 0.0f, 0.0f };
		const math::Vec3<> p1_3d{ 1.0f, 1.0f, 1.0f };
		const math::Vec3<> p2_3d{ 2.0f, 1.0f, 2.0f };
		const math::Vec3<> p3_3d{ 3.0f, 0.0f, 1.0f };

		const auto start_3d = catmullRom( p0_3d, p1_3d, p2_3d, p3_3d, 0.0f );
		REQUIRE( approxEqual( start_3d.x, p1_3d.x ) );
		REQUIRE( approxEqual( start_3d.y, p1_3d.y ) );
		REQUIRE( approxEqual( start_3d.z, p1_3d.z ) );
	}

	SECTION( "Arc Length Calculations" )
	{
		// Test simple line segment
		const std::vector<math::Vec2<>> line = {
			{ 0.0f, 0.0f },
			{ 3.0f, 4.0f } // 3-4-5 triangle, length = 5
		};

		const auto lineLength = arcLength( line );
		REQUIRE( approxEqual( lineLength, 5.0f ) );

		// Test square path
		const std::vector<math::Vec2<>> square = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
			{ 0.0f, 0.0f }
		};

		const auto squareLength = arcLength( square );
		REQUIRE( approxEqual( squareLength, 4.0f ) );

		// Test 3D arc length
		const std::vector<math::Vec3<>> line3d = {
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f } // sqrt(3) length
		};

		const auto line3dLength = arcLength( line3d );
		REQUIRE( approxEqual( line3dLength, std::sqrt( 3.0f ) ) );

		// Test Bezier arc length approximation
		const math::Vec2<> b0{ 0.0f, 0.0f };
		const math::Vec2<> b1{ 1.0f, 1.0f };
		const math::Vec2<> b2{ 2.0f, 1.0f };
		const math::Vec2<> b3{ 3.0f, 0.0f };

		const auto bezierLength = bezierArcLength( b0, b1, b2, b3, 64 );
		REQUIRE( bezierLength > 0.0f );
		REQUIRE( bezierLength < 10.0f ); // Reasonable bounds
	}

	SECTION( "Sample by Distance" )
	{
		const std::vector<math::Vec2<>> line = {
			{ 0.0f, 0.0f },
			{ 10.0f, 0.0f }
		};

		// Test sampling at start
		const auto start = sampleByDistance( line, 0.0f );
		REQUIRE( approxEqual( start.x, 0.0f ) );
		REQUIRE( approxEqual( start.y, 0.0f ) );

		// Test sampling at midpoint
		const auto mid = sampleByDistance( line, 5.0f );
		REQUIRE( approxEqual( mid.x, 5.0f ) );
		REQUIRE( approxEqual( mid.y, 0.0f ) );

		// Test sampling at end
		const auto end = sampleByDistance( line, 10.0f );
		REQUIRE( approxEqual( end.x, 10.0f ) );
		REQUIRE( approxEqual( end.y, 0.0f ) );

		// Test sampling beyond end
		const auto beyond = sampleByDistance( line, 15.0f );
		REQUIRE( approxEqual( beyond.x, 10.0f ) );
		REQUIRE( approxEqual( beyond.y, 0.0f ) );

		// Test 3D sampling
		const std::vector<math::Vec3<>> line3d = {
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 10.0f }
		};

		const auto mid3d = sampleByDistance( line3d, 5.0f );
		REQUIRE( approxEqual( mid3d.x, 0.0f ) );
		REQUIRE( approxEqual( mid3d.y, 0.0f ) );
		REQUIRE( approxEqual( mid3d.z, 5.0f ) );

		// Test empty curve
		const std::vector<math::Vec2<>> empty;
		const auto emptyResult = sampleByDistance( empty, 5.0f );
		REQUIRE( approxEqual( emptyResult.x, 0.0f ) );
		REQUIRE( approxEqual( emptyResult.y, 0.0f ) );

		// Test single point
		const std::vector<math::Vec2<>> single = { { 5.0f, 3.0f } };
		const auto singleResult = sampleByDistance( single, 10.0f );
		REQUIRE( approxEqual( singleResult.x, 5.0f ) );
		REQUIRE( approxEqual( singleResult.y, 3.0f ) );
	}

	SECTION( "Curve Utilities" )
	{
		const std::vector<math::Vec2<>> curve = {
			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 2.0f, 0.0f },
			{ 3.0f, -1.0f }
		};

		// Test closest point finding
		const math::Vec2<> queryPoint{ 1.1f, 0.9f };
		math::Vec2<> closestPoint;
		int segmentIndex;

		const auto distance = findClosestPointOnCurve( curve, queryPoint, closestPoint, segmentIndex );

		REQUIRE( distance >= 0.0f );
		REQUIRE( segmentIndex >= 0 );
		REQUIRE( segmentIndex < static_cast<int>( curve.size() ) );

		// Test smooth curve generation
		const std::vector<math::Vec2<>> controlPoints = {
			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 2.0f, 0.0f }
		};

		const auto smoothCurve = generateSmoothCurve( controlPoints, 8 );
		REQUIRE( smoothCurve.size() > controlPoints.size() );

		// First and last points should match original control points
		REQUIRE( approxEqual( smoothCurve.front().x, controlPoints.front().x ) );
		REQUIRE( approxEqual( smoothCurve.front().y, controlPoints.front().y ) );
		REQUIRE( approxEqual( smoothCurve.back().x, controlPoints.back().x ) );
		REQUIRE( approxEqual( smoothCurve.back().y, controlPoints.back().y ) );

		// Test 3D curve generation
		const std::vector<math::Vec3<>> controlPoints3d = {
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 2.0f, 0.0f, 2.0f }
		};

		const auto smoothCurve3d = generateSmoothCurve( controlPoints3d, 4 );
		REQUIRE( smoothCurve3d.size() > controlPoints3d.size() );
	}

	SECTION( "Edge Cases and Robustness" )
	{
		// Test Bezier with identical points
		const math::Vec2<> p{ 1.0f, 1.0f };
		const auto identical = cubicBezier( p, p, p, p, 0.5f );
		REQUIRE( approxEqual( identical.x, p.x ) );
		REQUIRE( approxEqual( identical.y, p.y ) );

		// Test Catmull-Rom with identical points
		const auto identicalSpline = catmullRom( p, p, p, p, 0.5f );
		REQUIRE( approxEqual( identicalSpline.x, p.x ) );
		REQUIRE( approxEqual( identicalSpline.y, p.y ) );

		// Test arc length with single point
		const std::vector<math::Vec2<>> singlePoint = { { 5.0f, 3.0f } };
		const auto singleLength = arcLength( singlePoint );
		REQUIRE( approxEqual( singleLength, 0.0f ) );

		// Test arc length with empty vector
		const std::vector<math::Vec2<>> empty;
		const auto emptyLength = arcLength( empty );
		REQUIRE( approxEqual( emptyLength, 0.0f ) );

		// Test boundary conditions
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 1.0f, 1.0f };

		const auto t_negative = linearBezier( p0, p1, -0.5f );
		const auto t_beyond = linearBezier( p0, p1, 1.5f );

		// Should extrapolate linearly
		REQUIRE( t_negative.x < 0.0f );
		REQUIRE( t_beyond.x > 1.0f );

		// Test closest point with empty curve
		const std::vector<math::Vec3<>> emptyCurve3d;
		const math::Vec3<> queryPoint3d{ 1.0f, 1.0f, 1.0f };
		math::Vec3<> closest3d;
		int index3d;

		const auto dist3d = findClosestPointOnCurve( emptyCurve3d, queryPoint3d, closest3d, index3d );
		REQUIRE( std::isinf( dist3d ) );
		REQUIRE( index3d == -1 );
	}

	SECTION( "Mathematical Properties" )
	{
		// Test Bezier curve convex hull property
		const math::Vec2<> p0{ 0.0f, 0.0f };
		const math::Vec2<> p1{ 0.0f, 2.0f };
		const math::Vec2<> p2{ 2.0f, 2.0f };
		const math::Vec2<> p3{ 2.0f, 0.0f };

		// Sample multiple points on cubic Bezier
		for ( int i = 0; i <= 10; ++i )
		{
			const float t = static_cast<float>( i ) / 10.0f;
			const auto point = cubicBezier( p0, p1, p2, p3, t );

			// Point should be within the convex hull [0,2] x [0,2]
			REQUIRE( point.x >= -0.001f ); // Small tolerance for floating point
			REQUIRE( point.x <= 2.001f );
			REQUIRE( point.y >= -0.001f );
			REQUIRE( point.y <= 2.001f );
		}

		// Test Catmull-Rom spline passes through intermediate points
		const math::Vec2<> cp0{ -1.0f, 0.0f };
		const math::Vec2<> cp1{ 0.0f, 1.0f };
		const math::Vec2<> cp2{ 1.0f, 0.0f };
		const math::Vec2<> cp3{ 2.0f, -1.0f };

		const auto splineStart = catmullRom( cp0, cp1, cp2, cp3, 0.0f );
		const auto splineEnd = catmullRom( cp0, cp1, cp2, cp3, 1.0f );

		REQUIRE( approxEqual( splineStart.x, cp1.x ) );
		REQUIRE( approxEqual( splineStart.y, cp1.y ) );
		REQUIRE( approxEqual( splineEnd.x, cp2.x ) );
		REQUIRE( approxEqual( splineEnd.y, cp2.y ) );

		// Test derivative continuity for smooth curves
		const auto deriv1 = quadraticBezierDerivative( p0, p1, p2, 0.3f );
		const auto deriv2 = quadraticBezierDerivative( p0, p1, p2, 0.301f );

		// Small change in t should produce small change in derivative
		const auto derivDiff = length( deriv2 - deriv1 );
		REQUIRE( derivDiff < 0.1f ); // Reasonable continuity bound
	}
}

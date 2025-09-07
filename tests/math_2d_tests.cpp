#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <cmath>
import engine.vec;
import engine.math_2d;

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

// Test BoundingBox2D structure
TEST_CASE( "BoundingBox2D functionality", "[math][2d][bounding_box]" )
{
	SECTION( "Construction and basic properties" )
	{
		const math::BoundingBox2D<float> box( math::Vec2f( 1.0f, 2.0f ), math::Vec2f( 5.0f, 6.0f ) );

		REQUIRE( box.min.x == 1.0f );
		REQUIRE( box.min.y == 2.0f );
		REQUIRE( box.max.x == 5.0f );
		REQUIRE( box.max.y == 6.0f );

		REQUIRE( box.isValid() );
	}

	SECTION( "contains function" )
	{
		const math::BoundingBox2D<float> box( math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 10.0f, 10.0f ) );

		// Points inside
		REQUIRE( box.contains( math::Vec2f( 5.0f, 5.0f ) ) );
		REQUIRE( box.contains( math::Vec2f( 0.0f, 0.0f ) ) );	// Corner
		REQUIRE( box.contains( math::Vec2f( 10.0f, 10.0f ) ) ); // Corner
		REQUIRE( box.contains( math::Vec2f( 0.0f, 5.0f ) ) );	// Edge

		// Points outside
		REQUIRE_FALSE( box.contains( math::Vec2f( -1.0f, 5.0f ) ) );
		REQUIRE_FALSE( box.contains( math::Vec2f( 11.0f, 5.0f ) ) );
		REQUIRE_FALSE( box.contains( math::Vec2f( 5.0f, -1.0f ) ) );
		REQUIRE_FALSE( box.contains( math::Vec2f( 5.0f, 11.0f ) ) );
	}

	SECTION( "intersects function" )
	{
		const math::BoundingBox2D<float> box1( math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 5.0f, 5.0f ) );
		const math::BoundingBox2D<float> box2( math::Vec2f( 3.0f, 3.0f ), math::Vec2f( 8.0f, 8.0f ) );	   // Overlapping
		const math::BoundingBox2D<float> box3( math::Vec2f( 10.0f, 10.0f ), math::Vec2f( 15.0f, 15.0f ) ); // Non-overlapping
		const math::BoundingBox2D<float> box4( math::Vec2f( 5.0f, 0.0f ), math::Vec2f( 10.0f, 5.0f ) );	   // Edge touching

		REQUIRE( box1.intersects( box2 ) );
		REQUIRE( box2.intersects( box1 ) ); // Commutative
		REQUIRE_FALSE( box1.intersects( box3 ) );
		REQUIRE( box1.intersects( box4 ) ); // Edge touching counts as intersection
	}

	SECTION( "expand function" )
	{
		math::BoundingBox2D<float> box( math::Vec2f( 2.0f, 2.0f ), math::Vec2f( 4.0f, 4.0f ) );

		// Expand with point inside (no change)
		box.expand( math::Vec2f( 3.0f, 3.0f ) );
		REQUIRE( box.min.x == 2.0f );
		REQUIRE( box.max.x == 4.0f );

		// Expand with point outside (should expand)
		box.expand( math::Vec2f( 1.0f, 5.0f ) );
		REQUIRE( box.min.x == 1.0f );
		REQUIRE( box.min.y == 2.0f );
		REQUIRE( box.max.x == 4.0f );
		REQUIRE( box.max.y == 5.0f );
	}

	SECTION( "utility functions" )
	{
		const math::BoundingBox2D<float> box( math::Vec2f( 1.0f, 2.0f ), math::Vec2f( 5.0f, 8.0f ) );

		// Center
		const auto center = box.center();
		REQUIRE_THAT( center.x, WithinAbs( 3.0f, 1e-6f ) );
		REQUIRE_THAT( center.y, WithinAbs( 5.0f, 1e-6f ) );

		// Size
		const auto size = box.size();
		REQUIRE_THAT( size.x, WithinAbs( 4.0f, 1e-6f ) );
		REQUIRE_THAT( size.y, WithinAbs( 6.0f, 1e-6f ) );

		// Area
		REQUIRE_THAT( box.area(), WithinAbs( 24.0f, 1e-6f ) );
	}

	SECTION( "validity check" )
	{
		const math::BoundingBox2D<float> validBox( math::Vec2f( 1.0f, 1.0f ), math::Vec2f( 5.0f, 5.0f ) );
		const math::BoundingBox2D<float> invalidBox( math::Vec2f( 5.0f, 5.0f ), math::Vec2f( 1.0f, 1.0f ) );

		REQUIRE( validBox.isValid() );
		REQUIRE_FALSE( invalidBox.isValid() );
	}

	SECTION( "default constructor creates invalid bounds" )
	{
		const math::BoundingBox2D<float> defaultBox;
		REQUIRE_FALSE( defaultBox.isValid() );
		// Default initialization should create min > max for easy expansion
		REQUIRE( defaultBox.min.x > defaultBox.max.x );
		REQUIRE( defaultBox.min.y > defaultBox.max.y );
	}
}

TEST_CASE( "Point-in-shape tests", "[math][2d][point_tests]" )
{
	SECTION( "pointInCircle function" )
	{
		const math::Vec2f center( 5.0f, 5.0f );
		const float radius = 3.0f;

		// Points inside
		REQUIRE( math::pointInCircle( math::Vec2f( 5.0f, 5.0f ), center, radius ) ); // Center
		REQUIRE( math::pointInCircle( math::Vec2f( 7.0f, 5.0f ), center, radius ) ); // Inside
		REQUIRE( math::pointInCircle( math::Vec2f( 8.0f, 5.0f ), center, radius ) ); // On edge

		// Points outside
		REQUIRE_FALSE( math::pointInCircle( math::Vec2f( 9.0f, 5.0f ), center, radius ) );
		REQUIRE_FALSE( math::pointInCircle( math::Vec2f( 5.0f, 9.0f ), center, radius ) );

		// Edge cases
		REQUIRE_FALSE( math::pointInCircle( math::Vec2f( 5.0f, 5.0f ), center, -1.0f ) ); // Negative radius
		REQUIRE_FALSE( math::pointInCircle( math::Vec2f( 5.0f, 5.0f ), center, 0.0f ) );  // Zero radius
	}

	SECTION( "pointInRect function" )
	{
		const math::Vec2f min( 1.0f, 2.0f );
		const math::Vec2f max( 6.0f, 8.0f );

		// Points inside
		REQUIRE( math::pointInRect( math::Vec2f( 3.0f, 5.0f ), min, max ) );
		REQUIRE( math::pointInRect( math::Vec2f( 1.0f, 2.0f ), min, max ) ); // Corner
		REQUIRE( math::pointInRect( math::Vec2f( 6.0f, 8.0f ), min, max ) ); // Corner
		REQUIRE( math::pointInRect( math::Vec2f( 3.0f, 2.0f ), min, max ) ); // Edge

		// Points outside
		REQUIRE_FALSE( math::pointInRect( math::Vec2f( 0.0f, 5.0f ), min, max ) );
		REQUIRE_FALSE( math::pointInRect( math::Vec2f( 7.0f, 5.0f ), min, max ) );
		REQUIRE_FALSE( math::pointInRect( math::Vec2f( 3.0f, 1.0f ), min, max ) );
		REQUIRE_FALSE( math::pointInRect( math::Vec2f( 3.0f, 9.0f ), min, max ) );
	}

	SECTION( "pointInTriangle function" )
	{
		const math::Vec2f a( 0.0f, 0.0f );
		const math::Vec2f b( 4.0f, 0.0f );
		const math::Vec2f c( 2.0f, 3.0f );

		// Points inside
		REQUIRE( math::pointInTriangle( math::Vec2f( 2.0f, 1.0f ), a, b, c ) ); // Center area
		REQUIRE( math::pointInTriangle( math::Vec2f( 1.0f, 0.5f ), a, b, c ) ); // Inside

		// Points on vertices (should be inside)
		REQUIRE( math::pointInTriangle( a, a, b, c ) );
		REQUIRE( math::pointInTriangle( b, a, b, c ) );
		REQUIRE( math::pointInTriangle( c, a, b, c ) );

		// Points outside
		REQUIRE_FALSE( math::pointInTriangle( math::Vec2f( -1.0f, 0.0f ), a, b, c ) );
		REQUIRE_FALSE( math::pointInTriangle( math::Vec2f( 2.0f, 4.0f ), a, b, c ) );
		REQUIRE_FALSE( math::pointInTriangle( math::Vec2f( 5.0f, 1.0f ), a, b, c ) );
	}

	SECTION( "pointInPolygon function" )
	{
		// Square polygon
		const std::vector<math::Vec2f> square = {
			{ 0.0f, 0.0f }, { 4.0f, 0.0f }, { 4.0f, 4.0f }, { 0.0f, 4.0f }
		};

		// Points inside
		REQUIRE( math::pointInPolygon( math::Vec2f( 2.0f, 2.0f ), square ) );
		REQUIRE( math::pointInPolygon( math::Vec2f( 1.0f, 1.0f ), square ) );

		// Points outside
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( -1.0f, 2.0f ), square ) );
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( 5.0f, 2.0f ), square ) );
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( 2.0f, -1.0f ), square ) );
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( 2.0f, 5.0f ), square ) );

		// Edge cases
		const std::vector<math::Vec2f> triangle = { { 0.0f, 0.0f }, { 2.0f, 0.0f } }; // Less than 3 points
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( 1.0f, 0.0f ), triangle ) );

		const std::vector<math::Vec2f> empty;
		REQUIRE_FALSE( math::pointInPolygon( math::Vec2f( 0.0f, 0.0f ), empty ) );
	}
}

TEST_CASE( "Line and ray intersections", "[math][2d][intersections]" )
{
	SECTION( "lineLineIntersection function" )
	{
		math::Vec2f intersection;

		// Normal intersection
		const bool result1 = math::lineLineIntersection(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 2.0f, 2.0f ), math::Vec2f( 0.0f, 2.0f ), math::Vec2f( 2.0f, 0.0f ), intersection );

		REQUIRE( result1 );
		REQUIRE_THAT( intersection.x, WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( intersection.y, WithinAbs( 1.0f, 1e-6f ) );

		// Parallel lines (no intersection)
		const bool result2 = math::lineLineIntersection(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 2.0f, 0.0f ), math::Vec2f( 0.0f, 1.0f ), math::Vec2f( 2.0f, 1.0f ), intersection );

		REQUIRE_FALSE( result2 );

		// Vertical and horizontal lines
		const bool result3 = math::lineLineIntersection(
			math::Vec2f( 1.0f, 0.0f ), math::Vec2f( 1.0f, 3.0f ), // Vertical
			math::Vec2f( 0.0f, 2.0f ),
			math::Vec2f( 3.0f, 2.0f ), // Horizontal
			intersection );

		REQUIRE( result3 );
		REQUIRE_THAT( intersection.x, WithinAbs( 1.0f, 1e-6f ) );
		REQUIRE_THAT( intersection.y, WithinAbs( 2.0f, 1e-6f ) );
	}

	SECTION( "rayCircleIntersection function" )
	{
		const math::Vec2f center( 5.0f, 5.0f );
		const float radius = 3.0f;

		// Ray pointing towards circle
		REQUIRE( math::rayCircleIntersection(
			math::Vec2f( 0.0f, 5.0f ), math::Vec2f( 1.0f, 0.0f ), center, radius ) );

		// Ray pointing away from circle
		REQUIRE_FALSE( math::rayCircleIntersection(
			math::Vec2f( 0.0f, 5.0f ), math::Vec2f( -1.0f, 0.0f ), center, radius ) );

		// Ray starting inside circle (pointing away)
		REQUIRE( math::rayCircleIntersection(
			math::Vec2f( 5.0f, 5.0f ), math::Vec2f( -1.0f, 0.0f ), center, radius ) );

		// Ray missing circle
		REQUIRE_FALSE( math::rayCircleIntersection(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 1.0f, 0.0f ), center, radius ) );

		// Edge cases
		REQUIRE_FALSE( math::rayCircleIntersection(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 1.0f, 0.0f ), center, -1.0f // Negative radius
			) );
	}

	SECTION( "segmentCircleIntersection function" )
	{
		const math::Vec2f center( 5.0f, 5.0f );
		const float radius = 2.0f;

		// Segment intersecting circle
		REQUIRE( math::segmentCircleIntersection(
			math::Vec2f( 1.0f, 5.0f ), math::Vec2f( 9.0f, 5.0f ), center, radius ) );

		// Segment missing circle
		REQUIRE_FALSE( math::segmentCircleIntersection(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 2.0f, 0.0f ), center, radius ) );

		// Segment entirely inside circle
		REQUIRE( math::segmentCircleIntersection(
			math::Vec2f( 4.0f, 5.0f ), math::Vec2f( 6.0f, 5.0f ), center, radius ) );

		// Degenerate segment (point)
		REQUIRE( math::segmentCircleIntersection(
			math::Vec2f( 5.0f, 5.0f ), math::Vec2f( 5.0f, 5.0f ), center, radius ) );

		REQUIRE_FALSE( math::segmentCircleIntersection(
			math::Vec2f( 10.0f, 10.0f ), math::Vec2f( 10.0f, 10.0f ), center, radius ) );
	}
}

TEST_CASE( "Distance functions", "[math][2d][distance]" )
{
	SECTION( "distancePointToLine function" )
	{
		// Point to horizontal line
		const float dist1 = math::distancePointToLine(
			math::Vec2f( 5.0f, 8.0f ),
			math::Vec2f( 0.0f, 3.0f ),
			math::Vec2f( 10.0f, 3.0f ) );
		REQUIRE_THAT( dist1, WithinAbs( 5.0f, 1e-6f ) );

		// Point to vertical line
		const float dist2 = math::distancePointToLine(
			math::Vec2f( 8.0f, 5.0f ),
			math::Vec2f( 3.0f, 0.0f ),
			math::Vec2f( 3.0f, 10.0f ) );
		REQUIRE_THAT( dist2, WithinAbs( 5.0f, 1e-6f ) );

		// Point to diagonal line
		const float dist3 = math::distancePointToLine(
			math::Vec2f( 0.0f, 2.0f ),
			math::Vec2f( 0.0f, 0.0f ),
			math::Vec2f( 2.0f, 2.0f ) );
		REQUIRE_THAT( dist3, WithinAbs( std::sqrt( 2.0f ), 1e-5f ) );

		// Degenerate line (point)
		const float dist4 = math::distancePointToLine(
			math::Vec2f( 3.0f, 4.0f ),
			math::Vec2f( 0.0f, 0.0f ),
			math::Vec2f( 0.0f, 0.0f ) );
		REQUIRE_THAT( dist4, WithinAbs( 5.0f, 1e-6f ) );
	}

	SECTION( "distancePointToSegment function" )
	{
		// Point closest to middle of segment
		const float dist1 = math::distancePointToSegment(
			math::Vec2f( 5.0f, 8.0f ),
			math::Vec2f( 0.0f, 3.0f ),
			math::Vec2f( 10.0f, 3.0f ) );
		REQUIRE_THAT( dist1, WithinAbs( 5.0f, 1e-6f ) );

		// Point closest to start of segment
		const float dist2 = math::distancePointToSegment(
			math::Vec2f( -2.0f, 8.0f ),
			math::Vec2f( 0.0f, 3.0f ),
			math::Vec2f( 10.0f, 3.0f ) );
		REQUIRE_THAT( dist2, WithinAbs( std::sqrt( 29.0f ), 1e-5f ) ); // sqrt(2^2 + 5^2)

		// Point closest to end of segment
		const float dist3 = math::distancePointToSegment(
			math::Vec2f( 12.0f, 8.0f ),
			math::Vec2f( 0.0f, 3.0f ),
			math::Vec2f( 10.0f, 3.0f ) );
		REQUIRE_THAT( dist3, WithinAbs( std::sqrt( 29.0f ), 1e-5f ) ); // sqrt(2^2 + 5^2)

		// Degenerate segment (point)
		const float dist4 = math::distancePointToSegment(
			math::Vec2f( 3.0f, 4.0f ),
			math::Vec2f( 0.0f, 0.0f ),
			math::Vec2f( 0.0f, 0.0f ) );
		REQUIRE_THAT( dist4, WithinAbs( 5.0f, 1e-6f ) );
	}
}

TEST_CASE( "Utility functions", "[math][2d][utilities]" )
{
	SECTION( "triangleArea function" )
	{
		// Right triangle
		const float area1 = math::triangleArea(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 3.0f, 0.0f ), math::Vec2f( 0.0f, 4.0f ) );
		REQUIRE_THAT( area1, WithinAbs( 6.0f, 1e-6f ) );

		// Equilateral triangle (approximately)
		const float side = 2.0f;
		const float height = side * std::sqrt( 3.0f ) * 0.5f;
		const float area2 = math::triangleArea(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( side, 0.0f ), math::Vec2f( side * 0.5f, height ) );
		const float expectedArea = side * height * 0.5f;
		REQUIRE_THAT( area2, WithinAbs( expectedArea, 1e-5f ) );

		// Degenerate triangle (collinear points)
		const float area3 = math::triangleArea(
			math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 1.0f, 1.0f ), math::Vec2f( 2.0f, 2.0f ) );
		REQUIRE_THAT( area3, WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "polygonArea function" )
	{
		// Square
		const std::vector<math::Vec2<float>> square = {
			{ 0.0f, 0.0f }, { 2.0f, 0.0f }, { 2.0f, 2.0f }, { 0.0f, 2.0f }
		};
		REQUIRE_THAT( math::polygonArea( square ), WithinAbs( 4.0f, 1e-6f ) );

		// Triangle
		const std::vector<math::Vec2<float>> triangle = {
			{ 0.0f, 0.0f }, { 4.0f, 0.0f }, { 2.0f, 3.0f }
		};
		REQUIRE_THAT( math::polygonArea( triangle ), WithinAbs( 6.0f, 1e-6f ) );

		// Edge cases
		const std::vector<math::Vec2<float>> line = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
		REQUIRE_THAT( math::polygonArea( line ), WithinAbs( 0.0f, 1e-6f ) );

		const std::vector<math::Vec2<float>> empty;
		REQUIRE_THAT( math::polygonArea( empty ), WithinAbs( 0.0f, 1e-6f ) );
	}

	SECTION( "isPolygonConvex function" )
	{
		// Convex square
		const std::vector<math::Vec2<float>> square = {
			{ 0.0f, 0.0f }, { 2.0f, 0.0f }, { 2.0f, 2.0f }, { 0.0f, 2.0f }
		};
		REQUIRE( math::isPolygonConvex( square ) );

		// Convex triangle
		const std::vector<math::Vec2<float>> triangle = {
			{ 0.0f, 0.0f }, { 4.0f, 0.0f }, { 2.0f, 3.0f }
		};
		REQUIRE( math::isPolygonConvex( triangle ) );

		// Non-convex polygon (L-shape)
		const std::vector<math::Vec2<float>> lShape = {
			{ 0.0f, 0.0f }, { 2.0f, 0.0f }, { 2.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 2.0f }, { 0.0f, 2.0f }
		};
		REQUIRE_FALSE( math::isPolygonConvex( lShape ) );

		// Edge cases
		const std::vector<math::Vec2<float>> line = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
		REQUIRE_FALSE( math::isPolygonConvex( line ) );

		const std::vector<math::Vec2<float>> empty;
		REQUIRE_FALSE( math::isPolygonConvex( empty ) );
	}
}

TEST_CASE( "2D Geometry const-correctness", "[math][2d][const]" )
{
	SECTION( "All 2D geometry functions are const-correct" )
	{
		// Const parameters for testing
		const math::Vec2f constPoint( 2.0f, 3.0f );
		const math::Vec2f constCenter( 5.0f, 5.0f );
		const float constRadius = 2.0f;
		const math::Vec2f constMin( 0.0f, 0.0f );
		const math::Vec2f constMax( 10.0f, 10.0f );
		const math::Vec2f constA( 0.0f, 0.0f );
		const math::Vec2f constB( 5.0f, 0.0f );
		const math::Vec2f constC( 2.5f, 4.0f );
		const std::vector<math::Vec2<float>> constPolygon = {
			{ 0.0f, 0.0f }, { 5.0f, 0.0f }, { 5.0f, 5.0f }, { 0.0f, 5.0f }
		};
		const math::BoundingBox2D<float> constBox( constMin, constMax );

		// Test that all functions can be called with const parameters
		const bool inCircle = math::pointInCircle( constPoint, constCenter, constRadius );
		const bool inRect = math::pointInRect( constPoint, constMin, constMax );
		const bool inTriangle = math::pointInTriangle( constPoint, constA, constB, constC );
		const bool inPolygon = math::pointInPolygon( constPoint, constPolygon );

		math::Vec2f intersection;
		const bool lineIntersection = math::lineLineIntersection( constA, constB, constC, constPoint, intersection );
		const bool rayIntersection = math::rayCircleIntersection( constPoint, constA, constCenter, constRadius );
		const bool segmentIntersection = math::segmentCircleIntersection( constA, constB, constCenter, constRadius );

		const float distToLine = math::distancePointToLine( constPoint, constA, constB );
		const float distToSegment = math::distancePointToSegment( constPoint, constA, constB );

		const float triArea = math::triangleArea( constA, constB, constC );
		const float polyArea = math::polygonArea( constPolygon );
		const bool isConvex = math::isPolygonConvex( constPolygon );

		const bool boxContains = constBox.contains( constPoint );
		const math::Vec2f boxCenter = constBox.center();
		const math::Vec2f boxSize = constBox.size();
		const float boxArea = constBox.area();
		const bool boxValid = constBox.isValid();

		// Verify functions produce reasonable values
		REQUIRE( ( inCircle == true || inCircle == false ) );
		REQUIRE( ( inRect == true || inRect == false ) );
		REQUIRE( ( inTriangle == true || inTriangle == false ) );
		REQUIRE( ( inPolygon == true || inPolygon == false ) );
		REQUIRE( ( lineIntersection == true || lineIntersection == false ) );
		REQUIRE( ( rayIntersection == true || rayIntersection == false ) );
		REQUIRE( ( segmentIntersection == true || segmentIntersection == false ) );
		REQUIRE( distToLine >= 0.0f );
		REQUIRE( distToSegment >= 0.0f );
		REQUIRE( triArea >= 0.0f );
		REQUIRE( polyArea >= 0.0f );
		REQUIRE( ( isConvex == true || isConvex == false ) );
		REQUIRE( ( boxContains == true || boxContains == false ) );
		REQUIRE( boxCenter.x > 0.0f );
		REQUIRE( boxSize.x > 0.0f );
		REQUIRE( boxArea > 0.0f );
		REQUIRE( boxValid );
	}
}

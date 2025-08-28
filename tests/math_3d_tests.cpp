#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

import engine.math_3d;
import engine.bounding_volumes;
import engine.vec;
import engine.math;

using namespace math;

// Type aliases for convenience
using Vec3f = Vec3<float>;
using BoundingBox3Df = BoundingBox3D<float>;
using BoundingSpheref = BoundingSphere<float>;
using Planef = Plane<float>;
using Frustumf = Frustum<float>;

TEST_CASE( "3D Point-in-Shape Tests", "[3d][geometry]" )
{
	SECTION( "Point in Sphere" )
	{
		Vec3f center( 0.0f, 0.0f, 0.0f );
		float radius = 5.0f;

		REQUIRE( pointInSphere( Vec3f( 0.0f, 0.0f, 0.0f ), center, radius ) );		 // Center
		REQUIRE( pointInSphere( Vec3f( 3.0f, 0.0f, 0.0f ), center, radius ) );		 // Inside
		REQUIRE( pointInSphere( Vec3f( 5.0f, 0.0f, 0.0f ), center, radius ) );		 // On surface
		REQUIRE_FALSE( pointInSphere( Vec3f( 6.0f, 0.0f, 0.0f ), center, radius ) ); // Outside
		REQUIRE_FALSE( pointInSphere( Vec3f( 4.0f, 4.0f, 0.0f ), center, radius ) ); // Outside diagonal
	}

	SECTION( "Point in AABB" )
	{
		Vec3f min( -2.0f, -3.0f, -1.0f );
		Vec3f max( 2.0f, 3.0f, 1.0f );

		REQUIRE( pointInAABB( Vec3f( 0.0f, 0.0f, 0.0f ), min, max ) );		  // Center
		REQUIRE( pointInAABB( Vec3f( -2.0f, -3.0f, -1.0f ), min, max ) );	  // Min corner
		REQUIRE( pointInAABB( Vec3f( 2.0f, 3.0f, 1.0f ), min, max ) );		  // Max corner
		REQUIRE_FALSE( pointInAABB( Vec3f( -3.0f, 0.0f, 0.0f ), min, max ) ); // Outside X
		REQUIRE_FALSE( pointInAABB( Vec3f( 0.0f, 4.0f, 0.0f ), min, max ) );  // Outside Y
		REQUIRE_FALSE( pointInAABB( Vec3f( 0.0f, 0.0f, 2.0f ), min, max ) );  // Outside Z
	}

	SECTION( "Point in Tetrahedron" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );
		Vec3f d( 0.0f, 0.0f, 1.0f );

		REQUIRE( pointInTetrahedron( Vec3f( 0.1f, 0.1f, 0.1f ), a, b, c, d ) );		   // Inside
		REQUIRE( pointInTetrahedron( Vec3f( 0.0f, 0.0f, 0.0f ), a, b, c, d ) );		   // On vertex
		REQUIRE_FALSE( pointInTetrahedron( Vec3f( 0.5f, 0.5f, 0.5f ), a, b, c, d ) );  // Outside
		REQUIRE_FALSE( pointInTetrahedron( Vec3f( -0.1f, 0.1f, 0.1f ), a, b, c, d ) ); // Outside
	}
}

TEST_CASE( "3D Ray-Shape Intersections", "[3d][geometry][ray]" )
{
	SECTION( "Ray-Sphere Intersection" )
	{
		Vec3f center( 0.0f, 0.0f, 0.0f );
		float radius = 2.0f;
		float hitDistance;

		// Ray from outside hitting sphere
		REQUIRE( raySphereIntersection( Vec3f( -5.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), center, radius, hitDistance ) );
		REQUIRE( hitDistance == Catch::Approx( 3.0f ) );

		// Ray missing sphere
		REQUIRE_FALSE( raySphereIntersection( Vec3f( -5.0f, 3.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), center, radius, hitDistance ) );

		// Ray from inside sphere
		REQUIRE( raySphereIntersection( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), center, radius, hitDistance ) );
		REQUIRE( hitDistance == Catch::Approx( 2.0f ) );

		// Ray going away from sphere
		REQUIRE_FALSE( raySphereIntersection( Vec3f( -5.0f, 0.0f, 0.0f ), Vec3f( -1.0f, 0.0f, 0.0f ), center, radius, hitDistance ) );
	}

	SECTION( "Ray-AABB Intersection" )
	{
		Vec3f min( -1.0f, -1.0f, -1.0f );
		Vec3f max( 1.0f, 1.0f, 1.0f );
		float hitDistance;

		// Ray hitting box from outside
		REQUIRE( rayAABBIntersection( Vec3f( -2.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), min, max, hitDistance ) );
		REQUIRE( hitDistance == Catch::Approx( 1.0f ) );

		// Ray missing box
		REQUIRE_FALSE( rayAABBIntersection( Vec3f( -2.0f, 2.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), min, max, hitDistance ) );

		// Ray from inside box
		REQUIRE( rayAABBIntersection( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), min, max, hitDistance ) );
		REQUIRE( hitDistance == Catch::Approx( 0.0f ) );

		// Ray parallel to box face
		REQUIRE_FALSE( rayAABBIntersection( Vec3f( -2.0f, 0.0f, 0.0f ), Vec3f( 0.0f, 1.0f, 0.0f ), min, max, hitDistance ) );
	}

	SECTION( "Ray-Triangle Intersection" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );
		Vec3f hitPoint;

		// Ray hitting triangle
		REQUIRE( rayTriangleIntersection( Vec3f( 0.25f, 0.25f, -1.0f ), Vec3f( 0.0f, 0.0f, 1.0f ), a, b, c, hitPoint ) );
		REQUIRE( hitPoint.x == Catch::Approx( 0.25f ) );
		REQUIRE( hitPoint.y == Catch::Approx( 0.25f ) );
		REQUIRE( hitPoint.z == Catch::Approx( 0.0f ) );

		// Ray missing triangle
		REQUIRE_FALSE( rayTriangleIntersection( Vec3f( 1.0f, 1.0f, -1.0f ), Vec3f( 0.0f, 0.0f, 1.0f ), a, b, c, hitPoint ) );

		// Ray parallel to triangle
		REQUIRE_FALSE( rayTriangleIntersection( Vec3f( 0.25f, 0.25f, -1.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), a, b, c, hitPoint ) );
	}

	SECTION( "Ray-Plane Intersection" )
	{
		Vec3f planePoint( 0.0f, 0.0f, 0.0f );
		Vec3f planeNormal( 0.0f, 0.0f, 1.0f );
		float hitDistance;

		// Ray hitting plane
		REQUIRE( rayPlaneIntersection( Vec3f( 1.0f, 1.0f, -2.0f ), Vec3f( 0.0f, 0.0f, 1.0f ), planePoint, planeNormal, hitDistance ) );
		REQUIRE( hitDistance == Catch::Approx( 2.0f ) );

		// Ray parallel to plane
		REQUIRE_FALSE( rayPlaneIntersection( Vec3f( 1.0f, 1.0f, -2.0f ), Vec3f( 1.0f, 0.0f, 0.0f ), planePoint, planeNormal, hitDistance ) );

		// Ray going away from plane
		REQUIRE_FALSE( rayPlaneIntersection( Vec3f( 1.0f, 1.0f, -2.0f ), Vec3f( 0.0f, 0.0f, -1.0f ), planePoint, planeNormal, hitDistance ) );
	}
}

TEST_CASE( "3D Distance Calculations", "[3d][geometry][distance]" )
{
	SECTION( "Distance Point to Plane" )
	{
		Vec3f planePoint( 0.0f, 0.0f, 0.0f );
		Vec3f planeNormal( 0.0f, 0.0f, 1.0f );

		REQUIRE( distancePointToPlane( Vec3f( 1.0f, 1.0f, 3.0f ), planePoint, planeNormal ) == Catch::Approx( 3.0f ) );
		REQUIRE( distancePointToPlane( Vec3f( 1.0f, 1.0f, -2.0f ), planePoint, planeNormal ) == Catch::Approx( 2.0f ) );
		REQUIRE( distancePointToPlane( Vec3f( 1.0f, 1.0f, 0.0f ), planePoint, planeNormal ) == Catch::Approx( 0.0f ) );
	}

	SECTION( "Distance Point to Line 3D" )
	{
		Vec3f linePoint( 0.0f, 0.0f, 0.0f );
		Vec3f lineDirection( 1.0f, 0.0f, 0.0f );

		REQUIRE( distancePointToLine3D( Vec3f( 2.0f, 3.0f, 0.0f ), linePoint, lineDirection ) == Catch::Approx( 3.0f ) );
		REQUIRE( distancePointToLine3D( Vec3f( 2.0f, 0.0f, 4.0f ), linePoint, lineDirection ) == Catch::Approx( 4.0f ) );
		REQUIRE( distancePointToLine3D( Vec3f( 2.0f, 0.0f, 0.0f ), linePoint, lineDirection ) == Catch::Approx( 0.0f ) );
	}

	SECTION( "Distance Point to Segment 3D" )
	{
		Vec3f segmentStart( 0.0f, 0.0f, 0.0f );
		Vec3f segmentEnd( 4.0f, 0.0f, 0.0f );

		// Point closest to middle of segment
		REQUIRE( distancePointToSegment3D( Vec3f( 2.0f, 3.0f, 0.0f ), segmentStart, segmentEnd ) == Catch::Approx( 3.0f ) );

		// Point closest to start of segment
		REQUIRE( distancePointToSegment3D( Vec3f( -1.0f, 2.0f, 0.0f ), segmentStart, segmentEnd ) == Catch::Approx( std::sqrt( 5.0f ) ) );

		// Point closest to end of segment
		REQUIRE( distancePointToSegment3D( Vec3f( 5.0f, 2.0f, 0.0f ), segmentStart, segmentEnd ) == Catch::Approx( std::sqrt( 5.0f ) ) );

		// Point on segment
		REQUIRE( distancePointToSegment3D( Vec3f( 2.0f, 0.0f, 0.0f ), segmentStart, segmentEnd ) == Catch::Approx( 0.0f ) );
	}
}

TEST_CASE( "3D Geometric Calculations", "[3d][geometry]" )
{
	SECTION( "Tetrahedron Volume" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );
		Vec3f d( 0.0f, 0.0f, 1.0f );

		float volume = tetrahedronVolume( a, b, c, d );
		REQUIRE( volume == Catch::Approx( 1.0f / 6.0f ) ); // Unit tetrahedron volume
	}

	SECTION( "Triangle Area 3D" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );

		float area = triangleArea3D( a, b, c );
		REQUIRE( area == Catch::Approx( 0.5f ) ); // Right triangle with legs of length 1
	}

	SECTION( "Triangle Normal" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );

		Vec3f normal = triangleNormal( a, b, c );
		REQUIRE( normal.x == Catch::Approx( 0.0f ) );
		REQUIRE( normal.y == Catch::Approx( 0.0f ) );
		REQUIRE( normal.z == Catch::Approx( 1.0f ) );
	}

	SECTION( "Barycentric Coordinates 3D" )
	{
		Vec3f a( 0.0f, 0.0f, 0.0f );
		Vec3f b( 1.0f, 0.0f, 0.0f );
		Vec3f c( 0.0f, 1.0f, 0.0f );

		// Point at vertex a
		Vec3f bary = barycentric3D( a, a, b, c );
		REQUIRE( bary.x == Catch::Approx( 1.0f ) );
		REQUIRE( bary.y == Catch::Approx( 0.0f ) );
		REQUIRE( bary.z == Catch::Approx( 0.0f ) );

		// Point at center of triangle
		Vec3f center = Vec3f( 1.0f / 3.0f, 1.0f / 3.0f, 0.0f );
		bary = barycentric3D( center, a, b, c );
		REQUIRE( bary.x == Catch::Approx( 1.0f / 3.0f ) );
		REQUIRE( bary.y == Catch::Approx( 1.0f / 3.0f ) );
		REQUIRE( bary.z == Catch::Approx( 1.0f / 3.0f ) );
	}
}

TEST_CASE( "3D Bounding Volumes", "[3d][bounding]" )
{
	SECTION( "BoundingBox3D Operations" )
	{
		BoundingBox3D<float> box( Vec3f( -1.0f, -2.0f, -3.0f ), Vec3f( 1.0f, 2.0f, 3.0f ) );

		// Contains test
		REQUIRE( box.contains( Vec3f( 0.0f, 0.0f, 0.0f ) ) );
		REQUIRE( box.contains( Vec3f( -1.0f, -2.0f, -3.0f ) ) );
		REQUIRE_FALSE( box.contains( Vec3f( 2.0f, 0.0f, 0.0f ) ) );

		// Intersection test
		BoundingBox3D<float> other( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 2.0f, 3.0f, 4.0f ) );
		REQUIRE( box.intersects( other ) );

		BoundingBox3D<float> separate( Vec3f( 3.0f, 3.0f, 3.0f ), Vec3f( 5.0f, 5.0f, 5.0f ) );
		REQUIRE_FALSE( box.intersects( separate ) );

		// Sphere intersection
		REQUIRE( box.intersects( Vec3f( 0.0f, 0.0f, 0.0f ), 1.0f ) );
		REQUIRE_FALSE( box.intersects( Vec3f( 5.0f, 5.0f, 5.0f ), 1.0f ) );

		// Properties
		Vec3f center = box.center();
		REQUIRE( center.x == Catch::Approx( 0.0f ) );
		REQUIRE( center.y == Catch::Approx( 0.0f ) );
		REQUIRE( center.z == Catch::Approx( 0.0f ) );

		Vec3f size = box.size();
		REQUIRE( size.x == Catch::Approx( 2.0f ) );
		REQUIRE( size.y == Catch::Approx( 4.0f ) );
		REQUIRE( size.z == Catch::Approx( 6.0f ) );

		REQUIRE( box.volume() == Catch::Approx( 48.0f ) );

		// Corner test
		Vec3f corner0 = box.corner( 0 );
		REQUIRE( corner0.x == Catch::Approx( -1.0f ) );
		REQUIRE( corner0.y == Catch::Approx( -2.0f ) );
		REQUIRE( corner0.z == Catch::Approx( -3.0f ) );

		Vec3f corner7 = box.corner( 7 );
		REQUIRE( corner7.x == Catch::Approx( 1.0f ) );
		REQUIRE( corner7.y == Catch::Approx( 2.0f ) );
		REQUIRE( corner7.z == Catch::Approx( 3.0f ) );
	}

	SECTION( "BoundingSphere Operations" )
	{
		BoundingSphere<float> sphere( Vec3f( 0.0f, 0.0f, 0.0f ), 2.0f );

		// Contains test
		REQUIRE( sphere.contains( Vec3f( 0.0f, 0.0f, 0.0f ) ) );
		REQUIRE( sphere.contains( Vec3f( 1.0f, 1.0f, 0.0f ) ) );
		REQUIRE_FALSE( sphere.contains( Vec3f( 3.0f, 0.0f, 0.0f ) ) );

		// Intersection test
		BoundingSphere<float> other( Vec3f( 3.0f, 0.0f, 0.0f ), 2.0f );
		REQUIRE( sphere.intersects( other ) );

		BoundingSphere<float> separate( Vec3f( 10.0f, 0.0f, 0.0f ), 2.0f );
		REQUIRE_FALSE( sphere.intersects( separate ) );

		// Properties
		REQUIRE( sphere.surfaceArea() == Catch::Approx( 4.0f * math::pi<float> * 4.0f ) );
		REQUIRE( sphere.volume() == Catch::Approx( ( 4.0f / 3.0f ) * math::pi<float> * 8.0f ) );
	}

	SECTION( "Plane Operations" )
	{
		Plane<float> plane( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 0.0f, 0.0f, 1.0f ) );

		// Distance test
		REQUIRE( plane.distanceToPoint( Vec3f( 1.0f, 1.0f, 3.0f ) ) == Catch::Approx( 3.0f ) );
		REQUIRE( plane.distanceToPoint( Vec3f( 1.0f, 1.0f, -2.0f ) ) == Catch::Approx( -2.0f ) );

		// Closest point
		Vec3f closest = plane.closestPoint( Vec3f( 1.0f, 2.0f, 3.0f ) );
		REQUIRE( closest.x == Catch::Approx( 1.0f ) );
		REQUIRE( closest.y == Catch::Approx( 2.0f ) );
		REQUIRE( closest.z == Catch::Approx( 0.0f ) );

		// Point on plane test
		REQUIRE( plane.isPointOnPlane( Vec3f( 1.0f, 2.0f, 0.0f ) ) );
		REQUIRE_FALSE( plane.isPointOnPlane( Vec3f( 1.0f, 2.0f, 1.0f ) ) );
	}

	SECTION( "Frustum Operations" )
	{
		Frustum<float> frustum;
		// Set up a simple frustum (normally would be calculated from projection matrix)
		frustum.planes[0] = Plane<float>( Vec3f( -1.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 0.0f, 0.0f ) ); // Left
		frustum.planes[1] = Plane<float>( Vec3f( 1.0f, 0.0f, 0.0f ), Vec3f( -1.0f, 0.0f, 0.0f ) ); // Right
		frustum.planes[2] = Plane<float>( Vec3f( 0.0f, -1.0f, 0.0f ), Vec3f( 0.0f, 1.0f, 0.0f ) ); // Bottom
		frustum.planes[3] = Plane<float>( Vec3f( 0.0f, 1.0f, 0.0f ), Vec3f( 0.0f, -1.0f, 0.0f ) ); // Top
		frustum.planes[4] = Plane<float>( Vec3f( 0.0f, 0.0f, -1.0f ), Vec3f( 0.0f, 0.0f, 1.0f ) ); // Near
		frustum.planes[5] = Plane<float>( Vec3f( 0.0f, 0.0f, 1.0f ), Vec3f( 0.0f, 0.0f, -1.0f ) ); // Far

		// Point containment
		REQUIRE( frustum.contains( Vec3f( 0.0f, 0.0f, 0.0f ) ) );
		REQUIRE_FALSE( frustum.contains( Vec3f( 2.0f, 0.0f, 0.0f ) ) );

		// Bounding box intersection
		BoundingBox3D<float> insideBox( Vec3f( -0.5f, -0.5f, -0.5f ), Vec3f( 0.5f, 0.5f, 0.5f ) );
		REQUIRE( frustum.intersects( insideBox ) );

		BoundingBox3D<float> outsideBox( Vec3f( 2.0f, 2.0f, 2.0f ), Vec3f( 3.0f, 3.0f, 3.0f ) );
		REQUIRE_FALSE( frustum.intersects( outsideBox ) );
	}
}

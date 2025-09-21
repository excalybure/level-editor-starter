#include <iostream>
#include <vector>
#include "src/engine/math/vec.h"
#include "src/engine/math/math_2d.h"

int main()
{
	std::cout << "=== 2D Geometry Functions Demo ===\n\n";

	// BoundingBox2D demo
	std::cout << "1. BoundingBox2D Operations:\n";
	math::BoundingBox2Df box( math::Vec2f( 1.0f, 2.0f ), math::Vec2f( 5.0f, 6.0f ) );
	std::cout << "   Box: min=(" << box.min.x << "," << box.min.y << ") max=(" << box.max.x << "," << box.max.y << ")\n";
	std::cout << "   Center: (" << box.center().x << "," << box.center().y << ")\n";
	std::cout << "   Area: " << box.area() << "\n";
	std::cout << "   Contains (3,4): " << ( box.contains( math::Vec2f( 3.0f, 4.0f ) ) ? "yes" : "no" ) << "\n\n";

	// Point-in-shape tests
	std::cout << "2. Point-in-Shape Tests:\n";
	const math::Vec2f testPoint( 2.0f, 3.0f );
	std::cout << "   Test point: (" << testPoint.x << "," << testPoint.y << ")\n";

	// Circle test
	const math::Vec2f circleCenter( 2.5f, 3.5f );
	const float radius = 1.0f;
	std::cout << "   In circle (center: (" << circleCenter.x << "," << circleCenter.y << "), radius: " << radius << "): "
			  << ( math::pointInCircle( testPoint, circleCenter, radius ) ? "yes" : "no" ) << "\n";

	// Rectangle test
	const math::Vec2f rectMin( 1.0f, 2.0f );
	const math::Vec2f rectMax( 3.0f, 4.0f );
	std::cout << "   In rectangle (" << rectMin.x << "," << rectMin.y << ") to (" << rectMax.x << "," << rectMax.y << "): "
			  << ( math::pointInRect( testPoint, rectMin, rectMax ) ? "yes" : "no" ) << "\n";

	// Triangle test
	const math::Vec2f triA( 0.0f, 0.0f );
	const math::Vec2f triB( 4.0f, 0.0f );
	const math::Vec2f triC( 2.0f, 4.0f );
	std::cout << "   In triangle: " << ( math::pointInTriangle( testPoint, triA, triB, triC ) ? "yes" : "no" ) << "\n\n";

	// Line and ray intersections
	std::cout << "3. Intersection Tests:\n";

	// Line-line intersection
	math::Vec2f intersection;
	const bool intersects = math::lineLineIntersection(
		math::Vec2f( 0.0f, 0.0f ), math::Vec2f( 2.0f, 2.0f ), math::Vec2f( 0.0f, 2.0f ), math::Vec2f( 2.0f, 0.0f ), intersection );
	std::cout << "   Lines intersect: " << ( intersects ? "yes" : "no" );
	if ( intersects )
	{
		std::cout << " at (" << intersection.x << "," << intersection.y << ")";
	}
	std::cout << "\n";

	// Ray-circle intersection
	const bool rayHitsCircle = math::rayCircleIntersection(
		math::Vec2f( -1.0f, 3.5f ), // Ray origin
		math::Vec2f( 1.0f, 0.0f ),	// Ray direction
		circleCenter,				// Circle center
		radius						// Circle radius
	);
	std::cout << "   Ray hits circle: " << ( rayHitsCircle ? "yes" : "no" ) << "\n\n";

	// Distance calculations
	std::cout << "4. Distance Calculations:\n";
	const math::Vec2f point( 0.0f, 3.0f );
	const math::Vec2f lineStart( 1.0f, 1.0f );
	const math::Vec2f lineEnd( 3.0f, 1.0f );

	const float distToLine = math::distancePointToLine( point, lineStart, lineEnd );
	const float distToSegment = math::distancePointToSegment( point, lineStart, lineEnd );

	std::cout << "   Point (" << point.x << "," << point.y << ") to line: " << distToLine << "\n";
	std::cout << "   Point (" << point.x << "," << point.y << ") to segment: " << distToSegment << "\n\n";

	// Geometric calculations
	std::cout << "5. Geometric Calculations:\n";
	const float triangleArea = math::triangleArea( triA, triB, triC );
	std::cout << "   Triangle area: " << triangleArea << "\n";

	const std::vector<math::Vec2f> square = {
		{ 0.0f, 0.0f }, { 2.0f, 0.0f }, { 2.0f, 2.0f }, { 0.0f, 2.0f }
	};
	const float squareArea = math::polygonArea( square );
	const bool isConvex = math::isPolygonConvex( square );

	std::cout << "   Square area: " << squareArea << "\n";
	std::cout << "   Square is convex: " << ( isConvex ? "yes" : "no" ) << "\n\n";

	std::cout << "2D Geometry demo completed successfully!\n";
	return 0;
}

export module engine.math_2d;

import std;
import engine.math;
import engine.vec;
export import engine.bounding_box;

export namespace math
{

// ============================================================================
// POINT-IN-SHAPE TESTS
// ============================================================================

// Point in circle test
template <typename T>
constexpr bool pointInCircle( const Vec2<T> &point, const Vec2<T> &center, const T radius ) noexcept
{
	if ( radius <= static_cast<T>( 0 ) )
		return false;

	const Vec2<T> diff = point - center;
	const T distanceSquared = diff.x * diff.x + diff.y * diff.y;
	return distanceSquared <= radius * radius;
}

// Point in rectangle test
template <typename T>
constexpr bool pointInRect( const Vec2<T> &point, const Vec2<T> &min, const Vec2<T> &max ) noexcept
{
	return point.x >= min.x && point.x <= max.x &&
		point.y >= min.y && point.y <= max.y;
}

// Point in triangle test using barycentric coordinates
template <typename T>
constexpr bool pointInTriangle( const Vec2<T> &point, const Vec2<T> &a, const Vec2<T> &b, const Vec2<T> &c ) noexcept
{
	// Compute barycentric coordinates
	const Vec2<T> v0 = c - a;
	const Vec2<T> v1 = b - a;
	const Vec2<T> v2 = point - a;

	const T dot00 = v0.x * v0.x + v0.y * v0.y;
	const T dot01 = v0.x * v1.x + v0.y * v1.y;
	const T dot02 = v0.x * v2.x + v0.y * v2.y;
	const T dot11 = v1.x * v1.x + v1.y * v1.y;
	const T dot12 = v1.x * v2.x + v1.y * v2.y;

	// Compute barycentric coordinates
	const T invDenom = static_cast<T>( 1 ) / ( dot00 * dot11 - dot01 * dot01 );
	const T u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
	const T v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

	// Check if point is in triangle
	return ( u >= static_cast<T>( 0 ) ) && ( v >= static_cast<T>( 0 ) ) && ( u + v <= static_cast<T>( 1 ) );
}

// Point in polygon test using ray casting algorithm
template <typename T>
bool pointInPolygon( const Vec2<T> &point, const std::vector<Vec2<T>> &polygon ) noexcept
{
	if ( polygon.size() < 3 )
		return false;

	bool inside = false;
	const size_t numVertices = polygon.size();

	for ( size_t i = 0, j = numVertices - 1; i < numVertices; j = i++ )
	{
		const Vec2<T> &vi = polygon[i];
		const Vec2<T> &vj = polygon[j];

		if ( ( ( vi.y > point.y ) != ( vj.y > point.y ) ) &&
			( point.x < ( vj.x - vi.x ) * ( point.y - vi.y ) / ( vj.y - vi.y ) + vi.x ) )
		{
			inside = !inside;
		}
	}

	return inside;
}

// ============================================================================
// LINE AND RAY INTERSECTION FUNCTIONS
// ============================================================================

// Line-line intersection
template <typename T>
bool lineLineIntersection( const Vec2<T> &a1, const Vec2<T> &a2, const Vec2<T> &b1, const Vec2<T> &b2, Vec2<T> &intersection ) noexcept
{
	const Vec2<T> da = a2 - a1;
	const Vec2<T> db = b2 - b1;
	const Vec2<T> dc = b1 - a1;

	const T denominator = da.x * db.y - da.y * db.x;

	// Lines are parallel or coincident
	if ( math::abs( denominator ) < static_cast<T>( 1e-10 ) )
		return false;

	const T t = ( dc.x * db.y - dc.y * db.x ) / denominator;
	intersection = a1 + da * t;

	return true;
}

// Ray-circle intersection
template <typename T>
bool rayCircleIntersection( const Vec2<T> &origin, const Vec2<T> &direction, const Vec2<T> &center, const T radius ) noexcept
{
	if ( radius < static_cast<T>( 0 ) )
		return false;

	const Vec2<T> toCenter = center - origin;
	const T projectionLength = toCenter.x * direction.x + toCenter.y * direction.y;

	// Ray points away from circle
	if ( projectionLength < static_cast<T>( 0 ) )
	{
		// Check if origin is inside circle
		return pointInCircle( origin, center, radius );
	}

	const Vec2<T> closestPoint = origin + direction * projectionLength;
	const T distanceSquared = lengthSquared( closestPoint - center );

	return distanceSquared <= radius * radius;
}

// Segment-circle intersection (finite line segment)
template <typename T>
bool segmentCircleIntersection( const Vec2<T> &start, const Vec2<T> &end, const Vec2<T> &center, const T radius ) noexcept
{
	if ( radius < static_cast<T>( 0 ) )
		return false;

	const Vec2<T> segment = end - start;
	const Vec2<T> toCenter = center - start;

	const T segmentLengthSquared = lengthSquared( segment );

	// Degenerate segment (start == end)
	if ( segmentLengthSquared < static_cast<T>( 1e-10 ) )
		return pointInCircle( start, center, radius );

	const T projectionLength = ( toCenter.x * segment.x + toCenter.y * segment.y ) / segmentLengthSquared;
	const T clampedProjection = math::clamp( projectionLength, static_cast<T>( 0 ), static_cast<T>( 1 ) );

	const Vec2<T> closestPoint = start + segment * clampedProjection;
	return pointInCircle( closestPoint, center, radius );
}

// ============================================================================
// DISTANCE FUNCTIONS
// ============================================================================

// Distance from point to line (infinite line)
template <typename T>
T distancePointToLine( const Vec2<T> &point, const Vec2<T> &lineStart, const Vec2<T> &lineEnd ) noexcept
{
	const Vec2<T> line = lineEnd - lineStart;
	const T lineLengthSquared = lengthSquared( line );

	// Degenerate line (start == end)
	if ( lineLengthSquared < static_cast<T>( 1e-10 ) )
		return length( point - lineStart );

	const Vec2<T> toPoint = point - lineStart;
	const T cross = line.x * toPoint.y - line.y * toPoint.x;

	return math::abs( cross ) / math::sqrt( lineLengthSquared );
}

// Distance from point to line segment (finite line segment)
template <typename T>
T distancePointToSegment( const Vec2<T> &point, const Vec2<T> &segmentStart, const Vec2<T> &segmentEnd ) noexcept
{
	const Vec2<T> segment = segmentEnd - segmentStart;
	const Vec2<T> toPoint = point - segmentStart;

	const T segmentLengthSquared = lengthSquared( segment );

	// Degenerate segment (start == end)
	if ( segmentLengthSquared < static_cast<T>( 1e-10 ) )
		return length( point - segmentStart );

	const T projectionLength = ( toPoint.x * segment.x + toPoint.y * segment.y ) / segmentLengthSquared;
	const T clampedProjection = math::clamp( projectionLength, static_cast<T>( 0 ), static_cast<T>( 1 ) );

	const Vec2<T> closestPoint = segmentStart + segment * clampedProjection;
	return length( point - closestPoint );
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate the area of a triangle
template <typename T>
constexpr T triangleArea( const Vec2<T> &a, const Vec2<T> &b, const Vec2<T> &c ) noexcept
{
	// Using the cross product formula: |AB × AC| / 2
	const Vec2<T> ab = b - a;
	const Vec2<T> ac = c - a;
	const T cross = ab.x * ac.y - ab.y * ac.x;
	return math::abs( cross ) * static_cast<T>( 0.5 );
}

// Calculate the area of a polygon using the shoelace formula
template <typename T>
T polygonArea( const std::vector<Vec2<T>> &polygon ) noexcept
{
	if ( polygon.size() < 3 )
		return static_cast<T>( 0 );

	T area = static_cast<T>( 0 );
	const size_t numVertices = polygon.size();

	for ( size_t i = 0; i < numVertices; ++i )
	{
		const size_t j = ( i + 1 ) % numVertices;
		area += polygon[i].x * polygon[j].y;
		area -= polygon[j].x * polygon[i].y;
	}

	return math::abs( area ) * static_cast<T>( 0.5 );
}

// Check if a polygon is convex
template <typename T>
bool isPolygonConvex( const std::vector<Vec2<T>> &polygon ) noexcept
{
	if ( polygon.size() < 3 )
		return false;

	const size_t numVertices = polygon.size();
	bool signSet = false;
	bool expectedSign = false;

	for ( size_t i = 0; i < numVertices; ++i )
	{
		const Vec2<T> &a = polygon[i];
		const Vec2<T> &b = polygon[( i + 1 ) % numVertices];
		const Vec2<T> &c = polygon[( i + 2 ) % numVertices];

		const Vec2<T> ab = b - a;
		const Vec2<T> bc = c - b;
		const T cross = ab.x * bc.y - ab.y * bc.x;

		if ( math::abs( cross ) > static_cast<T>( 1e-10 ) ) // Not collinear
		{
			const bool currentSign = cross > static_cast<T>( 0 );

			if ( !signSet )
			{
				expectedSign = currentSign;
				signSet = true;
			}
			else if ( currentSign != expectedSign )
			{
				return false; // Sign changed, polygon is not convex
			}
		}
	}

	return true;
}

} // namespace math

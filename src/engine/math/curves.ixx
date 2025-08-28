export module engine.curves;

import engine.vec;
import engine.math;
import <vector>;
import <cmath>;

export namespace math
{

// ============================================================================
// BEZIER CURVES
// ============================================================================

// Linear Bezier curve (simple lerp)
template <typename T>
constexpr Vec2<T> linearBezier( const Vec2<T> &p0, const Vec2<T> &p1, T t ) noexcept
{
	return lerp( p0, p1, t );
}

template <typename T>
constexpr Vec3<T> linearBezier( const Vec3<T> &p0, const Vec3<T> &p1, T t ) noexcept
{
	return lerp( p0, p1, t );
}

// Quadratic Bezier curve (3 control points)
template <typename T>
constexpr Vec2<T> quadraticBezier( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T uu = u * u;
	const T ut2 = T( 2 ) * u * t;

	return p0 * uu + p1 * ut2 + p2 * tt;
}

template <typename T>
constexpr Vec3<T> quadraticBezier( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T uu = u * u;
	const T ut2 = T( 2 ) * u * t;

	return p0 * uu + p1 * ut2 + p2 * tt;
}

// Cubic Bezier curve (4 control points)
template <typename T>
constexpr Vec2<T> cubicBezier( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T ttt = tt * t;
	const T uu = u * u;
	const T uuu = uu * u;
	const T uut3 = T( 3 ) * uu * t;
	const T utt3 = T( 3 ) * u * tt;

	return p0 * uuu + p1 * uut3 + p2 * utt3 + p3 * ttt;
}

template <typename T>
constexpr Vec3<T> cubicBezier( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T ttt = tt * t;
	const T uu = u * u;
	const T uuu = uu * u;
	const T uut3 = T( 3 ) * uu * t;
	const T utt3 = T( 3 ) * u * tt;

	return p0 * uuu + p1 * uut3 + p2 * utt3 + p3 * ttt;
}

// Bezier curve derivatives for tangent calculation
template <typename T>
constexpr Vec2<T> quadraticBezierDerivative( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, T t ) noexcept
{
	const T u = T( 1 ) - t;
	return ( p1 - p0 ) * ( T( 2 ) * u ) + ( p2 - p1 ) * ( T( 2 ) * t );
}

template <typename T>
constexpr Vec3<T> quadraticBezierDerivative( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, T t ) noexcept
{
	const T u = T( 1 ) - t;
	return ( p1 - p0 ) * ( T( 2 ) * u ) + ( p2 - p1 ) * ( T( 2 ) * t );
}

template <typename T>
constexpr Vec2<T> cubicBezierDerivative( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T uu = u * u;
	const T ut2 = T( 2 ) * u * t;

	return ( p1 - p0 ) * ( T( 3 ) * uu ) + ( p2 - p1 ) * ( T( 3 ) * ut2 ) + ( p3 - p2 ) * ( T( 3 ) * tt );
}

template <typename T>
constexpr Vec3<T> cubicBezierDerivative( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, T t ) noexcept
{
	const T u = T( 1 ) - t;
	const T tt = t * t;
	const T uu = u * u;
	const T ut2 = T( 2 ) * u * t;

	return ( p1 - p0 ) * ( T( 3 ) * uu ) + ( p2 - p1 ) * ( T( 3 ) * ut2 ) + ( p3 - p2 ) * ( T( 3 ) * tt );
}

// ============================================================================
// CATMULL-ROM SPLINES
// ============================================================================

// Catmull-Rom spline interpolation (4 control points: p0 before, p1 start, p2 end, p3 after)
template <typename T>
constexpr Vec2<T> catmullRom( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, T t ) noexcept
{
	const T tt = t * t;
	const T ttt = tt * t;

	// Catmull-Rom basis functions
	const T a = T( -0.5 ) * ttt + tt - T( 0.5 ) * t;
	const T b = T( 1.5 ) * ttt - T( 2.5 ) * tt + T( 1 );
	const T c = T( -1.5 ) * ttt + T( 2 ) * tt + T( 0.5 ) * t;
	const T d = T( 0.5 ) * ttt - T( 0.5 ) * tt;

	return p0 * a + p1 * b + p2 * c + p3 * d;
}

template <typename T>
constexpr Vec3<T> catmullRom( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, T t ) noexcept
{
	const T tt = t * t;
	const T ttt = tt * t;

	// Catmull-Rom basis functions
	const T a = T( -0.5 ) * ttt + tt - T( 0.5 ) * t;
	const T b = T( 1.5 ) * ttt - T( 2.5 ) * tt + T( 1 );
	const T c = T( -1.5 ) * ttt + T( 2 ) * tt + T( 0.5 ) * t;
	const T d = T( 0.5 ) * ttt - T( 0.5 ) * tt;

	return p0 * a + p1 * b + p2 * c + p3 * d;
}

// Catmull-Rom spline with custom tension parameter
template <typename T>
constexpr Vec2<T> catmullRomWithTension( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, T t, T tension = T( 0.5 ) ) noexcept
{
	const T tt = t * t;
	const T ttt = tt * t;

	const T a = -tension * ttt + T( 2 ) * tension * tt - tension * t;
	const T b = ( T( 2 ) - tension ) * ttt + ( tension - T( 3 ) ) * tt + T( 1 );
	const T c = ( tension - T( 2 ) ) * ttt + ( T( 3 ) - T( 2 ) * tension ) * tt + tension * t;
	const T d = tension * ttt - tension * tt;

	return p0 * a + p1 * b + p2 * c + p3 * d;
}

template <typename T>
constexpr Vec3<T> catmullRomWithTension( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, T t, T tension = T( 0.5 ) ) noexcept
{
	const T tt = t * t;
	const T ttt = tt * t;

	const T a = -tension * ttt + T( 2 ) * tension * tt - tension * t;
	const T b = ( T( 2 ) - tension ) * ttt + ( tension - T( 3 ) ) * tt + T( 1 );
	const T c = ( tension - T( 2 ) ) * ttt + ( T( 3 ) - T( 2 ) * tension ) * tt + tension * t;
	const T d = tension * ttt - tension * tt;

	return p0 * a + p1 * b + p2 * c + p3 * d;
}

// Catmull-Rom derivative for tangent calculation
template <typename T>
constexpr Vec2<T> catmullRomDerivative( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, T t ) noexcept
{
	const T tt = t * t;

	const T da = T( -1.5 ) * tt + T( 2 ) * t - T( 0.5 );
	const T db = T( 4.5 ) * tt - T( 5 ) * t;
	const T dc = T( -4.5 ) * tt + T( 4 ) * t + T( 0.5 );
	const T dd = T( 1.5 ) * tt - t;

	return p0 * da + p1 * db + p2 * dc + p3 * dd;
}

template <typename T>
constexpr Vec3<T> catmullRomDerivative( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, T t ) noexcept
{
	const T tt = t * t;

	const T da = T( -1.5 ) * tt + T( 2 ) * t - T( 0.5 );
	const T db = T( 4.5 ) * tt - T( 5 ) * t;
	const T dc = T( -4.5 ) * tt + T( 4 ) * t + T( 0.5 );
	const T dd = T( 1.5 ) * tt - t;

	return p0 * da + p1 * db + p2 * dc + p3 * dd;
}

// ============================================================================
// ARC LENGTH PARAMETERIZATION
// ============================================================================

// Calculate approximate arc length of a curve using Simpson's rule
template <typename T>
T arcLength( const std::vector<Vec2<T>> &points ) noexcept
{
	if ( points.size() < 2 )
		return T( 0 );

	T totalLength = T( 0 );
	for ( size_t i = 1; i < points.size(); ++i )
	{
		totalLength += length( points[i] - points[i - 1] );
	}

	return totalLength;
}

template <typename T>
T arcLength( const std::vector<Vec3<T>> &points ) noexcept
{
	if ( points.size() < 2 )
		return T( 0 );

	T totalLength = T( 0 );
	for ( size_t i = 1; i < points.size(); ++i )
	{
		totalLength += length( points[i] - points[i - 1] );
	}

	return totalLength;
}

// Calculate arc length of a Bezier curve using adaptive subdivision
template <typename T>
T bezierArcLength( const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, int subdivisions = 32 ) noexcept
{
	T totalLength = T( 0 );
	const T step = T( 1 ) / T( subdivisions );

	Vec2<T> prevPoint = p0;
	for ( int i = 1; i <= subdivisions; ++i )
	{
		const T t = T( i ) * step;
		const Vec2<T> currentPoint = cubicBezier( p0, p1, p2, p3, t );
		totalLength += length( currentPoint - prevPoint );
		prevPoint = currentPoint;
	}

	return totalLength;
}

template <typename T>
T bezierArcLength( const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, int subdivisions = 32 ) noexcept
{
	T totalLength = T( 0 );
	const T step = T( 1 ) / T( subdivisions );

	Vec3<T> prevPoint = p0;
	for ( int i = 1; i <= subdivisions; ++i )
	{
		const T t = T( i ) * step;
		const Vec3<T> currentPoint = cubicBezier( p0, p1, p2, p3, t );
		totalLength += length( currentPoint - prevPoint );
		prevPoint = currentPoint;
	}

	return totalLength;
}

// Sample a curve by distance along its arc length
template <typename T>
Vec2<T> sampleByDistance( const std::vector<Vec2<T>> &points, T distance ) noexcept
{
	if ( points.empty() )
		return Vec2<T>( T( 0 ), T( 0 ) );

	if ( points.size() == 1 )
		return points[0];

	if ( distance <= T( 0 ) )
		return points[0];

	T accumulatedDistance = T( 0 );
	for ( size_t i = 1; i < points.size(); ++i )
	{
		const T segmentLength = length( points[i] - points[i - 1] );

		if ( accumulatedDistance + segmentLength >= distance )
		{
			// Interpolate within this segment
			const T t = ( distance - accumulatedDistance ) / segmentLength;
			return lerp( points[i - 1], points[i], t );
		}

		accumulatedDistance += segmentLength;
	}

	// Distance is beyond the curve, return the last point
	return points.back();
}

template <typename T>
Vec3<T> sampleByDistance( const std::vector<Vec3<T>> &points, T distance ) noexcept
{
	if ( points.empty() )
		return Vec3<T>( T( 0 ), T( 0 ), T( 0 ) );

	if ( points.size() == 1 )
		return points[0];

	if ( distance <= T( 0 ) )
		return points[0];

	T accumulatedDistance = T( 0 );
	for ( size_t i = 1; i < points.size(); ++i )
	{
		const T segmentLength = length( points[i] - points[i - 1] );

		if ( accumulatedDistance + segmentLength >= distance )
		{
			// Interpolate within this segment
			const T t = ( distance - accumulatedDistance ) / segmentLength;
			return lerp( points[i - 1], points[i], t );
		}

		accumulatedDistance += segmentLength;
	}

	// Distance is beyond the curve, return the last point
	return points.back();
}

// ============================================================================
// CURVE UTILITIES
// ============================================================================

// Find the closest point on a curve to a given point
template <typename T>
T findClosestPointOnCurve( const std::vector<Vec2<T>> &curve, const Vec2<T> &point, Vec2<T> &closestPoint, int &segmentIndex ) noexcept
{
	if ( curve.empty() )
	{
		closestPoint = Vec2<T>( T( 0 ), T( 0 ) );
		segmentIndex = -1;
		return T( std::numeric_limits<float>::infinity() );
	}

	T minDistanceSq = std::numeric_limits<T>::infinity();
	segmentIndex = 0;

	for ( size_t i = 0; i < curve.size(); ++i )
	{
		const T distSq = lengthSquared( curve[i] - point );
		if ( distSq < minDistanceSq )
		{
			minDistanceSq = distSq;
			closestPoint = curve[i];
			segmentIndex = static_cast<int>( i );
		}
	}

	return std::sqrt( minDistanceSq );
}

template <typename T>
T findClosestPointOnCurve( const std::vector<Vec3<T>> &curve, const Vec3<T> &point, Vec3<T> &closestPoint, int &segmentIndex ) noexcept
{
	if ( curve.empty() )
	{
		closestPoint = Vec3<T>( T( 0 ), T( 0 ), T( 0 ) );
		segmentIndex = -1;
		return std::numeric_limits<T>::infinity();
	}

	T minDistanceSq = std::numeric_limits<T>::infinity();
	segmentIndex = 0;

	for ( size_t i = 0; i < curve.size(); ++i )
	{
		const T distSq = lengthSquared( curve[i] - point );
		if ( distSq < minDistanceSq )
		{
			minDistanceSq = distSq;
			closestPoint = curve[i];
			segmentIndex = static_cast<int>( i );
		}
	}

	return std::sqrt( minDistanceSq );
}

// Generate a smooth curve from control points using Catmull-Rom
template <typename T>
std::vector<Vec2<T>> generateSmoothCurve( const std::vector<Vec2<T>> &controlPoints, int pointsPerSegment = 16 ) noexcept
{
	if ( controlPoints.size() < 2 )
		return controlPoints;

	std::vector<Vec2<T>> result;
	result.reserve( ( controlPoints.size() - 1 ) * pointsPerSegment + 1 );

	// Add first point
	result.push_back( controlPoints[0] );

	for ( size_t i = 0; i < controlPoints.size() - 1; ++i )
	{
		const Vec2<T> p0 = ( i == 0 ) ? controlPoints[0] : controlPoints[i - 1];
		const Vec2<T> p1 = controlPoints[i];
		const Vec2<T> p2 = controlPoints[i + 1];
		const Vec2<T> p3 = ( i + 2 < controlPoints.size() ) ? controlPoints[i + 2] : controlPoints[i + 1];

		for ( int j = 1; j <= pointsPerSegment; ++j )
		{
			const T t = T( j ) / T( pointsPerSegment );
			result.push_back( catmullRom( p0, p1, p2, p3, t ) );
		}
	}

	return result;
}

template <typename T>
std::vector<Vec3<T>> generateSmoothCurve( const std::vector<Vec3<T>> &controlPoints, int pointsPerSegment = 16 ) noexcept
{
	if ( controlPoints.size() < 2 )
		return controlPoints;

	std::vector<Vec3<T>> result;
	result.reserve( ( controlPoints.size() - 1 ) * pointsPerSegment + 1 );

	// Add first point
	result.push_back( controlPoints[0] );

	for ( size_t i = 0; i < controlPoints.size() - 1; ++i )
	{
		const Vec3<T> p0 = ( i == 0 ) ? controlPoints[0] : controlPoints[i - 1];
		const Vec3<T> p1 = controlPoints[i];
		const Vec3<T> p2 = controlPoints[i + 1];
		const Vec3<T> p3 = ( i + 2 < controlPoints.size() ) ? controlPoints[i + 2] : controlPoints[i + 1];

		for ( int j = 1; j <= pointsPerSegment; ++j )
		{
			const T t = T( j ) / T( pointsPerSegment );
			result.push_back( catmullRom( p0, p1, p2, p3, t ) );
		}
	}

	return result;
}

} // namespace math

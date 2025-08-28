export module engine.math_3d;

import std;
import engine.vec;
import engine.math;
import engine.bounding_volumes;

export namespace math
{

// ============================================================================
// POINT-IN-SHAPE TESTS (3D)
// ============================================================================

// Test if point is inside sphere
template <typename T>
constexpr bool pointInSphere( const Vec3<T> &point, const Vec3<T> &center, T radius ) noexcept
{
	const Vec3<T> diff = point - center;
	return lengthSquared( diff ) <= ( radius * radius );
}

// Test if point is inside axis-aligned bounding box
template <typename T>
constexpr bool pointInAABB( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max ) noexcept
{
	return point.x >= min.x && point.x <= max.x &&
		point.y >= min.y && point.y <= max.y &&
		point.z >= min.z && point.z <= max.z;
}

// Test if point is inside oriented bounding box
template <typename T>
constexpr bool pointInOBB( const Vec3<T> &point, const OrientedBoundingBox<T> &obb ) noexcept
{
	return obb.contains( point );
}

// Test if point is inside tetrahedron using barycentric coordinates
template <typename T>
constexpr bool pointInTetrahedron( const Vec3<T> &point, const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c, const Vec3<T> &d ) noexcept
{
	// Calculate barycentric coordinates
	const Vec3<T> v0 = b - a;
	const Vec3<T> v1 = c - a;
	const Vec3<T> v2 = d - a;
	const Vec3<T> vp = point - a;

	// Calculate dot products
	const T dot00 = dot( v0, v0 );
	const T dot01 = dot( v0, v1 );
	const T dot02 = dot( v0, v2 );
	const T dot11 = dot( v1, v1 );
	const T dot12 = dot( v1, v2 );
	const T dot22 = dot( v2, v2 );
	const T dotp0 = dot( vp, v0 );
	const T dotp1 = dot( vp, v1 );
	const T dotp2 = dot( vp, v2 );

	// Calculate barycentric coordinates
	const T det = dot00 * dot11 * dot22 + T( 2 ) * dot01 * dot02 * dot12 -
		dot00 * dot12 * dot12 - dot11 * dot02 * dot02 - dot22 * dot01 * dot01;

	if ( std::abs( det ) < T( 1e-10 ) )
		return false; // Degenerate tetrahedron

	const T invDet = T( 1 ) / det;

	const T u = ( ( dot11 * dot22 - dot12 * dot12 ) * dotp0 +
					( dot02 * dot12 - dot01 * dot22 ) * dotp1 +
					( dot01 * dot12 - dot02 * dot11 ) * dotp2 ) *
		invDet;

	const T v = ( ( dot02 * dot12 - dot01 * dot22 ) * dotp0 +
					( dot00 * dot22 - dot02 * dot02 ) * dotp1 +
					( dot01 * dot02 - dot00 * dot12 ) * dotp2 ) *
		invDet;

	const T w = ( ( dot01 * dot12 - dot02 * dot11 ) * dotp0 +
					( dot01 * dot02 - dot00 * dot12 ) * dotp1 +
					( dot00 * dot11 - dot01 * dot01 ) * dotp2 ) *
		invDet;

	return u >= T( 0 ) && v >= T( 0 ) && w >= T( 0 ) && ( u + v + w ) <= T( 1 );
}

// ============================================================================
// RAY-SHAPE INTERSECTIONS
// ============================================================================

// Ray-sphere intersection
template <typename T>
constexpr bool raySphereIntersection( const Vec3<T> &origin, const Vec3<T> &direction, const Vec3<T> &center, T radius, T &hitDistance ) noexcept
{
	const Vec3<T> oc = origin - center;
	const T a = dot( direction, direction );
	const T b = T( 2 ) * dot( oc, direction );
	const T c = dot( oc, oc ) - radius * radius;

	const T discriminant = b * b - T( 4 ) * a * c;
	if ( discriminant < T( 0 ) )
		return false;

	const T sqrt_discriminant = std::sqrt( discriminant );
	const T t1 = ( -b - sqrt_discriminant ) / ( T( 2 ) * a );
	const T t2 = ( -b + sqrt_discriminant ) / ( T( 2 ) * a );

	if ( t1 >= T( 0 ) )
	{
		hitDistance = t1;
		return true;
	}
	else if ( t2 >= T( 0 ) )
	{
		hitDistance = t2;
		return true;
	}

	return false;
}

// Ray-AABB intersection using slab method
template <typename T>
constexpr bool rayAABBIntersection( const Vec3<T> &origin, const Vec3<T> &direction, const Vec3<T> &min, const Vec3<T> &max, T &hitDistance ) noexcept
{
	T tMin = T( 0 );
	T tMax = std::numeric_limits<T>::max();

	for ( int i = 0; i < 3; ++i )
	{
		const T component = ( i == 0 ) ? direction.x : ( i == 1 ) ? direction.y :
																	direction.z;
		const T origin_component = ( i == 0 ) ? origin.x : ( i == 1 ) ? origin.y :
																		origin.z;
		const T min_component = ( i == 0 ) ? min.x : ( i == 1 ) ? min.y :
																  min.z;
		const T max_component = ( i == 0 ) ? max.x : ( i == 1 ) ? max.y :
																  max.z;

		if ( std::abs( component ) < T( 1e-8 ) )
		{
			// Ray is parallel to slab
			if ( origin_component < min_component || origin_component > max_component )
				return false;
		}
		else
		{
			const T invDir = T( 1 ) / component;
			T t1 = ( min_component - origin_component ) * invDir;
			T t2 = ( max_component - origin_component ) * invDir;

			if ( t1 > t2 )
			{
				const T temp = t1;
				t1 = t2;
				t2 = temp;
			}

			tMin = std::max( tMin, t1 );
			tMax = std::min( tMax, t2 );

			if ( tMin > tMax )
				return false;
		}
	}

	hitDistance = tMin;
	return tMin >= T( 0 );
}

// Ray-triangle intersection using Möller-Trumbore algorithm
template <typename T>
constexpr bool rayTriangleIntersection( const Vec3<T> &origin, const Vec3<T> &direction, const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c, Vec3<T> &hitPoint ) noexcept
{
	const T epsilon = T( 1e-8 );

	const Vec3<T> edge1 = b - a;
	const Vec3<T> edge2 = c - a;
	const Vec3<T> h = cross( direction, edge2 );
	const T det = dot( edge1, h );

	if ( std::abs( det ) < epsilon )
		return false; // Ray is parallel to triangle

	const T invDet = T( 1 ) / det;
	const Vec3<T> s = origin - a;
	const T u = invDet * dot( s, h );

	if ( u < T( 0 ) || u > T( 1 ) )
		return false;

	const Vec3<T> q = cross( s, edge1 );
	const T v = invDet * dot( direction, q );

	if ( v < T( 0 ) || u + v > T( 1 ) )
		return false;

	const T t = invDet * dot( edge2, q );

	if ( t > epsilon )
	{
		hitPoint = origin + direction * t;
		return true;
	}

	return false;
}

// Ray-plane intersection
template <typename T>
constexpr bool rayPlaneIntersection( const Vec3<T> &origin, const Vec3<T> &direction, const Vec3<T> &planePoint, const Vec3<T> &planeNormal, T &hitDistance ) noexcept
{
	const T denom = dot( planeNormal, direction );
	if ( std::abs( denom ) < T( 1e-8 ) )
		return false; // Ray is parallel to plane

	const Vec3<T> p0l0 = planePoint - origin;
	const T t = dot( p0l0, planeNormal ) / denom;

	if ( t >= T( 0 ) )
	{
		hitDistance = t;
		return true;
	}

	return false;
}

// ============================================================================
// LINE AND SEGMENT INTERSECTIONS
// ============================================================================

// Closest points between two 3D lines
template <typename T>
constexpr bool lineLineIntersection3D( const Vec3<T> &a1, const Vec3<T> &a2, const Vec3<T> &b1, const Vec3<T> &b2, Vec3<T> &closest1, Vec3<T> &closest2 ) noexcept
{
	const Vec3<T> da = a2 - a1;
	const Vec3<T> db = b2 - b1;
	const Vec3<T> dc = b1 - a1;

	const Vec3<T> cross_da_db = cross( da, db );
	const T cross_magnitude_sq = lengthSquared( cross_da_db );

	if ( cross_magnitude_sq < T( 1e-8 ) )
		return false; // Lines are parallel

	const T t1 = dot( cross( dc, db ), cross_da_db ) / cross_magnitude_sq;
	const T t2 = dot( cross( dc, da ), cross_da_db ) / cross_magnitude_sq;

	closest1 = a1 + da * t1;
	closest2 = b1 + db * t2;

	return true;
}

// Segment-segment intersection (returns true if segments intersect)
template <typename T>
constexpr bool segmentSegmentIntersection( const Vec3<T> &a1, const Vec3<T> &a2, const Vec3<T> &b1, const Vec3<T> &b2, Vec3<T> &intersection ) noexcept
{
	Vec3<T> closest1, closest2;
	if ( !lineLineIntersection3D( a1, a2, b1, b2, closest1, closest2 ) )
		return false;

	// Check if closest points are within segments
	const Vec3<T> da = a2 - a1;
	const Vec3<T> db = b2 - b1;

	const T t1 = dot( closest1 - a1, da ) / lengthSquared( da );
	const T t2 = dot( closest2 - b1, db ) / lengthSquared( db );

	if ( t1 >= T( 0 ) && t1 <= T( 1 ) && t2 >= T( 0 ) && t2 <= T( 1 ) )
	{
		// Check if closest points are actually close enough to be considered intersecting
		const T distance_sq = lengthSquared( closest1 - closest2 );
		if ( distance_sq < T( 1e-6 ) )
		{
			intersection = ( closest1 + closest2 ) * T( 0.5 );
			return true;
		}
	}

	return false;
}

// ============================================================================
// DISTANCE CALCULATIONS
// ============================================================================

// Distance from point to plane
template <typename T>
constexpr T distancePointToPlane( const Vec3<T> &point, const Vec3<T> &planePoint, const Vec3<T> &planeNormal ) noexcept
{
	const Vec3<T> normalized_normal = normalize( planeNormal );
	return std::abs( dot( ( point - planePoint ), normalized_normal ) );
}

// Distance from point to 3D line
template <typename T>
constexpr T distancePointToLine3D( const Vec3<T> &point, const Vec3<T> &linePoint, const Vec3<T> &lineDirection ) noexcept
{
	const Vec3<T> normalized_direction = normalize( lineDirection );
	const Vec3<T> to_point = point - linePoint;
	const Vec3<T> projection = normalized_direction * dot( to_point, normalized_direction );
	const Vec3<T> rejection = to_point - projection;
	return length( rejection );
}

// Distance from point to 3D line segment
template <typename T>
constexpr T distancePointToSegment3D( const Vec3<T> &point, const Vec3<T> &segmentStart, const Vec3<T> &segmentEnd ) noexcept
{
	const Vec3<T> segment = segmentEnd - segmentStart;
	const T segment_length_sq = lengthSquared( segment );

	if ( segment_length_sq < T( 1e-8 ) )
		return length( point - segmentStart ); // Degenerate segment

	const Vec3<T> to_point = point - segmentStart;
	T t = dot( to_point, segment ) / segment_length_sq;
	t = math::clamp( t, T( 0 ), T( 1 ) );

	const Vec3<T> closest_point = segmentStart + segment * t;
	return length( point - closest_point );
}

// Distance between two 3D lines
template <typename T>
constexpr T distanceLinesToLines( const Vec3<T> &a1, const Vec3<T> &a2, const Vec3<T> &b1, const Vec3<T> &b2 ) noexcept
{
	Vec3<T> closest1, closest2;
	if ( lineLineIntersection3D( a1, a2, b1, b2, closest1, closest2 ) )
		return length( closest1 - closest2 );

	return T( 0 ); // Lines are parallel or degenerate
}

// ============================================================================
// GEOMETRIC CALCULATIONS
// ============================================================================

// Calculate tetrahedron volume
template <typename T>
constexpr T tetrahedronVolume( const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c, const Vec3<T> &d ) noexcept
{
	const Vec3<T> ab = b - a;
	const Vec3<T> ac = c - a;
	const Vec3<T> ad = d - a;

	return math::abs( dot( ab, cross( ac, ad ) ) ) / T( 6 );
}

// Calculate 3D triangle area
template <typename T>
constexpr T triangleArea3D( const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c ) noexcept
{
	const Vec3<T> ab = b - a;
	const Vec3<T> ac = c - a;
	return length( cross( ab, ac ) ) * T( 0.5 );
}

// Calculate triangle normal
template <typename T>
constexpr Vec3<T> triangleNormal( const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c ) noexcept
{
	const Vec3<T> ab = b - a;
	const Vec3<T> ac = c - a;
	return normalize( cross( ab, ac ) );
}

// Calculate barycentric coordinates for point on triangle
template <typename T>
constexpr Vec3<T> barycentric3D( const Vec3<T> &point, const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c ) noexcept
{
	const Vec3<T> v0 = b - a;
	const Vec3<T> v1 = c - a;
	const Vec3<T> v2 = point - a;

	const T dot00 = dot( v0, v0 );
	const T dot01 = dot( v0, v1 );
	const T dot11 = dot( v1, v1 );
	const T dot20 = dot( v2, v0 );
	const T dot21 = dot( v2, v1 );

	const T invDenom = T( 1 ) / ( dot00 * dot11 - dot01 * dot01 );
	const T u = ( dot11 * dot20 - dot01 * dot21 ) * invDenom;
	const T v = ( dot00 * dot21 - dot01 * dot20 ) * invDenom;

	return Vec3<T>( T( 1 ) - u - v, u, v ); // (alpha, beta, gamma) where alpha + beta + gamma = 1
}

} // namespace math

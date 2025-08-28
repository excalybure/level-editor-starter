export module engine.plane;

import std;
import engine.vec;

export namespace math
{

// ============================================================================
// PLANE STRUCTURE
// ============================================================================

template <typename T>
struct Plane
{
	Vec3<T> normal;
	T distance; // Distance from origin along normal

	constexpr Plane() noexcept : normal( T( 0 ), T( 1 ), T( 0 ) ), distance( T( 0 ) ) {}

	// Construct from point on plane and normal
	constexpr Plane( const Vec3<T> &point, const Vec3<T> &normal ) noexcept
		: normal( normalize( normal ) ), distance( dot( point, this->normal ) ) {}

	// Construct from three points (triangle)
	constexpr Plane( const Vec3<T> &a, const Vec3<T> &b, const Vec3<T> &c ) noexcept
	{
		Vec3<T> ab = b - a;
		Vec3<T> ac = c - a;
		normal = normalize( cross( ab, ac ) );
		distance = dot( a, normal );
	}

	// Distance from point to plane (signed)
	constexpr T distanceToPoint( const Vec3<T> &point ) const noexcept
	{
		return dot( normal, point ) - distance;
	}

	// Closest point on plane to given point
	constexpr Vec3<T> closestPoint( const Vec3<T> &point ) const noexcept
	{
		T dist = distanceToPoint( point );
		return point - normal * dist;
	}

	// Check if point is on plane within tolerance
	constexpr bool isPointOnPlane( const Vec3<T> &point, T tolerance = T( 1e-6 ) ) const noexcept
	{
		return std::abs( distanceToPoint( point ) ) <= tolerance;
	}
};

} // namespace math

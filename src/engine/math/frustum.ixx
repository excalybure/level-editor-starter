export module engine.frustum;

import std;
import engine.vec;
import engine.plane;
import engine.bounding_box_3d;
import engine.bounding_sphere;

export namespace math
{

// ============================================================================
// FRUSTUM STRUCTURE (for 3D culling)
// ============================================================================

template <typename T>
struct Frustum
{
	Plane<T> planes[6]; // Left, Right, Top, Bottom, Near, Far

	constexpr Frustum() noexcept = default;

	// Point containment test
	constexpr bool contains( const Vec3<T> &point ) const noexcept
	{
		for ( int i = 0; i < 6; ++i )
		{
			if ( planes[i].distanceToPoint( point ) < T( 0 ) )
				return false;
		}
		return true;
	}

	// Bounding box intersection test
	constexpr bool intersects( const BoundingBox3D<T> &box ) const noexcept
	{
		for ( int i = 0; i < 6; ++i )
		{
			// Find the positive vertex (farthest along plane normal)
			Vec3<T> positive;
			positive.x = ( planes[i].normal.x >= T( 0 ) ) ? box.max.x : box.min.x;
			positive.y = ( planes[i].normal.y >= T( 0 ) ) ? box.max.y : box.min.y;
			positive.z = ( planes[i].normal.z >= T( 0 ) ) ? box.max.z : box.min.z;

			// If positive vertex is behind plane, box is completely outside
			if ( planes[i].distanceToPoint( positive ) < T( 0 ) )
				return false;
		}
		return true;
	}

	// Bounding sphere intersection test
	constexpr bool intersects( const BoundingSphere<T> &sphere ) const noexcept
	{
		for ( int i = 0; i < 6; ++i )
		{
			const T distance = planes[i].distanceToPoint( sphere.center );
			if ( distance < -sphere.radius )
				return false;
		}
		return true;
	}
};

} // namespace math

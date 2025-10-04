#pragma once

#include "vec.h"

namespace math
{

// ============================================================================
// ORIENTED BOUNDING BOX STRUCTURE
// ============================================================================

template <typename T>
struct OrientedBoundingBox
{
	Vec3<T> center;
	Vec3<T> axes[3]; // Local coordinate system (should be orthonormal)
	Vec3<T> extents; // Half-sizes along each axis

	constexpr OrientedBoundingBox() noexcept
		: center( T( 0 ) ), extents( T( 0 ) )
	{
		axes[0] = Vec3<T>( T( 1 ), T( 0 ), T( 0 ) );
		axes[1] = Vec3<T>( T( 0 ), T( 1 ), T( 0 ) );
		axes[2] = Vec3<T>( T( 0 ), T( 0 ), T( 1 ) );
	}

	constexpr OrientedBoundingBox( const Vec3<T> &center, const Vec3<T> axes[3], const Vec3<T> &extents ) noexcept
		: center( center ), extents( extents )
	{
		this->axes[0] = axes[0];
		this->axes[1] = axes[1];
		this->axes[2] = axes[2];
	}

	// Point containment test using separating axes
	constexpr bool contains( const Vec3<T> &point ) const noexcept
	{
		Vec3<T> diff = point - center;

		for ( int i = 0; i < 3; ++i )
		{
			T projection = dot( diff, axes[i] );
			if ( std::abs( projection ) > extents[i] )
				return false;
		}

		return true;
	}

	// OBB intersection test using separating axes theorem
	bool intersects( const OrientedBoundingBox &other ) const noexcept
	{
		Vec3<T> diff = other.center - center;

		// Test the 15 potential separating axes
		for ( int i = 0; i < 3; ++i )
		{
			// Test axes of first OBB
			if ( !testSeparatingAxis( axes[i], diff, *this, other ) )
				return false;

			// Test axes of second OBB
			if ( !testSeparatingAxis( other.axes[i], diff, *this, other ) )
				return false;
		}

		// Test cross products of axes
		for ( int i = 0; i < 3; ++i )
		{
			for ( int j = 0; j < 3; ++j )
			{
				Vec3<T> axis = cross( axes[i], other.axes[j] );
				if ( lengthSquared( axis ) > T( 1e-6 ) ) // Avoid near-parallel axes
				{
					axis = normalize( axis );
					if ( !testSeparatingAxis( axis, diff, *this, other ) )
						return false;
				}
			}
		}

		return true; // No separating axis found - boxes intersect
	}

	// Get one of 8 corners
	constexpr Vec3<T> corner( int index ) const noexcept
	{
		Vec3<T> offset =
			axes[0] * ( ( ( index & 1 ) ? extents.x : -extents.x ) ) +
			axes[1] * ( ( ( index & 2 ) ? extents.y : -extents.y ) ) +
			axes[2] * ( ( ( index & 4 ) ? extents.z : -extents.z ) );

		return center + offset;
	}

private:
	// Helper function for separating axes test
	static bool testSeparatingAxis( const Vec3<T> &axis, const Vec3<T> &diff, const OrientedBoundingBox &obb1, const OrientedBoundingBox &obb2 ) noexcept
	{
		// Project centers difference onto axis
		T centerProjection = std::abs( dot( diff, axis ) );

		// Project first OBB onto axis
		T projection1 =
			std::abs( obb1.extents.x * dot( obb1.axes[0], axis ) ) +
			std::abs( obb1.extents.y * dot( obb1.axes[1], axis ) ) +
			std::abs( obb1.extents.z * dot( obb1.axes[2], axis ) );

		// Project second OBB onto axis
		T projection2 =
			std::abs( obb2.extents.x * dot( obb2.axes[0], axis ) ) +
			std::abs( obb2.extents.y * dot( obb2.axes[1], axis ) ) +
			std::abs( obb2.extents.z * dot( obb2.axes[2], axis ) );

		// Check for separation
		return centerProjection <= ( projection1 + projection2 );
	}
};

} // namespace math

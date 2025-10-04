#pragma once

#include "vec.h"

namespace math
{

// ============================================================================
// BOUNDING SPHERE STRUCTURE
// ============================================================================

template <typename T>
struct BoundingSphere
{
	Vec3<T> center;
	T radius;

	constexpr BoundingSphere() noexcept = default;

	constexpr BoundingSphere( const Vec3<T> &center, T radius ) noexcept
		: center( center ), radius( radius ) {}

	constexpr BoundingSphere( T centerX, T centerY, T centerZ, T radius ) noexcept
		: center( centerX, centerY, centerZ ), radius( radius ) {}

	// Point containment test
	constexpr bool contains( const Vec3<T> &point ) const noexcept
	{
		const Vec3<T> diff = point - center;
		return lengthSquared( diff ) <= ( radius * radius );
	}

	// Sphere intersection test
	constexpr bool intersects( const BoundingSphere &other ) const noexcept
	{
		const Vec3<T> diff = other.center - center;
		const T combinedRadius = radius + other.radius;
		return lengthSquared( diff ) <= ( combinedRadius * combinedRadius );
	}

	// Expand to include point
	constexpr void expand( const Vec3<T> &point ) noexcept
	{
		const Vec3<T> diff = point - center;
		const T distance = length( diff );
		if ( distance > radius )
		{
			const T newRadius = ( radius + distance ) * T( 0.5 );
			const T factor = ( newRadius - radius ) / distance;
			center = center + diff * factor;
			radius = newRadius;
		}
	}

	// Expand to include another sphere
	constexpr void expand( const BoundingSphere &other ) noexcept
	{
		const Vec3<T> diff = other.center - center;
		const T distance = length( diff );

		if ( distance + other.radius <= radius )
			return; // Other sphere is already contained

		if ( distance + radius <= other.radius )
		{
			// This sphere is contained in other
			*this = other;
			return;
		}

		// Spheres overlap or are separate - create encompassing sphere
		const T newRadius = ( distance + radius + other.radius ) * T( 0.5 );
		const T factor = ( newRadius - radius ) / distance;
		center = center + diff * factor;
		radius = newRadius;
	}

	// Get surface area
	constexpr T surfaceArea() const noexcept
	{
		return T( 4 ) * math::pi<T> * radius * radius;
	}

	// Get volume
	constexpr T volume() const noexcept
	{
		return ( T( 4 ) / T( 3 ) ) * math::pi<T> * radius * radius * radius;
	}
};

} // namespace math

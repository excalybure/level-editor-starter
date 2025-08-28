export module engine.bounding_box_3d;

import std;
import engine.vec;

export namespace math
{

// ============================================================================
// BOUNDING BOX 3D STRUCTURE
// ============================================================================

template <typename T>
struct BoundingBox3D
{
	Vec3<T> min;
	Vec3<T> max;

	constexpr BoundingBox3D() noexcept : min( T( 0 ) ), max( T( 0 ) ) {}

	constexpr BoundingBox3D( const Vec3<T> &min, const Vec3<T> &max ) noexcept
		: min( min ), max( max ) {}

	constexpr BoundingBox3D( T minX, T minY, T minZ, T maxX, T maxY, T maxZ ) noexcept
		: min( minX, minY, minZ ), max( maxX, maxY, maxZ ) {}

	// Point containment test
	constexpr bool contains( const Vec3<T> &point ) const noexcept
	{
		return point.x >= min.x && point.x <= max.x &&
			point.y >= min.y && point.y <= max.y &&
			point.z >= min.z && point.z <= max.z;
	}

	// Bounding box intersection test
	constexpr bool intersects( const BoundingBox3D &other ) const noexcept
	{
		return min.x <= other.max.x && max.x >= other.min.x &&
			min.y <= other.max.y && max.y >= other.min.y &&
			min.z <= other.max.z && max.z >= other.min.z;
	}

	// Sphere intersection test
	constexpr bool intersects( const Vec3<T> &sphereCenter, T radius ) const noexcept
	{
		const T dx = std::max( T( 0 ), std::max( min.x - sphereCenter.x, sphereCenter.x - max.x ) );
		const T dy = std::max( T( 0 ), std::max( min.y - sphereCenter.y, sphereCenter.y - max.y ) );
		const T dz = std::max( T( 0 ), std::max( min.z - sphereCenter.z, sphereCenter.z - max.z ) );
		return ( dx * dx + dy * dy + dz * dz ) <= ( radius * radius );
	}

	// Expand to include point
	constexpr void expand( const Vec3<T> &point ) noexcept
	{
		min.x = std::min( min.x, point.x );
		min.y = std::min( min.y, point.y );
		min.z = std::min( min.z, point.z );
		max.x = std::max( max.x, point.x );
		max.y = std::max( max.y, point.y );
		max.z = std::max( max.z, point.z );
	}

	// Expand to include another bounding box
	constexpr void expand( const BoundingBox3D &other ) noexcept
	{
		min.x = std::min( min.x, other.min.x );
		min.y = std::min( min.y, other.min.y );
		min.z = std::min( min.z, other.min.z );
		max.x = std::max( max.x, other.max.x );
		max.y = std::max( max.y, other.max.y );
		max.z = std::max( max.z, other.max.z );
	}

	// Get center point
	constexpr Vec3<T> center() const noexcept
	{
		return Vec3<T>( ( min.x + max.x ) * static_cast<T>( 0.5 ), ( min.y + max.y ) * static_cast<T>( 0.5 ), ( min.z + max.z ) * static_cast<T>( 0.5 ) );
	}

	// Get size (width, height, depth)
	constexpr Vec3<T> size() const noexcept
	{
		return Vec3<T>( max.x - min.x, max.y - min.y, max.z - min.z );
	}

	// Get volume
	constexpr T volume() const noexcept
	{
		const Vec3<T> sz = size();
		return sz.x * sz.y * sz.z;
	}

	// Get one of 8 corners (index 0-7)
	constexpr Vec3<T> corner( int index ) const noexcept
	{
		return Vec3<T>(
			( index & 1 ) ? max.x : min.x,
			( index & 2 ) ? max.y : min.y,
			( index & 4 ) ? max.z : min.z );
	}

	// Check if bounding box is valid (min <= max)
	constexpr bool isValid() const noexcept
	{
		return min.x <= max.x && min.y <= max.y && min.z <= max.z;
	}
};

} // namespace math

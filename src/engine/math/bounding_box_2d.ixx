export module engine.bounding_box_2d;

import std;
import engine.vec;

export namespace math
{

// ============================================================================
// BOUNDING BOX 2D STRUCTURE
// ============================================================================

template <typename T>
struct BoundingBox2D
{
	Vec2<T> min;
	Vec2<T> max;

	constexpr BoundingBox2D() noexcept = default;

	constexpr BoundingBox2D( const Vec2<T> &min, const Vec2<T> &max ) noexcept
		: min( min ), max( max ) {}

	constexpr BoundingBox2D( T minX, T minY, T maxX, T maxY ) noexcept
		: min( minX, minY ), max( maxX, maxY ) {}

	// Point containment test
	constexpr bool contains( const Vec2<T> &point ) const noexcept
	{
		return point.x >= min.x && point.x <= max.x &&
			point.y >= min.y && point.y <= max.y;
	}

	// Check if this bounding box intersects with another
	constexpr bool intersects( const BoundingBox2D<T> &other ) const noexcept
	{
		return min.x <= other.max.x && max.x >= other.min.x &&
			min.y <= other.max.y && max.y >= other.min.y;
	}

	// Circle intersection test
	constexpr bool intersects( const Vec2<T> &circleCenter, T radius ) const noexcept
	{
		T dx = std::max( T( 0 ), std::max( min.x - circleCenter.x, circleCenter.x - max.x ) );
		T dy = std::max( T( 0 ), std::max( min.y - circleCenter.y, circleCenter.y - max.y ) );
		return ( dx * dx + dy * dy ) <= ( radius * radius );
	}

	// Expand to include point
	constexpr void expand( const Vec2<T> &point ) noexcept
	{
		min.x = std::min( min.x, point.x );
		min.y = std::min( min.y, point.y );
		max.x = std::max( max.x, point.x );
		max.y = std::max( max.y, point.y );
	}

	// Expand to include another bounding box
	constexpr void expand( const BoundingBox2D &other ) noexcept
	{
		min.x = std::min( min.x, other.min.x );
		min.y = std::min( min.y, other.min.y );
		max.x = std::max( max.x, other.max.x );
		max.y = std::max( max.y, other.max.y );
	}

	// Get center point
	constexpr Vec2<T> center() const noexcept
	{
		return Vec2<T>( ( min.x + max.x ) * static_cast<T>( 0.5 ),
			( min.y + max.y ) * static_cast<T>( 0.5 ) );
	}

	// Get size (width, height)
	constexpr Vec2<T> size() const noexcept
	{
		return Vec2<T>( max.x - min.x, max.y - min.y );
	}

	// Get area
	constexpr T area() const noexcept
	{
		const Vec2<T> s = size();
		return s.x * s.y;
	}

	// Check if bounding box is valid (min <= max)
	constexpr bool isValid() const noexcept
	{
		return min.x <= max.x && min.y <= max.y;
	}
};

// Type aliases for common usage
using BoundingBox2Df = BoundingBox2D<float>;
using BoundingBox2Dd = BoundingBox2D<double>;

} // namespace math

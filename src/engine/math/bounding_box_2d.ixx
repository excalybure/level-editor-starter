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

	constexpr BoundingBox2D( const Vec2<T> &minPoint, const Vec2<T> &maxPoint ) noexcept
		: min( minPoint ), max( maxPoint )
	{
	}

	// Check if a point is inside the bounding box
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

	// Expand the bounding box to include a point
	constexpr void expand( const Vec2<T> &point ) noexcept
	{
		min.x = std::min( min.x, point.x );
		min.y = std::min( min.y, point.y );
		max.x = std::max( max.x, point.x );
		max.y = std::max( max.y, point.y );
	}

	// Get the center of the bounding box
	constexpr Vec2<T> center() const noexcept
	{
		return Vec2<T>( ( min.x + max.x ) * static_cast<T>( 0.5 ),
			( min.y + max.y ) * static_cast<T>( 0.5 ) );
	}

	// Get the size (width, height) of the bounding box
	constexpr Vec2<T> size() const noexcept
	{
		return Vec2<T>( max.x - min.x, max.y - min.y );
	}

	// Get the area of the bounding box
	constexpr T area() const noexcept
	{
		const Vec2<T> s = size();
		return s.x * s.y;
	}

	// Check if the bounding box is valid (min <= max)
	constexpr bool isValid() const noexcept
	{
		return min.x <= max.x && min.y <= max.y;
	}
};

// Type aliases for common usage
using BoundingBox2Df = BoundingBox2D<float>;
using BoundingBox2Dd = BoundingBox2D<double>;

} // namespace math

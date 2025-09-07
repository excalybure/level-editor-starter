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

	constexpr BoundingBox2D() noexcept
		: min( std::numeric_limits<T>::max(), std::numeric_limits<T>::max() ),
		  max( std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest() ) {}

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
		const Vec2<T> closest = math::max( min, math::min( circleCenter, max ) );
		return math::lengthSquared( circleCenter - closest ) <= ( radius * radius );
	}

	// Expand to include point
	constexpr void expand( const Vec2<T> &point ) noexcept
	{
		min = math::min( min, point );
		max = math::max( max, point );
	}

	// Expand to include another bounding box
	constexpr void expand( const BoundingBox2D &other ) noexcept
	{
		min = math::min( min, other.min );
		max = math::max( max, other.max );
	}

	// Get center point
	constexpr Vec2<T> center() const noexcept
	{
		return ( min + max ) * static_cast<T>( 0.5 );
	}

	// Get size (width, height)
	constexpr Vec2<T> size() const noexcept
	{
		return max - min;
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

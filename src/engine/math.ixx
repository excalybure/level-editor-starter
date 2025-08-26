module; // global module fragment for traditional includes prior to module declaration
#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
// Standard library headers required (avoid relying solely on experimental 'import std;')
#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include <cstdint>

export module engine.math;

export namespace math
{
// Generic 2D/3D/4D vector math (templated, defaulting to float)
template <typename T = float>
struct Vec2
{
	T x{}, y{};
	using value_type = T;

	constexpr Vec2() = default;
	constexpr Vec2( T x_, T y_ ) : x{ x_ }, y{ y_ } {}

	// Arithmetic operators (component-wise for +,- and scalar + component-wise for *,/)
	constexpr Vec2 operator+( const Vec2 &rhs ) const { return { x + rhs.x, y + rhs.y }; }
	constexpr Vec2 operator-( const Vec2 &rhs ) const { return { x - rhs.x, y - rhs.y }; }
	constexpr Vec2 operator*( T s ) const { return { x * s, y * s }; }
	constexpr Vec2 operator/( T s ) const { return { x / s, y / s }; }
	constexpr Vec2 operator*( const Vec2 &rhs ) const { return { x * rhs.x, y * rhs.y }; }
	constexpr Vec2 operator/( const Vec2 &rhs ) const { return { x / rhs.x, y / rhs.y }; }
	constexpr Vec2 operator-() const { return { -x, -y }; }

	constexpr Vec2 &operator+=( const Vec2 &rhs )
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	constexpr Vec2 &operator-=( const Vec2 &rhs )
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	constexpr Vec2 &operator*=( T s )
	{
		x *= s;
		y *= s;
		return *this;
	}
	constexpr Vec2 &operator/=( T s )
	{
		x /= s;
		y /= s;
		return *this;
	}
	constexpr Vec2 &operator*=( const Vec2 &rhs )
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}
	constexpr Vec2 &operator/=( const Vec2 &rhs )
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}
};

template <typename T>
constexpr Vec2<T> operator*( T s, const Vec2<T> &v )
{
	return v * s;
}

template <typename T = float>
struct Vec3
{
	T x{}, y{}, z{};
	using value_type = T;
	constexpr Vec3() = default;
	constexpr Vec3( T x_, T y_, T z_ ) : x{ x_ }, y{ y_ }, z{ z_ } {}

	constexpr Vec3 operator+( const Vec3 &r ) const { return { x + r.x, y + r.y, z + r.z }; }
	constexpr Vec3 operator-( const Vec3 &r ) const { return { x - r.x, y - r.y, z - r.z }; }
	constexpr Vec3 operator*( T s ) const { return { x * s, y * s, z * s }; }
	constexpr Vec3 operator/( T s ) const { return { x / s, y / s, z / s }; }
	constexpr Vec3 operator*( const Vec3 &r ) const { return { x * r.x, y * r.y, z * r.z }; }
	constexpr Vec3 operator/( const Vec3 &r ) const { return { x / r.x, y / r.y, z / r.z }; }
	constexpr Vec3 operator-() const { return { -x, -y, -z }; }

	constexpr Vec3 &operator+=( const Vec3 &r )
	{
		x += r.x;
		y += r.y;
		z += r.z;
		return *this;
	}
	constexpr Vec3 &operator-=( const Vec3 &r )
	{
		x -= r.x;
		y -= r.y;
		z -= r.z;
		return *this;
	}
	constexpr Vec3 &operator*=( T s )
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}
	constexpr Vec3 &operator/=( T s )
	{
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}
	constexpr Vec3 &operator*=( const Vec3 &r )
	{
		x *= r.x;
		y *= r.y;
		z *= r.z;
		return *this;
	}
	constexpr Vec3 &operator/=( const Vec3 &r )
	{
		x /= r.x;
		y /= r.y;
		z /= r.z;
		return *this;
	}
};

template <typename T>
constexpr Vec3<T> operator*( T s, const Vec3<T> &v )
{
	return v * s;
}

template <typename T = float>
struct Vec4
{
	T x{}, y{}, z{}, w{};
	using value_type = T;
	constexpr Vec4() = default;
	constexpr Vec4( T x_, T y_, T z_, T w_ ) : x{ x_ }, y{ y_ }, z{ z_ }, w{ w_ } {}

	constexpr Vec4 operator+( const Vec4 &r ) const { return { x + r.x, y + r.y, z + r.z, w + r.w }; }
	constexpr Vec4 operator-( const Vec4 &r ) const { return { x - r.x, y - r.y, z - r.z, w - r.w }; }
	constexpr Vec4 operator*( T s ) const { return { x * s, y * s, z * s, w * s }; }
	constexpr Vec4 operator/( T s ) const { return { x / s, y / s, z / s, w / s }; }
	constexpr Vec4 operator*( const Vec4 &r ) const { return { x * r.x, y * r.y, z * r.z, w * r.w }; }
	constexpr Vec4 operator/( const Vec4 &r ) const { return { x / r.x, y / r.y, z / r.z, w / r.w }; }
	constexpr Vec4 operator-() const { return { -x, -y, -z, -w }; }

	constexpr Vec4 &operator+=( const Vec4 &r )
	{
		x += r.x;
		y += r.y;
		z += r.z;
		w += r.w;
		return *this;
	}
	constexpr Vec4 &operator-=( const Vec4 &r )
	{
		x -= r.x;
		y -= r.y;
		z -= r.z;
		w -= r.w;
		return *this;
	}
	constexpr Vec4 &operator*=( T s )
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}
	constexpr Vec4 &operator/=( T s )
	{
		x /= s;
		y /= s;
		z /= s;
		w /= s;
		return *this;
	}
	constexpr Vec4 &operator*=( const Vec4 &r )
	{
		x *= r.x;
		y *= r.y;
		z *= r.z;
		w *= r.w;
		return *this;
	}
	constexpr Vec4 &operator/=( const Vec4 &r )
	{
		x /= r.x;
		y /= r.y;
		z /= r.z;
		w /= r.w;
		return *this;
	}
};

template <typename T>
constexpr Vec4<T> operator*( T s, const Vec4<T> &v )
{
	return v * s;
}

// Dot products
template <typename T>
constexpr T dot( const Vec2<T> &a, const Vec2<T> &b )
{
	return a.x * b.x + a.y * b.y;
}
template <typename T>
constexpr T dot( const Vec3<T> &a, const Vec3<T> &b )
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
template <typename T>
constexpr T dot( const Vec4<T> &a, const Vec4<T> &b )
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Cross product (only meaningful for 3D vectors)
template <typename T>
constexpr Vec3<T> cross( const Vec3<T> &a, const Vec3<T> &b )
{
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

// Length helpers
template <typename V>
constexpr auto lengthSquared( const V &v )
{
	if constexpr ( requires { v.w; } )
	{
		return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
	}
	else if constexpr ( requires { v.z; } )
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}
	else
	{
		return v.x * v.x + v.y * v.y;
	}
}

template <typename V>
inline auto length( const V &v )
{
	return std::sqrt( static_cast<long double>( lengthSquared( v ) ) );
}

// Normalize (returns zero vector if length is (near) zero)
template <typename V>
constexpr V normalize( const V &v )
{
	const auto ls = lengthSquared( v );
	using VT = typename V::value_type;
	if ( ls <= std::numeric_limits<decltype( ls )>::epsilon() )
	{
		return V{}; // cannot normalize zero vector
	}
	const long double invLen = 1.0L / std::sqrt( static_cast<long double>( ls ) );
	if constexpr ( requires { v.w; } )
	{
		return { static_cast<VT>( v.x * invLen ), static_cast<VT>( v.y * invLen ), static_cast<VT>( v.z * invLen ), static_cast<VT>( v.w * invLen ) };
	}
	else if constexpr ( requires { v.z; } )
	{
		return { static_cast<VT>( v.x * invLen ), static_cast<VT>( v.y * invLen ), static_cast<VT>( v.z * invLen ) };
	}
	else
	{
		return { static_cast<VT>( v.x * invLen ), static_cast<VT>( v.y * invLen ) };
	}
}

// Distance utilities
template <typename V>
constexpr auto distanceSquared( const V &a, const V &b )
{
	return lengthSquared( a - b );
}

template <typename V>
inline auto distance( const V &a, const V &b )
{
	return length( a - b );
}

// Linear interpolation: lerp(a,b,t) = a + t*(b-a)
template <typename V, typename T>
constexpr V lerp( const V &a, const V &b, T t )
{
	return a + ( b - a ) * static_cast<typename V::value_type>( t );
}

// Reflection of incident vector I about normal N (assumes N is normalized)
template <typename V>
constexpr V reflect( const V &I, const V &N )
{
	return I - ( static_cast<typename V::value_type>( 2 ) * dot( I, N ) ) * N;
}

// Projection of vector A onto vector B. If B is zero, returns zero vector.
template <typename V>
constexpr V project( const V &A, const V &B )
{
	const auto denom = dot( B, B );
	if ( denom == 0 )
		return V{};
	return ( dot( A, B ) / denom ) * B;
}

// Component-wise min/max
template <typename V>
constexpr V min( const V &a, const V &b )
{
	if constexpr ( requires { a.w; } )
		return { std::min( a.x, b.x ), std::min( a.y, b.y ), std::min( a.z, b.z ), std::min( a.w, b.w ) };
	else if constexpr ( requires { a.z; } )
		return { std::min( a.x, b.x ), std::min( a.y, b.y ), std::min( a.z, b.z ) };
	else
		return { std::min( a.x, b.x ), std::min( a.y, b.y ) };
}

template <typename V>
constexpr V max( const V &a, const V &b )
{
	if constexpr ( requires { a.w; } )
		return { std::max( a.x, b.x ), std::max( a.y, b.y ), std::max( a.z, b.z ), std::max( a.w, b.w ) };
	else if constexpr ( requires { a.z; } )
		return { std::max( a.x, b.x ), std::max( a.y, b.y ), std::max( a.z, b.z ) };
	else
		return { std::max( a.x, b.x ), std::max( a.y, b.y ) };
}

// Approximate equality
template <typename V>
constexpr bool nearEqual( const V &a, const V &b, typename V::value_type eps = static_cast<typename V::value_type>( 1e-5 ) )
{
	if constexpr ( requires { a.w; } )
		return std::abs( a.x - b.x ) <= eps && std::abs( a.y - b.y ) <= eps && std::abs( a.z - b.z ) <= eps && std::abs( a.w - b.w ) <= eps;
	else if constexpr ( requires { a.z; } )
		return std::abs( a.x - b.x ) <= eps && std::abs( a.y - b.y ) <= eps && std::abs( a.z - b.z ) <= eps;
	else
		return std::abs( a.x - b.x ) <= eps && std::abs( a.y - b.y ) <= eps;
}

// SIMD specializations (SSE2+) - placed after generic templates
// SIMD specializations for float vectors (SSE2 only, using store-based reduction for portability)
#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
namespace detail
{
inline float hsum2_xy( __m128 v )
{
	alignas( 16 ) float vals[4];
	_mm_storeu_ps( vals, v );
	return vals[0] + vals[1];
}
inline float hsum3_xyz( __m128 v )
{
	alignas( 16 ) float vals[4];
	_mm_storeu_ps( vals, v );
	return vals[0] + vals[1] + vals[2];
}
inline float hsum4_xyzw( __m128 v )
{
	alignas( 16 ) float vals[4];
	_mm_storeu_ps( vals, v );
	return vals[0] + vals[1] + vals[2] + vals[3];
}
} // namespace detail

export inline float dot( const Vec2<float> &a, const Vec2<float> &b ) noexcept
{
	const __m128 va = _mm_set_ps( 0.0f, 0.0f, a.y, a.x );
	const __m128 vb = _mm_set_ps( 0.0f, 0.0f, b.y, b.x );
	return detail::hsum2_xy( _mm_mul_ps( va, vb ) );
}

export inline float dot( const Vec3<float> &a, const Vec3<float> &b ) noexcept
{
	const __m128 va = _mm_set_ps( 0.0f, a.z, a.y, a.x );
	const __m128 vb = _mm_set_ps( 0.0f, b.z, b.y, b.x );
	return detail::hsum3_xyz( _mm_mul_ps( va, vb ) );
}

export inline float dot( const Vec4<float> &a, const Vec4<float> &b ) noexcept
{
	const __m128 va = _mm_set_ps( a.w, a.z, a.y, a.x );
	const __m128 vb = _mm_set_ps( b.w, b.z, b.y, b.x );
	return detail::hsum4_xyzw( _mm_mul_ps( va, vb ) );
}

// lengthSquared specializations
export inline float lengthSquared( const Vec2<float> &v ) noexcept
{
	return dot( v, v );
}
export inline float lengthSquared( const Vec3<float> &v ) noexcept
{
	return dot( v, v );
}
export inline float lengthSquared( const Vec4<float> &v ) noexcept
{
	return dot( v, v );
}

// length
export inline float length( const Vec2<float> &v ) noexcept
{
	return std::sqrt( lengthSquared( v ) );
}
export inline float length( const Vec3<float> &v ) noexcept
{
	return std::sqrt( lengthSquared( v ) );
}
export inline float length( const Vec4<float> &v ) noexcept
{
	return std::sqrt( lengthSquared( v ) );
}

// normalize
export inline Vec2<float> normalize( const Vec2<float> &v ) noexcept
{
	float ls = lengthSquared( v );
	if ( ls <= std::numeric_limits<float>::epsilon() )
		return {};
	float inv = 1.0f / std::sqrt( ls );
	return { v.x * inv, v.y * inv };
}
export inline Vec3<float> normalize( const Vec3<float> &v ) noexcept
{
	float ls = lengthSquared( v );
	if ( ls <= std::numeric_limits<float>::epsilon() )
		return {};
	float inv = 1.0f / std::sqrt( ls );
	return { v.x * inv, v.y * inv, v.z * inv };
}
export inline Vec4<float> normalize( const Vec4<float> &v ) noexcept
{
	float ls = lengthSquared( v );
	if ( ls <= std::numeric_limits<float>::epsilon() )
		return {};
	float inv = 1.0f / std::sqrt( ls );
	return { v.x * inv, v.y * inv, v.z * inv, v.w * inv };
}
#endif // SIMD specializations

// Clamp each component of v to [minValue, maxValue]
template <typename V>
constexpr V clamp( const V &v, typename V::value_type minValue, typename V::value_type maxValue )
{
	const auto lo = minValue;
	const auto hi = maxValue;
	if constexpr ( requires { v.w; } )
		return { std::clamp( v.x, lo, hi ), std::clamp( v.y, lo, hi ), std::clamp( v.z, lo, hi ), std::clamp( v.w, lo, hi ) };
	else if constexpr ( requires { v.z; } )
		return { std::clamp( v.x, lo, hi ), std::clamp( v.y, lo, hi ), std::clamp( v.z, lo, hi ) };
	else
		return { std::clamp( v.x, lo, hi ), std::clamp( v.y, lo, hi ) };
}

// Clamp to [0,1]
template <typename V>
constexpr V saturate( const V &v )
{
	return clamp( v, static_cast<typename V::value_type>( 0 ), static_cast<typename V::value_type>( 1 ) );
}

// Angle between vectors (in radians). Returns 0 if any vector length is zero.
template <typename V>
inline auto angle( const V &a, const V &b )
{
	using VT = typename V::value_type;
	const auto la2 = lengthSquared( a );
	const auto lb2 = lengthSquared( b );
	if ( la2 == 0 || lb2 == 0 )
		return VT{ 0 };
	const long double denom = std::sqrt( static_cast<long double>( la2 ) ) * std::sqrt( static_cast<long double>( lb2 ) );
	long double c = static_cast<long double>( dot( a, b ) ) / denom;
	c = std::min<long double>( 1.0L, std::max<long double>( -1.0L, c ) ); // clamp
	return static_cast<VT>( std::acos( c ) );
}

// Spherical linear interpolation between vectors a and b (not necessarily normalized). If angle is tiny, falls back to lerp.
template <typename V, typename T>
inline V slerp( const V &a, const V &b, T t )
{
	using VT = typename V::value_type;
	// Normalize inputs for direction; retain magnitudes via linear interpolation of lengths if desired (here we slerp direction only then scale by lerped length)
	const auto lenA = length( a );
	const auto lenB = length( b );
	if ( lenA == 0 || lenB == 0 )
	{
		return lerp( a, b, t ); // degeneracy
	}
	const V na = normalize( a );
	const V nb = normalize( b );
	long double cosTheta = static_cast<long double>( dot( na, nb ) );
	cosTheta = std::min<long double>( 1.0L, std::max<long double>( -1.0L, cosTheta ) );
	const long double theta = std::acos( cosTheta );

	const long double tld = static_cast<long double>( t );
	const VT magBlend = static_cast<VT>( ( 1.0L - tld ) * lenA + tld * lenB );
	if ( theta < 1e-6L )
	{
		// Very small angle: linear blend and renormalize, then scale length
		const V blended = normalize( lerp( na, nb, t ) );
		return blended * magBlend;
	}
	const long double sinTheta = std::sin( theta );
	const long double w1 = std::sin( ( 1 - static_cast<long double>( t ) ) * theta ) / sinTheta;
	const long double w2 = std::sin( static_cast<long double>( t ) * theta ) / sinTheta;
	V dir{};
	dir.x = static_cast<VT>( na.x * w1 + nb.x * w2 );
	dir.y = static_cast<VT>( na.y * w1 + nb.y * w2 );
	if constexpr ( requires { na.z; } )
	{
		dir.z = static_cast<VT>( na.z * w1 + nb.z * w2 );
	}
	if constexpr ( requires { na.w; } )
	{
		dir.w = static_cast<VT>( na.w * w1 + nb.w * w2 );
	}
	return dir * magBlend;
}

} // namespace math

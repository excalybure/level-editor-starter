export module engine.math;
import std;

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

} // namespace math

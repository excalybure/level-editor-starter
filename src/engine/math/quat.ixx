export module engine.quat;
import std;
import engine.math;
import engine.vec;

export namespace math
{

// Quaternion implementation for 3D rotations
template <typename T = float>
struct Quat
{
	T w{ 1 }, x{}, y{}, z{}; // w is the scalar part, (x,y,z) is the vector part
	using value_type = T;

	// Constructors
	constexpr Quat() = default;
	constexpr Quat( T w_, T x_, T y_, T z_ ) : w{ w_ }, x{ x_ }, y{ y_ }, z{ z_ } {}

	// Constructor from axis-angle (axis should be normalized)
	constexpr Quat( const Vec3<T> &axis, T angle )
	{
		const T halfAngle = angle * static_cast<T>( 0.5 );
		const T s = std::sin( halfAngle );
		w = std::cos( halfAngle );
		x = axis.x * s;
		y = axis.y * s;
		z = axis.z * s;
	}

	// Constructor from Euler angles (yaw, pitch, roll in radians)
	constexpr Quat( T yaw, T pitch, T roll )
	{
		const T cy = std::cos( yaw * static_cast<T>( 0.5 ) );
		const T sy = std::sin( yaw * static_cast<T>( 0.5 ) );
		const T cp = std::cos( pitch * static_cast<T>( 0.5 ) );
		const T sp = std::sin( pitch * static_cast<T>( 0.5 ) );
		const T cr = std::cos( roll * static_cast<T>( 0.5 ) );
		const T sr = std::sin( roll * static_cast<T>( 0.5 ) );

		w = cr * cp * cy + sr * sp * sy;
		x = sr * cp * cy - cr * sp * sy;
		y = cr * sp * cy + sr * cp * sy;
		z = cr * cp * sy - sr * sp * cy;
	}

	// Arithmetic operators
	constexpr Quat operator+( const Quat &rhs ) const
	{
		return { w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z };
	}

	constexpr Quat operator-( const Quat &rhs ) const
	{
		return { w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z };
	}

	constexpr Quat operator*( T scalar ) const
	{
		return { w * scalar, x * scalar, y * scalar, z * scalar };
	}

	constexpr Quat operator/( T scalar ) const
	{
		return { w / scalar, x / scalar, y / scalar, z / scalar };
	}

	// Quaternion multiplication (composition of rotations)
	constexpr Quat operator*( const Quat &rhs ) const
	{
		return {
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
		};
	}

	// Assignment operators
	constexpr Quat &operator+=( const Quat &rhs )
	{
		w += rhs.w;
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	constexpr Quat &operator-=( const Quat &rhs )
	{
		w -= rhs.w;
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	constexpr Quat &operator*=( T scalar )
	{
		w *= scalar;
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	constexpr Quat &operator/=( T scalar )
	{
		w /= scalar;
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	constexpr Quat &operator*=( const Quat &rhs )
	{
		*this = *this * rhs;
		return *this;
	}

	// Unary operators
	constexpr Quat operator-() const
	{
		return { -w, -x, -y, -z };
	}

	// Quaternion operations
	constexpr T dot( const Quat &other ) const
	{
		return w * other.w + x * other.x + y * other.y + z * other.z;
	}

	constexpr T magnitude() const
	{
		return std::sqrt( w * w + x * x + y * y + z * z );
	}

	constexpr T magnitudeSquared() const
	{
		return w * w + x * x + y * y + z * z;
	}

	constexpr Quat normalized() const
	{
		const T mag = magnitude();
		if ( mag > std::numeric_limits<T>::epsilon() )
		{
			return *this / mag;
		}
		return { static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) };
	}

	constexpr void normalize()
	{
		*this = normalized();
	}

	// Quaternion conjugate (inverse rotation)
	constexpr Quat conjugate() const
	{
		return { w, -x, -y, -z };
	}

	// Quaternion inverse
	constexpr Quat inverse() const
	{
		const T magSq = magnitudeSquared();
		if ( magSq > std::numeric_limits<T>::epsilon() )
		{
			return conjugate() / magSq;
		}
		return { static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) };
	}

	// Rotate a vector by this quaternion
	constexpr Vec3<T> rotate( const Vec3<T> &v ) const
	{
		// Using the formula: v' = q * (0, v) * q^-1
		// Optimized version without creating quaternion for v
		const Vec3<T> qvec{ x, y, z };
		const Vec3<T> cross1 = cross( qvec, v );
		const Vec3<T> cross2 = cross( qvec, cross1 );
		return v + static_cast<T>( 2 ) * ( w * cross1 + cross2 );
	}

	// Convert to axis-angle representation
	constexpr std::pair<Vec3<T>, T> toAxisAngle() const
	{
		const T norm = std::sqrt( x * x + y * y + z * z );
		if ( norm > std::numeric_limits<T>::epsilon() )
		{
			const T angle = static_cast<T>( 2 ) * std::atan2( norm, w );
			const Vec3<T> axis{ x / norm, y / norm, z / norm };
			return { axis, angle };
		}
		return { Vec3<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ) }, static_cast<T>( 0 ) };
	}

	// Convert to Euler angles (yaw, pitch, roll in radians)
	constexpr Vec3<T> toEulerAngles() const
	{
		Vec3<T> angles;

		// Roll (x-axis rotation)
		const T sinr_cosp = static_cast<T>( 2 ) * ( w * x + y * z );
		const T cosr_cosp = static_cast<T>( 1 ) - static_cast<T>( 2 ) * ( x * x + y * y );
		angles.x = std::atan2( sinr_cosp, cosr_cosp );

		// Pitch (y-axis rotation)
		const T sinp = static_cast<T>( 2 ) * ( w * y - z * x );
		if ( std::abs( sinp ) >= static_cast<T>( 1 ) )
		{
			angles.y = std::copysign( static_cast<T>( 1.5707963267948966 ), sinp ); // π/2
		}
		else
		{
			angles.y = std::asin( sinp );
		}

		// Yaw (z-axis rotation)
		const T siny_cosp = static_cast<T>( 2 ) * ( w * z + x * y );
		const T cosy_cosp = static_cast<T>( 1 ) - static_cast<T>( 2 ) * ( y * y + z * z );
		angles.z = std::atan2( siny_cosp, cosy_cosp );

		return angles;
	}

	// Spherical linear interpolation
	constexpr Quat slerp( const Quat &other, T t ) const
	{
		T dot_product = dot( other );

		// If the dot product is negative, slerp won't take the shorter path
		// Note that we should use the quaternion -other instead of other
		Quat other_quat = other;
		if ( dot_product < static_cast<T>( 0 ) )
		{
			other_quat = -other;
			dot_product = -dot_product;
		}

		// If the inputs are too close for comfort, linearly interpolate and normalize
		if ( dot_product > static_cast<T>( 0.9995 ) )
		{
			return ( *this + ( other_quat - *this ) * t ).normalized();
		}

		// Calculate the half-angle between the quaternions
		const T theta_0 = std::acos( std::abs( dot_product ) );
		const T theta = theta_0 * t;
		const T sin_theta = std::sin( theta );
		const T sin_theta_0 = std::sin( theta_0 );

		const T s0 = std::cos( theta ) - dot_product * sin_theta / sin_theta_0;
		const T s1 = sin_theta / sin_theta_0;

		return ( *this * s0 ) + ( other_quat * s1 );
	}

	// Comparison operators
	constexpr bool operator==( const Quat &rhs ) const
	{
		return std::abs( w - rhs.w ) <= std::numeric_limits<T>::epsilon() &&
			std::abs( x - rhs.x ) <= std::numeric_limits<T>::epsilon() &&
			std::abs( y - rhs.y ) <= std::numeric_limits<T>::epsilon() &&
			std::abs( z - rhs.z ) <= std::numeric_limits<T>::epsilon();
	}

	constexpr bool operator!=( const Quat &rhs ) const
	{
		return !( *this == rhs );
	}
};

// Scalar-quaternion multiplication (commutative)
template <typename T>
constexpr Quat<T> operator*( T scalar, const Quat<T> &q )
{
	return q * scalar;
}

// Common quaternion factory functions
template <typename T = float>
constexpr Quat<T> quatIdentity()
{
	return Quat<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) };
}

template <typename T = float>
constexpr Quat<T> quatFromAxisAngle( const Vec3<T> &axis, T angle )
{
	return Quat<T>{ axis, angle };
}

template <typename T = float>
constexpr Quat<T> quatFromEulerAngles( T yaw, T pitch, T roll )
{
	return Quat<T>{ yaw, pitch, roll };
}

template <typename T = float>
constexpr Quat<T> quatLookRotation( const Vec3<T> &forward, const Vec3<T> &up = Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ) } )
{
	// Create a rotation that looks in the forward direction with the given up vector
	Vec3<T> f = normalize( forward );
	Vec3<T> r = normalize( cross( up, f ) );
	Vec3<T> u = cross( f, r );

	// Convert the rotation matrix to quaternion
	const T trace = r.x + u.y + f.z;

	if ( trace > static_cast<T>( 0 ) )
	{
		const T s = std::sqrt( trace + static_cast<T>( 1 ) ) * static_cast<T>( 2 ); // s = 4 * qw
		return Quat<T>{
			static_cast<T>( 0.25 ) * s,
			( u.z - f.y ) / s,
			( f.x - r.z ) / s,
			( r.y - u.x ) / s
		};
	}
	else if ( r.x > u.y && r.x > f.z )
	{
		const T s = std::sqrt( static_cast<T>( 1 ) + r.x - u.y - f.z ) * static_cast<T>( 2 ); // s = 4 * qx
		return Quat<T>{
			( u.z - f.y ) / s,
			static_cast<T>( 0.25 ) * s,
			( u.x + r.y ) / s,
			( f.x + r.z ) / s
		};
	}
	else if ( u.y > f.z )
	{
		const T s = std::sqrt( static_cast<T>( 1 ) + u.y - r.x - f.z ) * static_cast<T>( 2 ); // s = 4 * qy
		return Quat<T>{
			( f.x - r.z ) / s,
			( u.x + r.y ) / s,
			static_cast<T>( 0.25 ) * s,
			( f.y + u.z ) / s
		};
	}
	else
	{
		const T s = std::sqrt( static_cast<T>( 1 ) + f.z - r.x - u.y ) * static_cast<T>( 2 ); // s = 4 * qz
		return Quat<T>{
			( r.y - u.x ) / s,
			( f.x + r.z ) / s,
			( f.y + u.z ) / s,
			static_cast<T>( 0.25 ) * s
		};
	}
}

// Type aliases for common quaternion types
using Quatf = Quat<float>;
using Quatd = Quat<double>;

} // namespace math

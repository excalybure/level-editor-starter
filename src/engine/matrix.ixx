export module engine.matrix;
import std;
import engine.vec;

export namespace math
{
// Forward declarations for matrix-vector operations
template <typename T>
struct Mat2;

template <typename T>
struct Mat3;

template <typename T>
struct Mat4;

// 2x2 Matrix implementation
template <typename T = float>
struct Mat2
{
	// Matrix stored in row-major order: m[row][column]
	// [m00 m01]
	// [m10 m11]
	T m00{}, m01{}, m10{}, m11{};
	using value_type = T;

	// Constructors
	constexpr Mat2() = default;

	// Constructor with all elements
	constexpr Mat2( T m00_, T m01_, T m10_, T m11_ ) : m00{ m00_ }, m01{ m01_ }, m10{ m10_ }, m11{ m11_ } {}

	// Constructor from row vectors
	constexpr Mat2( const Vec2<T> &row0, const Vec2<T> &row1 ) : m00{ row0.x }, m01{ row0.y }, m10{ row1.x }, m11{ row1.y } {}

	// Identity matrix constructor
	static constexpr Mat2 identity()
	{
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Scale matrix constructor
	static constexpr Mat2 scale( T sx, T sy )
	{
		return {
			sx, static_cast<T>( 0 ), static_cast<T>( 0 ), sy
		};
	}

	// Rotation matrix constructor (angle in radians)
	static Mat2 rotation( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			c, -s, s, c
		};
	}

	// Matrix arithmetic operations
	constexpr Mat2 operator+( const Mat2 &rhs ) const
	{
		return {
			m00 + rhs.m00, m01 + rhs.m01, m10 + rhs.m10, m11 + rhs.m11
		};
	}

	constexpr Mat2 operator-( const Mat2 &rhs ) const
	{
		return {
			m00 - rhs.m00, m01 - rhs.m01, m10 - rhs.m10, m11 - rhs.m11
		};
	}

	constexpr Mat2 operator*( T scalar ) const
	{
		return {
			m00 * scalar, m01 * scalar, m10 * scalar, m11 * scalar
		};
	}

	constexpr Mat2 operator/( T scalar ) const
	{
		return {
			m00 / scalar, m01 / scalar, m10 / scalar, m11 / scalar
		};
	}

	// Matrix multiplication
	constexpr Mat2 operator*( const Mat2 &rhs ) const
	{
		return {
			m00 * rhs.m00 + m01 * rhs.m10, m00 * rhs.m01 + m01 * rhs.m11, m10 * rhs.m00 + m11 * rhs.m10, m10 * rhs.m01 + m11 * rhs.m11
		};
	}

	// Matrix-vector multiplication
	constexpr Vec2<T> operator*( const Vec2<T> &v ) const
	{
		return {
			m00 * v.x + m01 * v.y,
			m10 * v.x + m11 * v.y
		};
	}

	// Compound assignment operators
	constexpr Mat2 &operator+=( const Mat2 &rhs )
	{
		m00 += rhs.m00;
		m01 += rhs.m01;
		m10 += rhs.m10;
		m11 += rhs.m11;
		return *this;
	}

	constexpr Mat2 &operator-=( const Mat2 &rhs )
	{
		m00 -= rhs.m00;
		m01 -= rhs.m01;
		m10 -= rhs.m10;
		m11 -= rhs.m11;
		return *this;
	}

	constexpr Mat2 &operator*=( T scalar )
	{
		m00 *= scalar;
		m01 *= scalar;
		m10 *= scalar;
		m11 *= scalar;
		return *this;
	}

	constexpr Mat2 &operator/=( T scalar )
	{
		m00 /= scalar;
		m01 /= scalar;
		m10 /= scalar;
		m11 /= scalar;
		return *this;
	}

	constexpr Mat2 &operator*=( const Mat2 &rhs )
	{
		*this = ( *this ) * rhs;
		return *this;
	}

	// Matrix determinant
	constexpr T determinant() const
	{
		return m00 * m11 - m01 * m10;
	}

	// Matrix transpose
	constexpr Mat2 transpose() const
	{
		return {
			m00, m10, m01, m11
		};
	}

	// Matrix inverse
	constexpr Mat2 inverse() const
	{
		const T det = determinant();
		if ( std::abs( det ) <= std::numeric_limits<T>::epsilon() )
		{
			return Mat2{}; // Return zero matrix for non-invertible matrices
		}

		const T invDet = static_cast<T>( 1 ) / det;
		return {
			m11 * invDet, -m01 * invDet, -m10 * invDet, m00 * invDet
		};
	}
};

// Scalar-matrix multiplication (commutative property)
template <typename T>
constexpr Mat2<T> operator*( T scalar, const Mat2<T> &mat )
{
	return mat * scalar;
}

// 3x3 Matrix implementation
template <typename T = float>
struct Mat3
{
	// Matrix stored in row-major order: m[row][column]
	// [m00 m01 m02]
	// [m10 m11 m12]
	// [m20 m21 m22]
	T m00{}, m01{}, m02{},
		m10{}, m11{}, m12{},
		m20{}, m21{}, m22{};
	using value_type = T;

	// Constructors
	constexpr Mat3() = default;

	// Constructor with all elements
	constexpr Mat3( T m00_, T m01_, T m02_, T m10_, T m11_, T m12_, T m20_, T m21_, T m22_ )
		: m00{ m00_ }, m01{ m01_ }, m02{ m02_ },
		  m10{ m10_ }, m11{ m11_ }, m12{ m12_ },
		  m20{ m20_ }, m21{ m21_ }, m22{ m22_ } {}

	// Constructor from row vectors
	constexpr Mat3( const Vec3<T> &row0, const Vec3<T> &row1, const Vec3<T> &row2 )
		: m00{ row0.x }, m01{ row0.y }, m02{ row0.z },
		  m10{ row1.x }, m11{ row1.y }, m12{ row1.z },
		  m20{ row2.x }, m21{ row2.y }, m22{ row2.z } {}

	// Identity matrix constructor
	static constexpr Mat3 identity()
	{
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Scale matrix constructor
	static constexpr Mat3 scale( T sx, T sy, T sz )
	{
		return {
			sx, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), sy, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), sz
		};
	}

	// Rotation matrix constructors (angles in radians)
	static Mat3 rotationX( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), c, -s, static_cast<T>( 0 ), s, c
		};
	}

	static Mat3 rotationY( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			c, static_cast<T>( 0 ), s, static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), -s, static_cast<T>( 0 ), c
		};
	}

	static Mat3 rotationZ( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			c, -s, static_cast<T>( 0 ), s, c, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Translation matrix constructor (for 2D with homogeneous coordinates)
	static constexpr Mat3 translation( T tx, T ty )
	{
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), tx, static_cast<T>( 0 ), static_cast<T>( 1 ), ty, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Matrix arithmetic operations
	constexpr Mat3 operator+( const Mat3 &rhs ) const
	{
		return {
			m00 + rhs.m00, m01 + rhs.m01, m02 + rhs.m02, m10 + rhs.m10, m11 + rhs.m11, m12 + rhs.m12, m20 + rhs.m20, m21 + rhs.m21, m22 + rhs.m22
		};
	}

	constexpr Mat3 operator-( const Mat3 &rhs ) const
	{
		return {
			m00 - rhs.m00, m01 - rhs.m01, m02 - rhs.m02, m10 - rhs.m10, m11 - rhs.m11, m12 - rhs.m12, m20 - rhs.m20, m21 - rhs.m21, m22 - rhs.m22
		};
	}

	constexpr Mat3 operator*( T scalar ) const
	{
		return {
			m00 * scalar, m01 * scalar, m02 * scalar, m10 * scalar, m11 * scalar, m12 * scalar, m20 * scalar, m21 * scalar, m22 * scalar
		};
	}

	constexpr Mat3 operator/( T scalar ) const
	{
		return {
			m00 / scalar, m01 / scalar, m02 / scalar, m10 / scalar, m11 / scalar, m12 / scalar, m20 / scalar, m21 / scalar, m22 / scalar
		};
	}

	// Matrix multiplication
	constexpr Mat3 operator*( const Mat3 &rhs ) const
	{
		return {
			m00 * rhs.m00 + m01 * rhs.m10 + m02 * rhs.m20,
			m00 * rhs.m01 + m01 * rhs.m11 + m02 * rhs.m21,
			m00 * rhs.m02 + m01 * rhs.m12 + m02 * rhs.m22,

			m10 * rhs.m00 + m11 * rhs.m10 + m12 * rhs.m20,
			m10 * rhs.m01 + m11 * rhs.m11 + m12 * rhs.m21,
			m10 * rhs.m02 + m11 * rhs.m12 + m12 * rhs.m22,

			m20 * rhs.m00 + m21 * rhs.m10 + m22 * rhs.m20,
			m20 * rhs.m01 + m21 * rhs.m11 + m22 * rhs.m21,
			m20 * rhs.m02 + m21 * rhs.m12 + m22 * rhs.m22
		};
	}

	// Matrix-vector multiplication
	constexpr Vec3<T> operator*( const Vec3<T> &v ) const
	{
		return {
			m00 * v.x + m01 * v.y + m02 * v.z,
			m10 * v.x + m11 * v.y + m12 * v.z,
			m20 * v.x + m21 * v.y + m22 * v.z
		};
	}

	// Compound assignment operators
	constexpr Mat3 &operator+=( const Mat3 &rhs )
	{
		m00 += rhs.m00;
		m01 += rhs.m01;
		m02 += rhs.m02;
		m10 += rhs.m10;
		m11 += rhs.m11;
		m12 += rhs.m12;
		m20 += rhs.m20;
		m21 += rhs.m21;
		m22 += rhs.m22;
		return *this;
	}

	constexpr Mat3 &operator-=( const Mat3 &rhs )
	{
		m00 -= rhs.m00;
		m01 -= rhs.m01;
		m02 -= rhs.m02;
		m10 -= rhs.m10;
		m11 -= rhs.m11;
		m12 -= rhs.m12;
		m20 -= rhs.m20;
		m21 -= rhs.m21;
		m22 -= rhs.m22;
		return *this;
	}

	constexpr Mat3 &operator*=( T scalar )
	{
		m00 *= scalar;
		m01 *= scalar;
		m02 *= scalar;
		m10 *= scalar;
		m11 *= scalar;
		m12 *= scalar;
		m20 *= scalar;
		m21 *= scalar;
		m22 *= scalar;
		return *this;
	}

	constexpr Mat3 &operator/=( T scalar )
	{
		m00 /= scalar;
		m01 /= scalar;
		m02 /= scalar;
		m10 /= scalar;
		m11 /= scalar;
		m12 /= scalar;
		m20 /= scalar;
		m21 /= scalar;
		m22 /= scalar;
		return *this;
	}

	constexpr Mat3 &operator*=( const Mat3 &rhs )
	{
		*this = ( *this ) * rhs;
		return *this;
	}

	// Matrix determinant
	constexpr T determinant() const
	{
		return m00 * ( m11 * m22 - m12 * m21 ) - m01 * ( m10 * m22 - m12 * m20 ) + m02 * ( m10 * m21 - m11 * m20 );
	}

	// Matrix transpose
	constexpr Mat3 transpose() const
	{
		return {
			m00, m10, m20, m01, m11, m21, m02, m12, m22
		};
	}

	// Matrix inverse
	constexpr Mat3 inverse() const
	{
		const T det = determinant();
		if ( std::abs( det ) <= std::numeric_limits<T>::epsilon() )
		{
			return Mat3{}; // Return zero matrix for non-invertible matrices
		}

		const T invDet = static_cast<T>( 1 ) / det;

		return {
			( m11 * m22 - m12 * m21 ) * invDet,
			( m02 * m21 - m01 * m22 ) * invDet,
			( m01 * m12 - m02 * m11 ) * invDet,

			( m12 * m20 - m10 * m22 ) * invDet,
			( m00 * m22 - m02 * m20 ) * invDet,
			( m02 * m10 - m00 * m12 ) * invDet,

			( m10 * m21 - m11 * m20 ) * invDet,
			( m01 * m20 - m00 * m21 ) * invDet,
			( m00 * m11 - m01 * m10 ) * invDet
		};
	}
};

// Scalar-matrix multiplication (commutative property)
template <typename T>
constexpr Mat3<T> operator*( T scalar, const Mat3<T> &mat )
{
	return mat * scalar;
}

// 4x4 Matrix implementation
template <typename T = float>
struct Mat4
{
	// Matrix stored in row-major order: m[row][column]
	// [m00 m01 m02 m03]
	// [m10 m11 m12 m13]
	// [m20 m21 m22 m23]
	// [m30 m31 m32 m33]
	T m00{}, m01{}, m02{}, m03{},
		m10{}, m11{}, m12{}, m13{},
		m20{}, m21{}, m22{}, m23{},
		m30{}, m31{}, m32{}, m33{};
	using value_type = T;

	// Constructors
	constexpr Mat4() = default;

	// Constructor with all elements
	constexpr Mat4( T m00_, T m01_, T m02_, T m03_, T m10_, T m11_, T m12_, T m13_, T m20_, T m21_, T m22_, T m23_, T m30_, T m31_, T m32_, T m33_ )
		: m00{ m00_ }, m01{ m01_ }, m02{ m02_ }, m03{ m03_ },
		  m10{ m10_ }, m11{ m11_ }, m12{ m12_ }, m13{ m13_ },
		  m20{ m20_ }, m21{ m21_ }, m22{ m22_ }, m23{ m23_ },
		  m30{ m30_ }, m31{ m31_ }, m32{ m32_ }, m33{ m33_ } {}

	// Constructor from row vectors
	constexpr Mat4( const Vec4<T> &row0, const Vec4<T> &row1, const Vec4<T> &row2, const Vec4<T> &row3 )
		: m00{ row0.x }, m01{ row0.y }, m02{ row0.z }, m03{ row0.w },
		  m10{ row1.x }, m11{ row1.y }, m12{ row1.z }, m13{ row1.w },
		  m20{ row2.x }, m21{ row2.y }, m22{ row2.z }, m23{ row2.w },
		  m30{ row3.x }, m31{ row3.y }, m32{ row3.z }, m33{ row3.w } {}

	// Identity matrix constructor
	static constexpr Mat4 identity()
	{
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Scale matrix constructor
	static constexpr Mat4 scale( T sx, T sy, T sz )
	{
		return {
			sx, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), sy, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), sz, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Rotation matrix constructors (angles in radians)
	static Mat4 rotationX( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), c, -s, static_cast<T>( 0 ), static_cast<T>( 0 ), s, c, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	static Mat4 rotationY( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			c, static_cast<T>( 0 ), s, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), -s, static_cast<T>( 0 ), c, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	static Mat4 rotationZ( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			c, -s, static_cast<T>( 0 ), static_cast<T>( 0 ), s, c, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Translation matrix constructor (for 3D with homogeneous coordinates)
	static constexpr Mat4 translation( T tx, T ty, T tz )
	{
		return {
			static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), tx, static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), ty, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), tz, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Perspective projection matrix
	static Mat4 perspective( T fovY, T aspect, T zNear, T zFar )
	{
		const T tanHalfFovY = static_cast<T>( std::tan( fovY / static_cast<T>( 2 ) ) );

		Mat4 result{};
		result.m00 = static_cast<T>( 1 ) / ( aspect * tanHalfFovY );
		result.m11 = static_cast<T>( 1 ) / tanHalfFovY;
		result.m22 = -( zFar + zNear ) / ( zFar - zNear );
		result.m23 = -( static_cast<T>( 2 ) * zFar * zNear ) / ( zFar - zNear );
		result.m32 = -static_cast<T>( 1 );

		return result;
	}

	// Orthographic projection matrix
	static constexpr Mat4 orthographic( T left, T right, T bottom, T top, T zNear, T zFar )
	{
		Mat4 result{};
		result.m00 = static_cast<T>( 2 ) / ( right - left );
		result.m11 = static_cast<T>( 2 ) / ( top - bottom );
		result.m22 = -static_cast<T>( 2 ) / ( zFar - zNear );
		result.m03 = -( right + left ) / ( right - left );
		result.m13 = -( top + bottom ) / ( top - bottom );
		result.m23 = -( zFar + zNear ) / ( zFar - zNear );
		result.m33 = static_cast<T>( 1 );

		return result;
	}

	// Look-at view matrix
	static Mat4 lookAt( const Vec3<T> &eye, const Vec3<T> &target, const Vec3<T> &up )
	{
		const Vec3<T> f = normalize( target - eye );
		const Vec3<T> r = normalize( cross( f, up ) );
		const Vec3<T> u = cross( r, f );

		return {
			r.x, r.y, r.z, -dot( r, eye ), u.x, u.y, u.z, -dot( u, eye ), -f.x, -f.y, -f.z, dot( f, eye ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 )
		};
	}

	// Matrix arithmetic operations
	constexpr Mat4 operator+( const Mat4 &rhs ) const
	{
		return {
			m00 + rhs.m00, m01 + rhs.m01, m02 + rhs.m02, m03 + rhs.m03, m10 + rhs.m10, m11 + rhs.m11, m12 + rhs.m12, m13 + rhs.m13, m20 + rhs.m20, m21 + rhs.m21, m22 + rhs.m22, m23 + rhs.m23, m30 + rhs.m30, m31 + rhs.m31, m32 + rhs.m32, m33 + rhs.m33
		};
	}

	constexpr Mat4 operator-( const Mat4 &rhs ) const
	{
		return {
			m00 - rhs.m00, m01 - rhs.m01, m02 - rhs.m02, m03 - rhs.m03, m10 - rhs.m10, m11 - rhs.m11, m12 - rhs.m12, m13 - rhs.m13, m20 - rhs.m20, m21 - rhs.m21, m22 - rhs.m22, m23 - rhs.m23, m30 - rhs.m30, m31 - rhs.m31, m32 - rhs.m32, m33 - rhs.m33
		};
	}

	constexpr Mat4 operator*( T scalar ) const
	{
		return {
			m00 * scalar, m01 * scalar, m02 * scalar, m03 * scalar, m10 * scalar, m11 * scalar, m12 * scalar, m13 * scalar, m20 * scalar, m21 * scalar, m22 * scalar, m23 * scalar, m30 * scalar, m31 * scalar, m32 * scalar, m33 * scalar
		};
	}

	constexpr Mat4 operator/( T scalar ) const
	{
		return {
			m00 / scalar, m01 / scalar, m02 / scalar, m03 / scalar, m10 / scalar, m11 / scalar, m12 / scalar, m13 / scalar, m20 / scalar, m21 / scalar, m22 / scalar, m23 / scalar, m30 / scalar, m31 / scalar, m32 / scalar, m33 / scalar
		};
	}

	// Matrix multiplication
	constexpr Mat4 operator*( const Mat4 &rhs ) const
	{
		return {
			m00 * rhs.m00 + m01 * rhs.m10 + m02 * rhs.m20 + m03 * rhs.m30,
			m00 * rhs.m01 + m01 * rhs.m11 + m02 * rhs.m21 + m03 * rhs.m31,
			m00 * rhs.m02 + m01 * rhs.m12 + m02 * rhs.m22 + m03 * rhs.m32,
			m00 * rhs.m03 + m01 * rhs.m13 + m02 * rhs.m23 + m03 * rhs.m33,

			m10 * rhs.m00 + m11 * rhs.m10 + m12 * rhs.m20 + m13 * rhs.m30,
			m10 * rhs.m01 + m11 * rhs.m11 + m12 * rhs.m21 + m13 * rhs.m31,
			m10 * rhs.m02 + m11 * rhs.m12 + m12 * rhs.m22 + m13 * rhs.m32,
			m10 * rhs.m03 + m11 * rhs.m13 + m12 * rhs.m23 + m13 * rhs.m33,

			m20 * rhs.m00 + m21 * rhs.m10 + m22 * rhs.m20 + m23 * rhs.m30,
			m20 * rhs.m01 + m21 * rhs.m11 + m22 * rhs.m21 + m23 * rhs.m31,
			m20 * rhs.m02 + m21 * rhs.m12 + m22 * rhs.m22 + m23 * rhs.m32,
			m20 * rhs.m03 + m21 * rhs.m13 + m22 * rhs.m23 + m23 * rhs.m33,

			m30 * rhs.m00 + m31 * rhs.m10 + m32 * rhs.m20 + m33 * rhs.m30,
			m30 * rhs.m01 + m31 * rhs.m11 + m32 * rhs.m21 + m33 * rhs.m31,
			m30 * rhs.m02 + m31 * rhs.m12 + m32 * rhs.m22 + m33 * rhs.m32,
			m30 * rhs.m03 + m31 * rhs.m13 + m32 * rhs.m23 + m33 * rhs.m33
		};
	}

	// Matrix-vector multiplication
	constexpr Vec4<T> operator*( const Vec4<T> &v ) const
	{
		return {
			m00 * v.x + m01 * v.y + m02 * v.z + m03 * v.w,
			m10 * v.x + m11 * v.y + m12 * v.z + m13 * v.w,
			m20 * v.x + m21 * v.y + m22 * v.z + m23 * v.w,
			m30 * v.x + m31 * v.y + m32 * v.z + m33 * v.w
		};
	}

	// Transform a 3D vector using this matrix (assuming w=1, ignoring output w)
	constexpr Vec3<T> transformPoint( const Vec3<T> &v ) const
	{
		const T w = m30 * v.x + m31 * v.y + m32 * v.z + m33;
		const T invW = static_cast<T>( 1 ) / w;
		return {
			( m00 * v.x + m01 * v.y + m02 * v.z + m03 ) * invW,
			( m10 * v.x + m11 * v.y + m12 * v.z + m13 ) * invW,
			( m20 * v.x + m21 * v.y + m22 * v.z + m23 ) * invW
		};
	}

	// Transform a 3D vector as a direction (w=0, no translation)
	constexpr Vec3<T> transformVector( const Vec3<T> &v ) const
	{
		return {
			m00 * v.x + m01 * v.y + m02 * v.z,
			m10 * v.x + m11 * v.y + m12 * v.z,
			m20 * v.x + m21 * v.y + m22 * v.z
		};
	}

	// Compound assignment operators
	constexpr Mat4 &operator+=( const Mat4 &rhs )
	{
		m00 += rhs.m00;
		m01 += rhs.m01;
		m02 += rhs.m02;
		m03 += rhs.m03;
		m10 += rhs.m10;
		m11 += rhs.m11;
		m12 += rhs.m12;
		m13 += rhs.m13;
		m20 += rhs.m20;
		m21 += rhs.m21;
		m22 += rhs.m22;
		m23 += rhs.m23;
		m30 += rhs.m30;
		m31 += rhs.m31;
		m32 += rhs.m32;
		m33 += rhs.m33;
		return *this;
	}

	constexpr Mat4 &operator-=( const Mat4 &rhs )
	{
		m00 -= rhs.m00;
		m01 -= rhs.m01;
		m02 -= rhs.m02;
		m03 -= rhs.m03;
		m10 -= rhs.m10;
		m11 -= rhs.m11;
		m12 -= rhs.m12;
		m13 -= rhs.m13;
		m20 -= rhs.m20;
		m21 -= rhs.m21;
		m22 -= rhs.m22;
		m23 -= rhs.m23;
		m30 -= rhs.m30;
		m31 -= rhs.m31;
		m32 -= rhs.m32;
		m33 -= rhs.m33;
		return *this;
	}

	constexpr Mat4 &operator*=( T scalar )
	{
		m00 *= scalar;
		m01 *= scalar;
		m02 *= scalar;
		m03 *= scalar;
		m10 *= scalar;
		m11 *= scalar;
		m12 *= scalar;
		m13 *= scalar;
		m20 *= scalar;
		m21 *= scalar;
		m22 *= scalar;
		m23 *= scalar;
		m30 *= scalar;
		m31 *= scalar;
		m32 *= scalar;
		m33 *= scalar;
		return *this;
	}

	constexpr Mat4 &operator/=( T scalar )
	{
		m00 /= scalar;
		m01 /= scalar;
		m02 /= scalar;
		m03 /= scalar;
		m10 /= scalar;
		m11 /= scalar;
		m12 /= scalar;
		m13 /= scalar;
		m20 /= scalar;
		m21 /= scalar;
		m22 /= scalar;
		m23 /= scalar;
		m30 /= scalar;
		m31 /= scalar;
		m32 /= scalar;
		m33 /= scalar;
		return *this;
	}

	constexpr Mat4 &operator*=( const Mat4 &rhs )
	{
		*this = ( *this ) * rhs;
		return *this;
	}

	// Matrix transpose
	constexpr Mat4 transpose() const
	{
		return {
			m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22, m32, m03, m13, m23, m33
		};
	}

	// Matrix determinant
	constexpr T determinant() const
	{
		// Using cofactor expansion along the first row
		const T s0 = m22 * m33 - m23 * m32;
		const T s1 = m21 * m33 - m23 * m31;
		const T s2 = m21 * m32 - m22 * m31;
		const T s3 = m20 * m33 - m23 * m30;
		const T s4 = m20 * m32 - m22 * m30;
		const T s5 = m20 * m31 - m21 * m30;

		return m00 * ( m11 * s0 - m12 * s1 + m13 * s2 ) -
			m01 * ( m10 * s0 - m12 * s3 + m13 * s4 ) +
			m02 * ( m10 * s1 - m11 * s3 + m13 * s5 ) -
			m03 * ( m10 * s2 - m11 * s4 + m12 * s5 );
	}

	// Matrix inverse (returns zero matrix if not invertible)
	Mat4 inverse() const
	{
		const T det = determinant();
		if ( std::abs( det ) <= std::numeric_limits<T>::epsilon() )
		{
			return Mat4{}; // Return zero matrix for non-invertible matrices
		}

		const T invDet = static_cast<T>( 1 ) / det;

		// Calculate cofactors and adjugate matrix
		const T c00 = ( m11 * ( m22 * m33 - m23 * m32 ) - m12 * ( m21 * m33 - m23 * m31 ) + m13 * ( m21 * m32 - m22 * m31 ) ) * invDet;
		const T c01 = -( m10 * ( m22 * m33 - m23 * m32 ) - m12 * ( m20 * m33 - m23 * m30 ) + m13 * ( m20 * m32 - m22 * m30 ) ) * invDet;
		const T c02 = ( m10 * ( m21 * m33 - m23 * m31 ) - m11 * ( m20 * m33 - m23 * m30 ) + m13 * ( m20 * m31 - m21 * m30 ) ) * invDet;
		const T c03 = -( m10 * ( m21 * m32 - m22 * m31 ) - m11 * ( m20 * m32 - m22 * m30 ) + m12 * ( m20 * m31 - m21 * m30 ) ) * invDet;

		const T c10 = -( m01 * ( m22 * m33 - m23 * m32 ) - m02 * ( m21 * m33 - m23 * m31 ) + m03 * ( m21 * m32 - m22 * m31 ) ) * invDet;
		const T c11 = ( m00 * ( m22 * m33 - m23 * m32 ) - m02 * ( m20 * m33 - m23 * m30 ) + m03 * ( m20 * m32 - m22 * m30 ) ) * invDet;
		const T c12 = -( m00 * ( m21 * m33 - m23 * m31 ) - m01 * ( m20 * m33 - m23 * m30 ) + m03 * ( m20 * m31 - m21 * m30 ) ) * invDet;
		const T c13 = ( m00 * ( m21 * m32 - m22 * m31 ) - m01 * ( m20 * m32 - m22 * m30 ) + m02 * ( m20 * m31 - m21 * m30 ) ) * invDet;

		const T c20 = ( m01 * ( m12 * m33 - m13 * m32 ) - m02 * ( m11 * m33 - m13 * m31 ) + m03 * ( m11 * m32 - m12 * m31 ) ) * invDet;
		const T c21 = -( m00 * ( m12 * m33 - m13 * m32 ) - m02 * ( m10 * m33 - m13 * m30 ) + m03 * ( m10 * m32 - m12 * m30 ) ) * invDet;
		const T c22 = ( m00 * ( m11 * m33 - m13 * m31 ) - m01 * ( m10 * m33 - m13 * m30 ) + m03 * ( m10 * m31 - m11 * m30 ) ) * invDet;
		const T c23 = -( m00 * ( m11 * m32 - m12 * m31 ) - m01 * ( m10 * m32 - m12 * m30 ) + m02 * ( m10 * m31 - m11 * m30 ) ) * invDet;

		const T c30 = -( m01 * ( m12 * m23 - m13 * m22 ) - m02 * ( m11 * m23 - m13 * m21 ) + m03 * ( m11 * m22 - m12 * m21 ) ) * invDet;
		const T c31 = ( m00 * ( m12 * m23 - m13 * m22 ) - m02 * ( m10 * m23 - m13 * m20 ) + m03 * ( m10 * m22 - m12 * m20 ) ) * invDet;
		const T c32 = -( m00 * ( m11 * m23 - m13 * m21 ) - m01 * ( m10 * m23 - m13 * m20 ) + m03 * ( m10 * m21 - m11 * m20 ) ) * invDet;
		const T c33 = ( m00 * ( m11 * m22 - m12 * m21 ) - m01 * ( m10 * m22 - m12 * m20 ) + m02 * ( m10 * m21 - m11 * m20 ) ) * invDet;

		return {
			c00, c01, c02, c03, c10, c11, c12, c13, c20, c21, c22, c23, c30, c31, c32, c33
		};
	}
};

// Scalar-matrix multiplication (commutative property)
template <typename T>
constexpr Mat4<T> operator*( T scalar, const Mat4<T> &mat )
{
	return mat * scalar;
}

// Utility function to convert degrees to radians
template <typename T>
constexpr T radians( T degrees )
{
	return degrees * static_cast<T>( 0.01745329251994329576923690768489 ); // π/180
}

// Utility function to convert radians to degrees
template <typename T>
constexpr T degrees( T radians )
{
	return radians * static_cast<T>( 57.295779513082320876798154814105 ); // 180/π
}

} // namespace math

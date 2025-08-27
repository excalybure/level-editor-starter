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
	// Matrix stored in row-major order as vectors
	// [row0.x row0.y]
	// [row1.x row1.y]
	Vec2<T> row0{}, row1{};
	using value_type = T;

	// Convenience accessors for compatibility
	T &m00() { return row0.x; }
	const T &m00() const { return row0.x; }
	T &m01() { return row0.y; }
	const T &m01() const { return row0.y; }
	T &m10() { return row1.x; }
	const T &m10() const { return row1.x; }
	T &m11() { return row1.y; }
	const T &m11() const { return row1.y; }

	// Constructors
	constexpr Mat2() = default;

	// Constructor with all elements
	constexpr Mat2( T m00_, T m01_, T m10_, T m11_ ) : row0{ m00_, m01_ }, row1{ m10_, m11_ } {}

	// Constructor from row vectors
	constexpr Mat2( const Vec2<T> &row0_, const Vec2<T> &row1_ ) : row0{ row0_ }, row1{ row1_ } {}

	// Identity matrix constructor
	static constexpr Mat2 identity()
	{
		return { Vec2<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ) }, Vec2<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ) } };
	}

	// Scale matrix constructor
	static constexpr Mat2 scale( T sx, T sy )
	{
		return { Vec2<T>{ sx, static_cast<T>( 0 ) }, Vec2<T>{ static_cast<T>( 0 ), sy } };
	}

	// Rotation matrix constructor (angle in radians)
	static Mat2 rotation( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return { Vec2<T>{ c, -s }, Vec2<T>{ s, c } };
	}

	// Matrix arithmetic operations
	constexpr Mat2 operator+( const Mat2 &rhs ) const
	{
		return { row0 + rhs.row0, row1 + rhs.row1 };
	}

	constexpr Mat2 operator-( const Mat2 &rhs ) const
	{
		return { row0 - rhs.row0, row1 - rhs.row1 };
	}

	constexpr Mat2 operator*( T scalar ) const
	{
		return { row0 * scalar, row1 * scalar };
	}

	constexpr Mat2 operator/( T scalar ) const
	{
		return { row0 / scalar, row1 / scalar };
	}

	// Matrix multiplication
	constexpr Mat2 operator*( const Mat2 &rhs ) const
	{
		return {
			Vec2<T>{ dot( row0, Vec2<T>{ rhs.row0.x, rhs.row1.x } ), dot( row0, Vec2<T>{ rhs.row0.y, rhs.row1.y } ) },
			Vec2<T>{ dot( row1, Vec2<T>{ rhs.row0.x, rhs.row1.x } ), dot( row1, Vec2<T>{ rhs.row0.y, rhs.row1.y } ) }
		};
	}

	// Matrix-vector multiplication
	constexpr Vec2<T> operator*( const Vec2<T> &v ) const
	{
		return { dot( row0, v ), dot( row1, v ) };
	}

	// Compound assignment operators
	constexpr Mat2 &operator+=( const Mat2 &rhs )
	{
		row0 += rhs.row0;
		row1 += rhs.row1;
		return *this;
	}

	constexpr Mat2 &operator-=( const Mat2 &rhs )
	{
		row0 -= rhs.row0;
		row1 -= rhs.row1;
		return *this;
	}

	constexpr Mat2 &operator*=( T scalar )
	{
		row0 *= scalar;
		row1 *= scalar;
		return *this;
	}

	constexpr Mat2 &operator/=( T scalar )
	{
		row0 /= scalar;
		row1 /= scalar;
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
		return row0.x * row1.y - row0.y * row1.x;
	}

	// Matrix transpose
	constexpr Mat2 transpose() const
	{
		return { Vec2<T>{ row0.x, row1.x }, Vec2<T>{ row0.y, row1.y } };
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
		return { Vec2<T>{ row1.y * invDet, -row0.y * invDet }, Vec2<T>{ -row1.x * invDet, row0.x * invDet } };
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
	// Matrix stored in row-major order as vectors
	// [row0.x row0.y row0.z]
	// [row1.x row1.y row1.z]
	// [row2.x row2.y row2.z]
	Vec3<T> row0{}, row1{}, row2{};
	using value_type = T;

	// Convenience accessors for compatibility
	T &m00() { return row0.x; }
	const T &m00() const { return row0.x; }
	T &m01() { return row0.y; }
	const T &m01() const { return row0.y; }
	T &m02() { return row0.z; }
	const T &m02() const { return row0.z; }
	T &m10() { return row1.x; }
	const T &m10() const { return row1.x; }
	T &m11() { return row1.y; }
	const T &m11() const { return row1.y; }
	T &m12() { return row1.z; }
	const T &m12() const { return row1.z; }
	T &m20() { return row2.x; }
	const T &m20() const { return row2.x; }
	T &m21() { return row2.y; }
	const T &m21() const { return row2.y; }
	T &m22() { return row2.z; }
	const T &m22() const { return row2.z; }

	// Constructors
	constexpr Mat3() = default;

	// Constructor with all elements
	constexpr Mat3( T m00_, T m01_, T m02_, T m10_, T m11_, T m12_, T m20_, T m21_, T m22_ )
		: row0{ m00_, m01_, m02_ }, row1{ m10_, m11_, m12_ }, row2{ m20_, m21_, m22_ } {}

	// Constructor from row vectors
	constexpr Mat3( const Vec3<T> &row0_, const Vec3<T> &row1_, const Vec3<T> &row2_ )
		: row0{ row0_ }, row1{ row1_ }, row2{ row2_ } {}

	// Identity matrix constructor
	static constexpr Mat3 identity()
	{
		return {
			Vec3<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Scale matrix constructor
	static constexpr Mat3 scale( T sx, T sy, T sz )
	{
		return {
			Vec3<T>{ sx, static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), sy, static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), sz }
		};
	}

	// Rotation matrix constructors (angles in radians)
	static Mat3 rotationX( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec3<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), c, -s },
			Vec3<T>{ static_cast<T>( 0 ), s, c }
		};
	}

	static Mat3 rotationY( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec3<T>{ c, static_cast<T>( 0 ), s },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ) },
			Vec3<T>{ -s, static_cast<T>( 0 ), c }
		};
	}

	static Mat3 rotationZ( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec3<T>{ c, -s, static_cast<T>( 0 ) },
			Vec3<T>{ s, c, static_cast<T>( 0 ) },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Translation matrix constructor (for 2D with homogeneous coordinates)
	static constexpr Mat3 translation( T tx, T ty )
	{
		return {
			Vec3<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), tx },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), ty },
			Vec3<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Matrix arithmetic operations
	constexpr Mat3 operator+( const Mat3 &rhs ) const
	{
		return { row0 + rhs.row0, row1 + rhs.row1, row2 + rhs.row2 };
	}

	constexpr Mat3 operator-( const Mat3 &rhs ) const
	{
		return { row0 - rhs.row0, row1 - rhs.row1, row2 - rhs.row2 };
	}

	constexpr Mat3 operator*( T scalar ) const
	{
		return { row0 * scalar, row1 * scalar, row2 * scalar };
	}

	constexpr Mat3 operator/( T scalar ) const
	{
		return { row0 / scalar, row1 / scalar, row2 / scalar };
	}

	// Matrix multiplication
	constexpr Mat3 operator*( const Mat3 &rhs ) const
	{
		Vec3<T> col0{ rhs.row0.x, rhs.row1.x, rhs.row2.x };
		Vec3<T> col1{ rhs.row0.y, rhs.row1.y, rhs.row2.y };
		Vec3<T> col2{ rhs.row0.z, rhs.row1.z, rhs.row2.z };

		return {
			Vec3<T>{ dot( row0, col0 ), dot( row0, col1 ), dot( row0, col2 ) },
			Vec3<T>{ dot( row1, col0 ), dot( row1, col1 ), dot( row1, col2 ) },
			Vec3<T>{ dot( row2, col0 ), dot( row2, col1 ), dot( row2, col2 ) }
		};
	}

	// Matrix-vector multiplication
	constexpr Vec3<T> operator*( const Vec3<T> &v ) const
	{
		return { dot( row0, v ), dot( row1, v ), dot( row2, v ) };
	}

	// Compound assignment operators
	constexpr Mat3 &operator+=( const Mat3 &rhs )
	{
		row0 += rhs.row0;
		row1 += rhs.row1;
		row2 += rhs.row2;
		return *this;
	}

	constexpr Mat3 &operator-=( const Mat3 &rhs )
	{
		row0 -= rhs.row0;
		row1 -= rhs.row1;
		row2 -= rhs.row2;
		return *this;
	}

	constexpr Mat3 &operator*=( T scalar )
	{
		row0 *= scalar;
		row1 *= scalar;
		row2 *= scalar;
		return *this;
	}

	constexpr Mat3 &operator/=( T scalar )
	{
		row0 /= scalar;
		row1 /= scalar;
		row2 /= scalar;
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
		return row0.x * ( row1.y * row2.z - row1.z * row2.y ) - row0.y * ( row1.x * row2.z - row1.z * row2.x ) + row0.z * ( row1.x * row2.y - row1.y * row2.x );
	}

	// Matrix transpose
	constexpr Mat3 transpose() const
	{
		return {
			Vec3<T>{ row0.x, row1.x, row2.x },
			Vec3<T>{ row0.y, row1.y, row2.y },
			Vec3<T>{ row0.z, row1.z, row2.z }
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
			Vec3<T>{
				( row1.y * row2.z - row1.z * row2.y ) * invDet,
				( row0.z * row2.y - row0.y * row2.z ) * invDet,
				( row0.y * row1.z - row0.z * row1.y ) * invDet },
			Vec3<T>{
				( row1.z * row2.x - row1.x * row2.z ) * invDet,
				( row0.x * row2.z - row0.z * row2.x ) * invDet,
				( row0.z * row1.x - row0.x * row1.z ) * invDet },
			Vec3<T>{
				( row1.x * row2.y - row1.y * row2.x ) * invDet,
				( row0.y * row2.x - row0.x * row2.y ) * invDet,
				( row0.x * row1.y - row0.y * row1.x ) * invDet }
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
	// Matrix stored in row-major order as vectors
	// [row0.x row0.y row0.z row0.w]
	// [row1.x row1.y row1.z row1.w]
	// [row2.x row2.y row2.z row2.w]
	// [row3.x row3.y row3.z row3.w]
	Vec4<T> row0{}, row1{}, row2{}, row3{};
	using value_type = T;

	// Convenience accessors for compatibility
	T &m00() { return row0.x; }
	const T &m00() const { return row0.x; }
	T &m01() { return row0.y; }
	const T &m01() const { return row0.y; }
	T &m02() { return row0.z; }
	const T &m02() const { return row0.z; }
	T &m03() { return row0.w; }
	const T &m03() const { return row0.w; }
	T &m10() { return row1.x; }
	const T &m10() const { return row1.x; }
	T &m11() { return row1.y; }
	const T &m11() const { return row1.y; }
	T &m12() { return row1.z; }
	const T &m12() const { return row1.z; }
	T &m13() { return row1.w; }
	const T &m13() const { return row1.w; }
	T &m20() { return row2.x; }
	const T &m20() const { return row2.x; }
	T &m21() { return row2.y; }
	const T &m21() const { return row2.y; }
	T &m22() { return row2.z; }
	const T &m22() const { return row2.z; }
	T &m23() { return row2.w; }
	const T &m23() const { return row2.w; }
	T &m30() { return row3.x; }
	const T &m30() const { return row3.x; }
	T &m31() { return row3.y; }
	const T &m31() const { return row3.y; }
	T &m32() { return row3.z; }
	const T &m32() const { return row3.z; }
	T &m33() { return row3.w; }
	const T &m33() const { return row3.w; }

	// Constructors
	constexpr Mat4() = default;

	// Constructor with all elements
	constexpr Mat4( T m00_, T m01_, T m02_, T m03_, T m10_, T m11_, T m12_, T m13_, T m20_, T m21_, T m22_, T m23_, T m30_, T m31_, T m32_, T m33_ )
		: row0{ m00_, m01_, m02_, m03_ }, row1{ m10_, m11_, m12_, m13_ }, row2{ m20_, m21_, m22_, m23_ }, row3{ m30_, m31_, m32_, m33_ } {}

	// Constructor from row vectors
	constexpr Mat4( const Vec4<T> &row0_, const Vec4<T> &row1_, const Vec4<T> &row2_, const Vec4<T> &row3_ )
		: row0{ row0_ }, row1{ row1_ }, row2{ row2_ }, row3{ row3_ } {}

	// Identity matrix constructor
	static constexpr Mat4 identity()
	{
		return {
			Vec4<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Scale matrix constructor
	static constexpr Mat4 scale( T sx, T sy, T sz )
	{
		return {
			Vec4<T>{ sx, static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), sy, static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), sz, static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Rotation matrix constructors (angles in radians)
	static Mat4 rotationX( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec4<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), c, -s, static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), s, c, static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	static Mat4 rotationY( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec4<T>{ c, static_cast<T>( 0 ), s, static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ -s, static_cast<T>( 0 ), c, static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	static Mat4 rotationZ( T angle )
	{
		const T c = static_cast<T>( std::cos( angle ) );
		const T s = static_cast<T>( std::sin( angle ) );
		return {
			Vec4<T>{ c, -s, static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ s, c, static_cast<T>( 0 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Translation matrix constructor (for 3D with homogeneous coordinates)
	static constexpr Mat4 translation( T tx, T ty, T tz )
	{
		return {
			Vec4<T>{ static_cast<T>( 1 ), static_cast<T>( 0 ), static_cast<T>( 0 ), tx },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 1 ), static_cast<T>( 0 ), ty },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ), tz },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Perspective projection matrix
	static Mat4 perspective( T fovY, T aspect, T zNear, T zFar )
	{
		const T tanHalfFovY = static_cast<T>( std::tan( fovY / static_cast<T>( 2 ) ) );

		Mat4 result{};
		result.row0.x = static_cast<T>( 1 ) / ( aspect * tanHalfFovY );
		result.row1.y = static_cast<T>( 1 ) / tanHalfFovY;
		result.row2.z = -( zFar + zNear ) / ( zFar - zNear );
		result.row2.w = -( static_cast<T>( 2 ) * zFar * zNear ) / ( zFar - zNear );
		result.row3.z = -static_cast<T>( 1 );

		return result;
	}

	// Orthographic projection matrix
	static constexpr Mat4 orthographic( T left, T right, T bottom, T top, T zNear, T zFar )
	{
		Mat4 result{};
		result.row0.x = static_cast<T>( 2 ) / ( right - left );
		result.row1.y = static_cast<T>( 2 ) / ( top - bottom );
		result.row2.z = -static_cast<T>( 2 ) / ( zFar - zNear );
		result.row0.w = -( right + left ) / ( right - left );
		result.row1.w = -( top + bottom ) / ( top - bottom );
		result.row2.w = -( zFar + zNear ) / ( zFar - zNear );
		result.row3.w = static_cast<T>( 1 );

		return result;
	}

	// Look-at view matrix
	static Mat4 lookAt( const Vec3<T> &eye, const Vec3<T> &target, const Vec3<T> &up )
	{
		const Vec3<T> f = normalize( target - eye );
		const Vec3<T> r = normalize( cross( f, up ) );
		const Vec3<T> u = cross( r, f );

		return {
			Vec4<T>{ r.x, r.y, r.z, -dot( r, eye ) },
			Vec4<T>{ u.x, u.y, u.z, -dot( u, eye ) },
			Vec4<T>{ -f.x, -f.y, -f.z, dot( f, eye ) },
			Vec4<T>{ static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) }
		};
	}

	// Matrix arithmetic operations
	constexpr Mat4 operator+( const Mat4 &rhs ) const
	{
		return { row0 + rhs.row0, row1 + rhs.row1, row2 + rhs.row2, row3 + rhs.row3 };
	}

	constexpr Mat4 operator-( const Mat4 &rhs ) const
	{
		return { row0 - rhs.row0, row1 - rhs.row1, row2 - rhs.row2, row3 - rhs.row3 };
	}

	constexpr Mat4 operator*( T scalar ) const
	{
		return { row0 * scalar, row1 * scalar, row2 * scalar, row3 * scalar };
	}

	constexpr Mat4 operator/( T scalar ) const
	{
		return { row0 / scalar, row1 / scalar, row2 / scalar, row3 / scalar };
	}

	// Matrix multiplication
	constexpr Mat4 operator*( const Mat4 &rhs ) const
	{
		Vec4<T> col0{ rhs.row0.x, rhs.row1.x, rhs.row2.x, rhs.row3.x };
		Vec4<T> col1{ rhs.row0.y, rhs.row1.y, rhs.row2.y, rhs.row3.y };
		Vec4<T> col2{ rhs.row0.z, rhs.row1.z, rhs.row2.z, rhs.row3.z };
		Vec4<T> col3{ rhs.row0.w, rhs.row1.w, rhs.row2.w, rhs.row3.w };

		return {
			Vec4<T>{ dot( row0, col0 ), dot( row0, col1 ), dot( row0, col2 ), dot( row0, col3 ) },
			Vec4<T>{ dot( row1, col0 ), dot( row1, col1 ), dot( row1, col2 ), dot( row1, col3 ) },
			Vec4<T>{ dot( row2, col0 ), dot( row2, col1 ), dot( row2, col2 ), dot( row2, col3 ) },
			Vec4<T>{ dot( row3, col0 ), dot( row3, col1 ), dot( row3, col2 ), dot( row3, col3 ) }
		};
	}

	// Matrix-vector multiplication
	constexpr Vec4<T> operator*( const Vec4<T> &v ) const
	{
		return { dot( row0, v ), dot( row1, v ), dot( row2, v ), dot( row3, v ) };
	}

	// Transform a 3D vector using this matrix (assuming w=1, ignoring output w)
	constexpr Vec3<T> transformPoint( const Vec3<T> &v ) const
	{
		Vec4<T> homogeneous{ v.x, v.y, v.z, static_cast<T>( 1 ) };
		Vec4<T> result = *this * homogeneous;
		const T invW = static_cast<T>( 1 ) / result.w;
		return { result.x * invW, result.y * invW, result.z * invW };
	}

	// Transform a 3D vector as a direction (w=0, no translation)
	constexpr Vec3<T> transformVector( const Vec3<T> &v ) const
	{
		Vec4<T> direction{ v.x, v.y, v.z, static_cast<T>( 0 ) };
		Vec4<T> result = *this * direction;
		return { result.x, result.y, result.z };
	}

	// Compound assignment operators
	constexpr Mat4 &operator+=( const Mat4 &rhs )
	{
		row0 += rhs.row0;
		row1 += rhs.row1;
		row2 += rhs.row2;
		row3 += rhs.row3;
		return *this;
	}

	constexpr Mat4 &operator-=( const Mat4 &rhs )
	{
		row0 -= rhs.row0;
		row1 -= rhs.row1;
		row2 -= rhs.row2;
		row3 -= rhs.row3;
		return *this;
	}

	constexpr Mat4 &operator*=( T scalar )
	{
		row0 *= scalar;
		row1 *= scalar;
		row2 *= scalar;
		row3 *= scalar;
		return *this;
	}

	constexpr Mat4 &operator/=( T scalar )
	{
		row0 /= scalar;
		row1 /= scalar;
		row2 /= scalar;
		row3 /= scalar;
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
		Vec4<T>{ row0.x, row1.x, row2.x, row3.x },
		Vec4<T>{ row0.y, row1.y, row2.y, row3.y },
		Vec4<T>{ row0.z, row1.z, row2.z, row3.z },
		Vec4<T>{ row0.w, row1.w, row2.w, row3.w }
	};
}

// Matrix determinant
constexpr T determinant() const
{
	// Using cofactor expansion along the first row
	const T s0 = row2.z * row3.w - row2.w * row3.z;
	const T s1 = row2.y * row3.w - row2.w * row3.y;
	const T s2 = row2.y * row3.z - row2.z * row3.y;
	const T s3 = row2.x * row3.w - row2.w * row3.x;
	const T s4 = row2.x * row3.z - row2.z * row3.x;
	const T s5 = row2.x * row3.y - row2.y * row3.x;

	return row0.x * ( row1.y * s0 - row1.z * s1 + row1.w * s2 ) -
		row0.y * ( row1.x * s0 - row1.z * s3 + row1.w * s4 ) +
		row0.z * ( row1.x * s1 - row1.y * s3 + row1.w * s5 ) -
		row0.w * ( row1.x * s2 - row1.y * s4 + row1.z * s5 );
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
	const T c00 = ( row1.y * ( row2.z * row3.w - row2.w * row3.z ) - row1.z * ( row2.y * row3.w - row2.w * row3.y ) + row1.w * ( row2.y * row3.z - row2.z * row3.y ) ) * invDet;
	const T c01 = -( row1.x * ( row2.z * row3.w - row2.w * row3.z ) - row1.z * ( row2.x * row3.w - row2.w * row3.x ) + row1.w * ( row2.x * row3.z - row2.z * row3.x ) ) * invDet;
	const T c02 = ( row1.x * ( row2.y * row3.w - row2.w * row3.y ) - row1.y * ( row2.x * row3.w - row2.w * row3.x ) + row1.w * ( row2.x * row3.y - row2.y * row3.x ) ) * invDet;
	const T c03 = -( row1.x * ( row2.y * row3.z - row2.z * row3.y ) - row1.y * ( row2.x * row3.z - row2.z * row3.x ) + row1.z * ( row2.x * row3.y - row2.y * row3.x ) ) * invDet;

	const T c10 = -( row0.y * ( row2.z * row3.w - row2.w * row3.z ) - row0.z * ( row2.y * row3.w - row2.w * row3.y ) + row0.w * ( row2.y * row3.z - row2.z * row3.y ) ) * invDet;
	const T c11 = ( row0.x * ( row2.z * row3.w - row2.w * row3.z ) - row0.z * ( row2.x * row3.w - row2.w * row3.x ) + row0.w * ( row2.x * row3.z - row2.z * row3.x ) ) * invDet;
	const T c12 = -( row0.x * ( row2.y * row3.w - row2.w * row3.y ) - row0.y * ( row2.x * row3.w - row2.w * row3.x ) + row0.w * ( row2.x * row3.y - row2.y * row3.x ) ) * invDet;
	const T c13 = ( row0.x * ( row2.y * row3.z - row2.z * row3.y ) - row0.y * ( row2.x * row3.z - row2.z * row3.x ) + row0.z * ( row2.x * row3.y - row2.y * row3.x ) ) * invDet;

	const T c20 = ( row0.y * ( row1.z * row3.w - row1.w * row3.z ) - row0.z * ( row1.y * row3.w - row1.w * row3.y ) + row0.w * ( row1.y * row3.z - row1.z * row3.y ) ) * invDet;
	const T c21 = -( row0.x * ( row1.z * row3.w - row1.w * row3.z ) - row0.z * ( row1.x * row3.w - row1.w * row3.x ) + row0.w * ( row1.x * row3.z - row1.z * row3.x ) ) * invDet;
	const T c22 = ( row0.x * ( row1.y * row3.w - row1.w * row3.y ) - row0.y * ( row1.x * row3.w - row1.w * row3.x ) + row0.w * ( row1.x * row3.y - row1.y * row3.x ) ) * invDet;
	const T c23 = -( row0.x * ( row1.y * row3.z - row1.z * row3.y ) - row0.y * ( row1.x * row3.z - row1.z * row3.x ) + row0.z * ( row1.x * row3.y - row1.y * row3.x ) ) * invDet;

	const T c30 = -( row0.y * ( row1.z * row2.w - row1.w * row2.z ) - row0.z * ( row1.y * row2.w - row1.w * row2.y ) + row0.w * ( row1.y * row2.z - row1.z * row2.y ) ) * invDet;
	const T c31 = ( row0.x * ( row1.z * row2.w - row1.w * row2.z ) - row0.z * ( row1.x * row2.w - row1.w * row2.x ) + row0.w * ( row1.x * row2.z - row1.z * row2.x ) ) * invDet;
	const T c32 = -( row0.x * ( row1.y * row2.w - row1.w * row2.y ) - row0.y * ( row1.x * row2.w - row1.w * row2.x ) + row0.w * ( row1.x * row2.y - row1.y * row2.x ) ) * invDet;
	const T c33 = ( row0.x * ( row1.y * row2.z - row1.z * row2.y ) - row0.y * ( row1.x * row2.z - row1.z * row2.x ) + row0.z * ( row1.x * row2.y - row1.y * row2.x ) ) * invDet;

	return {
		Vec4<T>{ c00, c01, c02, c03 },
		Vec4<T>{ c10, c11, c12, c13 },
		Vec4<T>{ c20, c21, c22, c23 },
		Vec4<T>{ c30, c31, c32, c33 }
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

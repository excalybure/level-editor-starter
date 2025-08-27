export module engine.math;

import std;

export namespace math
{

// Mathematical constants
template <typename T = double>
constexpr T pi = static_cast<T>( 3.141592653589793238462643383279502884L );

template <typename T = double>
constexpr T e = static_cast<T>( 2.718281828459045235360287471352662498L );

template <typename T = double>
constexpr T sqrt2 = static_cast<T>( 1.414213562373095048801688724209698079L );

template <typename T = double>
constexpr T sqrt3 = static_cast<T>( 1.732050807568877293527446341505872367L );

// Angle conversion
template <typename T>
constexpr T radians( T degrees )
{
	return degrees * static_cast<T>( 0.01745329251994329576923690768489 ); // π/180
}

template <typename T>
constexpr T degrees( T radians )
{
	return radians * static_cast<T>( 57.295779513082320876798154814105 ); // 180/π
}

// Linear interpolation
template <typename T>
constexpr T lerp( T a, T b, T t )
{
	return a + t * ( b - a );
}

// Clamping functions
template <typename T>
constexpr T clamp( T value, T minValue, T maxValue )
{
	return std::max( minValue, std::min( value, maxValue ) );
}

// Sign function
template <typename T>
constexpr T sign( T value )
{
	return ( value > static_cast<T>( 0 ) ) ? static_cast<T>( 1 ) :
		( value < static_cast<T>( 0 ) )	   ? static_cast<T>( -1 ) :
											 static_cast<T>( 0 );
}

// Absolute value
template <typename T>
constexpr T abs( T value )
{
	return std::abs( value );
}

// Square function
template <typename T>
constexpr T square( T value )
{
	return value * value;
}

// Power functions
template <typename T>
T pow( T base, T exponent )
{
	return std::pow( base, exponent );
}

template <typename T>
T sqrt( T value )
{
	return std::sqrt( value );
}

// Trigonometric functions
template <typename T>
T sin( T angle )
{
	return std::sin( angle );
}

template <typename T>
T cos( T angle )
{
	return std::cos( angle );
}

template <typename T>
T tan( T angle )
{
	return std::tan( angle );
}

template <typename T>
T asin( T value )
{
	return std::asin( value );
}

template <typename T>
T acos( T value )
{
	return std::acos( value );
}

template <typename T>
T atan( T value )
{
	return std::atan( value );
}

template <typename T>
T atan2( T y, T x )
{
	return std::atan2( y, x );
}

// Smoothing functions
template <typename T>
constexpr T smoothstep( T edge0, T edge1, T x )
{
	T t = clamp( ( x - edge0 ) / ( edge1 - edge0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) );
	return t * t * ( static_cast<T>( 3 ) - static_cast<T>( 2 ) * t );
}

template <typename T>
constexpr T smootherstep( T edge0, T edge1, T x )
{
	T t = clamp( ( x - edge0 ) / ( edge1 - edge0 ), static_cast<T>( 0 ), static_cast<T>( 1 ) );
	return t * t * t * ( t * ( t * static_cast<T>( 6 ) - static_cast<T>( 15 ) ) + static_cast<T>( 10 ) );
}

// Rounding functions
template <typename T>
T floor( T value )
{
	return std::floor( value );
}

template <typename T>
T ceil( T value )
{
	return std::ceil( value );
}

template <typename T>
T round( T value )
{
	return std::round( value );
}

template <typename T>
constexpr T frac( T value )
{
	return value - floor( value );
}

// Modulo function
template <typename T>
T mod( T a, T b )
{
	return std::fmod( a, b );
}

// Step function
template <typename T>
constexpr T step( T edge, T x )
{
	return x < edge ? static_cast<T>( 0 ) : static_cast<T>( 1 );
}

// Wrap function (wraps value to [0, max))
template <typename T>
constexpr T wrap( T value, T max )
{
	return mod( value, max );
}

// Is power of two
constexpr bool isPowerOfTwo( unsigned int value )
{
	return value != 0 && ( value & ( value - 1 ) ) == 0;
}

// Next power of two
constexpr unsigned int nextPowerOfTwo( unsigned int value )
{
	if ( value == 0 )
		return 1;

	// If the value is already a power of two, we want the next one
	if ( isPowerOfTwo( value ) )
		value++;

	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return ++value;
}

} // namespace math

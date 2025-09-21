#pragma once

#include "math.h"

namespace math
{
// ============================================================================
// QUADRATIC EASING FUNCTIONS
// ============================================================================

// Quadratic easing: f(t) = tÂ²
constexpr float easeInQuad( const float t ) noexcept
{
	return t * t;
}

constexpr float easeOutQuad( const float t ) noexcept
{
	const float u = 1.0f - t;
	return 1.0f - u * u;
}

constexpr float easeInOutQuad( const float t ) noexcept
{
	if ( t < 0.5f )
	{
		return 2.0f * t * t;
	}
	else
	{
		const float u = 1.0f - t;
		return 1.0f - 2.0f * u * u;
	}
}

// ============================================================================
// CUBIC EASING FUNCTIONS
// ============================================================================

// Cubic easing: f(t) = tÂ³
constexpr float easeInCubic( const float t ) noexcept
{
	return t * t * t;
}

constexpr float easeOutCubic( const float t ) noexcept
{
	const float u = 1.0f - t;
	return 1.0f - u * u * u;
}

constexpr float easeInOutCubic( const float t ) noexcept
{
	if ( t < 0.5f )
	{
		return 4.0f * t * t * t;
	}
	else
	{
		const float u = 1.0f - t;
		return 1.0f - 4.0f * u * u * u;
	}
}

// ============================================================================
// QUARTIC EASING FUNCTIONS
// ============================================================================

// Quartic easing: f(t) = tâ´
constexpr float easeInQuart( const float t ) noexcept
{
	const float t2 = t * t;
	return t2 * t2;
}

constexpr float easeOutQuart( const float t ) noexcept
{
	const float u = 1.0f - t;
	const float u2 = u * u;
	return 1.0f - u2 * u2;
}

constexpr float easeInOutQuart( const float t ) noexcept
{
	if ( t < 0.5f )
	{
		const float t2 = t * t;
		return 8.0f * t2 * t2;
	}
	else
	{
		const float u = 1.0f - t;
		const float u2 = u * u;
		return 1.0f - 8.0f * u2 * u2;
	}
}

// ============================================================================
// SINE EASING FUNCTIONS
// ============================================================================

// Sine easing: f(t) = 1 - cos(t * Ï€/2)
float easeInSine( const float t ) noexcept
{
	return 1.0f - math::cos( t * math::pi<float> * 0.5f );
}

float easeOutSine( const float t ) noexcept
{
	return math::sin( t * math::pi<float> * 0.5f );
}

float easeInOutSine( const float t ) noexcept
{
	return 0.5f * ( 1.0f - math::cos( t * math::pi<float> ) );
}

// ============================================================================
// BOUNCE EASING FUNCTIONS
// ============================================================================

// Helper function for bounce calculations
constexpr float bounceOut( const float t ) noexcept
{
	if ( t < 1.0f / 2.75f )
	{
		return 7.5625f * t * t;
	}
	else if ( t < 2.0f / 2.75f )
	{
		const float t2 = t - 1.5f / 2.75f;
		return 7.5625f * t2 * t2 + 0.75f;
	}
	else if ( t < 2.5f / 2.75f )
	{
		const float t2 = t - 2.25f / 2.75f;
		return 7.5625f * t2 * t2 + 0.9375f;
	}
	else
	{
		const float t2 = t - 2.625f / 2.75f;
		return 7.5625f * t2 * t2 + 0.984375f;
	}
}

constexpr float easeInBounce( const float t ) noexcept
{
	return 1.0f - bounceOut( 1.0f - t );
}

constexpr float easeOutBounce( const float t ) noexcept
{
	return bounceOut( t );
}

constexpr float easeInOutBounce( const float t ) noexcept
{
	if ( t < 0.5f )
	{
		return 0.5f * ( 1.0f - bounceOut( 1.0f - 2.0f * t ) );
	}
	else
	{
		return 0.5f * bounceOut( 2.0f * t - 1.0f ) + 0.5f;
	}
}

// ============================================================================
// ELASTIC EASING FUNCTIONS
// ============================================================================

// Elastic easing with configurable amplitude and period
float easeInElastic( const float t, const float amplitude = 1.0f, const float period = 0.3f ) noexcept
{
	if ( t == 0.0f || t == 1.0f )
		return t;

	const float s = period / 4.0f; // For sine wave
	const float t1 = t - 1.0f;
	return -( amplitude * math::pow( 2.0f, 10.0f * t1 ) * math::sin( ( t1 - s ) * 2.0f * math::pi<float> / period ) );
}

float easeOutElastic( const float t, const float amplitude = 1.0f, const float period = 0.3f ) noexcept
{
	if ( t == 0.0f || t == 1.0f )
		return t;

	const float s = period / 4.0f;
	return amplitude * math::pow( 2.0f, -10.0f * t ) * math::sin( ( t - s ) * 2.0f * math::pi<float> / period ) + 1.0f;
}

float easeInOutElastic( const float t, const float amplitude = 1.0f, const float period = 0.45f ) noexcept
{
	if ( t == 0.0f || t == 1.0f )
		return t;

	const float s = period / 4.0f;
	const float t1 = 2.0f * t - 1.0f;

	if ( t1 < 0.0f )
	{
		return -0.5f * amplitude * math::pow( 2.0f, 10.0f * t1 ) * math::sin( ( t1 - s ) * 2.0f * math::pi<float> / period );
	}
	else
	{
		return 0.5f * amplitude * math::pow( 2.0f, -10.0f * t1 ) * math::sin( ( t1 - s ) * 2.0f * math::pi<float> / period ) + 1.0f;
	}
}

// ============================================================================
// BACK EASING FUNCTIONS
// ============================================================================

// Back easing with configurable overshoot
constexpr float easeInBack( const float t, const float overshoot = 1.70158f ) noexcept
{
	return t * t * ( ( overshoot + 1.0f ) * t - overshoot );
}

constexpr float easeOutBack( const float t, const float overshoot = 1.70158f ) noexcept
{
	const float t1 = t - 1.0f;
	return t1 * t1 * ( ( overshoot + 1.0f ) * t1 + overshoot ) + 1.0f;
}

constexpr float easeInOutBack( const float t, const float overshoot = 1.70158f ) noexcept
{
	const float s = overshoot * 1.525f;
	if ( t < 0.5f )
	{
		const float t2 = 2.0f * t;
		return 0.5f * t2 * t2 * ( ( s + 1.0f ) * t2 - s );
	}
	else
	{
		const float t2 = 2.0f * t - 2.0f;
		return 0.5f * ( t2 * t2 * ( ( s + 1.0f ) * t2 + s ) + 2.0f );
	}
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Inverse linear interpolation: given a value between a and b, find t
constexpr float inverseLerp( const float a, const float b, const float value ) noexcept
{
	if ( math::abs( b - a ) < 1e-6f )
		return 0.0f; // Avoid division by zero
	return ( value - a ) / ( b - a );
}

// Remap a value from one range to another
constexpr float remap( const float value, const float oldMin, const float oldMax, const float newMin, const float newMax ) noexcept
{
	const float t = inverseLerp( oldMin, oldMax, value );
	return math::lerp( newMin, newMax, t );
}

// ============================================================================
// ANIMATION CURVE EVALUATION
// ============================================================================

// Generic easing function dispatcher
enum class EaseType
{
	Linear,
	InQuad,
	OutQuad,
	InOutQuad,
	InCubic,
	OutCubic,
	InOutCubic,
	InQuart,
	OutQuart,
	InOutQuart,
	InSine,
	OutSine,
	InOutSine,
	InBounce,
	OutBounce,
	InOutBounce,
	InElastic,
	OutElastic,
	InOutElastic,
	InBack,
	OutBack,
	InOutBack
};

// Evaluate any easing function by type
float ease( const EaseType type, const float t ) noexcept
{
	switch ( type )
	{
	case EaseType::Linear:
		return t;
	case EaseType::InQuad:
		return easeInQuad( t );
	case EaseType::OutQuad:
		return easeOutQuad( t );
	case EaseType::InOutQuad:
		return easeInOutQuad( t );
	case EaseType::InCubic:
		return easeInCubic( t );
	case EaseType::OutCubic:
		return easeOutCubic( t );
	case EaseType::InOutCubic:
		return easeInOutCubic( t );
	case EaseType::InQuart:
		return easeInQuart( t );
	case EaseType::OutQuart:
		return easeOutQuart( t );
	case EaseType::InOutQuart:
		return easeInOutQuart( t );
	case EaseType::InSine:
		return easeInSine( t );
	case EaseType::OutSine:
		return easeOutSine( t );
	case EaseType::InOutSine:
		return easeInOutSine( t );
	case EaseType::InBounce:
		return easeInBounce( t );
	case EaseType::OutBounce:
		return easeOutBounce( t );
	case EaseType::InOutBounce:
		return easeInOutBounce( t );
	case EaseType::InElastic:
		return easeInElastic( t, 1.0f, 0.3f );
	case EaseType::OutElastic:
		return easeOutElastic( t, 1.0f, 0.3f );
	case EaseType::InOutElastic:
		return easeInOutElastic( t, 1.0f, 0.3f );
	case EaseType::InBack:
		return easeInBack( t, 1.70158f );
	case EaseType::OutBack:
		return easeOutBack( t, 1.70158f );
	case EaseType::InOutBack:
		return easeInOutBack( t, 1.70158f );
	default:
		return t; // Default to linear
	}
}

} // namespace math

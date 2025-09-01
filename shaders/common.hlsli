// Common shader utilities and constants
// Copyright (c) 2025 Level Editor Project

#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// Common mathematical constants
#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define HALF_PI 1.57079632679
#define INV_PI 0.31830988618

// Common utility functions
float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

float2 saturate(float2 x)
{
    return clamp(x, 0.0, 1.0);
}

float3 saturate(float3 x)
{
    return clamp(x, 0.0, 1.0);
}

float4 saturate(float4 x)
{
    return clamp(x, 0.0, 1.0);
}

// Linear interpolation
float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float2 lerp(float2 a, float2 b, float t)
{
    return a + t * (b - a);
}

float3 lerp(float3 a, float3 b, float t)
{
    return a + t * (b - a);
}

float4 lerp(float4 a, float4 b, float t)
{
    return a + t * (b - a);
}

// Smooth step
float smoothstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

// Common color space conversions
float3 sRGBToLinear(float3 sRGB)
{
    return pow(sRGB, 2.2);
}

float3 linearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0 / 2.2);
}

// Grid utility functions
float fwidth(float x)
{
    return abs(ddx(x)) + abs(ddy(x));
}

float2 fwidth(float2 x)
{
    return abs(ddx(x)) + abs(ddy(x));
}

// Common vertex layouts for different rendering needs
struct StandardVertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR;
};

struct SimpleVertex
{
    float3 position : POSITION;
    float4 color : COLOR;
};

// Common constant buffer slots
// b0 - Per-frame constants (view, projection, camera data)
// b1 - Per-object constants (world matrix, material properties)
// b2 - Lighting constants
// b3 - Material constants

// Common texture slots
// t0 - Primary texture (diffuse, albedo)
// t1 - Normal map
// t2 - Material properties (metallic, roughness, AO)
// t3 - Additional textures

// Common sampler slots
// s0 - Point sampling
// s1 - Linear sampling
// s2 - Anisotropic sampling
// s3 - Comparison sampling (for shadow maps)

#endif // COMMON_HLSLI

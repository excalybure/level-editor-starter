// Selection Rectangle Shader - Renders rectangle selection overlay
// For drag-to-select functionality
// Copyright (c) 2025 Level Editor Project

#include "common.hlsli"

// Vertex input structure for rectangle selection
struct VertexInput
{
    float2 position : POSITION;
};

// Vertex output / Pixel input structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Rectangle selection constants
cbuffer RectConstants : register(b0)
{
    float4 rectColor;           // Selection rectangle color (RGBA)
    float4 rectBounds;          // minX, minY, maxX, maxY in normalized screen coordinates
    float4 styleParams;         // borderWidth, fadeAlpha, animTime, padding
};

// Vertex Shader - Transforms screen-space quad
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.uv = input.position * 0.5f + 0.5f; // Convert to 0-1 range
    return output;
}

// Pixel Shader - Renders rectangle with border effect
float4 PSMain(VertexOutput input) : SV_TARGET
{
    float2 center = (rectBounds.xy + rectBounds.zw) * 0.5f;
    float2 size = rectBounds.zw - rectBounds.xy;
    
    // Calculate distance from edges for border effect
    float2 edgeDist = min(input.uv - rectBounds.xy, rectBounds.zw - input.uv);
    float borderWidth = styleParams.x;
    float edge = min(edgeDist.x, edgeDist.y);
    
    float4 color = rectColor;
    
    // Create border vs fill effect
    if (edge < borderWidth)
    {
        color.a = 0.8f; // Solid border
    }
    else
    {
        color.a = 0.2f; // Transparent fill
    }
    
    // Apply fade animation if specified
    color.a *= styleParams.y;
    
    return color;
}
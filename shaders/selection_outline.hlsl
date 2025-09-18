// Selection Outline Shader - Renders outlines for selected objects
// Uses vertex expansion technique to create outline effect
// Copyright (c) 2025 Level Editor Project

#include "common.hlsli"

// Vertex input structure for selection outline rendering
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

// Vertex output / Pixel input structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
};

// Outline rendering constants
cbuffer OutlineConstants : register(b0)
{
    float4x4 worldViewProj;     // Combined world-view-projection matrix
    float4 outlineColor;        // Selection outline color (RGBA)
    float4 screenParams;        // width, height, outlineWidth, time
};

// Vertex Shader - Expands vertices along normals for outline effect
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    
    // Expand vertices along normals for outline effect
    // Scale expansion by outline width and distance to maintain consistent thickness
    float3 expandedPos = input.position + input.normal * screenParams.z * 0.01f;
    
    // Transform to clip space
    output.position = mul(float4(expandedPos, 1.0f), worldViewProj);
    output.worldPos = input.position;
    output.normal = input.normal;
    
    return output;
}

// Pixel Shader - Renders outline with optional animation
float4 PSMain(VertexOutput input) : SV_TARGET
{
    float4 color = outlineColor;
    
    // Simple animation pulse effect using time parameter
    float pulse = sin(screenParams.w * PI) * 0.2f + 0.8f;
    color.rgb *= pulse;
    
    return color;
}
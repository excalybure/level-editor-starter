// Grid Shader - Infinite world-space grid rendering
// Copyright (c) 2025 Level Editor Project

#include "common.hlsli"

// Vertex shader input for fullscreen quad
struct VertexInput
{
    uint vertexID : SV_VertexID;
};

// Vertex shader output / Pixel shader input
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 viewDir : VIEW_DIRECTION;
};

// Grid rendering parameters
cbuffer GridConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 invViewProjMatrix;
    
    float3 cameraPosition;
    float gridScale;
    
    float3 majorGridColor;
    float majorGridAlpha;
    
    float3 minorGridColor;
    float minorGridAlpha;
    
    float3 axisXColor;
    float axisXAlpha;
    
    float3 axisYColor;
    float axisYAlpha;
    
    float3 axisZColor;
    float axisZAlpha;
    
    float fadeDistance;
    float gridSpacing;
    float majorGridInterval;
    float nearPlane;
    
    float farPlane;
    int showGrid;
    int showAxes;
    float axisThickness;
};

// Vertex Shader - Generate fullscreen quad
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    
    // Generate fullscreen triangle
    // vertexID 0: (-1, -1), vertexID 1: (3, -1), vertexID 2: (-1, 3)
    // This covers the entire screen with just 3 vertices
    float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    float2 clipPos = uv * 2.0f - 1.0f;
    
    output.position = float4(clipPos, 0.0f, 1.0f);
    
    // Calculate world position on the near plane
    float4 nearPlanePos = mul(invViewProjMatrix, float4(clipPos, 0.0f, 1.0f));
    nearPlanePos.xyz /= nearPlanePos.w;
    
    // Calculate world position on the far plane  
    float4 farPlanePos = mul(invViewProjMatrix, float4(clipPos, 1.0f, 1.0f));
    farPlanePos.xyz /= farPlanePos.w;
    
    // World position for ray casting
    output.worldPos = nearPlanePos.xyz;
    output.viewDir = normalize(farPlanePos.xyz - nearPlanePos.xyz);
    
    return output;
}

// Grid line computation functions
float gridLine(float coord, float spacing, float thickness)
{
    float grid = abs(frac(coord / spacing + 0.5) - 0.5) / fwidth(coord / spacing);
    return 1.0 - min(grid / thickness, 1.0);
}

float axisLine(float coord, float thickness)
{
    float axis = abs(coord) / fwidth(coord);
    return 1.0 - min(axis / thickness, 1.0);
}

// Calculate distance-based fade
float calculateFade(float3 worldPos, float3 cameraPos, float fadeDistance)
{
    float distance = length(worldPos - cameraPos);
    return saturate(1.0 - distance / fadeDistance);
}

// Pixel Shader - Render infinite grid
float4 PSMain(VertexOutput input) : SV_Target
{
    if (!showGrid)
    {
        discard;
    }
    
    // Ray-plane intersection with Z=0 plane (XY grid)
    float3 rayStart = input.worldPos;
    float3 rayDir = normalize(input.viewDir);
    
    // Check if ray hits the ground plane (Z = 0)
    if (abs(rayDir.z) < 0.0001) // Ray is parallel to plane
    {
        discard;
    }
    
    // Calculate intersection with Z = 0 plane
    float t = -rayStart.z / rayDir.z;
    if (t < 0) // Ray is pointing away from plane
    {
        discard;
    }
    
    // World position on the grid plane
    float3 worldPos = rayStart + t * rayDir;
    float2 gridPos = worldPos.xy;
    
    // Distance-based fade
    float fade = calculateFade(worldPos, cameraPosition, fadeDistance);
    if (fade < 0.01)
    {
        discard;
    }
    
    // Grid line calculations
    float minorGridThickness = 1.0;
    float majorGridThickness = 1.5;
    
    // Minor grid lines
    float minorX = gridLine(gridPos.x, gridSpacing, minorGridThickness);
    float minorY = gridLine(gridPos.y, gridSpacing, minorGridThickness);
    float minorGrid = max(minorX, minorY);
    
    // Major grid lines (every N minor lines)
    float majorX = gridLine(gridPos.x, gridSpacing * majorGridInterval, majorGridThickness);
    float majorY = gridLine(gridPos.y, gridSpacing * majorGridInterval, majorGridThickness);
    float majorGrid = max(majorX, majorY);
    
    // Axis lines (X and Y axes)
    float axisX = 0.0;
    float axisY = 0.0;
    
    if (showAxes)
    {
        axisX = axisLine(gridPos.y, axisThickness); // X-axis is where Y = 0
        axisY = axisLine(gridPos.x, axisThickness); // Y-axis is where X = 0
    }
    
    // Combine all grid elements with priority: Axes > Major > Minor
    float4 finalColor = float4(0, 0, 0, 0);
    
    // Minor grid
    if (minorGrid > 0.0)
    {
        finalColor = float4(minorGridColor, minorGridAlpha * minorGrid * fade);
    }
    
    // Major grid (overrides minor)
    if (majorGrid > 0.0)
    {
        finalColor = float4(majorGridColor, majorGridAlpha * majorGrid * fade);
    }
    
    // X-axis (red, overrides grid lines)
    if (axisX > 0.0)
    {
        finalColor = float4(axisXColor, axisXAlpha * axisX * fade);
    }
    
    // Y-axis (green, overrides grid lines)  
    if (axisY > 0.0)
    {
        finalColor = float4(axisYColor, axisYAlpha * axisY * fade);
    }
    
    // Discard if alpha is too low
    if (finalColor.a < 0.01)
    {
        discard;
    }
    
    return finalColor;
}

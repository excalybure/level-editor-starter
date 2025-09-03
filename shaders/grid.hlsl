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
    float2 clipPos : CLIP_POSITION;
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
    int viewType; // 0=Perspective, 1=Top, 2=Front, 3=Side
    float3 padding; // Ensure 16-byte alignment
};

// Grid line computation functions
float gridLine(const float coord, const float spacing, const float thickness)
{
    const float grid = abs(frac(coord / spacing + 0.5) - 0.5) / fwidth(coord / spacing);
    return 1.0 - min(grid / thickness, 1.0);
}

float axisLine(const float coord, const float thickness)
{
    const float axis = abs(coord) / fwidth(coord);
    return 1.0 - min(axis / thickness, 1.0);
}

// Calculate distance-based fade
float calculateFade(const float3 worldPos, const float3 cameraPos, const float fadeDistance)
{
    const float distance = length(worldPos - cameraPos);
    return saturate(1.0 - distance / fadeDistance);
}

// Vertex Shader - Generate fullscreen quad
VertexOutput VSMain(const VertexInput input)
{
    VertexOutput output;
    
    // Generate fullscreen triangle
    // vertexID 0: (-1, -1), vertexID 1: (3, -1), vertexID 2: (-1, 3)
    // This covers the entire screen with just 3 vertices
    const float2 uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    const float2 clipPos = uv * 2.0f - 1.0f;
    
    output.position = float4(clipPos, 0.0f, 1.0f);
    output.clipPos = clipPos;

    return output;
}

// Pixel Shader - Render infinite grid
float4 PSMain(const VertexOutput input) : SV_Target
{
    if (!showGrid)
    {
        discard;
    }
    
    // Calculate world position and view direction per-pixel for correct perspective
    const float2 clipPos = input.clipPos;
    
    // Calculate world position on the near plane
    float4 nearPlanePos = mul(invViewProjMatrix, float4(clipPos, 0.0f, 1.0f));
    nearPlanePos.xyz /= nearPlanePos.w;
    
    // Calculate world position on the far plane  
    float4 farPlanePos = mul(invViewProjMatrix, float4(clipPos, 1.0f, 1.0f));
    farPlanePos.xyz /= farPlanePos.w;
    
    // Ray parameters
    const float3 rayStart = nearPlanePos.xyz;
    const float3 rayDir = normalize(farPlanePos.xyz - nearPlanePos.xyz);
    
    float3 worldPos;
    
    // Handle different view types: 0=Perspective, 1=Top, 2=Front, 3=Side
    if (viewType == 0) // Perspective view - use ray-plane intersection
    {
        float t;
        // Default to XY plane intersection for perspective

        if (abs(rayDir.z) < 0.0001)
            discard; // Ray parallel to plane
        
        t = (0.0 - rayStart.z) / rayDir.z; // Intersect with Z=0 plane
        
        if (t < 0) // Ray pointing away from plane
        {
            discard;
        }
        
        worldPos = rayStart + t * rayDir;
    }
    else if (viewType == 1) // Top view (orthographic) - XY plane
    {
        // Project world position onto XY plane at Z=0
        worldPos = float3(rayStart.xy, 0.0);
    }
    else if (viewType == 2) // Front view (orthographic) - XZ plane
    {
        // Project world position onto XZ plane at Y=0
        worldPos = float3(rayStart.x, 0.0, rayStart.z);
    }
    else if (viewType == 3) // Side view (orthographic) - YZ plane
    {
        // Project world position onto YZ plane at X=0
        worldPos = float3(0.0, rayStart.yz);
    }
    
    // Extract 2D grid coordinates based on view type
    float2 gridPos;
    if (viewType == 0 || viewType == 1) // Perspective or Top view - XY plane
    {
        gridPos = worldPos.xy;
    }
    else if (viewType == 2) // Front view - XZ plane
    {
        gridPos = worldPos.xz;
    }
    else if (viewType == 3) // Side view - YZ plane
    {
        gridPos = worldPos.yz;
    }
    
    // Distance-based fade
    const float fade = calculateFade(worldPos, cameraPosition, fadeDistance);
    if (fade < 0.01)
    {
        discard;
    }
    
    // Grid line calculations
    const float minorGridThickness = 1.0;
    const float majorGridThickness = 1.5;
    
    // Minor grid lines
    const float minorX = gridLine(gridPos.x, gridSpacing, minorGridThickness);
    const float minorY = gridLine(gridPos.y, gridSpacing, minorGridThickness);
    const float minorGrid = max(minorX, minorY);
    
    // Major grid lines (every N minor lines)
    const float majorX = gridLine(gridPos.x, gridSpacing * majorGridInterval, majorGridThickness);
    const float majorY = gridLine(gridPos.y, gridSpacing * majorGridInterval, majorGridThickness);
    const float majorGrid = max(majorX, majorY);
    
    // Axis lines - mapped to appropriate world axes based on view type
    float axisFirst = 0.0;  // First axis of the 2D grid
    float axisSecond = 0.0; // Second axis of the 2D grid
    float3 firstAxisColor, secondAxisColor;
    float firstAxisAlpha, secondAxisAlpha;
    
    if (showAxes)
    {
        if (viewType == 0 || viewType == 1) // Perspective or Top view - XY plane
        {
            axisFirst = axisLine(gridPos.y, axisThickness);  // X-axis (red) where Y=0
            axisSecond = axisLine(gridPos.x, axisThickness); // Y-axis (green) where X=0
            firstAxisColor = axisXColor;
            firstAxisAlpha = axisXAlpha;
            secondAxisColor = axisYColor;
            secondAxisAlpha = axisYAlpha;
        }
        else if (viewType == 2) // Front view - XZ plane
        {
            axisFirst = axisLine(gridPos.y, axisThickness);  // X-axis (red) where Z=0
            axisSecond = axisLine(gridPos.x, axisThickness); // Z-axis (blue) where X=0
            firstAxisColor = axisXColor;
            firstAxisAlpha = axisXAlpha;
            secondAxisColor = axisZColor;
            secondAxisAlpha = axisZAlpha;
        }
        else if (viewType == 3) // Side view - YZ plane
        {
            axisFirst = axisLine(gridPos.y, axisThickness);  // Y-axis (green) where Z=0
            axisSecond = axisLine(gridPos.x, axisThickness); // Z-axis (blue) where Y=0
            firstAxisColor = axisYColor;
            firstAxisAlpha = axisYAlpha;
            secondAxisColor = axisZColor;
            secondAxisAlpha = axisZAlpha;
        }
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
    
    // First axis (overrides grid lines)
    if (axisFirst > 0.0)
    {
        finalColor = float4(firstAxisColor, firstAxisAlpha * axisFirst * fade);
    }
    
    // Second axis (overrides grid lines)  
    if (axisSecond > 0.0)
    {
        finalColor = float4(secondAxisColor, secondAxisAlpha * axisSecond * fade);
    }
    
    // Discard if alpha is too low
    if (finalColor.a < 0.01)
    {
        discard;
    }
    
    return finalColor;
}

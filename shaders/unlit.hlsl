// Unlit Shader - Basic 3D mesh rendering with base color support
// Copyright (c) 2025 Level Editor Project

#include "common.hlsli"

// Vertex input structure for 3D meshes
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent : TANGENT;
    float4 color : COLOR0;
};

// Vertex output / Pixel input structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent : TANGENT;
    float4 color : COLOR0;
};

// Per-frame constants (camera and view data) - using root constants for better performance
cbuffer FrameConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix;    // Combined view-projection matrix for efficiency
    float3 cameraPosition;
    float padding0;
};

// Per-object constants (world transform) - using root constants for better performance
cbuffer ObjectConstants : register(b1)
{
    float4x4 worldMatrix;
    float4x4 normalMatrix;      // World matrix for normals (inverse transpose)
};

// Material constants (PBR material properties)
cbuffer MaterialConstants : register(b2)
{
    float4 baseColorFactor;     // Base color tint (RGBA)
    float metallicFactor;       // Metallic factor [0-1]
    float roughnessFactor;      // Roughness factor [0-1]
    float padding1;
    float padding2;
    float3 emissiveFactor;      // Emissive color (RGB)
    float padding3;
    uint4 textureIndices;       // Bindless texture indices: [0]=baseColor, [1]=normal, [2]=metallicRoughness, [3]=emissive (UINT_MAX=invalid)
    uint textureFlags;          // Bitfield for texture availability
    uint3 padding4;             // Padding for 16-byte alignment
};

// Bindless texture array - accessed via indices stored in MaterialConstants.textureIndices
// Note: SM 5.1 has limited array size, real bindless requires SM 6.6+ with DXC compiler
// For now we use a fixed-size array as a transition step
Texture2D g_textures[4] : register(t0);  // Array of up to 4 textures per material

// Samplers
SamplerState linearSampler : register(s0);

// Texture index constants for accessing textureIndices array
#define TEXTURE_FLAG_BASE_COLOR         0
#define TEXTURE_FLAG_METALLIC_ROUGHNESS 1
#define TEXTURE_FLAG_NORMAL             2
#define TEXTURE_FLAG_EMISSIVE           3

// Texture flag bit masks (must match MaterialConstants in C++)
#define TEXTURE_FLAG_BASE_COLOR_MASK         (1u << TEXTURE_FLAG_BASE_COLOR)
#define TEXTURE_FLAG_METALLIC_ROUGHNESS_MASK (1u << TEXTURE_FLAG_METALLIC_ROUGHNESS)
#define TEXTURE_FLAG_NORMAL_MASK             (1u << TEXTURE_FLAG_NORMAL)
#define TEXTURE_FLAG_EMISSIVE_MASK           (1u << TEXTURE_FLAG_EMISSIVE)

// Vertex Shader
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    
    // Transform vertex position to world space
    float4 worldPos = mul(worldMatrix, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    
    // Transform to clip space
    output.position = mul(viewProjMatrix, worldPos);
    
    // Transform normal to world space (using normal matrix for non-uniform scaling)
    output.normal = normalize(mul((float3x3)normalMatrix, input.normal));
    
    // Pass through texture coordinates
    output.texcoord = input.texcoord;
    
    // Pass through tangent (for future use)
    output.tangent = input.tangent;
    
    // Pass through vertex color
    output.color = input.color;
    
    return output;
}

// Pixel Shader
float4 PSMain(VertexOutput input) : SV_TARGET
{
    // Sample base color and multiply with vertex color
    float4 baseColor = baseColorFactor * input.color;
    
    // Apply base color texture if available (using bindless texture indexing)
    if (textureFlags & TEXTURE_FLAG_BASE_COLOR_MASK)
    {
        const uint texIndex = textureIndices[TEXTURE_FLAG_BASE_COLOR];
        const float4 texColor = g_textures[texIndex].Sample(linearSampler, input.texcoord);
        baseColor *= texColor;
    }
    
    // Simple unlit shading - just return the base color
    // In the future, this could be extended with:
    // - Normal mapping
    // - Emissive contribution
    // - Simple lighting calculations
    
    // Apply emissive factor
    float3 emissive = emissiveFactor;
    if (textureFlags & TEXTURE_FLAG_EMISSIVE_MASK)
    {
        const uint texIndex = textureIndices[TEXTURE_FLAG_EMISSIVE];
        const float3 emissiveSample = g_textures[texIndex].Sample(linearSampler, input.texcoord).rgb;
        emissive *= emissiveSample;
    }
    
    // Combine base color with emissive
    const float3 finalColor = baseColor.rgb + emissive;
    
    // Return final color with alpha
    return float4(finalColor, baseColor.a);
}
// Simple shader - Basic rendering with view-projection matrix and vertex colors
// Used by the immediate-mode renderer for basic drawing operations

cbuffer FrameConstants : register(b0)
{
    float4x4 viewProjectionMatrix;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), viewProjectionMatrix);
    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}

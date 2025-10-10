// Test shader for material define compilation
// Expected defines from material system:
// GLOBAL_DEFINE (from global scope)
// PASS_DEFINE (from pass scope)  
// MATERIAL_DEFINE (from material scope)

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(float3 position : POSITION)
{
    PSInput result;
    
    // Use defines to ensure they're actually applied
    #ifdef GLOBAL_DEFINE
        result.position = float4(position, 1.0f);
    #else
        result.position = float4(0, 0, 0, 1.0f);
    #endif
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    #if defined(PASS_DEFINE) && defined(MATERIAL_DEFINE)
        return float4(1, 1, 1, 1);
    #else
        return float4(0, 0, 0, 1);
    #endif
}

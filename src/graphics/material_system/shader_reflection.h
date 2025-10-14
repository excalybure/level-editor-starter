#pragma once

#include "graphics/material_system/root_signature_builder.h"
#include "graphics/shader_manager/shader_manager.h"
#include <vector>
#include <d3d12.h>
#include <d3dcommon.h>

namespace graphics::material_system
{

// Result of shader reflection operation
struct ShaderResourceBindings
{
	std::vector<ResourceBinding> bindings;
	bool success = false;
};

// Shader reflection utility for extracting resource bindings from compiled shader bytecode
// Uses D3D12 shader reflection API to analyze shader resource requirements
class ShaderReflection
{
public:
	// Reflect on compiled shader bytecode to extract all resource bindings
	// Returns bindings for all CBVs, SRVs, UAVs, and Samplers used by the shader
	// @param blob - Compiled shader blob containing bytecode
	// @return ShaderResourceBindings containing extracted bindings and success status
	static ShaderResourceBindings Reflect( const shader_manager::ShaderBlob *blob );

private:
	// Map D3D shader input type to our ResourceBindingType enum
	// @param d3dType - D3D_SHADER_INPUT_TYPE from shader reflection
	// @return Corresponding ResourceBindingType, or CBV as default
	static ResourceBindingType MapBindingType( D3D_SHADER_INPUT_TYPE d3dType );
};

} // namespace graphics::material_system

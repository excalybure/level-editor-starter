#include "shader_reflection.h"
#include "core/console.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace graphics::material_system
{

ShaderResourceBindings ShaderReflection::Reflect( const shader_manager::ShaderBlob *blob )
{
	ShaderResourceBindings result;
	result.success = false;

	// Validate input
	if ( !blob || !blob->blob )
	{
		console::error( "ShaderReflection::Reflect: Invalid shader blob" );
		return result;
	}

	if ( !blob->isValid() )
	{
		console::error( "ShaderReflection::Reflect: Shader blob is not valid" );
		return result;
	}

	// Get bytecode pointer and size
	const void *bytecodeData = blob->blob->GetBufferPointer();
	const SIZE_T bytecodeSize = blob->blob->GetBufferSize();

	if ( !bytecodeData || bytecodeSize == 0 )
	{
		console::error( "ShaderReflection::Reflect: Shader blob has no bytecode data" );
		return result;
	}

	// Create reflection interface
	ComPtr<ID3D12ShaderReflection> reflection;
	const HRESULT hr = D3DReflect(
		bytecodeData,
		bytecodeSize,
		IID_PPV_ARGS( &reflection ) );

	if ( FAILED( hr ) )
	{
		console::error( "ShaderReflection::Reflect: D3DReflect failed with HRESULT={:#x}",
			static_cast<unsigned int>( hr ) );
		return result;
	}

	// Get shader description
	D3D12_SHADER_DESC shaderDesc = {};
	if ( FAILED( reflection->GetDesc( &shaderDesc ) ) )
	{
		console::error( "ShaderReflection::Reflect: Failed to get shader description" );
		return result;
	}

	// Extract all bound resources
	result.bindings.reserve( shaderDesc.BoundResources );

	for ( UINT i = 0; i < shaderDesc.BoundResources; ++i )
	{
		D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
		if ( FAILED( reflection->GetResourceBindingDesc( i, &bindDesc ) ) )
		{
			console::error( "ShaderReflection::Reflect: Failed to get resource binding desc for index {}", i );
			continue;
		}

		// Create resource binding from D3D12 description
		ResourceBinding binding;
		binding.name = bindDesc.Name;
		binding.type = MapBindingType( bindDesc.Type );
		binding.slot = static_cast<int>( bindDesc.BindPoint );

		// Log the binding for debugging
		console::info( "ShaderReflection: Found binding '{}' type={} slot={}",
			binding.name,
			static_cast<int>( binding.type ),
			binding.slot );

		result.bindings.push_back( binding );
	}

	result.success = true;
	console::info( "ShaderReflection: Successfully reflected shader with {} resource bindings",
		result.bindings.size() );

	return result;
}

ResourceBindingType ShaderReflection::MapBindingType( D3D_SHADER_INPUT_TYPE d3dType )
{
	switch ( d3dType )
	{
	case D3D_SIT_CBUFFER:
		return ResourceBindingType::CBV;

	case D3D_SIT_TEXTURE:
	case D3D_SIT_STRUCTURED:
	case D3D_SIT_BYTEADDRESS:
		return ResourceBindingType::SRV;

	case D3D_SIT_SAMPLER:
		return ResourceBindingType::Sampler;

	case D3D_SIT_UAV_RWTYPED:
	case D3D_SIT_UAV_RWSTRUCTURED:
	case D3D_SIT_UAV_RWBYTEADDRESS:
	case D3D_SIT_UAV_APPEND_STRUCTURED:
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		return ResourceBindingType::UAV;

	default:
		console::error( "ShaderReflection::MapBindingType: Unknown D3D_SHADER_INPUT_TYPE={}", static_cast<int>( d3dType ) );
		return ResourceBindingType::CBV; // Default fallback
	}
}

} // namespace graphics::material_system

#include "shader_reflection.h"
#include "core/console.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string_view>

using Microsoft::WRL::ComPtr;

namespace graphics::material_system
{

// ============================================================================
// ShaderReflection Implementation
// ============================================================================

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

// ============================================================================
// ShaderReflectionCache Implementation
// ============================================================================

ShaderResourceBindings ShaderReflectionCache::GetOrReflect(
	const shader_manager::ShaderBlob *blob,
	shader_manager::ShaderHandle handle )
{
	// Validate input
	if ( !blob || !blob->blob || !blob->isValid() )
	{
		console::error( "ShaderReflectionCache::GetOrReflect: Invalid shader blob" );
		++m_missCount;
		return ShaderResourceBindings{ {}, false };
	}

	// Compute bytecode hash
	const size_t hash = HashBytecode( blob );
	const CacheKey key{ hash };

	// Check cache
	const auto it = m_cache.find( key );
	if ( it != m_cache.end() )
	{
		// Cache hit
		++m_hitCount;
		return it->second;
	}

	// Cache miss - perform reflection
	++m_missCount;

	const auto bindings = ShaderReflection::Reflect( blob );

	// Cache the result if successful
	if ( bindings.success )
	{
		m_cache[key] = bindings;
		m_handleToKey[handle] = key;
	}

	return bindings;
}

void ShaderReflectionCache::Invalidate( shader_manager::ShaderHandle handle )
{
	// Find the cache key for this handle
	const auto handleIt = m_handleToKey.find( handle );
	if ( handleIt == m_handleToKey.end() )
	{
		// Handle not tracked (never cached or already invalidated)
		return;
	}

	const CacheKey key = handleIt->second;

	// Remove from cache
	const auto cacheIt = m_cache.find( key );
	if ( cacheIt != m_cache.end() )
	{
		m_cache.erase( cacheIt );
	}

	// Remove handle tracking
	m_handleToKey.erase( handleIt );
}

void ShaderReflectionCache::Clear()
{
	const size_t cacheSize = m_cache.size();
	const size_t handleCount = m_handleToKey.size();

	m_cache.clear();
	m_handleToKey.clear();
	m_hitCount = 0;
	m_missCount = 0;
}

size_t ShaderReflectionCache::HashBytecode( const shader_manager::ShaderBlob *blob )
{
	if ( !blob || !blob->blob )
	{
		return 0;
	}

	const void *data = blob->blob->GetBufferPointer();
	const size_t size = blob->blob->GetBufferSize();

	if ( !data || size == 0 )
	{
		return 0;
	}

	// Hash the bytecode content using std::hash on string_view
	// This provides a reasonably fast hash suitable for caching
	// For production, consider FNV-1a or xxHash for better performance
	const std::string_view bytecodeView(
		static_cast<const char *>( data ),
		size );

	return std::hash<std::string_view>{}( bytecodeView );
}

} // namespace graphics::material_system

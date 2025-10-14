#pragma once

#include "graphics/material_system/root_signature_builder.h"
#include "graphics/shader_manager/shader_manager.h"
#include <vector>
#include <unordered_map>
#include <functional>
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

// Cache for shader reflection results to avoid redundant reflection operations
// Uses bytecode hash as key to support shader hot-reloading
class ShaderReflectionCache
{
public:
	ShaderReflectionCache() = default;
	~ShaderReflectionCache() = default;

	// No copy/move (cache is stateful)
	ShaderReflectionCache( const ShaderReflectionCache & ) = delete;
	ShaderReflectionCache &operator=( const ShaderReflectionCache & ) = delete;

	// Get cached reflection or perform reflection and cache result
	// Uses bytecode content hash as key (supports hot-reload)
	// @param blob - Compiled shader blob to reflect
	// @param handle - Shader handle for tracking (used for invalidation)
	// @return Cached or newly reflected bindings
	ShaderResourceBindings GetOrReflect(
		const shader_manager::ShaderBlob *blob,
		shader_manager::ShaderHandle handle );

	// Invalidate cache entry for a specific shader handle
	// Called when shader is hot-reloaded with new bytecode
	// @param handle - Shader handle to invalidate
	void Invalidate( shader_manager::ShaderHandle handle );

	// Clear entire cache
	void Clear();

	// Get cache statistics
	size_t GetCacheSize() const { return m_cache.size(); }
	size_t GetHitCount() const { return m_hitCount; }
	size_t GetMissCount() const { return m_missCount; }

private:
	// Cache key based on bytecode content hash
	struct CacheKey
	{
		size_t bytecodeHash;

		bool operator==( const CacheKey &other ) const
		{
			return bytecodeHash == other.bytecodeHash;
		}
	};

	// Hash function for CacheKey
	struct CacheKeyHash
	{
		size_t operator()( const CacheKey &key ) const
		{
			return key.bytecodeHash;
		}
	};

	// Cache storage: bytecode hash -> reflection result
	std::unordered_map<CacheKey, ShaderResourceBindings, CacheKeyHash> m_cache;

	// Track shader handle -> bytecode hash for invalidation
	std::unordered_map<shader_manager::ShaderHandle, CacheKey> m_handleToKey;

	// Statistics
	size_t m_hitCount = 0;
	size_t m_missCount = 0;

	// Compute hash of shader bytecode content
	// @param blob - Shader blob to hash
	// @return Hash of bytecode bytes
	static size_t HashBytecode( const shader_manager::ShaderBlob *blob );
};

} // namespace graphics::material_system

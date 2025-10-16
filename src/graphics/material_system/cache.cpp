#include "graphics/material_system/cache.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/pso_builder.h"
#include "core/console.h"
#include "core/hash_utils.h"
#include <functional>

namespace graphics::material_system
{

// Compute stable hash for PSO cache key
// Combines material id, pass name, shader ids, and state ids
PSOHash computePSOHash( const MaterialDefinition &material, const std::string &passName, const RenderPassConfig &passConfig )
{
	std::hash<std::string> hasher;

	// Start with material id
	size_t hash = hasher( material.id );

	// Combine with pass name (multi-pass materials need different PSOs per pass)
	if ( !passName.empty() )
	{
		core::hash_combine( hash, passName );
	}

	// Combine with render pass config name
	core::hash_combine( hash, passConfig.name );

	// Query pass-specific data
	const MaterialPass *materialPass = nullptr;
	if ( !passName.empty() )
	{
		materialPass = material.getPass( passName );
		if ( !materialPass )
		{
			console::error( "Material '{}': pass '{}' not found", material.id, passName );
			return 0; // Return invalid hash
		}
	}
	else
	{
		// No pass name provided - must have at least one pass
		if ( material.passes.empty() )
		{
			console::error( "Material '{}': no passes defined", material.id );
			return 0; // Return invalid hash
		}
		materialPass = &material.passes[0]; // Use first pass as default
	}

	// Use pass-specific shaders and states
	const auto &shadersToHash = materialPass->shaders;
	for ( const auto &shaderRef : shadersToHash )
	{
		core::hash_combine( hash, shaderStageToString( shaderRef.stage ) );
		core::hash_combine( hash, shaderRef.shaderId );
	}

	// Combine state block ids from pass
	const StateReferences &statesToHash = materialPass->states;
	core::hash_combine( hash, statesToHash.rasterizer );
	core::hash_combine( hash, statesToHash.depthStencil );
	core::hash_combine( hash, statesToHash.blend );

	return hash;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineCache::get( PSOHash hash ) const
{
	std::shared_lock<std::shared_mutex> lock( m_mutex );
	const auto it = m_cache.find( hash );
	if ( it == m_cache.end() )
		return nullptr;

	return it->second.pso;
}

void PipelineCache::store( PSOHash hash, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso, const std::string &materialId, const std::string &passName )
{
	std::unique_lock<std::shared_mutex> lock( m_mutex );
	// Check for hash collision (same hash, different material/pass)
	const auto it = m_cache.find( hash );
	if ( it != m_cache.end() )
	{
		// Same material + pass = expected cache hit, just update
		if ( it->second.materialId == materialId && it->second.passName == passName )
		{
			it->second.pso = pso;
			return;
		}

		// Different material/pass with same hash = collision
		console::errorAndThrow( "PSO cache hash collision detected: material '{}' pass '{}' collides with '{}' pass '{}'",
			materialId,
			passName,
			it->second.materialId,
			it->second.passName );
		return;
	}

	// New entry
	CacheEntry entry;
	entry.pso = pso;
	entry.materialId = materialId;
	entry.passName = passName;
	m_cache[hash] = entry;
}

void PipelineCache::clear()
{
	std::unique_lock<std::shared_mutex> lock( m_mutex );
	m_cache.clear();
}

void PipelineCache::invalidateByMaterial( const std::string &materialId )
{
	std::unique_lock<std::shared_mutex> lock( m_mutex );

	// Iterate and remove all entries matching the material ID
	for ( auto it = m_cache.begin(); it != m_cache.end(); )
	{
		if ( it->second.materialId == materialId )
		{
			it = m_cache.erase( it );
		}
		else
		{
			++it;
		}
	}
}

} // namespace graphics::material_system

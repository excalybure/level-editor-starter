#include "graphics/material_system/cache.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/pipeline_builder.h"
#include "core/console.h"
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
		hash ^= hasher( passName ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
	}

	// Combine with render pass config name
	hash ^= hasher( passConfig.name ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	// Query pass-specific data if passName provided
	const MaterialPass *materialPass = nullptr;
	if ( !passName.empty() )
	{
		materialPass = material.getPass( passName );
	}

	// Combine shader ids (use pass-specific or material shaders)
	const auto &shadersToHash = materialPass ? materialPass->shaders : material.shaders;
	for ( const auto &shaderRef : shadersToHash )
	{
		hash ^= hasher( shaderStageToString( shaderRef.stage ) ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
		hash ^= hasher( shaderRef.shaderId ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
	}

	// Combine state block ids (use pass-specific or material states)
	const StateReferences &statesToHash = materialPass ? materialPass->states : material.states;
	hash ^= hasher( statesToHash.rasterizer ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
	hash ^= hasher( statesToHash.depthStencil ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );
	hash ^= hasher( statesToHash.blend ) + 0x9e3779b9 + ( hash << 6 ) + ( hash >> 2 );

	return hash;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineCache::get( PSOHash hash ) const
{
	const auto it = m_cache.find( hash );
	if ( it == m_cache.end() )
		return nullptr;

	return it->second.pso;
}

void PipelineCache::store( PSOHash hash, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso, const std::string &materialId, const std::string &passName )
{
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
	m_cache.clear();
}

} // namespace graphics::material_system

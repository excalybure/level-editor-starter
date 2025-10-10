#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace graphics::material_system
{

struct MaterialDefinition;
struct RenderPassConfig;

// Hash value for PSO cache lookup
using PSOHash = size_t;

// Compute deterministic hash from material + pass configuration
// Used for PSO cache key generation
PSOHash computePSOHash( const MaterialDefinition &material, const RenderPassConfig &passConfig );

// Cache for Pipeline State Objects
// Stores PSOs by hash and detects collisions
class PipelineCache
{
public:
	PipelineCache() = default;

	// Retrieve cached PSO by hash, returns nullptr if not found
	Microsoft::WRL::ComPtr<ID3D12PipelineState> get( PSOHash hash ) const;

	// Store PSO in cache with given hash
	// Fatal error if hash collision detected (same hash, different PSO)
	void store( PSOHash hash, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso, const std::string &materialId, const std::string &passName );

private:
	struct CacheEntry
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		std::string materialId;
		std::string passName;
	};

	std::unordered_map<PSOHash, CacheEntry> m_cache;
};

} // namespace graphics::material_system

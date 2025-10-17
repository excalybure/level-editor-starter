#pragma once

#include <d3d12.h>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include "graphics/gpu/mesh_gpu.h"

namespace graphics
{

// GPU resource manager with caching support
class GPUResourceManager : public gpu::MaterialProvider
{
public:
	// Constructor
	explicit GPUResourceManager( dx12::Device &device );

	// Destructor
	~GPUResourceManager() = default;

	// Disable copy/move for now
	GPUResourceManager( const GPUResourceManager & ) = delete;
	GPUResourceManager &operator=( const GPUResourceManager & ) = delete;
	GPUResourceManager( GPUResourceManager && ) = delete;
	GPUResourceManager &operator=( GPUResourceManager && ) = delete;

	// Mesh resource caching
	std::shared_ptr<graphics::gpu::MeshGPU> getMeshGPU( std::shared_ptr<assets::Mesh> mesh );

	// Material resource caching
	std::shared_ptr<graphics::gpu::MaterialGPU> getMaterialGPU( std::shared_ptr<assets::Material> material ) override;
	std::shared_ptr<graphics::gpu::MaterialGPU> getDefaultMaterialGPU() override;

	// Cache management
	void clearCache();
	void unloadUnusedResources();
	void cleanupExpiredReferences();

	// Frame management for deferred resource cleanup
	void processPendingDeletes();

	// Queue resources for deferred deletion
	void queueForDeletion( std::shared_ptr<graphics::gpu::MeshGPU> meshGPU );
	void queueForDeletion( std::shared_ptr<graphics::gpu::MaterialGPU> materialGPU );

	// Statistics and monitoring
	struct Statistics
	{
		std::size_t meshCacheSize = 0;
		std::size_t materialCacheSize = 0;
		std::uint64_t cacheHits = 0;
		std::uint64_t cacheMisses = 0;
		std::size_t estimatedMemoryUsage = 0; // Bytes
	};

	const Statistics &getStatistics() const { return m_statistics; }
	void resetStatistics();

	// Validation
	bool isValid() const noexcept { return m_device != nullptr; }

private:
	dx12::Device *m_device = nullptr;

	// Default material for primitives without materials
	std::shared_ptr<graphics::gpu::MaterialGPU> m_defaultMaterialGPU;

	// Cache maps using weak_ptr for automatic cleanup
	std::unordered_map<assets::Mesh *, std::weak_ptr<graphics::gpu::MeshGPU>> m_meshCache;
	std::unordered_map<assets::Material *, std::weak_ptr<graphics::gpu::MaterialGPU>> m_materialCache;

	// Statistics tracking
	mutable Statistics m_statistics;

	// Deferred deletion queues
	std::vector<std::shared_ptr<graphics::gpu::MeshGPU>> m_pendingMeshDeletions;
	std::vector<std::shared_ptr<graphics::gpu::MaterialGPU>> m_pendingMaterialDeletions;

	// Helper methods for statistics
	void updateStatistics() const;
};

} // namespace graphics

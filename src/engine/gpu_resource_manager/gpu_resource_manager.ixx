// Global module fragment for headers
module;

#include <d3d12.h>
#include <wrl.h>

export module engine.gpu_resource_manager;

import std;
import platform.dx12;
import engine.assets;
import engine.asset_gpu_buffers;
import engine.material_gpu;
import runtime.console;

export namespace engine
{

// GPU resource manager with caching support
class GPUResourceManager
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
	std::shared_ptr<engine::gpu::MeshGPU> getMeshGPU( std::shared_ptr<assets::Mesh> mesh );

	// Material resource caching
	std::shared_ptr<engine::gpu::MaterialGPU> getMaterialGPU( std::shared_ptr<assets::Material> material );

	// Cache management
	void clearCache();
	void unloadUnusedResources();
	void cleanupExpiredReferences();

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

	// Cache maps using weak_ptr for automatic cleanup
	std::unordered_map<assets::Mesh *, std::weak_ptr<engine::gpu::MeshGPU>> m_meshCache;
	std::unordered_map<assets::Material *, std::weak_ptr<engine::gpu::MaterialGPU>> m_materialCache;

	// Statistics tracking
	mutable Statistics m_statistics;

	// Helper methods for statistics
	void updateStatistics() const;
};

} // namespace engine
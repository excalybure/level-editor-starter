module engine.gpu_resource_manager;

import runtime.console;

namespace engine
{

GPUResourceManager::GPUResourceManager( dx12::Device &device )
	: m_device( &device )
{
	// Basic validation
	if ( !m_device )
	{
		console::error( "GPUResourceManager: null device provided" );
		return;
	}

	console::info( "GPUResourceManager initialized successfully" );
}

std::shared_ptr<engine::gpu::MeshGPU> GPUResourceManager::getMeshGPU( std::shared_ptr<assets::Mesh> mesh )
{
	if ( !mesh )
	{
		console::error( "GPUResourceManager::getMeshGPU: null mesh provided" );
		return nullptr;
	}

	// Check cache first
	const auto it = m_meshCache.find( mesh.get() );
	if ( it != m_meshCache.end() )
	{
		if ( const auto cached = it->second.lock() )
		{
			++m_statistics.cacheHits;
			return cached;
		}
		// Weak pointer expired, remove from cache
		m_meshCache.erase( it );
	}

	// Cache miss - create new GPU buffers
	++m_statistics.cacheMisses;
	const auto gpuBuffers = std::make_shared<engine::gpu::MeshGPU>( *m_device, *mesh );
	if ( !gpuBuffers->isValid() )
	{
		console::error( "GPUResourceManager: failed to create GPU buffers for mesh" );
		return nullptr;
	}

	// Cache the new resource
	m_meshCache[mesh.get()] = gpuBuffers;

	return gpuBuffers;
}


std::shared_ptr<engine::gpu::MaterialGPU> GPUResourceManager::getMaterialGPU( std::shared_ptr<assets::Material> material )
{
	if ( !material )
	{
		console::error( "GPUResourceManager::getMaterialGPU: null material provided" );
		return nullptr;
	}

	// Check cache first
	const auto it = m_materialCache.find( material.get() );
	if ( it != m_materialCache.end() )
	{
		if ( const auto cached = it->second.lock() )
		{
			++m_statistics.cacheHits;
			return cached;
		}
		// Weak pointer expired, remove from cache
		m_materialCache.erase( it );
	}

	// Cache miss - create new MaterialGPU
	++m_statistics.cacheMisses;
	const auto materialGPU = std::make_shared<engine::gpu::MaterialGPU>( material );
	if ( !materialGPU->isValid() )
	{
		console::error( "GPUResourceManager: failed to create MaterialGPU" );
		return nullptr;
	}

	// Cache the new resource
	m_materialCache[material.get()] = materialGPU;

	return materialGPU;
}


void GPUResourceManager::clearCache()
{
	console::info( "GPUResourceManager: Clearing all caches" );
	m_meshCache.clear();
	m_materialCache.clear();

	// Reset cache sizes in statistics but preserve hit/miss counts
	m_statistics.meshCacheSize = 0;
	m_statistics.materialCacheSize = 0;
	m_statistics.estimatedMemoryUsage = 0;
}

void GPUResourceManager::unloadUnusedResources()
{
	cleanupExpiredReferences();
	console::info( "GPUResourceManager: Unused resources cleaned up" );
}

void GPUResourceManager::cleanupExpiredReferences()
{
	// Clean up expired weak_ptr references from mesh cache
	for ( auto it = m_meshCache.begin(); it != m_meshCache.end(); )
	{
		if ( it->second.expired() )
		{
			it = m_meshCache.erase( it );
		}
		else
		{
			++it;
		}
	}

	// Clean up expired weak_ptr references from material cache
	for ( auto it = m_materialCache.begin(); it != m_materialCache.end(); )
	{
		if ( it->second.expired() )
		{
			it = m_materialCache.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void GPUResourceManager::resetStatistics()
{
	m_statistics = Statistics{};
	updateStatistics();
}

void GPUResourceManager::updateStatistics() const
{
	// Count active cache entries (non-expired)
	m_statistics.meshCacheSize = 0;
	for ( const auto &pair : m_meshCache )
	{
		if ( !pair.second.expired() )
		{
			++m_statistics.meshCacheSize;
		}
	}

	m_statistics.materialCacheSize = 0;
	for ( const auto &pair : m_materialCache )
	{
		if ( !pair.second.expired() )
		{
			++m_statistics.materialCacheSize;
		}
	}

	// Rough memory estimation (this is a simplified calculation)
	// In a real implementation, you'd want more accurate memory tracking
	m_statistics.estimatedMemoryUsage =
		( m_statistics.meshCacheSize * 1024 * 1024 ) + // Assume 1MB per mesh on average
		( m_statistics.materialCacheSize * 1024 );	   // Assume 1KB per material on average
}

} // namespace engine
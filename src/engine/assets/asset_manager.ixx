export module engine.asset_manager;

import std;
import engine.assets;

// Forward declaration - avoid circular dependency by only declaring interface
export namespace ecs
{
class Scene;
}
export namespace components
{
struct Transform;
struct Name;
struct MeshRenderer;
} // namespace components

export namespace assets
{

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	// Prevent copying for now (can implement later if needed)
	AssetManager( const AssetManager & ) = delete;
	AssetManager &operator=( const AssetManager & ) = delete;

	// Main API: load and cache assets
	template <typename T>
	std::shared_ptr<T> load( const std::string &path );

	// Get already loaded asset from cache
	template <typename T>
	std::shared_ptr<T> get( const std::string &path );

	// Check if asset is cached
	bool isCached( const std::string &path ) const;

	// Unload asset if it's uniquely referenced
	bool unload( const std::string &path );

	// Clear all cached assets
	void clearCache();

	// Store already loaded assets (useful for integration with external loaders)
	template <typename T>
	void store( const std::string &path, std::shared_ptr<T> asset );

	// ECS import functionality - to be implemented by external integration
	// This method signature allows external code to provide the implementation
	// while keeping the AssetManager independent of ECS
	using ImportSceneCallback = std::function<void( std::shared_ptr<Scene>, ecs::Scene & )>;
	using SceneLoaderCallback = std::function<std::shared_ptr<Scene>( const std::string & )>;

	// Import scene into ECS (requires importSceneCallback to be set)
	bool importScene( const std::string &path, ecs::Scene &ecsScene );

	// Callback management methods
	static void setSceneLoaderCallback( SceneLoaderCallback callback );
	static void clearSceneLoaderCallback();
	static void setImportSceneCallback( ImportSceneCallback callback );
	static void clearImportSceneCallback();

private:
	// Cache storage: path -> shared_ptr<Asset>
	std::unordered_map<std::string, std::shared_ptr<Asset>> m_cache;

	// Static callback storage
	static ImportSceneCallback s_importSceneCallback;
	static SceneLoaderCallback s_sceneLoaderCallback;

	// Internal loading functions for different asset types
	std::shared_ptr<Scene> loadScene( const std::string &path );
	std::shared_ptr<Material> loadMaterial( const std::string &path );
	std::shared_ptr<Mesh> loadMesh( const std::string &path );
};

// Static callback definition (to be set by external integration code)
// Static member initialization
AssetManager::ImportSceneCallback AssetManager::s_importSceneCallback = nullptr;
AssetManager::SceneLoaderCallback AssetManager::s_sceneLoaderCallback = nullptr;

// Template specializations for supported asset types
template <>
inline std::shared_ptr<Scene> AssetManager::load<Scene>( const std::string &path )
{
	// Check cache first
	const auto it = m_cache.find( path );
	if ( it != m_cache.end() )
	{
		return std::static_pointer_cast<Scene>( it->second );
	}

	// Load new asset
	auto scene = loadScene( path );
	if ( scene && scene->isLoaded() )
	{
		// Only cache scenes that were successfully loaded
		m_cache[path] = scene;
	}
	return scene;
}

template <>
inline std::shared_ptr<Material> AssetManager::load<Material>( const std::string &path )
{
	// Check cache first
	const auto it = m_cache.find( path );
	if ( it != m_cache.end() )
	{
		return std::static_pointer_cast<Material>( it->second );
	}

	// Load new asset
	auto material = loadMaterial( path );
	if ( material )
	{
		m_cache[path] = material;
	}
	return material;
}

template <>
inline std::shared_ptr<Mesh> AssetManager::load<Mesh>( const std::string &path )
{
	// Check cache first
	const auto it = m_cache.find( path );
	if ( it != m_cache.end() )
	{
		return std::static_pointer_cast<Mesh>( it->second );
	}

	// Load new asset
	auto mesh = loadMesh( path );
	if ( mesh )
	{
		m_cache[path] = mesh;
	}
	return mesh;
}

// Template method implementations
template <typename T>
std::shared_ptr<T> AssetManager::get( const std::string &path )
{
	const auto it = m_cache.find( path );
	if ( it != m_cache.end() )
	{
		return std::static_pointer_cast<T>( it->second );
	}
	return nullptr;
}

template <typename T>
void AssetManager::store( const std::string &path, std::shared_ptr<T> asset )
{
	if ( asset )
	{
		asset->setPath( path );
		asset->setLoaded( true );
		m_cache[path] = asset;
	}
}

// Implementation of basic methods
inline bool AssetManager::isCached( const std::string &path ) const
{
	return m_cache.find( path ) != m_cache.end();
}

inline bool AssetManager::unload( const std::string &path )
{
	const auto it = m_cache.find( path );
	if ( it != m_cache.end() )
	{
		// Only unload if this is the last reference
		if ( it->second.use_count() == 1 )
		{
			m_cache.erase( it );
			return true;
		}
	}
	return false;
}

inline void AssetManager::clearCache()
{
	m_cache.clear();
}

inline bool AssetManager::importScene( const std::string &path, ecs::Scene &ecsScene )
{
	// Load the scene first (or get from cache)
	auto scene = load<Scene>( path );
	if ( !scene )
	{
		return false;
	}

	// Use the callback if available
	if ( s_importSceneCallback )
	{
		s_importSceneCallback( scene, ecsScene );
		return true;
	}

	// No callback available
	return false;
}

inline void AssetManager::setSceneLoaderCallback( SceneLoaderCallback callback )
{
	s_sceneLoaderCallback = callback;
}

inline void AssetManager::clearSceneLoaderCallback()
{
	s_sceneLoaderCallback = nullptr;
}

inline void AssetManager::setImportSceneCallback( ImportSceneCallback callback )
{
	s_importSceneCallback = callback;
}

inline void AssetManager::clearImportSceneCallback()
{
	s_importSceneCallback = nullptr;
}

// Internal loading implementations
inline std::shared_ptr<Scene> AssetManager::loadScene( const std::string &path )
{
	// Basic validation
	if ( path.empty() )
	{
		return nullptr;
	}

	// Use scene loader callback if available (for real glTF loading)
	if ( s_sceneLoaderCallback )
	{
		auto scene = s_sceneLoaderCallback( path );
		if ( scene )
		{
			scene->setPath( path );
			scene->setLoaded( true );
			return scene;
		}
	}

	return nullptr;
}

inline std::shared_ptr<Material> AssetManager::loadMaterial( const std::string &path )
{
	// TODO: Implement material loading from files
	// For now, create a basic material
	auto material = std::make_shared<Material>();
	material->setPath( path );
	material->setLoaded( true );
	return material;
}

inline std::shared_ptr<Mesh> AssetManager::loadMesh( const std::string &path )
{
	// TODO: Implement standalone mesh loading
	// For now, this would be part of scene loading
	return nullptr;
}

} // namespace assets
#pragma once

#include "engine/gltf_loader/gltf_loader.h"

namespace engine::integration
{

// Initialize GLTFLoader integration with AssetManager
// This function sets up the scene loader callback to enable AssetManager::loadScene
// to delegate to GLTFLoader for loading glTF files.
void initializeAssetGLTFIntegration()
{
	// Create a static GLTFLoader instance for the integration
	// Note: This uses lazy initialization to avoid issues with static initialization order
	static std::shared_ptr<gltf_loader::GLTFLoader> s_gltfLoader;

	// Initialize on first call
	if ( !s_gltfLoader )
	{
		s_gltfLoader = std::make_shared<gltf_loader::GLTFLoader>();
	}

	// Set up the scene loader callback to use the GLTFLoader
	assets::AssetManager::setSceneLoaderCallback(
		[]( const std::string &path ) -> std::shared_ptr<assets::Scene> {
			// Get the static loader instance (will be initialized by now)
			static auto loader = std::make_shared<gltf_loader::GLTFLoader>();

			// Convert unique_ptr from GLTFLoader to shared_ptr for AssetManager
			auto uniqueScene = loader->loadScene( path );
			if ( uniqueScene )
			{
				return std::shared_ptr<assets::Scene>( uniqueScene.release() );
			}
			return nullptr;
		} );
}

} // namespace engine::integration

module engine.asset_manager;

// Static member definitions
assets::AssetManager::ImportSceneCallback assets::AssetManager::s_importSceneCallback = nullptr;
assets::AssetManager::SceneLoaderCallback assets::AssetManager::s_sceneLoaderCallback = nullptr;
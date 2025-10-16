#pragma once

#include <memory>

// Forward declarations
namespace assets
{
class Scene;
}

namespace graphics::texture
{
class TextureManager;

/**
 * @brief Load all textures referenced by materials in a scene
 * 
 * Iterates through all materials in the scene and loads their textures using the TextureManager.
 * Uses the scene's base path to resolve relative texture paths.
 * 
 * @param scene The scene containing materials with texture paths
 * @param textureManager The texture manager to use for loading textures
 * @return Number of textures successfully loaded
 */
int loadSceneTextures( std::shared_ptr<assets::Scene> scene, TextureManager *textureManager );

} // namespace graphics::texture

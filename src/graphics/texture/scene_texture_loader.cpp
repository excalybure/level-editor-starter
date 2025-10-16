#include "scene_texture_loader.h"
#include "texture_manager.h"
#include "engine/assets/assets.h"
#include "core/console.h"

namespace graphics::texture
{

int loadSceneTextures( std::shared_ptr<assets::Scene> scene, TextureManager *textureManager )
{
	if ( !scene || !textureManager )
	{
		console::error( "loadSceneTextures: Invalid scene or texture manager" );
		return 0;
	}

	int texturesLoaded = 0;
	const std::string &basePath = scene->getBasePath();

	// Iterate through all materials in the scene
	for ( auto &material : scene->getMaterials() )
	{
		if ( !material )
			continue;

		auto &pbr = material->getPBRMaterial();

		// Load base color texture
		if ( !pbr.baseColorTexture.empty() )
		{
			const TextureHandle handle = textureManager->loadTexture( pbr.baseColorTexture, basePath );
			if ( handle != kInvalidTextureHandle )
			{
				pbr.baseColorTextureHandle = handle;
				texturesLoaded++;
			}
			else
			{
				console::warning( "Failed to load base color texture: {}", pbr.baseColorTexture );
			}
		}

		// Load metallic-roughness texture
		if ( !pbr.metallicRoughnessTexture.empty() )
		{
			const TextureHandle handle = textureManager->loadTexture( pbr.metallicRoughnessTexture, basePath );
			if ( handle != kInvalidTextureHandle )
			{
				pbr.metallicRoughnessTextureHandle = handle;
				texturesLoaded++;
			}
			else
			{
				console::warning( "Failed to load metallic-roughness texture: {}", pbr.metallicRoughnessTexture );
			}
		}

		// Load normal texture
		if ( !pbr.normalTexture.empty() )
		{
			const TextureHandle handle = textureManager->loadTexture( pbr.normalTexture, basePath );
			if ( handle != kInvalidTextureHandle )
			{
				pbr.normalTextureHandle = handle;
				texturesLoaded++;
			}
			else
			{
				console::warning( "Failed to load normal texture: {}", pbr.normalTexture );
			}
		}

		// Load emissive texture
		if ( !pbr.emissiveTexture.empty() )
		{
			const TextureHandle handle = textureManager->loadTexture( pbr.emissiveTexture, basePath );
			if ( handle != kInvalidTextureHandle )
			{
				pbr.emissiveTextureHandle = handle;
				texturesLoaded++;
			}
			else
			{
				console::warning( "Failed to load emissive texture: {}", pbr.emissiveTexture );
			}
		}
	}

	if ( texturesLoaded > 0 )
	{
		console::info( "Loaded {} textures for scene", texturesLoaded );
	}

	return texturesLoaded;
}

} // namespace graphics::texture

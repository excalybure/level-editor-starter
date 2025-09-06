// Global module fragment for C headers
module;
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

module engine.gltf_loader;

import std;
import engine.assets;

namespace gltf_loader
{

GLTFLoader::GLTFLoader()
{
	// Initialize any needed resources
}

std::unique_ptr<assets::Scene> GLTFLoader::loadScene( const std::string &filePath ) const
{
	// Simplified placeholder implementation - creates an empty scene
	auto scene = std::make_unique<assets::Scene>();

	// TODO: In a real implementation, this would:
	// 1. Parse the glTF file using cgltf library: cgltf::parse_file(filePath)
	// 2. Convert cgltf data structures to our asset system
	// 3. Create proper mesh, material, and scene node hierarchies
	// 4. Handle materials, textures, animations, etc.

	return scene;
}

std::unique_ptr<assets::Scene> GLTFLoader::loadFromString( const std::string &gltfContent ) const
{
	// Parse glTF JSON string
	cgltf_data *data = nullptr;
	cgltf_options options = {};

	cgltf_result result = cgltf_parse( &options, gltfContent.c_str(), gltfContent.size(), &data );

	if ( result != cgltf_result_success || !data )
	{
		if ( data )
		{
			cgltf_free( data );
		}
		throw std::runtime_error( "Failed to parse glTF content" );
	}

	// Create scene
	auto scene = std::make_unique<assets::Scene>();

	// Process default scene or first scene
	cgltf_scene *gltfScene = nullptr;
	if ( data->scene )
	{
		gltfScene = data->scene;
	}
	else if ( data->scenes_count > 0 )
	{
		gltfScene = &data->scenes[0];
	}

	if ( gltfScene )
	{
		// Process root nodes
		for ( cgltf_size i = 0; i < gltfScene->nodes_count; ++i )
		{
			cgltf_node *gltfNode = gltfScene->nodes[i];
			if ( gltfNode )
			{
				auto sceneNode = processNode( gltfNode, data );
				if ( sceneNode )
				{
					scene->addRootNode( std::move( sceneNode ) );
				}
			}
		}
	}

	cgltf_free( data );
	return scene;
}

std::unique_ptr<assets::SceneNode> GLTFLoader::processNode( void *gltfNodePtr, void *dataPtr ) const
{
	cgltf_node *gltfNode = static_cast<cgltf_node *>( gltfNodePtr );
	cgltf_data *data = static_cast<cgltf_data *>( dataPtr );

	if ( !gltfNode )
		return nullptr;

	// Create scene node with name if available
	std::string nodeName = gltfNode->name ? gltfNode->name : "UnnamedNode";
	auto sceneNode = std::make_unique<assets::SceneNode>( nodeName );

	// Process mesh if node has one
	if ( gltfNode->mesh )
	{
		// For now, just indicate that this node has a mesh
		// In a full implementation, we would load the actual mesh
		sceneNode->meshes.push_back( "mesh_placeholder" );
	}

	// Process material if available
	if ( gltfNode->mesh && gltfNode->mesh->primitives_count > 0 )
	{
		for ( cgltf_size i = 0; i < gltfNode->mesh->primitives_count; ++i )
		{
			cgltf_primitive *primitive = &gltfNode->mesh->primitives[i];
			if ( primitive->material )
			{
				sceneNode->materials.push_back( "material_placeholder" );
				break; // Just add one material for now
			}
		}
	}

	// Process child nodes recursively
	for ( cgltf_size i = 0; i < gltfNode->children_count; ++i )
	{
		cgltf_node *childNode = gltfNode->children[i];
		auto childSceneNode = processNode( childNode, data );
		if ( childSceneNode )
		{
			sceneNode->children.push_back( std::move( childSceneNode ) );
		}
	}

	return sceneNode;
}

} // namespace gltf_loader

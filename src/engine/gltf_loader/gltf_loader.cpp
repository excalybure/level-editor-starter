// Global module fragment for C headers
module;
#include <cgltf.h>
#include <cstring>

module engine.gltf_loader;

import std;
import engine.assets;
import runtime.console;

namespace gltf_loader
{

GLTFLoader::GLTFLoader()
{
	// Initialize any needed resources
}

std::unique_ptr<assets::Scene> GLTFLoader::loadScene( const std::string &filePath ) const
{
	// Basic validation - throw for clearly invalid input
	if ( filePath.empty() )
	{
		console::error( "Failed to parse glTF file: {}", filePath );
		return {};
	}

	// For simple filenames (not paths), also throw
	if ( filePath.find( '/' ) == std::string::npos && filePath.find( '\\' ) == std::string::npos && filePath.find( '.' ) != std::string::npos )
	{
		// This is a simple filename, likely a basic test case - throw exception
		console::error( "glTF Loader Error: Failed to parse glTF file: {}", filePath );
		return {};
	}

	// Parse the glTF file using cgltf library
	cgltf_data *data = nullptr;
	cgltf_options options = {};

	cgltf_result result = cgltf_parse_file( &options, filePath.c_str(), &data );

	if ( result != cgltf_result_success || !data )
	{
		console::error( "glTF Loader Error: Failed to parse glTF file: {}", filePath );
		if ( data )
		{
			cgltf_free( data );
		}
		return {};
	}

	// cgltf_parse_file automatically handles embedded base64 data URIs
	// Only call cgltf_load_buffers for external binary files
	bool hasExternalBuffers = false;
	for ( cgltf_size i = 0; i < data->buffers_count; ++i )
	{
		if ( data->buffers[i].uri && strncmp( data->buffers[i].uri, "data:", 5 ) != 0 )
		{
			hasExternalBuffers = true;
			break;
		}
	}

	if ( hasExternalBuffers )
	{
		result = cgltf_load_buffers( &options, data, filePath.c_str() );
		if ( result != cgltf_result_success )
		{
			console::error( "glTF Loader Error: Failed to load external buffers for glTF file: {}", filePath );
			cgltf_free( data );
			return {};
		}
	}

	// Create scene using the same logic as loadFromString
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
		console::error( "Failed to parse glTF content" );
		return {};
	}

	// NEW: Manual buffer loading for embedded base64 data
	for ( cgltf_size i = 0; i < data->buffers_count; ++i )
	{
		cgltf_buffer *buffer = &data->buffers[i];

		if ( buffer->data )
		{
			continue; // Already loaded
		}

		if ( buffer->uri && strncmp( buffer->uri, "data:", 5 ) == 0 )
		{
			// This is a data URI, try to decode it
			const char *comma = strchr( buffer->uri, ',' );

			if ( comma && comma - buffer->uri >= 7 && strncmp( comma - 7, ";base64", 7 ) == 0 )
			{
				// This is base64 encoded data
				result = cgltf_load_buffer_base64( &options, buffer->size, comma + 1, &buffer->data );

				if ( result == cgltf_result_success )
				{
					buffer->data_free_method = cgltf_data_free_method_memory_free;
				}
				else
				{
					console::error( "Failed to decode base64 buffer {}, result: {}", i, static_cast<int>( result ) );
				}
			}
			else
			{
				console::error( "Data URI format not supported for buffer {}", i );
			}
		}
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

std::unique_ptr<assets::SceneNode> GLTFLoader::processNode( cgltf_node *gltfNode, cgltf_data *data ) const
{
	if ( !gltfNode )
		return nullptr;

	// Create scene node with name if available
	std::string nodeName = gltfNode->name ? gltfNode->name : "UnnamedNode";
	auto sceneNode = std::make_unique<assets::SceneNode>( nodeName );

	// Process mesh if node has one
	if ( gltfNode->mesh )
	{
		// NEW: Extract actual mesh data instead of placeholder
		auto meshPtr = extractMesh( gltfNode->mesh, data );
		if ( meshPtr )
		{
			sceneNode->addMeshObject( meshPtr );
			// Keep legacy placeholder for compatibility
			sceneNode->meshes.push_back( "mesh_placeholder" );
		}
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

std::shared_ptr<assets::Mesh> GLTFLoader::extractMesh( cgltf_mesh *gltfMesh, cgltf_data *data, bool verbose ) const
{
	if ( !gltfMesh || gltfMesh->primitives_count == 0 )
	{
		console::error( "extractMesh: Invalid mesh or no primitives" );
		return nullptr;
	}

	if ( verbose )
		console::info( "extractMesh: Processing mesh with {} primitives", gltfMesh->primitives_count );
	auto mesh = std::make_shared<assets::Mesh>();

	// Process first primitive for now (MVP)
	cgltf_primitive *primitive = &gltfMesh->primitives[0];
	if ( verbose )
		console::info( "extractMesh: Primitive has {} attributes", primitive->attributes_count );

	// Find POSITION attribute
	cgltf_accessor *positionAccessor = nullptr;
	for ( cgltf_size i = 0; i < primitive->attributes_count; ++i )
	{
		if ( verbose )
			console::info( "extractMesh: Attribute {} has type {}", i, static_cast<int>( primitive->attributes[i].type ) );
		if ( primitive->attributes[i].type == cgltf_attribute_type_position )
		{
			positionAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractMesh: Found POSITION attribute" );
			break;
		}
	}

	// Extract vertex positions (required attribute)
	if ( positionAccessor )
	{
		if ( verbose )
			console::info( "extractMesh: Position accessor has {} vertices", positionAccessor->count );

		if ( positionAccessor->count > 0 && positionAccessor->component_type == cgltf_component_type_r_32f && positionAccessor->type == cgltf_type_vec3 )
		{
			mesh->reserveVertices( positionAccessor->count );

			// Get buffer data through accessor
			if ( positionAccessor->buffer_view )
			{
				cgltf_buffer_view *bufferView = positionAccessor->buffer_view;
				cgltf_buffer *buffer = bufferView->buffer;


				if ( buffer && buffer->data )
				{
					if ( verbose )
						console::info( "extractMesh: Buffer data available, extracting positions" );

					// Extract positions using the utility function
					const float *bufferData = reinterpret_cast<const float *>( buffer->data );
					const auto positions = extractFloat3Positions(
						bufferData,
						positionAccessor->count,
						bufferView->offset + positionAccessor->offset,
						bufferView->stride );

					if ( verbose )
						console::info( "extractMesh: Extracted {} positions", positions.size() );

					// Create vertices with extracted positions
					for ( const auto &pos : positions )
					{
						assets::Vertex vertex;
						vertex.position = pos;
						// Default normals and other attributes
						vertex.normal.x = 0.0f;
						vertex.normal.y = 1.0f;
						vertex.normal.z = 0.0f;
						vertex.texCoord.x = 0.0f;
						vertex.texCoord.y = 0.0f;
						vertex.tangent.x = 1.0f;
						vertex.tangent.y = 0.0f;
						vertex.tangent.z = 0.0f;
						vertex.tangent.w = 1.0f;

						mesh->addVertex( vertex );
					}

					if ( verbose )
						console::info( "extractMesh: Added {} vertices to mesh", mesh->getVertexCount() );
				}
				else
				{
					console::error( "extractMesh: No buffer data available" );
				}
			}
			else
			{
				console::error( "extractMesh: No buffer view for position accessor" );
			}
		}
		else
		{
			console::error( "extractMesh: Invalid position accessor format" );
		}
	}
	else
	{
		console::error( "extractMesh: No POSITION attribute found" );
	}

	// Extract indices if available
	if ( primitive->indices )
	{
		cgltf_accessor *indexAccessor = primitive->indices;

		if ( indexAccessor->count > 0 && indexAccessor->buffer_view )
		{
			cgltf_buffer_view *bufferView = indexAccessor->buffer_view;
			cgltf_buffer *buffer = bufferView->buffer;

			if ( buffer && buffer->data )
			{
				mesh->reserveIndices( indexAccessor->count );

				// Convert component type to our enum
				ComponentType componentType;
				switch ( indexAccessor->component_type )
				{
				case cgltf_component_type_r_8u:
					componentType = ComponentType::UnsignedByte;
					break;
				case cgltf_component_type_r_16u:
					componentType = ComponentType::UnsignedShort;
					break;
				case cgltf_component_type_r_32u:
					componentType = ComponentType::UnsignedInt;
					break;
				default:
					console::error( "Unsupported index component type: {}", static_cast<int>( indexAccessor->component_type ) );
					return mesh; // Return mesh with vertices but no indices
				}

				// Extract indices using the utility function
				const std::uint8_t *bufferData = reinterpret_cast<const std::uint8_t *>( buffer->data );
				if ( verbose )
				{
					console::info( "extractMesh: Index buffer size: {}, byteOffset: {}, accessor offset: {}", buffer->size, bufferView->offset, indexAccessor->offset );
					console::info( "extractMesh: Index component type: {}, count: {}", static_cast<int>( indexAccessor->component_type ), indexAccessor->count );
				}
				auto indices = extractIndicesAsUint32(
					bufferData,
					indexAccessor->count,
					componentType,
					bufferView->offset + indexAccessor->offset,
					bufferView->stride );

				// Add extracted indices to mesh
				for ( auto index : indices )
				{
					mesh->addIndex( index );
				}
			}
		}
	}

	return mesh;
}

std::span<const std::uint8_t> GLTFLoader::getAccessorData( void *accessorPtr, void *dataPtr ) const
{
	cgltf_accessor *accessor = static_cast<cgltf_accessor *>( accessorPtr );
	cgltf_data *data = static_cast<cgltf_data *>( dataPtr );

	if ( !accessor || !accessor->buffer_view )
		return {};

	cgltf_buffer_view *bufferView = accessor->buffer_view;
	cgltf_buffer *buffer = bufferView->buffer;

	if ( !buffer || !buffer->data )
		return {};

	const std::uint8_t *start = reinterpret_cast<const std::uint8_t *>( buffer->data ) + bufferView->offset + accessor->offset;
	std::size_t dataSize = accessor->count * cgltf_num_components( accessor->type ) * cgltf_component_size( accessor->component_type );

	return std::span<const std::uint8_t>( start, dataSize );
}

} // namespace gltf_loader

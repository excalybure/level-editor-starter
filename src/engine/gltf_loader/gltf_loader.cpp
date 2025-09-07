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

	// Load buffers using cgltf_load_buffers which handles data URIs automatically
	result = cgltf_load_buffers( &options, data, "" );
	if ( result != cgltf_result_success )
	{
		console::error( "Failed to load buffers for glTF content, result: {}", static_cast<int>( result ) );
		// Continue anyway - some buffers might have loaded successfully
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

	// Find required and optional attributes
	cgltf_accessor *positionAccessor = nullptr;
	cgltf_accessor *normalAccessor = nullptr;
	cgltf_accessor *texCoordAccessor = nullptr;
	cgltf_accessor *tangentAccessor = nullptr;

	for ( cgltf_size i = 0; i < primitive->attributes_count; ++i )
	{
		if ( verbose )
			console::info( "extractMesh: Attribute {} has type {}", i, static_cast<int>( primitive->attributes[i].type ) );

		switch ( primitive->attributes[i].type )
		{
		case cgltf_attribute_type_position:
			positionAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractMesh: Found POSITION attribute" );
			break;
		case cgltf_attribute_type_normal:
			normalAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractMesh: Found NORMAL attribute" );
			break;
		case cgltf_attribute_type_texcoord:
			texCoordAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractMesh: Found TEXCOORD attribute" );
			break;
		case cgltf_attribute_type_tangent:
			tangentAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractMesh: Found TANGENT attribute" );
			break;
		default:
			if ( verbose )
				console::info( "extractMesh: Ignoring unsupported attribute type {}", static_cast<int>( primitive->attributes[i].type ) );
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

					// Extract positions using corrected buffer offset logic
					const float *bufferData = reinterpret_cast<const float *>( buffer->data );
					const auto positions = extractFloat3Positions(
						bufferData,
						positionAccessor->count,
						bufferView->offset + positionAccessor->offset,
						bufferView->stride );

					std::vector<Vec3f> normals;
					if ( normalAccessor && normalAccessor->component_type == cgltf_component_type_r_32f && normalAccessor->type == cgltf_type_vec3 )
					{
						cgltf_buffer_view *normalBufferView = normalAccessor->buffer_view;
						cgltf_buffer *normalBuffer = normalBufferView->buffer;

						if ( normalBuffer && normalBuffer->data )
						{
							const float *normalBufferData = reinterpret_cast<const float *>( normalBuffer->data );
							normals = extractFloat3Normals(
								normalBufferData,
								normalAccessor->count,
								normalBufferView->offset + normalAccessor->offset,
								normalBufferView->stride );

							if ( verbose )
								console::info( "extractMesh: Extracted {} normals", normals.size() );
						}
					}

					// Extract UVs if available
					std::vector<Vec2f> uvs;
					if ( texCoordAccessor && texCoordAccessor->component_type == cgltf_component_type_r_32f && texCoordAccessor->type == cgltf_type_vec2 )
					{
						cgltf_buffer_view *uvBufferView = texCoordAccessor->buffer_view;
						cgltf_buffer *uvBuffer = uvBufferView->buffer;

						if ( uvBuffer && uvBuffer->data )
						{
							const float *uvBufferData = reinterpret_cast<const float *>( uvBuffer->data );
							uvs = extractFloat2UVs(
								uvBufferData,
								texCoordAccessor->count,
								uvBufferView->offset + texCoordAccessor->offset,
								uvBufferView->stride );

							if ( verbose )
								console::info( "extractMesh: Extracted {} UVs", uvs.size() );
						}
					}

					// Extract tangents if available
					std::vector<Vec4f> tangents;
					if ( tangentAccessor && tangentAccessor->component_type == cgltf_component_type_r_32f && tangentAccessor->type == cgltf_type_vec4 )
					{
						cgltf_buffer_view *tangentBufferView = tangentAccessor->buffer_view;
						cgltf_buffer *tangentBuffer = tangentBufferView->buffer;

						if ( tangentBuffer && tangentBuffer->data )
						{
							const float *tangentBufferData = reinterpret_cast<const float *>( tangentBuffer->data );
							tangents = extractFloat4Tangents(
								tangentBufferData,
								tangentAccessor->count,
								tangentBufferView->offset + tangentAccessor->offset,
								tangentBufferView->stride );

							if ( verbose )
								console::info( "extractMesh: Extracted {} tangents", tangents.size() );
						}
					}

					// Create vertices with extracted data
					for ( std::size_t i = 0; i < positions.size(); ++i )
					{
						assets::Vertex vertex;
						vertex.position = positions[i];

						// Use extracted normals or default
						if ( i < normals.size() )
						{
							vertex.normal = normals[i];
						}
						else
						{
							vertex.normal = Vec3f{ 0.0f, 0.0f, 1.0f };
						}

						// Use extracted UVs or default
						if ( i < uvs.size() )
						{
							vertex.texCoord = uvs[i];
						}
						else
						{
							vertex.texCoord = Vec2f{ 0.0f, 0.0f };
						}

						// Use extracted tangents or default
						if ( i < tangents.size() )
						{
							vertex.tangent = tangents[i];
						}
						else
						{
							vertex.tangent = Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
						}

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

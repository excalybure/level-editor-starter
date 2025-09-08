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

	// Extract transform data from glTF node
	const auto transform = extractTransformFromNode( gltfNode );
	sceneNode->setTransform( transform );

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

	// Process each primitive and create Primitive objects
	for ( cgltf_size primitiveIndex = 0; primitiveIndex < gltfMesh->primitives_count; ++primitiveIndex )
	{
		cgltf_primitive *gltfPrimitive = &gltfMesh->primitives[primitiveIndex];
		if ( verbose )
			console::info( "extractMesh: Processing primitive {} with {} attributes", primitiveIndex, gltfPrimitive->attributes_count );

		// Extract this primitive's data into a Primitive object
		const auto primitive = extractPrimitive( gltfPrimitive, data, verbose );
		if ( primitive )
		{
			mesh->addPrimitive( *primitive );
			if ( verbose )
				console::info( "extractMesh: Added primitive {} with {} vertices", primitiveIndex, primitive->getVertexCount() );
		}
		else
		{
			console::error( "extractMesh: Failed to extract primitive {}", primitiveIndex );
		}
	}

	if ( verbose )
	{
		std::uint32_t totalVertices = 0;
		for ( const auto &primitive : mesh->getPrimitives() )
		{
			totalVertices += primitive.getVertexCount();
		}
		console::info( "extractMesh: Extracted mesh with {} primitives, total {} vertices",
			mesh->getPrimitiveCount(),
			totalVertices );
	}

	return mesh;
}

// NEW: Helper function to extract a single primitive
std::unique_ptr<assets::Primitive> GLTFLoader::extractPrimitive( cgltf_primitive *primitive, cgltf_data *data, bool verbose ) const
{
	if ( !primitive )
	{
		console::error( "extractPrimitive: Invalid primitive" );
		return nullptr;
	}

	auto primitiveObj = std::make_unique<assets::Primitive>();

	// Find required and optional attributes
	cgltf_accessor *positionAccessor = nullptr;
	cgltf_accessor *normalAccessor = nullptr;
	cgltf_accessor *texCoordAccessor = nullptr;
	cgltf_accessor *tangentAccessor = nullptr;

	for ( cgltf_size i = 0; i < primitive->attributes_count; ++i )
	{
		if ( verbose )
			console::info( "extractPrimitive: Attribute {} has type {}", i, static_cast<int>( primitive->attributes[i].type ) );

		switch ( primitive->attributes[i].type )
		{
		case cgltf_attribute_type_position:
			positionAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractPrimitive: Found POSITION attribute" );
			break;
		case cgltf_attribute_type_normal:
			normalAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractPrimitive: Found NORMAL attribute" );
			break;
		case cgltf_attribute_type_texcoord:
			texCoordAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractPrimitive: Found TEXCOORD attribute" );
			break;
		case cgltf_attribute_type_tangent:
			tangentAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractPrimitive: Found TANGENT attribute" );
			break;
		default:
			if ( verbose )
				console::info( "extractPrimitive: Ignoring unsupported attribute type {}", static_cast<int>( primitive->attributes[i].type ) );
			break;
		}
	}

	// Extract vertex positions (required attribute)
	if ( positionAccessor )
	{
		if ( verbose )
			console::info( "extractPrimitive: Position accessor has {} vertices", positionAccessor->count );

		if ( positionAccessor->count > 0 && positionAccessor->component_type == cgltf_component_type_r_32f && positionAccessor->type == cgltf_type_vec3 )
		{
			// Get buffer data through accessor
			if ( positionAccessor->buffer_view )
			{
				cgltf_buffer_view *bufferView = positionAccessor->buffer_view;
				cgltf_buffer *buffer = bufferView->buffer;

				if ( buffer && buffer->data )
				{
					if ( verbose )
						console::info( "extractPrimitive: Buffer data available, extracting positions" );

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
								console::info( "extractPrimitive: Extracted {} normals", normals.size() );
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
								console::info( "extractPrimitive: Extracted {} UVs", uvs.size() );
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
								console::info( "extractPrimitive: Extracted {} tangents", tangents.size() );
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

						primitiveObj->addVertex( vertex );
					}

					if ( verbose )
						console::info( "extractPrimitive: Added {} vertices to primitive", primitiveObj->getVertexCount() );
				}
				else
				{
					console::error( "extractPrimitive: No buffer data available" );
					return nullptr;
				}
			}
			else
			{
				console::error( "extractPrimitive: No buffer view for position accessor" );
				return nullptr;
			}
		}
		else
		{
			console::error( "extractPrimitive: Invalid position accessor format" );
			return nullptr;
		}
	}
	else
	{
		console::error( "extractPrimitive: No POSITION attribute found" );
		return nullptr;
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
					console::error( "extractPrimitive: Unsupported index component type: {}", static_cast<int>( indexAccessor->component_type ) );
					return primitiveObj; // Return primitive with vertices but no indices
				}

				// Extract indices using the utility function
				const std::uint8_t *bufferData = reinterpret_cast<const std::uint8_t *>( buffer->data );
				if ( verbose )
				{
					console::info( "extractPrimitive: Index buffer size: {}, byteOffset: {}, accessor offset: {}", buffer->size, bufferView->offset, indexAccessor->offset );
					console::info( "extractPrimitive: Index component type: {}, count: {}", static_cast<int>( indexAccessor->component_type ), indexAccessor->count );
				}
				const auto indices = extractIndicesAsUint32(
					bufferData,
					indexAccessor->count,
					componentType,
					bufferView->offset + indexAccessor->offset,
					bufferView->stride );

				// Add extracted indices to primitive
				for ( const auto index : indices )
				{
					primitiveObj->addIndex( index );
				}

				if ( verbose )
					console::info( "extractPrimitive: Added {} indices to primitive", primitiveObj->getIndexCount() );
			}
		}
	}

	// Handle material assignment
	if ( primitive->material )
	{
		// For now, create a simple material identifier based on the material index in the glTF data
		// TODO: This should be replaced with actual material asset path/ID when AssetManager is implemented
		const std::string materialPath = std::string( "material_" ) + std::to_string( reinterpret_cast<std::uintptr_t>( primitive->material ) );
		primitiveObj->setMaterialPath( materialPath );

		if ( verbose )
			console::info( "extractPrimitive: Assigned material path: {}", materialPath );
	}

	return primitiveObj;
}

std::shared_ptr<assets::Material> GLTFLoader::extractMaterial( void *gltfMaterialPtr, void *dataPtr, bool verbose ) const
{
	cgltf_material *gltfMaterial = static_cast<cgltf_material *>( gltfMaterialPtr );
	cgltf_data *data = static_cast<cgltf_data *>( dataPtr );

	if ( !gltfMaterial )
	{
		console::error( "extractMaterial: Invalid material pointer" );
		return nullptr;
	}

	if ( verbose )
		console::info( "extractMaterial: Processing material '{}'", gltfMaterial->name ? gltfMaterial->name : "Unnamed" );

	auto material = std::make_shared<assets::Material>();
	auto &pbrMaterial = material->getPBRMaterial();

	// Extract PBR Metallic Roughness properties
	if ( gltfMaterial->has_pbr_metallic_roughness )
	{
		const auto &pbr = gltfMaterial->pbr_metallic_roughness;

		// Extract base color factor (default: [1.0, 1.0, 1.0, 1.0])
		pbrMaterial.baseColorFactor[0] = pbr.base_color_factor[0];
		pbrMaterial.baseColorFactor[1] = pbr.base_color_factor[1];
		pbrMaterial.baseColorFactor[2] = pbr.base_color_factor[2];
		pbrMaterial.baseColorFactor[3] = pbr.base_color_factor[3];

		// Extract metallic factor (default: 1.0)
		pbrMaterial.metallicFactor = pbr.metallic_factor;

		// Extract roughness factor (default: 1.0)
		pbrMaterial.roughnessFactor = pbr.roughness_factor;

		if ( verbose )
		{
			console::info( "extractMaterial: Base color factor: [{}, {}, {}, {}]",
				pbrMaterial.baseColorFactor[0],
				pbrMaterial.baseColorFactor[1],
				pbrMaterial.baseColorFactor[2],
				pbrMaterial.baseColorFactor[3] );
			console::info( "extractMaterial: Metallic factor: {}", pbrMaterial.metallicFactor );
			console::info( "extractMaterial: Roughness factor: {}", pbrMaterial.roughnessFactor );
		}

		// Extract base color texture
		if ( pbr.base_color_texture.texture )
		{
			pbrMaterial.baseColorTexture = extractTextureURI( const_cast<cgltf_texture_view *>( &pbr.base_color_texture ), data );
			if ( verbose && !pbrMaterial.baseColorTexture.empty() )
				console::info( "extractMaterial: Base color texture: {}", pbrMaterial.baseColorTexture );
		}

		// Extract metallic roughness texture
		if ( pbr.metallic_roughness_texture.texture )
		{
			pbrMaterial.metallicRoughnessTexture = extractTextureURI( const_cast<cgltf_texture_view *>( &pbr.metallic_roughness_texture ), data );
			if ( verbose && !pbrMaterial.metallicRoughnessTexture.empty() )
				console::info( "extractMaterial: Metallic roughness texture: {}", pbrMaterial.metallicRoughnessTexture );
		}
	}

	// Extract emissive factor (default: [0.0, 0.0, 0.0])
	pbrMaterial.emissiveFactor[0] = gltfMaterial->emissive_factor[0];
	pbrMaterial.emissiveFactor[1] = gltfMaterial->emissive_factor[1];
	pbrMaterial.emissiveFactor[2] = gltfMaterial->emissive_factor[2];

	if ( verbose )
	{
		console::info( "extractMaterial: Emissive factor: [{}, {}, {}]",
			pbrMaterial.emissiveFactor[0],
			pbrMaterial.emissiveFactor[1],
			pbrMaterial.emissiveFactor[2] );
	}

	// Extract normal texture
	if ( gltfMaterial->normal_texture.texture )
	{
		pbrMaterial.normalTexture = extractTextureURI( const_cast<cgltf_texture_view *>( &gltfMaterial->normal_texture ), data );
		if ( verbose && !pbrMaterial.normalTexture.empty() )
			console::info( "extractMaterial: Normal texture: {}", pbrMaterial.normalTexture );
	}

	// Extract emissive texture
	if ( gltfMaterial->emissive_texture.texture )
	{
		pbrMaterial.emissiveTexture = extractTextureURI( const_cast<cgltf_texture_view *>( &gltfMaterial->emissive_texture ), data );
		if ( verbose && !pbrMaterial.emissiveTexture.empty() )
			console::info( "extractMaterial: Emissive texture: {}", pbrMaterial.emissiveTexture );
	}

	if ( verbose )
		console::info( "extractMaterial: Material extraction completed" );

	return material;
}

std::string GLTFLoader::extractTextureURI( void *textureInfoPtr, void *dataPtr ) const
{
	cgltf_texture_view *textureInfo = static_cast<cgltf_texture_view *>( textureInfoPtr );
	cgltf_data *data = static_cast<cgltf_data *>( dataPtr );

	if ( !textureInfo || !textureInfo->texture || !data )
		return {};

	cgltf_texture *texture = textureInfo->texture;
	if ( !texture->image )
		return {};

	cgltf_image *image = texture->image;
	if ( !image->uri )
		return {};

	// Return the URI string directly
	return std::string( image->uri );
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

assets::Transform GLTFLoader::extractTransformFromNode( cgltf_node *gltfNode ) const
{
	if ( !gltfNode )
	{
		// Return identity transform
		return assets::Transform{};
	}

	// Check if node has matrix transformation
	if ( gltfNode->has_matrix )
	{
		return extractTransformFromMatrix( gltfNode->matrix );
	}

	// Extract from TRS components
	const float *translation = gltfNode->has_translation ? gltfNode->translation : nullptr;
	const float *rotation = gltfNode->has_rotation ? gltfNode->rotation : nullptr;
	const float *scale = gltfNode->has_scale ? gltfNode->scale : nullptr;

	return extractTransformFromTRS( translation, rotation, scale );
}

assets::Transform GLTFLoader::extractTransformFromTRS( const float *translation, const float *rotation, const float *scale ) const
{
	assets::Transform transform;

	// Extract translation (default: 0, 0, 0)
	if ( translation )
	{
		transform.position.x = translation[0];
		transform.position.y = translation[1];
		transform.position.z = translation[2];
	}
	else
	{
		transform.position = math::Vec3f{ 0.0f, 0.0f, 0.0f };
	}

	// Extract rotation from quaternion (default: 0, 0, 0, 1 - identity)
	if ( rotation )
	{
		// glTF quaternion format: [x, y, z, w]
		const auto eulerAngles = quaternionToEulerAngles( rotation[0], rotation[1], rotation[2], rotation[3] );
		transform.rotation = eulerAngles;
	}
	else
	{
		transform.rotation = math::Vec3f{ 0.0f, 0.0f, 0.0f };
	}

	// Extract scale (default: 1, 1, 1)
	if ( scale )
	{
		transform.scale.x = scale[0];
		transform.scale.y = scale[1];
		transform.scale.z = scale[2];
	}
	else
	{
		transform.scale = math::Vec3f{ 1.0f, 1.0f, 1.0f };
	}

	return transform;
}

assets::Transform GLTFLoader::extractTransformFromMatrix( const float *matrix ) const
{
	assets::Transform transform;

	if ( !matrix )
	{
		// Return identity transform
		return transform;
	}

	// glTF matrices are column-major, 4x4
	// Extract translation from the last column (indices 12, 13, 14)
	transform.position.x = matrix[12];
	transform.position.y = matrix[13];
	transform.position.z = matrix[14];

	// Extract scale from the lengths of the first three columns
	const float scaleX = std::sqrt( matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2] );
	const float scaleY = std::sqrt( matrix[4] * matrix[4] + matrix[5] * matrix[5] + matrix[6] * matrix[6] );
	const float scaleZ = std::sqrt( matrix[8] * matrix[8] + matrix[9] * matrix[9] + matrix[10] * matrix[10] );

	transform.scale.x = scaleX;
	transform.scale.y = scaleY;
	transform.scale.z = scaleZ;

	// For rotation extraction from matrix, we'd need to normalize the rotation part
	// and convert to Euler angles. For simplicity, we'll assume no rotation from matrix for now
	// TODO: Implement proper rotation extraction from matrix if needed
	transform.rotation = math::Vec3f{ 0.0f, 0.0f, 0.0f };

	return transform;
}

math::Vec3f GLTFLoader::quaternionToEulerAngles( float x, float y, float z, float w ) const
{
	// Create a quaternion and use the built-in conversion
	const math::Quatf quat{ w, x, y, z };
	return quat.toEulerAngles();
}

} // namespace gltf_loader

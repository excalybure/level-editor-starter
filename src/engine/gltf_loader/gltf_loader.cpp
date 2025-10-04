// GLTF Loader implementation with stub functionality
#define CGLTF_IMPLEMENTATION
#include "gltf_loader.h"

#include <memory>
#include <vector>
#include <string>
#include <span>
#include <cstring>

#include "engine/assets/assets.h"
#include "engine/math/math.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"
#include "engine/math/quat.h"
#include "runtime/console.h"

namespace gltf_loader
{

// Utility function implementations for accessor data extraction

std::vector<math::Vec3f> extractFloat3Positions( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	// Calculate effective stride (minimum of 12 bytes for float3 or specified stride)
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 3 * sizeof( float ) );

	std::vector<math::Vec3f> positions;
	positions.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *vertexBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *vertex = reinterpret_cast<const float *>( vertexBytes );
		positions.push_back( { vertex[0], vertex[1], vertex[2] } );
	}

	return positions;
}

std::vector<math::Vec3f> extractFloat3Normals( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	// Use the same logic as positions since they're both float3, but implemented directly for clarity
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 3 * sizeof( float ) );

	std::vector<math::Vec3f> normals;
	normals.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *normalBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *normal = reinterpret_cast<const float *>( normalBytes );
		normals.push_back( { normal[0], normal[1], normal[2] } );
	}

	return normals;
}

std::vector<math::Vec2f> extractFloat2UVs( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 2 * sizeof( float ) );

	std::vector<math::Vec2f> uvs;
	uvs.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *uvBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *uv = reinterpret_cast<const float *>( uvBytes );
		uvs.push_back( { uv[0], uv[1] } );
	}

	return uvs;
}

std::vector<math::Vec4f> extractFloat4Tangents( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 4 * sizeof( float ) );

	std::vector<math::Vec4f> tangents;
	tangents.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *tangentBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *tangent = reinterpret_cast<const float *>( tangentBytes );
		tangents.push_back( { tangent[0], tangent[1], tangent[2], tangent[3] } );
	}

	return tangents;
}

std::vector<math::Vec4f> extractFloat4Colors( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 4 * sizeof( float ) );

	std::vector<math::Vec4f> colors;
	colors.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *colorBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *color = reinterpret_cast<const float *>( colorBytes );
		colors.push_back( { color[0], color[1], color[2], color[3] } );
	}

	return colors;
}

std::vector<math::Vec4f> extractFloat3ColorsAsVec4( const float *buffer, size_t count, size_t byteOffset, size_t byteStride )
{
	const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 3 * sizeof( float ) );

	std::vector<math::Vec4f> colors;
	colors.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *colorBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *color = reinterpret_cast<const float *>( colorBytes );
		colors.push_back( { color[0], color[1], color[2], 1.0f } ); // Default alpha = 1.0
	}

	return colors;
}

std::vector<uint32_t> extractIndicesAsUint32( const std::uint8_t *buffer, size_t count, ComponentType componentType, size_t byteOffset, size_t byteStride )
{
	std::vector<uint32_t> indices;
	indices.reserve( count );

	const std::uint8_t *data = buffer + byteOffset;

	switch ( componentType )
	{
	case ComponentType::UnsignedByte: {
		const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint8_t );
		for ( size_t i = 0; i < count; ++i )
		{
			const std::uint8_t *indexPtr = data + ( i * effectiveStride );
			std::uint8_t indexValue = *indexPtr;
			indices.push_back( static_cast<std::uint32_t>( indexValue ) );
		}
		break;
	}
	case ComponentType::UnsignedShort: {
		const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint16_t );
		for ( size_t i = 0; i < count; ++i )
		{
			const std::uint8_t *indexPtr = data + ( i * effectiveStride );
			const std::uint16_t indexValue = *reinterpret_cast<const std::uint16_t *>( indexPtr );
			indices.push_back( static_cast<std::uint32_t>( indexValue ) );
		}
		break;
	}
	case ComponentType::UnsignedInt: {
		const size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint32_t );
		for ( size_t i = 0; i < count; ++i )
		{
			const std::uint8_t *indexPtr = data + ( i * effectiveStride );
			const std::uint32_t indexValue = *reinterpret_cast<const std::uint32_t *>( indexPtr );
			indices.push_back( indexValue );
		}
		break;
	}
	default:
		throw std::invalid_argument( "Unsupported component type for indices" );
	}

	return indices;
}

void validateComponentType( ComponentType componentType, AttributeType attributeType )
{
	// Define valid component type combinations based on glTF 2.0 specification
	switch ( attributeType )
	{
	case AttributeType::Position:
	case AttributeType::Normal:
		if ( componentType != ComponentType::Float )
		{
			throw std::invalid_argument( "Position and Normal attributes must use FLOAT component type" );
		}
		break;

	case AttributeType::TexCoord:
		if ( componentType != ComponentType::Float && componentType != ComponentType::UnsignedByte && componentType != ComponentType::UnsignedShort )
		{
			throw std::invalid_argument( "TexCoord attributes must use FLOAT, UNSIGNED_BYTE, or UNSIGNED_SHORT component type" );
		}
		break;

	case AttributeType::Tangent:
		if ( componentType != ComponentType::Float )
		{
			throw std::invalid_argument( "Tangent attributes must use FLOAT component type" );
		}
		break;

	case AttributeType::Indices:
		if ( componentType != ComponentType::UnsignedByte && componentType != ComponentType::UnsignedShort && componentType != ComponentType::UnsignedInt )
		{
			throw std::invalid_argument( "Indices must use UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT component type" );
		}
		break;

	default:
		throw std::invalid_argument( "Unknown attribute type" );
	}
}

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

	// Load buffers using cgltf_load_buffers which handles both data URIs and external files
	result = cgltf_load_buffers( &options, data, filePath.c_str() );
	if ( result != cgltf_result_success )
	{
		console::error( "glTF Loader Error: Failed to load buffers for glTF file: {}, result: {}", filePath, static_cast<int>( result ) );
		// Continue anyway - some buffers might have loaded successfully
	}

	// Process the parsed glTF data into a scene
	auto scene = processSceneData( data );

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

	// Process the parsed glTF data into a scene
	auto scene = processSceneData( data );

	cgltf_free( data );
	return scene;
}

std::unique_ptr<assets::Scene> GLTFLoader::processSceneData( cgltf_data *data ) const
{
	if ( !data )
	{
		console::error( "processSceneData: Invalid glTF data" );
		return nullptr;
	}

	// Create scene
	auto scene = std::make_unique<assets::Scene>();

	// 1. Extract ALL materials from root level first and add to scene
	std::vector<assets::MaterialHandle> materialHandles;
	materialHandles.reserve( data->materials_count );
	for ( cgltf_size i = 0; i < data->materials_count; ++i )
	{
		auto material = extractMaterial( &data->materials[i], data );
		if ( material )
		{
			assets::MaterialHandle handle = scene->addMaterial( material );
			materialHandles.push_back( handle );
		}
		else
		{
			// Add invalid handle placeholder to maintain indexing
			materialHandles.push_back( assets::INVALID_MATERIAL_HANDLE );
		}
	}

	// 2. Extract ALL meshes from root level and add to scene
	std::vector<assets::MeshHandle> meshHandles;
	meshHandles.reserve( data->meshes_count );
	for ( cgltf_size i = 0; i < data->meshes_count; ++i )
	{
		auto mesh = extractMesh( &data->meshes[i], data, materialHandles );
		if ( mesh )
		{
			assets::MeshHandle handle = scene->addMesh( mesh );
			meshHandles.push_back( handle );
		}
		else
		{
			// Add invalid handle placeholder to maintain indexing
			meshHandles.push_back( assets::INVALID_MESH_HANDLE );
		}
	}

	// 3. Process default scene or first scene
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
		// Process root nodes with extracted resources
		for ( cgltf_size i = 0; i < gltfScene->nodes_count; ++i )
		{
			cgltf_node *gltfNode = gltfScene->nodes[i];
			if ( gltfNode )
			{
				auto sceneNode = processNode( gltfNode, data, meshHandles, materialHandles );
				if ( sceneNode )
				{
					scene->addRootNode( std::move( sceneNode ) );
				}
			}
		}
	}

	return scene;
}

std::unique_ptr<assets::SceneNode> GLTFLoader::processNode(
	cgltf_node *gltfNode,
	cgltf_data *data,
	const std::vector<assets::MeshHandle> &meshHandles,
	const std::vector<assets::MaterialHandle> &materialHandles ) const
{
	if ( !gltfNode )
		return nullptr;

	// Create scene node with name if available
	// Priority: 1) node name, 2) mesh name (if node has mesh), 3) "UnnamedNode"
	std::string nodeName;
	if ( gltfNode->name )
	{
		nodeName = gltfNode->name;
	}
	else if ( gltfNode->mesh && gltfNode->mesh->name )
	{
		nodeName = gltfNode->mesh->name;
	}
	else
	{
		nodeName = "UnnamedNode";
	}
	auto sceneNode = std::make_unique<assets::SceneNode>( nodeName );

	// Process mesh if node has one (using mesh index)
	if ( gltfNode->mesh )
	{
		// Calculate mesh index from pointer offset
		const cgltf_size meshIndex = static_cast<cgltf_size>( gltfNode->mesh - data->meshes );
		if ( meshIndex < meshHandles.size() && meshHandles[meshIndex] != assets::INVALID_MESH_HANDLE )
		{
			// Add mesh handle to scene node
			sceneNode->addMeshHandle( meshHandles[meshIndex] );
		}
	}

	// Extract transform data from glTF node
	const auto transform = extractTransformFromNode( gltfNode );
	sceneNode->setTransform( transform );

	// Process child nodes recursively
	for ( cgltf_size i = 0; i < gltfNode->children_count; ++i )
	{
		cgltf_node *childNode = gltfNode->children[i];
		auto childSceneNode = processNode( childNode, data, meshHandles, materialHandles );
		if ( childSceneNode )
		{
			sceneNode->addChild( std::move( childSceneNode ) );
		}
	}

	return sceneNode;
}

std::shared_ptr<assets::Mesh> GLTFLoader::extractMesh( cgltf_mesh *gltfMesh, cgltf_data *data, const std::vector<assets::MaterialHandle> &materialHandles, bool verbose ) const
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
		const auto primitive = extractPrimitive( gltfPrimitive, data, materialHandles, verbose );
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
std::unique_ptr<assets::Primitive> GLTFLoader::extractPrimitive( cgltf_primitive *primitive, cgltf_data *data, const std::vector<assets::MaterialHandle> &materialHandles, bool verbose ) const
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
	cgltf_accessor *colorAccessor = nullptr;

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
		case cgltf_attribute_type_color:
			colorAccessor = primitive->attributes[i].data;
			if ( verbose )
				console::info( "extractPrimitive: Found COLOR attribute" );
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

					std::vector<math::Vec3f> normals;
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
					std::vector<math::Vec2f> uvs;
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
					std::vector<math::Vec4f> tangents;
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

					// Extract colors if available
					std::vector<math::Vec4f> colors;
					if ( colorAccessor && colorAccessor->component_type == cgltf_component_type_r_32f )
					{
						cgltf_buffer_view *colorBufferView = colorAccessor->buffer_view;
						cgltf_buffer *colorBuffer = colorBufferView->buffer;

						if ( colorBuffer && colorBuffer->data )
						{
							const float *colorBufferData = reinterpret_cast<const float *>( colorBuffer->data );

							if ( colorAccessor->type == cgltf_type_vec3 )
							{
								colors = extractFloat3ColorsAsVec4(
									colorBufferData,
									colorAccessor->count,
									colorBufferView->offset + colorAccessor->offset,
									colorBufferView->stride );
							}
							else if ( colorAccessor->type == cgltf_type_vec4 )
							{
								colors = extractFloat4Colors(
									colorBufferData,
									colorAccessor->count,
									colorBufferView->offset + colorAccessor->offset,
									colorBufferView->stride );
							}

							if ( verbose )
								console::info( "extractPrimitive: Extracted {} colors", colors.size() );
						}
					}

					// Create vertices with extracted data
					for ( size_t i = 0; i < positions.size(); ++i )
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
							vertex.normal = math::Vec3f{ 0.0f, 0.0f, 1.0f };
						}

						// Use extracted UVs or default
						if ( i < uvs.size() )
						{
							vertex.texCoord = uvs[i];
						}
						else
						{
							vertex.texCoord = math::Vec2f{ 0.0f, 0.0f };
						}

						// Use extracted tangents or default
						if ( i < tangents.size() )
						{
							vertex.tangent = tangents[i];
						}
						else
						{
							vertex.tangent = math::Vec4f{ 1.0f, 0.0f, 0.0f, 1.0f };
						}

						// Use extracted colors or default (white)
						if ( i < colors.size() )
						{
							vertex.color = colors[i];
						}
						else
						{
							vertex.color = math::Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
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
		// Calculate material index from the material pointer
		const cgltf_size materialIndex = primitive->material - data->materials;

		// Validate the material index and assign the corresponding handle
		if ( materialIndex < materialHandles.size() && materialHandles[materialIndex] != assets::INVALID_MATERIAL_HANDLE )
		{
			primitiveObj->setMaterialHandle( materialHandles[materialIndex] );

			if ( verbose )
				console::info( "extractPrimitive: Assigned material handle: {}", materialHandles[materialIndex] );
		}
		else
		{
			console::error( "extractPrimitive: Invalid material index {} or material handle", materialIndex );
		}
	}

	return primitiveObj;
}

std::shared_ptr<assets::Material> GLTFLoader::extractMaterial( cgltf_material *gltfMaterial, cgltf_data *data, bool verbose ) const
{
	if ( !gltfMaterial )
	{
		console::error( "extractMaterial: Invalid material pointer" );
		return nullptr;
	}

	if ( verbose )
		console::info( "extractMaterial: Processing material '{}'", gltfMaterial->name ? gltfMaterial->name : "Unnamed" );

	auto material = std::make_shared<assets::Material>();

	// Set material name if available
	if ( gltfMaterial->name )
	{
		material->setName( gltfMaterial->name );
	}

	auto &pbrMaterial = material->getPBRMaterial();

	// Extract PBR Metallic Roughness properties
	if ( gltfMaterial->has_pbr_metallic_roughness )
	{
		const auto &pbr = gltfMaterial->pbr_metallic_roughness;

		// Extract base color factor (default: [1.0, 1.0, 1.0, 1.0])
		pbrMaterial.baseColorFactor.x = pbr.base_color_factor[0];
		pbrMaterial.baseColorFactor.y = pbr.base_color_factor[1];
		pbrMaterial.baseColorFactor.z = pbr.base_color_factor[2];
		pbrMaterial.baseColorFactor.w = pbr.base_color_factor[3];

		// Extract metallic factor (default: 1.0)
		pbrMaterial.metallicFactor = pbr.metallic_factor;

		// Extract roughness factor (default: 1.0)
		pbrMaterial.roughnessFactor = pbr.roughness_factor;

		if ( verbose )
		{
			console::info( "extractMaterial: Base color factor: [{}, {}, {}, {}]",
				pbrMaterial.baseColorFactor.x,
				pbrMaterial.baseColorFactor.y,
				pbrMaterial.baseColorFactor.z,
				pbrMaterial.baseColorFactor.w );
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
	pbrMaterial.emissiveFactor.x = gltfMaterial->emissive_factor[0];
	pbrMaterial.emissiveFactor.y = gltfMaterial->emissive_factor[1];
	pbrMaterial.emissiveFactor.z = gltfMaterial->emissive_factor[2];

	if ( verbose )
	{
		console::info( "extractMaterial: Emissive factor: [{}, {}, {}]",
			pbrMaterial.emissiveFactor.x,
			pbrMaterial.emissiveFactor.y,
			pbrMaterial.emissiveFactor.z );
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

std::span<const std::uint8_t> GLTFLoader::getAccessorData( cgltf_accessor *accessor, cgltf_data * /*data*/ ) const
{
	if ( !accessor || !accessor->buffer_view )
		return {};

	cgltf_buffer_view *bufferView = accessor->buffer_view;
	cgltf_buffer *buffer = bufferView->buffer;

	if ( !buffer || !buffer->data )
		return {};

	const std::uint8_t *start = reinterpret_cast<const std::uint8_t *>( buffer->data ) + bufferView->offset + accessor->offset;
	size_t dataSize = accessor->count * cgltf_num_components( accessor->type ) * cgltf_component_size( accessor->component_type );

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

	// Convert glTF matrix (column-major, 4x4) to Mat4 using row-major constructor
	// glTF matrix layout: [m0,m1,m2,m3, m4,m5,m6,m7, m8,m9,m10,m11, m12,m13,m14,m15]
	// Need to transpose for row-major Mat4 constructor

	// clang-format off
	const math::Mat4<float> mat4{
		matrix[0], matrix[4], matrix[8], matrix[12],   // Row 0: m0, m4, m8, m12
		matrix[1], matrix[5], matrix[9], matrix[13],   // Row 1: m1, m5, m9, m13
		matrix[2], matrix[6], matrix[10], matrix[14],  // Row 2: m2, m6, m10, m14
		matrix[3], matrix[7], matrix[11], matrix[15]   // Row 3: m3, m7, m11, m15
	};
	// clang-format on

	// Extract translation directly from the last column (4th column in our row-major representation)
	transform.position.x = mat4.m03();
	transform.position.y = mat4.m13();
	transform.position.z = mat4.m23();

	// Extract scale using Mat4's extractScale functionality
	const auto scale = mat4.extractScale();
	transform.scale = scale;

	// Extract rotation by converting to Mat3, normalizing to remove scale, then to Euler angles
	const auto rotationMatrix = mat4.toMat3();
	const auto normalizedRotation = rotationMatrix.normalize();
	transform.rotation = normalizedRotation.toEulerAngles();

	return transform;
}

math::Vec3f GLTFLoader::quaternionToEulerAngles( float x, float y, float z, float w ) const
{
	// Create a quaternion and use the built-in conversion
	const math::Quatf quat{ w, x, y, z };
	return quat.toEulerAngles();
}

} // namespace gltf_loader

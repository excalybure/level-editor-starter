// Global module fragment for C headers
module;
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

export module engine.gltf_loader;

import std;
import engine.assets;
import engine.vec;
import runtime.console;

export namespace gltf_loader
{

// Type aliases for convenience
using Vec2f = math::Vec2f;
using Vec3f = math::Vec3f;
using Vec4f = math::Vec4f;

// Component types from glTF specification
export enum class ComponentType : std::uint32_t {
	Byte = 5120,
	UnsignedByte = 5121,
	Short = 5122,
	UnsignedShort = 5123,
	UnsignedInt = 5125,
	Float = 5126
};

// Attribute types for validation
export enum class AttributeType {
	Position,
	Normal,
	Tangent,
	TexCoord,
	Indices
};

// Utility function implementations for accessor data extraction

export std::vector<Vec3f> extractFloat3Positions( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	// Calculate effective stride (minimum of 12 bytes for float3 or specified stride)
	const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 3 * sizeof( float ) );

	std::vector<Vec3f> positions;
	positions.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *vertexBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *vertex = reinterpret_cast<const float *>( vertexBytes );
		positions.push_back( { vertex[0], vertex[1], vertex[2] } );
	}

	return positions;
}

export std::vector<Vec3f> extractFloat3Normals( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	// Use the same logic as positions since they're both float3, but implemented directly for clarity
	const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 3 * sizeof( float ) );

	std::vector<Vec3f> normals;
	normals.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *normalBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *normal = reinterpret_cast<const float *>( normalBytes );
		normals.push_back( { normal[0], normal[1], normal[2] } );
	}

	return normals;
}

export std::vector<Vec2f> extractFloat2UVs( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 2 * sizeof( float ) );

	std::vector<Vec2f> uvs;
	uvs.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *uvBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *uv = reinterpret_cast<const float *>( uvBytes );
		uvs.push_back( { uv[0], uv[1] } );
	}

	return uvs;
}

export std::vector<Vec4f> extractFloat4Tangents( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : ( 4 * sizeof( float ) );

	std::vector<Vec4f> tangents;
	tangents.reserve( count );

	// Work with byte-level addressing to handle unaligned offsets
	const std::uint8_t *byteBuffer = reinterpret_cast<const std::uint8_t *>( buffer );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const std::uint8_t *tangentBytes = byteBuffer + byteOffset + ( i * effectiveStride );
		const float *tangent = reinterpret_cast<const float *>( tangentBytes );
		tangents.push_back( { tangent[0], tangent[1], tangent[2], tangent[3] } );
	}

	return tangents;
}

export std::vector<std::uint32_t> extractIndicesAsUint32( const std::uint8_t *buffer, std::size_t count, ComponentType componentType, std::size_t byteOffset, std::size_t byteStride )
{
	std::vector<std::uint32_t> indices;
	indices.reserve( count );

	const std::uint8_t *data = buffer + byteOffset;

	switch ( componentType )
	{
	case ComponentType::UnsignedByte: {
		const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint8_t );
		for ( std::size_t i = 0; i < count; ++i )
		{
			const std::uint8_t *indexPtr = data + ( i * effectiveStride );
			std::uint8_t indexValue = *indexPtr;
			indices.push_back( static_cast<std::uint32_t>( indexValue ) );
		}
		break;
	}
	case ComponentType::UnsignedShort: {
		const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint16_t );
		for ( std::size_t i = 0; i < count; ++i )
		{
			const std::uint8_t *indexPtr = data + ( i * effectiveStride );
			const std::uint16_t indexValue = *reinterpret_cast<const std::uint16_t *>( indexPtr );
			indices.push_back( static_cast<std::uint32_t>( indexValue ) );
		}
		break;
	}
	case ComponentType::UnsignedInt: {
		const std::size_t effectiveStride = ( byteStride > 0 ) ? byteStride : sizeof( std::uint32_t );
		for ( std::size_t i = 0; i < count; ++i )
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

export void validateComponentType( ComponentType componentType, AttributeType attributeType )
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

export class GLTFLoader
{
public:
	GLTFLoader();

	// Main entry point for loading glTF scenes
	std::unique_ptr<assets::Scene> loadScene( const std::string &filePath ) const;

	// For testing: load from string content
	std::unique_ptr<assets::Scene> loadFromString( const std::string &gltfContent ) const;

private:
	// Helper methods for glTF processing (use void* to avoid forward declaration issues)
	std::unique_ptr<assets::SceneNode> processNode( cgltf_node *gltfNode, cgltf_data *data ) const;

	// Mesh extraction helpers
	std::shared_ptr<assets::Mesh> extractMesh( cgltf_mesh *gltfMesh, cgltf_data *data, bool verbose = false ) const;
	std::unique_ptr<assets::Primitive> extractPrimitive( cgltf_primitive *gltfPrimitive, cgltf_data *data, bool verbose = false ) const;

	// Material extraction helpers
	std::shared_ptr<assets::Material> extractMaterial( void *gltfMaterial, void *data, bool verbose = false ) const;
	std::string extractTextureURI( void *textureInfo, void *data ) const;

	// Helper to get accessor data as typed spans
	std::span<const std::uint8_t> getAccessorData( void *accessor, void *data ) const;
};

} // namespace gltf_loader

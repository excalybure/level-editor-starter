// Global module fragment for C headers
module;
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

export module engine.gltf_loader;

import std;
import engine.assets;

export namespace gltf_loader
{

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

export std::vector<std::array<float, 3>> extractFloat3Positions( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	// Calculate effective stride (minimum of 12 bytes for float3 or specified stride)
	const std::size_t floatOffset = byteOffset / sizeof( float );
	const std::size_t effectiveStride = ( byteStride > 0 ) ? ( byteStride / sizeof( float ) ) : 3;

	std::vector<std::array<float, 3>> positions;
	positions.reserve( count );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const float *vertex = buffer + floatOffset + ( i * effectiveStride );
		positions.push_back( { vertex[0], vertex[1], vertex[2] } );
	}

	return positions;
}

export std::vector<std::array<float, 3>> extractFloat3Normals( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	// Reuse the same logic as positions since they're both float3
	return extractFloat3Positions( buffer, count, byteOffset, byteStride );
}

export std::vector<std::array<float, 2>> extractFloat2UVs( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	const std::size_t floatOffset = byteOffset / sizeof( float );
	const std::size_t effectiveStride = ( byteStride > 0 ) ? ( byteStride / sizeof( float ) ) : 2;

	std::vector<std::array<float, 2>> uvs;
	uvs.reserve( count );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const float *uv = buffer + floatOffset + ( i * effectiveStride );
		uvs.push_back( { uv[0], uv[1] } );
	}

	return uvs;
}

export std::vector<std::array<float, 4>> extractFloat4Tangents( const float *buffer, std::size_t count, std::size_t byteOffset, std::size_t byteStride )
{
	const std::size_t floatOffset = byteOffset / sizeof( float );
	const std::size_t effectiveStride = ( byteStride > 0 ) ? ( byteStride / sizeof( float ) ) : 4;

	std::vector<std::array<float, 4>> tangents;
	tangents.reserve( count );

	for ( std::size_t i = 0; i < count; ++i )
	{
		const float *tangent = buffer + floatOffset + ( i * effectiveStride );
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
			indices.push_back( static_cast<std::uint32_t>( *indexPtr ) );
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
	std::unique_ptr<assets::SceneNode> processNode( void *gltfNode, void *data ) const;
	// TODO: Add private helper methods for actual cgltf processing when needed
	// These would be implemented when we add full glTF support:
	// std::unique_ptr<assets::Mesh> loadMesh(const cgltf::mesh& gltfMesh, const cgltf::model& model);
	// std::unique_ptr<assets::Material> loadMaterial(const cgltf::material& gltfMaterial);
	// assets::Vertex extractVertex(const cgltf::model& model, const cgltf::primitive& primitive, size_t index);
	// void extractTransform(const cgltf::node& node, assets::SceneNode& sceneNode);
	// std::unique_ptr<assets::SceneNode> processNode(const cgltf::node& node, const cgltf::model& model);
	// void calculateBounds(assets::Mesh* mesh);
};

} // namespace gltf_loader

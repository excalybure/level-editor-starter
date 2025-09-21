#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <cgltf.h>
#undef _CRT_SECURE_NO_WARNINGS

#include "../math/math.h"
#include "../math/vec.h"
#include "../math/matrix.h"
#include "../math/quat.h"
#include "../assets/assets.h"
#include <memory>
#include <vector>
#include <string>
#include <span>
#include <cstdint>
#include <stdexcept>

// Forward declarations for cgltf types (temporarily)
struct cgltf_data;
struct cgltf_node;
struct cgltf_mesh;
struct cgltf_material;
struct cgltf_primitive;
struct cgltf_texture_view;
struct cgltf_accessor;

namespace gltf_loader
{

// Component types from glTF specification
enum class ComponentType : uint32_t
{
	Byte = 5120,
	UnsignedByte = 5121,
	Short = 5122,
	UnsignedShort = 5123,
	UnsignedInt = 5125,
	Float = 5126
};

// Attribute types for validation
enum class AttributeType
{
	Position,
	Normal,
	Tangent,
	TexCoord,
	Indices
};

// Utility function declarations for accessor data extraction
std::vector<math::Vec3f> extractFloat3Positions( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<math::Vec3f> extractFloat3Normals( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<math::Vec2f> extractFloat2UVs( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<math::Vec4f> extractFloat4Tangents( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<math::Vec4f> extractFloat4Colors( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<math::Vec4f> extractFloat3ColorsAsVec4( const float *buffer, size_t count, size_t byteOffset, size_t byteStride );
std::vector<uint32_t> extractIndicesAsUint32( const std::uint8_t *buffer, size_t count, ComponentType componentType, size_t byteOffset, size_t byteStride );
void validateComponentType( ComponentType componentType, AttributeType attributeType );

class GLTFLoader
{
public:
	GLTFLoader();

	// Main entry point for loading glTF scenes
	std::unique_ptr<assets::Scene> loadScene( const std::string &filePath ) const;

	// For testing: load from string content
	std::unique_ptr<assets::Scene> loadFromString( const std::string &gltfContent ) const;

private:
	// Helper methods for glTF processing (use void* to avoid forward declaration issues)
	std::unique_ptr<assets::Scene> processSceneData( cgltf_data *data ) const;

	std::unique_ptr<assets::SceneNode> processNode(
		cgltf_node *gltfNode,
		cgltf_data *data,
		const std::vector<assets::MeshHandle> &meshHandles,
		const std::vector<assets::MaterialHandle> &materialHandles ) const;

	// Mesh extraction helpers
	std::shared_ptr<assets::Mesh> extractMesh( cgltf_mesh *gltfMesh, cgltf_data *data, const std::vector<assets::MaterialHandle> &materialHandles, bool verbose = false ) const;
	std::unique_ptr<assets::Primitive> extractPrimitive( cgltf_primitive *gltfPrimitive, cgltf_data *data, const std::vector<assets::MaterialHandle> &materialHandles, bool verbose = false ) const;

	// Material extraction helpers
	std::shared_ptr<assets::Material> extractMaterial( cgltf_material *gltfMaterial, cgltf_data *data, bool verbose = false ) const;
	std::string extractTextureURI( void *textureInfo, void *data ) const;

	// Transform extraction helpers
	assets::Transform extractTransformFromNode( cgltf_node *gltfNode ) const;
	assets::Transform extractTransformFromTRS( const float *translation, const float *rotation, const float *scale ) const;
	assets::Transform extractTransformFromMatrix( const float *matrix ) const;
	math::Vec3f quaternionToEulerAngles( float x, float y, float z, float w ) const;

	// Helper to get accessor data as typed spans
	std::span<const std::uint8_t> getAccessorData( cgltf_accessor *accessor, cgltf_data *data ) const;
};

} // namespace gltf_loader

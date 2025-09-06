export module engine.gltf_loader;

import std;
import engine.assets;

// Forward declarations for cgltf types
namespace cgltf
{
struct model;
struct mesh;
struct material;
struct node;
struct primitive;
} // namespace cgltf

export namespace gltf_loader
{

export class GLTFLoader
{
public:
	GLTFLoader();

	// Main entry point for loading glTF scenes
	std::unique_ptr<assets::Scene> loadScene( const std::string &filePath ) const;

private:
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

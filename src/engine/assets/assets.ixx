export module engine.assets;

import std;

export namespace assets
{

// Asset types
export enum class AssetType {
	Unknown,
	Mesh,
	Material,
	Texture,
	Scene
};

// Base asset interface
export class Asset
{
public:
	virtual ~Asset() = default;
	virtual AssetType getType() const = 0;

	const std::string &getPath() const { return m_path; }
	bool isLoaded() const { return m_loaded; }

protected:
	std::string m_path;
	bool m_loaded = false;
};

// Simple vertex structure for now
export struct Vertex
{
	float position[3] = { 0.0f, 0.0f, 0.0f };
	float normal[3] = { 0.0f, 1.0f, 0.0f };
	float texCoord[2] = { 0.0f, 0.0f };
	float tangent[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
};

export class Mesh : public Asset
{
public:
	AssetType getType() const override { return AssetType::Mesh; }

	const std::vector<Vertex> &getVertices() const { return m_vertices; }
	const std::vector<std::uint32_t> &getIndices() const { return m_indices; }

	// Vertex count accessors
	std::uint32_t getVertexCount() const { return static_cast<std::uint32_t>( m_vertices.size() ); }
	std::uint32_t getIndexCount() const { return static_cast<std::uint32_t>( m_indices.size() ); }

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;
};

// Material representation
export class Material : public Asset
{
public:
	AssetType getType() const override { return AssetType::Material; }

	struct PBRMaterial
	{
		float baseColorFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float metallicFactor = 0.0f;
		float roughnessFactor = 1.0f;
		float emissiveFactor[3] = { 0.0f, 0.0f, 0.0f };

		std::string baseColorTexture;
		std::string metallicRoughnessTexture;
		std::string normalTexture;
		std::string emissiveTexture;
	};

	const PBRMaterial &getPBRMaterial() const { return m_pbrMaterial; }
	PBRMaterial &getPBRMaterial() { return m_pbrMaterial; }

private:
	PBRMaterial m_pbrMaterial;
};

// Simple scene node structure
export struct SceneNode
{
	std::string name;
	std::vector<std::string> meshes;	// Mesh asset paths
	std::vector<std::string> materials; // Material asset paths
	std::vector<std::unique_ptr<SceneNode>> children;

	SceneNode() = default;
	SceneNode( const std::string &nodeName ) : name( nodeName ) {}

	// Utility methods
	bool hasMesh() const { return !meshes.empty(); }
	bool hasMaterial() const { return !materials.empty(); }
	bool hasChildren() const { return !children.empty(); }
};

export class Scene : public Asset
{
public:
	AssetType getType() const override { return AssetType::Scene; }

	const std::vector<std::unique_ptr<SceneNode>> &getRootNodes() const
	{
		return m_rootNodes;
	}

	std::vector<std::unique_ptr<SceneNode>> &getRootNodes()
	{
		return m_rootNodes;
	}

	// Scene operations
	void addRootNode( std::unique_ptr<SceneNode> node )
	{
		if ( node )
		{
			m_rootNodes.push_back( std::move( node ) );
		}
	}

	std::size_t getTotalNodeCount() const
	{
		return m_rootNodes.size(); // Simplified for now
	}

private:
	std::vector<std::unique_ptr<SceneNode>> m_rootNodes;
};

} // namespace assets

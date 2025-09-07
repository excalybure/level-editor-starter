export module engine.assets;

import std;
import engine.vec;
import engine.bounding_box_3d;

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
	math::Vec3f position = { 0.0f, 0.0f, 0.0f };
	math::Vec3f normal = { 0.0f, 1.0f, 0.0f };
	math::Vec2f texCoord = { 0.0f, 0.0f };
	math::Vec4f tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
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

	// Methods for building mesh data
	void addVertex( const Vertex &vertex )
	{
		m_vertices.push_back( vertex );
		updateBounds( vertex.position );
	}

	void addIndex( std::uint32_t index ) { m_indices.push_back( index ); }

	void clearVertices()
	{
		m_vertices.clear();
		resetBounds();
	}

	void clearIndices() { m_indices.clear(); }

	void reserveVertices( std::size_t count ) { m_vertices.reserve( count ); }
	void reserveIndices( std::size_t count ) { m_indices.reserve( count ); }

	// Bounding box accessors
	const math::BoundingBox3Df &getBounds() const { return m_bounds; }
	bool hasBounds() const { return m_bounds.isValid(); }

	// Compute bounds center and size
	void getBoundsCenter( float center[3] ) const
	{
		if ( !m_bounds.isValid() )
		{
			center[0] = center[1] = center[2] = 0.0f;
			return;
		}
		const auto centerVec = m_bounds.center();
		center[0] = centerVec.x;
		center[1] = centerVec.y;
		center[2] = centerVec.z;
	}

	void getBoundsSize( float size[3] ) const
	{
		if ( !m_bounds.isValid() )
		{
			size[0] = size[1] = size[2] = 0.0f;
			return;
		}
		const auto sizeVec = m_bounds.size();
		size[0] = sizeVec.x;
		size[1] = sizeVec.y;
		size[2] = sizeVec.z;
	}

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;

	// Bounding box data
	math::BoundingBox3Df m_bounds;

	void updateBounds( const math::Vec3f &position )
	{
		m_bounds.expand( position );
	}

	void resetBounds()
	{
		m_bounds = math::BoundingBox3Df{};
	}
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
	std::vector<std::string> materials; // Material asset paths
	std::vector<std::unique_ptr<SceneNode>> children;

	// NEW: Store actual mesh objects directly for simple cases
	std::vector<std::shared_ptr<Mesh>> meshObjects;

	SceneNode() = default;
	SceneNode( const std::string &nodeName ) : name( nodeName ) {}

	// Utility methods
	bool hasMesh() const { return !meshObjects.empty(); }
	bool hasMaterial() const { return !materials.empty(); }
	bool hasChildren() const { return !children.empty(); }

	// NEW: Get first mesh object for direct access
	std::shared_ptr<Mesh> getFirstMesh() const
	{
		return meshObjects.empty() ? nullptr : meshObjects[0];
	}

	// NEW: Add mesh object
	void addMeshObject( std::shared_ptr<Mesh> mesh )
	{
		if ( mesh )
		{
			meshObjects.push_back( mesh );
		}
	}
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

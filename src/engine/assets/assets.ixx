export module engine.assets;

import std;
import engine.vec;

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
	math::Vec3<float> position = { 0.0f, 0.0f, 0.0f };
	math::Vec3<float> normal = { 0.0f, 1.0f, 0.0f };
	math::Vec2<float> texCoord = { 0.0f, 0.0f };
	math::Vec4<float> tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
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
	const float *getBoundsMin() const { return m_boundsMin; }
	const float *getBoundsMax() const { return m_boundsMax; }
	bool hasBounds() const { return m_hasBounds; }

	// Compute bounds center and size
	void getBoundsCenter( float center[3] ) const
	{
		if ( !m_hasBounds )
		{
			center[0] = center[1] = center[2] = 0.0f;
			return;
		}
		center[0] = ( m_boundsMin[0] + m_boundsMax[0] ) * 0.5f;
		center[1] = ( m_boundsMin[1] + m_boundsMax[1] ) * 0.5f;
		center[2] = ( m_boundsMin[2] + m_boundsMax[2] ) * 0.5f;
	}

	void getBoundsSize( float size[3] ) const
	{
		if ( !m_hasBounds )
		{
			size[0] = size[1] = size[2] = 0.0f;
			return;
		}
		size[0] = m_boundsMax[0] - m_boundsMin[0];
		size[1] = m_boundsMax[1] - m_boundsMin[1];
		size[2] = m_boundsMax[2] - m_boundsMin[2];
	}

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;

	// Bounding box data
	float m_boundsMin[3] = { 0.0f, 0.0f, 0.0f };
	float m_boundsMax[3] = { 0.0f, 0.0f, 0.0f };
	bool m_hasBounds = false;

	void updateBounds( const math::Vec3<float> &position )
	{
		if ( !m_hasBounds )
		{
			m_boundsMin[0] = position.x;
			m_boundsMin[1] = position.y;
			m_boundsMin[2] = position.z;
			m_boundsMax[0] = position.x;
			m_boundsMax[1] = position.y;
			m_boundsMax[2] = position.z;
			m_hasBounds = true;
		}
		else
		{
			m_boundsMin[0] = std::min( m_boundsMin[0], position.x );
			m_boundsMin[1] = std::min( m_boundsMin[1], position.y );
			m_boundsMin[2] = std::min( m_boundsMin[2], position.z );

			m_boundsMax[0] = std::max( m_boundsMax[0], position.x );
			m_boundsMax[1] = std::max( m_boundsMax[1], position.y );
			m_boundsMax[2] = std::max( m_boundsMax[2], position.z );
		}
	}

	void resetBounds()
	{
		m_hasBounds = false;
		m_boundsMin[0] = m_boundsMin[1] = m_boundsMin[2] = 0.0f;
		m_boundsMax[0] = m_boundsMax[1] = m_boundsMax[2] = 0.0f;
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
	std::vector<std::string> meshes;	// Mesh asset paths (legacy)
	std::vector<std::string> materials; // Material asset paths
	std::vector<std::unique_ptr<SceneNode>> children;

	// NEW: Store actual mesh objects directly for simple cases
	std::vector<std::shared_ptr<Mesh>> meshObjects;

	SceneNode() = default;
	SceneNode( const std::string &nodeName ) : name( nodeName ) {}

	// Utility methods
	bool hasMesh() const { return !meshes.empty() || !meshObjects.empty(); }
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

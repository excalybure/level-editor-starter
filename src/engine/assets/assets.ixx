export module engine.assets;

import std;
import engine.vec;
import engine.math;
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

// Handle types for index-based resource references
export using MeshHandle = std::size_t;
export using MaterialHandle = std::size_t;

// Invalid handle constants
export constexpr MeshHandle INVALID_MESH_HANDLE = std::numeric_limits<MeshHandle>::max();
export constexpr MaterialHandle INVALID_MATERIAL_HANDLE = std::numeric_limits<MaterialHandle>::max();

// Simple transform structure for scene nodes
export struct Transform
{
	math::Vec3f position{ 0.0f, 0.0f, 0.0f };
	math::Vec3f rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles (radians)
	math::Vec3f scale{ 1.0f, 1.0f, 1.0f };

	Transform() = default;
	Transform( const math::Vec3f &pos, const math::Vec3f &rot, const math::Vec3f &scl )
		: position( pos ), rotation( rot ), scale( scl ) {}
};

// Base asset interface
export class Asset
{
public:
	virtual ~Asset() = default;
	virtual AssetType getType() const = 0;

	const std::string &getPath() const { return m_path; }
	bool isLoaded() const { return m_loaded; }

	// Setters for asset management
	void setPath( const std::string &path ) { m_path = path; }
	void setLoaded( bool loaded ) { m_loaded = loaded; }

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

// Material representation
export class Material : public Asset
{
public:
	AssetType getType() const override { return AssetType::Material; }

	struct PBRMaterial
	{
		math::Vec4f baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float metallicFactor = 0.0f;
		float roughnessFactor = 1.0f;
		math::Vec3f emissiveFactor = { 0.0f, 0.0f, 0.0f };

		std::string baseColorTexture;
		std::string metallicRoughnessTexture;
		std::string normalTexture;
		std::string emissiveTexture;
	};

	const PBRMaterial &getPBRMaterial() const { return m_pbrMaterial; }
	PBRMaterial &getPBRMaterial() { return m_pbrMaterial; }

	// Name functionality
	const std::string &getName() const { return m_name; }
	void setName( const std::string &name ) { m_name = name; }

	// PBR material setter methods
	void setBaseColorFactor( float r, float g, float b, float a )
	{
		m_pbrMaterial.baseColorFactor = { r, g, b, a };
	}

	void setMetallicFactor( float metallic )
	{
		m_pbrMaterial.metallicFactor = metallic;
	}

	void setRoughnessFactor( float roughness )
	{
		m_pbrMaterial.roughnessFactor = roughness;
	}

private:
	std::string m_name;
	PBRMaterial m_pbrMaterial;
};

// Primitive class - represents a single drawable primitive with its own vertex/index data and material
export class Primitive
{
public:
	// Vertex and index accessors
	const std::vector<Vertex> &getVertices() const { return m_vertices; }
	const std::vector<std::uint32_t> &getIndices() const { return m_indices; }

	// Vertex count accessors
	std::uint32_t getVertexCount() const { return static_cast<std::uint32_t>( m_vertices.size() ); }
	std::uint32_t getIndexCount() const { return static_cast<std::uint32_t>( m_indices.size() ); }

	// Methods for building primitive data
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

	// Material reference
	const std::string &getMaterialPath() const { return m_materialPath; }
	void setMaterialPath( const std::string &path ) { m_materialPath = path; }
	bool hasMaterial() const { return !m_materialPath.empty(); }

	// Compute bounds center and size
	math::Vec3f getBoundsCenter() const
	{
		if ( !m_bounds.isValid() )
			return { 0.0f, 0.0f, 0.0f };
		return m_bounds.center();
	}

	math::Vec3f getBoundsSize() const
	{
		if ( !m_bounds.isValid() )
			return { 0.0f, 0.0f, 0.0f };
		return m_bounds.size();
	}

private:
	std::vector<Vertex> m_vertices;
	std::vector<std::uint32_t> m_indices;
	std::string m_materialPath;

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

export class Mesh : public Asset
{
public:
	AssetType getType() const override { return AssetType::Mesh; }

	// Primitive-based access
	const std::vector<Primitive> &getPrimitives() const { return m_primitives; }
	std::vector<Primitive> &getPrimitives() { return m_primitives; }

	std::uint32_t getPrimitiveCount() const { return static_cast<std::uint32_t>( m_primitives.size() ); }

	const Primitive &getPrimitive( std::uint32_t index ) const
	{
		return m_primitives.at( index );
	}

	Primitive &getPrimitive( std::uint32_t index )
	{
		return m_primitives.at( index );
	}

	// Add a new primitive to this mesh
	void addPrimitive( const Primitive &primitive )
	{
		m_primitives.push_back( primitive );
		updateBounds( primitive.getBounds() );
	}

	void addPrimitive( Primitive &&primitive )
	{
		updateBounds( primitive.getBounds() );
		m_primitives.push_back( std::move( primitive ) );
	}


	// Bounding box accessors - aggregate from all primitives
	const math::BoundingBox3Df &getBounds() const { return m_bounds; }
	bool hasBounds() const { return m_bounds.isValid(); }

	// Compute bounds center and size
	math::Vec3f getBoundsCenter() const
	{
		if ( !m_bounds.isValid() )
			return { 0.0f, 0.0f, 0.0f };
		return m_bounds.center();
	}

	math::Vec3f getBoundsSize() const
	{
		if ( !m_bounds.isValid() )
			return { 0.0f, 0.0f, 0.0f };
		return m_bounds.size();
	}

	// Recalculate bounds from all primitives (call after modifying primitives directly)
	void recalculateBounds()
	{
		m_bounds = math::BoundingBox3Df{};
		// Recalculate from all primitives
		for ( const auto &primitive : m_primitives )
		{
			updateBounds( primitive.getBounds() );
		}
	}

private:
	std::vector<Primitive> m_primitives;

	// Aggregate bounding box data from all primitives
	math::BoundingBox3Df m_bounds;

	void updateBounds( const math::BoundingBox3Df &primitiveBounds )
	{
		if ( primitiveBounds.isValid() )
		{
			m_bounds.expand( primitiveBounds.min );
			m_bounds.expand( primitiveBounds.max );
		}
	}
};

// Scene node class with proper encapsulation
export class SceneNode
{
public:
	SceneNode() = default;
	SceneNode( const std::string &nodeName ) : m_name( nodeName ) {}

	// Name accessors
	const std::string &getName() const { return m_name; }
	void setName( const std::string &name ) { m_name = name; }

	// Children accessors
	bool hasChildren() const { return !m_children.empty(); }
	size_t getChildCount() const { return m_children.size(); }

	const SceneNode &getChild( size_t index ) const { return *m_children.at( index ); }

	template <typename Func>
	void foreachChild( Func &&func ) const
	{
		for ( const auto &child : m_children )
		{
			func( *child );
		}
	}

	void addChild( std::unique_ptr<SceneNode> child )
	{
		if ( child )
		{
			m_children.push_back( std::move( child ) );
		}
	}

	// Mesh handle accessors
	const std::vector<MeshHandle> &getMeshHandles() const { return m_meshHandles; }
	bool hasMeshHandles() const { return !m_meshHandles.empty(); }
	size_t meshCount() const { return m_meshHandles.size(); }
	MeshHandle getMeshHandle( size_t index ) const { return m_meshHandles.at( index ); }
	template <typename Func>
	void foreachMeshHandle( Func &&func ) const
	{
		for ( const auto &handle : m_meshHandles )
		{
			func( handle );
		}
	}
	void addMeshHandle( MeshHandle handle )
	{
		if ( handle != INVALID_MESH_HANDLE )
		{
			m_meshHandles.push_back( handle );
		}
	}

	// Transform accessors
	bool hasTransform() const { return m_hasTransformData; }
	const Transform &getTransform() const { return m_transform; }
	void setTransform( const Transform &t )
	{
		m_transform = t;
		m_hasTransformData = true;
	}

private:
	std::string m_name;
	std::vector<std::unique_ptr<SceneNode>> m_children;
	std::vector<MeshHandle> m_meshHandles;
	Transform m_transform;
	bool m_hasTransformData = false;
};

export class Scene : public Asset
{
public:
	AssetType getType() const override { return AssetType::Scene; }

	// Root-level resource collections (matching glTF structure)
	const std::vector<std::shared_ptr<Material>> &getMaterials() const { return m_materials; }
	const std::vector<std::shared_ptr<Mesh>> &getMeshes() const { return m_meshes; }

	// Resource management with handle-based access
	MaterialHandle addMaterial( std::shared_ptr<Material> material )
	{
		if ( !material )
			return INVALID_MATERIAL_HANDLE;
		m_materials.push_back( material );
		return static_cast<MaterialHandle>( m_materials.size() - 1 );
	}

	MeshHandle addMesh( std::shared_ptr<Mesh> mesh )
	{
		if ( !mesh )
			return INVALID_MESH_HANDLE;
		m_meshes.push_back( mesh );
		return static_cast<MeshHandle>( m_meshes.size() - 1 );
	}

	// Safe indexed access with bounds checking
	std::shared_ptr<Material> getMaterial( MaterialHandle handle ) const
	{
		return ( handle < m_materials.size() ) ? m_materials[handle] : nullptr;
	}

	std::shared_ptr<Mesh> getMesh( MeshHandle handle ) const
	{
		return ( handle < m_meshes.size() ) ? m_meshes[handle] : nullptr;
	}

	// Validation
	bool isValidMaterialHandle( MaterialHandle handle ) const
	{
		return handle < m_materials.size();
	}

	bool isValidMeshHandle( MeshHandle handle ) const
	{
		return handle < m_meshes.size();
	}

	// Resource counts
	std::size_t getMaterialCount() const { return m_materials.size(); }
	std::size_t getMeshCount() const { return m_meshes.size(); }

	// Scene graph access (unchanged)
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
	// Root-level resource collections (matching glTF structure)
	std::vector<std::shared_ptr<Material>> m_materials;
	std::vector<std::shared_ptr<Mesh>> m_meshes;

	// Scene graph
	std::vector<std::unique_ptr<SceneNode>> m_rootNodes;
};

} // namespace assets

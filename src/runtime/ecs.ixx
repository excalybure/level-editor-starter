export module runtime.ecs;
import std;
import <cstdint>;
import <memory>;
import <unordered_map>;
import <unordered_set>;
import <vector>;
import <queue>;
import <span>;
import <typeindex>;

export namespace ecs
{

// Enhanced Entity with generation for safe handles
export struct Entity
{
	std::uint32_t id{};
	std::uint32_t generation{};

	auto operator<=>( const Entity & ) const = default;
	bool operator==( const Entity &other ) const = default;

	bool isValid() const noexcept
	{
		return id != 0; // ID 0 is reserved for invalid entities
	}
};

} // namespace ecs

// Hash function for Entity to enable use in std::unordered_map
template <>
struct std::hash<ecs::Entity>
{
	std::size_t operator()( const ecs::Entity &entity ) const noexcept
	{
		// Combine id and generation for hash
		const std::uint64_t key = ( static_cast<std::uint64_t>( entity.generation ) << 32 ) | static_cast<std::uint64_t>( entity.id );
		return std::hash<std::uint64_t>{}( key );
	}
};

export namespace ecs
{

// Entity manager with recycling and generation tracking
export class EntityManager
{
public:
	Entity create()
	{
		std::uint32_t id;
		if ( !m_freeIds.empty() )
		{
			// Reuse a previously destroyed entity ID
			id = m_freeIds.front();
			m_freeIds.pop();
			m_generations[id - 1]++; // Increment generation to invalidate old references
		}
		else
		{
			// Create new entity ID
			id = static_cast<std::uint32_t>( m_generations.size() + 1 ); // Start from 1, 0 is invalid
			m_generations.push_back( 0 );
		}

		Entity entity{ id, m_generations[id - 1] }; // Adjust for 0-based indexing
		if ( id > m_entities.size() )
		{
			m_entities.resize( id );
		}
		m_entities[id - 1] = entity; // Store entity at index (id - 1)

		return entity;
	}

	bool destroy( Entity entity )
	{
		if ( !isValid( entity ) )
		{
			return false;
		}

		// Increment generation to invalidate all existing references
		m_generations[entity.id - 1]++;

		// Add ID to free list for reuse
		m_freeIds.push( entity.id );

		// Clear the entity slot
		m_entities[entity.id - 1] = Entity{}; // Invalid entity

		return true;
	}

	bool isValid( Entity entity ) const
	{
		if ( entity.id == 0 || entity.id > m_generations.size() )
		{
			return false;
		}
		return m_generations[entity.id - 1] == entity.generation;
	}

	std::span<const Entity> getAllEntities() const
	{
		return std::span<const Entity>( m_entities );
	}

private:
	std::vector<std::uint32_t> m_generations;
	std::vector<Entity> m_entities;
	std::queue<std::uint32_t> m_freeIds;
};

// Component concept - relaxed to allow more practical component types
export template <class T>
concept Component = std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> && std::is_destructible_v<T>;

// Enhanced ComponentStorage with better performance
export template <Component C>
class ComponentStorage
{
public:
	bool add( Entity entity, C component )
	{
		if ( !entity.isValid() )
		{
			return false;
		}

		m_components[entity] = component;
		return true;
	}

	bool remove( Entity entity )
	{
		const auto it = m_components.find( entity );
		if ( it != m_components.end() )
		{
			m_components.erase( it );
			return true;
		}
		return false;
	}

	bool has( Entity entity ) const
	{
		return m_components.find( entity ) != m_components.end();
	}

	C *get( Entity entity )
	{
		const auto it = m_components.find( entity );
		return it != m_components.end() ? &it->second : nullptr;
	}

	const C *get( Entity entity ) const
	{
		const auto it = m_components.find( entity );
		return it != m_components.end() ? &it->second : nullptr;
	}

	// Iteration support
	auto begin() { return m_components.begin(); }
	auto end() { return m_components.end(); }
	auto begin() const { return m_components.begin(); }
	auto end() const { return m_components.end(); }

	size_t size() const { return m_components.size(); }
	bool empty() const { return m_components.empty(); }

	void clear() { m_components.clear(); }

private:
	std::unordered_map<Entity, C> m_components;
};

// Forward declaration for Scene
export class Scene;

// Base class for type-erased component storage
class ComponentStorageBase
{
public:
	virtual ~ComponentStorageBase() = default;
	virtual bool removeComponent( Entity entity ) = 0;
	virtual bool hasComponent( Entity entity ) const = 0;
};

// Type-erased wrapper for ComponentStorage
template <Component C>
class TypedComponentStorage : public ComponentStorageBase
{
public:
	ComponentStorage<C> storage;

	bool removeComponent( Entity entity ) override
	{
		return storage.remove( entity );
	}

	bool hasComponent( Entity entity ) const override
	{
		return storage.has( entity );
	}
};

// Scene management with hierarchy support
export class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	// Prevent copying for now (can be implemented later if needed)
	Scene( const Scene & ) = delete;
	Scene &operator=( const Scene & ) = delete;

	Entity createEntity( const std::string &name = "Entity" )
	{
		const Entity entity = m_entityManager.create();

		// All entities have a name by default (we'll need to create the Name component later)
		// For now, just track entity creation

		return entity;
	}

	bool destroyEntity( Entity entity )
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return false;
		}

		// Remove from hierarchy
		removeParent( entity );

		// Remove all children (recursively)
		const auto childrenIt = m_childrenMap.find( entity );
		if ( childrenIt != m_childrenMap.end() )
		{
			// Make a copy since destroyEntity modifies the children map
			const auto children = childrenIt->second;
			for ( Entity child : children )
			{
				destroyEntity( child );
			}
		}

		// Remove all components
		for ( auto &[typeIndex, storage] : m_componentStorages )
		{
			storage->removeComponent( entity );
		}

		// Destroy the entity
		return m_entityManager.destroy( entity );
	}

	bool isValid( Entity entity ) const
	{
		return m_entityManager.isValid( entity );
	}

	template <Component C>
	bool addComponent( Entity entity, C component )
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return false;
		}

		const auto typeIndex = std::type_index( typeid( C ) );

		// Get or create storage for this component type
		if ( m_componentStorages.find( typeIndex ) == m_componentStorages.end() )
		{
			m_componentStorages[typeIndex] = std::make_unique<TypedComponentStorage<C>>();
		}

		auto *typedStorage = static_cast<TypedComponentStorage<C> *>( m_componentStorages[typeIndex].get() );
		return typedStorage->storage.add( entity, component );
	}

	template <Component C>
	bool removeComponent( Entity entity )
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return false;
		}

		const auto typeIndex = std::type_index( typeid( C ) );
		const auto it = m_componentStorages.find( typeIndex );

		if ( it != m_componentStorages.end() )
		{
			auto *typedStorage = static_cast<TypedComponentStorage<C> *>( it->second.get() );
			return typedStorage->storage.remove( entity );
		}

		return false;
	}

	template <Component C>
	C *getComponent( Entity entity )
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return nullptr;
		}

		const auto typeIndex = std::type_index( typeid( C ) );
		const auto it = m_componentStorages.find( typeIndex );

		if ( it != m_componentStorages.end() )
		{
			auto *typedStorage = static_cast<TypedComponentStorage<C> *>( it->second.get() );
			return typedStorage->storage.get( entity );
		}

		return nullptr;
	}

	template <Component C>
	const C *getComponent( Entity entity ) const
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return nullptr;
		}

		const auto typeIndex = std::type_index( typeid( C ) );
		const auto it = m_componentStorages.find( typeIndex );

		if ( it != m_componentStorages.end() )
		{
			auto *typedStorage = static_cast<const TypedComponentStorage<C> *>( it->second.get() );
			return typedStorage->storage.get( entity );
		}

		return nullptr;
	}

	template <Component C>
	bool hasComponent( Entity entity ) const
	{
		if ( !m_entityManager.isValid( entity ) )
		{
			return false;
		}

		const auto typeIndex = std::type_index( typeid( C ) );
		const auto it = m_componentStorages.find( typeIndex );

		if ( it != m_componentStorages.end() )
		{
			return it->second->hasComponent( entity );
		}

		return false;
	}

	// Hierarchy management
	void setParent( Entity child, Entity parent )
	{
		if ( !m_entityManager.isValid( child ) || !m_entityManager.isValid( parent ) )
		{
			return;
		}

		// Remove from old parent first
		removeParent( child );

		// Set new parent
		m_parentMap[child] = parent;
		m_childrenMap[parent].push_back( child );
	}

	void removeParent( Entity child )
	{
		const auto parentIt = m_parentMap.find( child );
		if ( parentIt != m_parentMap.end() )
		{
			const Entity parent = parentIt->second;

			// Remove from parent's children list
			auto &children = m_childrenMap[parent];
			children.erase( std::remove( children.begin(), children.end(), child ), children.end() );

			// Remove from parent map
			m_parentMap.erase( parentIt );
		}
	}

	Entity getParent( Entity child ) const
	{
		const auto it = m_parentMap.find( child );
		return it != m_parentMap.end() ? it->second : Entity{};
	}

	std::vector<Entity> getChildren( Entity parent ) const
	{
		const auto it = m_childrenMap.find( parent );
		return it != m_childrenMap.end() ? it->second : std::vector<Entity>{};
	}

	// Get all entities
	std::span<const Entity> getAllEntities() const
	{
		return m_entityManager.getAllEntities();
	}

	// Component iteration support
	template <Component C>
	ComponentStorage<C> *getComponentStorage()
	{
		const auto typeIndex = std::type_index( typeid( C ) );
		const auto it = m_componentStorages.find( typeIndex );

		if ( it != m_componentStorages.end() )
		{
			auto *typedStorage = static_cast<TypedComponentStorage<C> *>( it->second.get() );
			return &typedStorage->storage;
		}

		return nullptr;
	}

private:
	EntityManager m_entityManager;
	std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> m_componentStorages;

	// Hierarchy maps
	std::unordered_map<Entity, Entity> m_parentMap;
	std::unordered_map<Entity, std::vector<Entity>> m_childrenMap;
};

// Legacy Storage template for backward compatibility with existing tests
export template <Component C>
struct Storage
{
	ComponentStorage<C> storage;

	// Backward compatibility methods
	[[nodiscard]] Entity create( C c = {} )
	{
		// This is a simplified version for backward compatibility
		// In the new system, entities are created through Scene
		static std::uint32_t nextId = 1;
		Entity entity{ nextId++, 0 };
		storage.add( entity, c );
		return entity;
	}

	[[nodiscard]] bool has( Entity e ) const
	{
		return storage.has( e );
	}

	C &get( Entity e )
	{
		C *ptr = storage.get( e );
		if ( !ptr )
		{
			throw std::runtime_error( "Entity does not have component" );
		}
		return *ptr;
	}

	// Expose the internal vectors for backward compatibility (empty for now)
	std::vector<C> dense;			   // Not used in new implementation but kept for tests
	std::vector<std::uint32_t> sparse; // Not used in new implementation but kept for tests
};

} // namespace ecs

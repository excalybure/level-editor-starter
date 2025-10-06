#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <span>
#include <typeindex>
#include <string>
#include <functional>
#include "entity.h"
#include "components.h"

namespace ecs
{

// Developer note:
// - This ECS implementation is intentionally single-threaded. All public
//   APIs (create/destroy/add/remove/get/forEach/modifyComponent) assume
//   callers execute from a single thread. No internal synchronization is
//   performed. To extend for multi-threaded use, introduce atomic state or
//   external locking around `Scene` and `ComponentStorage` operations, and
//   consider read-write locks for concurrent read-heavy workloads.
// - Future work (TODOs):
//   * multi-component queries (forEach with multiple component types)
//   * predicate filtering helpers (forEachWhere)
//   * parallel iteration support for performance-critical systems


// Entity manager with recycling and generation tracking
class EntityManager
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

// Enhanced ComponentStorage with better performance
template <components::Component C>
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
class Scene;

// Base class for type-erased component storage
class ComponentStorageBase
{
public:
	virtual ~ComponentStorageBase() = default;
	virtual bool removeComponent( Entity entity ) = 0;
	virtual bool hasComponent( Entity entity ) const = 0;
};

// Type-erased wrapper for ComponentStorage
template <components::Component C>
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
class Scene
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

		// Auto-add Name component if a custom name is provided
		if ( !name.empty() && name != "Entity" )
		{
			addComponent( entity, components::Name{ name } );
		}

		// Auto-add Visible component with default visible state
		addComponent( entity, components::Visible{ true, true, true } );

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

	template <components::Component C>
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
		const bool result = typedStorage->storage.add( entity, component );

		// Special handling for Transform component addition
		if ( result && std::is_same_v<C, components::Transform> )
		{
			// Notify any registered Transform addition callbacks
			for ( auto &callback : m_transformAdditionCallbacks )
			{
				callback( entity );
			}
		}

		return result;
	}

	template <components::Component C>
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
			const bool result = typedStorage->storage.remove( entity );

			// Special handling for Transform component removal
			if ( result && std::is_same_v<C, components::Transform> )
			{
				// Notify any registered cache invalidation callbacks
				for ( auto &callback : m_transformRemovalCallbacks )
				{
					callback( entity );
				}
			}

			return result;
		}

		return false;
	}

	template <components::Component C>
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

	template <components::Component C>
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

	template <components::Component C>
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

		// Prevent self-parenting
		if ( child == parent )
		{
			return;
		}

		// Prevent parenting to a descendant (would create cycle)
		if ( isAncestor( child, parent ) )
		{
			return;
		}

		// Preserve world position by adjusting local transform
		// Get child's current world transform
		math::Mat4f childWorldTransform;
		if ( hasComponent<components::Transform>( child ) )
		{
			childWorldTransform = computeWorldTransform( child );
		}

		// Remove from old parent first
		removeParent( child );

		// Set new parent
		m_parentMap[child] = parent;
		m_childrenMap[parent].push_back( child );

		// Adjust child's local transform to preserve world position
		if ( hasComponent<components::Transform>( child ) && hasComponent<components::Transform>( parent ) )
		{
			const math::Mat4f parentWorldTransform = computeWorldTransform( parent );
			// The inverse() function returns the transpose of the actual inverse, so we transpose it back
			const math::Mat4f parentWorldInverse = parentWorldTransform.inverse().transpose();
			const math::Mat4f newLocalTransform = parentWorldInverse * childWorldTransform;

			// Extract position, rotation, and scale from the new local transform
			auto *childTransform = getComponent<components::Transform>( child );
			if ( childTransform )
			{
				// Extract translation from the right column (m03, m13, m23)
				childTransform->position.x = newLocalTransform.m03();
				childTransform->position.y = newLocalTransform.m13();
				childTransform->position.z = newLocalTransform.m23();

				// For now, keep rotation and scale unchanged (simplified approach)
				// A full implementation would extract rotation and scale from the matrix
				// TODO: Implement full matrix decomposition if rotation/scale preservation is needed

				childTransform->markDirty();
			}
		}

		// Notify Transform modification callbacks for hierarchy change
		for ( auto &callback : m_transformModificationCallbacks )
		{
			callback( child );
			callback( parent );
		}
	}

	Entity findEntityByName( const std::string &targetName )
	{
		Entity result{};

		auto *storage = getComponentStorage<components::Name>();
		if ( !storage )
		{
			return {};
		}

		for ( auto &[entity, component] : *storage )
		{
			// Only process valid entities
			if ( component.name == targetName )
			{
				return entity; // Stop iteration when found
			}
		}

		return Entity{};
	}

private:
	// Helper to compute world transform by traversing hierarchy
	math::Mat4f computeWorldTransform( Entity entity ) const
	{
		if ( !hasComponent<components::Transform>( entity ) )
		{
			return math::Mat4f::identity();
		}

		const auto *transform = getComponent<components::Transform>( entity );
		math::Mat4f localMatrix = transform->getLocalMatrix();

		// Traverse up the hierarchy to accumulate transforms
		const Entity parent = getParent( entity );
		if ( parent.isValid() && hasComponent<components::Transform>( parent ) )
		{
			const math::Mat4f parentWorld = computeWorldTransform( parent );
			return parentWorld * localMatrix;
		}

		return localMatrix;
	}

	// Helper to check if 'ancestor' is an ancestor of 'descendant'
	bool isAncestor( Entity ancestor, Entity descendant ) const
	{
		Entity current = descendant;
		while ( current.isValid() )
		{
			Entity currentParent = getParent( current );
			if ( !currentParent.isValid() )
			{
				break;
			}
			if ( currentParent == ancestor )
			{
				return true;
			}
			current = currentParent;
		}
		return false;
	}

public:
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

	// Get entity count (only counts valid entities)
	size_t getEntityCount() const
	{
		size_t count = 0;
		for ( const auto &entity : m_entityManager.getAllEntities() )
		{
			if ( m_entityManager.isValid( entity ) )
			{
				++count;
			}
		}
		return count;
	}

	// Component iteration support
	template <components::Component C>
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

	// Query / Iteration Utility: forEach for single component type
	// Iterates over all entities that have the specified component type
	// Usage: scene.forEach<Transform>([](Entity e, Transform& t) { ... });
	//
	// This method provides a clean and efficient way to iterate over all components
	// of a specific type without manual storage access. Only valid entities are
	// processed during iteration.
	//
	// Future Extensions:
	// - Multi-component query support: forEach<Transform, Visible>(...)
	// - Filtering predicates: forEachWhere<Transform>([](Entity e) { return ...; }, ...)
	// - Parallel iteration for performance-critical systems
	template <components::Component C, typename Functor>
	void forEach( Functor functor )
	{
		ComponentStorage<C> *storage = getComponentStorage<C>();
		if ( !storage )
		{
			return; // No storage means no components of this type
		}

		// Iterate over all entity-component pairs in the storage
		for ( auto &[entity, component] : *storage )
		{
			// Only process valid entities
			if ( m_entityManager.isValid( entity ) )
			{
				functor( entity, component );
			}
		}
	}

	// Component modification helper with automatic dirty marking
	template <components::Component C, typename Functor>
	bool modifyComponent( Entity entity, Functor functor )
	{
		C *component = getComponent<C>( entity );
		if ( !component )
		{
			return false;
		}

		functor( *component );

		// Optionally call markDirty if the component has this method
		if constexpr ( requires { component->markDirty(); } )
		{
			component->markDirty();
		}

		return true;
	}

	// Register callbacks for Transform component lifecycle (for cache invalidation)
	void registerTransformRemovalCallback( std::function<void( Entity )> callback )
	{
		m_transformRemovalCallbacks.push_back( std::move( callback ) );
	}

	void registerTransformAdditionCallback( std::function<void( Entity )> callback )
	{
		m_transformAdditionCallbacks.push_back( std::move( callback ) );
	}

	void registerTransformModificationCallback( std::function<void( Entity )> callback )
	{
		m_transformModificationCallbacks.push_back( std::move( callback ) );
	}

private:
	EntityManager m_entityManager;
	std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> m_componentStorages;

	// Hierarchy maps
	std::unordered_map<Entity, Entity> m_parentMap;
	std::unordered_map<Entity, std::vector<Entity>> m_childrenMap;

	// Transform lifecycle callbacks for cache invalidation
	std::vector<std::function<void( Entity )>> m_transformRemovalCallbacks;
	std::vector<std::function<void( Entity )>> m_transformAdditionCallbacks;
	std::vector<std::function<void( Entity )>> m_transformModificationCallbacks;
};

// Legacy Storage template for backward compatibility with existing tests
template <components::Component C>
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

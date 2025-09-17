export module runtime.systems;

import runtime.ecs;
import runtime.components;
import runtime.entity;
import engine.matrix;
import engine.vec;
import <memory>;
import <vector>;
import <unordered_map>;
import <unordered_set>;
import <typeinfo>;

export namespace systems
{

// Base system interface
class System
{
public:
	virtual ~System() = default;
	virtual void update( ecs::Scene &scene, float deltaTime ) = 0;
	virtual void initialize( ecs::Scene &scene ) {}
	virtual void shutdown( ecs::Scene &scene ) {}
};

// Transform hierarchy system
class TransformSystem : public System
{
public:
	void initialize( ecs::Scene &scene ) override
	{
		m_scene = &scene; // store scene pointer for hierarchy traversal
		// Clear any existing cached data
		m_dirtyTransforms.clear();
		m_worldMatrices.clear();

		// Register for Transform component removal notifications
		scene.registerTransformRemovalCallback(
			[this]( ecs::Entity entity ) {
				this->onTransformRemoved( entity );
			} );

		// Mark all entities with a Transform component as dirty
		for ( const auto entity : scene.getAllEntities() )
		{
			if ( scene.hasComponent<components::Transform>( entity ) )
			{
				markDirty( entity );
			}
		}
	}

	void update( ecs::Scene &scene, float deltaTime ) override
	{
		// Update world matrices for all dirty transforms
		while ( !m_dirtyTransforms.empty() )
		{
			auto it = m_dirtyTransforms.begin();
			ecs::Entity entity = *it;
			m_dirtyTransforms.erase( it );

			updateWorldMatrix( scene, entity );
		}
	}

	// Get world transform for entity
	math::Mat4<> getWorldTransform( ecs::Scene &scene, ecs::Entity entity )
	{
		// Check if entity has a Transform component first
		if ( !scene.hasComponent<components::Transform>( entity ) )
		{
			return math::Mat4<>::identity();
		}

		auto it = m_worldMatrices.find( entity );
		if ( it != m_worldMatrices.end() )
		{
			return it->second;
		}

		// Calculate and cache if not found
		updateWorldMatrix( scene, entity );

		// Check again if matrix was successfully created
		auto it2 = m_worldMatrices.find( entity );
		if ( it2 != m_worldMatrices.end() )
		{
			return it2->second;
		}

		// Fallback to identity if something went wrong
		return math::Mat4<>::identity();
	}

	// Mark transforms as dirty when changed
	void markDirty( ecs::Entity entity )
	{
		m_dirtyTransforms.insert( entity );

		// Also mark all children as dirty (recursive)
		markChildrenDirty( entity );
	}

private:
	std::unordered_set<ecs::Entity> m_dirtyTransforms;
	std::unordered_map<ecs::Entity, math::Mat4<>> m_worldMatrices;

	void onTransformRemoved( ecs::Entity entity )
	{
		// Remove from dirty set if present
		m_dirtyTransforms.erase( entity );

		// Remove cached world matrix
		m_worldMatrices.erase( entity );
	}

	void updateWorldMatrix( ecs::Scene &scene, ecs::Entity entity )
	{
		auto *transform = scene.getComponent<components::Transform>( entity );
		if ( !transform )
		{
			return;
		}

		// Get parent's world matrix
		ecs::Entity parent = scene.getParent( entity );
		math::Mat4<> parentMatrix = math::Mat4<>::identity();

		if ( parent.id != 0 )
		{
			// Recursively ensure parent matrix is up to date
			if ( m_dirtyTransforms.find( parent ) != m_dirtyTransforms.end() )
			{
				updateWorldMatrix( scene, parent );
			}

			auto parentIt = m_worldMatrices.find( parent );
			if ( parentIt != m_worldMatrices.end() )
			{
				parentMatrix = parentIt->second;
			}
		}

		// Calculate world matrix
		math::Mat4<> localMatrix = transform->getLocalMatrix();
		math::Mat4<> worldMatrix = parentMatrix * localMatrix;

		m_worldMatrices[entity] = worldMatrix;
	}

	void markChildrenDirty( ecs::Entity entity )
	{
		if ( m_scene == nullptr )
		{
			return;
		}

		// Depth-first traversal using an explicit stack
		std::vector<ecs::Entity> stack;
		const auto rootChildren = m_scene->getChildren( entity );
		stack.insert( stack.end(), rootChildren.begin(), rootChildren.end() );

		std::unordered_set<ecs::Entity> visited;
		while ( !stack.empty() )
		{
			auto current = stack.back();
			stack.pop_back();
			if ( visited.contains( current ) )
			{
				continue;
			}
			visited.insert( current );
			m_dirtyTransforms.insert( current );
			const auto children = m_scene->getChildren( current );
			stack.insert( stack.end(), children.begin(), children.end() );
		}
	}

private:
	ecs::Scene *m_scene = nullptr;
};

// System manager
class SystemManager
{
public:
	template <typename T, typename... Args>
	T *addSystem( Args &&...args )
	{
		static_assert( std::is_base_of_v<System, T>, "T must derive from System" );

		auto system = std::make_unique<T>( std::forward<Args>( args )... );
		T *ptr = system.get();
		m_systems.push_back( std::move( system ) );
		return ptr;
	}

	template <typename T>
	T *getSystem()
	{
		static_assert( std::is_base_of_v<System, T>, "T must derive from System" );

		for ( auto &system : m_systems )
		{
			if ( auto *ptr = dynamic_cast<T *>( system.get() ) )
			{
				return ptr;
			}
		}
		return nullptr;
	}

	void initialize( ecs::Scene &scene )
	{
		for ( auto &system : m_systems )
		{
			system->initialize( scene );
		}
	}

	void update( ecs::Scene &scene, float deltaTime )
	{
		for ( auto &system : m_systems )
		{
			system->update( scene, deltaTime );
		}
	}

	void shutdown( ecs::Scene &scene )
	{
		for ( auto &system : m_systems )
		{
			system->shutdown( scene );
		}
	}

	void clear()
	{
		m_systems.clear();
	}

private:
	std::vector<std::unique_ptr<System>> m_systems;
};

} // namespace systems

#pragma once

#include "editor/commands/Command.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include <string>
#include <memory>
#include <optional>
#include <typeinfo>

namespace editor
{

/**
 * @brief Command for creating new entities in the scene
 * 
 * Creates an entity with the specified name on execute, and removes it on undo.
 * Handles proper entity lifecycle and cleanup.
 */
class CreateEntityCommand : public Command
{
public:
	CreateEntityCommand( ecs::Scene &scene, const std::string &name = "Entity" );

	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;
	ecs::Entity getRecreatedEntity() const override;
	ecs::Entity getOriginalEntity() const { return m_originalEntity; }

	ecs::Entity getCreatedEntity() const { return m_entity; }

private:
	ecs::Scene &m_scene;
	std::string m_name;
	ecs::Entity m_entity{};
	ecs::Entity m_originalEntity{}; // Store the original entity for reference fixup
	bool m_executed = false;
};

/**
 * @brief Command for deleting entities from the scene with state restoration
 * 
 * Captures all entity state (components and hierarchy) before deletion,
 * allowing complete restoration on undo.
 */
class DeleteEntityCommand : public Command
{
public:
	DeleteEntityCommand( ecs::Scene &scene, ecs::Entity entity );

	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;
	ecs::Entity getRecreatedEntity() const override;
	ecs::Entity getOriginalEntity() const { return m_originalEntity; }

private:
	ecs::Scene &m_scene;
	ecs::Entity m_entity;
	ecs::Entity m_originalEntity; // Store the original entity for reference fixup
	std::string m_entityName;
	bool m_executed = false;

	// Component state capture for restoration
	std::optional<components::Transform> m_transform;
	std::optional<components::Visible> m_visible;
	std::optional<components::MeshRenderer> m_meshRenderer;
	std::optional<components::Selected> m_selected;

	void captureEntityState();
	void restoreEntityState();
};

/**
 * @brief Generic template command for adding components to entities
 * 
 * Template-based command that can add any component type to an entity.
 * Supports undo by removing the component.
 */
template <components::Component T>
class AddComponentCommand : public Command
{
public:
	AddComponentCommand( ecs::Scene &scene, ecs::Entity entity, const T &component )
		: m_scene( scene ), m_entity( entity ), m_component( component ), m_executed( false ) {}

	bool execute() override
	{
		if ( m_executed || !m_scene.isValid( m_entity ) )
			return false;

		const bool added = m_scene.addComponent( m_entity, m_component );
		if ( added )
		{
			m_executed = true;
		}
		return added;
	}

	bool undo() override
	{
		if ( !m_executed )
			return false;

		const bool removed = m_scene.removeComponent<T>( m_entity );
		if ( removed )
		{
			m_executed = false;
		}
		return removed;
	}

	std::string getDescription() const override
	{
		// Get component type name (simplified for now)
		std::string typeName = typeid( T ).name();
		// Remove class/struct prefixes and namespace if present
		size_t lastSpace = typeName.find_last_of( ' ' );
		if ( lastSpace != std::string::npos )
			typeName = typeName.substr( lastSpace + 1 );
		size_t lastColon = typeName.find_last_of( ':' );
		if ( lastColon != std::string::npos )
			typeName = typeName.substr( lastColon + 1 );

		return "Add " + typeName + " Component";
	}

	size_t getMemoryUsage() const override
	{
		return sizeof( *this );
	}

	bool canMergeWith( const Command * /* other */ ) const override
	{
		// Component commands typically cannot be merged
		return false;
	}

	bool mergeWith( std::unique_ptr<Command> /* other */ ) override
	{
		// Component commands typically cannot be merged
		return false;
	}

	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override
	{
		return editor::updateEntityReference( m_entity, oldEntity, newEntity );
	}

private:
	ecs::Scene &m_scene;
	ecs::Entity m_entity;
	T m_component;
	bool m_executed;
};

/**
 * @brief Generic template command for removing components from entities
 * 
 * Template-based command that captures component state before removal
 * to enable proper restoration on undo.
 */
template <components::Component T>
class RemoveComponentCommand : public Command
{
public:
	RemoveComponentCommand( ecs::Scene &scene, ecs::Entity entity )
		: m_scene( scene ), m_entity( entity ), m_executed( false )
	{
		// Capture component state if it exists
		if ( m_scene.hasComponent<T>( entity ) )
		{
			m_component = *m_scene.getComponent<T>( entity );
			m_hadComponent = true;
		}
		else
		{
			m_hadComponent = false;
		}
	}

	bool execute() override
	{
		if ( m_executed || !m_scene.isValid( m_entity ) || !m_hadComponent )
			return false;

		const bool removed = m_scene.removeComponent<T>( m_entity );
		if ( removed )
		{
			m_executed = true;
		}
		return removed;
	}

	bool undo() override
	{
		if ( !m_executed || !m_hadComponent )
			return false;

		const bool added = m_scene.addComponent( m_entity, m_component );
		if ( added )
		{
			m_executed = false;
		}
		return added;
	}

	std::string getDescription() const override
	{
		// Get component type name (simplified for now)
		std::string typeName = typeid( T ).name();
		// Remove class/struct prefixes and namespace if present
		const size_t lastSpace = typeName.find_last_of( ' ' );
		if ( lastSpace != std::string::npos )
			typeName = typeName.substr( lastSpace + 1 );
		const size_t lastColon = typeName.find_last_of( ':' );
		if ( lastColon != std::string::npos )
			typeName = typeName.substr( lastColon + 1 );

		return "Remove " + typeName + " Component";
	}

	size_t getMemoryUsage() const override
	{
		return sizeof( *this );
	}

	bool canMergeWith( const Command * /* other */ ) const override
	{
		// Component commands typically cannot be merged
		return false;
	}

	bool mergeWith( std::unique_ptr<Command> /* other */ ) override
	{
		// Component commands typically cannot be merged
		return false;
	}

	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override
	{
		return editor::updateEntityReference( m_entity, oldEntity, newEntity );
	}

private:
	ecs::Scene &m_scene;
	ecs::Entity m_entity;
	T m_component{};
	bool m_hadComponent = false;
	bool m_executed;
};

/**
 * @brief Command for setting parent-child hierarchy relationships
 * 
 * Changes the parent of an entity while preserving the previous parent
 * to enable proper undo functionality.
 */
class SetParentCommand : public Command
{
public:
	SetParentCommand( ecs::Scene &scene, ecs::Entity child, ecs::Entity newParent );

	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Scene &m_scene;
	ecs::Entity m_child;
	ecs::Entity m_newParent;
	ecs::Entity m_oldParent;
	std::string m_childName;
	std::string m_newParentName;
	bool m_executed = false;
	bool m_hadOldParent = false;

	void captureNames();
};

/**
 * @brief Command for renaming entities
 * 
 * Changes the Name component of an entity while preserving the old name
 * for undo functionality.
 */
class RenameEntityCommand : public Command
{
public:
	RenameEntityCommand( ecs::Scene &scene, ecs::Entity entity, const std::string &newName );

	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Scene &m_scene;
	ecs::Entity m_entity;
	std::string m_newName;
	std::string m_oldName;
	bool m_executed = false;
	bool m_hadNameComponent = false;

	void captureOldName();
};

/**
 * @brief Factory for creating ECS command instances
 * 
 * Provides convenient static methods for creating all types of ECS commands
 * with proper type deduction and error checking.
 */
class EcsCommandFactory
{
public:
	// Entity commands
	static std::unique_ptr<CreateEntityCommand> createEntity( ecs::Scene &scene, const std::string &name = "Entity" );
	static std::unique_ptr<DeleteEntityCommand> deleteEntity( ecs::Scene &scene, ecs::Entity entity );

	// Component commands
	template <components::Component T>
	static std::unique_ptr<AddComponentCommand<T>> addComponent( ecs::Scene &scene, ecs::Entity entity, const T &component )
	{
		return std::make_unique<AddComponentCommand<T>>( scene, entity, component );
	}

	template <components::Component T>
	static std::unique_ptr<RemoveComponentCommand<T>> removeComponent( ecs::Scene &scene, ecs::Entity entity )
	{
		return std::make_unique<RemoveComponentCommand<T>>( scene, entity );
	}

	// Hierarchy commands
	static std::unique_ptr<SetParentCommand> setParent( ecs::Scene &scene, ecs::Entity child, ecs::Entity newParent );

	// Naming commands
	static std::unique_ptr<RenameEntityCommand> renameEntity( ecs::Scene &scene, ecs::Entity entity, const std::string &newName );
};

} // namespace editor
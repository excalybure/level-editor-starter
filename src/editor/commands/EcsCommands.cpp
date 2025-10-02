#include "editor/commands/EcsCommands.h"

namespace editor
{

CreateEntityCommand::CreateEntityCommand( ecs::Scene &scene, const std::string &name )
	: m_scene( scene ), m_name( name ), m_entity{}, m_executed( false )
{
}

bool CreateEntityCommand::execute()
{
	if ( m_executed )
		return false;

	m_entity = m_scene.createEntity( m_name );

	// Store original entity on first execution for reference fixup
	if ( !m_originalEntity.isValid() )
	{
		m_originalEntity = m_entity;
	}

	m_executed = true;
	return m_entity.isValid();
}

bool CreateEntityCommand::undo()
{
	if ( !m_executed || !m_entity.isValid() )
		return false;

	const bool destroyed = m_scene.destroyEntity( m_entity );
	if ( destroyed )
	{
		m_entity = ecs::Entity{}; // Reset to invalid
		m_executed = false;
	}
	return destroyed;
}

std::string CreateEntityCommand::getDescription() const
{
	return "Create Entity: " + m_name;
}

size_t CreateEntityCommand::getMemoryUsage() const
{
	return sizeof( *this ) + m_name.size();
}

bool CreateEntityCommand::canMergeWith( const Command * /* other */ ) const
{
	// Entity creation commands cannot be merged
	return false;
}

bool CreateEntityCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	// Entity creation commands cannot be merged
	return false;
}

bool CreateEntityCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

ecs::Entity CreateEntityCommand::getRecreatedEntity() const
{
	// Return the recreated entity (m_entity) if this command has been executed and m_entity differs from m_originalEntity
	// This indicates the entity was created, then undone (deleted), then redone (recreated with new generation)
	if ( m_executed && m_entity.id == m_originalEntity.id && m_entity.generation != m_originalEntity.generation )
	{
		return m_entity;
	}
	return ecs::Entity{ 0, 0 }; // Invalid entity if not recreated
}

DeleteEntityCommand::DeleteEntityCommand( ecs::Scene &scene, ecs::Entity entity )
	: m_scene( scene ), m_entity( entity ), m_originalEntity( entity ), m_executed( false )
{
	// Capture the entity name if it has one
	if ( scene.hasComponent<components::Name>( entity ) )
	{
		const auto *nameComp = scene.getComponent<components::Name>( entity );
		m_entityName = nameComp->name;
	}
	else
	{
		m_entityName = "Entity";
	}
}

bool DeleteEntityCommand::execute()
{
	if ( m_executed || !m_scene.isValid( m_entity ) )
		return false;

	// Capture all component state before deletion
	captureEntityState();

	// Delete the entity
	const bool destroyed = m_scene.destroyEntity( m_entity );
	if ( destroyed )
	{
		m_executed = true;
	}
	return destroyed;
}

bool DeleteEntityCommand::undo()
{
	if ( !m_executed )
		return false;

	// Recreate the entity (it will get a new ID)
	m_entity = m_scene.createEntity( m_entityName );
	if ( !m_entity.isValid() )
		return false;

	// Restore all captured component state
	restoreEntityState();

	m_executed = false;
	return true;
}

std::string DeleteEntityCommand::getDescription() const
{
	return "Delete Entity: " + m_entityName;
}

size_t DeleteEntityCommand::getMemoryUsage() const
{
	return sizeof( *this ) + m_entityName.size();
}

bool DeleteEntityCommand::canMergeWith( const Command * /* other */ ) const
{
	// Entity deletion commands cannot be merged
	return false;
}

bool DeleteEntityCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	// Entity deletion commands cannot be merged
	return false;
}

bool DeleteEntityCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

ecs::Entity DeleteEntityCommand::getRecreatedEntity() const
{
	// Return the recreated entity (m_entity) if this command has been executed and then undone
	// Only return it if it differs from the original entity (meaning it was recreated)
	if ( !m_executed && m_entity.id == m_originalEntity.id && m_entity.generation != m_originalEntity.generation )
	{
		return m_entity;
	}
	return ecs::Entity{ 0, 0 }; // Invalid entity if not recreated
}

void DeleteEntityCommand::captureEntityState()
{
	// Capture all possible components
	if ( m_scene.hasComponent<components::Transform>( m_entity ) )
	{
		m_transform = *m_scene.getComponent<components::Transform>( m_entity );
	}

	if ( m_scene.hasComponent<components::Visible>( m_entity ) )
	{
		m_visible = *m_scene.getComponent<components::Visible>( m_entity );
	}

	if ( m_scene.hasComponent<components::MeshRenderer>( m_entity ) )
	{
		m_meshRenderer = *m_scene.getComponent<components::MeshRenderer>( m_entity );
	}

	if ( m_scene.hasComponent<components::Selected>( m_entity ) )
	{
		m_selected = *m_scene.getComponent<components::Selected>( m_entity );
	}
}

void DeleteEntityCommand::restoreEntityState()
{
	// Restore all captured components
	if ( m_transform.has_value() )
	{
		m_scene.addComponent( m_entity, m_transform.value() );
	}

	if ( m_visible.has_value() )
	{
		m_scene.addComponent( m_entity, m_visible.value() );
	}

	if ( m_meshRenderer.has_value() )
	{
		m_scene.addComponent( m_entity, m_meshRenderer.value() );
	}

	if ( m_selected.has_value() )
	{
		m_scene.addComponent( m_entity, m_selected.value() );
	}
}

SetParentCommand::SetParentCommand( ecs::Scene &scene, ecs::Entity child, ecs::Entity newParent )
	: m_scene( scene ), m_child( child ), m_newParent( newParent ), m_executed( false )
{
	// Capture old parent if exists
	m_oldParent = m_scene.getParent( child );
	m_hadOldParent = m_oldParent.isValid();

	captureNames();
}

bool SetParentCommand::execute()
{
	if ( m_executed || !m_scene.isValid( m_child ) || !m_scene.isValid( m_newParent ) )
		return false;

	// Prevent self-parenting
	if ( m_child.id == m_newParent.id )
		return false;

	// Prevent circular references (check if newParent is a descendant of child)
	ecs::Entity current = m_newParent;
	while ( m_scene.isValid( current ) )
	{
		const ecs::Entity parent = m_scene.getParent( current );
		if ( !m_scene.isValid( parent ) )
			break; // Reached root
		if ( parent.id == m_child.id )
			return false; // Would create circular reference
		current = parent;
	}

	m_scene.setParent( m_child, m_newParent );
	m_executed = true;
	return true;
}

bool SetParentCommand::undo()
{
	if ( !m_executed )
		return false;

	if ( m_hadOldParent )
	{
		m_scene.setParent( m_child, m_oldParent );
	}
	else
	{
		m_scene.removeParent( m_child );
	}

	m_executed = false;
	return true;
}

std::string SetParentCommand::getDescription() const
{
	return "Set Parent: " + m_childName + " -> " + m_newParentName;
}

size_t SetParentCommand::getMemoryUsage() const
{
	return sizeof( *this ) + m_childName.size() + m_newParentName.size();
}

bool SetParentCommand::canMergeWith( const Command * /* other */ ) const
{
	// Hierarchy commands typically cannot be merged
	return false;
}

bool SetParentCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	// Hierarchy commands typically cannot be merged
	return false;
}

bool SetParentCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	bool updated = false;
	updated |= editor::updateEntityReference( m_child, oldEntity, newEntity );
	updated |= editor::updateEntityReference( m_newParent, oldEntity, newEntity );
	updated |= editor::updateEntityReference( m_oldParent, oldEntity, newEntity );
	return updated;
}

void SetParentCommand::captureNames()
{
	// Capture child name
	if ( m_scene.hasComponent<components::Name>( m_child ) )
	{
		m_childName = m_scene.getComponent<components::Name>( m_child )->name;
	}
	else
	{
		m_childName = "Entity";
	}

	// Capture new parent name
	if ( m_scene.hasComponent<components::Name>( m_newParent ) )
	{
		m_newParentName = m_scene.getComponent<components::Name>( m_newParent )->name;
	}
	else
	{
		m_newParentName = "Entity";
	}
}

RenameEntityCommand::RenameEntityCommand( ecs::Scene &scene, ecs::Entity entity, const std::string &newName )
	: m_scene( scene ), m_entity( entity ), m_newName( newName ), m_executed( false )
{
	captureOldName();
}

bool RenameEntityCommand::execute()
{
	if ( m_executed || !m_scene.isValid( m_entity ) )
		return false;

	if ( m_hadNameComponent )
	{
		// Update existing name component
		auto *nameComp = m_scene.getComponent<components::Name>( m_entity );
		nameComp->name = m_newName;
	}
	else
	{
		// Add new name component
		m_scene.addComponent( m_entity, components::Name{ m_newName } );
	}

	m_executed = true;
	return true;
}

bool RenameEntityCommand::undo()
{
	if ( !m_executed )
		return false;

	if ( m_hadNameComponent )
	{
		// Restore old name
		auto *nameComp = m_scene.getComponent<components::Name>( m_entity );
		nameComp->name = m_oldName;
	}
	else
	{
		// Remove name component that was added
		m_scene.removeComponent<components::Name>( m_entity );
	}

	m_executed = false;
	return true;
}

std::string RenameEntityCommand::getDescription() const
{
	return "Rename Entity: " + m_oldName + " -> " + m_newName;
}

size_t RenameEntityCommand::getMemoryUsage() const
{
	return sizeof( *this ) + m_oldName.size() + m_newName.size();
}

bool RenameEntityCommand::canMergeWith( const Command * /* other */ ) const
{
	// Rename commands typically cannot be merged
	return false;
}

bool RenameEntityCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	// Rename commands typically cannot be merged
	return false;
}

bool RenameEntityCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

void RenameEntityCommand::captureOldName()
{
	if ( m_scene.hasComponent<components::Name>( m_entity ) )
	{
		m_oldName = m_scene.getComponent<components::Name>( m_entity )->name;
		m_hadNameComponent = true;
	}
	else
	{
		m_oldName = "Entity";
		m_hadNameComponent = false;
	}
}

// EcsCommandFactory implementations
std::unique_ptr<CreateEntityCommand> EcsCommandFactory::createEntity( ecs::Scene &scene, const std::string &name )
{
	return std::make_unique<CreateEntityCommand>( scene, name );
}

std::unique_ptr<DeleteEntityCommand> EcsCommandFactory::deleteEntity( ecs::Scene &scene, ecs::Entity entity )
{
	return std::make_unique<DeleteEntityCommand>( scene, entity );
}

std::unique_ptr<SetParentCommand> EcsCommandFactory::setParent( ecs::Scene &scene, ecs::Entity child, ecs::Entity newParent )
{
	return std::make_unique<SetParentCommand>( scene, child, newParent );
}

std::unique_ptr<RenameEntityCommand> EcsCommandFactory::renameEntity( ecs::Scene &scene, ecs::Entity entity, const std::string &newName )
{
	return std::make_unique<RenameEntityCommand>( scene, entity, newName );
}

} // namespace editor
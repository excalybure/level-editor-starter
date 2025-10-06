#include "editor/commands/EcsCommands.h"
#include "runtime/scene_importer.h"
#include <algorithm>
#include <filesystem>

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
	bool updated = editor::updateEntityReference( m_entity, oldEntity, newEntity );
	updated |= editor::updateEntityReference( m_parent, oldEntity, newEntity );
	return updated;
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

	// Capture parent-child hierarchy relationship
	m_parent = m_scene.getParent( m_entity );
	m_hadParent = m_parent.isValid();
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

	// Restore parent-child hierarchy relationship
	if ( m_hadParent )
	{
		m_scene.setParent( m_entity, m_parent );
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

// ModifyVisibleCommand implementation
ModifyVisibleCommand::ModifyVisibleCommand( ecs::Scene &scene, ecs::Entity entity, const components::Visible &newVisible )
	: m_scene( scene ), m_entity( entity ), m_newVisible( newVisible ), m_executed( false ), m_hadVisibleComponent( false )
{
	captureOldVisible();
}

bool ModifyVisibleCommand::execute()
{
	if ( m_executed || !m_scene.isValid( m_entity ) )
		return false;

	// Ensure the entity has a Visible component
	if ( !m_scene.hasComponent<components::Visible>( m_entity ) )
	{
		// Add Visible component if it doesn't exist
		m_scene.addComponent( m_entity, m_newVisible );
	}
	else
	{
		// Modify existing Visible component
		auto *visible = m_scene.getComponent<components::Visible>( m_entity );
		*visible = m_newVisible;
	}

	m_executed = true;
	return true;
}

bool ModifyVisibleCommand::undo()
{
	if ( !m_executed )
		return false;

	if ( m_hadVisibleComponent )
	{
		// Restore old visible state
		auto *visible = m_scene.getComponent<components::Visible>( m_entity );
		*visible = m_oldVisible;
	}
	else
	{
		// Remove visible component that was added
		m_scene.removeComponent<components::Visible>( m_entity );
	}

	m_executed = false;
	return true;
}

std::string ModifyVisibleCommand::getDescription() const
{
	return "Modify Visibility";
}

size_t ModifyVisibleCommand::getMemoryUsage() const
{
	return sizeof( *this );
}

bool ModifyVisibleCommand::canMergeWith( const Command * /* other */ ) const
{
	// Visible modification commands typically cannot be merged
	return false;
}

bool ModifyVisibleCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	// Visible modification commands typically cannot be merged
	return false;
}

bool ModifyVisibleCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

void ModifyVisibleCommand::captureOldVisible()
{
	if ( m_scene.hasComponent<components::Visible>( m_entity ) )
	{
		m_oldVisible = *m_scene.getComponent<components::Visible>( m_entity );
		m_hadVisibleComponent = true;
	}
	else
	{
		// Default visible state
		m_oldVisible = components::Visible{};
		m_hadVisibleComponent = false;
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

std::unique_ptr<ModifyVisibleCommand> EcsCommandFactory::modifyVisible( ecs::Scene &scene, ecs::Entity entity, const components::Visible &newVisible )
{
	return std::make_unique<ModifyVisibleCommand>( scene, entity, newVisible );
}

// CreateEntityFromAssetCommand implementation
CreateEntityFromAssetCommand::CreateEntityFromAssetCommand( ecs::Scene &scene, assets::AssetManager &assetManager, engine::GPUResourceManager &gpuManager, const std::string &assetPath, const math::Vec3f &worldPosition, ecs::Entity parent )
	: m_scene( scene ), m_assetManager( assetManager ), m_gpuManager( gpuManager ), m_assetPath( assetPath ), m_worldPosition( worldPosition ), m_parent( parent ), m_executed( false )
{
}

bool CreateEntityFromAssetCommand::execute()
{
	if ( m_executed )
		return false;

	// Load asset from file
	auto assetScene = m_assetManager.load<assets::Scene>( m_assetPath );
	if ( !assetScene || !assetScene->isLoaded() )
	{
		return false; // Asset loading failed
	}

	// Capture existing entities before import to identify new ones
	// IMPORTANT: Copy the entities because getAllEntities() returns a span that may be invalidated
	const std::vector<ecs::Entity> existingEntities( m_scene.getAllEntities().begin(), m_scene.getAllEntities().end() );

	// Import scene using SceneImporter
	const bool imported = runtime::SceneImporter::importScene( assetScene, m_scene );
	if ( !imported )
	{
		return false; // Import failed
	}

	// Create GPU resources for the imported meshes
	const bool gpuResourcesCreated = runtime::SceneImporter::createGPUResources( assetScene, m_scene, m_gpuManager );
	if ( !gpuResourcesCreated )
	{
		// Note: We don't fail the command if GPU upload fails, as the entities are still created
		// The meshes just won't be visible until GPU resources are available
	}

	// Find the root entity created by the import by comparing entities before and after
	const auto allEntities = m_scene.getAllEntities();
	if ( allEntities.size() <= existingEntities.size() )
	{
		return false; // No new entities created
	}

	// Build a set of existing entity IDs for faster lookup
	std::vector<std::uint32_t> existingIds;
	existingIds.reserve( existingEntities.size() );
	for ( const auto &entity : existingEntities )
	{
		existingIds.push_back( entity.id );
	}

	// Find newly created root entities (entities without a parent that didn't exist before)
	for ( const auto &entity : allEntities )
	{
		// Check if this entity is new by comparing IDs
		const bool isNewEntity = std::find( existingIds.begin(), existingIds.end(), entity.id ) == existingIds.end();
		if ( !isNewEntity )
			continue;

		// Check if it's a root entity
		const auto parent = m_scene.getParent( entity );
		if ( !parent.isValid() )
		{
			// New root entity found
			m_rootEntity = entity;
			break;
		}
	}

	if ( !m_rootEntity.isValid() )
	{
		return false; // Couldn't find new root entity
	}

	// Set world position on root entity
	if ( m_scene.hasComponent<components::Transform>( m_rootEntity ) )
	{
		auto *transform = m_scene.getComponent<components::Transform>( m_rootEntity );
		transform->position = m_worldPosition;
		transform->localMatrixDirty = true;
	}

	// Set parent if specified
	if ( m_parent.isValid() && m_scene.isValid( m_parent ) )
	{
		m_scene.setParent( m_rootEntity, m_parent );
	}

	// Capture all created entities for undo
	captureCreatedEntities( m_rootEntity );

	m_executed = true;
	return true;
}

bool CreateEntityFromAssetCommand::undo()
{
	if ( !m_executed )
		return false;

	// Destroy all created entities in reverse order (children first)
	for ( auto it = m_createdEntities.rbegin(); it != m_createdEntities.rend(); ++it )
	{
		if ( m_scene.isValid( *it ) )
		{
			m_scene.destroyEntity( *it );
		}
	}

	m_createdEntities.clear();
	m_rootEntity = ecs::Entity{};
	m_executed = false;
	return true;
}

std::string CreateEntityFromAssetCommand::getDescription() const
{
	const std::filesystem::path path( m_assetPath );
	return "Create entity from " + path.filename().string();
}

size_t CreateEntityFromAssetCommand::getMemoryUsage() const
{
	return sizeof( *this ) + m_assetPath.size() + ( m_createdEntities.size() * sizeof( ecs::Entity ) );
}

bool CreateEntityFromAssetCommand::canMergeWith( const Command * /* other */ ) const
{
	return false;
}

bool CreateEntityFromAssetCommand::mergeWith( std::unique_ptr<Command> /* other */ )
{
	return false;
}

bool CreateEntityFromAssetCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	bool updated = editor::updateEntityReference( m_rootEntity, oldEntity, newEntity );
	updated |= editor::updateEntityReference( m_parent, oldEntity, newEntity );

	for ( auto &entity : m_createdEntities )
	{
		updated |= editor::updateEntityReference( entity, oldEntity, newEntity );
	}

	return updated;
}

ecs::Entity CreateEntityFromAssetCommand::getRecreatedEntity() const
{
	return m_rootEntity;
}

void CreateEntityFromAssetCommand::captureCreatedEntities( ecs::Entity root )
{
	if ( !root.isValid() )
		return;

	// Add root entity
	m_createdEntities.push_back( root );

	// Recursively capture all children
	const auto children = m_scene.getChildren( root );
	for ( const auto &child : children )
	{
		captureCreatedEntities( child );
	}
}

} // namespace editor

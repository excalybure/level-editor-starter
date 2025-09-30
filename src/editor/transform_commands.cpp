#include "transform_commands.h"
#include <typeinfo>

namespace editor
{

TransformEntityCommand::TransformEntityCommand( ecs::Entity entity, ecs::Scene &scene ) noexcept
	: m_entity( entity ), m_scene( &scene )
{
	// Capture the current transform state as the "before" state
	if ( const auto *transform = m_scene->getComponent<components::Transform>( m_entity ) )
	{
		m_beforeTransform = *transform;
		m_hasBeforeState = true;
	}
}

TransformEntityCommand::TransformEntityCommand( ecs::Entity entity, ecs::Scene &scene, const components::Transform &beforeTransform, const components::Transform &afterTransform ) noexcept
	: m_entity( entity ), m_scene( &scene ), m_beforeTransform( beforeTransform ), m_afterTransform( afterTransform ),
	  m_hasBeforeState( true ), m_hasAfterState( true )
{
}

bool TransformEntityCommand::execute()
{
	// Apply the "after" transform state
	if ( !m_hasAfterState )
	{
		// If no explicit after state was provided, capture the current state
		if ( const auto *transform = m_scene->getComponent<components::Transform>( m_entity ) )
		{
			m_afterTransform = *transform;
			m_hasAfterState = true;
		}
		else
		{
			return false; // Cannot execute without a valid after state
		}
	}

	// Apply the transform
	if ( auto *transform = m_scene->getComponent<components::Transform>( m_entity ) )
	{
		*transform = m_afterTransform;
		return true;
	}
	else
	{
		// Add the transform component if it doesn't exist
		return m_scene->addComponent( m_entity, m_afterTransform );
	}
}

bool TransformEntityCommand::undo()
{
	// Revert to the "before" transform state
	if ( !m_hasBeforeState )
	{
		return false; // Cannot undo without a valid before state
	}

	if ( auto *transform = m_scene->getComponent<components::Transform>( m_entity ) )
	{
		*transform = m_beforeTransform;
		return true;
	}
	else
	{
		// Add the transform component if it doesn't exist
		return m_scene->addComponent( m_entity, m_beforeTransform );
	}
}

std::string TransformEntityCommand::getDescription() const
{
	// Get entity name if it has one
	if ( m_scene->hasComponent<components::Name>( m_entity ) )
	{
		const auto *nameComp = m_scene->getComponent<components::Name>( m_entity );
		return "Transform " + nameComp->name;
	}
	return "Transform Entity";
}

size_t TransformEntityCommand::getMemoryUsage() const
{
	// Calculate memory usage: object size + any dynamic allocations
	return sizeof( *this );
}

bool TransformEntityCommand::canMergeWith( const Command *other ) const
{
	// Can merge with another TransformEntityCommand for the same entity
	if ( const auto *otherTransform = dynamic_cast<const TransformEntityCommand *>( other ) )
	{
		return m_entity == otherTransform->m_entity;
	}
	return false;
}

bool TransformEntityCommand::mergeWith( std::unique_ptr<Command> other )
{
	// Merge with another TransformEntityCommand for the same entity
	if ( auto *otherTransform = dynamic_cast<TransformEntityCommand *>( other.get() ) )
	{
		if ( m_entity == otherTransform->m_entity )
		{
			// Update our after state to match the other command's after state
			if ( otherTransform->m_hasAfterState )
			{
				m_afterTransform = otherTransform->m_afterTransform;
				m_hasAfterState = true;
			}
			return true;
		}
	}
	return false;
}

bool TransformEntityCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	return editor::updateEntityReference( m_entity, oldEntity, newEntity );
}

void TransformEntityCommand::updateAfterTransform( const components::Transform &afterTransform ) noexcept
{
	m_afterTransform = afterTransform;
	m_hasAfterState = true;
}

// BatchTransformCommand implementation

BatchTransformCommand::BatchTransformCommand( const std::vector<ecs::Entity> &entities, ecs::Scene &scene ) noexcept
	: m_scene( &scene )
{
	// Create individual transform commands for each entity
	m_commands.reserve( entities.size() );
	for ( const auto &entity : entities )
	{
		if ( scene.hasComponent<components::Transform>( entity ) )
		{
			m_commands.emplace_back( std::make_unique<TransformEntityCommand>( entity, scene ) );
		}
	}
}

void BatchTransformCommand::addTransform( ecs::Entity entity, const components::Transform &beforeTransform, const components::Transform &afterTransform )
{
	m_commands.emplace_back( std::make_unique<TransformEntityCommand>( entity, *m_scene, beforeTransform, afterTransform ) );
}

bool BatchTransformCommand::execute()
{
	bool allSucceeded = true;

	// Execute all individual commands
	for ( auto &command : m_commands )
	{
		if ( !command->execute() )
		{
			allSucceeded = false;
		}
	}

	return allSucceeded;
}

bool BatchTransformCommand::undo()
{
	bool allSucceeded = true;

	// Undo all individual commands in reverse order
	for ( auto it = m_commands.rbegin(); it != m_commands.rend(); ++it )
	{
		if ( !( *it )->undo() )
		{
			allSucceeded = false;
		}
	}

	return allSucceeded;
}

std::string BatchTransformCommand::getDescription() const
{
	const size_t entityCount = m_commands.size();
	if ( entityCount == 0 )
	{
		return "Transform 0 Entities";
	}
	else if ( entityCount == 1 )
	{
		return "Transform 1 Entity";
	}
	else
	{
		return "Transform " + std::to_string( entityCount ) + " Entities";
	}
}

size_t BatchTransformCommand::getMemoryUsage() const
{
	size_t totalMemory = sizeof( *this );

	// Add memory usage of all individual commands
	for ( const auto &command : m_commands )
	{
		totalMemory += command->getMemoryUsage();
	}

	// Add memory for the vector storage
	totalMemory += m_commands.capacity() * sizeof( std::unique_ptr<TransformEntityCommand> );

	return totalMemory;
}

bool BatchTransformCommand::canMergeWith( const Command *other ) const
{
	// Can merge with another BatchTransformCommand if they have the same entities
	if ( const auto *otherBatch = dynamic_cast<const BatchTransformCommand *>( other ) )
	{
		if ( m_commands.size() != otherBatch->m_commands.size() )
		{
			return false;
		}

		// Check that all entities match
		for ( size_t i = 0; i < m_commands.size(); ++i )
		{
			if ( m_commands[i]->getEntity() != otherBatch->m_commands[i]->getEntity() )
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

bool BatchTransformCommand::mergeWith( std::unique_ptr<Command> other )
{
	// Merge with another BatchTransformCommand
	if ( auto *otherBatch = dynamic_cast<BatchTransformCommand *>( other.get() ) )
	{
		if ( canMergeWith( otherBatch ) )
		{
			// Merge each individual command
			for ( size_t i = 0; i < m_commands.size(); ++i )
			{
				auto otherCommand = std::move( otherBatch->m_commands[i] );
				if ( !m_commands[i]->mergeWith( std::move( otherCommand ) ) )
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool BatchTransformCommand::updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	bool updated = false;
	for ( auto &command : m_commands )
	{
		if ( command->updateEntityReference( oldEntity, newEntity ) )
		{
			updated = true;
		}
	}
	return updated;
}

std::vector<ecs::Entity> BatchTransformCommand::getEntities() const
{
	std::vector<ecs::Entity> entities;
	entities.reserve( m_commands.size() );

	for ( const auto &command : m_commands )
	{
		entities.push_back( command->getEntity() );
	}

	return entities;
}

void BatchTransformCommand::updateAfterTransforms( const std::vector<components::Transform> &afterTransforms )
{
	if ( afterTransforms.size() != m_commands.size() )
	{
		return; // Size mismatch, cannot update
	}

	for ( size_t i = 0; i < m_commands.size(); ++i )
	{
		m_commands[i]->updateAfterTransform( afterTransforms[i] );
	}
}

// TransformCommandFactory implementation

std::unique_ptr<Command> TransformCommandFactory::createCommand( const std::vector<ecs::Entity> &entities, ecs::Scene &scene )
{
	if ( entities.size() == 1 )
	{
		return std::make_unique<TransformEntityCommand>( entities[0], scene );
	}
	else if ( entities.size() > 1 )
	{
		return std::make_unique<BatchTransformCommand>( entities, scene );
	}

	// Return empty command for no entities
	return nullptr;
}

std::unique_ptr<Command> TransformCommandFactory::createFromGizmoResult( const struct GizmoResult &gizmoResult,
	const std::vector<ecs::Entity> &entities,
	ecs::Scene &scene )
{
	// For now, create a basic command and let the caller handle state capture
	// This is a placeholder until GizmoResult is fully defined
	if ( entities.size() == 1 )
	{
		return std::make_unique<TransformEntityCommand>( entities[0], scene );
	}
	else if ( entities.size() > 1 )
	{
		return std::make_unique<BatchTransformCommand>( entities, scene );
	}

	return nullptr;
}

// TransformUtils implementation

namespace TransformUtils
{

std::vector<components::Transform> captureTransforms( const std::vector<ecs::Entity> &entities, ecs::Scene &scene )
{
	std::vector<components::Transform> transforms;
	transforms.reserve( entities.size() );

	for ( const auto entity : entities )
	{
		if ( const auto *transform = scene.getComponent<components::Transform>( entity ) )
		{
			transforms.push_back( *transform );
		}
		else
		{
			// Add default transform for entities without transform component
			transforms.push_back( components::Transform{} );
		}
	}

	return transforms;
}

std::vector<components::Transform> applyGizmoDeltas( const std::vector<components::Transform> &currentTransforms,
	const struct GizmoResult &gizmoResult )
{
	// This is a placeholder until GizmoResult is fully defined
	// For now, just return the current transforms unchanged
	return currentTransforms;
}

} // namespace TransformUtils

} // namespace editor
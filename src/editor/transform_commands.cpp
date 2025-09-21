#include "transform_commands.h"

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
	return "Transform Entity";
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
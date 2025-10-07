#include "CommandHistory.h"
#include "runtime/entity.h"

bool CommandHistory::undo()
{
	PROFILE_COMMAND_OPERATION( "CommandHistory::undo" );

	if ( !canUndo() )
		return false;

	--m_currentIndex;

	bool success;
	{
		PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::undo",
			m_commands[m_currentIndex].context.getMemoryUsage() );
		success = m_commands[m_currentIndex].command->undo();
	}

	// If undo was successful, check if the command recreated an entity
	if ( success )
	{
		const ecs::Entity recreatedEntity = m_commands[m_currentIndex].command->getRecreatedEntity();
		if ( recreatedEntity.isValid() )
		{
			// Get the original entity from the command
			const ecs::Entity originalEntity = m_commands[m_currentIndex].command->getOriginalEntity();

			// Fixup all commands to use the new entity reference
			fixupEntityReferences( originalEntity, recreatedEntity );
		}

		// Notify that history has changed
		notifyHistoryChanged();
	}

	return success;
}

size_t CommandHistory::fixupEntityReferences( ecs::Entity oldEntity, ecs::Entity newEntity )
{
	size_t updatedCount = 0;
	for ( auto &entry : m_commands )
	{
		if ( entry.command->updateEntityReference( oldEntity, newEntity ) )
		{
			++updatedCount;
		}
	}
	return updatedCount;
}

bool CommandHistory::redo()
{
	PROFILE_COMMAND_OPERATION( "CommandHistory::redo" );

	if ( !canRedo() )
		return false;

	bool success;
	{
		PROFILE_COMMAND_OPERATION_WITH_MEMORY( "Command::redo",
			m_commands[m_currentIndex].context.getMemoryUsage() );
		success = m_commands[m_currentIndex].command->execute();
	}

	if ( success )
	{
		// Check if the command recreated an entity (e.g., CreateEntityCommand after undo)
		const ecs::Entity recreatedEntity = m_commands[m_currentIndex].command->getRecreatedEntity();
		if ( recreatedEntity.isValid() )
		{
			// Get the original entity from the command
			const ecs::Entity originalEntity = m_commands[m_currentIndex].command->getOriginalEntity();

			// Fixup all commands to use the new entity reference
			fixupEntityReferences( originalEntity, recreatedEntity );
		}

		++m_currentIndex;

		// Notify that history has changed
		notifyHistoryChanged();
	}
	return success;
}
#pragma once

#include <memory>
#include <string>
#include "runtime/entity.h"

/**
 * @brief Abstract base class for all commands in the editor
 * 
 * Provides the core command pattern interface with execute/undo capabilities,
 * memory usage tracking, and command merging support for smooth operations.
 */
class Command
{
public:
	virtual ~Command() = default;

	/**
     * @brief Execute the command
     * @return true if execution was successful, false otherwise
     */
	virtual bool execute() = 0;

	/**
     * @brief Undo the command, reverting its effects
     * @return true if undo was successful, false otherwise
     */
	virtual bool undo() = 0;

	/**
     * @brief Get a user-friendly description of the command
     * @return Human-readable description for UI display
     */
	virtual std::string getDescription() const = 0;

	/**
     * @brief Get the memory usage of this command in bytes
     * @return Approximate memory usage for tracking and cleanup
     */
	virtual size_t getMemoryUsage() const = 0;

	/**
     * @brief Check if this command can be merged with another command
     * @param other The command to potentially merge with
     * @return true if merging is possible, false otherwise
     */
	virtual bool canMergeWith( const Command *other ) const = 0;

	/**
     * @brief Merge another command into this command
     * @param other The command to merge (will be consumed)
     * @return true if merge was successful, false otherwise
     */
	virtual bool mergeWith( std::unique_ptr<Command> other ) = 0;

	/**
     * @brief Update entity references when entities are recreated
     * @param oldEntity The old entity reference that is no longer valid
     * @param newEntity The new entity reference to use instead
     * @return true if any references were updated, false if this command doesn't reference the entity
     */
	virtual bool updateEntityReference( ecs::Entity /* oldEntity */, ecs::Entity /* newEntity */ ) { return false; }

	/**
     * @brief Called after command undo to get the entity that was recreated (if any)
     * @return Entity that was recreated, or invalid entity if none
     */
	virtual ecs::Entity getRecreatedEntity() const { return ecs::Entity{ 0, 0 }; }

	/**
     * @brief Get the original entity before recreation (if applicable)
     * @return Original entity before recreation, or invalid entity if not applicable
     */
	virtual ecs::Entity getOriginalEntity() const { return ecs::Entity{ 0, 0 }; }
};

namespace editor
{
inline bool updateEntityReference( ecs::Entity &entityRef, ecs::Entity oldEntity, ecs::Entity newEntity )
{
	if ( entityRef.id == oldEntity.id && entityRef.generation == oldEntity.generation )
	{
		entityRef = newEntity;
		return true;
	}
	return false;
}
}
#pragma once

#include <memory>
#include <string>
#include <vector>
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/math/vec.h"

namespace editor
{

/**
 * @brief Base interface for all command objects in the undo/redo system
 * 
 * Commands encapsulate operations that can be executed and undone, providing
 * the foundation for the undo/redo functionality in the level editor.
 */
class Command
{
public:
	virtual ~Command() = default;

	/**
	 * @brief Execute the command operation
	 * @return true if the command was executed successfully, false otherwise
	 */
	virtual bool execute() = 0;

	/**
	 * @brief Undo the command operation
	 * @return true if the command was undone successfully, false otherwise
	 */
	virtual bool undo() = 0;

	/**
	 * @brief Get a human-readable description of the command
	 * @return String description suitable for display in UI (e.g., "Move Object", "Rotate 3 Objects")
	 */
	virtual std::string getDescription() const = 0;
};

/**
 * @brief Command for transforming a single entity
 * 
 * Captures the before and after transform states of an entity and provides
 * execute/undo functionality to apply or revert the transformation.
 */
class TransformEntityCommand : public Command
{
public:
	/**
	 * @brief Construct a transform command for a single entity
	 * @param entity The entity to transform
	 * @param scene The scene containing the entity
	 */
	TransformEntityCommand( ecs::Entity entity, ecs::Scene &scene ) noexcept;

	/**
	 * @brief Construct a transform command with explicit before/after states
	 * @param entity The entity to transform
	 * @param scene The scene containing the entity
	 * @param beforeTransform The transform state before the change
	 * @param afterTransform The transform state after the change
	 */
	TransformEntityCommand( ecs::Entity entity, ecs::Scene &scene, const components::Transform &beforeTransform, const components::Transform &afterTransform ) noexcept;

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;

private:
	ecs::Entity m_entity;
	ecs::Scene *m_scene;
	components::Transform m_beforeTransform;
	components::Transform m_afterTransform;
	bool m_hasBeforeState = false;
	bool m_hasAfterState = false;
};

/**
 * @brief Command for transforming multiple entities simultaneously
 * 
 * Manages a collection of individual transform commands for multiple entities,
 * allowing batch operations to be executed and undone as a single unit.
 */
class BatchTransformCommand : public Command
{
public:
	/**
	 * @brief Construct a batch transform command for multiple entities
	 * @param entities The entities to transform
	 * @param scene The scene containing the entities
	 */
	BatchTransformCommand( const std::vector<ecs::Entity> &entities, ecs::Scene &scene ) noexcept;

	/**
	 * @brief Add a specific transform command to the batch
	 * @param entity The entity to transform
	 * @param beforeTransform The transform state before the change
	 * @param afterTransform The transform state after the change
	 */
	void addTransform( ecs::Entity entity, const components::Transform &beforeTransform, const components::Transform &afterTransform );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;

private:
	std::vector<std::unique_ptr<TransformEntityCommand>> m_commands;
	ecs::Scene *m_scene;
};

} // namespace editor
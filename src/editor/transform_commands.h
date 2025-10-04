#pragma once

#include <memory>
#include <string>
#include <vector>
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "math/vec.h"
#include "editor/commands/Command.h"

// Forward declarations
namespace editor
{
struct GizmoResult;
}

namespace editor
{

/**
 * @brief Command for transforming a single entity
 * 
 * Captures the before and after transform states of an entity and provides
 * execute/undo functionality to apply or revert the transformation.
 * Supports command merging for smooth gizmo manipulation.
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
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

	/**
	 * @brief Get the entity being transformed
	 * @return The entity ID
	 */
	ecs::Entity getEntity() const noexcept { return m_entity; }

	/**
	 * @brief Update the after transform state (for command merging)
	 * @param afterTransform The new after transform state
	 */
	void updateAfterTransform( const components::Transform &afterTransform ) noexcept;

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
 * Supports command merging for smooth multi-entity gizmo manipulation.
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
	 * @brief Add a transform for a specific entity with explicit before/after states
	 * @param entity The entity to transform
	 * @param beforeTransform The transform state before the change
	 * @param afterTransform The transform state after the change
	 */
	void addTransform( ecs::Entity entity, const components::Transform &beforeTransform, const components::Transform &afterTransform );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

	/**
	 * @brief Get the entities being transformed
	 * @return Vector of entity IDs
	 */
	std::vector<ecs::Entity> getEntities() const;

	/**
	 * @brief Update after transform states for all entities (for command merging)
	 * @param afterTransforms Vector of new after transform states (must match entity count)
	 */
	void updateAfterTransforms( const std::vector<components::Transform> &afterTransforms );

private:
	ecs::Scene *m_scene;
	std::vector<std::unique_ptr<TransformEntityCommand>> m_commands;
};

/**
 * @brief Factory class for creating transform commands
 * 
 * Provides convenient methods for creating transform commands from various inputs,
 * ensuring proper command creation for use with the command history system.
 */
class TransformCommandFactory
{
public:
	/**
	 * @brief Create a transform command for the given entities
	 * @param entities The entities to transform
	 * @param scene The scene containing the entities
	 * @return Command for single entity or batch command for multiple entities
	 */
	static std::unique_ptr<Command> createCommand( const std::vector<ecs::Entity> &entities, ecs::Scene &scene );
};

} // namespace editor
#pragma once

#include <memory>
#include <string>
#include <variant>
#include <optional>
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "math/vec.h"
#include "editor/commands/Command.h"

// Forward declarations
namespace assets
{
class Scene;
class Mesh;
class MaterialInstance;
} // namespace assets

namespace graphics
{
class GPUResourceManager;
} // namespace graphics

namespace editor
{

/**
 * @brief Enum for material property types
 */
enum class MaterialPropertyType
{
	BaseColorFactor,
	MetallicFactor,
	RoughnessFactor,
	EmissiveFactor,
	BaseColorTexture,
	MetallicRoughnessTexture,
	NormalTexture,
	EmissiveTexture
};

/**
 * @brief Type-safe variant for material property values
 */
using MaterialPropertyValue = std::variant<
	math::Vec4f, // BaseColorFactor
	float,		 // MetallicFactor, RoughnessFactor
	math::Vec3f, // EmissiveFactor
	std::string	 // Texture paths
	>;

/**
 * @brief Command for setting a material property override on a primitive
 * 
 * Captures the before and after states of a material property override for a specific
 * primitive within a mesh. Supports execute/undo functionality and triggers GPU updates.
 * 
 * This command modifies the MaterialInstance stored in the Primitive, which contains
 * per-primitive overrides of material properties. The command stores both the old and
 * new values to support undo/redo operations.
 */
class SetPrimitiveMaterialPropertyCommand : public Command
{
public:
	/**
	 * @brief Construct a command to set a material property override
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive within the mesh
	 * @param propertyType The type of property to modify
	 * @param newValue The new value to set
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 * @param gpuManager Optional GPU resource manager for triggering material updates
	 */
	SetPrimitiveMaterialPropertyCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		MaterialPropertyType propertyType,
		MaterialPropertyValue newValue,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene,
		graphics::GPUResourceManager *gpuManager = nullptr );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command *other ) const override;
	bool mergeWith( std::unique_ptr<Command> other ) override;
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

	/**
	 * @brief Get the entity being modified
	 * @return The entity ID
	 */
	ecs::Entity getEntity() const noexcept { return m_entity; }

	/**
	 * @brief Get the primitive index being modified
	 * @return The primitive index
	 */
	uint32_t getPrimitiveIndex() const noexcept { return m_primitiveIndex; }

	/**
	 * @brief Get the property type being modified
	 * @return The property type
	 */
	MaterialPropertyType getPropertyType() const noexcept { return m_propertyType; }

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;
	MaterialPropertyType m_propertyType;
	MaterialPropertyValue m_oldValue;
	MaterialPropertyValue m_newValue;
	bool m_hadOverrideBefore; // Track if override existed before this command

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
	graphics::GPUResourceManager *m_gpuManager;

	// Helper methods
	bool applyPropertyValue( const MaterialPropertyValue &value, bool isOverride );
	MaterialPropertyValue getCurrentPropertyValue() const;
	void triggerGPUUpdate();
	std::string propertyTypeToString() const;
};

/**
 * @brief Command for clearing a material property override on a primitive
 * 
 * Removes a specific property override from a primitive's MaterialInstance,
 * reverting it back to using the base material's value.
 */
class ClearPrimitiveMaterialPropertyCommand : public Command
{
public:
	/**
	 * @brief Construct a command to clear a material property override
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive within the mesh
	 * @param propertyType The type of property to clear
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 * @param gpuManager Optional GPU resource manager for triggering material updates
	 */
	ClearPrimitiveMaterialPropertyCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		MaterialPropertyType propertyType,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene,
		graphics::GPUResourceManager *gpuManager = nullptr );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command * ) const override { return false; } // No merging for clear commands
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;
	MaterialPropertyType m_propertyType;
	MaterialPropertyValue m_oldValue; // Store old value for undo
	bool m_hadOverride;				  // Track if override existed before clear

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
	graphics::GPUResourceManager *m_gpuManager;

	// Helper methods
	MaterialPropertyValue getCurrentPropertyValue() const;
	void triggerGPUUpdate();
	std::string propertyTypeToString() const;
};

/**
 * @brief Command for clearing all material property overrides on a primitive
 * 
 * Removes all property overrides from a primitive's MaterialInstance,
 * reverting the entire primitive back to using the base material's values.
 */
class ClearAllPrimitiveMaterialOverridesCommand : public Command
{
public:
	/**
	 * @brief Construct a command to clear all material property overrides
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive within the mesh
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 * @param gpuManager Optional GPU resource manager for triggering material updates
	 */
	ClearAllPrimitiveMaterialOverridesCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene,
		graphics::GPUResourceManager *gpuManager = nullptr );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;
	assets::MaterialInstance m_oldMaterialInstance; // Store entire MaterialInstance for undo

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
	graphics::GPUResourceManager *m_gpuManager;

	void triggerGPUUpdate();
};

/**
 * @brief Command for copying a primitive's material overrides to clipboard
 * 
 * Serializes the MaterialInstance of a primitive to JSON and places it on the clipboard.
 * This command does not modify the primitive; it's primarily for UI interaction.
 */
class CopyPrimitiveMaterialCommand : public Command
{
public:
	/**
	 * @brief Construct a command to copy material overrides to clipboard
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive within the mesh
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 */
	CopyPrimitiveMaterialCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene );

	// Command interface implementation
	bool execute() override;
	bool undo() override { return true; } // Copy doesn't need undo
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
};

/**
 * @brief Command for pasting material overrides from clipboard to a primitive
 * 
 * Parses a MaterialInstance from clipboard JSON and applies it to a primitive,
 * replacing all overrides on the target primitive with the copied values.
 * 
 * Note: Clipboard access is deferred to execute() rather than constructor,
 * allowing this command to be unit-tested without requiring an active ImGui context.
 */
class PastePrimitiveMaterialCommand : public Command
{
public:
	/**
	 * @brief Construct a command to paste material overrides from clipboard
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive to paste into
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 * @param gpuManager Optional GPU resource manager for triggering material updates
	 */
	PastePrimitiveMaterialCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene,
		graphics::GPUResourceManager *gpuManager = nullptr );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;
	assets::MaterialInstance m_oldMaterialInstance; // Store old state for undo
	assets::MaterialInstance m_newMaterialInstance; // Store new state to apply
	bool m_clipboardParsed = false;					// Track if clipboard has been read/parsed

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
	graphics::GPUResourceManager *m_gpuManager;

	// Helper: parse JSON string into MaterialInstance
	void parseClipboardJson( const char *jsonText );
	void triggerGPUUpdate();

public:
	// For unit testing: allows direct JSON parsing without ImGui context
	void parseJsonForTesting( const char *jsonText ) { parseClipboardJson( jsonText ); }
};

/**
 * @brief Command for applying a material preset to a primitive
 * 
 * Applies a saved material preset to a primitive's MaterialInstance.
 * Only properties present in the preset are applied (partial override support).
 * Supports undo/redo by storing the previous MaterialInstance state.
 */
class ApplyMaterialPresetCommand : public Command
{
public:
	/**
	 * @brief Construct a command to apply a material preset
	 * @param entity The entity with the MeshRenderer component
	 * @param primitiveIndex Index of the primitive within the mesh
	 * @param preset The preset to apply
	 * @param ecsScene The ECS scene containing the entity
	 * @param assetScene The asset scene containing the mesh data
	 * @param gpuManager Optional GPU resource manager for triggering material updates
	 */
	ApplyMaterialPresetCommand(
		ecs::Entity entity,
		uint32_t primitiveIndex,
		const assets::MaterialPreset &preset,
		ecs::Scene &ecsScene,
		assets::Scene &assetScene,
		graphics::GPUResourceManager *gpuManager = nullptr );

	// Command interface implementation
	bool execute() override;
	bool undo() override;
	std::string getDescription() const override;
	size_t getMemoryUsage() const override;
	bool canMergeWith( const Command * ) const override { return false; }
	bool mergeWith( std::unique_ptr<Command> ) override { return false; }
	bool updateEntityReference( ecs::Entity oldEntity, ecs::Entity newEntity ) override;

private:
	ecs::Entity m_entity;
	uint32_t m_primitiveIndex;
	std::string m_presetName;						// For description
	assets::MaterialInstance m_oldMaterialInstance; // Store old state for undo
	assets::MaterialPreset m_preset;				// Preset to apply

	ecs::Scene *m_ecsScene;
	assets::Scene *m_assetScene;
	graphics::GPUResourceManager *m_gpuManager;

	void triggerGPUUpdate();
};

// Helper: update MaterialGPU from asset primitive's MaterialInstance
// Exposed for unit tests
bool updateMaterialGPUForPrimitive(
	ecs::Entity entity,
	uint32_t primitiveIndex,
	ecs::Scene *ecsScene,
	assets::Scene *assetScene,
	graphics::GPUResourceManager *gpuManager );

} // namespace editor

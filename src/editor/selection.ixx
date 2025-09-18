export module editor.selection;

import runtime.ecs;
import runtime.entity;
import runtime.components;
import runtime.systems;
import engine.bounding_box_3d;
import engine.vec;
import runtime.time;
import <vector>;
import <functional>;
import <algorithm>;

export namespace editor
{

// Selection change event data
export struct SelectionChangedEvent
{
	std::vector<ecs::Entity> previousSelection;
	std::vector<ecs::Entity> currentSelection;
	std::vector<ecs::Entity> added;
	std::vector<ecs::Entity> removed;
	ecs::Entity newPrimarySelection{};
	ecs::Entity previousPrimarySelection{};
};

// Central selection management
export class SelectionManager
{
public:
	explicit SelectionManager( ecs::Scene &scene, systems::SystemManager &systemManager );

	// Basic selection operations
	void select( ecs::Entity entity, bool additive = false );
	void select( const std::vector<ecs::Entity> &entities, bool additive = false );
	void deselect( ecs::Entity entity );
	void deselectAll();
	void toggleSelection( ecs::Entity entity );

	// Selection queries
	const std::vector<ecs::Entity> &getSelectedEntities() const { return m_selection; }
	bool isSelected( ecs::Entity entity ) const;
	size_t getSelectionCount() const { return m_selection.size(); }
	bool hasSelection() const { return !m_selection.empty(); }
	ecs::Entity getFirstSelected() const;

	// Primary selection (for transform gizmos)
	ecs::Entity getPrimarySelection() const { return m_primarySelection; }
	void setPrimarySelection( ecs::Entity entity );

	// Spatial selection queries
	math::BoundingBox3Df getSelectionBounds() const;
	math::Vec3<> getSelectionCenter() const;
	float getSelectionRadius() const; // Bounding sphere radius

	// Event registration
	using SelectionListener = std::function<void( const SelectionChangedEvent & )>;
	void registerListener( SelectionListener listener );
	void unregisterAllListeners();

	// Validation and cleanup
	void validateSelection(); // Remove invalid/destroyed entities
	void refreshFromECS();	  // Rebuild selection from Selected components

	// Serialization support
	std::vector<ecs::Entity> captureSelection() const;
	void restoreSelection( const std::vector<ecs::Entity> &entities, ecs::Entity primary = {} );

private:
	ecs::Scene &m_scene;
	systems::SystemManager &m_systemManager;
	std::vector<ecs::Entity> m_selection;
	ecs::Entity m_primarySelection{};
	std::vector<SelectionListener> m_listeners;

	void notifySelectionChanged( const std::vector<ecs::Entity> &previousSelection,
		ecs::Entity previousPrimary );
	void syncToECS( const std::vector<ecs::Entity> &added,
		const std::vector<ecs::Entity> &removed );

	SelectionChangedEvent createChangeEvent( const std::vector<ecs::Entity> &previous,
		ecs::Entity previousPrimary ) const;

	// Legacy fallback method for when TransformSystem is not available
	math::BoundingBox3Df getSelectionBoundsLegacy() const;
};

} // namespace editor
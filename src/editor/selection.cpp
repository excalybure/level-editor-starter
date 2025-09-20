module editor.selection;

import runtime.entity;
import runtime.components;
import runtime.systems;
import engine.math;

namespace editor
{

SelectionManager::SelectionManager( ecs::Scene &scene, systems::SystemManager &systemManager )
	: m_scene( scene ), m_systemManager( systemManager )
{
}

void SelectionManager::select( ecs::Entity entity, bool additive )
{
	if ( !m_scene.isValid( entity ) )
	{
		// console::warning("Attempting to select invalid entity");
		return;
	}

	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	if ( !additive )
	{
		// Clear current selection
		const auto toRemove = m_selection;
		m_selection.clear();
		m_primarySelection = ecs::Entity{};
		syncToECS( {}, toRemove );
	}

	// Check if entity is already selected
	const auto it = std::find( m_selection.begin(), m_selection.end(), entity );
	if ( it == m_selection.end() )
	{
		// Add to selection
		m_selection.push_back( entity );

		// Set as primary if no current primary or if this is the only selection
		if ( m_primarySelection == ecs::Entity{} || m_selection.size() == 1 )
		{
			m_primarySelection = entity;
		}

		syncToECS( { entity }, {} );
	}

	notifySelectionChanged( previousSelection, previousPrimary );
}

void SelectionManager::select( const std::vector<ecs::Entity> &entities, bool additive )
{
	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	std::vector<ecs::Entity> toAdd;
	std::vector<ecs::Entity> toRemove;

	if ( !additive )
	{
		// Clear current selection
		toRemove = m_selection;
		m_selection.clear();
		m_primarySelection = ecs::Entity{};
	}

	// Add valid entities
	for ( const auto entity : entities )
	{
		if ( !m_scene.isValid( entity ) )
		{
			// console::warning("Skipping invalid entity in batch selection");
			continue;
		}

		const auto it = std::find( m_selection.begin(), m_selection.end(), entity );
		if ( it == m_selection.end() )
		{
			m_selection.push_back( entity );
			toAdd.push_back( entity );
		}
	}

	// Set first valid entity as primary if none set
	if ( m_primarySelection == ecs::Entity{} && !m_selection.empty() )
	{
		m_primarySelection = m_selection[0];
	}

	syncToECS( toAdd, toRemove );
	notifySelectionChanged( previousSelection, previousPrimary );
}

void SelectionManager::deselect( ecs::Entity entity )
{
	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	const auto it = std::find( m_selection.begin(), m_selection.end(), entity );
	if ( it != m_selection.end() )
	{
		m_selection.erase( it );

		// If this was primary, choose new primary
		if ( m_primarySelection == entity )
		{
			m_primarySelection = m_selection.empty() ? ecs::Entity{} : m_selection[0];
		}

		syncToECS( {}, { entity } );
		notifySelectionChanged( previousSelection, previousPrimary );
	}
}

void SelectionManager::deselectAll()
{
	if ( m_selection.empty() )
	{
		return;
	}
	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	const auto toRemove = m_selection;
	m_selection.clear();
	m_primarySelection = ecs::Entity{};

	syncToECS( {}, toRemove );
	notifySelectionChanged( previousSelection, previousPrimary );
}

void SelectionManager::toggleSelection( ecs::Entity entity )
{
	if ( !m_scene.isValid( entity ) )
	{
		// console::warning("Attempting to toggle selection on invalid entity");
		return;
	}

	if ( isSelected( entity ) )
	{
		deselect( entity );
	}
	else
	{
		select( entity, true ); // Additive
	}
}

bool SelectionManager::isSelected( ecs::Entity entity ) const
{
	const auto it = std::find( m_selection.begin(), m_selection.end(), entity );
	return it != m_selection.end();
}

ecs::Entity SelectionManager::getFirstSelected() const
{
	return m_selection.empty() ? ecs::Entity{} : m_selection[0];
}

void SelectionManager::setPrimarySelection( ecs::Entity entity )
{
	if ( entity != ecs::Entity{} && !isSelected( entity ) )
	{
		// console::warning("Cannot set non-selected entity as primary");
		return;
	}

	const auto previousPrimary = m_primarySelection;
	m_primarySelection = entity;

	// Update ECS components to reflect primary change
	if ( previousPrimary != ecs::Entity{} )
	{
		if ( auto *const selected = m_scene.getComponent<components::Selected>( previousPrimary ) )
		{
			selected->isPrimary = false;
		}
	}

	if ( entity != ecs::Entity{} )
	{
		if ( auto *const selected = m_scene.getComponent<components::Selected>( entity ) )
		{
			selected->isPrimary = true;
		}
	}

	notifySelectionChanged( m_selection, previousPrimary );
}

math::BoundingBox3Df SelectionManager::getSelectionBounds() const
{
	if ( m_selection.empty() )
	{
		return math::BoundingBox3Df{};
	}

	// Get the TransformSystem for proper world transforms
	auto *transformSystem = m_systemManager.getSystem<systems::TransformSystem>();
	if ( !transformSystem )
	{
		// TransformSystem is required for proper hierarchical transforms
		return math::BoundingBox3Df{};
	}

	math::BoundingBox3Df combinedBounds;
	bool firstBounds = true;

	for ( const auto entity : m_selection )
	{
		auto *const transform = m_scene.getComponent<components::Transform>( entity );
		auto *const meshRenderer = m_scene.getComponent<components::MeshRenderer>( entity );

		if ( transform && meshRenderer && meshRenderer->bounds.isValid() )
		{
			// Get world transform using TransformSystem for proper hierarchical transforms
			const auto worldMatrix = transformSystem->getWorldTransform( m_scene, entity );

			// Transform corners of bounding box
			for ( int i = 0; i < 8; ++i )
			{
				const auto corner = meshRenderer->bounds.corner( i );
				const auto worldCorner = worldMatrix.transformPoint( corner );
				combinedBounds.expand( worldCorner );
			}
		}
	}

	return combinedBounds;
}


float SelectionManager::getSelectionRadius() const
{
	const auto bounds = getSelectionBounds();
	if ( !bounds.isValid() )
	{
		return 0.0f;
	}

	const auto size = bounds.size();
	return math::length( size ) * 0.5f;
}

void SelectionManager::registerListener( SelectionListener listener )
{
	m_listeners.push_back( listener );
}

void SelectionManager::unregisterAllListeners()
{
	m_listeners.clear();
}

void SelectionManager::validateSelection()
{

	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	std::vector<ecs::Entity> toRemove;

	// Remove invalid entities
	const auto it = std::remove_if( m_selection.begin(), m_selection.end(), [&]( ecs::Entity entity ) {
		const bool invalid = !m_scene.isValid( entity );
		if ( invalid )
		{
			toRemove.push_back( entity );
		}
		return invalid;
	} );

	m_selection.erase( it, m_selection.end() );

	// Update primary if it was removed
	if ( !m_scene.isValid( m_primarySelection ) )
	{
		m_primarySelection = m_selection.empty() ? ecs::Entity{} : m_selection[0];
	}

	if ( !toRemove.empty() )
	{
		syncToECS( {}, toRemove );
		notifySelectionChanged( previousSelection, previousPrimary );
	}
}

void SelectionManager::refreshFromECS()
{
	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	m_selection.clear();
	m_primarySelection = ecs::Entity{};

	// Rebuild selection from ECS components
	m_scene.forEach<components::Selected>( [&]( ecs::Entity entity, const components::Selected &selected ) {
		m_selection.push_back( entity );
		if ( selected.isPrimary )
		{
			m_primarySelection = entity;
		}
	} );

	notifySelectionChanged( previousSelection, previousPrimary );
}

std::vector<ecs::Entity> SelectionManager::captureSelection() const
{
	return m_selection;
}

void SelectionManager::restoreSelection( const std::vector<ecs::Entity> &entities, ecs::Entity primary )
{
	const auto previousSelection = m_selection;
	const auto previousPrimary = m_primarySelection;

	// Clear current selection
	const auto toRemove = m_selection;
	m_selection.clear();
	m_primarySelection = ecs::Entity{};

	// Restore entities (only valid ones)
	std::vector<ecs::Entity> toAdd;
	for ( const auto entity : entities )
	{
		if ( m_scene.isValid( entity ) )
		{
			m_selection.push_back( entity );
			toAdd.push_back( entity );
		}
	}

	// Set primary if valid and in selection
	if ( primary != ecs::Entity{} && isSelected( primary ) )
	{
		m_primarySelection = primary;
	}
	else if ( !m_selection.empty() )
	{
		m_primarySelection = m_selection[0];
	}

	syncToECS( toAdd, toRemove );
	notifySelectionChanged( previousSelection, previousPrimary );
}

void SelectionManager::notifySelectionChanged( const std::vector<ecs::Entity> &previousSelection,
	ecs::Entity previousPrimary )
{
	auto event = createChangeEvent( previousSelection, previousPrimary );

	for ( auto &listener : m_listeners )
	{
		listener( event );
	}
}

void SelectionManager::syncToECS( const std::vector<ecs::Entity> &added,
	const std::vector<ecs::Entity> &removed )
{
	// Remove Selected components from deselected entities
	for ( const auto entity : removed )
	{
		if ( m_scene.hasComponent<components::Selected>( entity ) )
		{
			m_scene.removeComponent<components::Selected>( entity );
		}
	}

	// Add Selected components to newly selected entities
	for ( const auto entity : added )
	{
		bool isPrimary = ( entity == m_primarySelection );
		m_scene.addComponent( entity, components::Selected{ isPrimary } );
	}

	// Update primary status for all selected entities
	for ( const auto entity : m_selection )
	{
		if ( auto *selected = m_scene.getComponent<components::Selected>( entity ) )
		{
			selected->isPrimary = ( entity == m_primarySelection );
		}
	}
}

SelectionChangedEvent SelectionManager::createChangeEvent( const std::vector<ecs::Entity> &previous,
	ecs::Entity previousPrimary ) const
{
	SelectionChangedEvent event;
	event.previousSelection = previous;
	event.currentSelection = m_selection;
	event.newPrimarySelection = m_primarySelection;
	event.previousPrimarySelection = previousPrimary;

	// Calculate added entities
	for ( const auto entity : m_selection )
	{
		if ( std::find( previous.begin(), previous.end(), entity ) == previous.end() )
		{
			event.added.push_back( entity );
		}
	}

	// Calculate removed entities
	for ( const auto entity : previous )
	{
		if ( std::find( m_selection.begin(), m_selection.end(), entity ) == m_selection.end() )
		{
			event.removed.push_back( entity );
		}
	}

	return event;
}

} // namespace editor
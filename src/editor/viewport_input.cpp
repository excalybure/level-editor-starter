#include "viewport_input.h"

#include <vector>
#include <algorithm>

#include "editor/selection.h"
#include "engine/picking.h"
#include "editor/viewport/viewport.h"
#include "runtime/ecs.h"
#include "runtime/entity.h"
#include "runtime/components.h"
#include "runtime/systems.h"
#include "engine/math/vec.h"
#include "engine/math/matrix.h"

namespace editor
{

ViewportInputHandler::ViewportInputHandler( SelectionManager &selectionManager,
	picking::PickingSystem &pickingSystem,
	systems::SystemManager &systemManager )
	: m_selectionManager( selectionManager ), m_pickingSystem( pickingSystem ), m_systemManager( systemManager )
{
}

void ViewportInputHandler::handleMouseClick( ecs::Scene &scene,
	const Viewport &viewport,
	const math::Vec2f &screenPos,
	bool leftButton,
	bool rightButton,
	bool ctrlPressed,
	bool shiftPressed )
{
	// Only handle left button clicks for selection
	if ( !leftButton || rightButton )
	{
		return;
	}

	// Get selection mode from modifier keys
	const auto mode = getSelectionMode( ctrlPressed, shiftPressed );

	// Perform picking using viewport's ray casting
	const auto viewportRay = viewport.getPickingRay( screenPos );
	const auto hitResult = m_pickingSystem.raycast( scene, viewportRay.origin, viewportRay.direction, viewportRay.length );

	if ( hitResult.hit && scene.isValid( hitResult.entity ) )
	{
		// Entity hit - apply selection based on mode
		switch ( mode )
		{
		case SelectionMode::Replace:
			m_selectionManager.select( hitResult.entity, false );
			break;

		case SelectionMode::Add:
			m_selectionManager.select( hitResult.entity, true );
			break;

		case SelectionMode::Subtract:
			m_selectionManager.deselect( hitResult.entity );
			break;

		case SelectionMode::Toggle:
			m_selectionManager.toggleSelection( hitResult.entity );
			break;
		}
	}
	else
	{
		// No entity hit - clear selection unless adding
		if ( mode == SelectionMode::Replace )
		{
			m_selectionManager.deselectAll();
		}
	}
}

void ViewportInputHandler::handleMouseDrag( ecs::Scene & /*scene*/,
	const Viewport & /*viewport*/,
	const math::Vec2f &startPos,
	const math::Vec2f &currentPos,
	bool ctrlPressed,
	bool shiftPressed )
{
	// Calculate drag distance to determine if this is a drag operation
	const auto dragDistance = math::length( currentPos - startPos );
	const float MIN_DRAG_DISTANCE = 5.0f; // Minimum pixels to start rectangle selection

	if ( dragDistance >= MIN_DRAG_DISTANCE )
	{
		// Start/update rectangle selection
		if ( !m_rectSelection.active )
		{
			m_rectSelection.active = true;
			m_rectSelection.startPos = startPos;
			m_rectSelection.mode = getSelectionMode( ctrlPressed, shiftPressed );
		}

		m_rectSelection.endPos = currentPos;
	}
}

void ViewportInputHandler::handleMouseRelease( ecs::Scene &scene,
	const Viewport &viewport,
	const math::Vec2f & /*releasePos*/ )
{
	if ( m_rectSelection.active )
	{
		// Apply rectangle selection
		applyRectSelection( scene, viewport );

		// Clear rectangle selection state
		m_rectSelection.active = false;
	}
}

void ViewportInputHandler::handleMouseMove( ecs::Scene &scene,
	const Viewport &viewport,
	const math::Vec2f &screenPos )
{
	// Update hover state
	updateHoverState( scene, viewport, screenPos );

	// Store last mouse position for delta calculations
	m_lastMousePos = screenPos;
}

SelectionMode ViewportInputHandler::getSelectionMode( bool ctrlPressed, bool shiftPressed ) const
{
	if ( ctrlPressed && shiftPressed )
	{
		return SelectionMode::Subtract;
	}
	else if ( ctrlPressed )
	{
		return SelectionMode::Add;
	}
	else if ( shiftPressed )
	{
		return SelectionMode::Toggle;
	}
	else
	{
		return SelectionMode::Replace;
	}
}

std::vector<ecs::Entity> ViewportInputHandler::getEntitiesInRect( ecs::Scene &scene,
	const Viewport &viewport,
	const math::Vec2f &minPos,
	const math::Vec2f &maxPos ) const
{
	std::vector<ecs::Entity> entitiesInRect;

	// Get TransformSystem for proper hierarchical transforms
	auto *transformSystem = m_systemManager.getSystem<systems::TransformSystem>();
	if ( !transformSystem )
	{
		// TransformSystem is required for proper hierarchical transforms
		return entitiesInRect;
	}

	// Simple implementation: test center point of each entity's screen projection
	// In a more advanced implementation, we could test entity bounds overlap with rectangle

	for ( const auto entity : scene.getAllEntities() )
	{
		if ( scene.hasComponent<components::Transform>( entity ) &&
			scene.hasComponent<components::MeshRenderer>( entity ) )
		{
			// Get entity world position using TransformSystem for proper hierarchical transforms
			const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );
			const auto worldPos = math::Vec3f{ worldMatrix.m30(), worldMatrix.m31(), worldMatrix.m32() }; // Extract translation

			// Project to screen coordinates
			const auto screenPos = viewport.worldToScreen( worldPos );

			// Check if screen position is within rectangle bounds
			if ( screenPos.x >= minPos.x && screenPos.x <= maxPos.x &&
				screenPos.y >= minPos.y && screenPos.y <= maxPos.y )
			{
				entitiesInRect.push_back( entity );
			}
		}
	}

	return entitiesInRect;
}

void ViewportInputHandler::applyRectSelection( ecs::Scene &scene, const Viewport &viewport )
{
	// Calculate rectangle bounds
	const auto minPos = math::Vec2f{
		std::min( m_rectSelection.startPos.x, m_rectSelection.endPos.x ),
		std::min( m_rectSelection.startPos.y, m_rectSelection.endPos.y )
	};

	const auto maxPos = math::Vec2f{
		std::max( m_rectSelection.startPos.x, m_rectSelection.endPos.x ),
		std::max( m_rectSelection.startPos.y, m_rectSelection.endPos.y )
	};

	// Get entities within rectangle
	const auto entitiesInRect = getEntitiesInRect( scene, viewport, minPos, maxPos );

	// Apply selection based on mode
	switch ( m_rectSelection.mode )
	{
	case SelectionMode::Replace:
		// Clear current selection and select entities in rectangle
		m_selectionManager.deselectAll();
		if ( !entitiesInRect.empty() )
		{
			m_selectionManager.select( entitiesInRect, false );
		}
		break;

	case SelectionMode::Add:
		// Add entities in rectangle to current selection
		if ( !entitiesInRect.empty() )
		{
			m_selectionManager.select( entitiesInRect, true );
		}
		break;

	case SelectionMode::Subtract:
		// Remove entities in rectangle from current selection
		for ( const auto entity : entitiesInRect )
		{
			m_selectionManager.deselect( entity );
		}
		break;

	case SelectionMode::Toggle:
		// Toggle selection state of entities in rectangle
		for ( const auto entity : entitiesInRect )
		{
			m_selectionManager.toggleSelection( entity );
		}
		break;
	}
}

void ViewportInputHandler::updateHoverState( ecs::Scene &scene, const Viewport &viewport, const math::Vec2f &screenPos )
{
	// Perform picking using viewport's ray casting
	const auto viewportRay = viewport.getPickingRay( screenPos );
	const auto hitResult = m_pickingSystem.raycast( scene, viewportRay.origin, viewportRay.direction, viewportRay.length );

	if ( hitResult.hit && scene.isValid( hitResult.entity ) )
	{
		m_hoveredEntity = hitResult.entity;
	}
	else
	{
		m_hoveredEntity = ecs::Entity{};
	}
}

} // namespace editor
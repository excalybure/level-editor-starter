#include "gizmos.h"
#include "selection.h"
#include "runtime/components.h"
#include "runtime/ecs.h"

namespace editor
{

GizmoSystem::GizmoSystem( SelectionManager &selectionManager, ecs::Scene &scene ) noexcept
	: m_selectionManager( &selectionManager ), m_scene( &scene )
{
	// Initialize with default values (already set in header)
}

math::Vec3<> GizmoSystem::calculateSelectionCenter() const
{
	if ( !m_selectionManager || !m_scene )
	{
		return math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return math::Vec3<>{ 0.0f, 0.0f, 0.0f };
	}

	// Calculate average position of all selected entities
	math::Vec3<> center{ 0.0f, 0.0f, 0.0f };
	int validEntityCount = 0;

	for ( const auto entity : selectedEntities )
	{
		if ( m_scene->hasComponent<components::Transform>( entity ) )
		{
			const auto *transform = m_scene->getComponent<components::Transform>( entity );
			center += transform->position;
			validEntityCount++;
		}
	}

	if ( validEntityCount > 0 )
	{
		center /= static_cast<float>( validEntityCount );
	}

	return center;
}

math::Mat4<> GizmoSystem::calculateGizmoMatrix() const
{
	if ( !m_selectionManager || !m_scene )
	{
		return math::Mat4<>::identity();
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return math::Mat4<>::identity();
	}

	// For now, we'll create a translation matrix positioned at the selection center
	// In the future, this could account for rotation based on gizmo mode (local vs world)
	const auto center = calculateSelectionCenter();

	// Create translation matrix
	return math::Mat4<>::translation( center );
}

void GizmoSystem::applyTransformDelta( const GizmoResult &delta )
{
	if ( !m_selectionManager || !m_scene )
	{
		return;
	}

	const auto &selectedEntities = m_selectionManager->getSelectedEntities();
	if ( selectedEntities.empty() )
	{
		return;
	}

	// Apply delta to all selected entities
	for ( const auto entity : selectedEntities )
	{
		if ( m_scene->hasComponent<components::Transform>( entity ) )
		{
			auto *transform = m_scene->getComponent<components::Transform>( entity );

			// Apply translation delta (additive)
			transform->position += delta.translationDelta;

			// Apply rotation delta (additive)
			transform->rotation += delta.rotationDelta;

			// Apply scale delta (multiplicative)
			transform->scale *= delta.scaleDelta;

			// Mark transform as dirty for matrix recalculation
			transform->markDirty();
		}
	}
}

} // namespace editor
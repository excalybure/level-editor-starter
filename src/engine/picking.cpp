#include "picking.h"

#include "engine/math/math_3d.h"
#include "engine/math/bounding_volumes.h"
#include "engine/math/matrix.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "runtime/entity.h"
#include "runtime/systems.h"
#include "editor/viewport/viewport.h"

namespace picking
{

using namespace math;

PickingSystem::PickingSystem( systems::SystemManager &systemManager )
	: m_systemManager( systemManager )
{
}

HitResult PickingSystem::raycast( ecs::Scene &scene,
	const Vec3<> &rayOrigin,
	const Vec3<> &rayDirection,
	float maxDistance ) const
{
	HitResult closestHit;
	closestHit.distance = maxDistance;

	// Iterate through all entities and check for both Transform and MeshRenderer components
	for ( const auto entity : scene.getAllEntities() )
	{
		if ( scene.hasComponent<components::Transform>( entity ) &&
			scene.hasComponent<components::MeshRenderer>( entity ) )
		{
			float hitDistance = 0.0f;

			// Test entity bounds first for early rejection
			if ( testEntityBounds( scene, entity, rayOrigin, rayDirection, hitDistance ) )
			{
				if ( hitDistance < closestHit.distance )
				{
					HitResult result;
					if ( testEntityMesh( scene, entity, rayOrigin, rayDirection, result ) )
					{
						if ( result.distance < closestHit.distance )
						{
							closestHit = result;
							closestHit.entity = entity;
							closestHit.hit = true;
						}
					}
				}
			}
		}
	}

	return closestHit;
}

std::vector<HitResult> PickingSystem::raycastAll( ecs::Scene &scene,
	const Vec3<> &rayOrigin,
	const Vec3<> &rayDirection,
	float maxDistance ) const
{
	std::vector<HitResult> hits;

	// Iterate through all entities and check for both Transform and MeshRenderer components
	for ( const auto entity : scene.getAllEntities() )
	{
		if ( scene.hasComponent<components::Transform>( entity ) &&
			scene.hasComponent<components::MeshRenderer>( entity ) )
		{
			float hitDistance = 0.0f;

			// Test entity bounds first for early rejection
			if ( testEntityBounds( scene, entity, rayOrigin, rayDirection, hitDistance ) )
			{
				if ( hitDistance <= maxDistance )
				{
					HitResult result;
					if ( testEntityMesh( scene, entity, rayOrigin, rayDirection, result ) )
					{
						if ( result.distance <= maxDistance )
						{
							result.entity = entity;
							result.hit = true;
							hits.push_back( result );
						}
					}
				}
			}
		}
	}

	// Sort hits by distance
	std::sort( hits.begin(), hits.end() );

	return hits;
}

bool PickingSystem::testEntityBounds( ecs::Scene &scene, ecs::Entity entity, const Vec3<> &rayOrigin, const Vec3<> &rayDirection, float &hitDistance ) const
{
	auto *const meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	auto *const transform = scene.getComponent<components::Transform>( entity );

	if ( !meshRenderer || !transform )
	{
		return false;
	}

	const auto &bounds = meshRenderer->bounds;
	if ( !bounds.isValid() )
	{
		return false;
	}

	// Get TransformSystem for proper hierarchical transforms
	auto *transformSystem = m_systemManager.getSystem<systems::TransformSystem>();
	if ( !transformSystem )
	{
		// TransformSystem is required for proper hierarchical transforms
		return false;
	}

	// Get world transform using TransformSystem for proper hierarchical transforms
	const auto worldMatrix = transformSystem->getWorldTransform( scene, entity );

	// For simplicity, we'll use the AABB in world space
	// In a more advanced implementation, we could transform the ray to local space
	// and test against the local AABB, or transform the AABB corners to world space

	// Transform bounds center and size to world space
	const auto localCenter = bounds.center();
	const auto localSize = bounds.size();

	// Simple approximation: transform center to world space and keep original size
	// This isn't perfectly accurate for rotated/scaled objects but works for basic cases
	const Vec4<> worldCenter4 = worldMatrix * Vec4<>{ localCenter.x, localCenter.y, localCenter.z, 1.0f };
	const Vec3<> worldCenter = worldCenter4.xyz();

	// Create world space AABB (simplified)
	const Vec3<> worldMin = worldCenter - localSize * 0.5f;
	const Vec3<> worldMax = worldCenter + localSize * 0.5f;

	return rayAABBIntersection( rayOrigin, rayDirection, worldMin, worldMax, hitDistance );
}

bool PickingSystem::testEntityMesh( ecs::Scene &scene, ecs::Entity entity, const Vec3<> &rayOrigin, const Vec3<> &rayDirection, HitResult &result ) const
{
	// For now, we'll just use the bounds test result
	// In a more advanced implementation, we would test against actual mesh triangles
	auto *const meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	auto *const transform = scene.getComponent<components::Transform>( entity );

	if ( !meshRenderer || !transform )
	{
		return false;
	}

	float hitDistance = 0.0f;
	if ( testEntityBounds( scene, entity, rayOrigin, rayDirection, hitDistance ) )
	{
		result.distance = hitDistance;
		result.worldPosition = rayOrigin + rayDirection * hitDistance;
		result.normal = Vec3<>{ 0.0f, 0.0f, 1.0f }; // Placeholder normal
		result.primitiveIndex = 0;
		return true;
	}

	return false;
}

HitResult PickingSystem::pickFromScreen( ecs::Scene &scene,
	const editor::Viewport &viewport,
	const Vec2<> &viewportPos ) const
{
	// Get ray from viewport coordinates (relative to viewport, not window)
	const auto viewportRay = viewport.getPickingRay( viewportPos );

	// Use existing raycast functionality
	return raycast( scene, viewportRay.origin, viewportRay.direction, viewportRay.length );
}

std::vector<HitResult> PickingSystem::pickAllFromScreen( ecs::Scene &scene,
	const editor::Viewport &viewport,
	const Vec2<> &viewportPos ) const
{
	// Get ray from viewport coordinates (relative to viewport, not window)
	const auto viewportRay = viewport.getPickingRay( viewportPos );

	// Use existing raycastAll functionality
	return raycastAll( scene, viewportRay.origin, viewportRay.direction, viewportRay.length );
}

} // namespace picking
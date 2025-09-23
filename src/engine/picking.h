#pragma once

#include <vector>

#include "engine/math/vec.h"
#include "runtime/entity.h"
#include "runtime/systems.h"

namespace editor
{
class Viewport;
}

namespace picking
{

// Ray-object intersection result
struct HitResult
{
	bool hit = false;
	float distance = std::numeric_limits<float>::max();
	ecs::Entity entity{};
	math::Vec3<> worldPosition{};
	math::Vec3<> localPosition{};
	math::Vec3<> normal{};
	size_t primitiveIndex = 0; // Which primitive in mesh was hit

	// Comparison for distance sorting
	bool operator<( const HitResult &other ) const
	{
		return distance < other.distance;
	}
};

// Core picking system
class PickingSystem
{
public:
	explicit PickingSystem( systems::SystemManager &systemManager );

	// Ray casting against all renderable entities
	HitResult raycast( ecs::Scene &scene,
		const math::Vec3<> &rayOrigin,
		const math::Vec3<> &rayDirection,
		float maxDistance = 1000.0f ) const;

	// Get all entities intersecting ray (sorted by distance)
	std::vector<HitResult> raycastAll( ecs::Scene &scene,
		const math::Vec3<> &rayOrigin,
		const math::Vec3<> &rayDirection,
		float maxDistance = 1000.0f ) const;

	// Viewport integration for mouse picking
	HitResult pickFromScreen( ecs::Scene &scene,
		const editor::Viewport &viewport,
		const math::Vec2<> &viewportPos ) const;

	std::vector<HitResult> pickAllFromScreen( ecs::Scene &scene,
		const editor::Viewport &viewport,
		const math::Vec2<> &viewportPos ) const;

private:
	systems::SystemManager &m_systemManager;

	// Core intersection testing
	bool testEntityBounds( ecs::Scene &scene, ecs::Entity entity, const math::Vec3<> &rayOrigin, const math::Vec3<> &rayDirection, float &hitDistance ) const;

	bool testEntityMesh( ecs::Scene &scene, ecs::Entity entity, const math::Vec3<> &rayOrigin, const math::Vec3<> &rayDirection, HitResult &result ) const;
};

} // namespace picking

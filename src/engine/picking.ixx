export module engine.picking;

import engine.math_3d;
import engine.bounding_volumes;
import engine.vec;
import engine.matrix;
import runtime.ecs;
import runtime.components;
import runtime.entity;
import runtime.systems;
import std;

// Forward declarations for viewport integration
namespace editor
{
class Viewport;
}

export namespace picking
{

using namespace math;

// Ray-object intersection result
export struct HitResult
{
	bool hit = false;
	float distance = std::numeric_limits<float>::max();
	ecs::Entity entity{};
	Vec3<> worldPosition{};
	Vec3<> localPosition{};
	Vec3<> normal{};
	size_t primitiveIndex = 0; // Which primitive in mesh was hit

	// Comparison for distance sorting
	bool operator<( const HitResult &other ) const
	{
		return distance < other.distance;
	}
};

// Core picking system
export class PickingSystem
{
public:
	explicit PickingSystem( systems::SystemManager &systemManager );

	// Ray casting against all renderable entities
	HitResult raycast( ecs::Scene &scene,
		const Vec3<> &rayOrigin,
		const Vec3<> &rayDirection,
		float maxDistance = 1000.0f ) const;

	// Get all entities intersecting ray (sorted by distance)
	std::vector<HitResult> raycastAll( ecs::Scene &scene,
		const Vec3<> &rayOrigin,
		const Vec3<> &rayDirection,
		float maxDistance = 1000.0f ) const;

	// Viewport integration for mouse picking
	HitResult pickFromScreen( ecs::Scene &scene,
		const editor::Viewport &viewport,
		const Vec2<> &screenPos ) const;

	std::vector<HitResult> pickAllFromScreen( ecs::Scene &scene,
		const editor::Viewport &viewport,
		const Vec2<> &screenPos ) const;

private:
	systems::SystemManager &m_systemManager;

	// Core intersection testing
	bool testEntityBounds( ecs::Scene &scene, ecs::Entity entity, const Vec3<> &rayOrigin, const Vec3<> &rayDirection, float &hitDistance ) const;

	bool testEntityMesh( ecs::Scene &scene, ecs::Entity entity, const Vec3<> &rayOrigin, const Vec3<> &rayDirection, HitResult &result ) const;
};

} // namespace picking
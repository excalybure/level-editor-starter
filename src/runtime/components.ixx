export module runtime.components;

import engine.math;
import engine.matrix;
import engine.vec;
import engine.bounding_box_3d;
import runtime.entity;
import <string>;
import <vector>;
import <memory>;

// Forward declaration for MeshGPU to avoid circular dependencies
namespace engine::gpu
{
class MeshGPU;
}

export namespace components
{

// Core transform component
struct Transform
{
	math::Vec3<> position{ 0.0f, 0.0f, 0.0f };
	math::Vec3<> rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles (radians)
	math::Vec3<> scale{ 1.0f, 1.0f, 1.0f };

	// Cached matrices (marked mutable for lazy evaluation)
	mutable math::Mat4<> localMatrix;
	mutable math::Mat4<> worldMatrix;
	mutable bool localMatrixDirty = true;
	mutable bool worldMatrixDirty = true;

	// Get local transformation matrix
	const math::Mat4<> &getLocalMatrix() const
	{
		if ( localMatrixDirty )
		{
			// Create translation matrix
			const math::Mat4<> translationMatrix = math::Mat4<>::translation( position.x, position.y, position.z );

			// Create rotation matrices for each axis
			const math::Mat4<> rotationX = math::Mat4<>::rotationX( rotation.x );
			const math::Mat4<> rotationY = math::Mat4<>::rotationY( rotation.y );
			const math::Mat4<> rotationZ = math::Mat4<>::rotationZ( rotation.z );

			// Combine rotations (order: Z * Y * X)
			const math::Mat4<> rotationMatrix = rotationZ * rotationY * rotationX;

			// Create scale matrix
			const math::Mat4<> scaleMatrix = math::Mat4<>::scale( scale.x, scale.y, scale.z );

			// Combine: Translation * Rotation * Scale
			localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
			localMatrixDirty = false;
		}
		return localMatrix;
	}

	// Get world transformation matrix
	const math::Mat4<> &getWorldMatrix() const
	{
		return worldMatrix;
	}

	// Mark matrices as dirty
	void markDirty() const
	{
		localMatrixDirty = true;
		worldMatrixDirty = true;
	}
};

// Name component for editor display
struct Name
{
	std::string name = "Unnamed";

	Name() = default;
	Name( const std::string &n ) : name( n ) {}
};

// Visibility control
struct Visible
{
	bool visible = true;
	bool castShadows = true;
	bool receiveShadows = true;
};

// Renderable mesh component
struct MeshRenderer
{
	std::shared_ptr<engine::gpu::MeshGPU> gpuBuffers;
	math::BoundingBox3Df bounds; // Local space bounding box
	float lodBias = 0.0f;		 // Level of detail bias for rendering

	MeshRenderer() = default;
	MeshRenderer( std::shared_ptr<engine::gpu::MeshGPU> buffers ) : gpuBuffers( std::move( buffers ) ) {}
};

// Selection state for editor
struct Selected
{
	bool selected = false;
	math::Vec3<> highlightColor{ 1.0f, 0.8f, 0.2f };
};

// Component concept - relaxed to allow more practical component types
export template <class T>
concept Component = std::is_copy_constructible_v<T> && std::is_move_constructible_v<T> && std::is_destructible_v<T>;

} // namespace components

// Ensure all components satisfy the Component concept
static_assert( components::Component<components::Transform> );
static_assert( components::Component<components::Name> );
static_assert( components::Component<components::Visible> );
static_assert( components::Component<components::MeshRenderer> );
static_assert( components::Component<components::Selected> );

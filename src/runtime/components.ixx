export module runtime.components;

import engine.math;
import engine.matrix;
import engine.vec;
import engine.bounding_box_3d;
import engine.gpu.mesh_gpu;
import engine.assets;
import runtime.entity;
import runtime.time;
import <string>;
import <vector>;
import <memory>;

export namespace components
{

// Core transform component
struct Transform
{
	math::Vec3<> position{ 0.0f, 0.0f, 0.0f };
	math::Vec3<> rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles (radians)
	math::Vec3<> scale{ 1.0f, 1.0f, 1.0f };

	// Cached local matrix (marked mutable for lazy evaluation)
	mutable math::Mat4<> localMatrix;
	mutable bool localMatrixDirty = true;

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

	// Mark local matrix as dirty
	// Note: World matrices are managed by TransformSystem
	void markDirty() const
	{
		localMatrixDirty = true;
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
	assets::MeshHandle meshHandle; // Handle to the source mesh asset
	std::shared_ptr<engine::gpu::MeshGPU> gpuMesh;
	math::BoundingBox3Df bounds; // Local space bounding box
	float lodBias = 0.0f;		 // Level of detail bias for rendering

	MeshRenderer() = default;
	MeshRenderer( assets::MeshHandle handle ) : meshHandle( handle ) {}
};

// Selection state for editor
struct Selected
{
	bool isPrimary = false;									  // Primary selection for gizmo operations
	float selectionTime = 0.0f;								  // When selected (for animation/UI effects)
	math::Vec4<> highlightColor = { 1.0f, 0.6f, 0.0f, 1.0f }; // Selection outline color

	Selected() : selectionTime( runtime::time::getCurrentTime() ) {}
	Selected( bool primary ) : isPrimary( primary ), selectionTime( runtime::time::getCurrentTime() ) {}
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

#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "math/math.h"
#include "math/matrix.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/gpu/mesh_gpu.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/material_system/compiled_material.h"
#include "systems.h"

namespace camera
{
class Camera;
}
namespace engine::gpu
{
class MaterialGPU;
}

namespace graphics
{
class GraphicsContext;
} // namespace graphics

namespace dx12
{
class Device;
} // namespace dx12

namespace systems
{

// Constant buffer structures matching unlit.hlsl shader expectations
struct ObjectConstants
{
	math::Mat4f worldMatrix;
	math::Mat4f normalMatrix;

	ObjectConstants() = default;
};

class MeshRenderingSystem : public System
{
public:
	// Constructor with GraphicsContext and optional SystemManager for world transform support
	// Pass nullptr for systemManager in tests that don't need hierarchy support
	MeshRenderingSystem( graphics::GraphicsContext &graphicsContext,
		systems::SystemManager *systemManager );
	void update( ecs::Scene &scene, float deltaTime ) override;
	void render( ecs::Scene &scene, const camera::Camera &camera, ID3D12GraphicsCommandList *commandList, D3D12_GPU_VIRTUAL_ADDRESS frameConstantsGPUAddress = 0 );

	// Public for testing
	math::Mat4f calculateMVPMatrix(
		const components::Transform &transform,
		const camera::Camera &camera );

	// Render entity using world transform from TransformSystem (supports hierarchy)
	void renderEntity( ecs::Scene &scene, ecs::Entity entity, const camera::Camera &camera, ID3D12GraphicsCommandList *commandList );

private:
	graphics::GraphicsContext &m_graphicsContext;
	systems::SystemManager *m_systemManager;

	// Default material instance for mesh rendering
	std::unique_ptr<graphics::material_system::CompiledMaterial> m_defaultCompiledMaterial;

	// Per-frame storage for object constant buffers to keep them alive until GPU execution
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_frameConstantBuffers;

	// Clear per-frame resources (called at start of render)
	void clearFrameResources();
};

} // namespace systems

#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "math/math.h"
#include "math/matrix.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/material_system/material_instance.h"
#include "systems.h"

namespace camera
{
class Camera;
}
namespace engine::gpu
{
class MaterialGPU;
}

namespace graphics::material_system
{
class MaterialSystem;
}

namespace graphics
{
class SamplerManager;
class ImmediateRenderer;
} // namespace graphics

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
	// Constructor with MaterialSystem, ShaderManager, SamplerManager and optional SystemManager for world transform support
	// Pass nullptr for systemManager in tests that don't need hierarchy support
	MeshRenderingSystem( graphics::ImmediateRenderer &renderer,
		graphics::material_system::MaterialSystem *materialSystem,
		std::shared_ptr<shader_manager::ShaderManager> shaderManager,
		graphics::SamplerManager &samplerManager,
		systems::SystemManager *systemManager );
	void update( ecs::Scene &scene, float deltaTime ) override;
	void render( ecs::Scene &scene, const camera::Camera &camera );

	// Public for testing
	math::Mat4f calculateMVPMatrix(
		const components::Transform &transform,
		const camera::Camera &camera );

	// Render entity using world transform from TransformSystem (supports hierarchy)
	void renderEntity( ecs::Scene &scene, ecs::Entity entity, const camera::Camera &camera );

private:
	graphics::ImmediateRenderer &m_renderer;
	graphics::material_system::MaterialSystem *m_materialSystem;
	std::shared_ptr<shader_manager::ShaderManager> m_shaderManager;
	graphics::SamplerManager &m_samplerManager;
	systems::SystemManager *m_systemManager;

	// Default material instance for mesh rendering
	std::unique_ptr<graphics::material_system::MaterialInstance> m_defaultMaterialInstance;

	// Per-frame storage for object constant buffers to keep them alive until GPU execution
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_frameConstantBuffers;

	// Clear per-frame resources (called at start of render)
	void clearFrameResources();
};

} // namespace systems

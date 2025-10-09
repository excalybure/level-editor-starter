#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "math/math.h"
#include "math/matrix.h"
#include "engine/shader_manager/shader_manager.h"
#include "systems.h"

namespace renderer
{
class Renderer;
}

namespace camera
{
class Camera;
}
namespace engine::gpu
{
class MaterialGPU;
}

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
	// Constructor with ShaderManager and optional SystemManager for world transform support
	// Pass nullptr for systemManager in tests that don't need hierarchy support
	MeshRenderingSystem( renderer::Renderer &renderer,
		std::shared_ptr<shader_manager::ShaderManager> shaderManager,
		systems::SystemManager *systemManager );
	void update( ecs::Scene &scene, float deltaTime ) override;
	void render( ecs::Scene &scene, const camera::Camera &camera );

	// Public for testing
	math::Mat4f calculateMVPMatrix(
		const components::Transform &transform,
		const camera::Camera &camera );

	// Render entity using world transform from TransformSystem (supports hierarchy)
	void renderEntity( ecs::Scene &scene, ecs::Entity entity, const camera::Camera &camera );

	// Pipeline state management for materials
	ID3D12PipelineState *getMaterialPipelineState( const engine::gpu::MaterialGPU &material );

	// Root signature management - must be called before binding any parameters
	void setRootSignature( ID3D12GraphicsCommandList *commandList );

private:
	renderer::Renderer &m_renderer;
	std::shared_ptr<shader_manager::ShaderManager> m_shaderManager;
	systems::SystemManager *m_systemManager = nullptr;

	// Shader handles for the unlit shader
	shader_manager::ShaderHandle m_vertexShaderHandle = shader_manager::INVALID_SHADER_HANDLE;
	shader_manager::ShaderHandle m_pixelShaderHandle = shader_manager::INVALID_SHADER_HANDLE;
	shader_manager::CallbackHandle m_callbackHandle = shader_manager::INVALID_CALLBACK_HANDLE;

	// Root signature for mesh rendering (shared by all materials)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	// Pipeline state cache for materials
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pipelineStateCache;

	// Helper methods for root signature and pipeline state management
	void createRootSignature();
	bool registerShaders();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> createMaterialPipelineState( const engine::gpu::MaterialGPU &material );
};

} // namespace systems

// Global module fragment for Windows headers
module;

#include <d3d12.h>
#include <wrl.h>

export module runtime.mesh_rendering_system;

import runtime.systems;
import runtime.ecs;
import runtime.components;
import engine.renderer;
import engine.camera;
import engine.gpu.mesh_gpu;
import engine.matrix;
import engine.vec;
import platform.dx12;
import engine.shader_manager;
import std;

// Forward declarations
namespace engine::gpu
{
class MaterialGPU;
}

export namespace runtime::systems
{

// Constant buffer structures matching unlit.hlsl shader expectations
struct ObjectConstants
{
	math::Mat4<> worldMatrix;
	math::Mat4<> normalMatrix;

	ObjectConstants() = default;
};

export class MeshRenderingSystem : public ::systems::System
{
public:
	// New constructor with ShaderManager (preferred)
	MeshRenderingSystem( renderer::Renderer &renderer, std::shared_ptr<shader_manager::ShaderManager> shaderManager );

	// Legacy constructor for backward compatibility (deprecated)
	MeshRenderingSystem( renderer::Renderer &renderer );

	void update( ecs::Scene &scene, float deltaTime ) override;
	void render( ecs::Scene &scene, const camera::Camera &camera );

	// Public for testing
	math::Mat4<> calculateMVPMatrix(
		const components::Transform &transform,
		const camera::Camera &camera );

	// Public for testing
	void renderEntity( const components::Transform &transform,
		const components::MeshRenderer &meshRenderer,
		const camera::Camera &camera );

	// Pipeline state management for materials
	ID3D12PipelineState *getMaterialPipelineState( const engine::gpu::MaterialGPU &material );

	// Root signature management - must be called before binding any parameters
	void setRootSignature( ID3D12GraphicsCommandList *commandList );

private:
	renderer::Renderer &m_renderer;
	std::shared_ptr<shader_manager::ShaderManager> m_shaderManager;

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

} // namespace runtime::systems
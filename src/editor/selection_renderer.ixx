module;

#include <d3d12.h>
#include <wrl.h>

export module editor.selection_renderer;

import runtime.ecs;
import runtime.entity;
import runtime.components;
import platform.dx12;
import engine.math;
import engine.vec;
import engine.matrix;
import engine.renderer;
import engine.shader_manager;
import std;

export namespace editor
{

// Selection visual styles
export struct SelectionStyle
{
	math::Vec4<> selectedColor{ 1.0f, 0.6f, 0.0f, 1.0f };	// Orange
	math::Vec4<> primaryColor{ 1.0f, 1.0f, 0.0f, 1.0f };	// Yellow
	math::Vec4<> hoveredColor{ 0.8f, 0.8f, 1.0f, 0.5f };	// Light blue
	math::Vec4<> rectSelectColor{ 0.0f, 0.8f, 1.0f, 0.3f }; // Cyan
	float outlineWidth = 2.0f;
	bool animateSelection = true;
	float animationSpeed = 2.0f;
};

// Selection rendering system
export class SelectionRenderer
{
public:
	SelectionRenderer( dx12::Device &device, shader_manager::ShaderManager &shaderManager );
	~SelectionRenderer() = default;

	// No copy/move for now
	SelectionRenderer( const SelectionRenderer & ) = delete;
	SelectionRenderer &operator=( const SelectionRenderer & ) = delete;

	void render( ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix );

	void renderSelectionOutlines( ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix );

	void renderHoverHighlight( ecs::Entity hoveredEntity,
		ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix );

	void renderRectSelection( const math::Vec2<> &startPos,
		const math::Vec2<> &endPos,
		ID3D12GraphicsCommandList *commandList,
		const math::Vec2<> &viewportSize );

	// Configuration
	SelectionStyle &getStyle() { return m_style; }
	const SelectionStyle &getStyle() const { return m_style; }

private:
	dx12::Device &m_device;
	shader_manager::ShaderManager &m_shaderManager;
	SelectionStyle m_style;

	// Shader handles
	shader_manager::ShaderHandle m_outlineVertexShader = shader_manager::INVALID_SHADER_HANDLE;
	shader_manager::ShaderHandle m_outlinePixelShader = shader_manager::INVALID_SHADER_HANDLE;
	shader_manager::ShaderHandle m_rectVertexShader = shader_manager::INVALID_SHADER_HANDLE;
	shader_manager::ShaderHandle m_rectPixelShader = shader_manager::INVALID_SHADER_HANDLE;

	// Rendering resources
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_outlinePipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_rectPipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	void *m_constantBufferData = nullptr;

	// Rectangle rendering buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> m_rectVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_rectIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_rectVertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW m_rectIndexBufferView{};

	// Constant buffer data for outline rendering
	struct OutlineConstants
	{
		math::Mat4<> worldViewProj;
		math::Vec4<> outlineColor;
		math::Vec4<> screenParams; // width, height, outlineWidth, time
	};

	void setupRenderingResources();
	void createRootSignature();
	void createOutlinePipelineState();
	void createRectPipelineState();
	void createConstantBuffer();
	void createRectVertexBuffer();

	void renderEntityOutline( ecs::Entity entity,
		ecs::Scene &scene,
		const math::Vec4<> &color,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix );

	// Helper to get entity world matrix
	math::Mat4<> getEntityWorldMatrix( ecs::Entity entity, ecs::Scene &scene ) const;

	// Helper to check if entity has renderable mesh
	bool entityHasRenderableMesh( ecs::Entity entity, ecs::Scene &scene ) const;

	// Animation helpers
	float getAnimationTime() const;
	math::Vec4<> animateColor( const math::Vec4<> &baseColor, float time ) const;
};

} // namespace editor
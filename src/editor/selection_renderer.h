#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "math/vec.h"
#include "math/matrix.h"
#include "graphics/shader_manager/shader_manager.h"
#include "graphics/material_system/material_system.h"
#include "graphics/material_system/material_instance.h"
#include "platform/dx12/dx12_device.h"
#include "runtime/entity.h"

// Forward declarations
namespace ecs
{
class Scene;
}

namespace systems
{
class SystemManager;
}

namespace editor
{

// Selection visual styles
struct SelectionStyle
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
class SelectionRenderer
{
public:
	SelectionRenderer( dx12::Device &device, graphics::material_system::MaterialSystem *materialSystem, shader_manager::ShaderManager &shaderManager, systems::SystemManager *systemManager = nullptr );
	~SelectionRenderer() = default;

	// No copy/move for now
	SelectionRenderer( const SelectionRenderer & ) = delete;
	SelectionRenderer &operator=( const SelectionRenderer & ) = delete;

	void render( ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		const math::Vec2<> &viewportSize );

	void renderSelectionOutlines( ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		const math::Vec2<> &viewportSize );

	void renderHoverHighlight( ecs::Entity hoveredEntity,
		ecs::Scene &scene,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		const math::Vec2<> &viewportSize );

	void renderRectSelection( const math::Vec2<> &startPos,
		const math::Vec2<> &endPos,
		ID3D12GraphicsCommandList *commandList,
		const math::Vec2<> &viewportSize );

	// Configuration
	SelectionStyle &getStyle() { return m_style; }
	const SelectionStyle &getStyle() const { return m_style; }

private:
	dx12::Device &m_device;
	graphics::material_system::MaterialSystem *m_materialSystem = nullptr;
	shader_manager::ShaderManager &m_shaderManager;
	systems::SystemManager *m_systemManager = nullptr;
	SelectionStyle m_style;

	// Material instances for rendering
	std::unique_ptr<graphics::material_system::MaterialInstance> m_outlineMaterialInstance;
	std::unique_ptr<graphics::material_system::MaterialInstance> m_rectMaterialInstance;

	// Constant buffer for both outline and rect rendering
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
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
	void createConstantBuffer();
	void createRectVertexBuffer();

	void renderEntityOutline( ecs::Entity entity,
		ecs::Scene &scene,
		const math::Vec4<> &color,
		ID3D12GraphicsCommandList *commandList,
		const math::Mat4<> &viewMatrix,
		const math::Mat4<> &projMatrix,
		const math::Vec2<> &viewportSize );

	// Helper to get entity world matrix
	math::Mat4<> getEntityWorldMatrix( ecs::Entity entity, ecs::Scene &scene ) const;

	// Helper to check if entity has renderable mesh
	bool entityHasRenderableMesh( ecs::Entity entity, ecs::Scene &scene ) const;

	// Animation helpers
	float getAnimationTime() const;
	math::Vec4<> animateColor( const math::Vec4<> &baseColor, float time ) const;
};

} // namespace editor

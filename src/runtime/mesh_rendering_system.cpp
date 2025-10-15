#include "runtime/mesh_rendering_system.h"
#include "graphics/renderer/immediate_renderer.h"
#include "graphics/gpu/material_gpu.h"
#include "graphics/material_system/material_instance.h"
#include "core/console.h"
#include "runtime/ecs.h"
#include "runtime/components.h"
#include "engine/camera/camera.h"
#include "graphics/gpu/mesh_gpu.h"
#include "engine/assets/assets.h"

#include <d3d12.h>
#include <wrl.h>
#include <cstring>

namespace
{
/**
 * Checks if an entity is effectively visible by walking up the parent chain.
 * An entity is effectively visible only if it and all its ancestors have visible=true.
 * 
 * @param scene The scene containing the entity hierarchy
 * @param entity The entity to check
 * @return true if the entity and all ancestors are visible, false otherwise
 */
bool isEffectivelyVisible( const ecs::Scene &scene, ecs::Entity entity )
{
	ecs::Entity current = entity;
	while ( current.isValid() )
	{
		const auto *visible = scene.getComponent<components::Visible>( current );
		if ( !visible || !visible->visible )
		{
			return false;
		}
		current = scene.getParent( current );
	}
	return true;
}
} // anonymous namespace

namespace systems
{

MeshRenderingSystem::MeshRenderingSystem( renderer::ImmediateRenderer &renderer, graphics::material_system::MaterialSystem *materialSystem, std::shared_ptr<shader_manager::ShaderManager> shaderManager, systems::SystemManager *systemManager )
	: m_renderer( renderer ), m_materialSystem( materialSystem ), m_shaderManager( shaderManager ), m_systemManager( systemManager )
{
	// Phase 2: Create default MaterialInstance if MaterialSystem available
	if ( m_materialSystem )
	{
		m_defaultMaterialInstance = std::make_unique<graphics::material_system::MaterialInstance>(
			&renderer.getDevice(),
			m_materialSystem,
			"mesh_unlit" );

		if ( !m_defaultMaterialInstance->isValid() )
		{
			console::error( "MeshRenderingSystem: Failed to create default material instance" );
		}
		else if ( !m_defaultMaterialInstance->hasPass( "forward" ) )
		{
			console::error( "MeshRenderingSystem: Material 'mesh_unlit' does not have 'forward' pass" );
		}
		else
		{
			console::info( "MeshRenderingSystem: Successfully created MaterialInstance for 'mesh_unlit'" );
		}
	}
	else
	{
		console::warning( "MeshRenderingSystem: No MaterialSystem provided - system may not render correctly" );
	}

	if ( !systemManager )
	{
		console::warning( "MeshRenderingSystem: Created without SystemManager - parent-child hierarchy transforms will not work correctly" );
	}
}

void MeshRenderingSystem::update( ecs::Scene &scene, float deltaTime )
{
	// The mesh rendering system doesn't need to update per frame
	// Rendering happens in the render() method
}

void MeshRenderingSystem::render( ecs::Scene &scene, const camera::Camera &camera )
{
	// Clear previous frame's constant buffers
	clearFrameResources();

	// Get command context for binding
	auto *commandContext = m_renderer.getCommandContext();
	if ( !commandContext )
	{
		return;
	}

	auto *commandList = commandContext->get();
	if ( !commandList )
	{
		return;
	}

	// Phase 3: Setup material for rendering using MaterialInstance
	if ( !m_defaultMaterialInstance || !m_defaultMaterialInstance->isValid() )
	{
		console::error( "MeshRenderingSystem: No valid MaterialInstance available for rendering" );
		return;
	}

	if ( !m_defaultMaterialInstance->setupCommandList( commandList, "forward" ) )
	{
		console::warning( "MeshRenderingSystem: Failed to setup MaterialInstance for rendering" );
		return;
	}

	// Iterate through all entities to find those with both MeshRenderer and Transform components
	const auto allEntities = scene.getAllEntities();
	for ( const auto entity : allEntities )
	{
		if ( !entity.isValid() )
		{
			continue;
		}

		// Check if entity has both required components
		const auto *transform = scene.getComponent<components::Transform>( entity );
		const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );

		if ( transform && meshRenderer )
		{
			// Check hierarchical visibility - skip rendering if entity or any ancestor is invisible
			if ( !isEffectivelyVisible( scene, entity ) )
			{
				// Entity or one of its ancestors is marked as invisible, skip rendering
				continue;
			}

			// Use the new hierarchy-aware renderEntity method
			renderEntity( scene, entity, camera );
		}
	}
}

void MeshRenderingSystem::renderEntity( ecs::Scene &scene, ecs::Entity entity, const camera::Camera &camera )
{
	// Get components
	const auto *meshRenderer = scene.getComponent<components::MeshRenderer>( entity );
	if ( !meshRenderer )
	{
		return;
	}

	// Early return if no GPU mesh available
	if ( !meshRenderer->gpuMesh )
	{
		// This is expected for entities that haven't been processed by GPUResourceManager yet
		return;
	}

	// Get command context for direct GPU commands
	auto *commandContext = m_renderer.getCommandContext();
	if ( !commandContext )
	{
		// No active command context (e.g., during headless tests)
		return;
	}

	auto *commandList = commandContext->get();
	if ( !commandList )
	{
		return;
	}

	// Calculate world matrix using TransformSystem if available, otherwise use local transform
	math::Mat4<> worldMatrix;
	if ( m_systemManager )
	{
		auto *transformSystem = m_systemManager->getSystem<systems::TransformSystem>();
		if ( transformSystem )
		{
			// Use world transform from TransformSystem (supports parent-child hierarchy)
			worldMatrix = transformSystem->getWorldTransform( scene, entity );
		}
		else
		{
			// Fallback to local transform if TransformSystem not available
			const auto *transform = scene.getComponent<components::Transform>( entity );
			worldMatrix = transform ? transform->getLocalMatrix() : math::Mat4<>::identity();
		}
	}
	else
	{
		// Fallback to local transform if SystemManager not available
		const auto *transform = scene.getComponent<components::Transform>( entity );
		worldMatrix = transform ? transform->getLocalMatrix() : math::Mat4<>::identity();
	}

	// Calculate object constants for this entity
	ObjectConstants objectConstants;
	// HLSL expects column-major matrices when using mul(matrix, vector)
	// Our C++ matrices are row-major, so transpose before sending to GPU
	objectConstants.worldMatrix = worldMatrix.transpose();
	// For normal matrix, we need the inverse transpose of the world matrix
	// This is required for correct normal transformation under non-uniform scaling
	// Normal matrix = transpose(inverse(worldMatrix)) for transforming normals
	objectConstants.normalMatrix = worldMatrix.inverse().transpose();

	// Create temporary upload buffer for object constants
	// Note: This is a workaround for the root signature mismatch
	// The shader declares b1 as cbuffer (CBV) but code was using root constants
	// TODO: Optimize with a ring buffer or per-frame constant buffer pool
	const UINT constantBufferSize = ( sizeof( ObjectConstants ) + 255 ) & ~255; // Align to 256 bytes

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = constantBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
	HRESULT hr = m_renderer.getDevice().get()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &constantBuffer ) );

	if ( FAILED( hr ) )
	{
		console::error( "MeshRenderingSystem: Failed to create object constants buffer" );
		return;
	}

	// Map and copy data
	void *mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	hr = constantBuffer->Map( 0, &readRange, &mappedData );
	if ( SUCCEEDED( hr ) )
	{
		memcpy( mappedData, &objectConstants, sizeof( ObjectConstants ) );
		constantBuffer->Unmap( 0, nullptr );
	}

	// Store constant buffer for this frame to keep it alive until GPU execution
	m_frameConstantBuffers.push_back( constantBuffer );

	// Bind object constants to register b1 using CBV (not root constants)
	commandList->SetGraphicsRootConstantBufferView( 1, constantBuffer->GetGPUVirtualAddress() );

	const auto &gpuMesh = *meshRenderer->gpuMesh;
	if ( !gpuMesh.isValid() )
	{
		return;
	}

	// Iterate through all primitives in the mesh
	for ( std::uint32_t i = 0; i < gpuMesh.getPrimitiveCount(); ++i )
	{
		const auto &primitive = gpuMesh.getPrimitive( i );
		if ( !primitive.isValid() )
		{
			continue;
		}

		// Bind primitive GPU buffers and material (material binding is handled by primitive)
		primitive.bindForRendering( commandList );

		// Phase 3: Material PSO is already set via MaterialInstance in render() method
		// No need to call getMaterialPipelineState() per primitive anymore

		// Issue the draw call
		if ( primitive.hasIndexBuffer() )
		{
			// Indexed drawing
			commandList->DrawIndexedInstanced( primitive.getIndexCount(), 1, 0, 0, 0 );
		}
		else
		{
			// Non-indexed drawing
			commandList->DrawInstanced( primitive.getVertexCount(), 1, 0, 0 );
		}
	}
}

math::Mat4f MeshRenderingSystem::calculateMVPMatrix(
	const components::Transform &transform,
	const camera::Camera &camera )
{
	// Get model matrix from transform
	const auto modelMatrix = transform.getLocalMatrix();

	// Get view matrix from camera
	const auto viewMatrix = camera.getViewMatrix();

	// Get projection matrix from camera (assuming 16:9 aspect ratio for now)
	const auto projectionMatrix = camera.getProjectionMatrix( 16.0f / 9.0f );

	// Calculate MVP matrix: Projection * View * Model
	return projectionMatrix * viewMatrix * modelMatrix;
}

void MeshRenderingSystem::clearFrameResources()
{
	// Clear constant buffers from previous frame
	// This is safe because the GPU has finished with the previous frame's command list
	m_frameConstantBuffers.clear();
}

} // namespace systems
module editor.selection_renderer;

import runtime.console;
import runtime.time;
import std;

namespace editor
{

namespace
{
// Outline vertex shader source
const char *kOutlineVertexShader = R"(
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float3 normal : NORMAL;
};

cbuffer OutlineConstants : register(b0) {
    float4x4 worldViewProj;
    float4 outlineColor;
    float4 screenParams; // width, height, outlineWidth, time
};

VSOutput main(VSInput input) {
    VSOutput output;
    
    // Expand vertices along normals for outline effect
    float3 expandedPos = input.position + input.normal * screenParams.z * 0.01f;
    
    output.position = mul(float4(expandedPos, 1.0f), worldViewProj);
    output.worldPos = input.position;
    output.normal = input.normal;
    
    return output;
}
)";

// Outline pixel shader source
const char *kOutlinePixelShader = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POS;
    float3 normal : NORMAL;
};

cbuffer OutlineConstants : register(b0) {
    float4x4 worldViewProj;
    float4 outlineColor;
    float4 screenParams; // width, height, outlineWidth, time
};

float4 main(PSInput input) : SV_Target {
    float4 color = outlineColor;
    
    // Simple animation pulse effect
    float pulse = sin(screenParams.w * 3.14159f) * 0.2f + 0.8f;
    color.rgb *= pulse;
    
    return color;
}
)";

// Rectangle vertex shader source
const char *kRectVertexShader = R"(
struct VSInput {
    float2 position : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.uv = input.position * 0.5f + 0.5f; // Convert to 0-1 range
    return output;
}
)";

// Rectangle pixel shader source
const char *kRectPixelShader = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer RectConstants : register(b0) {
    float4 rectColor;
    float4 rectBounds; // minX, minY, maxX, maxY
};

float4 main(PSInput input) : SV_Target {
    float2 center = (rectBounds.xy + rectBounds.zw) * 0.5f;
    float2 size = rectBounds.zw - rectBounds.xy;
    
    // Create border effect
    float2 edgeDist = min(input.uv - rectBounds.xy, rectBounds.zw - input.uv);
    float borderWidth = 0.02f;
    float edge = min(edgeDist.x, edgeDist.y);
    
    float4 color = rectColor;
    if (edge < borderWidth) {
        color.a = 0.8f; // Solid border
    } else {
        color.a = 0.2f; // Transparent fill
    }
    
    return color;
}
)";

} // anonymous namespace

SelectionRenderer::SelectionRenderer( dx12::Device &device )
	: m_device( device )
{
	setupRenderingResources();
}

void SelectionRenderer::render( ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
{
	if ( !commandList )
	{
		// Headless mode - just return
		return;
	}

	// Render selection outlines for all selected entities
	renderSelectionOutlines( scene, commandList, viewMatrix, projMatrix );
}

void SelectionRenderer::renderSelectionOutlines( ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
{
	if ( !commandList )
	{
		return;
	}

	// Set pipeline state for outline rendering
	if ( m_outlinePipelineState )
	{
		commandList->SetPipelineState( m_outlinePipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	// Iterate through all entities with Selected component
	scene.forEach<components::Selected>( [&]( ecs::Entity entity, const components::Selected &selected ) {
		// Check if entity also has Transform component
		if ( !scene.hasComponent<components::Transform>( entity ) )
		{
			return;
		}

		// Skip entities without renderable meshes
		if ( !entityHasRenderableMesh( entity, scene ) )
		{
			return;
		}

		// Choose color based on selection state
		math::Vec4<> outlineColor = selected.isPrimary ? m_style.primaryColor : m_style.selectedColor;

		// Apply animation if enabled
		if ( m_style.animateSelection )
		{
			outlineColor = animateColor( outlineColor, getAnimationTime() );
		}

		renderEntityOutline( entity, scene, outlineColor, commandList, viewMatrix, projMatrix );
	} );
}

void SelectionRenderer::renderHoverHighlight( ecs::Entity hoveredEntity,
	ecs::Scene &scene,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
{
	if ( !commandList || hoveredEntity == ecs::Entity{} )
	{
		return;
	}

	// Skip if entity is already selected (avoid double highlighting)
	if ( scene.hasComponent<components::Selected>( hoveredEntity ) )
	{
		return;
	}

	// Skip entities without renderable meshes
	if ( !entityHasRenderableMesh( hoveredEntity, scene ) )
	{
		return;
	}

	// Set pipeline state for hover rendering
	if ( m_outlinePipelineState )
	{
		commandList->SetPipelineState( m_outlinePipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	renderEntityOutline( hoveredEntity, scene, m_style.hoveredColor, commandList, viewMatrix, projMatrix );
}

void SelectionRenderer::renderRectSelection( const math::Vec2<> &startPos,
	const math::Vec2<> &endPos,
	ID3D12GraphicsCommandList *commandList )
{
	if ( !commandList )
	{
		return;
	}

	// Set pipeline state for rectangle rendering
	if ( m_rectPipelineState )
	{
		commandList->SetPipelineState( m_rectPipelineState.Get() );
	}

	if ( m_rootSignature )
	{
		commandList->SetGraphicsRootSignature( m_rootSignature.Get() );
	}

	// TODO: Update constant buffer with rectangle bounds and render quad
	// For now, this is a placeholder implementation
	console::info( "Rendering rectangle selection from ({}, {}) to ({}, {})",
		startPos.x,
		startPos.y,
		endPos.x,
		endPos.y );
}

void SelectionRenderer::setupRenderingResources()
{
	try
	{
		createRootSignature();
		createOutlinePipelineState();
		createRectPipelineState();
		createConstantBuffer();
		createRectVertexBuffer();
	}
	catch ( const std::exception &e )
	{
		console::error( "Failed to setup selection renderer resources: {}", e.what() );
	}
}

void SelectionRenderer::createRootSignature()
{
	// For now, create a simple root signature with one constant buffer
	// In a real implementation, this would be more sophisticated
	console::info( "Creating selection renderer root signature" );
}

void SelectionRenderer::createOutlinePipelineState()
{
	// For now, just log creation
	// In a real implementation, this would compile shaders and create PSO
	console::info( "Creating outline pipeline state" );
}

void SelectionRenderer::createRectPipelineState()
{
	// For now, just log creation
	console::info( "Creating rectangle pipeline state" );
}

void SelectionRenderer::createConstantBuffer()
{
	// For now, just log creation
	console::info( "Creating selection renderer constant buffer" );
}

void SelectionRenderer::createRectVertexBuffer()
{
	// For now, just log creation
	console::info( "Creating rectangle vertex buffer" );
}

void SelectionRenderer::renderEntityOutline( ecs::Entity entity,
	ecs::Scene &scene,
	const math::Vec4<> &color,
	ID3D12GraphicsCommandList *commandList,
	const math::Mat4<> &viewMatrix,
	const math::Mat4<> &projMatrix )
{
	if ( !commandList )
	{
		return;
	}

	// Get entity world matrix
	const math::Mat4<> worldMatrix = getEntityWorldMatrix( entity, scene );
	const math::Mat4<> worldViewProj = projMatrix * viewMatrix * worldMatrix;

	// Update constant buffer with outline data
	OutlineConstants constants;
	constants.worldViewProj = worldViewProj;
	constants.outlineColor = color;
	constants.screenParams = math::Vec4<>{ 800.0f, 600.0f, m_style.outlineWidth, getAnimationTime() };

	// For now, just log the rendering
	console::info( "Rendering outline for entity {}.{} with color ({:.2f}, {:.2f}, {:.2f}, {:.2f})",
		entity.id,
		entity.generation,
		color.x,
		color.y,
		color.z,
		color.w );
}

math::Mat4<> SelectionRenderer::getEntityWorldMatrix( ecs::Entity entity, ecs::Scene &scene ) const
{
	if ( auto *transform = scene.getComponent<components::Transform>( entity ) )
	{
		return transform->getLocalMatrix();
	}
	return math::Mat4<>::identity();
}

bool SelectionRenderer::entityHasRenderableMesh( ecs::Entity entity, ecs::Scene &scene ) const
{
	return scene.hasComponent<components::MeshRenderer>( entity );
}

float SelectionRenderer::getAnimationTime() const
{
	return static_cast<float>( runtime::time::getCurrentTime() ) * m_style.animationSpeed;
}

math::Vec4<> SelectionRenderer::animateColor( const math::Vec4<> &baseColor, float time ) const
{
	const float pulse = std::sin( time ) * 0.2f + 0.8f;
	return math::Vec4<>{
		baseColor.x * pulse,
		baseColor.y * pulse,
		baseColor.z * pulse,
		baseColor.w
	};
}

} // namespace editor
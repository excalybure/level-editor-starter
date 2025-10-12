#include "graphics/material_system/material_instance.h"
#include "graphics/material_system/pipeline_builder.h"

namespace graphics::material_system
{

MaterialInstance::MaterialInstance(
	dx12::Device *device,
	MaterialSystem *materialSystem,
	const std::string &materialId )
	: m_device( device ), m_materialSystem( materialSystem )
{
	// AF2: Query MaterialHandle from MaterialSystem using material ID
	m_materialHandle = m_materialSystem->getMaterialHandle( materialId );

	// T302-AF1: Call PipelineBuilder::getRootSignature() if material is valid
	if ( m_materialHandle.isValid() )
	{
		const MaterialDefinition *material = m_materialSystem->getMaterial( m_materialHandle );
		if ( material )
		{
			m_rootSignature = PipelineBuilder::getRootSignature( m_device, *material );
		}
	}
}

MaterialInstance::~MaterialInstance()
{
	// Cleanup will be added as needed
}

bool MaterialInstance::isValid() const
{
	// AF3: Check if material handle is valid and has at least one pass
	if ( !m_materialHandle.isValid() )
	{
		return false;
	}

	const MaterialDefinition *material = m_materialSystem->getMaterial( m_materialHandle );
	if ( !material )
	{
		return false;
	}

	// Check that material has at least one pass
	return !material->passes.empty();
}

bool MaterialInstance::hasPass( const std::string &passName ) const
{
	// AF4: Query material definition and use MaterialDefinition::hasPass()
	const MaterialDefinition *material = m_materialSystem->getMaterial( m_materialHandle );
	if ( !material )
	{
		return false;
	}

	return material->hasPass( passName );
}

const MaterialDefinition *MaterialInstance::getMaterial() const
{
	// AF5: Query from MaterialSystem using handle
	return m_materialSystem->getMaterial( m_materialHandle );
}

const MaterialPass *MaterialInstance::getPass( const std::string &passName ) const
{
	// AF5: Query specific pass from MaterialSystem
	return m_materialSystem->getMaterialPass( m_materialHandle, passName );
}

ID3D12RootSignature *MaterialInstance::getRootSignature() const
{
	// T302-AF3: Return raw pointer from ComPtr, nullptr if not created
	return m_rootSignature.Get();
}

bool MaterialInstance::createPipelineStateForPass( const std::string &passName )
{
	// T303-AF1: Create PSO using PipelineBuilder::buildPSO()
	const MaterialDefinition *material = getMaterial();
	if ( !material )
	{
		return false;
	}

	// Query render pass config from MaterialSystem
	const RenderPassConfig passConfig = m_materialSystem->getRenderPassConfig( passName );

	// Build PSO using PipelineBuilder
	auto pso = PipelineBuilder::buildPSO( m_device, *material, passConfig, m_materialSystem, passName );
	if ( !pso )
	{
		return false;
	}

	// Store in cache
	m_pipelineStates[passName] = std::move( pso );

	// T303-AF3: Remove from dirty set on successful creation
	m_dirtyPasses.erase( passName );

	return true;
}

ID3D12PipelineState *MaterialInstance::getPipelineState( const std::string &passName )
{
	// T303-AF2: Lazy creation with caching
	// Check if pass exists
	if ( !hasPass( passName ) )
	{
		return nullptr;
	}

	// Check if PSO already cached and not dirty
	const auto it = m_pipelineStates.find( passName );
	const bool needsCreation = ( it == m_pipelineStates.end() ) || ( m_dirtyPasses.count( passName ) > 0 );

	if ( needsCreation )
	{
		// Create or recreate PSO
		if ( !createPipelineStateForPass( passName ) )
		{
			return nullptr;
		}
	}

	// Return raw pointer from cache
	return m_pipelineStates[passName].Get();
}

bool MaterialInstance::setupCommandList( ID3D12GraphicsCommandList *commandList, const std::string &passName )
{
	// T304-AF1: Validate command list is not nullptr
	if ( !commandList )
	{
		return false;
	}

	// T304-AF1: Get PSO via getPipelineState (internally validates pass and creates if needed)
	ID3D12PipelineState *pso = getPipelineState( passName );
	if ( !pso )
	{
		return false;
	}

	// T304-AF3: Check root signature availability
	ID3D12RootSignature *rootSig = getRootSignature();
	if ( !rootSig )
	{
		return false;
	}

	// T304-AF2: Set PSO and root signature on command list
	commandList->SetPipelineState( pso );
	commandList->SetGraphicsRootSignature( rootSig );

	return true;
}

} // namespace graphics::material_system

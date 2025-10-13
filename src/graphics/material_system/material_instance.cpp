#include "graphics/material_system/material_instance.h"
#include "graphics/material_system/pipeline_builder.h"
#include "graphics/shader_manager/shader_manager.h"

namespace graphics::material_system
{

MaterialInstance::MaterialInstance(
	dx12::Device *device,
	MaterialSystem *materialSystem,
	shader_manager::ShaderManager *shaderManager,
	const std::string &materialId )
	: m_device( device ), m_materialSystem( materialSystem ), m_shaderManager( shaderManager )
{
	// AF2: Query MaterialHandle from MaterialSystem using material ID
	m_materialHandle = m_materialSystem->getMaterialHandle( materialId );

	// T306: Cache MaterialDefinition pointer for performance
	if ( m_materialHandle.isValid() )
	{
		m_materialDefinition = m_materialSystem->getMaterial( m_materialHandle );
	}

	// T302-AF1: Call PipelineBuilder::getRootSignature() if material is valid
	if ( m_materialDefinition )
	{
		m_rootSignature = PipelineBuilder::getRootSignature( m_device, *m_materialDefinition );
	}

	// T305-AF3: Register hot-reload callback if ShaderManager provided
	if ( m_shaderManager )
	{
		m_hotReloadCallbackHandle = m_shaderManager->registerReloadCallback(
			[this]( shader_manager::ShaderHandle, const shader_manager::ShaderBlob & ) {
				this->onShaderReloaded();
			} );
	}
}

MaterialInstance::~MaterialInstance()
{
	// T305-AF4: Unregister hot-reload callback
	if ( m_shaderManager && m_hotReloadCallbackHandle != 0 )
	{
		m_shaderManager->unregisterReloadCallback( m_hotReloadCallbackHandle );
	}
}

bool MaterialInstance::isValid() const
{
	// AF3: Check if material handle is valid and has at least one pass
	// T306: Use cached pointer instead of querying MaterialSystem
	if ( !m_materialDefinition )
	{
		return false;
	}

	// Check that material has at least one pass
	return !m_materialDefinition->passes.empty();
}

bool MaterialInstance::hasPass( const std::string &passName ) const
{
	// AF4: Query material definition and use MaterialDefinition::hasPass()
	// T306: Use cached pointer instead of querying MaterialSystem
	if ( !m_materialDefinition )
	{
		return false;
	}

	return m_materialDefinition->hasPass( passName );
}

const MaterialDefinition *MaterialInstance::getMaterial() const
{
	// AF5: Return cached pointer (T306)
	return m_materialDefinition;
}

const MaterialPass *MaterialInstance::getPass( const std::string &passName ) const
{
	// AF5: Query specific pass using cached pointer (T306)
	if ( !m_materialDefinition )
	{
		return nullptr;
	}
	return m_materialDefinition->getPass( passName );
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

void MaterialInstance::onShaderReloaded()
{
	// T305-AF2: Mark all passes dirty and clear PSO cache
	// T306: Refresh cached pointer in case MaterialSystem updated definition in-place
	if ( m_materialHandle.isValid() )
	{
		m_materialDefinition = m_materialSystem->getMaterial( m_materialHandle );
	}

	if ( !m_materialDefinition )
	{
		return;
	}

	// Add all pass names to dirty set
	for ( const auto &pass : m_materialDefinition->passes )
	{
		m_dirtyPasses.insert( pass.passName );
	}

	// Clear cached PSOs (they will be recreated on next access)
	m_pipelineStates.clear();
}

} // namespace graphics::material_system

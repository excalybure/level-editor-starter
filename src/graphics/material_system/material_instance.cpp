#include "graphics/material_system/material_instance.h"
#include "graphics/material_system/pso_builder.h"

namespace graphics::material_system
{

MaterialInstance::MaterialInstance(
	dx12::Device *device,
	MaterialSystem *materialSystem,
	const std::string &materialId )
	: m_device( device ), m_materialSystem( materialSystem )
{
	const MaterialHandle materialHandle = m_materialSystem->getMaterialHandle( materialId );

	if ( materialHandle.isValid() )
	{
		m_materialDefinition = m_materialSystem->getMaterial( materialHandle );
	}

	if ( m_materialDefinition )
	{
		m_rootSignature = PSOBuilder::getRootSignature( m_device, *m_materialDefinition );
	}
}

bool MaterialInstance::isValid() const
{
	if ( !m_materialDefinition )
	{
		return false;
	}

	// Check that material has at least one pass
	return !m_materialDefinition->passes.empty();
}

bool MaterialInstance::hasPass( const std::string &passName ) const
{
	if ( !m_materialDefinition )
	{
		return false;
	}

	return m_materialDefinition->hasPass( passName );
}

const MaterialDefinition *MaterialInstance::getMaterial() const
{
	return m_materialDefinition;
}

const MaterialPass *MaterialInstance::getPass( const std::string &passName ) const
{
	if ( !m_materialDefinition )
	{
		return nullptr;
	}
	return m_materialDefinition->getPass( passName );
}

ID3D12RootSignature *MaterialInstance::getRootSignature() const
{
	return m_rootSignature.Get();
}

bool MaterialInstance::createPipelineStateForPass( const std::string &passName )
{
	if ( !m_materialDefinition )
	{
		return false;
	}

	// Query render pass config from MaterialSystem
	const RenderPassConfig passConfig = m_materialSystem->getRenderPassConfig( passName );

	// PSOBuilder handles shader recompilation and its own caching
	// Pass ShaderManager and ReflectionCache for reflection-based root signatures
	auto pso = PSOBuilder::build(
		m_device,
		*m_materialDefinition,
		passConfig,
		m_materialSystem,
		passName,
		m_materialSystem->getShaderManager(),
		m_materialSystem->getReflectionCache() );
	if ( !pso )
	{
		return false;
	}

	// Store in local cache for quick access
	m_pipelineStates[passName] = std::move( pso );

	return true;
}

ID3D12PipelineState *MaterialInstance::getPipelineState( const std::string &passName )
{
	if ( !hasPass( passName ) )
	{
		return nullptr;
	}

	// Check if PSO already cached (PSOBuilder handles shader recompilation internally)
	const auto it = m_pipelineStates.find( passName );
	if ( it == m_pipelineStates.end() )
	{
		if ( !createPipelineStateForPass( passName ) )
		{
			return nullptr;
		}
	}

	return m_pipelineStates[passName].Get();
}

bool MaterialInstance::setupCommandList( ID3D12GraphicsCommandList *commandList, const std::string &passName )
{
	if ( !commandList )
	{
		return false;
	}

	ID3D12PipelineState *pso = getPipelineState( passName );
	if ( !pso )
	{
		return false;
	}

	ID3D12RootSignature *rootSig = getRootSignature();
	if ( !rootSig )
	{
		return false;
	}

	commandList->SetPipelineState( pso );
	commandList->SetGraphicsRootSignature( rootSig );

	return true;
}

} // namespace graphics::material_system

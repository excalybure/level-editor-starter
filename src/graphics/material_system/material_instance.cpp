#include "graphics/material_system/material_instance.h"

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

} // namespace graphics::material_system

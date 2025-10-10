#include "graphics/material_system/material_system.h"
#include "graphics/material_system/loader.h"
#include "graphics/material_system/parser.h"
#include "core/console.h"

namespace graphics::material_system
{

bool MaterialSystem::initialize( const std::string &jsonPath )
{
	// Load and merge JSON documents
	::material_system::JsonLoader loader;
	if ( !loader.load( jsonPath ) )
	{
		console::error( "MaterialSystem: failed to load JSON from '{}'", jsonPath );
		return false;
	}

	const auto &mergedDoc = loader.getMergedDocument();

	// Parse materials array
	if ( !mergedDoc.contains( "materials" ) || !mergedDoc["materials"].is_array() )
	{
		console::error( "MaterialSystem: missing or invalid 'materials' array in JSON" );
		return false;
	}

	const auto &materialsArray = mergedDoc["materials"];
	m_materials.reserve( materialsArray.size() );

	// Parse each material
	for ( size_t i = 0; i < materialsArray.size(); ++i )
	{
		const auto material = MaterialParser::parse( materialsArray[i] );

		// Build index map
		m_materialIdToIndex[material.id] = static_cast<uint32_t>( i );
		m_materials.push_back( material );
	}

	console::info( "MaterialSystem: initialized with {} materials", m_materials.size() );
	return true;
}

MaterialHandle MaterialSystem::getMaterialHandle( const std::string &materialId ) const
{
	const auto it = m_materialIdToIndex.find( materialId );
	if ( it == m_materialIdToIndex.end() )
	{
		return MaterialHandle{}; // Invalid handle
	}

	MaterialHandle handle;
	handle.index = it->second;
	return handle;
}

const MaterialDefinition *MaterialSystem::getMaterial( MaterialHandle handle ) const
{
	if ( !handle.isValid() || handle.index >= m_materials.size() )
	{
		return nullptr;
	}

	return &m_materials[handle.index];
}

} // namespace graphics::material_system

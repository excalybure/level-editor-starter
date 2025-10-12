#include "graphics/material_system/material_system.h"
#include "graphics/material_system/loader.h"
#include "graphics/material_system/parser.h"
#include "graphics/material_system/state_parser.h"
#include "graphics/material_system/pipeline_builder.h"
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

	// Parse state blocks if present
	if ( mergedDoc.contains( "states" ) && mergedDoc["states"].is_object() )
	{
		const auto &statesSection = mergedDoc["states"];

		// Parse rasterizer states
		if ( statesSection.contains( "rasterizerStates" ) && statesSection["rasterizerStates"].is_object() )
		{
			for ( const auto &[id, stateJson] : statesSection["rasterizerStates"].items() )
			{
				auto state = StateBlockParser::parseRasterizer( stateJson );
				state.id = id; // Ensure id matches key

				// Check for duplicate IDs
				if ( m_rasterizerStates.find( id ) != m_rasterizerStates.end() )
				{
					console::errorAndThrow( "Duplicate rasterizer state ID: '{}'", id );
				}

				m_rasterizerStates[id] = state;
			}
		}

		// Parse depth stencil states
		if ( statesSection.contains( "depthStencilStates" ) && statesSection["depthStencilStates"].is_object() )
		{
			for ( const auto &[id, stateJson] : statesSection["depthStencilStates"].items() )
			{
				auto state = StateBlockParser::parseDepthStencil( stateJson );
				state.id = id;

				if ( m_depthStencilStates.find( id ) != m_depthStencilStates.end() )
				{
					console::errorAndThrow( "Duplicate depth stencil state ID: '{}'", id );
				}

				m_depthStencilStates[id] = state;
			}
		}

		// Parse blend states
		if ( statesSection.contains( "blendStates" ) && statesSection["blendStates"].is_object() )
		{
			for ( const auto &[id, stateJson] : statesSection["blendStates"].items() )
			{
				auto state = StateBlockParser::parseBlend( stateJson );
				state.id = id;

				if ( m_blendStates.find( id ) != m_blendStates.end() )
				{
					console::errorAndThrow( "Duplicate blend state ID: '{}'", id );
				}

				m_blendStates[id] = state;
			}
		}

		// Parse render target states
		if ( statesSection.contains( "renderTargetStates" ) && statesSection["renderTargetStates"].is_object() )
		{
			for ( const auto &[id, stateJson] : statesSection["renderTargetStates"].items() )
			{
				auto state = StateBlockParser::parseRenderTarget( stateJson );
				state.id = id;

				if ( m_renderTargetStates.find( id ) != m_renderTargetStates.end() )
				{
					console::errorAndThrow( "Duplicate render target state ID: '{}'", id );
				}

				m_renderTargetStates[id] = state;
			}
		}

		// Parse vertex formats
		if ( statesSection.contains( "vertexFormats" ) && statesSection["vertexFormats"].is_object() )
		{
			for ( const auto &[id, formatJson] : statesSection["vertexFormats"].items() )
			{
				auto format = StateBlockParser::parseVertexFormat( formatJson );
				format.id = id;

				if ( m_vertexFormats.find( id ) != m_vertexFormats.end() )
				{
					console::errorAndThrow( "Duplicate vertex format ID: '{}'", id );
				}

				m_vertexFormats[id] = format;
			}
		}
	}

	// Parse render passes array (optional)
	if ( mergedDoc.contains( "renderPasses" ) && mergedDoc["renderPasses"].is_array() )
	{
		const auto &renderPassesArray = mergedDoc["renderPasses"];
		for ( const auto &renderPassJson : renderPassesArray )
		{
			const auto renderPass = MaterialParser::parseRenderPass( renderPassJson );

			// Check for duplicate names
			if ( m_renderPasses.find( renderPass.name ) != m_renderPasses.end() )
			{
				console::errorAndThrow( "Duplicate render pass name: '{}'", renderPass.name );
			}

			m_renderPasses[renderPass.name] = renderPass;
		}
	}

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

const RasterizerStateBlock *MaterialSystem::getRasterizerState( const std::string &id ) const
{
	const auto it = m_rasterizerStates.find( id );
	if ( it == m_rasterizerStates.end() )
	{
		return nullptr;
	}
	return &it->second;
}

const DepthStencilStateBlock *MaterialSystem::getDepthStencilState( const std::string &id ) const
{
	const auto it = m_depthStencilStates.find( id );
	if ( it == m_depthStencilStates.end() )
	{
		return nullptr;
	}
	return &it->second;
}

const BlendStateBlock *MaterialSystem::getBlendState( const std::string &id ) const
{
	const auto it = m_blendStates.find( id );
	if ( it == m_blendStates.end() )
	{
		return nullptr;
	}
	return &it->second;
}

const RenderTargetStateBlock *MaterialSystem::getRenderTargetState( const std::string &id ) const
{
	const auto it = m_renderTargetStates.find( id );
	if ( it == m_renderTargetStates.end() )
	{
		return nullptr;
	}
	return &it->second;
}

const VertexFormat *MaterialSystem::getVertexFormat( const std::string &id ) const
{
	const auto it = m_vertexFormats.find( id );
	if ( it == m_vertexFormats.end() )
	{
		return nullptr;
	}
	return &it->second;
}

const RenderPassDefinition *MaterialSystem::getRenderPass( const std::string &name ) const
{
	const auto it = m_renderPasses.find( name );
	if ( it == m_renderPasses.end() )
	{
		return nullptr;
	}
	return &it->second;
}

RenderPassConfig MaterialSystem::getRenderPassConfig( const std::string &passName ) const
{
	// Query render pass by name
	const auto *passDef = getRenderPass( passName );
	if ( !passDef )
	{
		console::errorAndThrow( "MaterialSystem::getRenderPassConfig: Undefined render pass: {}", passName );
		return {};
	}

	// Query render target state block
	const auto *rtState = getRenderTargetState( passDef->states.renderTarget );
	if ( !rtState )
	{
		console::errorAndThrow( "MaterialSystem::getRenderPassConfig: Undefined render target state '{}' referenced by render pass '{}'",
			passDef->states.renderTarget,
			passName );
		return {};
	}

	// Populate RenderPassConfig from render target state block
	RenderPassConfig config;
	config.name = passDef->name;
	config.numRenderTargets = static_cast<UINT>( rtState->rtvFormats.size() );
	for ( size_t i = 0; i < rtState->rtvFormats.size() && i < 8; ++i )
	{
		config.rtvFormats[i] = rtState->rtvFormats[i];
	}
	config.dsvFormat = rtState->dsvFormat;

	return config;
}

} // namespace graphics::material_system

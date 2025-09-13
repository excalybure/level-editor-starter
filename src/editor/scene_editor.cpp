// Global module fragment for ImGui headers
module;
#include "imgui.h"

module editor.scene_editor;

import std;
import runtime.console;
import runtime.scene_importer;

namespace editor
{

SceneEditor::SceneEditor()
	: m_scene( nullptr ), m_systemManager( nullptr ), m_assetManager( nullptr ), m_gpuManager( nullptr )
{
}

SceneEditor::SceneEditor( ecs::Scene &scene,
	systems::SystemManager &systemManager,
	assets::AssetManager &assetManager,
	engine::GPUResourceManager &gpuManager )
	: m_scene( &scene ), m_systemManager( &systemManager ), m_assetManager( &assetManager ), m_gpuManager( &gpuManager )
{
}

bool SceneEditor::loadScene( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		m_lastError = "File path is empty";
		console::error( "SceneEditor: Cannot load scene - file path is empty" );
		return false;
	}

	// Check if dependencies are available
	if ( !m_assetManager || !m_scene || !m_gpuManager )
	{
		m_lastError = "SceneEditor dependencies not available";
		console::error( "SceneEditor: Cannot load scene - dependencies not initialized" );
		return false;
	}

	// Clear existing scene first
	clearScene();

	// Load scene via AssetManager
	auto assetScene = m_assetManager->load<assets::Scene>( filePath );
	if ( !assetScene )
	{
		m_lastError = "Failed to load scene from file: " + filePath;
		console::error( "SceneEditor: Failed to load scene from file: {}", filePath );
		return false;
	}

	// Import scene into ECS
	if ( !runtime::SceneImporter::importScene( assetScene, *m_scene ) )
	{
		m_lastError = "Failed to import scene into ECS";
		console::error( "SceneEditor: Failed to import scene into ECS" );
		return false;
	}

	// Create GPU resources
	if ( !runtime::SceneImporter::createGPUResources( assetScene, *m_scene, *m_gpuManager ) )
	{
		m_lastError = "Failed to create GPU resources for scene";
		console::error( "SceneEditor: Failed to create GPU resources for scene" );
		return false;
	}

	// Update current path
	m_currentPath = filePath;
	m_lastError.clear();

	console::info( "SceneEditor: Successfully loaded scene: {}", filePath );
	return true;
}

void SceneEditor::clearScene()
{
	if ( !m_scene )
	{
		console::warning( "SceneEditor: Cannot clear scene - scene not available" );
		return;
	}

	// Get all entities and destroy them
	const auto entities = m_scene->getAllEntities();
	for ( const auto &entity : entities )
	{
		if ( entity.isValid() )
		{
			m_scene->destroyEntity( entity );
		}
	}

	m_currentPath.clear();
	m_lastError.clear();

	console::info( "SceneEditor: Scene cleared" );
}

void SceneEditor::openFileDialog()
{
	m_showFileDialog = true;
	console::info( "SceneEditor: File dialog opened" );
}

void SceneEditor::renderMenuBar()
{
	if ( ImGui::BeginMenu( "Scene" ) )
	{
		if ( ImGui::MenuItem( "Open Scene..." ) )
		{
			m_showFileDialog = true;
		}

		ImGui::Separator();

		if ( ImGui::MenuItem( "Clear Scene" ) )
		{
			clearScene();
		}

		ImGui::EndMenu();
	}

	// Process file dialog if requested
	processFileDialog();
}

void SceneEditor::renderStatusBar()
{
	if ( ImGui::BeginTable( "StatusBar", 3, ImGuiTableFlags_SizingFixedFit ) )
	{
		ImGui::TableNextColumn();

		// Current file path
		if ( !m_currentPath.empty() )
		{
			ImGui::Text( "Scene: %s", m_currentPath.c_str() );
		}
		else
		{
			ImGui::Text( "No scene loaded" );
		}

		ImGui::TableNextColumn();

		// Entity count
		const size_t entityCount = getEntityCount();
		ImGui::Text( "Entities: %zu", entityCount );

		ImGui::TableNextColumn();

		// Error status
		if ( !m_lastError.empty() )
		{
			ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ), "Error: %s", m_lastError.c_str() );
		}
		else
		{
			ImGui::Text( "Ready" );
		}

		ImGui::EndTable();
	}
}

size_t SceneEditor::getEntityCount() const
{
	if ( !m_scene )
	{
		return 0;
	}

	const auto entities = m_scene->getAllEntities();
	size_t count = 0;
	for ( const auto &entity : entities )
	{
		if ( entity.isValid() )
		{
			count++;
		}
	}
	return count;
}

void SceneEditor::processFileDialog()
{
	if ( m_showFileDialog )
	{
		// For now, just simulate closing the dialog
		// TODO: Implement actual file dialog using ImGui or native Windows dialog
		console::info( "SceneEditor: File dialog processed (closed without selection)" );
		m_showFileDialog = false;
	}
}

} // namespace editor
// Global module fragment for ImGui headers
module;
#include "imgui.h"

export module editor.scene_editor;

import std;
import runtime.ecs;
import runtime.systems;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;

export namespace editor
{

export class SceneEditor
{
public:
	// Default constructor for integration testing
	SceneEditor();

	SceneEditor( ecs::Scene &scene,
		systems::SystemManager &systemManager,
		assets::AssetManager &assetManager,
		engine::GPUResourceManager &gpuManager );

	// File operations
	bool loadScene( const std::string &filePath );
	void clearScene();

	// File dialog operations
	void openFileDialog();
	bool isFileDialogActive() const { return m_showFileDialog; }
	void processFileDialog();

	// UI rendering
	void renderMenuBar();
	void renderStatusBar();

	// State queries
	const std::string &getCurrentScenePath() const { return m_currentPath; }
	size_t getEntityCount() const;

private:
	ecs::Scene *m_scene;
	systems::SystemManager *m_systemManager;
	assets::AssetManager *m_assetManager;
	engine::GPUResourceManager *m_gpuManager;

	std::string m_currentPath;
	bool m_showFileDialog = false;
	std::string m_lastError;
};

} // namespace editor
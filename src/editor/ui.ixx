export module editor.ui;

import std;
import editor.viewport;
import runtime.ecs;
import runtime.systems;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;
import platform.dx12;
import platform.win32.win32_window;
import engine.shader_manager;

export namespace editor
{

// Simple 2D vector for ImGui integration (avoids ImVec2 conflicts)
struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	Vec2( float x_, float y_ ) : x( x_ ), y( y_ ) {}
};

// UI layout configuration for docked viewports
struct ViewportLayout
{
	struct ViewportPane
	{
		const char *name;
		ViewportType type;
		Vec2 minSize{ 400.0f, 300.0f };
		bool isOpen = true;
	};

	// Four viewport panes in the standard layout
	std::array<ViewportPane, 4> panes = { { { "Perspective", ViewportType::Perspective, { 400.0f, 300.0f }, true },
		{ "Top (XY)", ViewportType::Top, { 400.0f, 300.0f }, true },
		{ "Front (XZ)", ViewportType::Front, { 400.0f, 300.0f }, true },
		{ "Side (YZ)", ViewportType::Side, { 400.0f, 300.0f }, true } } };
};

// Main UI class managing ImGui docking and viewport integration
export class UI
{
public:
	UI();
	~UI();

	// No copy/move for now
	UI( const UI & ) = delete;
	UI &operator=( const UI & ) = delete;

	// Initialize ImGui with D3D12 device and shader manager
	bool initialize( void *window_handle, dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager );

	// Cleanup ImGui resources
	void shutdown();

	// Frame management
	void beginFrame();
	void endFrame();
	// Submit ImGui draw data to the provided D3D12 graphics command list
	void renderDrawData( void *command_list );

	// Get viewport layout configuration
	const ViewportLayout &getLayout() const { return m_layout; }
	ViewportLayout &getLayout() { return m_layout; }

	// Check if ImGui wants to capture mouse/keyboard
	bool wantsCaptureMouse() const;
	bool wantsCaptureKeyboard() const;

	// Check if user requested to exit via menu
	bool shouldExit() const;

	// Viewport management
	Viewport *getViewport( ViewportType type );
	const Viewport *getViewport( ViewportType type ) const;

	// Grid settings management
	void showGridSettingsWindow( bool show = true );
	bool isGridSettingsWindowOpen() const;

	// Camera settings management
	void showCameraSettingsWindow( bool show = true );
	bool isCameraSettingsWindowOpen() const;

	// Scene Operations (unified interface)
	void initializeSceneOperations( ecs::Scene &scene,
		systems::SystemManager &systemManager,
		assets::AssetManager &assetManager,
		engine::GPUResourceManager &gpuManager );

	// File operations
	bool loadScene( const std::string &filePath );
	void clearScene();

	// File dialog operations
	void openFileDialog();
	bool isFileDialogOpen() const;
	void processFileDialog();

	// Scene state queries
	const std::string &getCurrentScenePath() const;
	size_t getEntityCount() const;
	const std::string &getLastError() const;

	// Get viewport manager for input handling
	ViewportManager &getViewportManager();

	void processInputEvents( platform::Win32Window &window );
	void updateViewports( const float deltaTime );

private:
	ViewportLayout m_layout;
	bool m_initialized = false;
	mutable bool m_shouldExit = false;

	// Implementation details hidden from module interface
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace editor

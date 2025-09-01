export module editor.ui;

import std;
import editor.viewport;
import platform.dx12;

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

	// Initialize ImGui with D3D12 device
	bool initialize( void *window_handle, dx12::Device *device );

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

private:
	ViewportLayout m_layout;
	bool m_initialized = false;
	mutable bool m_shouldExit = false;

	// Implementation details hidden from module interface
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace editor

export module editor.ui;

import std;
import editor.viewport;

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

	// Initialize ImGui with D3D12 backend
	bool initialize( void *window_handle, void *d3d_device, void *d3d_descriptor_heap );

	// Cleanup ImGui resources
	void shutdown();

	// Frame management
	void beginFrame();
	void endFrame();

	// Get viewport layout configuration
	const ViewportLayout &get_layout() const { return m_layout; }
	ViewportLayout &get_layout() { return m_layout; }

	// Check if ImGui wants to capture mouse/keyboard
	bool wants_capture_mouse() const;
	bool wants_capture_keyboard() const;

private:
	ViewportLayout m_layout;
	bool m_initialized = false;

	// Implementation details hidden from module interface
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace editor

// Include ImGui implementation in the global module fragment to avoid conflicts
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <windows.h>

module editor.ui;

import std;

namespace editor
{

// Helper functions to convert between our Vec2 and ImVec2
ImVec2 to_imgui_vec2( const Vec2 &v )
{
	return ImVec2( v.x, v.y );
}
Vec2 from_imgui_vec2( const ImVec2 &v )
{
	return Vec2( v.x, v.y );
}

// Implementation details hidden using pImpl pattern
struct UI::Impl
{
	ImGuiID dockspace_id = 0;
	bool first_layout = true;

	// Setup the main dockspace
	void setup_dockspace( ViewportLayout &layout );

	// Setup initial docked layout
	void setup_initial_layout( ImGuiID dockspace_id );

	// Render viewport windows
	void render_viewport_windows( ViewportLayout &layout );

	// Render individual viewport pane
	void render_viewport_pane( const ViewportLayout::ViewportPane &pane );
};

UI::UI() : m_impl( std::make_unique<Impl>() )
{
}

UI::~UI()
{
	shutdown();
}

bool UI::initialize( void *window_handle, void *d3d_device, void *d3d_descriptor_heap )
{
	HWND hwnd = static_cast<HWND>( window_handle );
	ID3D12Device *device = static_cast<ID3D12Device *>( d3d_device );
	ID3D12DescriptorHeap *heap = static_cast<ID3D12DescriptorHeap *>( d3d_descriptor_heap );

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	// Enable docking and multi-viewport features
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled, adjust window rounding to be compatible with OS decorations
	ImGuiStyle &style = ImGui::GetStyle();
	if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	if ( !ImGui_ImplWin32_Init( hwnd ) )
		return false;

	if ( !ImGui_ImplDX12_Init(
			 device,
			 3, // Number of frames in flight
			 DXGI_FORMAT_R8G8B8A8_UNORM,
			 heap,
			 heap->GetCPUDescriptorHandleForHeapStart(),
			 heap->GetGPUDescriptorHandleForHeapStart() ) )
	{
		ImGui_ImplWin32_Shutdown();
		return false;
	}

	m_initialized = true;
	return true;
}

void UI::shutdown()
{
	if ( !m_initialized )
		return;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_initialized = false;
}

void UI::beginFrame()
{
	if ( !m_initialized )
		return;

	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Setup the main dockspace and render viewports
	m_impl->setup_dockspace( m_layout );
	m_impl->render_viewport_windows( m_layout );
}

void UI::endFrame()
{
	if ( !m_initialized )
		return;

	// End the dockspace
	ImGui::End(); // End dockspace window

	// Rendering
	ImGui::Render();

	// Note: ImGui_ImplDX12_RenderDrawData should be called by the graphics system
	// after setting up the command list properly. For now, we'll skip this.

	// Update and Render additional Platform Windows
	ImGuiIO &io = ImGui::GetIO();
	if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

bool UI::wants_capture_mouse() const
{
	if ( !m_initialized )
		return false;
	return ImGui::GetIO().WantCaptureMouse;
}

bool UI::wants_capture_keyboard() const
{
	if ( !m_initialized )
		return false;
	return ImGui::GetIO().WantCaptureKeyboard;
}

// Implementation details
void UI::Impl::setup_dockspace( ViewportLayout &layout )
{
	// Create a fullscreen dockspace window
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos( viewport->WorkPos );
	ImGui::SetNextWindowSize( viewport->WorkSize );
	ImGui::SetNextWindowViewport( viewport->ID );

	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

	ImGui::Begin( "Level Editor Dockspace", nullptr, window_flags );
	ImGui::PopStyleVar( 3 );

	// Create the dockspace
	ImGuiID dockspace_id = ImGui::GetID( "LevelEditorDockspace" );
	this->dockspace_id = dockspace_id;

	ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

	// Setup initial layout on first run
	if ( first_layout )
	{
		setup_initial_layout( dockspace_id );
		first_layout = false;
	}

	// Add menu bar
	if ( ImGui::BeginMenuBar() )
	{
		if ( ImGui::BeginMenu( "File" ) )
		{
			if ( ImGui::MenuItem( "New" ) )
			{ /* TODO: Implement */
			}
			if ( ImGui::MenuItem( "Open" ) )
			{ /* TODO: Implement */
			}
			if ( ImGui::MenuItem( "Save" ) )
			{ /* TODO: Implement */
			}
			ImGui::Separator();
			if ( ImGui::MenuItem( "Exit" ) )
			{ /* TODO: Implement */
			}
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "View" ) )
		{
			for ( auto &pane : layout.panes )
			{
				ImGui::Checkbox( pane.name, &pane.isOpen );
			}
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Tools" ) )
		{
			ImGui::MenuItem( "Grid Settings", nullptr, false, false );	 // Disabled for now
			ImGui::MenuItem( "Camera Settings", nullptr, false, false ); // Disabled for now
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void UI::Impl::setup_initial_layout( ImGuiID dockspace_id )
{
	// For now, just use the default ImGui docking behavior
	// In a full implementation, you would use ImGui::DockBuilderXXX functions
	// to create a specific layout, but those require the docking branch
	// which may not be available in the current vcpkg version
}

void UI::Impl::render_viewport_windows( ViewportLayout &layout )
{
	for ( const auto &pane : layout.panes )
	{
		if ( pane.isOpen )
		{
			render_viewport_pane( pane );
		}
	}
}

void UI::Impl::render_viewport_pane( const ViewportLayout::ViewportPane &pane )
{
	ImGui::SetNextWindowSizeConstraints( to_imgui_vec2( pane.minSize ), ImVec2( FLT_MAX, FLT_MAX ) );

	if ( ImGui::Begin( pane.name, const_cast<bool *>( &pane.isOpen ) ) )
	{
		// Get the content region size
		ImVec2 content_size = ImGui::GetContentRegionAvail();

		// For now, just show placeholder content
		const char *viewport_info = "";
		switch ( pane.type )
		{
		case ViewportType::Perspective:
			viewport_info = "3D Perspective View\nCamera controls: Mouse to orbit, WASD to move";
			break;
		case ViewportType::Top:
			viewport_info = "Top View (XY Plane)\nLooking down Z-axis";
			break;
		case ViewportType::Front:
			viewport_info = "Front View (XZ Plane)\nLooking down Y-axis";
			break;
		case ViewportType::Side:
			viewport_info = "Side View (YZ Plane)\nLooking down X-axis";
			break;
		}

		// Center the text in the viewport
		ImVec2 text_size = ImGui::CalcTextSize( viewport_info );
		ImVec2 center = ImVec2(
			( content_size.x - text_size.x ) * 0.5f,
			( content_size.y - text_size.y ) * 0.5f );

		ImGui::SetCursorPos( center );
		ImGui::TextUnformatted( viewport_info );

		// TODO: Render actual 3D viewport content here
		// This is where we'll integrate the Viewport class from editor.viewport

		// Show viewport dimensions for debugging
		ImGui::SetCursorPos( ImVec2( 5, 5 ) );
		ImGui::Text( "Size: %.0fx%.0f", content_size.x, content_size.y );
	}
	ImGui::End();
}

} // namespace editor

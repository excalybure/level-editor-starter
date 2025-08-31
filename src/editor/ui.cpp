// Include ImGui implementation in the global module fragment to avoid conflicts
module; // start global module fragment so we can include headers before the module declaration
#include "imgui.h"
#include "imgui_internal.h" // For DockBuilder API
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
	ImGuiID dockspaceId = 0;
	bool firstLayout = true;

	// Viewport manager for coordinated viewport management
	ViewportManager viewportManager;

	// Setup the main dockspace
	void setupDockspace( ViewportLayout &layout, UI &ui );

	// Setup initial docked layout
	void setupInitialLayout( ImGuiID inDockspaceId );

	// Render viewport windows
	void renderViewportWindows( ViewportLayout &layout );

	// Render individual viewport pane
	void renderViewportPane( const ViewportLayout::ViewportPane &pane );

	// Initialize viewports with D3D12 device
	bool initializeViewports( dx12::Device *device );
	void shutdownViewports();

	// Get viewport by type
	Viewport *getViewport( ViewportType type );
};

UI::UI() : m_impl( std::make_unique<Impl>() )
{
	// Viewport initialization will happen in initialize() once we have the D3D12 device
}

UI::~UI()
{
	shutdown();
}

bool UI::initialize( void *window_handle, dx12::Device *dx12Device )
{
	HWND hwnd = static_cast<HWND>( window_handle );

	// Basic validation to allow safe negative-path unit tests without crashing inside backends
	if ( !hwnd || !dx12Device )
	{
		return false;
	}

	// Initialize viewports with D3D12 device first
	if ( !m_impl->initializeViewports( dx12Device ) )
	{
		return false;
	}

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
			 dx12Device->getDevice(),
			 3, // Number of frames in flight
			 DXGI_FORMAT_R8G8B8A8_UNORM,
			 dx12Device->getImguiDescriptorHeap(),
			 dx12Device->getImguiDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
			 dx12Device->getImguiDescriptorHeap()->GetGPUDescriptorHandleForHeapStart() ) )
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

	// Shutdown viewports first
	m_impl->shutdownViewports();

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
	m_impl->setupDockspace( m_layout, *this );
	m_impl->renderViewportWindows( m_layout );
}

void UI::endFrame()
{
	if ( !m_initialized )
		return;

	// End the dockspace
	ImGui::End(); // End dockspace window

	// Finalize ImGui command lists
	ImGui::Render();

	// Platform windows (multi-viewport) need to be updated before the caller records ImGui draw data
	ImGuiIO &io = ImGui::GetIO();
	if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void UI::renderDrawData( void *command_list )
{
	if ( !m_initialized || !command_list )
		return;
	// Command list assumed to be in a recording state with render target + descriptor heap bound
	ImDrawData *drawData = ImGui::GetDrawData();
	if ( drawData )
	{
		ImGui_ImplDX12_RenderDrawData( drawData, static_cast<ID3D12GraphicsCommandList *>( command_list ) );
	}
}

bool UI::wantsCaptureMouse() const
{
	if ( !m_initialized )
		return false;
	return ImGui::GetIO().WantCaptureMouse;
}

bool UI::wantsCaptureKeyboard() const
{
	if ( !m_initialized )
		return false;
	return ImGui::GetIO().WantCaptureKeyboard;
}

bool UI::shouldExit() const
{
	return m_shouldExit;
}

// Implementation details
void UI::Impl::setupDockspace( ViewportLayout &layout, UI &ui )
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
	dockspaceId = ImGui::GetID( "LevelEditorDockspace" );

	ImGui::DockSpace( dockspaceId, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

	// Setup initial layout on first run
	if ( firstLayout )
	{
		setupInitialLayout( dockspaceId );
		firstLayout = false;
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
			{
				// Set the exit flag when user selects Exit from menu
				ui.m_shouldExit = true;
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

void UI::Impl::setupInitialLayout( ImGuiID inDockspaceId )
{
	// Only set up the initial layout if the dockspace is empty (first run or reset)
	// This preserves user's layout modifications between runs
	ImGuiDockNode *node = ImGui::DockBuilderGetNode( inDockspaceId );
	if ( node == nullptr || node->IsEmpty() )
	{
		// Clear any existing layout and rebuild
		ImGui::DockBuilderRemoveNode( inDockspaceId );							  // Clear out existing layout
		ImGui::DockBuilderAddNode( inDockspaceId, ImGuiDockNodeFlags_DockSpace ); // Add back the dockspace node
		ImGui::DockBuilderSetNodeSize( inDockspaceId, ImGui::GetMainViewport()->WorkSize );

		// Split the dockspace into regions for our four viewports
		// We'll create a 2x2 grid layout:
		// +-------------+-------------+
		// | Perspective |    Top      |
		// |     3D      |   (XY)      |
		// +-------------+-------------+
		// |   Front     |    Side     |
		// |   (XZ)      |   (YZ)      |
		// +-------------+-------------+

		ImGuiID dockLeftColumn, dockRightColumn;
		ImGuiID dockTopLeft, dockBottomLeft;
		ImGuiID dockTopRight, dockBottomRight;

		// First split: divide main dockspace into left and right columns
		ImGui::DockBuilderSplitNode( inDockspaceId, ImGuiDir_Left, 0.5f, &dockLeftColumn, &dockRightColumn );

		// Split left column into top and bottom
		ImGui::DockBuilderSplitNode( dockLeftColumn, ImGuiDir_Up, 0.5f, &dockTopLeft, &dockBottomLeft );

		// Split right column into top and bottom
		ImGui::DockBuilderSplitNode( dockRightColumn, ImGuiDir_Up, 0.5f, &dockTopRight, &dockBottomRight );

		// Dock each viewport window to its designated area
		ImGui::DockBuilderDockWindow( "Perspective", dockTopLeft );	  // Top-left: 3D Perspective
		ImGui::DockBuilderDockWindow( "Top (XY)", dockTopRight );	  // Top-right: Top view
		ImGui::DockBuilderDockWindow( "Front (XZ)", dockBottomLeft ); // Bottom-left: Front view
		ImGui::DockBuilderDockWindow( "Side (YZ)", dockBottomRight ); // Bottom-right: Side view

		// Finalize the layout
		ImGui::DockBuilderFinish( inDockspaceId );
	}
}

void UI::Impl::renderViewportWindows( ViewportLayout &layout )
{
	for ( const auto &pane : layout.panes )
	{
		if ( pane.isOpen )
		{
			renderViewportPane( pane );
		}
	}
}

void UI::Impl::renderViewportPane( const ViewportLayout::ViewportPane &pane )
{
	ImGui::SetNextWindowSizeConstraints( to_imgui_vec2( pane.minSize ), ImVec2( FLT_MAX, FLT_MAX ) );

	if ( ImGui::Begin( pane.name, const_cast<bool *>( &pane.isOpen ) ) )
	{
		// Get the content region size
		const ImVec2 contentSize = ImGui::GetContentRegionAvail();

		// Get the corresponding viewport
		Viewport *viewport = getViewport( pane.type );

		if ( viewport )
		{
			// Update viewport size if it has changed
			const auto &currentSize = viewport->getSize();
			if ( currentSize.x != static_cast<int>( contentSize.x ) ||
				currentSize.y != static_cast<int>( contentSize.y ) )
			{
				viewport->setRenderTargetSize( static_cast<int>( contentSize.x ), static_cast<int>( contentSize.y ) );
			}

			// Check if this viewport has focus
			bool hasFocus = ImGui::IsWindowFocused();
			viewport->setFocused( hasFocus );
			viewport->setActive( hasFocus );

			// Get render target texture for ImGui rendering
			void *textureHandle = viewport->getImGuiTextureId();

			if ( textureHandle )
			{
				// Render the actual 3D viewport content
				ImGui::Image( reinterpret_cast<ImTextureID>( textureHandle ), contentSize );
			}
			else
			{
				// Show placeholder content until render targets are implemented
				const char *viewportInfo = ViewportUtils::getViewportTypeName( pane.type );
				const char *cameraInfo = "";

				// Get camera information
				if ( auto *camera = viewport->getCamera() )
				{
					const auto &pos = camera->getPosition();
					const auto &target = camera->getTarget();
					static char cameraBuffer[256];
					sprintf_s( cameraBuffer, sizeof( cameraBuffer ), "\nCamera: (%.1f, %.1f, %.1f)\nTarget: (%.1f, %.1f, %.1f)\nAspect: %.2f", pos.x, pos.y, pos.z, target.x, target.y, target.z, viewport->getAspectRatio() );
					cameraInfo = cameraBuffer;
				}

				// Combine viewport and camera info
				static char fullInfo[512];
				sprintf_s( fullInfo, sizeof( fullInfo ), "%s\n%s\n\nTODO: D3D12 render targets", viewportInfo, cameraInfo );

				// Center the text in the viewport
				const ImVec2 text_size = ImGui::CalcTextSize( fullInfo );
				const ImVec2 center = ImVec2(
					( contentSize.x - text_size.x ) * 0.5f,
					( contentSize.y - text_size.y ) * 0.5f );

				ImGui::SetCursorPos( center );
				ImGui::TextUnformatted( fullInfo );
			}

			// Show viewport status for debugging
			ImGui::SetCursorPos( ImVec2( 5, 5 ) );
			ImGui::Text( "Size: %.0fx%.0f %s", contentSize.x, contentSize.y, hasFocus ? "(focused)" : "" );
		}
		else
		{
			// Fallback if viewport is null
			ImGui::TextUnformatted( "Viewport not initialized" );
		}
	}
	ImGui::End();
}

// UI::Impl viewport management methods
bool UI::Impl::initializeViewports( dx12::Device *device )
{
	if ( !device )
		return false;

	// Initialize viewport manager with D3D12 device
	if ( !viewportManager.initialize( device ) )
		return false;

	// Create all four viewports
	viewportManager.createViewport( ViewportType::Perspective );
	viewportManager.createViewport( ViewportType::Top );
	viewportManager.createViewport( ViewportType::Front );
	viewportManager.createViewport( ViewportType::Side );

	return true;
}

void UI::Impl::shutdownViewports()
{
	viewportManager.shutdown();
}

Viewport *UI::Impl::getViewport( ViewportType type )
{
	// Find the viewport of the specified type in the manager
	const auto &viewports = viewportManager.getViewports();
	for ( const auto &viewport : viewports )
	{
		if ( viewport->getType() == type )
			return viewport.get();
	}
	return nullptr;
}

// UI public viewport access methods
Viewport *UI::getViewport( ViewportType type )
{
	return m_impl->getViewport( type );
}

const Viewport *UI::getViewport( ViewportType type ) const
{
	return m_impl->getViewport( type );
}

} // namespace editor

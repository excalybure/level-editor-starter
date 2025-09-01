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
import engine.grid;

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

	// Grid settings window state
	bool showGridSettingsWindow = false;

	// Setup the main dockspace
	void setupDockspace( ViewportLayout &layout, UI &ui );

	// Setup initial docked layout
	void setupInitialLayout( ImGuiID inDockspaceId );

	// Render viewport windows
	void renderViewportWindows( ViewportLayout &layout );

	// Render individual viewport pane
	void renderViewportPane( const ViewportLayout::ViewportPane &pane );

	// Render grid settings window
	void renderGridSettingsWindow();

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

bool UI::initialize( void *window_handle, dx12::Device *device )
{
	HWND hwnd = static_cast<HWND>( window_handle );

	// Basic validation to allow safe negative-path unit tests without crashing inside backends
	if ( !hwnd || !device )
	{
		return false;
	}

	// Initialize viewports with D3D12 device first
	if ( !m_impl->initializeViewports( device ) )
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
			 device->getDevice(),
			 3, // Number of frames in flight
			 DXGI_FORMAT_R8G8B8A8_UNORM,
			 device->getImguiDescriptorHeap(),
			 device->getImguiDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
			 device->getImguiDescriptorHeap()->GetGPUDescriptorHandleForHeapStart() ) )
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

	// Render grid settings window if open
	m_impl->renderGridSettingsWindow();
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
			if ( ImGui::MenuItem( "Grid Settings" ) )
			{
				showGridSettingsWindow = true;
			}
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
			// Update viewport size if it has changed and is valid
			const auto &currentSize = viewport->getSize();
			const int newWidth = static_cast<int>( contentSize.x );
			const int newHeight = static_cast<int>( contentSize.y );
			const bool isValid = newWidth > 0 && newHeight > 0;

			// Only update render target size if dimensions are valid (> 0)
			if ( isValid && ( currentSize.x != newWidth || currentSize.y != newHeight ) )
			{
				// Set the viewport render target size
				viewport->setRenderTargetSize( newWidth, newHeight );
			}

			// Check if this viewport has focus
			const bool hasFocus = ImGui::IsWindowFocused();
			viewport->setFocused( hasFocus );
			viewport->setActive( hasFocus );

			// Get render target texture for ImGui rendering
			void *textureHandle = viewport->getImGuiTextureId();

			if ( isValid && textureHandle )
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
				static std::string cameraBuffer;
				if ( auto *camera = viewport->getCamera() )
				{
					const auto &pos = camera->getPosition();
					const auto &target = camera->getTarget();
					cameraBuffer = std::format( "\nCamera: ({:.1f}, {:.1f}, {:.1f})\nTarget: ({:.1f}, {:.1f}, {:.1f})\nAspect: {:.2f}", pos.x, pos.y, pos.z, target.x, target.y, target.z, viewport->getAspectRatio() );
					cameraInfo = cameraBuffer.c_str();
				}

				// Combine viewport and camera info
				static std::string fullInfo = std::format( "{}\n{}\n\nTODO: D3D12 render targets", viewportInfo, cameraInfo );

				// Center the text in the viewport
				const ImVec2 text_size = ImGui::CalcTextSize( fullInfo.c_str() );
				const ImVec2 center = ImVec2(
					( contentSize.x - text_size.x ) * 0.5f,
					( contentSize.y - text_size.y ) * 0.5f );

				ImGui::SetCursorPos( center );
				ImGui::TextUnformatted( fullInfo.c_str() );
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

void UI::Impl::renderGridSettingsWindow()
{
	if ( !showGridSettingsWindow )
		return;

	if ( ImGui::Begin( "Grid Settings", &showGridSettingsWindow ) )
	{
		// Get the first viewport to access grid settings (we'll apply to all)
		Viewport *viewport = getViewport( ViewportType::Perspective );
		if ( viewport )
		{
			// Get current grid settings
			auto gridSettings = viewport->getGridSettings();
			bool settingsChanged = false;

			// Grid visibility section
			ImGui::SeparatorText( "Visibility" );

			bool gridVisible = viewport->isGridVisible();
			if ( ImGui::Checkbox( "Show Grid", &gridVisible ) )
			{
				// Apply to all viewports
				viewportManager.setGlobalGridVisible( gridVisible );
				settingsChanged = true;
			}

			if ( ImGui::Checkbox( "Show Axes", &gridSettings.showAxes ) )
				settingsChanged = true;

			// Grid appearance section
			ImGui::SeparatorText( "Appearance" );

			// Major grid controls
			ImGui::Text( "Major Grid:" );
			if ( ImGui::ColorEdit3( "Major Color", &gridSettings.majorGridColor.x ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Major Alpha", &gridSettings.majorGridAlpha, 0.0f, 1.0f, "%.2f" ) )
				settingsChanged = true;

			// Minor grid controls
			ImGui::Text( "Minor Grid:" );
			if ( ImGui::ColorEdit3( "Minor Color", &gridSettings.minorGridColor.x ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Minor Alpha", &gridSettings.minorGridAlpha, 0.0f, 1.0f, "%.2f" ) )
				settingsChanged = true;

			// Axis colors section
			ImGui::SeparatorText( "Axis Colors" );

			ImGui::Text( "X-Axis (Red):" );
			if ( ImGui::ColorEdit3( "X Color", &gridSettings.axisXColor.x ) )
				settingsChanged = true;
			if ( ImGui::SliderFloat( "X Alpha", &gridSettings.axisXAlpha, 0.0f, 1.0f, "%.2f" ) )
				settingsChanged = true;

			ImGui::Text( "Y-Axis (Green):" );
			if ( ImGui::ColorEdit3( "Y Color", &gridSettings.axisYColor.x ) )
				settingsChanged = true;
			if ( ImGui::SliderFloat( "Y Alpha", &gridSettings.axisYAlpha, 0.0f, 1.0f, "%.2f" ) )
				settingsChanged = true;

			ImGui::Text( "Z-Axis (Blue):" );
			if ( ImGui::ColorEdit3( "Z Color", &gridSettings.axisZColor.x ) )
				settingsChanged = true;
			if ( ImGui::SliderFloat( "Z Alpha", &gridSettings.axisZAlpha, 0.0f, 1.0f, "%.2f" ) )
				settingsChanged = true;

			// Grid spacing section
			ImGui::SeparatorText( "Spacing" );

			if ( ImGui::SliderFloat( "Grid Spacing", &gridSettings.gridSpacing, 0.1f, 10.0f, "%.2f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Major Interval", &gridSettings.majorGridInterval, 2.0f, 20.0f, "%.1f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Axis Thickness", &gridSettings.axisThickness, 1.0f, 5.0f, "%.1f" ) )
				settingsChanged = true;

			// Advanced settings section
			ImGui::SeparatorText( "Advanced" );

			if ( ImGui::SliderFloat( "Fade Distance", &gridSettings.fadeDistance, 50.0f, 500.0f, "%.1f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Zoom Threshold", &gridSettings.zoomThreshold, 0.01f, 1.0f, "%.3f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Max Spacing", &gridSettings.maxGridSpacing, 10.0f, 1000.0f, "%.1f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Min Spacing", &gridSettings.minGridSpacing, 0.001f, 1.0f, "%.3f" ) )
				settingsChanged = true;

			// Apply changes section
			ImGui::Separator();

			if ( ImGui::Button( "Reset to Defaults" ) )
			{
				gridSettings = grid::GridSettings{}; // Reset to default values
				settingsChanged = true;
			}

			ImGui::SameLine();

			if ( ImGui::Button( "Apply to All Viewports" ) || settingsChanged )
			{
				// Apply settings to all viewports
				const auto &viewports = viewportManager.getViewports();
				for ( const auto &vp : viewports )
				{
					vp->setGridSettings( gridSettings );
				}
			}
		}
		else
		{
			ImGui::TextUnformatted( "No viewport available for grid settings" );
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

// Grid settings window management
void UI::showGridSettingsWindow( bool show )
{
	m_impl->showGridSettingsWindow = show;
}

bool UI::isGridSettingsWindowOpen() const
{
	return m_impl->showGridSettingsWindow;
}

} // namespace editor

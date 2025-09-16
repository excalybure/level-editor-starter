// Include ImGui implementation in the global module fragment to avoid conflicts
module; // start global module fragment so we can include headers before the module declaration
#include "imgui.h"
#include "imgui_internal.h" // For DockBuilder API
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <windows.h>
#include <commdlg.h> // For GetOpenFileName

module editor.ui;

import std;
import engine.grid;
import runtime.scene_importer;
import runtime.console;

namespace editor
{
const float kStatusBarHeight = 23.0f;
const float kStatusBarHeightPadding = 2.0f; // ImGui seems to be adding th

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

	// Store device reference for swap chain resize operations
	dx12::Device *device = nullptr;

	// Grid settings window state
	bool showGridSettingsWindow = false;

	// Camera settings window state
	bool showCameraSettingsWindow = false;

	// Scene Operations state
	ecs::Scene *scene = nullptr;
	systems::SystemManager *systemManager = nullptr;
	assets::AssetManager *assetManager = nullptr;
	engine::GPUResourceManager *gpuManager = nullptr;

	std::string currentScenePath;
	std::string lastError;

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

	// Render camera settings window
	void renderCameraSettingsWindow();

	// Render status bar
	void renderStatusBar( UI &ui );

	// Initialize viewports with D3D12 device
	bool initializeViewports( std::shared_ptr<shader_manager::ShaderManager> shaderManager );
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

bool UI::initialize( void *window_handle, dx12::Device *device, std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	HWND hwnd = static_cast<HWND>( window_handle );

	// Basic validation to allow safe negative-path unit tests without crashing inside backends
	if ( !hwnd || !device )
	{
		return false;
	}

	// Store device reference for later use (e.g., swap chain resizing)
	m_impl->device = device;

	// Initialize viewports with D3D12 device and shader manager first
	if ( !m_impl->initializeViewports( shaderManager ) )
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
	ImGuiStyle &style = ImGui::GetStyle();

	// When viewports are enabled, adjust window rounding to be compatible with OS decorations
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

	// Render all viewports to their render targets first
	m_impl->viewportManager.render();

	// Setup the main dockspace and render viewports
	m_impl->setupDockspace( m_layout, *this );
	m_impl->renderViewportWindows( m_layout );

	// Render grid settings window if open
	m_impl->renderGridSettingsWindow();

	// Render camera settings window if open
	m_impl->renderCameraSettingsWindow();
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

	// Create the dockspace with space reserved for status bar
	const ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	const ImVec2 dockspaceSize = ImVec2( availableRegion.x, availableRegion.y - ( kStatusBarHeight + 2 * kStatusBarHeightPadding ) );

	dockspaceId = ImGui::GetID( "LevelEditorDockspace" );
	ImGui::DockSpace( dockspaceId, dockspaceSize, ImGuiDockNodeFlags_None );

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
			{
				// Clear current scene
				ui.clearScene();
			}
			if ( ImGui::MenuItem( "Open Scene..." ) )
			{
				// Open file dialog for scene loading
				ui.openFileDialog();
			}
			if ( ImGui::MenuItem( "Save" ) )
			{ /* TODO: Implement scene saving */
			}
			ImGui::Separator();
			if ( ImGui::MenuItem( "Clear Scene" ) )
			{
				// Clear current scene
				ui.clearScene();
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
			if ( ImGui::MenuItem( "Camera Settings" ) )
			{
				showCameraSettingsWindow = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Render status bar at the bottom of the dockspace
	renderStatusBar( ui );
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

			// Viewport is active if it's open (visible), regardless of focus
			viewport->setActive( pane.isOpen && isValid );

			// Focus is separate from active state - only one viewport can have focus
			const bool hasFocus = ImGui::IsWindowFocused();
			viewport->setFocused( hasFocus );

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

			if ( ImGui::SliderFloat( "Fade Distance Multiplier", &gridSettings.fadeDistanceMultiplier, 1.0f, 100.0f, "%.1f" ) )
				settingsChanged = true;

			if ( ImGui::SliderFloat( "Zoom Threshold", &gridSettings.zoomThreshold, 0.01f, 1.0f, "%.3f" ) )
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

void UI::Impl::renderCameraSettingsWindow()
{
	if ( !showCameraSettingsWindow )
		return;

	if ( ImGui::Begin( "Camera Settings", &showCameraSettingsWindow ) )
	{
		// Get the focused viewport or default to perspective
		Viewport *viewport = viewportManager.getFocusedViewport();
		if ( !viewport )
		{
			viewport = getViewport( ViewportType::Perspective );
		}

		if ( viewport )
		{
			auto *controller = viewport->getController();
			if ( controller )
			{
				// Determine camera type and show appropriate settings
				auto *perspController = dynamic_cast<camera::PerspectiveCameraController *>( controller );
				auto *orthoController = dynamic_cast<camera::OrthographicCameraController *>( controller );

				if ( perspController )
				{
					ImGui::TextUnformatted( "Perspective Camera Settings" );
					ImGui::Separator();

					// Control sensitivity settings
					ImGui::SeparatorText( "Control Sensitivity" );

					float orbitSensitivity = perspController->getOrbitSensitivity();
					if ( ImGui::SliderFloat( "Orbit Sensitivity", &orbitSensitivity, 0.1f, 2.0f, "%.2f" ) )
					{
						perspController->setOrbitSensitivity( orbitSensitivity );
					}

					float panSensitivity = perspController->getPanSensitivity();
					if ( ImGui::SliderFloat( "Pan Sensitivity", &panSensitivity, 0.1f, 5.0f, "%.2f" ) )
					{
						perspController->setPanSensitivity( panSensitivity );
					}

					float zoomSensitivity = perspController->getZoomSensitivity();
					if ( ImGui::SliderFloat( "Zoom Sensitivity", &zoomSensitivity, 0.1f, 3.0f, "%.2f" ) )
					{
						perspController->setZoomSensitivity( zoomSensitivity );
					}

					// Keyboard movement settings
					ImGui::SeparatorText( "Keyboard Navigation" );

					float keyboardSpeed = perspController->getKeyboardMoveSpeed();
					if ( ImGui::SliderFloat( "Move Speed", &keyboardSpeed, 1.0f, 50.0f, "%.1f" ) )
					{
						perspController->setKeyboardMoveSpeed( keyboardSpeed );
					}

					// Auto-rotation settings
					ImGui::SeparatorText( "Auto-Rotation (Demo Mode)" );

					bool autoRotate = perspController->getAutoRotate();
					if ( ImGui::Checkbox( "Enable Auto-Rotation", &autoRotate ) )
					{
						perspController->setAutoRotate( autoRotate );
					}

					if ( autoRotate )
					{
						float autoRotateSpeed = perspController->getAutoRotateSpeed();
						if ( ImGui::SliderFloat( "Auto-Rotation Speed", &autoRotateSpeed, 1.0f, 180.0f, "%.1f °/sec" ) )
						{
							perspController->setAutoRotateSpeed( autoRotateSpeed );
						}
					}

					// Focus actions
					ImGui::SeparatorText( "Focus Actions" );

					if ( ImGui::Button( "Focus on Origin" ) )
					{
						perspController->focusOnPoint( math::Vec3<>{ 0.0f, 0.0f, 0.0f }, 10.0f );
					}

					ImGui::SameLine();

					if ( ImGui::Button( "Focus on Scene" ) )
					{
						// Focus on a typical scene bounds
						math::Vec3<> center{ 0.0f, 0.0f, 0.0f };
						math::Vec3<> size{ 20.0f, 20.0f, 20.0f };
						perspController->focusOnBounds( center, size );
					}
				}
				else if ( orthoController )
				{
					ImGui::TextUnformatted( "Orthographic Camera Settings" );
					ImGui::Separator();

					// Control sensitivity settings
					ImGui::SeparatorText( "Control Sensitivity" );

					float panSensitivity = orthoController->getPanSensitivity();
					if ( ImGui::SliderFloat( "Pan Sensitivity", &panSensitivity, 0.1f, 5.0f, "%.2f" ) )
					{
						orthoController->setPanSensitivity( panSensitivity );
					}

					float zoomSensitivity = orthoController->getZoomSensitivity();
					if ( ImGui::SliderFloat( "Zoom Sensitivity", &zoomSensitivity, 0.1f, 3.0f, "%.2f" ) )
					{
						orthoController->setZoomSensitivity( zoomSensitivity );
					}

					// Zoom limits
					ImGui::SeparatorText( "Zoom Limits" );

					float minZoom = orthoController->getMinZoom();
					float maxZoom = orthoController->getMaxZoom();

					ImGui::TextDisabled( "Min Zoom: %.3f", minZoom );
					ImGui::TextDisabled( "Max Zoom: %.3f", maxZoom );

					if ( ImGui::Button( "Reset Zoom Limits" ) )
					{
						orthoController->setZoomLimits( 0.1f, 1000.0f );
					}

					// Frame actions
					ImGui::SeparatorText( "Frame Actions" );

					if ( ImGui::Button( "Frame Origin" ) )
					{
						math::Vec3<> center{ 0.0f, 0.0f, 0.0f };
						math::Vec3<> size{ 10.0f, 10.0f, 10.0f };
						orthoController->frameBounds( center, size );
					}

					ImGui::SameLine();

					if ( ImGui::Button( "Frame Scene" ) )
					{
						math::Vec3<> center{ 0.0f, 0.0f, 0.0f };
						math::Vec3<> size{ 50.0f, 50.0f, 50.0f };
						orthoController->frameBounds( center, size );
					}
				}
				else
				{
					ImGui::TextUnformatted( "Unknown camera controller type" );
				}

				// Common settings
				ImGui::SeparatorText( "General" );

				bool enabled = controller->isEnabled();
				if ( ImGui::Checkbox( "Camera Controller Enabled", &enabled ) )
				{
					controller->setEnabled( enabled );
				}

				// Reset button
				ImGui::Separator();
				if ( ImGui::Button( "Reset to Defaults" ) )
				{
					if ( perspController )
					{
						perspController->setOrbitSensitivity( 0.5f );
						perspController->setPanSensitivity( 1.0f );
						perspController->setZoomSensitivity( 1.0f );
						perspController->setKeyboardMoveSpeed( 10.0f );
						perspController->setAutoRotate( false );
						perspController->setAutoRotateSpeed( 30.0f );
					}
					else if ( orthoController )
					{
						orthoController->setPanSensitivity( 1.0f );
						orthoController->setZoomSensitivity( 1.0f );
						orthoController->setZoomLimits( 0.1f, 1000.0f );
					}
				}

				// Apply to similar viewports
				ImGui::SameLine();
				if ( ImGui::Button( "Apply to Similar Cameras" ) )
				{
					if ( perspController )
					{
						// Apply to all perspective cameras
						const auto &viewports = viewportManager.getViewports();
						for ( const auto &vp : viewports )
						{
							if ( vp->getType() == ViewportType::Perspective )
							{
								auto *otherController = dynamic_cast<camera::PerspectiveCameraController *>( vp->getController() );
								if ( otherController && otherController != perspController )
								{
									otherController->setOrbitSensitivity( perspController->getOrbitSensitivity() );
									otherController->setPanSensitivity( perspController->getPanSensitivity() );
									otherController->setZoomSensitivity( perspController->getZoomSensitivity() );
									otherController->setKeyboardMoveSpeed( perspController->getKeyboardMoveSpeed() );
									otherController->setAutoRotate( perspController->getAutoRotate() );
									otherController->setAutoRotateSpeed( perspController->getAutoRotateSpeed() );
								}
							}
						}
					}
					else if ( orthoController )
					{
						// Apply to all orthographic cameras
						const auto &viewports = viewportManager.getViewports();
						for ( const auto &vp : viewports )
						{
							if ( vp->getType() != ViewportType::Perspective )
							{
								auto *otherController = dynamic_cast<camera::OrthographicCameraController *>( vp->getController() );
								if ( otherController && otherController != orthoController )
								{
									otherController->setPanSensitivity( orthoController->getPanSensitivity() );
									otherController->setZoomSensitivity( orthoController->getZoomSensitivity() );
									otherController->setZoomLimits( orthoController->getMinZoom(), orthoController->getMaxZoom() );
								}
							}
						}
					}
				}
			}
			else
			{
				ImGui::TextUnformatted( "No camera controller available" );
			}
		}
		else
		{
			ImGui::TextUnformatted( "No viewport available for camera settings" );
		}
	}
	ImGui::End();
}

void UI::Impl::renderStatusBar( UI &ui )
{
	// Create a child region for the status bar with styling
	ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
	ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.2f, 0.2f, 0.2f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ) );

	if ( ImGui::BeginChild( "StatusBarRegion", ImVec2( -1, kStatusBarHeight ), true, ImGuiWindowFlags_NoScrollbar ) )
	{
		// Align content vertically with some padding from the top to prevent clipping
		const float verticalPadding = 2.0f;
		const float centeredY = ( kStatusBarHeight - ImGui::GetTextLineHeight() ) * 0.5f;
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() + centeredY - verticalPadding );

		// Current scene file
		if ( !ui.getCurrentScenePath().empty() )
		{
			ImGui::Text( "Scene: %s", ui.getCurrentScenePath().c_str() );
		}
		else
		{
			ImGui::Text( "No scene loaded" );
		}

		ImGui::SameLine();
		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
		ImGui::SameLine();

		// Entity count
		const size_t entityCount = ui.getEntityCount();
		ImGui::Text( "Entities: %zu", entityCount );

		ImGui::SameLine();
		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
		ImGui::SameLine();

		// Error status
		if ( !lastError.empty() )
		{
			ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ), "Error: %s", lastError.c_str() );
		}
		else
		{
			ImGui::Text( "Ready" );
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleColor( 2 );
	ImGui::PopStyleVar( 2 );
}

// UI::Impl viewport management methods
bool UI::Impl::initializeViewports( std::shared_ptr<shader_manager::ShaderManager> shaderManager )
{
	if ( !device )
		return false;

	// Initialize viewport manager with D3D12 device and shader manager
	if ( !viewportManager.initialize( device, shaderManager ) )
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

// Camera settings window management
void UI::showCameraSettingsWindow( bool show )
{
	m_impl->showCameraSettingsWindow = show;
}

bool UI::isCameraSettingsWindowOpen() const
{
	return m_impl->showCameraSettingsWindow;
}

ViewportManager &UI::getViewportManager()
{
	return m_impl->viewportManager;
}

void UI::processInputEvents( platform::Win32Window &window )
{
	if ( !m_initialized )
		return;

	platform::WindowEvent windowEvent;
	while ( window.getEvent( windowEvent ) )
	{
		// Forward input events to viewports
		// Check if ImGui wants to capture input (will be set during UI frame)
		// For now, we'll forward all input and let the viewport system handle priority

		// Get viewport manager
		auto &viewportManager = m_impl->viewportManager;

		// Convert WindowEvent to ViewportInputEvent
		ViewportInputEvent viewportEvent;
		bool hasValidEvent = false;

		switch ( windowEvent.type )
		{
		case platform::WindowEvent::Type::MouseMove:
			viewportEvent.type = ViewportInputEvent::Type::MouseMove;
			viewportEvent.mouse.x = windowEvent.mouse.x;
			viewportEvent.mouse.y = windowEvent.mouse.y;
			viewportEvent.mouse.deltaX = windowEvent.mouse.deltaX;
			viewportEvent.mouse.deltaY = windowEvent.mouse.deltaY;
			hasValidEvent = true;
			break;

		case platform::WindowEvent::Type::MouseButton:
			viewportEvent.type = ViewportInputEvent::Type::MouseButton;
			viewportEvent.mouse.x = windowEvent.mouse.x;
			viewportEvent.mouse.y = windowEvent.mouse.y;
			viewportEvent.mouse.button = windowEvent.mouse.button;
			viewportEvent.mouse.pressed = windowEvent.mouse.pressed;
			hasValidEvent = true;
			break;

		case platform::WindowEvent::Type::MouseWheel:
			viewportEvent.type = ViewportInputEvent::Type::MouseWheel;
			viewportEvent.mouse.x = windowEvent.mouse.x;
			viewportEvent.mouse.y = windowEvent.mouse.y;
			viewportEvent.mouse.wheelDelta = windowEvent.mouse.wheelDelta;
			hasValidEvent = true;
			break;

		case platform::WindowEvent::Type::KeyPress:
			viewportEvent.type = ViewportInputEvent::Type::KeyPress;
			viewportEvent.keyboard.keyCode = windowEvent.keyboard.keycode;
			viewportEvent.keyboard.shift = windowEvent.keyboard.shift;
			viewportEvent.keyboard.ctrl = windowEvent.keyboard.ctrl;
			viewportEvent.keyboard.alt = windowEvent.keyboard.alt;
			hasValidEvent = true;
			break;

		case platform::WindowEvent::Type::KeyRelease:
			viewportEvent.type = ViewportInputEvent::Type::KeyRelease;
			viewportEvent.keyboard.keyCode = windowEvent.keyboard.keycode;
			viewportEvent.keyboard.shift = windowEvent.keyboard.shift;
			viewportEvent.keyboard.ctrl = windowEvent.keyboard.ctrl;
			viewportEvent.keyboard.alt = windowEvent.keyboard.alt;
			hasValidEvent = true;
			break;

		case platform::WindowEvent::Type::Resize:
			// Handle window resize - update ImGui display size and resize DirectX 12 resources
			{
				ImGuiIO &io = ImGui::GetIO();
				io.DisplaySize = ImVec2( static_cast<float>( windowEvent.resize.width ), static_cast<float>( windowEvent.resize.height ) );

				// Resize the DirectX 12 device resources (swap chain, depth buffer, render target views)
				// This prevents font stretching and rendering artifacts
				if ( m_impl->device )
				{
					// Reinitialize the device (this is a brute force approach)
					m_impl->device->resize(
						static_cast<UINT>( windowEvent.resize.width ),
						static_cast<UINT>( windowEvent.resize.height ) );
				}

				// Note: We don't forward resize events to viewports as ViewportInputEvent,
				// they handle their own resize through the viewport manager
			}
			break;

		default:
			// Ignore unsupported events
			break;
		}

		// Forward to focused viewport if we have a valid event
		if ( hasValidEvent )
		{
			auto *focusedViewport = viewportManager.getFocusedViewport();
			if ( focusedViewport )
			{
				focusedViewport->handleInput( viewportEvent );
			}
		}
	}
}

void UI::updateViewports( const float deltaTime )
{
	m_impl->viewportManager.update( deltaTime );
}

// Scene Operations Implementation
// Scene Operations Implementation
void UI::initializeSceneOperations( ecs::Scene &scene,
	systems::SystemManager &systemManager,
	assets::AssetManager &assetManager,
	engine::GPUResourceManager &gpuManager )
{
	m_impl->scene = &scene;
	m_impl->systemManager = &systemManager;
	m_impl->assetManager = &assetManager;
	m_impl->gpuManager = &gpuManager;
}

bool UI::loadScene( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		m_impl->lastError = "File path is empty";
		console::error( "UI: Cannot load scene - file path is empty" );
		return false;
	}

	// Check if dependencies are available
	if ( !m_impl->assetManager || !m_impl->scene || !m_impl->gpuManager )
	{
		m_impl->lastError = "Scene operation dependencies not available";
		console::error( "UI: Cannot load scene - dependencies not initialized" );
		return false;
	}

	// Clear existing scene first
	clearScene();

	// Load scene via AssetManager
	auto assetScene = m_impl->assetManager->load<assets::Scene>( filePath );
	if ( !assetScene )
	{
		m_impl->lastError = "Failed to load scene from file: " + filePath;
		console::error( "UI: Failed to load scene from file: {}", filePath );
		return false;
	}

	// Import scene into ECS
	if ( !runtime::SceneImporter::importScene( assetScene, *m_impl->scene ) )
	{
		m_impl->lastError = "Failed to import scene into ECS";
		console::error( "UI: Failed to import scene into ECS" );
		return false;
	}

	// Create GPU resources
	if ( !runtime::SceneImporter::createGPUResources( assetScene, *m_impl->scene, *m_impl->gpuManager ) )
	{
		m_impl->lastError = "Failed to create GPU resources for scene";
		console::error( "UI: Failed to create GPU resources for scene" );
		return false;
	}

	// Update current path
	m_impl->currentScenePath = filePath;
	m_impl->lastError.clear();

	console::info( "UI: Successfully loaded scene: {}", filePath );
	return true;
}

void UI::clearScene()
{
	if ( !m_impl->scene )
	{
		console::warning( "UI: Cannot clear scene - scene not available" );
		return;
	}

	// Get all entities and destroy them
	const auto entities = m_impl->scene->getAllEntities();
	for ( const auto &entity : entities )
	{
		if ( entity.isValid() )
		{
			m_impl->scene->destroyEntity( entity );
		}
	}

	m_impl->currentScenePath.clear();
	m_impl->lastError.clear();

	console::info( "UI: Scene cleared" );
}

void UI::openFileDialog()
{
	// Check if we're in test mode (no ImGui context or headless environment)
	if ( !ImGui::GetCurrentContext() )
	{
		// In test mode, don't show actual dialog to avoid blocking tests
		console::info( "UI: File dialog skipped (test mode)" );
		return;
	}

	// Use native Windows file dialog
	OPENFILENAMEA ofn;
	char szFile[260] = { 0 };

	// Initialize OPENFILENAME
	ZeroMemory( &ofn, sizeof( ofn ) );
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof( szFile );
	ofn.lpstrFilter = "glTF Files\0*.gltf;*.glb\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// Display the modal dialog box (blocks until user interaction)
	if ( GetOpenFileNameA( &ofn ) == TRUE )
	{
		// User selected a file - immediately load it
		const std::string selectedFile( szFile );
		loadScene( selectedFile );
	}
	// If GetOpenFileNameA returns FALSE, user cancelled - no action needed
}


const std::string &UI::getCurrentScenePath() const
{
	return m_impl->currentScenePath;
}

size_t UI::getEntityCount() const
{
	if ( !m_impl->scene )
	{
		return 0;
	}

	const auto entities = m_impl->scene->getAllEntities();
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

const std::string &UI::getLastError() const
{
	return m_impl->lastError;
}

} // namespace editor

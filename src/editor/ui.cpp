// Include ImGui implementation in the global module fragment to avoid conflicts
#include "imgui.h"
#include "imgui_internal.h" // For DockBuilder API
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <ImGuizmo.h>
#include <d3d12.h>
#include <windows.h>
#include <commdlg.h> // For GetOpenFileName

#include "editor/ui.h"

#include <memory>
#include <string>
#include <array>
#include <filesystem>

#include "engine/assets/asset_manager.h"
#include "engine/camera/camera.h"
#include "engine/gpu/gpu_resource_manager.h"
#include "engine/shader_manager/shader_manager.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"
#include "runtime/scene_importer.h"
#include "runtime/scene_serialization/SceneSerializer.h"
#include "runtime/console.h"
#include "platform/dx12/dx12_device.h"
#include "platform/win32/win32_window.h"
#include "editor/commands/CommandUI.h"
#include "editor/commands/EcsCommands.h"
#include "gizmos.h"
#include "editor/selection.h"
#include "editor/scene_hierarchy/SceneHierarchyPanel.h"
#include "editor/entity_inspector/EntityInspectorPanel.h"
#include "editor/asset_browser/AssetBrowserPanel.h"
#include "editor/commands/EcsCommands.h"
#include "editor/config/EditorConfig.h"

namespace editor
{
const float kStatusBarHeight = 23.0f;
const float kStatusBarHeightPadding = 2.0f; // ImGui seems to be adding th

// Config key constants for window visibility persistence
namespace ConfigKeys
{
// Panel visibility keys
static constexpr const char *kHierarchyPanelVisible = "ui.panels.hierarchy.visible";
static constexpr const char *kInspectorPanelVisible = "ui.panels.inspector.visible";
static constexpr const char *kAssetBrowserVisible = "ui.panels.assetBrowser.visible";

// Tool window visibility keys
static constexpr const char *kGridSettingsVisible = "ui.tools.gridSettings.visible";
static constexpr const char *kCameraSettingsVisible = "ui.tools.cameraSettings.visible";
static constexpr const char *kGizmoToolsVisible = "ui.tools.gizmoTools.visible";
static constexpr const char *kGizmoSettingsVisible = "ui.tools.gizmoSettings.visible";
static constexpr const char *kCommandHistoryVisible = "ui.tools.commandHistory.visible";

// Viewport visibility keys
static constexpr const char *kViewportPerspectiveOpen = "ui.viewports.perspective.open";
static constexpr const char *kViewportTopOpen = "ui.viewports.top.open";
static constexpr const char *kViewportFrontOpen = "ui.viewports.front.open";
static constexpr const char *kViewportSideOpen = "ui.viewports.side.open";
} // namespace ConfigKeys

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

	// Gizmo window states
	bool showGizmoToolsWindow = true;	 // Default to visible
	bool showGizmoSettingsWindow = true; // Default to visible

	// Scene Operations state
	ecs::Scene *scene = nullptr;
	systems::SystemManager *systemManager = nullptr;
	assets::AssetManager *assetManager = nullptr;
	engine::GPUResourceManager *gpuManager = nullptr;

	// Gizmo system for object manipulation
	std::unique_ptr<GizmoSystem> gizmoSystem;
	std::unique_ptr<GizmoUI> gizmoUI;

	// Command history and UI integration
	std::unique_ptr<CommandHistory> commandHistory;
	std::unique_ptr<UndoRedoUI> undoRedoUI;
	std::unique_ptr<CommandHistoryWindow> commandHistoryWindow;
	bool showCommandHistoryWindow = false;

	// Scene editing panels
	SelectionManager *selectionManager = nullptr;
	std::unique_ptr<SceneHierarchyPanel> hierarchyPanel;
	std::unique_ptr<EntityInspectorPanel> inspectorPanel;
	std::unique_ptr<AssetBrowserPanel> assetBrowserPanel;
	bool showHierarchyPanel = true;
	bool showInspectorPanel = true;
	bool showAssetBrowserPanel = true;

	std::string currentScenePath;
	std::string lastError;
	bool m_sceneModified = false;

	// Toast notification system (T8.7)
	struct Toast
	{
		std::string message;
		float remainingTime = 0.0f;
		bool isError = false;
	};
	std::vector<Toast> toasts;
	static constexpr float kToastDuration = 3.0f; // 3 seconds

	// Track previous frame's gizmo state for input blocking
	bool wasGizmoHoveredLastFrame = false;

	// Editor configuration persistence
	std::unique_ptr<EditorConfig> editorConfig;
	bool wasGizmoUsingLastFrame = false;

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

	// Render command history window
	void renderCommandHistoryWindow();

	// Render status bar
	void renderStatusBar( UI &ui );

	// Render main toolbar below menu bar
	void renderToolbar();

	// Render toast notifications (T8.7)
	void renderToasts();

	// Show a toast notification
	void showToast( const std::string &message, bool isError = false );

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

	// Initialize command history system
	m_impl->commandHistory = std::make_unique<CommandHistory>();
	m_impl->undoRedoUI = std::make_unique<UndoRedoUI>( *m_impl->commandHistory );
	m_impl->commandHistoryWindow = std::make_unique<CommandHistoryWindow>( *m_impl->commandHistory );

	// Initialize editor config and load saved window states
	m_impl->editorConfig = std::make_unique<EditorConfig>( "editor_config.json" );
	if ( m_impl->editorConfig->load() )
	{
		// Restore panel visibility states
		m_impl->showHierarchyPanel = m_impl->editorConfig->getBool( ConfigKeys::kHierarchyPanelVisible, true );
		m_impl->showInspectorPanel = m_impl->editorConfig->getBool( ConfigKeys::kInspectorPanelVisible, true );
		m_impl->showAssetBrowserPanel = m_impl->editorConfig->getBool( ConfigKeys::kAssetBrowserVisible, true );

		// Restore tool window visibility states
		m_impl->showGridSettingsWindow = m_impl->editorConfig->getBool( ConfigKeys::kGridSettingsVisible, false );
		m_impl->showCameraSettingsWindow = m_impl->editorConfig->getBool( ConfigKeys::kCameraSettingsVisible, false );
		m_impl->showGizmoToolsWindow = m_impl->editorConfig->getBool( ConfigKeys::kGizmoToolsVisible, true );
		m_impl->showGizmoSettingsWindow = m_impl->editorConfig->getBool( ConfigKeys::kGizmoSettingsVisible, true );
		m_impl->showCommandHistoryWindow = m_impl->editorConfig->getBool( ConfigKeys::kCommandHistoryVisible, false );

		// Restore viewport visibility states
		m_layout.panes[0].isOpen = m_impl->editorConfig->getBool( ConfigKeys::kViewportPerspectiveOpen, true );
		m_layout.panes[1].isOpen = m_impl->editorConfig->getBool( ConfigKeys::kViewportTopOpen, true );
		m_layout.panes[2].isOpen = m_impl->editorConfig->getBool( ConfigKeys::kViewportFrontOpen, true );
		m_layout.panes[3].isOpen = m_impl->editorConfig->getBool( ConfigKeys::kViewportSideOpen, true );
	}

	m_initialized = true;
	return true;
}

void UI::shutdown()
{
	if ( !m_initialized )
		return;

	// Save window visibility states to config before shutdown
	if ( m_impl->editorConfig )
	{
		// Save panel visibility states
		m_impl->editorConfig->setBool( ConfigKeys::kHierarchyPanelVisible, m_impl->showHierarchyPanel );
		m_impl->editorConfig->setBool( ConfigKeys::kInspectorPanelVisible, m_impl->showInspectorPanel );
		m_impl->editorConfig->setBool( ConfigKeys::kAssetBrowserVisible, m_impl->showAssetBrowserPanel );

		// Save tool window visibility states
		m_impl->editorConfig->setBool( ConfigKeys::kGridSettingsVisible, m_impl->showGridSettingsWindow );
		m_impl->editorConfig->setBool( ConfigKeys::kCameraSettingsVisible, m_impl->showCameraSettingsWindow );
		m_impl->editorConfig->setBool( ConfigKeys::kGizmoToolsVisible, m_impl->showGizmoToolsWindow );
		m_impl->editorConfig->setBool( ConfigKeys::kGizmoSettingsVisible, m_impl->showGizmoSettingsWindow );
		m_impl->editorConfig->setBool( ConfigKeys::kCommandHistoryVisible, m_impl->showCommandHistoryWindow );

		// Save viewport visibility states
		m_impl->editorConfig->setBool( ConfigKeys::kViewportPerspectiveOpen, m_layout.panes[0].isOpen );
		m_impl->editorConfig->setBool( ConfigKeys::kViewportTopOpen, m_layout.panes[1].isOpen );
		m_impl->editorConfig->setBool( ConfigKeys::kViewportFrontOpen, m_layout.panes[2].isOpen );
		m_impl->editorConfig->setBool( ConfigKeys::kViewportSideOpen, m_layout.panes[3].isOpen );

		// Write config to disk
		m_impl->editorConfig->save();
	}

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

	// Check for Alt-F4 keyboard shortcut
	const ImGuiIO &io = ImGui::GetIO();
	if ( io.KeyAlt && ImGui::IsKeyPressed( ImGuiKey_F4, false ) )
	{
		m_shouldExit = true;
	}

	// Initialize ImGuizmo context for this frame
	ImGuizmo::BeginFrame();

	// Render all viewports to their render targets first
	m_impl->viewportManager.render();

	// Setup the main dockspace and render viewports
	m_impl->setupDockspace( m_layout, *this );
	m_impl->renderViewportWindows( m_layout );

	// Render grid settings window if open
	m_impl->renderGridSettingsWindow();

	// Render camera settings window if open
	m_impl->renderCameraSettingsWindow();

	// Render command history window if open
	m_impl->renderCommandHistoryWindow();

	// Render toolbar below menu bar
	m_impl->renderToolbar();

	// Render scene editing panels
	if ( m_impl->hierarchyPanel && m_impl->showHierarchyPanel )
	{
		m_impl->hierarchyPanel->render();
	}
	if ( m_impl->inspectorPanel && m_impl->showInspectorPanel )
	{
		m_impl->inspectorPanel->render();
	}
	if ( m_impl->assetBrowserPanel && m_impl->showAssetBrowserPanel )
	{
		m_impl->assetBrowserPanel->render();
	}

	// Render toast notifications (T8.7)
	m_impl->renderToasts();
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

	// Set window title with modified indicator
	std::string windowTitle = "Level Editor Dockspace";
	if ( m_sceneModified )
	{
		windowTitle += " *";
	}

	ImGui::Begin( windowTitle.c_str(), nullptr, window_flags );
	ImGui::PopStyleVar( 3 );

	// Reserve space for toolbar and status bar
	const float kToolbarHeight = 32.0f;
	const ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	const ImVec2 dockspaceSize = ImVec2( availableRegion.x, availableRegion.y - ( kToolbarHeight + kStatusBarHeight + 2 * kStatusBarHeightPadding ) );

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
				// Check for unsaved changes
				if ( m_sceneModified )
				{
					// Show warning popup (will be handled in next frame)
					ImGui::OpenPopup( "Unsaved Changes##New" );
				}
				else
				{
					// Clear current scene
					ui.clearScene();
					m_sceneModified = false;
				}
			}

			// Warning dialog for New Scene
			if ( ImGui::BeginPopupModal( "Unsaved Changes##New", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
			{
				ImGui::Text( "The current scene has unsaved changes." );
				ImGui::Text( "Do you want to save before creating a new scene?" );
				ImGui::Separator();

				if ( ImGui::Button( "Save", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
					// If we have a current scene path, save it; otherwise open Save As dialog
					if ( !ui.getCurrentScenePath().empty() )
					{
						ui.saveScene( ui.getCurrentScenePath() );
						ui.newScene();
					}
					else
					{
						ui.openSaveFileDialog();
						ui.newScene();
					}
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Don't Save", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
					ui.newScene();
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Cancel", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if ( ImGui::MenuItem( "Open Scene..." ) )
			{
				// Check for unsaved changes
				if ( m_sceneModified )
				{
					ImGui::OpenPopup( "Unsaved Changes##Open" );
				}
				else
				{
					// Open file dialog for scene loading
					ui.openFileDialog();
				}
			}

			// Warning dialog for Open Scene
			if ( ImGui::BeginPopupModal( "Unsaved Changes##Open", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
			{
				ImGui::Text( "The current scene has unsaved changes." );
				ImGui::Text( "Do you want to save before opening a different scene?" );
				ImGui::Separator();

				if ( ImGui::Button( "Save", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
					// If we have a current scene path, save it; otherwise open Save As dialog
					if ( !ui.getCurrentScenePath().empty() )
					{
						ui.saveScene( ui.getCurrentScenePath() );
					}
					else
					{
						ui.openSaveFileDialog();
					}
					ui.openFileDialog();
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Don't Save", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
					ui.openFileDialog();
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Cancel", ImVec2( 120, 0 ) ) )
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if ( ImGui::MenuItem( "Save", "Ctrl+S" ) )
			{
				// If we have a current scene path, save to it; otherwise open Save As dialog
				if ( !ui.getCurrentScenePath().empty() )
				{
					ui.saveScene( ui.getCurrentScenePath() );
				}
				else
				{
					ui.openSaveFileDialog();
				}
			}
			if ( ImGui::MenuItem( "Save As...", "Ctrl+Shift+S" ) )
			{
				ui.openSaveFileDialog();
			}
			ImGui::Separator();
			if ( ImGui::MenuItem( "Clear Scene" ) )
			{
				// Clear current scene
				ui.clearScene();
			}
			ImGui::Separator();
			if ( ImGui::MenuItem( "Exit", "Alt+F4" ) )
			{
				// Set the exit flag when user selects Exit from menu
				ui.m_shouldExit = true;
			}
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Edit" ) )
		{
			// Render undo/redo menu items
			if ( undoRedoUI )
			{
				undoRedoUI->renderMenuItems();
			}

			ImGui::Separator();

			// Entity operations (require scene and selection manager)
			if ( scene && selectionManager )
			{
				ImGui::SeparatorText( "Entity" );

				// Create Entity submenu (T8.5)
				if ( ImGui::BeginMenu( "Create Entity" ) )
				{
					// Create Empty Entity operation
					if ( ImGui::MenuItem( "Empty Entity", "Ins" ) )
					{
						// Create a new entity at world origin
						auto cmd = std::make_unique<CreateEntityCommand>( *scene, "New Entity" );
						if ( commandHistory )
						{
							if ( commandHistory->executeCommand( std::move( cmd ) ) )
							{
								m_sceneModified = true;
							}
						}
					}

					// Create Entity from Asset File operation
					if ( ImGui::MenuItem( "From Asset File...", "Ctrl+Shift+N" ) )
					{
						ui.openAssetFileDialog();
					}

					ImGui::EndMenu();
				}

				const bool hasSelection = selectionManager && !selectionManager->getSelectedEntities().empty();

				// Delete Entity operation (requires selection)
				if ( ImGui::MenuItem( "Delete Entity", "Del", false, hasSelection ) )
				{
					// Delete all selected entities
					const auto selected = selectionManager->getSelectedEntities();
					for ( const auto entity : selected )
					{
						auto cmd = std::make_unique<DeleteEntityCommand>( *scene, entity );
						if ( commandHistory )
						{
							if ( commandHistory->executeCommand( std::move( cmd ) ) )
							{
								m_sceneModified = true;
							}
						}
					}
				}

				// Duplicate Entity operation (requires selection) - TODO: Implement DuplicateEntityCommand
				if ( ImGui::MenuItem( "Duplicate Entity", "Ctrl+D", false, false ) ) // Disabled for now
				{
					// TODO: Implement DuplicateEntityCommand
				}
			}

			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "View" ) )
		{
			// Viewport visibility toggles
			ImGui::SeparatorText( "Viewports" );
			for ( auto &pane : layout.panes )
			{
				ImGui::Checkbox( pane.name, &pane.isOpen );
			}

			ImGui::Separator();

			// Scene editing panel visibility toggles
			ImGui::SeparatorText( "Panels" );
			if ( ImGui::MenuItem( "Scene Hierarchy", nullptr, showHierarchyPanel ) )
			{
				showHierarchyPanel = !showHierarchyPanel;
			}
			if ( ImGui::MenuItem( "Entity Inspector", nullptr, showInspectorPanel ) )
			{
				showInspectorPanel = !showInspectorPanel;
			}
			if ( ImGui::MenuItem( "Asset Browser", nullptr, showAssetBrowserPanel ) )
			{
				showAssetBrowserPanel = !showAssetBrowserPanel;
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

			ImGui::Separator();

			if ( ImGui::MenuItem( "Gizmo Tools" ) )
			{
				showGizmoToolsWindow = true;
			}
			if ( ImGui::MenuItem( "Gizmo Settings" ) )
			{
				showGizmoSettingsWindow = true;
			}

			ImGui::Separator();

			if ( ImGui::MenuItem( "Command History" ) )
			{
				showCommandHistoryWindow = true;
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

		// Create a layout with scene editing panels and viewports:
		// +----------+------------------+------------------+----------+
		// |          |   Perspective    |    Top (XY)      |          |
		// | Hierarchy|      3D          |                  | Inspector|
		// |  (20%)   +------------------+------------------+  (25%)   |
		// |          |   Front (XZ)     |    Side (YZ)     |          |
		// +----------+------------------+------------------+----------+
		// |                  Asset Browser (30%)                     |
		// +--------------------------------------------------------------+

		ImGuiID dockLeft, dockMainArea;
		ImGuiID dockRight, dockCenterArea;
		ImGuiID dockBottom, dockViewportArea;
		ImGuiID dockLeftColumn, dockRightColumn;
		ImGuiID dockTopLeft, dockBottomLeft;
		ImGuiID dockTopRight, dockBottomRight;

		// First split: hierarchy panel on left (20%)
		ImGui::DockBuilderSplitNode( inDockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockMainArea );

		// Second split: inspector panel on right (25% of remaining)
		ImGui::DockBuilderSplitNode( dockMainArea, ImGuiDir_Right, 0.3125f, &dockRight, &dockCenterArea ); // 0.25/0.80 ≈ 0.3125

		// Third split: asset browser on bottom (30% of center area)
		ImGui::DockBuilderSplitNode( dockCenterArea, ImGuiDir_Down, 0.30f, &dockBottom, &dockViewportArea );

		// Split viewport area into 2x2 grid for four viewports
		ImGui::DockBuilderSplitNode( dockViewportArea, ImGuiDir_Left, 0.5f, &dockLeftColumn, &dockRightColumn );
		ImGui::DockBuilderSplitNode( dockLeftColumn, ImGuiDir_Up, 0.5f, &dockTopLeft, &dockBottomLeft );
		ImGui::DockBuilderSplitNode( dockRightColumn, ImGuiDir_Up, 0.5f, &dockTopRight, &dockBottomRight );

		// Dock scene editing panels
		ImGui::DockBuilderDockWindow( "Scene Hierarchy", dockLeft );
		ImGui::DockBuilderDockWindow( "Entity Inspector", dockRight );
		ImGui::DockBuilderDockWindow( "Asset Browser", dockBottom );

		// Dock viewport windows
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
	const ImVec2 windowPos = ImGui::GetWindowPos(); // Window position in screen space

	ImGui::SetNextWindowSizeConstraints( to_imgui_vec2( pane.minSize ), ImVec2( FLT_MAX, FLT_MAX ) );

	if ( ImGui::Begin( pane.name, const_cast<bool *>( &pane.isOpen ) ) )
	{
		// Get ImGui content positioning information for coordinate conversion
		// Use recommended functions instead of GetWindowPos which is in screen space
		const ImVec2 contentRegionMin = ImGui::GetCursorScreenPos(); // Top-left of content area in screen space

		// Calculate content offset relative to window
		const ImVec2 offsetFromWindow = ImVec2(
			contentRegionMin.x - windowPos.x,
			contentRegionMin.y - windowPos.y );

		// Get the content region size
		const ImVec2 contentSize = ImGui::GetContentRegionAvail();

		// Get the corresponding viewport
		Viewport *viewport = getViewport( pane.type );

		if ( viewport )
		{
			viewport->setOffsetFromWindow( math::Vec2<>{ offsetFromWindow.x, offsetFromWindow.y } );

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

			// Update focused viewport through ViewportManager for proper input routing
			if ( hasFocus && viewportManager.getFocusedViewport() != viewport )
			{
				viewportManager.setFocusedViewport( viewport );
			}

			// Get render target texture for ImGui rendering
			void *textureHandle = viewport->getImGuiTextureId();

			if ( isValid && textureHandle )
			{
				// Render the actual 3D viewport content
				ImGui::Image( reinterpret_cast<ImTextureID>( textureHandle ), contentSize );

				// Handle drag-drop for asset instantiation (T8.2)
				if ( ImGui::BeginDragDropTarget() )
				{
					// Visual feedback: highlight viewport border when dragging asset (T8.7)
					if ( const ImGuiPayload *payload = ImGui::GetDragDropPayload() )
					{
						if ( payload->IsDataType( "ASSET_BROWSER_ITEM" ) )
						{
							// Draw highlight border around viewport
							const ImVec2 minPos = ImGui::GetItemRectMin();
							const ImVec2 maxPos = ImGui::GetItemRectMax();
							ImGui::GetWindowDrawList()->AddRect(
								minPos, maxPos, IM_COL32( 100, 200, 255, 200 ), // Light blue highlight
								0.0f,
								0,
								3.0f // No rounding, all corners, 3px thickness
							);
						}
					}

					if ( const ImGuiPayload *payload = ImGui::AcceptDragDropPayload( "ASSET_BROWSER_ITEM" ) )
					{
						// Extract asset path from payload
						const char *assetPath = static_cast<const char *>( payload->Data );

						// Create entity from asset at world origin (0, 0, 0)
						// Future enhancement: Use ray-cast to find drop position in 3D space
						const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

						if ( scene && assetManager && commandHistory && gpuManager )
						{
							auto cmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
								*scene, *assetManager, *gpuManager, assetPath, worldPosition );

							if ( commandHistory->executeCommand( std::move( cmd ) ) )
							{
								// Entity created successfully
								console::info( "Created entity from asset: {}", assetPath );
								showToast( "Entity created from asset", false );
							}
							else
							{
								// Failed to create entity
								console::error( "Failed to create entity from asset: {}", assetPath );
								showToast( "Failed to create entity from asset", true );
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// Render gizmos on top of the viewport if gizmo system is available
				if ( gizmoSystem && scene && viewport->areGizmosVisible() && gizmoSystem->isVisible() )
				{
					// Setup ImGuizmo for this viewport
					if ( auto *camera = viewport->getCamera() )
					{
						const auto viewMatrix = camera->getViewMatrix();
						const auto projMatrix = camera->getProjectionMatrix( viewport->getAspectRatio() );

						// Get absolute screen coordinates for ImGuizmo viewport
						const ImVec2 viewportWindowPos = ImGui::GetWindowPos();
						const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
						const float absoluteX = viewportWindowPos.x + contentMin.x;
						const float absoluteY = viewportWindowPos.y + contentMin.y;
						const math::Vec4<> viewportRect{ absoluteX, absoluteY, static_cast<float>( contentSize.x ), static_cast<float>( contentSize.y ) };

						// Set ImGuizmo to draw to the current window's draw list
						ImGuizmo::SetDrawlist();

						// Setup ImGuizmo context and render gizmos
						if ( gizmoSystem->setupImGuizmo( viewMatrix, projMatrix, viewportRect ) )
						{
							const auto gizmoResult = gizmoSystem->renderGizmo();

							// Get current mouse position for bounds checking
							const ImVec2 mousePos = ImGui::GetIO().MousePos;
							const math::Vec2<> windowMousePos{ mousePos.x, mousePos.y };

							// Only update gizmo state when mouse is within this viewport
							if ( viewport->isPointInViewport( windowMousePos ) )
							{
								wasGizmoHoveredLastFrame = gizmoResult.isHovered;
								wasGizmoUsingLastFrame = gizmoResult.isManipulating;
							}

							// Only apply transform changes when viewport has focus to prevent unintended modifications
							if ( hasFocus && gizmoResult.wasManipulated )
							{
								// Apply transform changes to selected entities
								gizmoSystem->applyTransformDelta( gizmoResult );
							}
						}
					}
				}
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

void UI::Impl::renderCommandHistoryWindow()
{
	if ( !showCommandHistoryWindow )
		return;

	if ( commandHistoryWindow )
	{
		commandHistoryWindow->render( &showCommandHistoryWindow );
	}
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

		// Selection count
		if ( selectionManager )
		{
			const size_t selectionCount = selectionManager->getSelectedEntities().size();
			if ( selectionCount > 0 )
			{
				ImGui::Text( "Selected: %zu", selectionCount );
			}
			else
			{
				ImGui::Text( "Selected: None" );
			}

			ImGui::SameLine();
			ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
			ImGui::SameLine();
		}

		// Gizmo mode and space
		if ( gizmoSystem )
		{
			const char *modeName = "None";
			switch ( gizmoSystem->getCurrentOperation() )
			{
			case editor::GizmoOperation::Translate:
				modeName = "Translate";
				break;
			case editor::GizmoOperation::Rotate:
				modeName = "Rotate";
				break;
			case editor::GizmoOperation::Scale:
				modeName = "Scale";
				break;
			case editor::GizmoOperation::Universal:
				modeName = "Universal";
				break;
			default:
				break;
			}

			const char *spaceName = ( gizmoSystem->getCurrentMode() == editor::GizmoMode::Local ) ? "Local" : "World";
			ImGui::Text( "Gizmo: %s (%s)", modeName, spaceName );

			ImGui::SameLine();
			ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
			ImGui::SameLine();
		}

		// Frame time and FPS
		const float frameTime = ImGui::GetIO().DeltaTime * 1000.0f; // Convert to milliseconds
		const float fps = ImGui::GetIO().Framerate;
		ImGui::Text( "%.1f ms (%.0f FPS)", frameTime, fps );

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

void UI::Impl::renderToolbar()
{
	// Use the new visibility-controlled methods
	if ( gizmoUI )
	{
		gizmoUI->renderToolbar( &showGizmoToolsWindow );
		gizmoUI->renderSettings( &showGizmoSettingsWindow );
	}
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

void UI::Impl::renderToasts()
{
	// Update and render toast notifications (T8.7)
	const float deltaTime = ImGui::GetIO().DeltaTime;
	const ImGuiViewport *viewport = ImGui::GetMainViewport();

	// Position toasts in bottom-right corner
	const float padding = 20.0f;
	const float toastWidth = 300.0f;
	const float toastHeight = 60.0f;
	float yOffset = viewport->WorkSize.y - padding;

	// Update timers and render active toasts
	for ( auto it = toasts.begin(); it != toasts.end(); )
	{
		it->remainingTime -= deltaTime;

		if ( it->remainingTime <= 0.0f )
		{
			// Remove expired toast
			it = toasts.erase( it );
			continue;
		}

		// Calculate position for this toast
		const float xPos = viewport->WorkPos.x + viewport->WorkSize.x - toastWidth - padding;
		const float yPos = viewport->WorkPos.y + yOffset - toastHeight;
		yOffset -= ( toastHeight + 10.0f ); // Stack toasts vertically

		// Set window position and size
		ImGui::SetNextWindowPos( ImVec2( xPos, yPos ), ImGuiCond_Always );
		ImGui::SetNextWindowSize( ImVec2( toastWidth, toastHeight ), ImGuiCond_Always );

		// Calculate fade alpha based on remaining time
		const float fadeTime = 0.5f;
		const float alpha = ( it->remainingTime < fadeTime ) ? ( it->remainingTime / fadeTime ) : 1.0f;

		// Style the toast window
		ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 5.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10.0f, 10.0f ) );
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.2f, 0.2f, 0.2f, alpha * 0.95f ) );
		ImGui::PushStyleColor( ImGuiCol_Border, it->isError ? ImVec4( 1.0f, 0.3f, 0.3f, alpha ) : ImVec4( 0.3f, 0.7f, 1.0f, alpha ) );

		// Unique window name for each toast
		const std::string windowName = "##toast_" + std::to_string( reinterpret_cast<uintptr_t>( &( *it ) ) );

		if ( ImGui::Begin( windowName.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus ) )
		{
			// Icon and message
			const char *icon = it->isError ? "  ✖" : "  ℹ";
			const ImVec4 iconColor = it->isError ? ImVec4( 1.0f, 0.4f, 0.4f, alpha ) : ImVec4( 0.4f, 0.8f, 1.0f, alpha );

			ImGui::PushStyleColor( ImGuiCol_Text, iconColor );
			ImGui::Text( "%s", icon );
			ImGui::PopStyleColor();

			ImGui::SameLine();
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, alpha ) );
			ImGui::TextWrapped( "%s", it->message.c_str() );
			ImGui::PopStyleColor();

			ImGui::End();
		}

		ImGui::PopStyleColor( 2 );
		ImGui::PopStyleVar( 2 );

		++it;
	}
}

void UI::Impl::showToast( const std::string &message, bool isError )
{
	// Add new toast to the queue
	Toast toast;
	toast.message = message;
	toast.remainingTime = kToastDuration;
	toast.isError = isError;
	toasts.push_back( toast );

	// Limit to 5 toasts at once
	if ( toasts.size() > 5 )
	{
		toasts.erase( toasts.begin() );
	}
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

// Gizmo windows management
void UI::showGizmoToolsWindow( bool show )
{
	m_impl->showGizmoToolsWindow = show;
}

bool UI::isGizmoToolsWindowOpen() const
{
	return m_impl->showGizmoToolsWindow;
}

void UI::showGizmoSettingsWindow( bool show )
{
	m_impl->showGizmoSettingsWindow = show;
}

bool UI::isGizmoSettingsWindowOpen() const
{
	return m_impl->showGizmoSettingsWindow;
}

ViewportManager &UI::getViewportManager()
{
	return m_impl->viewportManager;
}

GizmoSystem *UI::getGizmoSystem()
{
	return m_impl->gizmoSystem.get();
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
				// Check if UI (including ImGuizmo) wants to capture input to prevent camera interference
				bool shouldBlockCameraInput = false;

				// Block mouse events when UI systems (ImGui/ImGuizmo) want to capture mouse input
				if ( viewportEvent.type == ViewportInputEvent::Type::MouseButton ||
					viewportEvent.type == ViewportInputEvent::Type::MouseMove ||
					viewportEvent.type == ViewportInputEvent::Type::MouseWheel )
				{
					// Block input when gizmo is being actively used OR when gizmo was hovered last frame
					// This prevents both active manipulation interference and initial mouse press issues
					shouldBlockCameraInput = ImGuizmo::IsUsing() || m_impl->wasGizmoHoveredLastFrame;
				}

				// Block keyboard events for gizmo shortcut keys ONLY when gizmos are active
				if ( viewportEvent.type == ViewportInputEvent::Type::KeyPress )
				{
					const char key = static_cast<char>( viewportEvent.keyboard.keyCode );
					// Only block gizmo shortcut keys when gizmos are visible and there's a valid selection
					const bool gizmosAreActive = m_impl->gizmoSystem &&
						m_impl->gizmoSystem->isVisible() &&
						m_impl->gizmoSystem->hasValidSelection();

					if ( gizmosAreActive && ( key == 'W' || key == 'E' || key == 'R' || key == 'X' || key == 'G' ) )
					{
						shouldBlockCameraInput = true;
					}
				}

				// Forward the event to the viewport unless UI is capturing input
				if ( !shouldBlockCameraInput )
				{
					focusedViewport->handleInput( viewportEvent );
				}
			}
		}
	}

	// Handle gizmo keyboard shortcuts
	if ( m_impl->gizmoUI )
	{
		m_impl->gizmoUI->handleKeyboardShortcuts();
	}

	// Handle undo/redo keyboard shortcuts
	if ( m_impl->undoRedoUI )
	{
		m_impl->undoRedoUI->handleKeyboardShortcuts();
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
	engine::GPUResourceManager &gpuManager,
	editor::SelectionManager &selectionManager )
{
	m_impl->scene = &scene;
	m_impl->systemManager = &systemManager;
	m_impl->assetManager = &assetManager;
	m_impl->gpuManager = &gpuManager;
	m_impl->selectionManager = &selectionManager;

	// Create GizmoSystem with SelectionManager, Scene, SystemManager, and CommandHistory
	m_impl->gizmoSystem = std::make_unique<GizmoSystem>( selectionManager, scene, systemManager, m_impl->commandHistory.get() );

	// Create GizmoUI with GizmoSystem dependency
	m_impl->gizmoUI = std::make_unique<GizmoUI>( *m_impl->gizmoSystem );

	// Create scene editing panels
	m_impl->hierarchyPanel = std::make_unique<SceneHierarchyPanel>(
		scene, selectionManager, *m_impl->commandHistory, &assetManager, m_impl->gpuManager );
	m_impl->inspectorPanel = std::make_unique<EntityInspectorPanel>(
		scene, selectionManager, *m_impl->commandHistory );
	m_impl->assetBrowserPanel = std::make_unique<AssetBrowserPanel>(
		assetManager, scene, *m_impl->commandHistory );

	// Configure asset browser root path
	m_impl->assetBrowserPanel->setRootPath( "assets/" );

	// Wire focus callback for hierarchy panel to focus camera on entity
	m_impl->hierarchyPanel->setFocusCallback( [this]( ecs::Entity entity ) {
		// Get the perspective viewport
		auto *viewport = getViewport( ViewportType::Perspective );
		if ( !viewport )
			return;

		// Get the camera controller
		auto *controller = viewport->getController();
		if ( !controller )
			return;

		// Try to cast to perspective camera controller for focusOnPoint
		auto *perspController = dynamic_cast<camera::PerspectiveCameraController *>( controller );
		if ( !perspController )
			return;

		// Get entity transform to determine focus point
		if ( !m_impl->scene->hasComponent<components::Transform>( entity ) )
			return;

		const auto *transform = m_impl->scene->getComponent<components::Transform>( entity );
		if ( !transform )
			return;

		// Focus camera on entity position with appropriate distance
		const float focusDistance = 10.0f;
		perspController->focusOnPoint( transform->position, focusDistance );
	} );
}

bool UI::loadScene( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		m_impl->lastError = "File path is empty";
		console::error( "UI: Cannot load scene - file path is empty" );
		return false;
	}

	// Check if scene is available
	if ( !m_impl->scene )
	{
		m_impl->lastError = "Scene not available";
		console::error( "UI: Cannot load scene - scene not initialized" );
		return false;
	}

	// Clear existing scene first
	clearScene();

	// Load scene using SceneSerializer
	const auto result = scene::SceneSerializer::loadScene( *m_impl->scene, filePath );
	if ( !result )
	{
		// Extract error message from SerializationError enum
		std::string errorMsg;
		switch ( result.error().error )
		{
		case scene::SerializationError::FileNotFound:
			errorMsg = "Scene file not found";
			break;
		case scene::SerializationError::InvalidJSON:
			errorMsg = "Invalid or corrupt scene file";
			break;
		case scene::SerializationError::UnsupportedVersion:
			errorMsg = "Unsupported scene file version";
			break;
		default:
			errorMsg = "Unknown error occurred";
			break;
		}

		m_impl->lastError = errorMsg + ": " + filePath;
		console::error( "UI: Failed to load scene: {}", m_impl->lastError );
		return false;
	}

	// Update current path and clear modified flag
	m_impl->currentScenePath = filePath;
	m_impl->lastError.clear();
	m_impl->m_sceneModified = false;

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

	// Queue GPU resources for deferred deletion before destroying entities
	if ( m_impl->gpuManager )
	{
		const auto entities = m_impl->scene->getAllEntities();
		for ( const auto &entity : entities )
		{
			if ( entity.isValid() && m_impl->scene->hasComponent<components::MeshRenderer>( entity ) )
			{
				const auto *meshRenderer = m_impl->scene->getComponent<components::MeshRenderer>( entity );
				if ( meshRenderer && meshRenderer->gpuMesh )
				{
					// Queue the GPU mesh for deferred deletion
					m_impl->gpuManager->queueForDeletion( meshRenderer->gpuMesh );
				}
			}
		}
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
	m_impl->m_sceneModified = false;
}

void UI::newScene()
{
	// Clear the current scene
	clearScene();

	// Reset scene path
	m_impl->currentScenePath.clear();
	m_impl->lastError.clear();
	m_impl->m_sceneModified = false;

	console::info( "UI: New scene created" );
}

bool UI::saveScene( const std::string &filePath )
{
	if ( filePath.empty() )
	{
		m_impl->lastError = "File path is empty";
		console::error( "UI: Cannot save scene - file path is empty" );
		return false;
	}

	// Check if scene is available
	if ( !m_impl->scene )
	{
		m_impl->lastError = "Scene not available";
		console::error( "UI: Cannot save scene - scene not initialized" );
		return false;
	}

	// Save scene using SceneSerializer
	const auto result = scene::SceneSerializer::saveScene( *m_impl->scene, filePath );
	if ( !result )
	{
		// Extract error message from SerializationErrorInfo
		const auto &errorInfo = result.error();
		std::string errorMsg;
		switch ( errorInfo.error )
		{
		case scene::SerializationError::FileAccessDenied:
			errorMsg = "Failed to write scene file (access denied)";
			break;
		case scene::SerializationError::InvalidJSON:
			errorMsg = "Invalid scene data (JSON error)";
			break;
		default:
			errorMsg = errorInfo.message.empty() ? "Unknown error occurred" : errorInfo.message;
			break;
		}

		m_impl->lastError = errorMsg + ": " + filePath;
		console::error( "UI: Failed to save scene: {}", m_impl->lastError );
		return false;
	}

	// Update current path and clear modified flag
	m_impl->currentScenePath = filePath;
	m_impl->lastError.clear();
	m_impl->m_sceneModified = false;

	console::info( "UI: Successfully saved scene: {}", filePath );
	return true;
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
	ofn.lpstrFilter = "Scene Files\0*.scene\0All Files\0*.*\0";
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

void UI::openSaveFileDialog()
{
	// Check if we're in test mode (no ImGui context or headless environment)
	if ( !ImGui::GetCurrentContext() )
	{
		// In test mode, don't show actual dialog to avoid blocking tests
		console::info( "UI: Save file dialog skipped (test mode)" );
		return;
	}

	// Use native Windows file dialog for saving
	OPENFILENAMEA ofn;
	char szFile[260] = { 0 };

	// If we have a current scene path, use it as the default filename
	if ( !m_impl->currentScenePath.empty() )
	{
		const auto filename = std::filesystem::path( m_impl->currentScenePath ).filename().string();
		strncpy_s( szFile, sizeof( szFile ), filename.c_str(), _TRUNCATE );
	}

	// Initialize OPENFILENAME
	ZeroMemory( &ofn, sizeof( ofn ) );
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof( szFile );
	ofn.lpstrFilter = "Scene Files\0*.scene\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = "scene"; // Default extension
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	// Display the save dialog box (blocks until user interaction)
	if ( GetSaveFileNameA( &ofn ) == TRUE )
	{
		// User selected a file - save it
		const std::string selectedFile( szFile );
		saveScene( selectedFile );
	}
	// If GetSaveFileNameA returns FALSE, user cancelled - no action needed
}

void UI::openAssetFileDialog()
{
	// Check if we're in test mode (no ImGui context or headless environment)
	if ( !ImGui::GetCurrentContext() )
	{
		// In test mode, don't show actual dialog to avoid blocking tests
		console::info( "UI: Asset file dialog skipped (test mode)" );
		return;
	}

	// Use native Windows file dialog for asset selection
	OPENFILENAMEA ofn;
	char szFile[260] = { 0 };

	// Initialize OPENFILENAME
	ZeroMemory( &ofn, sizeof( ofn ) );
	ofn.lStructSize = sizeof( ofn );
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof( szFile );
	ofn.lpstrFilter = "glTF Files\0*.gltf;*.glb\0glTF Text\0*.gltf\0glTF Binary\0*.glb\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// Display the modal dialog box (blocks until user interaction)
	if ( GetOpenFileNameA( &ofn ) == TRUE )
	{
		// User selected a file - create entity from asset
		const std::string assetPath( szFile );

		// Create entity from asset at world origin (0, 0, 0)
		const math::Vec3f worldPosition{ 0.0f, 0.0f, 0.0f };

		if ( m_impl->scene && m_impl->assetManager && m_impl->commandHistory && m_impl->gpuManager )
		{
			auto cmd = std::make_unique<editor::CreateEntityFromAssetCommand>(
				*m_impl->scene, *m_impl->assetManager, *m_impl->gpuManager, assetPath, worldPosition );

			if ( m_impl->commandHistory->executeCommand( std::move( cmd ) ) )
			{
				// Entity created successfully
				console::info( "Created entity from asset: {}", assetPath );
				m_impl->m_sceneModified = true;
				m_impl->showToast( "Entity created from asset", false );
			}
			else
			{
				// Failed to create entity
				console::error( "Failed to create entity from asset: {}", assetPath );
				m_impl->showToast( "Failed to create entity from asset", true );
			}
		}
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

void UI::showCommandHistoryWindow( bool show )
{
	m_impl->showCommandHistoryWindow = show;
}

bool UI::isCommandHistoryWindowOpen() const
{
	return m_impl->showCommandHistoryWindow;
}

CommandHistory *UI::getCommandHistory()
{
	return m_impl->commandHistory.get();
}

void UI::showSceneHierarchyPanel( bool show )
{
	m_impl->showHierarchyPanel = show;
}

bool UI::isSceneHierarchyPanelOpen() const
{
	return m_impl->showHierarchyPanel;
}

void UI::showEntityInspectorPanel( bool show )
{
	m_impl->showInspectorPanel = show;
}

bool UI::isEntityInspectorPanelOpen() const
{
	return m_impl->showInspectorPanel;
}

void UI::showAssetBrowserPanel( bool show )
{
	m_impl->showAssetBrowserPanel = show;
}

bool UI::isAssetBrowserPanelOpen() const
{
	return m_impl->showAssetBrowserPanel;
}

} // namespace editor

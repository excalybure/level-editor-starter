import runtime.app;
import runtime.console;
import runtime.ecs;
import runtime.systems;
import runtime.mesh_rendering_system;
import engine.renderer;
import engine.asset_manager;
import engine.gpu.gpu_resource_manager;
import platform.win32.win32_window;
import platform.dx12;
import platform.pix;
import editor.ui;
import engine.shader_manager;

#include <iostream>
#include <filesystem>
#include <chrono>

// Helper function to fix the current working directory by searching for the shaders folder
void fixWorkingDirectory()
{
	namespace fs = std::filesystem;
	fs::path currentPath = fs::current_path();
	fs::path originalPath = currentPath;
	bool foundShaders = false;

	// Check up to 5 parent directories for shaders folder
	for ( int i = 0; i < 5; ++i )
	{
		const fs::path shadersPath = currentPath / "shaders";
		if ( fs::exists( shadersPath ) && fs::is_directory( shadersPath ) )
		{
			try
			{
				fs::current_path( currentPath );
				foundShaders = true;
				console::info( "Found shaders directory, set working directory to: {}", currentPath.string() );
				break;
			}
			catch ( const std::exception &e )
			{
				console::error( "Failed to set current directory to {}: {}", currentPath.string(), e.what() );
				break;
			}
		}

		// Move to parent directory
		const fs::path parentPath = currentPath.parent_path();
		if ( parentPath == currentPath )
		{
			// Reached root directory
			break;
		}
		currentPath = parentPath;
	}

	if ( !foundShaders )
	{
		console::error( "Could not find shaders directory. Current working directory remains: {}", originalPath.string() );
		console::error( "Application may not function correctly without proper asset paths." );
	}
}

int main()
{
	// Fix current working directory - search for shaders folder
	fixWorkingDirectory();

	// Create the window
	platform::Win32Window window;
	if ( !window.create( "Level Editor - Multi-Viewport", 600, 900 ) )
	{
		console::fatal( "Failed to create window" );
		return 1;
	}

	// Initialize D3D12 device
	dx12::Device device;
	if ( !device.initialize( static_cast<HWND>( window.getHandle() ) ) )
	{
		console::fatal( "Failed to initialize D3D12 device" );
		return 1;
	}

	// Create shader manager for automatic shader reloading
	auto shaderManager = std::make_shared<shader_manager::ShaderManager>();

	// Create ECS Scene for runtime entities
	ecs::Scene scene;

	// Create asset manager for loading 3D assets
	assets::AssetManager assetManager;

	// Create GPU resource manager for GPU resource creation and management
	engine::GPUResourceManager gpuResourceManager( device );

	// Create renderer for 3D graphics
	renderer::Renderer renderer( device );

	// Create system manager and add systems
	systems::SystemManager systemManager;

	// Add MeshRenderingSystem to handle 3D mesh rendering
	auto *meshRenderingSystem = systemManager.addSystem<runtime::systems::MeshRenderingSystem>( renderer );

	// Initialize all systems with the scene
	systemManager.initialize( scene );

	// Initialize UI system with D3D12 device
	editor::UI ui;
	if ( !ui.initialize(
			 static_cast<void *>( window.getHandle() ),
			 &device,
			 shaderManager ) )
	{
		std::cerr << "Failed to initialize UI system\n";
		return 1;
	}

	// Initialize Scene Operations with required managers
	ui.initializeSceneOperations( scene, systemManager, assetManager, gpuResourceManager );

	// Connect scene and systems to viewport manager for 3D rendering
	ui.getViewportManager().setSceneAndSystems( &scene, &systemManager );

	// Create application
	app::App app;

	std::cout << "Starting Level Editor with ImGui Docking...\n";

	// Main loop with exception handling
	int frameCount = 0;
	try
	{
		// Calculate deltaTime (rough approximation - you might want to improve this)
		auto lastTime = std::chrono::high_resolution_clock::now();
		float deltaTime = 0.0f;
		while ( window.poll() && !ui.shouldExit() )
		{
			frameCount++;

			ui.processInputEvents( window );

			// Begin D3D12 frame - this opens the command list
			device.beginFrame();
			ID3D12GraphicsCommandList *commandList = device.getCommandList();

			// All command list PIX events must be between beginFrame() and endFrame()
			{
				pix::ScopedEvent pixFrame( commandList, pix::MarkerColor::White, std::format( "Frame {}", frameCount ) );

				// Update shader manager (check for shader file changes and hot-reload)
				{
					pix::ScopedEvent pixShaderUpdate( commandList, pix::MarkerColor::Red, "Shader Manager Update" );
					shaderManager->update();
				}

				// Update systems (including MeshRenderingSystem)
				{
					pix::ScopedEvent pixSystemUpdate( commandList, pix::MarkerColor::Orange, "System Update" );
					systemManager.update( scene, deltaTime );
				}

				// Update viewports with timing (for camera controllers)
				{
					pix::ScopedEvent pixViewportUpdate( commandList, pix::MarkerColor::Magenta, "Viewport Update" );
					ui.updateViewports( deltaTime );
				}

				// Begin UI frame (sets up docking)
				{
					pix::ScopedEvent pixUIBegin( commandList, pix::MarkerColor::Green, "UI Begin Frame" );
					ui.beginFrame();
				}

				// End UI frame (renders ImGui)
				{
					pix::ScopedEvent pixUIEnd( commandList, pix::MarkerColor::LightGreen, "UI End Frame" );
					ui.endFrame();
				}

				// Submit ImGui draw data to command list
				{
					pix::ScopedEvent pixImGui( commandList, pix::MarkerColor::Cyan, "ImGui Render" );
					// Restore backbuffer render target after viewport rendering
					device.setBackbufferRenderTarget();
					ui.renderDrawData( static_cast<void *>( commandList ) );
				}
			} // Frame PIX event ends here, before endFrame() closes the command list

			// End D3D12 frame - this closes the command list
			device.endFrame();

			// Present D3D12 frame - command list is closed at this point
			device.present();

			// App tick
			app.tick();

			const auto currentTime = std::chrono::high_resolution_clock::now();
			deltaTime = std::chrono::duration<float>( currentTime - lastTime ).count();
			lastTime = currentTime;
		}
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Exception caught in main loop: " << e.what() << std::endl;
		std::cerr << "Application will exit with error code 1" << std::endl;

		// Cleanup before exit
		ui.shutdown();
		device.shutdown();
		return 1;
	}
	catch ( ... )
	{
		std::cerr << "Unknown exception caught in main loop" << std::endl;
		std::cerr << "Application will exit with error code 1" << std::endl;

		// Cleanup before exit
		ui.shutdown();
		device.shutdown();
		return 1;
	}

	// Cleanup
	ui.shutdown();
	device.shutdown();

	return 0;
}

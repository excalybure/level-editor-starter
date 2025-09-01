import runtime.app;
import runtime.console;
import platform.win32.win32_window;
import platform.dx12;
import platform.pix;
import editor.ui;
#include <iostream>
#include <filesystem>

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
		fs::path shadersPath = currentPath / "shaders";
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
		fs::path parentPath = currentPath.parent_path();
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
	if ( !window.create( "Level Editor - Multi-Viewport", 1600, 900 ) )
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

	// Initialize UI system with D3D12 device
	editor::UI ui;
	if ( !ui.initialize(
			 static_cast<void *>( window.getHandle() ),
			 &device ) )
	{
		std::cerr << "Failed to initialize UI system\n";
		return 1;
	}

	// Create application
	app::App app;

	std::cout << "Starting Level Editor with ImGui Docking...\n";

	// Main loop with exception handling
	int frameCount = 0;
	try
	{
		while ( window.poll() && !ui.shouldExit() )
		{
			// PIX frame marker
			frameCount++;
			ID3D12GraphicsCommandList *commandList = device.getCommandList();
			pix::ScopedEvent pixFrame( commandList, pix::MarkerColor::White, std::format( "Frame {}", frameCount ) );

			// Begin D3D12 frame
			{
				pix::ScopedEvent pixBeginFrame( commandList, pix::MarkerColor::Blue, "Begin D3D12 Frame" );
				device.beginFrame();
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

			// End D3D12 frame
			{
				pix::ScopedEvent pixEndFrame( commandList, pix::MarkerColor::Purple, "End D3D12 Frame" );
				device.endFrame();
			}

			// Present D3D12 frame
			{
				pix::ScopedEvent pixPresent( commandList, pix::MarkerColor::Orange, "Present Frame" );
				device.present();
			}

			// App tick
			app.tick();
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

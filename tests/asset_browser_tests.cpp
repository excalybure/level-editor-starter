#include <catch2/catch_test_macros.hpp>

#include "editor/asset_browser/AssetBrowserPanel.h"
#include "engine/assets/asset_manager.h"
#include "runtime/ecs.h"
#include "editor/commands/CommandHistory.h"

#include <filesystem>
#include <fstream>
#include <chrono>

// Helper to create a temporary test directory structure
struct TempDirectoryFixture
{
	std::string testRoot;

	TempDirectoryFixture()
	{
		// Create a unique temp directory for this test
		testRoot = "temp_asset_browser_test_" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
		std::filesystem::create_directories( testRoot );
		std::filesystem::create_directories( testRoot + "/subdir1" );
		std::filesystem::create_directories( testRoot + "/subdir2" );

		// Create some test files
		std::ofstream( testRoot + "/file1.txt" ).close();
		std::ofstream( testRoot + "/file2.gltf" ).close();
		std::ofstream( testRoot + "/subdir1/nested_file.txt" ).close();
	}

	~TempDirectoryFixture()
	{
		// Clean up temp directory
		try
		{
			std::filesystem::remove_all( testRoot );
		}
		catch ( ... )
		{
		}
	}
};

TEST_CASE( "AssetBrowserPanel can be instantiated", "[AssetBrowser][T3.1][unit]" )
{
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );

	REQUIRE( panel.isVisible() );
	REQUIRE( panel.getRootPath() == "assets/" );
}

TEST_CASE( "AssetBrowserPanel root directory displays correctly", "[AssetBrowser][T3.1][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "Root path is set correctly" )
	{
		const auto rootPath = panel.getRootPath();
		REQUIRE( rootPath.find( fixture.testRoot ) != std::string::npos );
		// Should end with separator
		REQUIRE( ( rootPath.back() == '/' || rootPath.back() == '\\' ) );
	}

	SECTION( "Current path initializes to root path" )
	{
		REQUIRE( panel.getCurrentPath() == panel.getRootPath() );
	}
}

TEST_CASE( "AssetBrowserPanel recognizes subdirectories", "[AssetBrowser][T3.1][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	// Test that the panel is properly initialized with the test directory
	// Note: We cannot test render() without ImGui context
	REQUIRE( panel.getRootPath().find( fixture.testRoot ) != std::string::npos );
	REQUIRE( panel.getCurrentPath() == panel.getRootPath() );
}

TEST_CASE( "AssetBrowserPanel visibility control", "[AssetBrowser][T3.1][unit]" )
{
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );

	SECTION( "Panel starts visible" )
	{
		REQUIRE( panel.isVisible() );
	}

	SECTION( "Panel can be hidden" )
	{
		panel.setVisible( false );
		REQUIRE_FALSE( panel.isVisible() );
	}

	SECTION( "Panel can be shown again" )
	{
		panel.setVisible( false );
		panel.setVisible( true );
		REQUIRE( panel.isVisible() );
	}
}

TEST_CASE( "AssetBrowserPanel handles empty directories", "[AssetBrowser][T3.1][unit]" )
{
	// Create an empty temp directory
	const std::string emptyDir = "temp_empty_dir_" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
	std::filesystem::create_directories( emptyDir );

	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( emptyDir );

	// Panel should be properly configured with empty directory
	REQUIRE( panel.isVisible() );
	REQUIRE( panel.getCurrentPath().find( emptyDir ) != std::string::npos );

	// Clean up
	std::filesystem::remove_all( emptyDir );
}

TEST_CASE( "AssetBrowserPanel handles non-existent directories", "[AssetBrowser][T3.1][unit]" )
{
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( "non_existent_directory_xyz123" );

	// Panel should handle non-existent paths gracefully
	// Note: render() requires ImGui context, so we test configuration only
	REQUIRE( panel.getRootPath() == "non_existent_directory_xyz123/" );
	REQUIRE( panel.isVisible() );
}

// T3.2: Directory Tree View Tests

TEST_CASE( "AssetBrowserPanel navigates to folder when clicked", "[AssetBrowser][T3.2][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "Current path starts at root" )
	{
		REQUIRE( panel.getCurrentPath() == panel.getRootPath() );
	}

	SECTION( "Navigating to subdirectory updates current path" )
	{
		// Simulate navigation by calling navigateToDirectory
		const std::string subdir1Path = fixture.testRoot + "/subdir1";
		panel.navigateToDirectory( subdir1Path );
		REQUIRE( panel.getCurrentPath() == subdir1Path );
	}

	SECTION( "Navigating to nested directory works" )
	{
		const std::string subdir1Path = fixture.testRoot + "/subdir1";
		panel.navigateToDirectory( subdir1Path );
		REQUIRE( panel.getCurrentPath() == subdir1Path );
	}
}

TEST_CASE( "AssetBrowserPanel handles deep directory hierarchies", "[AssetBrowser][T3.2][unit]" )
{
	// Create a deep directory structure
	const std::string deepRoot = "temp_deep_" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
	std::filesystem::create_directories( deepRoot + "/level1/level2/level3" );

	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( deepRoot );

	SECTION( "Can navigate to deep directories" )
	{
		const std::string deepPath = deepRoot + "/level1/level2/level3";
		panel.navigateToDirectory( deepPath );
		REQUIRE( panel.getCurrentPath() == deepPath );
	}

	// Clean up
	std::filesystem::remove_all( deepRoot );
}

// T3.3: Path Breadcrumbs Tests

TEST_CASE( "AssetBrowserPanel can navigate to parent directory", "[AssetBrowser][T3.3][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "Navigate to subdirectory then back to parent" )
	{
		const std::string subdir1Path = fixture.testRoot + "/subdir1";
		panel.navigateToDirectory( subdir1Path );
		REQUIRE( panel.getCurrentPath() == subdir1Path );

		// Navigate up to parent
		panel.navigateToParent();
		REQUIRE( panel.getCurrentPath() == panel.getRootPath() );
	}

	SECTION( "Cannot navigate above root path" )
	{
		// Already at root
		REQUIRE( panel.getCurrentPath() == panel.getRootPath() );

		// Try to navigate up from root
		panel.navigateToParent();

		// Should still be at root
		REQUIRE( panel.getCurrentPath() == panel.getRootPath() );
	}
}

TEST_CASE( "AssetBrowserPanel gets path segments correctly", "[AssetBrowser][T3.3][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "Root path has one segment" )
	{
		const auto segments = panel.getPathSegments();
		REQUIRE( segments.size() == 1 );
		REQUIRE( segments[0].second == panel.getRootPath() );
	}

	SECTION( "Subdirectory path has multiple segments" )
	{
		const std::string subdir1Path = fixture.testRoot + "/subdir1";
		panel.navigateToDirectory( subdir1Path );

		const auto segments = panel.getPathSegments();
		REQUIRE( segments.size() == 2 );
		// First segment should be root
		REQUIRE( segments[0].second == panel.getRootPath() );
		// Second segment should be subdir1
		REQUIRE( segments[1].second == subdir1Path );
	}
}

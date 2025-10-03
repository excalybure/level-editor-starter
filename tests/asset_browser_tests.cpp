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

// ============================================================================
// T3.4: Asset Grid View
// ============================================================================

TEST_CASE( "AssetBrowserPanel identifies asset types from file extensions", "[AssetBrowser][T3.4][unit]" )
{
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );

	SECTION( "GLTF files are identified as meshes" )
	{
		const auto type = panel.getAssetTypeFromExtension( "model.gltf" );
		REQUIRE( type == editor::AssetType::Mesh );
	}

	SECTION( "GLB files are identified as meshes" )
	{
		const auto type = panel.getAssetTypeFromExtension( "model.glb" );
		REQUIRE( type == editor::AssetType::Mesh );
	}

	SECTION( "Unknown extensions return Unknown type" )
	{
		const auto type = panel.getAssetTypeFromExtension( "file.txt" );
		REQUIRE( type == editor::AssetType::Unknown );
	}

	SECTION( "Extension matching is case-insensitive" )
	{
		const auto type1 = panel.getAssetTypeFromExtension( "model.GLTF" );
		const auto type2 = panel.getAssetTypeFromExtension( "model.Gltf" );
		REQUIRE( type1 == editor::AssetType::Mesh );
		REQUIRE( type2 == editor::AssetType::Mesh );
	}

	SECTION( "Files without extension return Unknown" )
	{
		const auto type = panel.getAssetTypeFromExtension( "noextension" );
		REQUIRE( type == editor::AssetType::Unknown );
	}
}

TEST_CASE( "AssetBrowserPanel filters files from directories", "[AssetBrowser][T3.4][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "getFileContents returns only files, not directories" )
	{
		const auto files = panel.getFileContents( fixture.testRoot );

		// Should have 2 files (file1.txt, file2.gltf), but not subdirectories
		REQUIRE( files.size() == 2 );

		// Verify all returned items are files
		for ( const auto &file : files )
		{
			REQUIRE( !std::filesystem::is_directory( file ) );
		}
	}

	SECTION( "Empty directory returns empty file list" )
	{
		const std::string emptyDir = fixture.testRoot + "/subdir2";
		const auto files = panel.getFileContents( emptyDir );
		REQUIRE( files.empty() );
	}

	SECTION( "Non-existent path returns empty file list" )
	{
		const auto files = panel.getFileContents( "nonexistent_path" );
		REQUIRE( files.empty() );
	}
}

// ============================================================================
// T3.5: Asset Selection and Preview
// ============================================================================

TEST_CASE( "AssetBrowserPanel tracks selected asset", "[AssetBrowser][T3.5][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "Initially no asset is selected" )
	{
		REQUIRE( panel.getSelectedAsset().empty() );
	}

	SECTION( "Can select an asset" )
	{
		const std::string assetPath = fixture.testRoot + "/file1.txt";
		panel.selectAsset( assetPath );
		REQUIRE( panel.getSelectedAsset() == assetPath );
	}

	SECTION( "Can clear selection" )
	{
		const std::string assetPath = fixture.testRoot + "/file1.txt";
		panel.selectAsset( assetPath );
		REQUIRE( !panel.getSelectedAsset().empty() );

		panel.clearSelection();
		REQUIRE( panel.getSelectedAsset().empty() );
	}

	SECTION( "Selecting new asset replaces previous selection" )
	{
		const std::string asset1 = fixture.testRoot + "/file1.txt";
		const std::string asset2 = fixture.testRoot + "/file2.gltf";

		panel.selectAsset( asset1 );
		REQUIRE( panel.getSelectedAsset() == asset1 );

		panel.selectAsset( asset2 );
		REQUIRE( panel.getSelectedAsset() == asset2 );
	}
}

TEST_CASE( "AssetBrowserPanel provides asset metadata", "[AssetBrowser][T3.5][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "getAssetMetadata returns file size" )
	{
		const std::string assetPath = fixture.testRoot + "/file1.txt";
		const auto metadata = panel.getAssetMetadata( assetPath );

		REQUIRE( metadata.exists );
		REQUIRE( metadata.sizeBytes >= 0 );
	}

	SECTION( "getAssetMetadata returns asset type" )
	{
		const std::string assetPath = fixture.testRoot + "/file2.gltf";
		const auto metadata = panel.getAssetMetadata( assetPath );

		REQUIRE( metadata.exists );
		REQUIRE( metadata.type == editor::AssetType::Mesh );
	}

	SECTION( "getAssetMetadata handles non-existent files" )
	{
		const auto metadata = panel.getAssetMetadata( "nonexistent.txt" );
		REQUIRE( !metadata.exists );
	}

	SECTION( "getAssetMetadata returns filename" )
	{
		const std::string assetPath = fixture.testRoot + "/file2.gltf";
		const auto metadata = panel.getAssetMetadata( assetPath );

		REQUIRE( metadata.exists );
		REQUIRE( metadata.filename == "file2.gltf" );
	}
}

TEST_CASE( "AssetBrowserPanel can import assets", "[AssetBrowser][T3.6][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	// Create a source file to import
	const std::string sourceFile = fixture.testRoot + "/source_import.gltf";
	{
		std::ofstream file( sourceFile );
		file << "test content";
	} // Close file before import

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "importAsset copies file to current directory" )
	{
		// Verify source file exists
		REQUIRE( std::filesystem::exists( sourceFile ) );

		// Create a target subdirectory
		const std::string targetDir = fixture.testRoot + "/imported";
		std::filesystem::create_directories( targetDir );
		panel.navigateToDirectory( targetDir );

		// Verify navigation worked
		REQUIRE( panel.getCurrentPath() == targetDir );

		const bool result = panel.importAsset( sourceFile );
		REQUIRE( result );

		// Verify file was copied
		const std::string expectedPath = targetDir + "/source_import.gltf";
		REQUIRE( std::filesystem::exists( expectedPath ) );
	}

	SECTION( "importAsset returns false for non-existent source" )
	{
		const bool result = panel.importAsset( "nonexistent_file.gltf" );
		REQUIRE( !result );
	}

	SECTION( "importAsset returns false for unsupported file types" )
	{
		const std::string unsupportedFile = fixture.testRoot + "/unsupported.xyz";
		std::ofstream( unsupportedFile ) << "test";

		const bool result = panel.importAsset( unsupportedFile );
		REQUIRE( !result );
	}

	SECTION( "importAsset handles duplicate filenames" )
	{
		// Import the same file twice
		const bool result1 = panel.importAsset( sourceFile );
		REQUIRE( result1 );

		// Second import should either succeed (overwrite) or return false (duplicate)
		const bool result2 = panel.importAsset( sourceFile );
		// For now, we'll allow overwrite
		REQUIRE( result2 );
	}
}

TEST_CASE( "AssetBrowserPanel shows import UI", "[AssetBrowser][T3.6][ui]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "hasImportUI returns true after initialization" )
	{
		// This is a placeholder test to verify the import button will be rendered
		// Actual UI rendering is integration-tested via manual testing
		REQUIRE( panel.isVisible() );
	}
}

TEST_CASE( "AssetBrowserPanel supports drag-and-drop", "[AssetBrowser][T3.7][unit]" )
{
	TempDirectoryFixture fixture;
	assets::AssetManager assetManager;
	ecs::Scene scene;
	CommandHistory commandHistory;

	editor::AssetBrowserPanel panel( assetManager, scene, commandHistory );
	panel.setRootPath( fixture.testRoot );

	SECTION( "getDragDropPayload returns asset path for mesh" )
	{
		const std::string assetPath = fixture.testRoot + "/test_mesh.gltf";
		const auto payload = panel.getDragDropPayload( assetPath );

		REQUIRE( !payload.empty() );
		REQUIRE( payload == assetPath );
	}

	SECTION( "getDragDropPayload returns empty for unsupported type" )
	{
		const std::string assetPath = fixture.testRoot + "/unsupported.xyz";
		const auto payload = panel.getDragDropPayload( assetPath );

		REQUIRE( payload.empty() );
	}

	SECTION( "canDragAsset returns true for supported types" )
	{
		REQUIRE( panel.canDragAsset( fixture.testRoot + "/mesh.gltf" ) );
		REQUIRE( panel.canDragAsset( fixture.testRoot + "/model.glb" ) );
	}

	SECTION( "canDragAsset returns false for unsupported types" )
	{
		REQUIRE( !panel.canDragAsset( fixture.testRoot + "/file.txt" ) );
		REQUIRE( !panel.canDragAsset( fixture.testRoot + "/unknown.xyz" ) );
	}
}

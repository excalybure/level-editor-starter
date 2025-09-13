#include <catch2/catch_test_macros.hpp>

import editor.ui;
import editor.scene_editor;

TEST_CASE( "UI SceneEditor integration basic functionality", "[integration][ui][scene_editor]" )
{
	SECTION( "UI can be created and initialized with SceneEditor" )
	{
		editor::UI ui;

		// Test that UI creation doesn't crash
		REQUIRE_NOTHROW( ui.getLayout() );
	}

	SECTION( "UI SceneEditor window management" )
	{
		editor::UI ui;

		// Test scene editor window state management
		REQUIRE_FALSE( ui.isSceneEditorWindowOpen() );

		ui.showSceneEditorWindow( true );
		REQUIRE( ui.isSceneEditorWindowOpen() );

		ui.showSceneEditorWindow( false );
		REQUIRE_FALSE( ui.isSceneEditorWindowOpen() );
	}
}

TEST_CASE( "SceneEditor default constructor integration", "[integration][scene_editor]" )
{
	SECTION( "SceneEditor can be created with default constructor" )
	{
		editor::SceneEditor sceneEditor;

		// Test basic operations work with null dependencies
		REQUIRE( sceneEditor.getCurrentScenePath().empty() );
		REQUIRE( sceneEditor.getEntityCount() == 0 );
		REQUIRE_FALSE( sceneEditor.isFileDialogActive() );
	}

	SECTION( "SceneEditor handles null dependencies gracefully" )
	{
		editor::SceneEditor sceneEditor;

		// These operations should not crash but should fail gracefully
		REQUIRE_FALSE( sceneEditor.loadScene( "test.gltf" ) );
		REQUIRE_NOTHROW( sceneEditor.clearScene() );
		REQUIRE_NOTHROW( sceneEditor.openFileDialog() );
	}
}
#pragma once

#include <string>
#include <vector>

// Forward declarations
namespace assets
{
class AssetManager;
}

namespace ecs
{
class Scene;
}

class CommandHistory;

namespace editor
{

// Asset browser panel for importing and managing assets
class AssetBrowserPanel
{
public:
	AssetBrowserPanel( assets::AssetManager &assetManager,
		ecs::Scene &scene,
		CommandHistory &commandHistory );

	// Render the asset browser panel
	void render();

	// Visibility control
	void setVisible( bool visible ) { m_visible = visible; }
	bool isVisible() const { return m_visible; }

	// Root path configuration
	void setRootPath( const std::string &path );
	const std::string &getRootPath() const { return m_rootPath; }

	// Current path navigation
	const std::string &getCurrentPath() const { return m_currentPath; }
	void navigateToDirectory( const std::string &path );
	void navigateToParent();

	// Path utilities
	std::vector<std::pair<std::string, std::string>> getPathSegments() const;

private:
	// Helper methods for file system operations
	std::vector<std::string> getDirectoryContents( const std::string &path ) const;
	bool isDirectory( const std::string &path ) const;

	// UI rendering helpers
	void renderDirectoryTree( const std::string &path );
	void renderPathBar();

	// Reference members
	assets::AssetManager &m_assetManager;
	ecs::Scene &m_scene;
	CommandHistory &m_commandHistory;

	// State members
	bool m_visible = true;
	std::string m_rootPath = "assets/";
	std::string m_currentPath;
};

} // namespace editor

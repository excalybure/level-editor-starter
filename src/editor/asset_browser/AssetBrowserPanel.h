#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Forward declarations
namespace assets
{
class AssetManager;
}

namespace ecs
{
class Scene;
}

namespace dx12
{
class Device;
class Texture;
} // namespace dx12

class CommandHistory;

namespace editor
{

// Asset type enumeration for file classification
enum class AssetType
{
	Unknown,
	Mesh,
	Texture,
	Material
};

// Asset metadata structure
struct AssetMetadata
{
	bool exists = false;
	AssetType type = AssetType::Unknown;
	std::string filename;
	std::size_t sizeBytes = 0;
};

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

	// Device for texture loading
	void setDevice( dx12::Device *device ) { m_device = device; }

	// Current path navigation
	const std::string &getCurrentPath() const { return m_currentPath; }
	void navigateToDirectory( const std::string &path );
	void navigateToParent();

	// Path utilities
	std::vector<std::pair<std::string, std::string>> getPathSegments() const;

	// Asset type detection
	AssetType getAssetTypeFromExtension( const std::string &filename ) const;

	// File system queries (public for testability)
	std::vector<std::string> getFileContents( const std::string &path ) const;

	// Asset selection
	void selectAsset( const std::string &assetPath );
	void clearSelection();
	const std::string &getSelectedAsset() const { return m_selectedAsset; }

	// Asset metadata
	AssetMetadata getAssetMetadata( const std::string &assetPath ) const;

	// Asset import
	bool importAsset( const std::string &sourceFilePath );

	// Drag-and-drop support
	bool canDragAsset( const std::string &assetPath ) const;
	std::string getDragDropPayload( const std::string &assetPath ) const;

	// Tooltip support
	std::string buildTooltipText( const std::string &assetPath ) const;

private:
	// Helper methods for file system operations
	std::vector<std::string> getDirectoryContents( const std::string &path ) const;
	bool isDirectory( const std::string &path ) const;

	// UI rendering helpers
	void renderDirectoryTree( const std::string &path );
	void renderPathBar();
	void renderAssetGrid();
	void renderAssetPreview();

	// Reference members
	assets::AssetManager &m_assetManager;
	ecs::Scene &m_scene;
	CommandHistory &m_commandHistory;
	dx12::Device *m_device = nullptr;

	// State members
	bool m_visible = true;
	std::string m_rootPath = "assets/";
	std::string m_currentPath;
	std::string m_selectedAsset;

	// Texture preview cache (path -> texture)
	std::unordered_map<std::string, std::shared_ptr<dx12::Texture>> m_textureCache;
};

} // namespace editor

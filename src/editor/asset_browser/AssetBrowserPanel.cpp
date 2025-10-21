#include "AssetBrowserPanel.h"
#include "engine/assets/asset_manager.h"
#include "runtime/ecs.h"
#include "editor/commands/CommandHistory.h"
#include "core/console.h"
#include "platform/dx12/dx12_device.h"
#include "graphics/texture/texture_loader.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <windows.h>

namespace editor
{

AssetBrowserPanel::AssetBrowserPanel( assets::AssetManager &assetManager,
	ecs::Scene &scene,
	CommandHistory &commandHistory )
	: m_assetManager( assetManager ), m_scene( scene ), m_commandHistory( commandHistory )
{
	// Initialize current path to root path
	m_currentPath = m_rootPath;
}

std::string AssetBrowserPanel::pathToUtf8String( const std::filesystem::path &path )
{
	try
	{
		// Convert wide path to UTF-8 string
		// Use wstring first (native Windows format), then encode to UTF-8
		const std::wstring widePath = path.wstring();

		// Convert wide string to UTF-8
		int utf8Size = WideCharToMultiByte( CP_UTF8, 0, widePath.c_str(), -1, nullptr, 0, nullptr, nullptr );
		if ( utf8Size <= 0 )
		{
			return "(unknown)";
		}

		std::string utf8String( utf8Size - 1, '\0' ); // -1 to exclude null terminator
		WideCharToMultiByte( CP_UTF8, 0, widePath.c_str(), -1, &utf8String[0], utf8Size, nullptr, nullptr );

		return utf8String;
	}
	catch ( const std::exception & )
	{
		return "(unknown)";
	}
}

void AssetBrowserPanel::render()
{
	if ( !m_visible )
		return;

	if ( ImGui::Begin( "Asset Browser", &m_visible ) )
	{
		// Left panel: Directory tree
		ImGui::BeginChild( "DirectoryTree", ImVec2( 200, 0 ), true );
		ImGui::Text( "Folders" );
		ImGui::Separator();
		renderDirectoryTree( m_rootPath );
		ImGui::EndChild();

		ImGui::SameLine();

		// Right panel: Current directory contents and preview
		ImGui::BeginChild( "DirectoryContents", ImVec2( 0, 0 ), true );

		// Path breadcrumbs
		renderPathBar();
		ImGui::Separator();

		// Split into grid view and preview panel
		const float previewWidth = 250.0f;
		const bool hasSelection = !m_selectedAsset.empty();

		if ( hasSelection )
		{
			// Grid takes remaining width minus preview width
			ImGui::BeginChild( "AssetGrid", ImVec2( -previewWidth - 10, 0 ), false );
			renderAssetGrid();
			ImGui::EndChild();

			ImGui::SameLine();

			// Preview panel on the right
			ImGui::BeginChild( "AssetPreview", ImVec2( previewWidth, 0 ), true );
			renderAssetPreview();
			ImGui::EndChild();
		}
		else
		{
			// No selection, use full width for grid
			renderAssetGrid();
		}

		ImGui::EndChild();
	}
	ImGui::End();
}

void AssetBrowserPanel::setRootPath( const std::string &path )
{
	m_rootPath = path;
	// Ensure root path ends with separator
	if ( !m_rootPath.empty() && m_rootPath.back() != '/' && m_rootPath.back() != '\\' )
	{
		m_rootPath += '/';
	}
	m_currentPath = m_rootPath;
}

void AssetBrowserPanel::navigateToDirectory( const std::string &path )
{
	// Validate that the path exists and is a directory
	if ( std::filesystem::exists( path ) && std::filesystem::is_directory( path ) )
	{
		m_currentPath = path;
	}
}

void AssetBrowserPanel::navigateToParent()
{
	// Don't navigate above root path
	if ( m_currentPath == m_rootPath )
	{
		return;
	}

	try
	{
		const std::filesystem::path currentPath( m_currentPath );
		const std::filesystem::path parentPath = currentPath.parent_path();

		// Check if parent is at or above root
		const std::filesystem::path rootPath( m_rootPath );
		const auto parentStr = parentPath.string();
		const auto rootStr = rootPath.string();

		// Navigate to parent if it's within or equal to root
		if ( parentStr.size() >= rootStr.size() )
		{
			navigateToDirectory( parentStr );
		}
		else
		{
			// Parent is above root, navigate to root instead
			m_currentPath = m_rootPath;
		}
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		// On error, stay at current path
	}
}

std::vector<std::pair<std::string, std::string>> AssetBrowserPanel::getPathSegments() const
{
	std::vector<std::pair<std::string, std::string>> segments;

	try
	{
		const std::filesystem::path currentPath( m_currentPath );
		const std::filesystem::path rootPath( m_rootPath );

		// Build segments from root to current
		std::filesystem::path buildPath = rootPath;
		segments.push_back( { rootPath.filename().string(), rootPath.string() } );

		// If we're at root, we're done
		if ( m_currentPath == m_rootPath )
		{
			return segments;
		}

		// Get relative path from root to current
		const auto relativePath = std::filesystem::relative( currentPath, rootPath );

		// Add each segment of the relative path
		for ( const auto &part : relativePath )
		{
			buildPath /= part;
			segments.push_back( { part.string(), buildPath.string() } );
		}
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		// On error, return at least the root
		if ( segments.empty() )
		{
			segments.push_back( { "assets", m_rootPath } );
		}
	}

	return segments;
}

std::vector<std::string> AssetBrowserPanel::getDirectoryContents( const std::string &path ) const
{
	std::vector<std::string> contents;

	// Check if path exists
	if ( !std::filesystem::exists( path ) )
	{
		return contents;
	}

	try
	{
		// Iterate through directory entries
		for ( const auto &entry : std::filesystem::directory_iterator( path ) )
		{
			contents.push_back( entry.path().string() );
		}

		// Sort: directories first, then files, alphabetically within each group
		std::sort( contents.begin(), contents.end(), [this]( const std::string &a, const std::string &b ) {
			const bool aIsDir = isDirectory( a );
			const bool bIsDir = isDirectory( b );

			if ( aIsDir != bIsDir )
			{
				return aIsDir; // Directories come first
			}

			// Both are same type, sort alphabetically
			return a < b;
		} );
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		// Return empty vector on error
	}

	return contents;
}

bool AssetBrowserPanel::isDirectory( const std::string &path ) const
{
	try
	{
		return std::filesystem::is_directory( path );
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		return false;
	}
}

void AssetBrowserPanel::renderDirectoryTree( const std::string &path )
{
	// Check if path exists
	if ( !std::filesystem::exists( path ) )
	{
		return;
	}

	try
	{
		// Get immediate subdirectories
		std::vector<std::filesystem::path> subdirs;
		for ( const auto &entry : std::filesystem::directory_iterator( path ) )
		{
			if ( entry.is_directory() )
			{
				subdirs.push_back( entry.path() );
			}
		}

		// Sort alphabetically
		std::sort( subdirs.begin(), subdirs.end() );

		// Render tree nodes for each subdirectory
		for ( const auto &subdir : subdirs )
		{
			// Get display name safely using UTF-8 conversion
			const std::string displayName = pathToUtf8String( subdir.filename() );

			// Convert path to UTF-8 for comparison and navigation
			const std::string subdirPath = pathToUtf8String( subdir );
			const bool isCurrentPath = ( subdirPath == m_currentPath );

			// Use different flags for current directory (highlight)
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if ( isCurrentPath )
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			// Check if this directory has subdirectories
			bool hasSubdirs = false;
			try
			{
				for ( const auto &entry : std::filesystem::directory_iterator( subdir ) )
				{
					if ( entry.is_directory() )
					{
						hasSubdirs = true;
						break;
					}
				}
			}
			catch ( const std::filesystem::filesystem_error & )
			{
				// Ignore errors when checking for subdirectories
			}

			// If no subdirectories, make it a leaf node
			if ( !hasSubdirs )
			{
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			}

			// Render tree node
			const bool nodeOpen = ImGui::TreeNodeEx( displayName.c_str(), flags );

			// Handle click on tree node
			if ( ImGui::IsItemClicked() )
			{
				navigateToDirectory( subdirPath );
			}

			// Recursively render children if node is open and has subdirectories
			if ( nodeOpen && hasSubdirs )
			{
				renderDirectoryTree( subdirPath );
				ImGui::TreePop();
			}
		}
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		// Silently handle errors
	}
}

void AssetBrowserPanel::renderPathBar()
{
	// Import button
	if ( ImGui::Button( "Import Asset" ) )
	{
		// For now, show a simple text input for file path
		// Future: Use native file dialog
		ImGui::OpenPopup( "Import Asset##popup" );
	}

	// Import dialog popup
	static char importPathBuffer[512] = "";
	if ( ImGui::BeginPopup( "Import Asset##popup" ) )
	{
		ImGui::Text( "Enter file path to import:" );
		ImGui::InputText( "##importPath", importPathBuffer, sizeof( importPathBuffer ) );

		if ( ImGui::Button( "Import" ) )
		{
			const std::string filePath( importPathBuffer );
			if ( !filePath.empty() )
			{
				const bool success = importAsset( filePath );
				if ( success )
				{
					// Clear the input
					importPathBuffer[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
				else
				{
					ImGui::TextColored( ImVec4( 1.0f, 0.3f, 0.3f, 1.0f ), "Import failed! Check file path and type." );
				}
			}
		}

		ImGui::SameLine();
		if ( ImGui::Button( "Cancel" ) )
		{
			importPathBuffer[0] = '\0';
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// Up button
	if ( m_currentPath != m_rootPath )
	{
		if ( ImGui::Button( "^" ) )
		{
			navigateToParent();
		}
		ImGui::SameLine();
	}

	// Path segments
	const auto segments = getPathSegments();
	for ( size_t i = 0; i < segments.size(); ++i )
	{
		const auto &segment = segments[i];
		const std::string &name = segment.first;
		const std::string &path = segment.second;

		// Display segment as button
		// Use ## ID suffix to handle empty names (root path case)
		const std::string buttonLabel = name.empty() ? "assets##root" : name + "##" + std::to_string( i );
		if ( ImGui::Button( buttonLabel.c_str() ) )
		{
			navigateToDirectory( path );
		}

		// Add separator between segments (but not after last)
		if ( i < segments.size() - 1 )
		{
			ImGui::SameLine();
			ImGui::Text( "/" );
			ImGui::SameLine();
		}
	}
}

AssetType AssetBrowserPanel::getAssetTypeFromExtension( const std::string &filename ) const
{
	try
	{
		const std::filesystem::path path( filename );
		std::string extension = path.extension().string();

		// Convert to lowercase for case-insensitive comparison
		std::transform( extension.begin(), extension.end(), extension.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

		// Check for mesh formats
		if ( extension == ".gltf" || extension == ".glb" )
		{
			return AssetType::Mesh;
		}

		// Check for texture formats
		if ( extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
			extension == ".dds" || extension == ".tga" )
		{
			return AssetType::Texture;
		}

		// Future: Add material formats (.mat, etc.)

		return AssetType::Unknown;
	}
	catch ( const std::exception & )
	{
		return AssetType::Unknown;
	}
}

std::vector<std::string> AssetBrowserPanel::getFileContents( const std::string &path ) const
{
	std::vector<std::string> files;

	// Check if path exists
	if ( !std::filesystem::exists( path ) )
	{
		return files;
	}

	try
	{
		// Iterate through directory entries
		for ( const auto &entry : std::filesystem::directory_iterator( path ) )
		{
			// Only add files, not directories
			if ( entry.is_regular_file() )
			{
				files.push_back( entry.path().string() );
			}
		}

		// Sort alphabetically
		std::sort( files.begin(), files.end() );
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		// Return empty vector on error
	}

	return files;
}

void AssetBrowserPanel::renderAssetGrid()
{
	// Get files in current directory (excluding subdirectories)
	const auto files = getFileContents( m_currentPath );

	// If there are no files, show a message
	if ( files.empty() )
	{
		ImGui::TextDisabled( "(no assets in this directory)" );
		return;
	}

	// Grid layout parameters
	const float cellSize = 100.0f;
	const float cellPadding = 10.0f;
	const float totalCellSize = cellSize + cellPadding;

	// Calculate number of columns based on available width
	const float availableWidth = ImGui::GetContentRegionAvail().x;
	const int columnCount = std::max( 1, static_cast<int>( availableWidth / totalCellSize ) );

	// Render grid
	int currentColumn = 0;
	for ( const auto &filePath : files )
	{
		const std::filesystem::path path( filePath );
		const std::string filename = path.filename().string();
		const AssetType assetType = getAssetTypeFromExtension( filename );

		// Start new row if needed
		if ( currentColumn > 0 )
		{
			ImGui::SameLine();
		}

		// Begin grid cell
		ImGui::BeginGroup();

		// Check if this is selected
		const bool isSelected = ( filePath == m_selectedAsset );
		if ( isSelected )
		{
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.8f, 1.0f ) );
		}

		// For texture assets, try to load and show the actual image
		bool renderedImage = false;
		if ( assetType == AssetType::Texture && m_device )
		{
			// Check if texture is already cached
			std::shared_ptr<dx12::Texture> texture;
			const auto it = m_textureCache.find( filePath );
			if ( it != m_textureCache.end() )
			{
				texture = it->second;
			}
			else
			{
				// Load texture from file
				auto *textureManager = m_device->getTextureManager();
				if ( textureManager )
				{
					texture = textureManager->createTextureFromFile( filePath );
					if ( texture )
					{
						m_textureCache[filePath] = texture;
					}
				}
			}

			// Render the texture if we have it
			if ( texture )
			{
				const void *textureId = texture->getImGuiTextureId();
				if ( textureId )
				{
					// Calculate aspect-preserving size
					const float aspectRatio = static_cast<float>( texture->getWidth() ) / static_cast<float>( texture->getHeight() );
					ImVec2 imageSize;
					if ( aspectRatio > 1.0f )
					{
						// Wider than tall
						imageSize.x = cellSize;
						imageSize.y = cellSize / aspectRatio;
					}
					else
					{
						// Taller than wide
						imageSize.x = cellSize * aspectRatio;
						imageSize.y = cellSize;
					}

					// Center the image
					const float offsetX = ( cellSize - imageSize.x ) * 0.5f;
					const float offsetY = ( cellSize - imageSize.y ) * 0.5f;
					if ( offsetX > 0.0f )
						ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offsetX );
					if ( offsetY > 0.0f )
						ImGui::Dummy( ImVec2( 0.0f, offsetY ) );

					ImGui::Image( reinterpret_cast<ImTextureID>( textureId ), imageSize );

					// Make clickable
					if ( ImGui::IsItemClicked() )
					{
						selectAsset( filePath );
					}

					renderedImage = true;
				}
			}
		}

		// Fallback to text icon if no image rendered
		if ( !renderedImage )
		{
			// Display icon based on asset type
			const char *icon = "?";
			switch ( assetType )
			{
			case AssetType::Mesh:
				icon = "[M]";
				break;
			case AssetType::Texture:
				icon = "[T]";
				break;
			case AssetType::Material:
				icon = "[Mat]";
				break;
			case AssetType::Unknown:
			default:
				icon = "[?]";
				break;
			}

			// Create unique button ID using the file path
			const std::string buttonLabel = std::string( icon ) + "##" + filePath;
			if ( ImGui::Button( buttonLabel.c_str(), ImVec2( cellSize, cellSize ) ) )
			{
				selectAsset( filePath );
			}
		}

		// Tooltip on hover
		if ( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNormal ) )
		{
			const auto tooltipText = buildTooltipText( filePath );
			if ( !tooltipText.empty() )
			{
				ImGui::BeginTooltip();
				ImGui::Text( "%s", tooltipText.c_str() );
				ImGui::EndTooltip();
			}
		}

		// Drag-and-drop source
		if ( canDragAsset( filePath ) && ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
		{
			const std::string payload = getDragDropPayload( filePath );
			ImGui::SetDragDropPayload( "ASSET_BROWSER_ITEM", payload.c_str(), payload.size() + 1 );
			ImGui::Text( "Drag %s", filename.c_str() );
			ImGui::EndDragDropSource();
		}

		if ( isSelected )
		{
			ImGui::PopStyleColor();
		}

		// Display filename below icon (truncate if too long)
		const float textWidth = ImGui::CalcTextSize( filename.c_str() ).x;
		if ( textWidth > cellSize )
		{
			// Truncate and add ellipsis
			std::string truncated = filename.substr( 0, 15 ) + "...";
			ImGui::TextWrapped( "%s", truncated.c_str() );
		}
		else
		{
			ImGui::Text( "%s", filename.c_str() );
		}

		ImGui::EndGroup();

		// Update column counter
		currentColumn = ( currentColumn + 1 ) % columnCount;
	}
}

void AssetBrowserPanel::selectAsset( const std::string &assetPath )
{
	m_selectedAsset = assetPath;
}

void AssetBrowserPanel::clearSelection()
{
	m_selectedAsset.clear();
}

AssetMetadata AssetBrowserPanel::getAssetMetadata( const std::string &assetPath ) const
{
	AssetMetadata metadata;

	try
	{
		const std::filesystem::path path( assetPath );

		// Check if file exists
		if ( !std::filesystem::exists( path ) )
		{
			metadata.exists = false;
			return metadata;
		}

		metadata.exists = true;
		metadata.filename = path.filename().string();
		metadata.type = getAssetTypeFromExtension( metadata.filename );

		// Get file size
		if ( std::filesystem::is_regular_file( path ) )
		{
			metadata.sizeBytes = std::filesystem::file_size( path );
		}

		// For textures, load and extract additional metadata
		if ( metadata.type == AssetType::Texture && m_device )
		{
			// Try to load texture from cache or create new
			std::shared_ptr<dx12::Texture> texture;
			const auto it = m_textureCache.find( assetPath );
			if ( it != m_textureCache.end() )
			{
				texture = it->second;
			}
			else
			{
				// Load texture from file
				auto *textureManager = m_device->getTextureManager();
				if ( textureManager )
				{
					texture = textureManager->createTextureFromFile( assetPath );
					if ( texture )
					{
						m_textureCache[assetPath] = texture;
					}
				}
			}

			// Extract texture properties
			if ( texture )
			{
				metadata.textureWidth = texture->getWidth();
				metadata.textureHeight = texture->getHeight();
				metadata.textureFormat = static_cast<uint32_t>( texture->getFormat() );
			}
		}
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		metadata.exists = false;
	}

	return metadata;
}

void AssetBrowserPanel::renderAssetPreview()
{
	if ( m_selectedAsset.empty() )
	{
		ImGui::TextDisabled( "(no asset selected)" );
		return;
	}

	const auto metadata = getAssetMetadata( m_selectedAsset );

	if ( !metadata.exists )
	{
		ImGui::TextColored( ImVec4( 1.0f, 0.3f, 0.3f, 1.0f ), "Asset not found" );
		if ( ImGui::Button( "Clear Selection" ) )
		{
			clearSelection();
		}
		return;
	}

	// Display asset information
	ImGui::TextWrapped( "%s", metadata.filename.c_str() );
	ImGui::Separator();

	// Asset type
	const char *typeStr = "Unknown";
	switch ( metadata.type )
	{
	case AssetType::Mesh:
		typeStr = "Mesh";
		break;
	case AssetType::Texture:
		typeStr = "Texture";
		break;
	case AssetType::Material:
		typeStr = "Material";
		break;
	case AssetType::Unknown:
	default:
		typeStr = "Unknown";
		break;
	}

	ImGui::Text( "Type: %s", typeStr );

	// Display texture-specific metadata if available
	if ( metadata.type == AssetType::Texture )
	{
		if ( metadata.textureWidth > 0 && metadata.textureHeight > 0 )
		{
			ImGui::Text( "Dimensions: %u x %u", metadata.textureWidth, metadata.textureHeight );
		}

		// Display format
		if ( metadata.textureFormat != 0 )
		{
			const DXGI_FORMAT format = static_cast<DXGI_FORMAT>( metadata.textureFormat );
			const char *formatStr = "Unknown";
			switch ( format )
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				formatStr = "R8G8B8A8_UNORM";
				break;
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				formatStr = "R8G8B8A8_UNORM_SRGB";
				break;
			case DXGI_FORMAT_B8G8R8A8_UNORM:
				formatStr = "B8G8R8A8_UNORM";
				break;
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				formatStr = "B8G8R8A8_UNORM_SRGB";
				break;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
				formatStr = "R16G16B16A16_FLOAT";
				break;
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				formatStr = "R32G32B32A32_FLOAT";
				break;
			case DXGI_FORMAT_BC1_UNORM:
				formatStr = "BC1_UNORM";
				break;
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				formatStr = "BC1_UNORM_SRGB";
				break;
			case DXGI_FORMAT_BC3_UNORM:
				formatStr = "BC3_UNORM";
				break;
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				formatStr = "BC3_UNORM_SRGB";
				break;
			case DXGI_FORMAT_BC4_UNORM:
				formatStr = "BC4_UNORM";
				break;
			case DXGI_FORMAT_BC5_UNORM:
				formatStr = "BC5_UNORM";
				break;
			case DXGI_FORMAT_BC7_UNORM:
				formatStr = "BC7_UNORM";
				break;
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				formatStr = "BC7_UNORM_SRGB";
				break;
			default:
				ImGui::Text( "Format: Format(%u)", static_cast<uint32_t>( format ) );
				formatStr = nullptr;
				break;
			}
			if ( formatStr )
			{
				ImGui::Text( "Format: %s", formatStr );
			}
		}
	}

	// File size
	const double sizeKB = metadata.sizeBytes / 1024.0;
	const double sizeMB = sizeKB / 1024.0;
	if ( sizeMB >= 1.0 )
	{
		ImGui::Text( "Size: %.2f MB", sizeMB );
	}
	else
	{
		ImGui::Text( "Size: %.2f KB", sizeKB );
	}

	ImGui::Separator();

	// Clear selection button
	if ( ImGui::Button( "Clear Selection", ImVec2( -1, 0 ) ) )
	{
		clearSelection();
	}
}

bool AssetBrowserPanel::importAsset( const std::string &sourceFilePath )
{
	try
	{
		const std::filesystem::path sourcePath( sourceFilePath );

		// Check if source file exists
		if ( !std::filesystem::exists( sourcePath ) )
		{
			return false;
		}

		// Check if source is a regular file
		if ( !std::filesystem::is_regular_file( sourcePath ) )
		{
			return false;
		}

		// Check if file type is supported
		const auto assetType = getAssetTypeFromExtension( sourcePath.filename().string() );
		if ( assetType == AssetType::Unknown )
		{
			return false;
		}

		// Build destination path in current directory
		const std::filesystem::path destPath = std::filesystem::path( m_currentPath ) / sourcePath.filename();

		// Check if source and destination are the same (already imported)
		// Use canonical paths for comparison to handle different path representations
		const auto sourceCanonical = std::filesystem::canonical( sourcePath );
		const auto destCanonical = std::filesystem::weakly_canonical( destPath ); // weakly_canonical works even if dest doesn't exist

		if ( sourceCanonical == destCanonical )
		{
			return true; // Already in correct location, consider this success
		}

		// Copy file to destination (overwrite if exists)
		std::filesystem::copy_file( sourcePath, destPath, std::filesystem::copy_options::overwrite_existing );

		return true;
	}
	catch ( const std::filesystem::filesystem_error & )
	{
		return false;
	}
}

bool AssetBrowserPanel::canDragAsset( const std::string &assetPath ) const
{
	// Check if asset type is supported for drag-and-drop
	const std::filesystem::path path( assetPath );
	const auto assetType = getAssetTypeFromExtension( path.filename().string() );

	// Currently only Mesh assets support drag-to-scene
	return assetType == AssetType::Mesh;
}

std::string AssetBrowserPanel::getDragDropPayload( const std::string &assetPath ) const
{
	// Only return payload for draggable assets
	if ( !canDragAsset( assetPath ) )
	{
		return "";
	}

	// Return the full asset path as payload
	return assetPath;
}

std::string AssetBrowserPanel::buildTooltipText( const std::string &assetPath ) const
{
	const auto metadata = getAssetMetadata( assetPath );

	// Return empty string if file doesn't exist
	if ( !metadata.exists )
	{
		return "";
	}

	// Build tooltip text with filename, type, and size
	std::string tooltip;

	// Filename
	tooltip += "File: " + metadata.filename + "\n";

	// Asset type
	switch ( metadata.type )
	{
	case AssetType::Mesh:
		tooltip += "Type: Mesh\n";
		break;
	case AssetType::Texture:
		tooltip += "Type: Texture\n";
		// Add texture dimensions if available
		if ( metadata.textureWidth > 0 && metadata.textureHeight > 0 )
		{
			char dimBuffer[32];
			snprintf( dimBuffer, sizeof( dimBuffer ), "Dimensions: %ux%u\n", metadata.textureWidth, metadata.textureHeight );
			tooltip += dimBuffer;
		}
		// Add texture format if available
		if ( metadata.textureFormat != 0 )
		{
			const DXGI_FORMAT format = static_cast<DXGI_FORMAT>( metadata.textureFormat );
			// Use DirectX format name lookup
			try
			{
				// Note: D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetName() is available in d3dx12 headers
				// For now, we'll use a simple approach to format common texture formats
				std::string formatStr;
				switch ( format )
				{
				case DXGI_FORMAT_R8G8B8A8_UNORM:
					formatStr = "R8G8B8A8_UNORM";
					break;
				case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
					formatStr = "R8G8B8A8_UNORM_SRGB";
					break;
				case DXGI_FORMAT_B8G8R8A8_UNORM:
					formatStr = "B8G8R8A8_UNORM";
					break;
				case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
					formatStr = "B8G8R8A8_UNORM_SRGB";
					break;
				case DXGI_FORMAT_R16G16B16A16_FLOAT:
					formatStr = "R16G16B16A16_FLOAT";
					break;
				case DXGI_FORMAT_R32G32B32A32_FLOAT:
					formatStr = "R32G32B32A32_FLOAT";
					break;
				case DXGI_FORMAT_BC1_UNORM:
					formatStr = "BC1_UNORM";
					break;
				case DXGI_FORMAT_BC1_UNORM_SRGB:
					formatStr = "BC1_UNORM_SRGB";
					break;
				case DXGI_FORMAT_BC3_UNORM:
					formatStr = "BC3_UNORM";
					break;
				case DXGI_FORMAT_BC3_UNORM_SRGB:
					formatStr = "BC3_UNORM_SRGB";
					break;
				case DXGI_FORMAT_BC4_UNORM:
					formatStr = "BC4_UNORM";
					break;
				case DXGI_FORMAT_BC5_UNORM:
					formatStr = "BC5_UNORM";
					break;
				case DXGI_FORMAT_BC7_UNORM:
					formatStr = "BC7_UNORM";
					break;
				case DXGI_FORMAT_BC7_UNORM_SRGB:
					formatStr = "BC7_UNORM_SRGB";
					break;
				default:
					char formatBuffer[32];
					snprintf( formatBuffer, sizeof( formatBuffer ), "Format(%u)", static_cast<uint32_t>( format ) );
					formatStr = formatBuffer;
					break;
				}
				tooltip += "Format: " + formatStr + "\n";
			}
			catch ( const std::exception & )
			{
				// If format lookup fails, skip it
			}
		}
		break;
	case AssetType::Material:
		tooltip += "Type: Material\n";
		break;
	case AssetType::Unknown:
		tooltip += "Type: Unknown\n";
		break;
	}

	// File size (convert to human-readable format)
	const double sizeKB = metadata.sizeBytes / 1024.0;
	const double sizeMB = sizeKB / 1024.0;

	if ( sizeMB >= 1.0 )
	{
		char sizeBuffer[32];
		snprintf( sizeBuffer, sizeof( sizeBuffer ), "Size: %.2f MB", sizeMB );
		tooltip += sizeBuffer;
	}
	else if ( sizeKB >= 1.0 )
	{
		char sizeBuffer[32];
		snprintf( sizeBuffer, sizeof( sizeBuffer ), "Size: %.2f KB", sizeKB );
		tooltip += sizeBuffer;
	}
	else
	{
		char sizeBuffer[32];
		snprintf( sizeBuffer, sizeof( sizeBuffer ), "Size: %zu B", metadata.sizeBytes );
		tooltip += sizeBuffer;
	}

	return tooltip;
}

} // namespace editor

#include "AssetBrowserPanel.h"
#include "engine/assets/asset_manager.h"
#include "runtime/ecs.h"
#include "editor/commands/CommandHistory.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>

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

		// Right panel: Current directory contents
		ImGui::BeginChild( "DirectoryContents", ImVec2( 0, 0 ), true );

		// Path breadcrumbs
		renderPathBar();
		ImGui::Separator();

		// Get directory contents
		const auto contents = getDirectoryContents( m_currentPath );

		// Display directories and files
		for ( const auto &item : contents )
		{
			const bool isDir = isDirectory( item );
			const std::string displayName = std::filesystem::path( item ).filename().string();

			if ( isDir )
			{
				// Display folder with icon
				ImGui::Text( "[DIR] %s", displayName.c_str() );
			}
			else
			{
				// Display file
				ImGui::Text( "      %s", displayName.c_str() );
			}
		}

		// Show message for empty directories
		if ( contents.empty() )
		{
			ImGui::TextDisabled( "(empty directory)" );
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
		std::vector<std::string> subdirs;
		for ( const auto &entry : std::filesystem::directory_iterator( path ) )
		{
			if ( entry.is_directory() )
			{
				subdirs.push_back( entry.path().string() );
			}
		}

		// Sort alphabetically
		std::sort( subdirs.begin(), subdirs.end() );

		// Render tree nodes for each subdirectory
		for ( const auto &subdir : subdirs )
		{
			const std::string displayName = std::filesystem::path( subdir ).filename().string();
			const bool isCurrentPath = ( subdir == m_currentPath );

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
				navigateToDirectory( subdir );
			}

			// Recursively render children if node is open and has subdirectories
			if ( nodeOpen && hasSubdirs )
			{
				renderDirectoryTree( subdir );
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
		if ( ImGui::Button( name.c_str() ) )
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

} // namespace editor

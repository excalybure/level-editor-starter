#include "CommandUI.h"

// Include ImGui after our headers to avoid conflicts
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

UndoRedoUI::UndoRedoUI( CommandHistory &commandHistory )
	: m_commandHistory( commandHistory )
{
}

bool UndoRedoUI::handleKeyboardShortcuts()
{
	const auto &io = ImGui::GetIO();

	// Only handle shortcuts when Ctrl is pressed
	if ( !io.KeyCtrl )
		return false;

	// Handle Undo (Ctrl+Z) - but not when Shift is also pressed
	if ( !io.KeyShift && ImGui::IsKeyPressed( ImGuiKey_Z, false ) )
	{
		if ( canUndo() )
		{
			return executeUndo();
		}
		return false;
	}

	// Handle Redo (Ctrl+Y) & (Ctrl+Shift+Z)
	bool redo = ImGui::IsKeyPressed( ImGuiKey_Y, false );
	redo = redo || ( io.KeyShift && ImGui::IsKeyPressed( ImGuiKey_Z, false ) );
	if ( redo )
	{
		if ( canRedo() )
		{
			return executeRedo();
		}
		return false;
	}

	return false;
}

void UndoRedoUI::renderMenuItems()
{
	const bool undoAvailable = canUndo();
	const bool redoAvailable = canRedo();

	// Build menu item text with operation description if available
	std::string undoText = "Undo";
	if ( undoAvailable )
	{
		const auto undoDesc = getUndoDescription();
		if ( !undoDesc.empty() )
		{
			undoText = "Undo " + undoDesc;
		}
	}
	undoText += "\tCtrl+Z";

	std::string redoText = "Redo";
	if ( redoAvailable )
	{
		const auto redoDesc = getRedoDescription();
		if ( !redoDesc.empty() )
		{
			redoText = "Redo " + redoDesc;
		}
	}
	redoText += "\tCtrl+Y";

	// Render undo menu item
	if ( ImGui::MenuItem( undoText.c_str(), nullptr, false, undoAvailable ) )
	{
		executeUndo();
	}

	// Render redo menu item
	if ( ImGui::MenuItem( redoText.c_str(), nullptr, false, redoAvailable ) )
	{
		executeRedo();
	}
}

bool UndoRedoUI::renderToolbarButtons()
{
	bool actionPerformed = false;

	const bool undoAvailable = canUndo();
	const bool redoAvailable = canRedo();

	// Undo button
	if ( !undoAvailable )
	{
		// Disable button visually
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.6f );
	}

	if ( ImGui::Button( "Undo" ) && undoAvailable )
	{
		executeUndo();
		actionPerformed = true;
	}

	if ( !undoAvailable )
	{
		ImGui::PopStyleVar();
	}

	// Tooltip with operation description
	if ( ImGui::IsItemHovered() && undoAvailable )
	{
		const auto undoDesc = getUndoDescription();
		if ( !undoDesc.empty() )
		{
			ImGui::SetTooltip( "Undo: %s", undoDesc.c_str() );
		}
	}

	ImGui::SameLine();

	// Redo button
	if ( !redoAvailable )
	{
		// Disable button visually
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.6f );
	}

	if ( ImGui::Button( "Redo" ) && redoAvailable )
	{
		executeRedo();
		actionPerformed = true;
	}

	if ( !redoAvailable )
	{
		ImGui::PopStyleVar();
	}

	// Tooltip with operation description
	if ( ImGui::IsItemHovered() && redoAvailable )
	{
		const auto redoDesc = getRedoDescription();
		if ( !redoDesc.empty() )
		{
			ImGui::SetTooltip( "Redo: %s", redoDesc.c_str() );
		}
	}

	return actionPerformed;
}

bool UndoRedoUI::canUndo() const
{
	return m_commandHistory.canUndo();
}

bool UndoRedoUI::canRedo() const
{
	return m_commandHistory.canRedo();
}

std::string UndoRedoUI::getUndoDescription() const
{
	if ( !canUndo() )
		return "";

	// TODO: Access command history to get the description of the next undo operation
	// For now, return a placeholder - this will need to be implemented when CommandHistory
	// provides access to command descriptions
	return "Operation";
}

std::string UndoRedoUI::getRedoDescription() const
{
	if ( !canRedo() )
		return "";

	// TODO: Access command history to get the description of the next redo operation
	// For now, return a placeholder - this will need to be implemented when CommandHistory
	// provides access to command descriptions
	return "Operation";
}

void UndoRedoUI::addHistoryChangeListener( const HistoryChangeListener &listener )
{
	m_listeners.push_back( listener );
}

void UndoRedoUI::clearHistoryChangeListeners()
{
	m_listeners.clear();
}

bool UndoRedoUI::executeUndo()
{
	const bool success = m_commandHistory.undo();
	if ( success )
	{
		notifyHistoryChange();
	}
	return success;
}

bool UndoRedoUI::executeRedo()
{
	const bool success = m_commandHistory.redo();
	if ( success )
	{
		notifyHistoryChange();
	}
	return success;
}

void UndoRedoUI::notifyHistoryChange()
{
	for ( const auto &listener : m_listeners )
	{
		listener();
	}
}

// CommandHistoryWindow implementation
CommandHistoryWindow::CommandHistoryWindow( CommandHistory &commandHistory )
	: m_commandHistory( commandHistory )
{
}

void CommandHistoryWindow::render( bool *isOpen )
{
	if ( !isOpen || !*isOpen )
		return;

	if ( ImGui::Begin( "Command History", isOpen ) )
	{
		renderMemoryInfo();
		ImGui::Separator();
		renderCommandList();
	}
	ImGui::End();
}

void CommandHistoryWindow::renderCommandList()
{
	ImGui::Text( "Commands in history: %zu", m_commandHistory.getCommandCount() );
	ImGui::Text( "Can Undo: %s", m_commandHistory.canUndo() ? "Yes" : "No" );
	ImGui::Text( "Can Redo: %s", m_commandHistory.canRedo() ? "Yes" : "No" );

	// TODO: This would need more API support from CommandHistory to show individual commands
	// For now, just show basic information
	ImGui::Text( "Command details require additional CommandHistory API support" );
}

void CommandHistoryWindow::renderMemoryInfo()
{
	const size_t currentMemory = m_commandHistory.getCurrentMemoryUsage();
	const size_t maxMemory = m_commandHistory.getMaxMemoryUsage();
	const float memoryPercent = maxMemory > 0 ? static_cast<float>( currentMemory ) / maxMemory : 0.0f;

	ImGui::Text( "Memory Usage:" );
	ImGui::SameLine();
	ImGui::ProgressBar( memoryPercent, ImVec2( 200, 0 ), ( formatMemorySize( currentMemory ) + " / " + formatMemorySize( maxMemory ) ).c_str() );

	ImGui::Text( "Command Count: %zu / %zu",
		m_commandHistory.getCommandCount(),
		m_commandHistory.getMaxCommands() );
}

std::string CommandHistoryWindow::formatMemorySize( size_t bytes ) const
{
	std::ostringstream ss;
	ss << std::fixed << std::setprecision( 1 );

	if ( bytes >= 1024 * 1024 * 1024 )
	{
		ss << static_cast<double>( bytes ) / ( 1024 * 1024 * 1024 ) << " GB";
	}
	else if ( bytes >= 1024 * 1024 )
	{
		ss << static_cast<double>( bytes ) / ( 1024 * 1024 ) << " MB";
	}
	else if ( bytes >= 1024 )
	{
		ss << static_cast<double>( bytes ) / 1024 << " KB";
	}
	else
	{
		ss << bytes << " bytes";
	}

	return ss.str();
}
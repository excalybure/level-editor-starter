#pragma once

#include "CommandHistory.h"
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief UI integration for command system with undo/redo controls
 * 
 * Provides ImGui-based UI controls for command history management including:
 * - Menu items for Undo/Redo operations
 * - Toolbar buttons with visual feedback
 * - Keyboard shortcut handling (Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z)
 * - Command history window for debugging and power users
 */
class UndoRedoUI
{
public:
	/**
	 * @brief Listener callback for UI updates when history changes
	 */
	using HistoryChangeListener = std::function<void()>;

	/**
	 * @brief Construct UndoRedoUI with command history reference
	 * @param commandHistory The command history to manage
	 */
	explicit UndoRedoUI( CommandHistory &commandHistory );

	/**
	 * @brief Destructor
	 */
	~UndoRedoUI() = default;

	// No copy/move for now
	UndoRedoUI( const UndoRedoUI & ) = delete;
	UndoRedoUI &operator=( const UndoRedoUI & ) = delete;

	/**
	 * @brief Handle keyboard shortcuts for undo/redo operations
	 * Should be called every frame to detect keyboard input
	 * @return true if a shortcut was handled
	 */
	bool handleKeyboardShortcuts();

	/**
	 * @brief Render undo/redo menu items in current menu context
	 * Should be called within ImGui::BeginMenu() context
	 */
	void renderMenuItems();

	/**
	 * @brief Render undo/redo toolbar buttons
	 * Should be called within toolbar rendering context
	 * @return true if any button was clicked
	 */
	bool renderToolbarButtons();

	/**
	 * @brief Check if undo operation is possible
	 * @return true if undo is available
	 */
	bool canUndo() const;

	/**
	 * @brief Check if redo operation is possible
	 * @return true if redo is available
	 */
	bool canRedo() const;

	/**
	 * @brief Get description of next undo operation
	 * @return Description string or empty if no undo available
	 */
	std::string getUndoDescription() const;

	/**
	 * @brief Get description of next redo operation
	 * @return Description string or empty if no redo available
	 */
	std::string getRedoDescription() const;

	/**
	 * @brief Add a listener for history change notifications
	 * @param listener Callback to be called when history changes
	 */
	void addHistoryChangeListener( const HistoryChangeListener &listener );

	/**
	 * @brief Remove all history change listeners
	 */
	void clearHistoryChangeListeners();

private:
	CommandHistory &m_commandHistory;
	std::vector<HistoryChangeListener> m_listeners;

	/**
	 * @brief Notify all listeners that history has changed
	 */
	void notifyHistoryChange();

	/**
	 * @brief Execute undo operation with listener notification
	 * @return true if undo was successful
	 */
	bool executeUndo();

	/**
	 * @brief Execute redo operation with listener notification
	 * @return true if redo was successful
	 */
	bool executeRedo();
};

/**
 * @brief Command history visualization window for debugging and power users
 * 
 * Displays complete command history with:
 * - Command descriptions and timestamps
 * - Memory usage information
 * - Ability to jump to specific commands in history
 * - Real-time updates as commands are executed
 */
class CommandHistoryWindow
{
public:
	/**
	 * @brief Construct CommandHistoryWindow with command history reference
	 * @param commandHistory The command history to visualize
	 */
	explicit CommandHistoryWindow( CommandHistory &commandHistory );

	/**
	 * @brief Destructor
	 */
	~CommandHistoryWindow() = default;

	// No copy/move for now
	CommandHistoryWindow( const CommandHistoryWindow & ) = delete;
	CommandHistoryWindow &operator=( const CommandHistoryWindow & ) = delete;

	/**
	 * @brief Render the command history window
	 * @param isOpen Pointer to bool controlling window visibility
	 */
	void render( bool *isOpen );

	/**
	 * @brief Check if the window is currently visible
	 * @return true if window should be rendered
	 */
	bool isVisible() const { return m_isVisible; }

	/**
	 * @brief Set window visibility
	 * @param visible true to show window, false to hide
	 */
	void setVisible( bool visible ) { m_isVisible = visible; }

private:
	CommandHistory &m_commandHistory;
	bool m_isVisible = false;

	/**
	 * @brief Render the command list with selection support
	 */
	void renderCommandList();

	/**
	 * @brief Render memory usage information
	 */
	void renderMemoryInfo();

	/**
	 * @brief Format memory size for display
	 * @param bytes Memory size in bytes
	 * @return Formatted string (e.g., "1.5 MB")
	 */
	std::string formatMemorySize( size_t bytes ) const;
};
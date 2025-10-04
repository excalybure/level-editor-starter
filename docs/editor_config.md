# Implementation Plan: Window Visibility Configuration System

## Overview
Based on the attached `imgui_visibility.md`, we need to create a custom configuration system to persist window visibility state across sessions. ImGui's `imgui.ini` handles window geometry and docking but NOT visibility state.

## Current State Analysis

### Windows That Need Visibility Management:
1. **Scene Editing Panels** (already have `m_visible` flags):
   - Scene Hierarchy Panel (`showHierarchyPanel`)
   - Entity Inspector Panel (`showInspectorPanel`)
   - Asset Browser Panel (`showAssetBrowserPanel`)

2. **Tool Windows** (already have visibility flags):
   - Grid Settings Window (`showGridSettingsWindow`)
   - Camera Settings Window (`showCameraSettingsWindow`)
   - Gizmo Tools Window (`showGizmoToolsWindow`)
   - Gizmo Settings Window (`showGizmoSettingsWindow`)
   - Command History Window (`showCommandHistoryWindow`)

3. **Viewport Panes** (have `isOpen` flags in `ViewportLayout`):
   - Perspective viewport
   - Top (XY) viewport
   - Front (XZ) viewport
   - Side (YZ) viewport

### Current Issues:
- All visibility flags are stored in `UI::Impl` but not persisted
- When application restarts, windows revert to hardcoded defaults
- No way for users to have their preferred panel layout persist

---

## Proposed Solution Architecture

### 1. Config Module (`editor/config`)
**Files to create:**
- `src/editor/config/EditorConfig.h` - Public interface
- `src/editor/config/EditorConfig.cpp` - Implementation using nlohmann/json

**Responsibilities:**
- Load/save JSON configuration file (`editor_config.json` in working directory)
- Provide type-safe getters/setters for boolean, string, int, float values
- Auto-save on application shutdown
- Lazy load on first access

**Key Features:**
- Hierarchical key paths using dot notation (e.g., `"ui.panels.hierarchy.visible"`)
- Default values when keys don't exist
- Thread-safe (may be accessed from different systems in future)
- Error handling with console logging (following project conventions)

### 2. Window State Registry
**Integration point:** `UI::Impl` in `src/editor/ui.cpp`

**Responsibilities:**
- Register all windows that need persistence
- Save visibility state to config on shutdown
- Restore visibility state on startup

---

## Implementation Tasks Breakdown

### Phase 1: Core Config System (Foundation)
**Atomic Functionalities:**

**AF1.1**: Create `EditorConfig` class structure
- Header with class declaration
- Constructor/destructor
- Basic member variables (file path, JSON object)
- No implementation yet

**AF1.2**: Implement config file loading
- `load()` method reads JSON from disk
- Handle file not found (use defaults)
- Handle parse errors (log and use defaults)
- Store loaded data in member variable

**AF1.3**: Implement config file saving
- `save()` method writes JSON to disk
- Create parent directories if needed
- Handle write errors (log and fail gracefully)
- Pretty-print JSON for readability

**AF1.4**: Implement boolean get/set operations
- `getBool(key, defaultValue)` retrieves boolean
- `setBool(key, value)` stores boolean
- Support dot-notation paths (parse into nested objects)
- Return default if key doesn't exist

**AF1.5**: Add file existence and directory utilities
- Check if config file exists
- Create config directory if missing
- Platform-independent path handling (use `<filesystem>`)

### Phase 2: UI Integration (Connection)
**Atomic Functionalities:**

**AF2.1**: Add `EditorConfig` member to `UI::Impl`
- Include config header
- Add `std::unique_ptr<EditorConfig> config` member
- Initialize in `UI::initialize()`
- Cleanup in `UI::shutdown()`

**AF2.2**: Define config keys as constants
- Create string constants for all window keys
- Group by category (panels, tools, viewports)
- Document default values
- Example: `"ui.panels.hierarchy.visible"`, `"ui.tools.grid.visible"`

**AF2.3**: Implement config save on shutdown
- Call `saveWindowStates()` in `UI::shutdown()`
- Save all panel visibility flags to config
- Save all tool window flags to config
- Save all viewport pane states to config
- Call `config->save()` to persist to disk

**AF2.4**: Implement config load on startup
- Call `loadWindowStates()` in `UI::initialize()`
- Read panel visibility from config (with defaults)
- Read tool window visibility from config (with defaults)
- Read viewport pane visibility from config (with defaults)
- Apply loaded values to UI state variables

**AF2.5**: Handle first-run scenario
- Detect when config file doesn't exist
- Use hardcoded defaults (current behavior)
- Save initial config on first shutdown
- Log info message about creating new config

### Phase 3: Dynamic Updates (Polish)
**Atomic Functionalities:**

**AF3.1**: Auto-save on window visibility changes
- Detect when panel visibility changes in menu
- Update config immediately (or after small delay)
- Mark config as dirty for batch save
- Avoid file I/O spam (debounce saves)

**AF3.2**: Add config versioning
- Add `"version"` field to JSON (start at 1)
- Check version on load
- Handle version mismatches gracefully
- Plan for future schema migrations

**AF3.3**: Add reset-to-defaults functionality
- Create `resetToDefaults()` method
- Clear all UI-related keys
- Restore hardcoded default values
- Optionally add menu item for this

---

## Configuration Schema (JSON Structure)

```json
{
  "version": 1,
  "ui": {
    "panels": {
      "sceneHierarchy": {
        "visible": true
      },
      "entityInspector": {
        "visible": true
      },
      "assetBrowser": {
        "visible": true
      }
    },
    "tools": {
      "grid": {
        "visible": false
      },
      "camera": {
        "visible": false
      },
      "gizmoTools": {
        "visible": true
      },
      "gizmoSettings": {
        "visible": true
      },
      "commandHistory": {
        "visible": false
      }
    },
    "viewports": {
      "perspective": {
        "visible": true
      },
      "top": {
        "visible": true
      },
      "front": {
        "visible": true
      },
      "side": {
        "visible": true
      }
    }
  }
}
```

---

## File Structure

```
src/
  editor/
    config/
      EditorConfig.h          # Public interface
      EditorConfig.cpp        # Implementation
    ui.h                      # Add config getter (optional)
    ui.cpp                    # Integrate config load/save

tests/
  editor_config_tests.cpp     # Unit tests for config system

editor_config.json           # Created at runtime (gitignored)
```

---

## Testing Strategy

### Unit Tests (per TDD requirements):
1. **Config File I/O**:
   - Test creating new config file
   - Test loading existing config file
   - Test handling missing file (use defaults)
   - Test handling malformed JSON (use defaults)
   - Test saving config to disk

2. **Key-Value Operations**:
   - Test getBool with existing key
   - Test getBool with missing key (returns default)
   - Test setBool creates nested structure
   - Test dot-notation path parsing

3. **UI Integration**:
   - Test visibility state is saved on shutdown
   - Test visibility state is restored on startup
   - Test default values when config missing
   - Mock config in tests (avoid file I/O in most tests)

### Integration Testing:
- Manual testing: close panels, restart app, verify they stay closed
- Manual testing: open all tools, restart, verify they reopen
- Manual testing: delete config file, restart, verify defaults

---

## Dependencies & Compatibility

### Existing Systems:
- **nlohmann/json**: Already available in vcpkg dependencies
- **Console module**: Use for error/info logging
- **std::filesystem**: C++17 feature for path handling

### CMakeLists.txt Updates:
```cmake
# In src/editor/CMakeLists.txt (or equivalent)
target_sources(editor_lib PRIVATE
  config/EditorConfig.cpp
)

target_link_libraries(editor_lib
  PRIVATE
    nlohmann_json::nlohmann_json
    runtime.console  # For logging
)
```

---

## Non-Goals (Future Extensions)

The following are explicitly **OUT OF SCOPE** for this implementation but could be added later:

1. **User Preferences**: Camera speed, grid color, snap settings, etc.
2. **Recent Files List**: Track recently opened scenes
3. **Editor Layout Profiles**: Save/load named layout configurations
4. **Per-Project Settings**: Different config per project/scene
5. **Cloud Sync**: Sync settings across machines
6. **UI for Config Editing**: Settings panel to view/edit raw config

These can be added incrementally by extending the same `EditorConfig` system.

---

## Migration Path & Backward Compatibility

### First Release:
- Config file is created on first run with defaults
- No migration needed (new feature)

### Future Updates:
- Use `version` field to detect old configs
- Write migration functions for schema changes
- Preserve unknown keys (forward compatibility)

---

## Risk Assessment

### Low Risk:
- Config system is isolated from rendering/core logic
- Failure to load/save config is non-fatal (use defaults)
- JSON library is mature and widely used

### Medium Risk:
- File I/O could fail (disk full, permissions)
  - **Mitigation**: Log errors, continue with defaults
- Config corruption (manual editing)
  - **Mitigation**: Validate on load, fallback to defaults

### No Risk:
- Does not affect existing features
- Purely additive functionality
- Can be disabled if problematic (use hardcoded defaults)

---

## Success Criteria

### Functional:
✅ Window visibility state persists across application restarts  
✅ User can close panels and they stay closed on next launch  
✅ User can open tools and they reopen automatically  
✅ Default layout is used when config file is missing  
✅ Invalid config files are handled gracefully  

### Technical:
✅ All code follows C++23 guidelines (const correctness, modern features)  
✅ Unit tests cover config load/save/get/set operations  
✅ Integration with UI system is clean and non-invasive  
✅ Logging uses existing console module  
✅ File I/O errors are handled without crashes  

### User Experience:
✅ Seamless persistence (no user action required)  
✅ First-run experience is unchanged (uses defaults)  
✅ Configuration file is human-readable (JSON)  
✅ No performance impact (load/save only at startup/shutdown)  

---

## Timeline Estimate (TDD)

- **Phase 1** (Core Config): ~6-8 atomic functionalities × TDD cycle = 3-4 hours
- **Phase 2** (UI Integration): ~5 atomic functionalities × TDD cycle = 2-3 hours  
- **Phase 3** (Polish): ~3 atomic functionalities × TDD cycle = 1-2 hours

**Total**: ~6-9 hours of development time (with strict TDD)

---

## Open Questions for Review

1. **Config file location**: Should it be in working directory, user home, or alongside `imgui.ini`?
2. **Auto-save frequency**: Save immediately on every change or batch on shutdown?
3. **Config scope**: Should we plan for per-project settings now or defer?
4. **Additional preferences**: Are there other settings we should persist now (camera speed, grid settings)?
5. **Menu integration**: Should we add "Reset Layout" menu item in View menu?

---

## Summary

This implementation plan provides a **minimal, focused solution** to the window visibility persistence problem described in `imgui_visibility.md`. It:

- ✅ Complements ImGui's built-in `imgui.ini` (geometry/docking)
- ✅ Handles only visibility state (focused scope)
- ✅ Uses established patterns (JSON, dot-notation keys)
- ✅ Follows project conventions (TDD, C++23, console logging)
- ✅ Extensible for future preferences (versioning, schema)
- ✅ Low risk, high value for UX improvement

**Ready for implementation once approved!**

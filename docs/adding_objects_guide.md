# Adding Objects to Your Scene

## Overview

This guide explains how to add objects (entities) to your scene using the Level Editor. There are three primary workflows for creating entities from assets:

1. **Drag-and-Drop from Asset Browser to Viewport** - Place objects at world origin
2. **Drag-and-Drop from Asset Browser to Hierarchy Panel** - Create child entities at parent position
3. **Create Menu (Keyboard-Driven)** - Use menu commands and keyboard shortcuts

All creation methods support full **Undo/Redo** via the command history system.

---

## Method 1: Drag-and-Drop to Viewport

**Best for**: Quick placement of objects at world origin

### Steps:

1. **Open the Asset Browser Panel** (usually at the bottom of the editor)
2. **Navigate** to the folder containing your asset (e.g., `.gltf` or `.glb` files)
3. **Select** an asset in the grid view by clicking on it
4. **Drag** the asset from the Asset Browser by clicking and holding the mouse button
   - A drag preview will appear showing the filename (e.g., "Drag my_model.gltf")
5. **Hover** over the Viewport window
   - The viewport will show a **blue highlight border** when it's ready to accept the drop
6. **Release** the mouse button to drop the asset
   - A **toast notification** will appear in the bottom-right corner confirming creation
   - The entity will be created at world origin `(0, 0, 0)`

### Visual Feedback:
- **During Drag**: Blue border (3px) appears around viewport when hovering
- **On Success**: Green toast notification with ✓ icon: "Entity created from asset"
- **On Error**: Red toast notification with ✗ icon showing the error message

### Example:
```
Asset Browser: models/spaceship.gltf
         ↓ (drag)
Viewport (blue border appears)
         ↓ (drop)
Entity "spaceship" created at (0, 0, 0)
```

---

## Method 2: Drag-and-Drop to Hierarchy Panel

**Best for**: Creating child entities with specific parent-child relationships

### Steps:

1. **Open the Asset Browser Panel** and navigate to your asset
2. **Open the Scene Hierarchy Panel** (usually on the left side)
3. **Drag** the asset from the Asset Browser
4. **Hover** over a **parent entity** in the hierarchy panel
   - The hierarchy node will show a **blue background highlight** when it's ready to accept the drop
5. **Release** the mouse button to drop the asset onto the parent entity
   - The new entity will be created as a **child** of the parent
   - The child entity will be positioned at the **parent's transform position**
   - A **console message** will log the creation (check console for details)

### Visual Feedback:
- **During Drag (Asset)**: Blue background highlight on hierarchy nodes
- **During Drag (Entity Reparenting)**: Yellow background highlight (different operation)
- **On Success**: Console message: "Created entity from asset: [filename] under parent: [parent name]"
- **On Error**: Console error message with details

### Example:
```
Asset Browser: models/weapon.gltf
         ↓ (drag)
Hierarchy Panel: "Player" entity (blue highlight appears)
         ↓ (drop)
Child entity "weapon" created under "Player" at Player's position
```

### Benefits:
- Automatically sets up parent-child relationship
- Child inherits parent's transform (position, rotation, scale)
- Useful for creating complex hierarchies (e.g., character with equipment)

---

## Method 3: Create Menu (Keyboard-Driven)

**Best for**: Accessibility, keyboard-focused workflows, and precise control

### Steps:

1. **Open the Edit Menu** from the main menu bar
2. **Navigate** to `Edit → Create Entity` (submenu)
3. **Select** one of the following options:
   - **Empty Entity** (`Ins` key) - Creates an empty entity at origin
   - **From Asset File...** (`Ctrl+Shift+N`) - Opens file dialog to select asset

### Using "From Asset File..." Option:

1. **Press** `Ctrl+Shift+N` or click `Edit → Create Entity → From Asset File...`
2. **File Dialog** opens with glTF file filter
   - Filter: "glTF Files (*.gltf, *.glb)"
   - Navigate to your asset folder
3. **Select** a `.gltf` or `.glb` file
4. **Click** "Open" button
   - Entity will be created at world origin `(0, 0, 0)`
   - **Toast notification** confirms creation or shows error

### Keyboard Shortcuts:
- **`Ins`** - Create Empty Entity
- **`Ctrl+Shift+N`** - Create Entity from Asset File

### Visual Feedback:
- **On Success**: Green toast notification: "Entity created from asset: [filename]"
- **On Error**: Red toast notification with error details (e.g., "Failed to load asset")

---

## Visual Feedback Summary

### Toast Notifications:
- **Location**: Bottom-right corner of the viewport
- **Duration**: 3 seconds (with 0.5-second fade-out)
- **Success Icon**: ✓ with green background
- **Error Icon**: ✗ with red background
- **Queue Limit**: Maximum 5 toasts visible at once (stacked vertically)

### Drop Target Highlights:
- **Viewport**: 3px blue border (`IM_COL32(100, 200, 255, 200)`)
- **Hierarchy (Asset Drop)**: Blue background fill (`IM_COL32(100, 200, 255, 80)`)
- **Hierarchy (Reparenting)**: Yellow background fill (`IM_COL32(255, 255, 100, 80)`)

---

## Undo and Redo

All entity creation operations are fully undoable:

- **Undo**: `Ctrl+Z` or `Edit → Undo`
- **Redo**: `Ctrl+Y` or `Edit → Redo`

### What Gets Undone:
- Entity creation
- All components (Transform, MeshRenderer, Visible, etc.)
- Hierarchy relationships (parent-child)
- Asset loading and references

---

## Supported Asset Formats

Currently supported:
- **glTF 2.0** (`.gltf`) - Text-based format
- **GLB** (`.glb`) - Binary glTF format

### Asset Requirements:
- Must be valid glTF 2.0 format
- Can contain single or multiple nodes (hierarchies preserved)
- Meshes, materials, and transforms are imported automatically

---

## Common Workflows

### Workflow 1: Building a Simple Scene
1. Use `Edit → Create Entity → Empty Entity` to create a parent "Level" entity
2. Drag assets from Asset Browser to the "Level" entity in hierarchy
3. Children are positioned at parent's location
4. Adjust transforms in Entity Inspector

### Workflow 2: Creating a Character with Equipment
1. Drag character model to viewport (creates root entity)
2. Select character entity in hierarchy
3. Drag weapon/armor models onto character entity
4. Equipment becomes children, positioned at character's location

### Workflow 3: Rapid Prototyping
1. Use `Ctrl+Shift+N` to quickly add multiple assets
2. Select assets one by one from file dialog
3. Use Entity Inspector to adjust positions after creation

---

## Troubleshooting

### Issue: "Failed to load asset" Error

**Causes:**
- File path is incorrect or file doesn't exist
- Asset file is not valid glTF format
- Asset file is corrupted

**Solutions:**
- Verify the file exists at the specified path
- Open the asset in a glTF validator (e.g., https://gltf-viewer.donmccurdy.com/)
- Try re-exporting the asset from your 3D modeling software

---

### Issue: Entity Created but Not Visible

**Causes:**
- Entity created at origin behind camera
- Visible component set to `false`
- No mesh data in the asset

**Solutions:**
- Press `F` key in Scene Hierarchy to focus camera on selected entity
- Check Entity Inspector → Visible component checkbox
- Verify the asset contains mesh data (open in external glTF viewer)

---

### Issue: Drop Target Not Highlighting

**Causes:**
- Dragging unsupported file type (not .gltf/.glb)
- Asset Browser not properly initialized
- Viewport or Hierarchy panel not focused

**Solutions:**
- Verify you're dragging a `.gltf` or `.glb` file
- Ensure Asset Browser has loaded the directory contents
- Click once on the viewport/hierarchy panel to focus it

---

### Issue: Parent-Child Relationship Not Created

**Causes:**
- Dropped asset on viewport instead of hierarchy node
- Dropped on empty space in hierarchy (creates root entity)

**Solutions:**
- Ensure you're dropping directly onto a hierarchy node (blue highlight should appear)
- If dropped on empty space, entity is created as root (not a child)
- Use Entity Inspector to manually set parent if needed

---

## Advanced Topics

### Asset Loading and Caching
- Assets are loaded via the `AssetManager` singleton
- Once loaded, assets are cached for reuse
- Multiple entities can reference the same asset (instancing)

### Command System Integration
- All creation operations use `CreateEntityFromAssetCommand`
- Commands are executed via `CommandHistory`
- Full command metadata available in Command History window

### Scene Serialization
- Entities created from assets are serialized in scene files
- Asset references stored as relative paths
- Hierarchy relationships preserved on save/load

---

## Keyboard Shortcuts Reference

| Shortcut | Action |
|----------|--------|
| `Ins` | Create Empty Entity |
| `Ctrl+Shift+N` | Create Entity from Asset File |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `F` | Focus on Selected Entity (in Hierarchy) |
| `Delete` | Delete Selected Entity |

---

## See Also

- [Scene Format Specification](scene_format.md) - Details on scene serialization
- [Asset Browser Guide](asset_browser_guide.md) - Asset management workflows
- [Entity Inspector Guide](entity_inspector_guide.md) - Component editing
- [Keyboard Shortcuts](keyboard_shortcuts.md) - Complete shortcut reference

---

## Feedback and Issues

If you encounter issues or have suggestions for improving the object creation workflow, please:
- Check the console output for detailed error messages
- Verify your asset files are valid glTF format
- Report issues with asset path, error message, and steps to reproduce

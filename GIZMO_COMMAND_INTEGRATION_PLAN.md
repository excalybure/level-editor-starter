# Gizmo-Command Integration Implementation Plan
**Date**: 2025-09-30  
**Task**: M2-P5-T3-AF3.6 - Integrate with GizmoSystem for seamless manipulation  
**Status**: Ready for Implementation

---

## Executive Summary

**Objective**: Complete Task 3 AF3.6 by integrating the GizmoSystem with CommandHistory so that all gizmo manipulations create undoable/redoable commands.

**Current State**: 
- ✅ Command infrastructure fully implemented and tested
- ✅ TransformEntityCommand and BatchTransformCommand implement Command interface
- ✅ Command merging, memory tracking, and descriptions complete
- ✅ UI keyboard shortcuts and menu items functional
- ❌ **GizmoSystem directly modifies transforms without creating commands**

**Impact**: Users cannot undo/redo gizmo transform operations, which is a critical usability gap.

---

## Architecture Decision: Option 1 ✅ APPROVED

**Approach**: GizmoSystem creates and executes commands directly

### Rationale:
1. **Clean Separation**: GizmoSystem owns manipulation logic and command creation
2. **Minimal Surface Area**: Only adds CommandHistory* parameter to constructor
3. **Existing Infrastructure**: Leverages already-implemented command merging
4. **Single Responsibility**: Transform manipulation → command conversion in one place

### Design Pattern:
```
User Drags Gizmo
    ↓
beginManipulation() → Capture "before" transform state
    ↓
applyTransformDelta() → Direct updates for real-time feedback (unchanged)
    ↓
endManipulation() → Create TransformEntityCommand, execute via CommandHistory
    ↓
Command in History → User can undo/redo
```

---

## Implementation Strategy (TDD Red-Green-Refactor)

### Atomic Functionalities (AF3.6.x):

1. **AF3.6.1**: Add CommandHistory parameter to GizmoSystem constructor
2. **AF3.6.2**: Pass CommandHistory to GizmoSystem in UI initialization  
3. **AF3.6.3**: Capture before-state in beginManipulation()
4. **AF3.6.4**: Create commands in endManipulation()
5. **AF3.6.5**: Write unit tests for single entity manipulation
6. **AF3.6.6**: Write unit tests for multi-entity manipulation
7. **AF3.6.7**: Write tests for command merging during continuous manipulation
8. **AF3.6.8**: Write null-safety tests (CommandHistory = nullptr)
9. **AF3.6.9**: Integration test for full workflow
10. **AF3.6.10**: Update documentation and progress files

---

## Detailed Implementation Steps

### **AF3.6.1: Add CommandHistory Parameter to GizmoSystem**

**Files**: `src/editor/gizmos.h`, `src/editor/gizmos.cpp`

#### Changes to `gizmos.h`:

```cpp
// Forward declaration (add near top)
class CommandHistory;

class GizmoSystem
{
public:
    // Update constructor signature
    GizmoSystem(SelectionManager& selectionManager, 
                ecs::Scene& scene, 
                systems::SystemManager& systemManager,
                CommandHistory* commandHistory = nullptr) noexcept;
    
private:
    // Add member variable
    CommandHistory* m_commandHistory = nullptr;
    
    // Add snapshot structure for before-state tracking
    struct TransformSnapshot {
        ecs::Entity entity;
        components::Transform beforeTransform;
    };
    std::vector<TransformSnapshot> m_manipulationSnapshots;
};
```

#### Changes to `gizmos.cpp`:

```cpp
#include "editor/commands/CommandHistory.h"
#include "editor/transform_commands.h"

GizmoSystem::GizmoSystem(SelectionManager& selectionManager, 
                         ecs::Scene& scene, 
                         systems::SystemManager& systemManager,
                         CommandHistory* commandHistory) noexcept
    : m_selectionManager(&selectionManager)
    , m_scene(&scene)
    , m_systemManager(&systemManager)
    , m_commandHistory(commandHistory)  // ADDED
{
}
```

**TDD Order**:
1. **Red**: Write test verifying GizmoSystem can be constructed with CommandHistory
2. **Green**: Add parameter and member variable
3. **Refactor**: Ensure existing tests still pass

---

### **AF3.6.2: Pass CommandHistory in UI Initialization**

**Files**: `src/editor/ui.cpp`

#### Change in `UI::initializeSceneOperations()`:

```cpp
void UI::initializeSceneOperations(ecs::Scene& scene,
    systems::SystemManager& systemManager,
    assets::AssetManager& assetManager,
    engine::GPUResourceManager& gpuManager,
    editor::SelectionManager& selectionManager)
{
    m_impl->scene = &scene;
    m_impl->systemManager = &systemManager;
    m_impl->assetManager = &assetManager;
    m_impl->gpuManager = &gpuManager;

    // Create gizmo system with command history
    m_impl->gizmoSystem = std::make_unique<GizmoSystem>(
        selectionManager, 
        scene, 
        systemManager,
        m_impl->commandHistory.get()  // ADD THIS PARAMETER
    );
    
    // ... rest unchanged ...
}
```

**TDD Order**:
1. **Red**: Write test verifying UI passes CommandHistory to GizmoSystem
2. **Green**: Add parameter to constructor call
3. **Refactor**: Verify no regressions

---

### **AF3.6.3: Capture Before-State in beginManipulation()**

**Files**: `src/editor/gizmos.cpp`

#### Modify `GizmoSystem::beginManipulation()`:

```cpp
void GizmoSystem::beginManipulation() noexcept
{
    m_isManipulating = true;
    m_wasManipulated = false;

    // Clear previous snapshots
    m_manipulationSnapshots.clear();
    m_originalEntityScales.clear();

    // Capture BEFORE state for all selected entities
    if (m_selectionManager && m_scene)
    {
        const auto& selectedEntities = m_selectionManager->getSelectedEntities();
        for (const auto entity : selectedEntities)
        {
            if (m_scene->hasComponent<components::Transform>(entity))
            {
                const auto* transform = m_scene->getComponent<components::Transform>(entity);
                
                // Store before-state snapshot
                m_manipulationSnapshots.push_back({entity, *transform});
                
                // Store original scale (existing code)
                m_originalEntityScales[entity] = transform->scale;
            }
        }

        // Store original gizmo scale (existing code)
        if (!selectedEntities.empty())
        {
            auto gizmoMatrix = calculateGizmoMatrix();
            const auto gizmoMatrixTransposed = gizmoMatrix.transpose();
            math::Vec3f dummyTranslation, dummyRotation;
            ImGuizmo::DecomposeMatrixToComponents(
                gizmoMatrixTransposed.data(),
                dummyTranslation.data(),
                dummyRotation.data(),
                m_originalGizmoScale.data());
        }
    }
}
```

**TDD Order**:
1. **Red**: Write test verifying snapshots are captured on beginManipulation
2. **Green**: Add snapshot capture code
3. **Refactor**: Ensure existing manipulation tests pass

---

### **AF3.6.4: Create Commands in endManipulation()**

**Files**: `src/editor/gizmos.cpp`

#### Modify `GizmoSystem::endManipulation()`:

```cpp
void GizmoSystem::endManipulation() noexcept
{
    m_isManipulating = false;
    m_wasManipulated = true;

    // Create transform command(s) if CommandHistory is available and we have snapshots
    if (m_commandHistory && m_scene && !m_manipulationSnapshots.empty())
    {
        if (m_manipulationSnapshots.size() == 1)
        {
            // Single entity - create TransformEntityCommand
            const auto& snapshot = m_manipulationSnapshots[0];
            const auto* afterTransform = m_scene->getComponent<components::Transform>(snapshot.entity);
            
            if (afterTransform)
            {
                auto command = std::make_unique<editor::TransformEntityCommand>(
                    snapshot.entity,
                    *m_scene,
                    snapshot.beforeTransform,  // before
                    *afterTransform            // after
                );
                m_commandHistory->executeCommand(std::move(command));
            }
        }
        else if (m_manipulationSnapshots.size() > 1)
        {
            // Multiple entities - create BatchTransformCommand
            std::vector<ecs::Entity> entities;
            entities.reserve(m_manipulationSnapshots.size());
            for (const auto& snapshot : m_manipulationSnapshots)
            {
                entities.push_back(snapshot.entity);
            }
            
            auto batchCommand = std::make_unique<editor::BatchTransformCommand>(
                entities,
                *m_scene
            );
            
            // Add each transform with before/after states
            for (const auto& snapshot : m_manipulationSnapshots)
            {
                const auto* afterTransform = m_scene->getComponent<components::Transform>(snapshot.entity);
                if (afterTransform)
                {
                    batchCommand->addTransform(
                        snapshot.entity,
                        snapshot.beforeTransform,
                        *afterTransform
                    );
                }
            }
            
            m_commandHistory->executeCommand(std::move(batchCommand));
        }
    }

    // Clear snapshots and original scales
    m_manipulationSnapshots.clear();
    m_originalEntityScales.clear();
    m_originalGizmoScale = math::Vec3<>{1.0f, 1.0f, 1.0f};
}
```

**TDD Order**:
1. **Red**: Write test verifying TransformEntityCommand created for single entity
2. **Green**: Implement single entity command creation
3. **Red**: Write test verifying BatchTransformCommand created for multiple entities
4. **Green**: Implement batch command creation
5. **Refactor**: Extract common patterns, ensure clean code

---

### **AF3.6.5-3.6.9: Comprehensive Testing**

**New File**: `tests/gizmo_command_integration_tests.cpp`

#### Test Structure:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "editor/gizmos.h"
#include "editor/commands/CommandHistory.h"
#include "editor/transform_commands.h"
#include "editor/selection.h"
#include "runtime/ecs.h"
#include "runtime/systems.h"

TEST_CASE("GizmoSystem creates commands for manipulations", "[gizmo-commands][AF3.6]")
{
    SECTION("Single entity manipulation creates TransformEntityCommand")
    {
        // Setup: scene, selection, gizmo with CommandHistory
        // Arrange: entity with transform, select it
        // Act: beginManipulation → applyDelta → endManipulation
        // Assert: CommandHistory has 1 command (TransformEntityCommand)
        // Assert: Undo restores original position
        // Assert: Redo reapplies new position
    }
    
    SECTION("Multiple entity manipulation creates BatchTransformCommand")
    {
        // Setup: 3 entities selected
        // Act: manipulate
        // Assert: CommandHistory has 1 BatchTransformCommand
        // Assert: Undo restores all 3 entities
    }
    
    SECTION("Continuous manipulation with merging")
    {
        // Act: Begin → delta → delta → delta → end (simulate drag)
        // Assert: Only 1 merged command in history
        // Assert: Undo restores to original state (not intermediate)
    }
    
    SECTION("Null CommandHistory safety")
    {
        // Setup: GizmoSystem with nullptr CommandHistory
        // Act: Manipulate
        // Assert: No crashes, transforms still update
        // Assert: No commands created
    }
}

TEST_CASE("Gizmo manipulation integration workflow", "[integration][gizmo-commands][AF3.6.9]")
{
    SECTION("Full workflow: Create → Manipulate → Undo → Redo")
    {
        // Arrange: Create entity, add transform, select
        // Act: Use gizmo to move entity (5, 0, 0)
        // Assert: Entity moved
        // Act: Undo
        // Assert: Entity back at origin
        // Act: Redo
        // Assert: Entity at (5, 0, 0) again
        // Act: Manipulate again (rotation)
        // Assert: 2 commands in history
        // Act: Undo twice
        // Assert: Entity at origin with default rotation
    }
}
```

**TDD Order (Strict Red-Green-Refactor for each)**:
1. Write failing test for AF3.6.5 (single entity)
2. Implement until test passes
3. Write failing test for AF3.6.6 (multi entity)
4. Implement until test passes
5. Write failing test for AF3.6.7 (merging)
6. Verify merging works (should already be functional)
7. Write failing test for AF3.6.8 (null safety)
8. Implement null checks
9. Write failing test for AF3.6.9 (integration)
10. Verify end-to-end workflow

---

## Command Merging Considerations

**Existing Infrastructure** (already implemented):
- `CommandHistory::executeCommand()` checks `canMergeWith()`
- `TransformEntityCommand::canMergeWith()` returns true for same entity
- `TransformEntityCommand::mergeWith()` updates `m_afterTransform`

**Expected Behavior**:
- User drags gizmo smoothly → many applyDelta calls
- Each endManipulation creates a new command
- Commands arriving within merge window (100ms default) automatically merge
- Result: One merged command for entire drag operation

**Configuration**:
Command merging is enabled by default in CommandHistory. No additional configuration needed.

**Testing Scenario**:
```cpp
// Simulate dragging in 3 steps with small time gaps
beginManipulation();
applyDelta({1, 0, 0});
endManipulation();

// Small delay (< 100ms)
beginManipulation();
applyDelta({1, 0, 0});
endManipulation();

// Small delay (< 100ms)
beginManipulation();
applyDelta({1, 0, 0});
endManipulation();

// Assert: CommandHistory has 1 merged command
// Assert: Undo moves entity from (3,0,0) back to (0,0,0)
```

---

## Edge Cases & Null Safety

### Scenarios to Handle:

1. **CommandHistory is nullptr**
   - ✅ GizmoSystem should work normally (manipulations apply)
   - ✅ No commands created
   - ✅ No crashes

2. **No selected entities**
   - ✅ beginManipulation creates empty snapshots
   - ✅ endManipulation does nothing (no command)

3. **Entity deleted during manipulation**
   - ⚠️ Current code may create invalid command
   - ✅ Solution: Check entity validity in endManipulation

4. **Transform component removed during manipulation**
   - ⚠️ Similar issue
   - ✅ Solution: Validate component exists before creating command

### Safety Checks in endManipulation():

```cpp
if (m_commandHistory && m_scene && !m_manipulationSnapshots.empty())
{
    for (const auto& snapshot : m_manipulationSnapshots)
    {
        // Validate entity still exists and has transform
        if (!m_scene->isEntityValid(snapshot.entity) ||
            !m_scene->hasComponent<components::Transform>(snapshot.entity))
        {
            // Skip this entity (or clear all snapshots and return?)
            continue;
        }
        // ... create command ...
    }
}
```

---

## Performance Considerations

### Memory Usage:
- **Per Manipulation**: Store N × sizeof(TransformSnapshot) where N = selected entities
- **TransformSnapshot Size**: ~80 bytes (Entity + Transform with Vec3f × 3)
- **Example**: 100 selected entities = 8KB temporary storage
- **Mitigation**: Snapshots cleared after command creation

### Command History Memory:
- Default limit: 100 commands, 100MB
- TransformEntityCommand: ~200 bytes each
- 100 commands = 20KB (negligible)

### Large Selection Scenario:
- **1000 entities selected**:
  - Snapshot storage: 80KB (temporary)
  - BatchTransformCommand: ~200KB (persistent until cleanup)
  - Still well within 100MB limit

**Conclusion**: No performance concerns for typical usage.

---

## Integration Points

### Files Modified:
1. `src/editor/gizmos.h` - Constructor signature, member variables
2. `src/editor/gizmos.cpp` - Constructor, beginManipulation, endManipulation
3. `src/editor/ui.cpp` - Pass CommandHistory to GizmoSystem

### New Files:
1. `tests/gizmo_command_integration_tests.cpp` - Comprehensive test suite

### Files Updated:
1. `M2_P5.md` - Mark AF3.6 complete
2. `PROGRESS_2.md` - Add dated completion entry

---

## Acceptance Criteria (Task 3 AF3.6)

- [ ] GizmoSystem accepts CommandHistory parameter
- [ ] beginManipulation() captures before-state for all selected entities
- [ ] endManipulation() creates TransformEntityCommand (single) or BatchTransformCommand (multi)
- [ ] Commands are executed via CommandHistory
- [ ] Undo/redo works correctly for gizmo manipulations
- [ ] Command merging works for continuous manipulations
- [ ] Null CommandHistory handled safely (no crashes)
- [ ] Unit tests cover all scenarios (8+ test cases)
- [ ] Integration test validates full workflow
- [ ] Existing tests continue to pass
- [ ] Documentation updated

---

## Testing Checklist

### Unit Tests (gizmo_command_integration_tests.cpp):
- [ ] Test: Single entity → TransformEntityCommand created
- [ ] Test: Single entity → Undo restores original position
- [ ] Test: Single entity → Redo reapplies new position
- [ ] Test: Multiple entities → BatchTransformCommand created
- [ ] Test: Multiple entities → Undo restores all
- [ ] Test: Continuous manipulation → Commands merge
- [ ] Test: Null CommandHistory → No crashes
- [ ] Test: Empty selection → No commands created

### Integration Tests (command_integration_tests.cpp):
- [ ] Test: Create entity → Transform via gizmo → Undo → Redo
- [ ] Test: Multiple operations → Undo all → Redo all
- [ ] Test: Gizmo + ECS commands mixed → Undo order correct

### Regression Tests:
- [ ] All existing gizmo tests pass
- [ ] All existing command tests pass
- [ ] All existing integration tests pass

---

## Commit Message Template

```
gizmo: integrate CommandHistory for undoable transformations (M2-P5-T3-AF3.6)

Complete Task 3 AF3.6 by connecting GizmoSystem to CommandHistory,
enabling undo/redo for all gizmo manipulation operations.

Implementation:
- Add CommandHistory parameter to GizmoSystem constructor
- Capture before-state in beginManipulation()
- Create TransformEntityCommand/BatchTransformCommand in endManipulation()
- Execute commands via CommandHistory for automatic undo/redo support

Testing:
- Unit tests: Single/multi entity manipulation, merging, null safety
- Integration tests: Full workflow validation with undo/redo cycles
- 10+ test cases with [gizmo-commands][AF3.6] tags

Architecture:
- GizmoSystem owns command creation for clean separation
- Leverages existing command merging infrastructure
- Null-safe: Works without CommandHistory (no undo, no crashes)

Result:
- Users can now undo/redo all gizmo transform operations
- Command merging provides smooth UX for dragging operations
- Completes Phase 5 Task 3 integration requirements

Refs: M2-P5-T3-AF3.6
Files: gizmos.h, gizmos.cpp, ui.cpp, gizmo_command_integration_tests.cpp
```

---

## Next Steps

### 1. **Approval Checkpoint** ✅ COMPLETED
   - [x] Review plan
   - [x] Confirm Option 1 approach
   - [x] Approve command merging default-on
   - [x] Confirm no performance concerns

### 2. **Implementation** (Ready to Begin)
   - [ ] Follow TDD Red-Green-Refactor strictly
   - [ ] Implement AF3.6.1 through AF3.6.10 sequentially
   - [ ] Run tests after each atomic functionality
   - [ ] Refactor for clean code before moving to next AF

### 3. **Validation**
   - [ ] Run full test suite: `unit_test_runner.exe`
   - [ ] Run gizmo tests: `unit_test_runner.exe "*gizmo*"`
   - [ ] Run command tests: `unit_test_runner.exe "*command*"`
   - [ ] Manual testing in editor UI

### 4. **Documentation**
   - [ ] Update M2_P5.md with completion status
   - [ ] Update PROGRESS_2.md with dated entry
   - [ ] Generate commit message

### 5. **Review**
   - [ ] Code review
   - [ ] Test coverage review
   - [ ] Performance validation

---

## Estimated Timeline

- **AF3.6.1-3.6.2**: 1 hour (constructor changes, plumbing)
- **AF3.6.3**: 1 hour (capture before-state)
- **AF3.6.4**: 2 hours (command creation logic)
- **AF3.6.5-3.6.8**: 3 hours (unit tests)
- **AF3.6.9**: 1 hour (integration test)
- **AF3.6.10**: 1 hour (documentation)
- **Buffer**: 1 hour (debugging, refinement)

**Total**: 10 hours (~1.5 days)

---

## Risk Mitigation

### Known Risks:
1. **Entity lifecycle issues** (entity deleted during manipulation)
   - Mitigation: Validate entity/component in endManipulation
   
2. **Command merging timing**
   - Mitigation: Already tested in existing infrastructure
   
3. **Breaking existing tests**
   - Mitigation: Run tests after each AF, fix immediately

### Rollback Plan:
- All changes isolated to GizmoSystem and one line in UI
- Easy to revert if issues arise
- Null CommandHistory ensures backward compatibility

---

**Status**: ✅ Ready for Implementation  
**Approval**: ✅ Confirmed by user (2025-09-30)  
**Next Action**: Begin AF3.6.1 (Red phase: Write failing test)

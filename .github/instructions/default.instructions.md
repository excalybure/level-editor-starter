---
applyTo: '**'
---
---
mode: agent
---
# How LLMs should perform coding tasks

> **TL;DR**: Use strict **TDD (Red → Green → Refactor)** per **atomic functionality**. Build & run tests from the CLI. Verify a failing test first, then make the **smallest** change to pass, repeat until the whole task is complete. When finished, update the **milestone progress file** and provide a concise **commit message**.

---

## 0) Scope & environment
- Language: **C++23** (use modern features: modules, concepts, `std::expected`, ranges, `constexpr`/`consteval`, `std::string_view`, `std::span`, etc.).
- Unit tests: **Catch2** (already scaffolded).
- Test runner (Windows):
  ```cmd
  D:\cod_users\setienne\level-editor-starter\build\vs2022-x64\Debug\unit_test_runner.exe
  ```
  - List all/matching tests (supports wildcards/regex):
    ```cmd
    unit_test_runner.exe --list-tests "*gltf*"
    ```
  - Run specific tests: pass one or more names/patterns. See built‑in help (included at end of this doc) for flags like `--reporter`, `--durations`, etc.
- Build system: **CMake**. Re‑configure if any `CMakeLists.txt` changes:
  ```cmd
  cmake --preset vs2022-x64
  ```
- Milestone progress files: **`PROGRESS_<N>.md`** (e.g., milestone 2 ⇒ `PROGRESS_2.md`).

---

## 1) Golden rules
1. **One atomic functionality at a time.** If it can be split, split it. Finish TDD for each atom before moving to the next.
2. **Red, Green, Refactor** strictly:
   - **Red**: write one failing unit test for the next atom; build & run; confirm it fails for the expected reason.
   - **Green**: implement the **smallest** code change that makes that test pass; build & run just that test.
   - **Refactor**: improve names, structure, duplication, and const‑correctness with all tests green. No behavior changes.
3. Prefer **small diffs** and **narrow scope**. Do not edit unrelated files.
4. **Const correctness**:
   - Functions and methods that do not mutate observable state **must be `const`**.
   - **Local variables** are `const` if not mutated; prefer `constexpr` where viable.
   - Prefer **value/`string_view`/`span`** over raw pointers for ownership clarity.
5. **C++23 features** are encouraged when they simplify or clarify code (concepts, ranges, modules). Use them pragmatically; don’t over‑engineer.
6. **Logging noise**: test output may include `[INFO]`, `[WARNING]`, `[ERROR]/[ERRPR]` messages that are expected; **judge success solely by Catch2’s test results**.
7. **Circular dependencies**: when facing circular dependencies due to inline function, when possible (i.e non templated), move inline function in cpp and import or include correct module or header
8. **Linter errors**: They are to be ignored as they are due to not properly supporting c++23 modules
9. **Undefined symbols**: When using functionality from another module/library, make sure CMakelists.txt properly reference that module/library in target_link_libraries
10. **do not use throw**: Instead, prefer console::error to log an error and return an empty object, if applicable. When the error is non recoverable, use console::fatal. Note that runtime.console module may have to be specified to target_link_libraries
11. **const**: local read-only variables must be const

---

## 2) Task decomposition checklist
For every **task** (often spanning multiple functions and files):

1) Read the task description and identify **atomic functionalities** (AF1, AF2, ...). Each AF should be implementable with one small test.
2) For each AF, follow the TDD loop below. Keep a running list of completed AFs.
3) When all AFs are done, run the **full test suite**, update `PROGRESS_<N>.md`, and generate a **commit message**.

---

## 3) TDD loop (per atomic functionality)
**Inputs**: AF name, affected files, quick acceptance notes.

1. **Write a focused test** (Catch2):
   - Place tests in the appropriate test TU. Use descriptive names and tags.
   - Example skeleton:
     ```cpp
     #include <catch2/catch_test_macros.hpp>

     TEST_CASE("<AF short name> computes expected result", "[AF][unit]") {
         // Arrange
         // ...
         // Act
         // ...
         // Assert
         REQUIRE(/* condition */);
     }
     ```
   - Prefer `REQUIRE` for single critical assertions; use `CHECK` for multiple, independent checks.

2. **Build & run the new test**, and confirm it **fails**:
   ```cmd
   unit_test_runner.exe --list-tests "*<AF short name>*"
   unit_test_runner.exe "*<AF short name>*"
   ```
   - If it passes immediately, **tighten** the test (make it actually assert the missing behavior).

3. **Make the smallest code change** to pass the test:
   - Modify only the files/functions necessary.
   - Maintain **const correctness**, exception‑safety, RAII.
   - If a module/header boundary exists, add the minimal interface surface.

4. **Rebuild & run just that test** until green:
   ```cmd
   unit_test_runner.exe "*<AF short name>*"
   ```

5. **Refactor** (with all tests green):
   - Remove duplication, improve names, enforce invariants, apply concepts/constraints, tidy includes/module imports.
   - Keep behavior identical. Re‑run the AF test and suitable neighbors.

6. **Add more tests** for corner cases until AF feels fully specified.

> Repeat steps 1–6 for the next AF. Keep diffs small; commit after one or a few AFs if appropriate.

---

## 4) Building & running tests — practical commands
> Use quotes for patterns on Windows.

- **List tests**:
  ```cmd
  unit_test_runner.exe --list-tests
  unit_test_runner.exe --list-tests "*parser*"
  ```
- **Run specific test(s)**:
  ```cmd
  unit_test_runner.exe "*AF short name*"
  unit_test_runner.exe "[unit]"
  unit_test_runner.exe "[M2][phase-geometry]"
  ```
- **Useful flags**: `--durations yes`, `-a/--abort`, `-x/--abortx 1`, `--order rand`, reporters.
- **Reconfigure CMake if needed**:
  ```cmd
  cmake --preset vs2022-x64
  ```

---

## 5) Coding standards (C++23)
- **Modules**: Prefer adding **new** code as modules when feasible; keep module interfaces minimal. Example CMake pattern:
  ```cmake
  target_sources(my_target PRIVATE
    FILE_SET CXX_MODULES FILES
      src/math/math.ixx
      src/math/detail/add_impl.ixx
  )
  ```
- **Concepts**: Constrain templates for clarity (e.g., `std::integral`, `std::ranges::range`).
- **Ranges/algorithms**: Prefer `<ranges>` pipelines over raw loops when clearer.
- **Error handling**: Prefer `std::expected<T,E>` or status enums over exceptions for predictable flows; use exceptions for truly exceptional failures.
- **Ownership/borrowing**: `std::unique_ptr`/`std::shared_ptr` where needed; pass by value or by `span`/`string_view` to borrow.
- **No raw `new`/`delete`** in business logic.
- **Const‑correctness** everywhere (functions, methods, locals). Mark methods `noexcept` if they can’t throw.
- **Formatting/naming**: Keep consistent with existing code; prefer expressive names and small functions.

---

## 6) Naming Conventions

This project follows **Modern C++ conventions** (avoiding snake_case) for consistency and readability:

### 🏷️ Types & Classes: **PascalCase**
```cpp
class ViewportLayout {};
struct Vec3 {};
enum class MouseButton { Left, Right, Middle };
template<typename T> class BoundingBox {};
```

### 🔧 Functions & Methods: **camelCase**
```cpp
void beginFrame();           // Lifecycle functions
void endFrame();
bool initialize();
void shutdown();
ID3D12Device* getDevice();   // Getters/accessors
float lengthSquared();       // Math operations
void setupDockspace();       // Internal functions
```

### 📦 Variables: **camelCase**
```cpp
float clearColor = 0.0f;
int windowWidth = 800;
auto lengthSquared = dot(v, v);
Vec3 cameraPosition{0, 0, 10};
```

### 👥 Member Variables: **m_** prefix + **camelCase**
```cpp
class UI {
    ViewportLayout m_layout;
    std::unique_ptr<Impl> m_impl;
    ComPtr<ID3D12Device> m_device;
};
```

### 🔢 Constants: **kPascalCase** (modern style)
```cpp
static constexpr float kPi = 3.14159f;
static constexpr int kMaxEntities = 1000;
static constexpr Vec3 kWorldUp{0, 0, 1};  // Z-up convention
```

### 🗂️ Namespaces: **lowercase**
```cpp
namespace math {}    // Core math operations
namespace ecs {}     // Entity-component system  
namespace dx12 {}    // DirectX 12 abstractions
namespace editor {}  // Editor-specific code
```

### 📁 Files & Modules: **lowercase + underscores**
```cpp
// Module names
export module engine.math;
export module runtime.ecs;
export module platform.win32.window;

// File names
math_tests.cpp
win32_window.ixx
dx12_device.cpp
```

**Rationale:** This convention aligns with Unreal Engine, modern DirectX APIs, and contemporary C++ codebases while avoiding snake_case as requested.

---

## 7) Progress updates — `PROGRESS_<N>.md`
After the task (all AFs) is complete:

Append a dated entry at the top of the current milestone’s progress file (`PROGRESS_<N>.md`). Use the milestone number (e.g., `2` → `PROGRESS_2.md`).
Update the file name `M<X>_P<Y>.md` where <X> refers to the milestone number and <Y> refers to the phase with the completed tasks

**Template**:
```markdown
## YYYY‑MM‑DD — <Task title>
**Summary:** one‑paragraph outcome.
**Atomic functionalities completed:**
- AF1: <one line>
- AF2: <one line>
- ...
**Tests:** <N> new/updated; filtered commands used.
**Notes:** decisions, trade‑offs, follow‑ups.
```

> If the milestone number is not provided in the prompt, infer it from the working folder (e.g., `docs/milestones/M2-...`) or ask once.

---

## 8) Commit message
Generate a concise message that explains **what changed and why** without verbosity. Aim for ~72‑char subject; short bullet body. Reference milestone/phase/task IDs if available. Do not commit the change as it will have to be reviewed and approved beforehand, buit write the commit message in the chat window

**Template**:
```
<area>: implement <feature> via TDD; add tests

- Add tests: <files/tags>
- Implement: <key functions/modules>
- Refactor: <high‑level, if any>
- Result: <behavior now guaranteed>

Refs: M<N>-P<X>-T<Y>
```

**Example**:
```
geometry: add triangle normal calc (TDD) and tests

- Tests: [geometry][unit] triangle normal cases incl degenerate
- Impl: add module geometry.normals; use concepts + ranges
- Refactor: clarify vector types; mark funcs const/noexcept
- Result: callers get robust normal or expected<error>

Refs: M2-P1-T03
```

> If the change spans multiple atoms, keep the subject generic and list atoms in bullets.

---

## 9) Guardrails for LLMs
- **Stay in scope.** Only touch files relevant to the current AF or explicit refactors.
- **Small steps.** Prefer many tiny edits over sweeping rewrites.
- **Re-run tests frequently.** After each Green/Refactor step.
- **Explain assumptions** in comments or the progress note when specs are ambiguous.
- **Do not silence failing tests** (no `#ifdef`, no skipping without justification).

---

## 10) Catch2 quick reference (subset)
- Create test: `TEST_CASE("name", "[tags]")`.
- Assertions: `REQUIRE(expr)`, `CHECK(expr)`, `REQUIRE_THROWS`, `REQUIRE_THROWS_AS`, etc.
- Sections: `SECTION("variant A") { ... }` to share setup across variants.
- Tags help filtering: `[unit]`, `[integration]`, `[M2]`, `[phase-xyz]`.
- CLI examples:
  ```cmd
  unit_test_runner.exe --list-tests
  unit_test_runner.exe "[unit]"
  unit_test_runner.exe "*parser*"
  unit_test_runner.exe --durations yes "*gltf*"
  unit_test_runner.exe -a "*critical*"
  ```

---

## 11) Built-in runner help (for convenience)
The runner supports flags such as `--list-tests`, `--list-tags`, `--reporter`, `--durations`, `--abort`, `--abortx`, `--order`, `--rng-seed`, `--colour-mode`, and sharding options. Use these to target and speed up local test runs. Ignore incidental `[INFO]/[WARNING]/[ERROR]` lines unless the test itself fails.

---
## 12) Base 64 encoding
If you need to produce a base64 encoded string, generate a python script that will do the encoding for you. The script should look like this:
```
python -c "
import struct
import base64

# Positions: (0,0,0), (1,0,0), (0,1,0)
positions = struct.pack('<9f', 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0)
# UVs: (0,0), (1,0), (0.5,1)
uvs = struct.pack('<6f', 0.0, 0.0, 1.0, 0.0, 0.5, 1.0)

# Combine positions and UVs
combined = positions + uvs
print('data:application/octet-stream;base64,' + base64.b64encode(combined).decode())
print('Length:', len(combined))
"
```

When you run this script, it will output a base64 encoded string that you can use in your code. For example:
```
data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAD8AAIA/
Length: 60
```

### End
Follow this document mechanically. For each atomic functionality: **Red → Green → Refactor**. Keep diffs small, tests focused, and code idiomatic C++23 with const correctness. After finishing the task, **update `PROGRESS_<N>.md`** and **update `M<X>_P<Y>.md`** and **emit a commit message** using the template above.

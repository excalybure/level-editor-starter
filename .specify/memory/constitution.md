# Level Editor Starter Constitution

This constitution codifies the non‑negotiable engineering and workflow principles for the Level Editor Starter project. It is the single authoritative reference for expectations on code, tests, architecture, and process. All contributors are responsible for knowing and applying it.

## I. Core Non‑Negotiable Principles ("Hard Stops")
The following items are absolute. A change that violates any of them must be revised before merge.

1. Test‑Driven Development (TDD) Only
   - Every new atomic functionality begins with a failing Catch2 test (Red → Green → Refactor).
   - A test that passes immediately is tightened before implementation proceeds.
   - No production code without at least one covering test (except explicitly documented UI rendering code that cannot be unit tested).

2. Atomic Changes & Small Diffs
   - Implement one narrowly defined behavior per iteration.
   - Keep diffs reviewable; avoid opportunistic large refactors bundled with new behavior.
   - Refactors only occur with all tests green.

3. Const Correctness & Immutability
   - Every local that is not mutated is declared const (or constexpr when viable).
   - Functions/methods that do not mutate observable state are marked const; non‑throwing pure functions prefer noexcept.

4. Modern C++23
   - Prefer C++23 language/library features (ranges, concepts, std::expected) for clarity and safety.
   - New logical groupings ship as small public headers with minimal exported surface; avoid gratuitous headers.

5. No Exceptions for Flow Control
   - Do not use throw for routine error paths; use logging (console::error / console::fatal) plus empty/expected returns.
   - Irrecoverable failures escalate via console::fatal.

6. Explicit Ownership / No Raw new & delete
   - Use smart pointers or value semantics. Raw owning pointers are prohibited in new code.

7. Clear Naming & Style Consistency
   - Follow established conventions: PascalCase types, camelCase functions/variables, m_ prefix for members, kPascalCase constants, lowercase namespaces and underscored file names.

8. Deterministic Error Handling
   - Prefer std::expected<T,E> or status enums for recoverable operations.
   - Silent failure is forbidden; every failure path must either propagate structured status or log.

9. UI Code Test Separation
   - Rendering / ImGui calls are not unit tested directly.
   - Extract and test pure logic/state transformation separately.
   - Document (in progress notes) when behavior cannot be unit tested and why.

10. Progress & Traceability
	- After completing a task’s atomic functionalities, update the current milestone progress file (PROGRESS_<N>.md) with date, summary, and test references.
	- Tests must be filterable with meaningful tags (e.g., [unit], [integration], [M2]).

11. Minimal Public Surface
	- Make public only what current use cases require

12. Logging Discipline
	- Avoid noisy info logs in hot paths; logging should aid debugging, not obscure test output.

13. Build & Test Integrity
	- All tests must pass locally before merge. A failing or flaky test blocks integration.
	- Never “comment out” or disable failing tests to obtain green status.

14. Deterministic Reproducibility
	- Build configuration (CMake presets, vcpkg manifest) is the source of truth; do not rely on undocumented local flags.

15. Intentional Refactoring
	- Refactors have zero behavioral change and retain full test coverage; broaden tests first if coverage is thin.

16. Explicit Assumptions
	- When spec gaps exist, implement with 1–2 minimal, documented assumptions recorded in the progress note.

17. No Hidden Global State
	- Shared state must be explicit (passed, injected, or owned). Avoid implicit singletons unless already standardized and documented.

18. Performance by Measurement
	- Optimize only after identifying a measurable issue; add a micro/integration test or benchmark reference where feasible.

19. Code Review Gate
	- PRs must articulate: (a) atomic functionalities covered, (b) tests added/changed, (c) rationale for any refactor.

20. Security & Safety Baseline
	- Validate external inputs (asset files, scene data) before use. Reject or sanitize invalid data rather than proceeding in a corrupted state.

## II. Supporting Engineering Practices
These reinforce the non‑negotiables and guide day‑to‑day execution.

- Red → Green → Refactor loop is visible in commit history (separate logical commits encouraged).
- Favor pure functions; push side effects to edges (I/O, rendering, logging, device interaction).
- Prefer compile‑time validation (constexpr, concepts) where it simplifies runtime logic.
- Keep public interfaces lean; move implementation details in cpp files.
- Tag slow or integration tests distinctly to allow focused fast iteration on unit suites.
- Provide defensive early exits with clear log messages instead of cascading undefined behavior.

## III. Development Workflow

1. Decompose task → enumerate atomic functionalities (AFs).
2. For each AF:
   - Write failing test (Red) → Confirm fail reason is expected.
   - Implement smallest passing change (Green).
   - Refactor locally (Refactor) → Re‑run affected tests.
3. After all AFs: run full test suite; update progress file with summary, list AFs, and test tally.
4. Prepare concise commit message (≤ ~72 char subject + bullet body) referencing milestone/phase/task.
5. Submit PR ensuring constitution checklist is satisfied.

## IV. Quality Gates

Failure of any gate halts merge:

- Compilation (all presets used in CI) passes.
- All tests green; no newly introduced flakiness.
- Static / style constraints: naming & const correctness applied.
- No raw ownership, no unintended exceptions, no disabled tests.
- Progress file updated; commit message template followed.

## V. Amendment Process

1. Proposal: Author drafts change rationale + impact analysis (backwards compatibility & migration considerations).
2. Review: Requires consensus from maintainers (≥2 approvals) and updated version number.
3. Implementation: Update this constitution + any referenced guidance docs; reflect changes in next milestone planning.
4. Ratification: Merge only after all blocking discussions resolved and test/process updates (if any) are in place.

Non‑negotiable sections (I) may only be altered with a major version increment and explicit migration notes.

## VI. Enforcement

- Reviewers enforce adherence; any exception must be explicitly documented in the PR description with justification & expiration (follow‑up task).
- Repeated violations trigger a remediation review of process gaps.
- Tooling (test tags, presets) should evolve to reduce manual enforcement burden.

## VII. Glossary (Selective)

- AF: Atomic Functionality – smallest independently testable behavior change.
- Red/Green/Refactor: Standard TDD cycle enforcing discipline & minimal diffs.
- Progress File: PROGRESS_<N>.md documenting chronological advancement per milestone.

## Governance

This constitution supersedes ad‑hoc practices or undocumented conventions. Where conflict exists, this document wins. Contributors propose amendments through the process above. Ambiguities default to: keep it smaller, testable, explicit.

**Version**: 1.0.0 | **Ratified**: 2025-10-08 | **Last Amended**: 2025-10-08

---
Maintainers affirm that adherence to these principles is essential for velocity, reliability, and long‑term maintainability of the Level Editor Starter project.
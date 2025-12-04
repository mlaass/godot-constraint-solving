# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build for Linux (release)
scons platform=linux target=release

# Build for Linux (debug)
scons platform=linux target=debug

# Build for Windows (cross-compile from Linux)
scons platform=windows target=release

# Build for macOS
scons platform=macos target=release
```

The build requires godot-cpp to be available at `../godot-cpp` with pre-built binaries in `../godot-cpp/bin/`.

Output binaries are placed in `bin/`. The `wfc_solver_native.gdextension` file is in the project root.

## Architecture

This is a GDExtension implementing a Wave Function Collapse (WFC) solver in C++ for Godot 4.4+.

### Core Classes

- **WFCSolverNative**: Main solver class supporting AC3/AC4 constraint propagation and backtracking
- **WFCProblemNative**: Abstract base class defining the constraint satisfaction problem interface
- **WFC2DProblemNative**: Concrete 2D grid implementation of WFCProblemNative
- **WFCRules2DNative**: Resource storing adjacency rules as bit matrices for 2D problems
- **WFCSolverStateNative**: Tracks cell domains, solutions, entropy, and backtracking state
- **WFCSolverSettingsNative**: Configuration for the solver

### Data Structures

- **WFCBitSetNative**: Optimized bitset (2 inline int64s + overflow array) for tile domains
- **WFCBitMatrixNative**: 2D bit matrix for storing adjacency rules

### Constraint Propagation

The solver supports both AC3 and AC4 arc consistency algorithms:
- AC4 uses `WFCProblemAC4BinaryConstraintNative` for binary constraints
- `WFC2DAC4BinaryConstraintNative` implements AC4 constraints for 2D grids

### Extension Point

To create custom WFC problems, subclass `WFCProblemNative` and implement:
- `get_cell_count()` / `get_default_domain()`
- `compute_cell_domain()` for constraint checking
- `mark_related_cells()` for propagation
- `supports_ac4()` / `get_ac4_binary_constraints()` for AC4 support

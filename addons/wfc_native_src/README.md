# WFC Solver Native

A C++ GDExtension implementing a Wave Function Collapse (WFC) solver for Godot 4.4+.

## Prerequisites

- [godot-cpp](https://github.com/godotengine/godot-cpp) built and available at `../godot-cpp`
- SCons build system
- C++17 compatible compiler

## Building

```bash
# Linux
scons platform=linux target=release

# Windows (cross-compile from Linux)
scons platform=windows target=release

# macOS
scons platform=macos target=release
```

Output binaries are placed in the `bin/` directory.

## Installation

1. Build the extension for your target platform(s)
2. Copy `wfc_solver_native.gdextension` and the built library from `bin/` to your Godot project's `addons/wfc_native/` directory

## Usage

```gdscript
# Create rules for a 2D tilemap
var rules = WFCRules2DNative.new()
rules.initialize(tile_count, [Vector2i(1, 0), Vector2i(0, 1)])  # Right and down axes

# Set adjacency rules
rules.set_rule(0, tile_a, tile_b, true)  # tile_b can be to the right of tile_a
rules.complete_all_matrices()

# Create and solve a problem
var problem = WFC2DProblemNative.new()
problem.initialize(rules, Rect2i(0, 0, width, height))

var solver = WFCSolverNative.new()
solver.initialize(problem)
var result = solver.solve()

# Read solutions
for y in height:
    for x in width:
        var cell_id = problem.coord_to_id(Vector2i(x, y))
        var tile = result.get_cell_solution(cell_id)
```

## Features

- AC3 and AC4 arc consistency algorithms for constraint propagation
- Backtracking with configurable limits
- Optimized bitset operations for tile domains
- Extensible problem interface for custom WFC variants

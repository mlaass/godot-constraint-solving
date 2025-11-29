# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A Wave Function Collapse (WFC) and generic constraint satisfaction problem solver addon for Godot 4. Supports TileMapLayer, TileMap, and GridMap nodes with features including backtracking, multithreading, and learning rules from example maps.

## Development Commands

**Run tests with GUT (Godot Unit Test):**
```bash
godot --headless -s addons/gut/gut_cmdln.gd -gdir=res://test/unit
```

**Run project/examples:**
```bash
godot --path . res://addons/wfc/examples/demo_wfc_2d_layered_tilemap.tscn
```

## Architecture

### Core Solver (`addons/wfc/solver/`)
- `WFCSolver` - Generic constraint satisfaction solver using AC-3/AC-4 constraint propagation with optional backtracking. Despite the WFC prefix, it solves a broader class of problems where variables have finite discrete domains.
- `WFCSolverState` - Tracks cell domains, solutions, and entropy values. Supports state copying for backtracking.
- `WFCSolverSettings` - Configuration for backtracking limits, sparse history, AC3/AC4 selection.

### Problem Abstraction (`addons/wfc/problems/`)
- `WFCProblem` (base class) - Defines interface for constraint satisfaction problems: `get_cell_count()`, `get_default_domain()`, `compute_cell_domain()`, `mark_related_cells()`, `split()` for multithreading.
- `WFC2DProblem` - 2D WFC implementation. Converts grid coordinates to cell IDs, handles rule matrices for adjacency constraints, supports problem splitting along X/Y axis for parallel solving.
- `WFCRules2D` - Stores adjacency rules as bit matrices per axis direction. Can be learned from sample maps.

### Mappers (`addons/wfc/problems/2d/mappers/`)
Mappers abstract different map node types, converting between Godot nodes and numeric tile IDs:
- `WFCMapper2D` - Base class defining the mapper interface
- `WFCMapper2DTileMapLayer` - For Godot 4.3+ TileMapLayer nodes
- `WFCMapper2DTileMap` - For legacy TileMap nodes
- `WFCMapper2DGridMap` - For 3D GridMap nodes (generates 2D slices)
- `WFCMapper2DLayeredMap` - For multi-layer tilemap setups
- `WFC2DMapperFactory` - Creates appropriate mapper for a given node type

### Preconditions (`addons/wfc/problems/2d/preconditions/`)
Constrain initial cell domains before WFC runs:
- `WFC2DPrecondition` - Base class
- `WFC2DPreconditionReadExisting` - Reads existing tiles from target map
- `WFC2DPreconditionDungeon` - Generates dungeon-like room/corridor layouts
- `WFC2DPreconditionRemap` - Remaps tile types

### Runners (`addons/wfc/runners/`)
Execute solvers with different threading strategies:
- `WFCSolverRunner` - Base class with signals: `partial_solution`, `sub_problem_solved`, `all_solved`
- `WFCMainThreadSolverRunner` - Single-threaded, yields to main loop each frame
- `WFCMultithreadedSolverRunner` - Splits problem and solves sub-problems in parallel

### High-Level Node (`addons/wfc/nodes/`)
- `WFC2DGenerator` - Main user-facing node. Configures target/sample maps, rules, preconditions, runner settings. Orchestrates the full generation pipeline.

### Utilities (`addons/wfc/utils/`)
- `WFCBitSet` - Bit array for representing cell domains (allowed tile types)
- `WFCBitMatrix` - Matrix of bits for adjacency rules, supports transform and transpose operations

## Key Patterns

1. **Constraint Propagation**: When a cell's domain changes, `mark_related_cells()` identifies affected neighbors, then `compute_cell_domain()` recalculates their domains using adjacency matrices.

2. **Problem Splitting**: `WFC2DProblem.split()` divides the problem along X or Y axis. Even-indexed sub-problems run first with extended rects; odd-indexed sub-problems depend on neighbors and read their solutions from overlapping regions.

3. **Rule Learning**: Rules are inferred from positive sample maps by recording all observed adjacencies. Negative samples can exclude unwanted adjacencies. Optionally, rules can be derived from tileset terrain settings.

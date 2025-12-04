extends Node

const WFCBitSet = preload("res://addons/wfc/utils/bitset.gd")
const WFCBitMatrix = preload("res://addons/wfc/utils/bitmatrix.gd")
const WFCSolverSettings = preload("res://addons/wfc/solver/solver_settings.gd")
const WFCSolverState = preload("res://addons/wfc/solver/solver_state.gd")
const WFCSolver = preload("res://addons/wfc/solver/solver.gd")
const WFCProblem = preload("res://addons/wfc/problems/problem.gd")


# Simple GDScript problem implementation for testing (no mapper needed)
class TestProblem2D extends WFCProblem:
	var rules_axes: Array[Vector2i]
	var axis_matrices: Array[WFCBitMatrix]
	var rect: Rect2i
	var tile_count: int

	func _init(tile_count_: int, axes_: Array[Vector2i], rect_: Rect2i):
		tile_count = tile_count_
		rules_axes = []
		axis_matrices = []
		rect = rect_

		# Build axes and matrices (including reverse directions)
		for axis in axes_:
			rules_axes.append(axis)
			axis_matrices.append(WFCBitMatrix.new(tile_count, tile_count))
			rules_axes.append(-axis)
			axis_matrices.append(WFCBitMatrix.new(tile_count, tile_count))

	func set_rule(axis_index: int, from_tile: int, to_tile: int, allowed: bool):
		# Set forward rule
		axis_matrices[axis_index * 2].set_bit(from_tile, to_tile, allowed)
		# Set reverse rule (transposed)
		axis_matrices[axis_index * 2 + 1].set_bit(to_tile, from_tile, allowed)

	func get_cell_count() -> int:
		return rect.get_area()

	func get_default_domain() -> WFCBitSet:
		return WFCBitSet.new(tile_count, true)

	func coord_to_id(coord: Vector2i) -> int:
		return rect.size.x * coord.y + coord.x

	func id_to_coord(id: int) -> Vector2i:
		var szx := rect.size.x
		@warning_ignore("integer_division")
		return Vector2i(id % szx, id / szx)

	func compute_cell_domain(state: WFCSolverState, cell_id: int) -> WFCBitSet:
		var current_domain: WFCBitSet = state.cell_domains[cell_id]
		var res := current_domain.copy()
		var pos := id_to_coord(cell_id)

		for i in range(rules_axes.size()):
			var axis := rules_axes[i]
			var other_pos := pos + axis
			if not rect.has_point(other_pos + rect.position):
				continue
			var other_id := coord_to_id(other_pos)
			if state.cell_solution_or_entropy[other_id] == WFCSolverState.CELL_SOLUTION_FAILED:
				continue
			var other_domain: WFCBitSet = state.cell_domains[other_id]
			var matrix := axis_matrices[i]
			res.intersect_in_place(matrix.transform(other_domain))

		return res

	func mark_related_cells(changed_cell_id: int, mark_cell: Callable):
		var pos := id_to_coord(changed_cell_id)
		for axis in rules_axes:
			var other_pos := pos + axis
			if rect.has_point(other_pos + rect.position):
				mark_cell.call(coord_to_id(other_pos))


func _ready():
	print("=== Native WFC Solver Test ===")
	print("")

	# Check if native classes are available
	var classes_to_check = [
		"WFCBitSetNative",
		"WFCBitMatrixNative",
		"WFCSolverSettingsNative",
		"WFCSolverStateNative",
		"WFCProblemNative",
		"WFCSolverNative",
		"WFCRules2DNative",
		"WFC2DProblemNative"
	]

	print("Checking native classes:")
	var all_found = true
	for cname in classes_to_check:
		var exists = ClassDB.class_exists(cname)
		print("  ", cname, ": ", "FOUND" if exists else "NOT FOUND")
		if not exists:
			all_found = false

	print("")

	if not all_found:
		print("ERROR: Some native classes not found!")
		print("Make sure the GDExtension is properly loaded.")
		get_tree().quit(1)
		return

	print("All native classes found!")
	print("")

	# Run basic tests
	test_bitset()
	test_rules()
	test_problem()
	test_solver()

	# Run comparison tests
	print("=== Native vs GDScript Comparison ===")
	print("")

	compare_solvers(12345, Vector2i(10, 10), 4)
	compare_solvers(12345, Vector2i(20, 20), 4)
	compare_solvers(12345, Vector2i(50, 50), 4)

	print("=== All Tests Passed ===")
	get_tree().quit(0)


func test_bitset():
	print("Testing WFCBitSetNative:")
	var bs = WFCBitSetNative.new()
	bs.initialize(64, false)
	bs.set_bit(0, true)
	bs.set_bit(10, true)
	bs.set_bit(63, true)
	print("  Size: ", bs.get_size())
	print("  Bit 0: ", bs.get_bit(0))
	print("  Bit 10: ", bs.get_bit(10))
	print("  Bit 5: ", bs.get_bit(5))
	print("  Count: ", bs.count_set_bits())
	print("")


func test_rules():
	print("Testing WFCRules2DNative:")
	var rules = WFCRules2DNative.new()
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	rules.initialize(4, axes)
	rules.set_rule(0, 0, 1, true)
	rules.set_rule(0, 1, 0, true)
	rules.set_rule(1, 0, 0, true)
	rules.set_rule(1, 1, 1, true)
	print("  Tile count: ", rules.get_tile_count())
	print("  Rule (0, 0->1): ", rules.get_rule(0, 0, 1))
	print("  Rule (0, 2->3): ", rules.get_rule(0, 2, 3))
	print("")


func test_problem():
	print("Testing WFC2DProblemNative:")
	var rules = WFCRules2DNative.new()
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	rules.initialize(4, axes)
	var problem = WFC2DProblemNative.new()
	problem.initialize(rules, Rect2i(0, 0, 10, 10))
	print("  Cell count: ", problem.get_cell_count())
	print("  Default domain size: ", problem.get_default_domain().get_size())

	print("")


func test_solver():
	print("Testing WFCSolverNative:")
	var rules = WFCRules2DNative.new()
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	rules.initialize(4, axes)
	# Allow all tiles next to all tiles
	for i in range(4):
		for j in range(4):
			rules.set_rule(0, i, j, true)
			rules.set_rule(1, i, j, true)

	var problem = WFC2DProblemNative.new()
	problem.initialize(rules, Rect2i(0, 0, 10, 10))

	var settings = WFCSolverSettingsNative.new()
	var solver = WFCSolverNative.new()
	solver.initialize(problem, settings)
	print("  Solver initialized successfully")

	var start_time = Time.get_ticks_msec()
	var state = solver.solve()
	var end_time = Time.get_ticks_msec()

	print("  Solve time: ", end_time - start_time, " ms")
	print("  Unsolved cells: ", state.get_unsolved_cells())
	print("  Backtracking count: ", solver.get_backtracking_count())

	# Debug: show first 20 cell solutions
	var solutions = state.get_cell_solution_or_entropy()
	var first_20 = []
	for i in range(min(20, solutions.size())):
		first_20.append(solutions[i])
	print("  First 20 solutions: ", first_20)
	print("")


func compare_solvers(test_seed: int, grid_size: Vector2i, tile_count: int):
	print("Test: %dx%d, %d tiles, seed=%d" % [grid_size.x, grid_size.y, tile_count, test_seed])

	var rect = Rect2i(Vector2i.ZERO, grid_size)
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]

	# Create GDScript problem with rules
	var gd_problem = TestProblem2D.new(tile_count, axes, rect)
	# Allow all tiles next to all tiles for simplicity
	for i in range(tile_count):
		for j in range(tile_count):
			gd_problem.set_rule(0, i, j, true)
			gd_problem.set_rule(1, i, j, true)

	# Create Native problem with identical rules
	var native_rules = WFCRules2DNative.new()
	native_rules.initialize(tile_count, axes)
	for i in range(tile_count):
		for j in range(tile_count):
			native_rules.set_rule(0, i, j, true)
			native_rules.set_rule(1, i, j, true)

	var native_problem = WFC2DProblemNative.new()
	native_problem.initialize(native_rules, rect)

	# Run GDScript solver
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true  # Use AC3 for both to ensure same algorithm
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	var gd_start = Time.get_ticks_usec()
	var gd_state = gd_solver.solve()
	var gd_time = Time.get_ticks_usec() - gd_start

	# Run Native solver
	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)  # Use AC3 for both
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	var native_start = Time.get_ticks_usec()
	var native_state = native_solver.solve()
	var native_time = Time.get_ticks_usec() - native_start

	# Compare solutions
	var differences = compare_solutions(gd_state, native_state)

	# Report results
	print("  GDScript: %.1f ms" % (gd_time / 1000.0))
	print("  Native:   %.1f ms" % (native_time / 1000.0))
	if native_time > 0:
		print("  Speedup:  %.1fx" % (float(gd_time) / float(native_time)))

	if differences.is_empty():
		print("  Result:   IDENTICAL")
	else:
		print("  Result:   DIFFERENT (%d cells differ)" % differences.size())
		for diff in differences.slice(0, 5):
			print("    Cell %d: GDScript=%d, Native=%d" % [diff.cell, diff.gdscript, diff.native])
		if differences.size() > 5:
			print("    ... and %d more" % (differences.size() - 5))

	print("")


func compare_solutions(gd_state: WFCSolverState, native_state: WFCSolverStateNative) -> Array:
	var differences = []
	var gd_solutions = gd_state.cell_solution_or_entropy
	var native_solutions = native_state.get_cell_solution_or_entropy()

	if gd_solutions.size() != native_solutions.size():
		print("  WARNING: Solution sizes differ: GDScript=%d, Native=%d" % [gd_solutions.size(), native_solutions.size()])
		return [{"cell": -1, "gdscript": gd_solutions.size(), "native": native_solutions.size()}]

	for i in range(gd_solutions.size()):
		if gd_solutions[i] != native_solutions[i]:
			differences.append({
				"cell": i,
				"gdscript": gd_solutions[i],
				"native": native_solutions[i]
			})

	return differences

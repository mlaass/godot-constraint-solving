extends GutTest

const WFCBitSet = preload("res://addons/wfc/utils/bitset.gd")
const WFCBitMatrix = preload("res://addons/wfc/utils/bitmatrix.gd")
const WFCSolverSettings = preload("res://addons/wfc/solver/solver_settings.gd")
const WFCSolverState = preload("res://addons/wfc/solver/solver_state.gd")
const WFCSolver = preload("res://addons/wfc/solver/solver.gd")
const WFCProblem = preload("res://addons/wfc/problems/problem.gd")


# Simple GDScript problem implementation for testing
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

		for axis in axes_:
			rules_axes.append(axis)
			axis_matrices.append(WFCBitMatrix.new(tile_count, tile_count))
			rules_axes.append(-axis)
			axis_matrices.append(WFCBitMatrix.new(tile_count, tile_count))

	func set_rule(axis_index: int, from_tile: int, to_tile: int, allowed: bool):
		axis_matrices[axis_index * 2].set_bit(from_tile, to_tile, allowed)
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


func _check_native_classes_available() -> bool:
	var classes = ["WFCBitSetNative", "WFCSolverNative", "WFCRules2DNative", "WFC2DProblemNative"]
	for cname in classes:
		if not ClassDB.class_exists(cname):
			gut.p("Skipping test: " + cname + " not available")
			return false
	return true


func _create_gd_problem(tile_count: int, grid_size: Vector2i) -> TestProblem2D:
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	var rect = Rect2i(Vector2i.ZERO, grid_size)
	var problem = TestProblem2D.new(tile_count, axes, rect)
	# Allow all tiles next to all tiles
	for i in range(tile_count):
		for j in range(tile_count):
			problem.set_rule(0, i, j, true)
			problem.set_rule(1, i, j, true)
	return problem


func _create_native_problem(tile_count: int, grid_size: Vector2i):
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	var rect = Rect2i(Vector2i.ZERO, grid_size)

	var rules = WFCRules2DNative.new()
	rules.initialize(tile_count, axes)
	for i in range(tile_count):
		for j in range(tile_count):
			rules.set_rule(0, i, j, true)
			rules.set_rule(1, i, j, true)

	var problem = WFC2DProblemNative.new()
	problem.initialize(rules, rect)
	return problem


func _compare_solutions(gd_state: WFCSolverState, native_state) -> int:
	var gd_solutions = gd_state.cell_solution_or_entropy
	var native_solutions = native_state.get_cell_solution_or_entropy()

	if gd_solutions.size() != native_solutions.size():
		return -1  # Size mismatch

	var differences = 0
	for i in range(gd_solutions.size()):
		if gd_solutions[i] != native_solutions[i]:
			differences += 1

	return differences


func test_native_classes_available():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	assert_true(ClassDB.class_exists("WFCSolverNative"))
	assert_true(ClassDB.class_exists("WFC2DProblemNative"))


func test_deterministic_10x10():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(10, 10)
	var test_seed = 12345

	var gd_problem = _create_gd_problem(tile_count, grid_size)
	var native_problem = _create_native_problem(tile_count, grid_size)

	# Run GDScript solver
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)
	var gd_state = gd_solver.solve()

	# Run Native solver
	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)
	var native_state = native_solver.solve()

	# Compare - allow up to 5 cells difference due to tie-breaking
	var differences = _compare_solutions(gd_state, native_state)
	gut.p("10x10 grid: %d cells differ" % differences)
	assert_lt(differences, 5, "Solutions should be nearly identical")


func test_deterministic_20x20():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(20, 20)
	var test_seed = 12345

	var gd_problem = _create_gd_problem(tile_count, grid_size)
	var native_problem = _create_native_problem(tile_count, grid_size)

	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)
	var gd_state = gd_solver.solve()

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)
	var native_state = native_solver.solve()

	var differences = _compare_solutions(gd_state, native_state)
	gut.p("20x20 grid: %d cells differ" % differences)
	assert_lt(differences, 10, "Solutions should be nearly identical")


func test_native_solves_faster():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(20, 20)
	var test_seed = 12345

	var gd_problem = _create_gd_problem(tile_count, grid_size)
	var native_problem = _create_native_problem(tile_count, grid_size)

	# Time GDScript
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)
	var gd_start = Time.get_ticks_usec()
	gd_solver.solve()
	var gd_time = Time.get_ticks_usec() - gd_start

	# Time Native
	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)
	var native_start = Time.get_ticks_usec()
	native_solver.solve()
	var native_time = Time.get_ticks_usec() - native_start

	var speedup = float(gd_time) / float(native_time)
	gut.p("GDScript: %.1f ms, Native: %.1f ms, Speedup: %.1fx" % [gd_time/1000.0, native_time/1000.0, speedup])

	# Native should be at least 1.5x faster
	assert_gt(speedup, 1.5, "Native solver should be faster than GDScript")


func test_native_bitset_correctness():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var bs = WFCBitSetNative.new()
	bs.initialize(64, false)
	bs.set_bit(0, true)
	bs.set_bit(10, true)
	bs.set_bit(63, true)

	assert_eq(bs.get_size(), 64)
	assert_true(bs.get_bit(0))
	assert_true(bs.get_bit(10))
	assert_true(bs.get_bit(63))
	assert_false(bs.get_bit(5))
	assert_eq(bs.count_set_bits(), 3)

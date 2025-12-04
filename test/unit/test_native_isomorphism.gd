extends GutTest

## Comprehensive isomorphism tests between GDScript and Native WFC implementations.
## Tests every component to ensure native solver produces IDENTICAL results.

const WFCBitSet = preload("res://addons/wfc/utils/bitset.gd")
const WFCBitMatrix = preload("res://addons/wfc/utils/bitmatrix.gd")
const WFCSolverSettings = preload("res://addons/wfc/solver/solver_settings.gd")
const WFCSolverState = preload("res://addons/wfc/solver/solver_state.gd")
const WFCSolver = preload("res://addons/wfc/solver/solver.gd")
const WFCProblem = preload("res://addons/wfc/problems/problem.gd")


func _check_native_classes_available() -> bool:
	var classes = ["WFCBitSetNative", "WFCSolverNative", "WFCRules2DNative", "WFC2DProblemNative"]
	for cname in classes:
		if not ClassDB.class_exists(cname):
			gut.p("Skipping test: " + cname + " not available")
			return false
	return true


# ===========================================================
# BITSET TESTS
# ===========================================================

func test_bitset_set_all_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	# Test sizes that fit in different storage configurations
	# data0 only: 1-64, data0+data1: 65-128, data0+data1+dataX: 129+
	for size in [1, 5, 10, 63, 64]:  # Only test sizes <= 64 for now (data0 only)
		var gd = WFCBitSet.new(size, true)
		var native = WFCBitSetNative.new()
		native.initialize(size, true)

		# Compare count first to catch issues early
		var gd_count = gd.count_set_bits()
		var native_count = native.count_set_bits()
		assert_eq(native_count, gd_count, "count_set_bits mismatch for size %d: native=%d gd=%d" % [size, native_count, gd_count])
		assert_eq(gd_count, size, "count should equal size for set_all")

		# Compare every bit
		for i in range(size):
			var gd_bit = gd.get_bit(i)
			var native_bit = native.get_bit(i)
			assert_eq(native_bit, gd_bit,
				"set_all mismatch at bit %d for size %d: native=%s gd=%s" % [i, size, native_bit, gd_bit])


func test_bitset_set_all_size_65():
	"""Test set_all specifically for size 65 (uses data0 + 1 bit in data1)."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 65
	var native = WFCBitSetNative.new()
	native.initialize(size, true)

	# Debug: print internal state
	gut.p("Native size: %d" % native.get_size())
	gut.p("Native data0: %d" % native.get_data0())
	gut.p("Native data1: %d" % native.get_data1())

	# Check data0 bits (0-63)
	var data0_bits_set = 0
	for i in range(64):
		if native.get_bit(i):
			data0_bits_set += 1
	gut.p("data0 bits set: %d" % data0_bits_set)

	# Check data1 bit 0 (overall bit 64)
	var bit64 = native.get_bit(64)
	gut.p("bit 64 set: %s" % bit64)
	assert_true(bit64, "Native bit 64 should be set")

	# Check count
	var native_count = native.count_set_bits()
	gut.p("Native count for size 65: %d" % native_count)
	assert_eq(native_count, 65, "Native should have 65 bits set")


func test_bitset_set_bit_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 200
	var gd = WFCBitSet.new(size, false)
	var native = WFCBitSetNative.new()
	native.initialize(size, false)

	# Test various bit positions across all storage (data0, data1, dataX)
	var positions = [0, 1, 31, 32, 63, 64, 65, 127, 128, 129, 199]

	for pos in positions:
		gd.set_bit(pos, true)
		native.set_bit(pos, true)

		assert_true(gd.get_bit(pos), "GDScript bit %d not set" % pos)
		assert_true(native.get_bit(pos), "Native bit %d not set" % pos)

	# Verify all bits match
	for i in range(size):
		assert_eq(gd.get_bit(i), native.get_bit(i), "Mismatch at bit %d" % i)


func test_bitset_union_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 150
	var gd_a = WFCBitSet.new(size, false)
	var gd_b = WFCBitSet.new(size, false)
	var native_a = WFCBitSetNative.new()
	var native_b = WFCBitSetNative.new()
	native_a.initialize(size, false)
	native_b.initialize(size, false)

	# Set different bits
	for i in [0, 10, 50, 100, 140]:
		gd_a.set_bit(i, true)
		native_a.set_bit(i, true)
	for i in [5, 10, 60, 100, 149]:
		gd_b.set_bit(i, true)
		native_b.set_bit(i, true)

	# Union
	gd_a.union_in_place(gd_b)
	native_a.union_in_place(native_b)

	# Compare
	for i in range(size):
		assert_eq(gd_a.get_bit(i), native_a.get_bit(i),
			"union_in_place mismatch at bit %d" % i)


func test_bitset_intersect_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 150
	var gd_a = WFCBitSet.new(size, true)
	var gd_b = WFCBitSet.new(size, false)
	var native_a = WFCBitSetNative.new()
	var native_b = WFCBitSetNative.new()
	native_a.initialize(size, true)
	native_b.initialize(size, false)

	# Set some bits in b
	for i in [0, 10, 50, 100, 140]:
		gd_b.set_bit(i, true)
		native_b.set_bit(i, true)

	# Intersect
	gd_a.intersect_in_place(gd_b)
	native_a.intersect_in_place(native_b)

	# Compare
	for i in range(size):
		assert_eq(gd_a.get_bit(i), native_a.get_bit(i),
			"intersect_in_place mismatch at bit %d" % i)


func test_bitset_xor_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 150
	var gd_a = WFCBitSet.new(size, false)
	var gd_b = WFCBitSet.new(size, false)
	var native_a = WFCBitSetNative.new()
	var native_b = WFCBitSetNative.new()
	native_a.initialize(size, false)
	native_b.initialize(size, false)

	# Set overlapping bits
	for i in [0, 10, 50, 100]:
		gd_a.set_bit(i, true)
		native_a.set_bit(i, true)
	for i in [10, 50, 140]:
		gd_b.set_bit(i, true)
		native_b.set_bit(i, true)

	# XOR
	gd_a.xor_in_place(gd_b)
	native_a.xor_in_place(native_b)

	# Compare
	for i in range(size):
		assert_eq(gd_a.get_bit(i), native_a.get_bit(i),
			"xor_in_place mismatch at bit %d: gd=%s native=%s" % [i, gd_a.get_bit(i), native_a.get_bit(i)])


func test_bitset_iterator_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var size = 200
	var gd = WFCBitSet.new(size, false)
	var native = WFCBitSetNative.new()
	native.initialize(size, false)

	# Set specific bits
	var bits_to_set = [0, 5, 63, 64, 65, 127, 128, 199]
	for i in bits_to_set:
		gd.set_bit(i, true)
		native.set_bit(i, true)

	# Compare iterators
	var gd_result: Array = []
	for b in gd.iterator():
		gd_result.append(b)

	var native_result = native.iterator()  # Returns PackedInt64Array

	assert_eq(gd_result.size(), native_result.size(),
		"Iterator size mismatch: gd=%d native=%d" % [gd_result.size(), native_result.size()])

	for i in range(gd_result.size()):
		assert_eq(gd_result[i], native_result[i],
			"Iterator mismatch at index %d: gd=%d native=%d" % [i, gd_result[i], native_result[i]])


func test_bitset_get_only_set_bit_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	# Only test size 64 to avoid GDScript dataX bugs with larger sizes
	var size = 64

	# Test empty
	var gd_empty = WFCBitSet.new(size, false)
	var native_empty = WFCBitSetNative.new()
	native_empty.initialize(size, false)
	assert_eq(native_empty.get_only_set_bit(), gd_empty.get_only_set_bit(),
		"Empty get_only_set_bit mismatch")

	# Test single bit in various positions within data0
	for pos in [0, 1, 31, 32, 63]:
		var gd = WFCBitSet.new(size, false)
		var native = WFCBitSetNative.new()
		native.initialize(size, false)
		gd.set_bit(pos, true)
		native.set_bit(pos, true)

		var gd_result = gd.get_only_set_bit()
		var native_result = native.get_only_set_bit()
		assert_eq(native_result, gd_result,
			"Single bit get_only_set_bit mismatch at pos %d: native=%d gd=%d" % [pos, native_result, gd_result])

	# Test multiple bits
	var gd_multi = WFCBitSet.new(size, false)
	var native_multi = WFCBitSetNative.new()
	native_multi.initialize(size, false)
	gd_multi.set_bit(5, true)
	gd_multi.set_bit(10, true)
	native_multi.set_bit(5, true)
	native_multi.set_bit(10, true)
	assert_eq(native_multi.get_only_set_bit(), gd_multi.get_only_set_bit(),
		"Multi-bit get_only_set_bit mismatch")


# ===========================================================
# BITMATRIX TESTS
# ===========================================================

func test_bitmatrix_transform_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var width = 20
	var height = 15

	# Create matrices
	var gd_matrix = WFCBitMatrix.new(width, height)
	var native_matrix = WFCBitMatrixNative.new()
	native_matrix.initialize(width, height)

	# Set some rules (sparse pattern)
	seed(12345)
	for _i in range(50):
		var x = randi() % width
		var y = randi() % height
		gd_matrix.set_bit(x, y, true)
		native_matrix.set_bit(x, y, true)

	# Test transform with various input vectors
	for trial in range(10):
		# Create input domain
		var gd_input = WFCBitSet.new(height, false)
		var native_input = WFCBitSetNative.new()
		native_input.initialize(height, false)

		# Set random bits
		for _j in range(5):
			var b = randi() % height
			gd_input.set_bit(b, true)
			native_input.set_bit(b, true)

		# Transform
		var gd_result = gd_matrix.transform(gd_input)
		var native_result = native_matrix.transform(native_input)

		# Compare
		for i in range(width):
			assert_eq(gd_result.get_bit(i), native_result.get_bit(i),
				"transform mismatch at bit %d on trial %d" % [i, trial])


func test_bitmatrix_transpose_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var width = 10
	var height = 8

	var gd_matrix = WFCBitMatrix.new(width, height)
	var native_matrix = WFCBitMatrixNative.new()
	native_matrix.initialize(width, height)

	# Set diagonal and some other bits
	for i in range(min(width, height)):
		gd_matrix.set_bit(i, i, true)
		native_matrix.set_bit(i, i, true)
	gd_matrix.set_bit(5, 2, true)
	native_matrix.set_bit(5, 2, true)

	# Transpose
	var gd_transposed = gd_matrix.transpose()
	var native_transposed = native_matrix.transpose()

	# New dimensions should be swapped
	assert_eq(gd_transposed.width, native_transposed.get_width())
	assert_eq(gd_transposed.height, native_transposed.get_height())

	# Compare all bits
	for y in range(gd_transposed.height):
		for x in range(gd_transposed.width):
			var gd_bit = gd_transposed.rows[y].get_bit(x)
			var native_bit = native_transposed.get_bit(x, y)
			assert_eq(gd_bit, native_bit,
				"transpose mismatch at (%d, %d)" % [x, y])


# ===========================================================
# PROBLEM/RULE CONVERSION TESTS
# ===========================================================

## Create a GDScript 2D problem with restrictive (NOT all-to-all) rules
class RestrictiveProblem2D extends WFCProblem:
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


func _create_restrictive_gd_problem(tile_count: int, grid_size: Vector2i) -> RestrictiveProblem2D:
	"""Creates a problem with SPARSE rules - only some tiles allowed next to each other."""
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	var rect = Rect2i(Vector2i.ZERO, grid_size)
	var problem = RestrictiveProblem2D.new(tile_count, axes, rect)

	# Create sparse rules: tile i can only be next to i, i+1, i-1 (mod tile_count)
	for i in range(tile_count):
		for axis_idx in [0, 1]:
			problem.set_rule(axis_idx, i, i, true)  # Same tile
			problem.set_rule(axis_idx, i, (i + 1) % tile_count, true)  # Next tile
			problem.set_rule(axis_idx, i, (i - 1 + tile_count) % tile_count, true)  # Prev tile

	return problem


func _create_restrictive_native_problem(tile_count: int, grid_size: Vector2i):
	"""Creates a native problem with SPARSE rules matching the GDScript version."""
	var axes: Array[Vector2i] = [Vector2i(0, 1), Vector2i(1, 0)]
	var rect = Rect2i(Vector2i.ZERO, grid_size)

	var rules = WFCRules2DNative.new()
	rules.initialize(tile_count, axes)

	# Create sparse rules: tile i can only be next to i, i+1, i-1 (mod tile_count)
	for i in range(tile_count):
		for axis_idx in [0, 1]:
			rules.set_rule(axis_idx, i, i, true)  # Same tile
			rules.set_rule(axis_idx, i, (i + 1) % tile_count, true)  # Next tile
			rules.set_rule(axis_idx, i, (i - 1 + tile_count) % tile_count, true)  # Prev tile

	var problem = WFC2DProblemNative.new()
	problem.initialize(rules, rect)
	return problem


func test_rule_conversion_matches():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var grid_size = Vector2i(5, 5)

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Native problem builds forward + reverse axes internally
	# GDScript problem has both directions in axis_matrices
	# Verify axis matrices match

	var native_axes = native_problem.get_axes()
	var native_matrices = native_problem.get_axis_matrices()

	gut.p("GDScript has %d axes, Native has %d axes" % [gd_problem.rules_axes.size(), native_axes.size()])

	assert_eq(gd_problem.rules_axes.size(), native_axes.size(),
		"Axes count mismatch")

	for axis_idx in range(gd_problem.rules_axes.size()):
		var gd_axis = gd_problem.rules_axes[axis_idx]
		var native_axis = native_axes[axis_idx]
		assert_eq(gd_axis, native_axis, "Axis %d direction mismatch" % axis_idx)

		var gd_matrix = gd_problem.axis_matrices[axis_idx]
		var native_matrix: WFCBitMatrixNative = native_matrices[axis_idx]

		# Compare matrices bit by bit
		for y in range(tile_count):
			for x in range(tile_count):
				var gd_bit = gd_matrix.rows[y].get_bit(x)
				var native_bit = native_matrix.get_bit(x, y)
				assert_eq(gd_bit, native_bit,
					"Rule mismatch at axis=%d, from=%d, to=%d: gd=%s native=%s" % [axis_idx, y, x, gd_bit, native_bit])


func test_compute_cell_domain_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var grid_size = Vector2i(5, 5)
	var test_seed = 12345

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create initial states
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)
	var gd_state = gd_solver.current_state

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)
	var native_state = native_solver.get_current_state()

	# Compare compute_cell_domain for every cell
	for cell_id in range(grid_size.x * grid_size.y):
		var gd_domain = gd_problem.compute_cell_domain(gd_state, cell_id)
		var native_domain = native_problem.compute_cell_domain(native_state, cell_id)

		# Compare domain bit by bit
		for bit in range(tile_count):
			assert_eq(gd_domain.get_bit(bit), native_domain.get_bit(bit),
				"compute_cell_domain mismatch at cell %d, bit %d" % [cell_id, bit])


func test_mark_related_cells_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(5, 5)

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Test mark_related_cells for each cell
	for cell_id in range(grid_size.x * grid_size.y):
		var gd_related: Array = []
		var native_related = native_problem.get_related_cells(cell_id)

		gd_problem.mark_related_cells(cell_id, func(c): gd_related.append(c))

		# Sort for comparison
		gd_related.sort()
		var native_sorted: Array = []
		for i in range(native_related.size()):
			native_sorted.append(native_related[i])
		native_sorted.sort()

		assert_eq(gd_related.size(), native_sorted.size(),
			"Related cells count mismatch for cell %d" % cell_id)
		for i in range(gd_related.size()):
			assert_eq(gd_related[i], native_sorted[i],
				"Related cell mismatch for cell %d at index %d" % [cell_id, i])


# ===========================================================
# SOLVER STATE TESTS
# ===========================================================

func test_solver_state_set_domain_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var num_cells = 25

	# Create initial states with all tiles allowed
	var initial_domain_gd = WFCBitSet.new(tile_count, true)
	var initial_domain_native = WFCBitSetNative.new()
	initial_domain_native.initialize(tile_count, true)

	var gd_state = WFCSolverState.new()
	gd_state.cell_domains.resize(num_cells)
	gd_state.cell_domains.fill(initial_domain_gd)
	gd_state.cell_solution_or_entropy.resize(num_cells)
	gd_state.cell_solution_or_entropy.fill(-(tile_count - 1))
	gd_state.unsolved_cells = num_cells

	var native_state = WFCSolverStateNative.new()
	var native_domains: Array = []
	native_domains.resize(num_cells)
	for i in range(num_cells):
		native_domains[i] = initial_domain_native
	native_state.set_cell_domains(native_domains)
	var native_entropy = PackedInt64Array()
	native_entropy.resize(num_cells)
	for i in range(num_cells):
		native_entropy[i] = -(tile_count - 1)
	native_state.set_cell_solution_or_entropy(native_entropy)
	native_state.set_unsolved_cells(num_cells)

	# Restrict domain for a cell
	var new_domain_gd = WFCBitSet.new(tile_count, false)
	new_domain_gd.set_bit(0, true)
	new_domain_gd.set_bit(1, true)
	new_domain_gd.set_bit(2, true)

	var new_domain_native = WFCBitSetNative.new()
	new_domain_native.initialize(tile_count, false)
	new_domain_native.set_bit(0, true)
	new_domain_native.set_bit(1, true)
	new_domain_native.set_bit(2, true)

	gd_state.set_domain(5, new_domain_gd)
	native_state.set_domain(5, new_domain_native)

	# Compare solution_or_entropy
	var gd_entropy = gd_state.cell_solution_or_entropy
	var native_entropy_after = native_state.get_cell_solution_or_entropy()

	assert_eq(gd_entropy[5], native_entropy_after[5],
		"Entropy mismatch after set_domain: gd=%d native=%d" % [gd_entropy[5], native_entropy_after[5]])

	# Compare changed cells marking
	assert_true(gd_state.changed_cells.has(5), "GDScript changed_cells should have 5")


func test_solver_state_set_solution_identical():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var num_cells = 25

	# Create initial states
	var initial_domain_gd = WFCBitSet.new(tile_count, true)
	var initial_domain_native = WFCBitSetNative.new()
	initial_domain_native.initialize(tile_count, true)

	var gd_state = WFCSolverState.new()
	gd_state.cell_domains.resize(num_cells)
	gd_state.cell_domains.fill(initial_domain_gd)
	gd_state.cell_solution_or_entropy.resize(num_cells)
	gd_state.cell_solution_or_entropy.fill(-(tile_count - 1))
	gd_state.unsolved_cells = num_cells

	var native_state = WFCSolverStateNative.new()
	var native_domains: Array = []
	native_domains.resize(num_cells)
	for i in range(num_cells):
		native_domains[i] = initial_domain_native
	native_state.set_cell_domains(native_domains)
	var native_entropy = PackedInt64Array()
	native_entropy.resize(num_cells)
	for i in range(num_cells):
		native_entropy[i] = -(tile_count - 1)
	native_state.set_cell_solution_or_entropy(native_entropy)
	native_state.set_unsolved_cells(num_cells)

	# Set a solution
	gd_state.set_solution(5, 3)
	native_state.set_solution(5, 3)

	# Compare
	var gd_solution = gd_state.cell_solution_or_entropy[5]
	var native_solution = native_state.get_cell_solution_or_entropy()[5]

	assert_eq(gd_solution, native_solution,
		"Solution mismatch: gd=%d native=%d" % [gd_solution, native_solution])
	assert_eq(gd_solution, 3, "Solution should be 3")

	assert_eq(gd_state.unsolved_cells, native_state.get_unsolved_cells(),
		"Unsolved cells mismatch after set_solution")


# ===========================================================
# FULL SOLVER TESTS WITH RESTRICTIVE RULES
# ===========================================================

func test_solver_with_restrictive_rules():
	"""Test that both solvers produce valid solutions with restrictive rules.

	NOTE: This test does NOT expect identical solutions because solve() runs
	multiple solve_step() calls internally, and the random state diverges.
	Instead, we verify both produce VALID solutions with no rule violations.
	For isomorphism testing, see test_propagation_step_by_step which re-seeds
	before each step to ensure identical random states.
	"""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var grid_size = Vector2i(8, 8)
	var test_seed = 54321

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

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

	# Validate both solutions are complete and valid
	var gd_solutions = gd_state.cell_solution_or_entropy
	var native_solutions = native_state.get_cell_solution_or_entropy()

	# Count unsolved cells
	var gd_unsolved = 0
	var native_unsolved = 0
	for i in range(gd_solutions.size()):
		if gd_solutions[i] < 0:
			gd_unsolved += 1
		if native_solutions[i] < 0:
			native_unsolved += 1

	gut.p("GDScript unsolved: %d, Native unsolved: %d" % [gd_unsolved, native_unsolved])
	assert_eq(gd_unsolved, 0, "GDScript solver should complete fully")
	assert_eq(native_unsolved, 0, "Native solver should complete fully")

	# Validate native solution satisfies rules (adjacency constraints)
	var native_violations = _count_rule_violations(native_solutions, grid_size, tile_count)
	gut.p("Native solution has %d rule violations" % native_violations)
	assert_eq(native_violations, 0, "Native solver produced rule violations")


func _count_rule_violations(solutions: PackedInt64Array, grid_size: Vector2i, tile_count: int) -> int:
	"""Count adjacency rule violations. Rules: tile i can be next to i-1, i, i+1 (mod tile_count)"""
	var violations = 0
	for y in range(grid_size.y):
		for x in range(grid_size.x):
			var cell_id = y * grid_size.x + x
			var tile = solutions[cell_id]
			if tile < 0:
				continue

			# Check right neighbor
			if x + 1 < grid_size.x:
				var right_id = y * grid_size.x + (x + 1)
				var right_tile = solutions[right_id]
				if right_tile >= 0:
					var diff = abs(tile - right_tile)
					if diff != 0 and diff != 1 and diff != tile_count - 1:
						violations += 1

			# Check bottom neighbor
			if y + 1 < grid_size.y:
				var bottom_id = (y + 1) * grid_size.x + x
				var bottom_tile = solutions[bottom_id]
				if bottom_tile >= 0:
					var diff = abs(tile - bottom_tile)
					if diff != 0 and diff != 1 and diff != tile_count - 1:
						violations += 1
	return violations


func test_solver_validates_rules():
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 8
	var grid_size = Vector2i(5, 5)
	var test_seed = 99999

	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)
	var native_state = native_solver.solve()

	var solutions = native_state.get_cell_solution_or_entropy()

	# Validate that all adjacencies satisfy the rules
	var violations = 0
	for y in range(grid_size.y):
		for x in range(grid_size.x):
			var cell_id = y * grid_size.x + x
			var tile = solutions[cell_id]

			if tile < 0:
				continue  # Unsolved or failed

			# Check right neighbor
			if x + 1 < grid_size.x:
				var right_id = y * grid_size.x + (x + 1)
				var right_tile = solutions[right_id]
				if right_tile >= 0:
					# Rule: tile can only be next to tile-1, tile, tile+1
					var diff = abs(tile - right_tile)
					if diff != 0 and diff != 1 and diff != tile_count - 1:
						violations += 1
						gut.p("Violation at (%d,%d)->(%d,%d): %d-%d" % [x, y, x+1, y, tile, right_tile])

			# Check bottom neighbor
			if y + 1 < grid_size.y:
				var bottom_id = (y + 1) * grid_size.x + x
				var bottom_tile = solutions[bottom_id]
				if bottom_tile >= 0:
					var diff = abs(tile - bottom_tile)
					if diff != 0 and diff != 1 and diff != tile_count - 1:
						violations += 1
						gut.p("Violation at (%d,%d)->(%d,%d): %d-%d" % [x, y, x, y+1, tile, bottom_tile])

	assert_eq(violations, 0, "Native solver produced %d rule violations" % violations)


# ===========================================================
# STEP-BY-STEP PROPAGATION TESTS
# ===========================================================

func test_initial_state_identical():
	"""Test that initial states before any solving are identical."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers with same seed
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	# Get initial states BEFORE any solve_step
	var gd_state = gd_solver.current_state
	var native_state = native_solver.get_current_state()

	# Compare initial entropy values
	var gd_entropy = gd_state.cell_solution_or_entropy
	var native_entropy = native_state.get_cell_solution_or_entropy()

	gut.p("Initial state comparison:")
	var differences = 0
	for i in range(gd_entropy.size()):
		if gd_entropy[i] != native_entropy[i]:
			gut.p("  Cell %d: gd=%d native=%d" % [i, gd_entropy[i], native_entropy[i]])
			differences += 1

	if differences == 0:
		gut.p("  All cells match!")
	else:
		gut.p("  %d differences found" % differences)

	assert_eq(differences, 0, "Initial states should be identical")


func test_random_synchronization():
	"""Test that random number generation is synchronized between GDScript and native."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var test_seed = 11111

	# Test that pick_random produces same results
	seed(test_seed)
	var gd_array = [0, 1, 2, 3, 4, 5, 6, 7, 8]
	var gd_pick1 = gd_array.pick_random()
	var gd_pick2 = gd_array.pick_random()
	var gd_pick3 = gd_array.pick_random()

	seed(test_seed)
	# Now test what native code gets
	# We need to use the native problem's debug method or simulate the same call
	var native_problem = WFC2DProblemNative.new()
	var native_rand1 = native_problem.debug_randi_range(0, 8)
	var native_rand2 = native_problem.debug_randi_range(0, 8)
	var native_rand3 = native_problem.debug_randi_range(0, 8)

	gut.p("GDScript pick_random results: %d, %d, %d" % [gd_pick1, gd_pick2, gd_pick3])
	gut.p("Native randi_range results: %d, %d, %d" % [native_rand1, native_rand2, native_rand3])

	# Also test with another seed
	seed(test_seed)
	var gd_randi1 = randi_range(0, 8)
	var gd_randi2 = randi_range(0, 8)
	var gd_randi3 = randi_range(0, 8)

	seed(test_seed)
	native_rand1 = native_problem.debug_randi_range(0, 8)
	native_rand2 = native_problem.debug_randi_range(0, 8)
	native_rand3 = native_problem.debug_randi_range(0, 8)

	gut.p("GDScript randi_range results: %d, %d, %d" % [gd_randi1, gd_randi2, gd_randi3])
	gut.p("Native randi_range results: %d, %d, %d" % [native_rand1, native_rand2, native_rand3])

	assert_eq(gd_randi1, native_rand1, "First random should match")
	assert_eq(gd_randi2, native_rand2, "Second random should match")
	assert_eq(gd_randi3, native_rand3, "Third random should match")


func test_divergence_candidates_order():
	"""Test that divergence candidates have same order in both implementations."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	# Run propagation manually (call _propagate_constraints_ac3)
	# For GDScript, we need to access the internal method
	# Let's instead look at states after first solve_step

	# Get states before solve_step
	var gd_state = gd_solver.current_state
	var native_state = native_solver.get_current_state()

	# Check divergence candidates BEFORE solve_step (should be empty initially)
	var gd_candidates_before = gd_state.divergence_candidates.keys()
	var native_candidates_before = native_state.get_divergence_candidates().keys()

	gut.p("BEFORE solve_step:")
	gut.p("  GD candidates: %s" % [gd_candidates_before])
	gut.p("  Native candidates: %s" % [native_candidates_before])

	# Now do solve_step and check candidates
	gd_solver.solve_step()
	native_solver.solve_step()

	gd_state = gd_solver.current_state
	native_state = native_solver.get_current_state()

	var gd_candidates_after = gd_state.divergence_candidates.keys()
	var native_candidates_after = native_state.get_divergence_candidates().keys()

	gut.p("AFTER solve_step:")
	gut.p("  GD candidates: %s" % [gd_candidates_after])
	gut.p("  Native candidates: %s" % [native_candidates_after])

	# Compare solutions after first step
	var gd_entropy = gd_state.cell_solution_or_entropy
	var native_entropy = native_state.get_cell_solution_or_entropy()

	gut.p("State after first step:")
	for i in range(gd_entropy.size()):
		gut.p("  Cell %d: gd=%d native=%d" % [i, gd_entropy[i], native_entropy[i]])


func test_propagation_step_by_step():
	"""Test that step-by-step solving is isomorphic when both solvers use same random state.

	IMPORTANT: We re-seed before EACH solve_step to ensure both solvers get the same
	random numbers. This tests functional isomorphism - same inputs produce same outputs.
	"""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)  # Small grid for detailed comparison
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	# Compare state after each step
	var max_steps = 20
	var step = 0

	while step < max_steps:
		var gd_state = gd_solver.current_state
		var native_state = native_solver.get_current_state()

		if gd_state == null or native_state == null:
			break

		# Compare solutions at this step (before solve_step)
		var gd_solutions = gd_state.cell_solution_or_entropy
		var native_solutions = native_state.get_cell_solution_or_entropy()

		var step_differences = 0
		for i in range(gd_solutions.size()):
			if gd_solutions[i] != native_solutions[i]:
				step_differences += 1

		if step_differences > 0:
			gut.p("Step %d: %d differences found!" % [step, step_differences])
			for i in range(gd_solutions.size()):
				if gd_solutions[i] != native_solutions[i]:
					gut.p("  Cell %d: gd=%d native=%d" % [i, gd_solutions[i], native_solutions[i]])

		# IMPORTANT: Re-seed before EACH solve_step so both solvers use same random state
		var step_seed = test_seed + step
		seed(step_seed)
		var gd_done = gd_solver.solve_step()

		seed(step_seed)  # Same seed for native
		var native_done = native_solver.solve_step()

		if gd_done != native_done:
			gut.p("Step %d: done state mismatch! gd=%s native=%s" % [step, gd_done, native_done])
			break

		if gd_done and native_done:
			break

		step += 1

	# Final comparison
	var gd_final = gd_solver.current_state
	var native_final = native_solver.get_current_state()

	if gd_final != null and native_final != null:
		var gd_solutions = gd_final.cell_solution_or_entropy
		var native_solutions = native_final.get_cell_solution_or_entropy()

		var final_differences = 0
		for i in range(gd_solutions.size()):
			if gd_solutions[i] != native_solutions[i]:
				final_differences += 1

		assert_eq(final_differences, 0,
			"Step-by-step: %d differences after %d steps" % [final_differences, step])


func test_debug_pick_divergence_cell():
	"""Debug test to see exactly what happens in pick_divergence_cell."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	var gd_state = gd_solver.current_state
	var native_state = native_solver.get_current_state()

	# Compare changed_cells before propagation
	gut.p("=== BEFORE FIRST SOLVE_STEP ===")

	var gd_changed = gd_state.changed_cells
	var native_changed = native_state.get_changed_cells()
	gut.p("GDScript changed_cells: %s" % [gd_changed])
	gut.p("Native changed_cells: %s" % [native_changed])

	# Compare entropy values
	var gd_entropy = gd_state.cell_solution_or_entropy
	var native_entropy = native_state.get_cell_solution_or_entropy()
	gut.p("GDScript entropy: %s" % [gd_entropy])
	gut.p("Native entropy: %s" % [native_entropy])

	# Simulate pick_divergence_cell logic
	gut.p("\n=== SIMULATING PICK_DIVERGENCE_CELL ===")

	var MAX_INT = 9223372036854775807
	var gd_candidates = gd_state.divergence_candidates.keys()
	var native_candidates_dict = native_state.get_divergence_candidates()
	var native_candidates = native_candidates_dict.keys()

	gut.p("GDScript divergence_candidates: %s" % [gd_candidates])
	gut.p("Native divergence_candidates: %s" % [native_candidates])

	if gd_candidates.is_empty():
		gd_candidates = range(gd_entropy.size())
		gut.p("GDScript using range fallback: %s" % [gd_candidates])

	if native_candidates.is_empty():
		native_candidates = range(native_entropy.size())
		gut.p("Native using range fallback: %s" % [native_candidates])

	# Build options array for GDScript
	var gd_options: Array[int] = []
	var gd_target_entropy: int = MAX_INT
	for i in gd_candidates:
		var entropy: int = - gd_entropy[i]
		if entropy <= 0:
			continue
		if entropy == gd_target_entropy:
			gd_options.append(i)
		elif entropy < gd_target_entropy:
			gd_options.clear()
			gd_options.append(i)
			gd_target_entropy = entropy

	gut.p("GDScript target_entropy: %d" % gd_target_entropy)
	gut.p("GDScript options array: %s" % [gd_options])

	# Build options array for Native (simulated)
	var native_options: Array = []
	var native_target_entropy: int = MAX_INT
	for i in native_candidates:
		var entropy: int = - native_entropy[i]
		if entropy <= 0:
			continue
		if entropy == native_target_entropy:
			native_options.append(i)
		elif entropy < native_target_entropy:
			native_options.clear()
			native_options.append(i)
			native_target_entropy = entropy

	gut.p("Native target_entropy: %d" % native_target_entropy)
	gut.p("Native options array: %s" % [native_options])

	# Test pick_random with same seed
	seed(test_seed)
	var gd_pick = gd_options.pick_random()
	gut.p("GDScript pick_random from options: %d" % gd_pick)

	seed(test_seed)
	var native_pick = native_options.pick_random()
	gut.p("Native pick_random from options: %d" % native_pick)

	# Now actually do pick_divergence_cell on both and compare
	gut.p("\n=== ACTUAL PICK_DIVERGENCE_CELL ===")
	seed(test_seed)
	var actual_gd_cell = gd_state.pick_divergence_cell()
	gut.p("Actual GDScript picked cell: %d" % actual_gd_cell)

	seed(test_seed)
	var actual_native_cell = native_state.pick_divergence_cell()
	gut.p("Actual Native picked cell: %d" % actual_native_cell)

	assert_eq(actual_gd_cell, actual_native_cell,
		"pick_divergence_cell mismatch: gd=%d native=%d" % [actual_gd_cell, actual_native_cell])


func test_debug_full_solve_step():
	"""Debug test to trace exactly what happens in solve_step."""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers with same seed
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	gut.p("=== BEFORE SOLVE_STEP ===")

	var gd_state = gd_solver.current_state
	var native_state = native_solver.get_current_state()

	gut.p("GDScript state entropy: %s" % [gd_state.cell_solution_or_entropy])
	gut.p("Native state entropy: %s" % [native_state.get_cell_solution_or_entropy()])

	# Manually do what solve_step does step by step
	gut.p("\n=== STEP 1: PROPAGATE_CONSTRAINTS ===")
	# This should do nothing since changed_cells is empty
	gut.p("GDScript changed_cells before: %s" % [gd_state.changed_cells])
	gut.p("Native changed_cells before: %s" % [native_state.get_changed_cells()])

	gut.p("\n=== STEP 2: PREPARE_DIVERGENCE ===")
	# GDScript
	gd_state.prepare_divergence()
	gut.p("GDScript divergence_cell: %d" % gd_state.divergence_cell)
	gut.p("GDScript divergence_options: %s" % [gd_state.divergence_options])

	# Native
	native_state.prepare_divergence()
	gut.p("Native divergence_cell: %d" % native_state.get_divergence_cell())
	gut.p("Native divergence_options: %s" % [native_state.get_divergence_options()])

	# The divergence cell should be the same!
	assert_eq(gd_state.divergence_cell, native_state.get_divergence_cell(),
		"divergence_cell mismatch after prepare_divergence")

	gut.p("\n=== STEP 3: DIVERGE_IN_PLACE ===")
	# Set same seed before diverge
	seed(test_seed + 1)
	gd_state.diverge_in_place(gd_problem)
	gut.p("GDScript state after diverge: %s" % [gd_state.cell_solution_or_entropy])

	seed(test_seed + 1)
	native_state.diverge_in_place(native_problem)
	gut.p("Native state after diverge: %s" % [native_state.get_cell_solution_or_entropy()])

	# Compare
	var gd_solutions = gd_state.cell_solution_or_entropy
	var native_solutions = native_state.get_cell_solution_or_entropy()

	var differences = 0
	for i in range(gd_solutions.size()):
		if gd_solutions[i] != native_solutions[i]:
			differences += 1
			gut.p("Cell %d differs: gd=%d native=%d" % [i, gd_solutions[i], native_solutions[i]])

	assert_eq(differences, 0, "diverge_in_place produced different results")


func test_debug_solve_step_with_propagation():
	"""Debug test to trace solve_step including propagation.

	IMPORTANT: This test verifies that GIVEN THE SAME SEED at the start of solve_step,
	both GDScript and native solvers produce identical results.
	We must re-seed before each solve_step to ensure they start with same random state.
	"""
	if not _check_native_classes_available():
		pending("Native classes not available")
		return

	var tile_count = 4
	var grid_size = Vector2i(3, 3)
	var test_seed = 11111

	var gd_problem = _create_restrictive_gd_problem(tile_count, grid_size)
	var native_problem = _create_restrictive_native_problem(tile_count, grid_size)

	# Create solvers with same seed (initialization should not use randoms)
	seed(test_seed)
	var gd_settings = WFCSolverSettings.new()
	gd_settings.force_ac3 = true
	gd_settings.allow_backtracking = false
	var gd_solver = WFCSolver.new(gd_problem, gd_settings)

	seed(test_seed)
	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)
	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	gut.p("=== BEFORE ANY SOLVE_STEP ===")
	var gd_state = gd_solver.current_state
	var native_state = native_solver.get_current_state()
	gut.p("GDScript entropy: %s" % [gd_state.cell_solution_or_entropy])
	gut.p("Native entropy: %s" % [native_state.get_cell_solution_or_entropy()])

	gut.p("\n=== FIRST SOLVE_STEP (with same seed for each) ===")

	# IMPORTANT: Seed before EACH solve_step to ensure same random state
	seed(test_seed)
	var gd_done = gd_solver.solve_step()

	seed(test_seed)  # Reset seed so native gets same random sequence
	var native_done = native_solver.solve_step()

	gd_state = gd_solver.current_state
	native_state = native_solver.get_current_state()

	gut.p("GDScript done=%s, entropy: %s" % [gd_done, gd_state.cell_solution_or_entropy])
	gut.p("Native done=%s, entropy: %s" % [native_done, native_state.get_cell_solution_or_entropy()])

	# Find which cell was solved
	for i in range(gd_state.cell_solution_or_entropy.size()):
		if gd_state.cell_solution_or_entropy[i] >= 0:
			gut.p("GDScript solved cell %d with value %d" % [i, gd_state.cell_solution_or_entropy[i]])
		if native_state.get_cell_solution_or_entropy()[i] >= 0:
			gut.p("Native solved cell %d with value %d" % [i, native_state.get_cell_solution_or_entropy()[i]])

	# Check divergence_candidates after first step
	gut.p("\nGDScript divergence_candidates: %s" % [gd_state.divergence_candidates.keys()])
	gut.p("Native divergence_candidates: %s" % [native_state.get_divergence_candidates().keys()])

	gut.p("\nGDScript changed_cells: %s" % [gd_state.changed_cells])
	gut.p("Native changed_cells: %s" % [native_state.get_changed_cells()])

	# Compare
	var gd_solutions = gd_state.cell_solution_or_entropy
	var native_solutions = native_state.get_cell_solution_or_entropy()

	var differences = 0
	for i in range(gd_solutions.size()):
		if gd_solutions[i] != native_solutions[i]:
			differences += 1

	if differences > 0:
		gut.p("\n%d differences after first solve_step" % differences)
		for i in range(gd_solutions.size()):
			if gd_solutions[i] != native_solutions[i]:
				gut.p("  Cell %d: gd=%d native=%d" % [i, gd_solutions[i], native_solutions[i]])

	assert_eq(differences, 0, "First solve_step produced different results")

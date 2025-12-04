extends SceneTree

# Compare native vs GDScript solver output on actual demo scene
# Run: godot45 --headless --script test/compare_demo_output.gd

func _init():
	run_comparison()
	quit()

func run_comparison():
	var test_seed = 12345

	print("=== DEMO SCENE COMPARISON: Native vs GDScript ===")
	print("")

	# Load demo scene to get sample maps
	var demo = load("res://addons/wfc/examples/demo_wfc_2d_tilemap.tscn").instantiate()
	var sample = demo.get_node("sample")
	var negative_sample = demo.get_node("negative_sample")
	var target = demo.get_node("target")

	# Get rect from generator node
	var generator = demo.get_node("generator")
	var rect = generator.rect

	print("Grid size: %d x %d = %d cells" % [rect.size.x, rect.size.y, rect.size.x * rect.size.y])
	print("Rect: %s" % str(rect))
	print("Test seed: %d" % test_seed)
	print("")

	# Create mapper for the tileset
	var mapper_factory = WFC2DMapperFactory.new()
	var mapper = mapper_factory.create_mapper_for(sample)
	mapper.learn_from(sample)
	print("Mapper size (number of tiles): %d" % mapper.size())

	# Learn rules from samples
	var rules = WFCRules2D.new()
	rules.mapper = mapper
	rules.learn_from(sample)
	rules.learn_negative_from(negative_sample)

	print("Rules learned - axes: %s" % str(rules.axes))
	print("Edge domain set: %s" % ("Yes" if rules.edge_domain != null else "No"))
	print("Probabilities enabled: %s" % str(rules.probabilities_enabled))

	# Compute hash of rule matrices to verify they're identical
	var matrix_hash = 0
	for axis_idx in range(rules.axis_matrices.size()):
		var matrix = rules.axis_matrices[axis_idx]
		for i in range(matrix.height):
			for j in matrix.rows[i].iterator():
				matrix_hash = (matrix_hash * 31 + axis_idx * 1000 + i * 100 + j) % 1000000007
	print("Rule matrix hash: %d" % matrix_hash)
	print("")

	# Create precondition (read existing tiles from target)
	var precondition = WFC2DPreconditionReadExistingMap.new(target, mapper)

	# Count preconditioned cells for debugging
	var precondition_count = 0
	for x in range(rect.size.x):
		for y in range(rect.size.y):
			var abs_pos = rect.position + Vector2i(x, y)
			var domain = precondition.read_domain(abs_pos)
			if domain != null:
				precondition_count += 1
	print("Preconditioned cells (existing tiles in target): %d" % precondition_count)

	# === Run GDScript Solver ===
	print("Running GDScript solver...")
	seed(test_seed)

	var gd_settings_obj = WFC2DProblem.WFC2DProblemSettings.new()
	gd_settings_obj.rules = rules
	gd_settings_obj.rect = rect

	var gd_problem = WFC2DProblem.new(gd_settings_obj, target, precondition)
	var gd_solver_settings = WFCSolverSettings.new()
	gd_solver_settings.force_ac3 = true
	var gd_solver = WFCSolver.new(gd_problem, gd_solver_settings)

	# Debug: Check first random call
	print("DEBUG GDScript: First randi_range(0, 99) = %d" % randi_range(0, 99))

	# Debug: Check state after initialization (before solving)
	var gd_init_state = gd_solver.current_state
	print("DEBUG GDScript after init: unsolved=%d, total=%d" % [gd_init_state.unsolved_cells, gd_init_state.cell_solution_or_entropy.size()])

	# Check changed_cells
	var gd_changed = gd_init_state.changed_cells.duplicate()
	print("DEBUG GDScript changed_cells count: %d, first 5: %s" % [gd_changed.size(), str(gd_changed.slice(0, 5))])

	# Check entropy distribution
	var gd_entropy_dist = {}
	for i in range(gd_init_state.cell_solution_or_entropy.size()):
		var e = gd_init_state.cell_solution_or_entropy[i]
		if e < 0:  # unsolved
			var entropy = -e
			gd_entropy_dist[entropy] = gd_entropy_dist.get(entropy, 0) + 1
	print("DEBUG GDScript entropy distribution: %s" % str(gd_entropy_dist))

	# Test pick_random behavior
	seed(test_seed)
	var test_array: Array[int] = [100, 200, 300, 400, 500]
	var gd_picks = []
	for i in range(5):
		gd_picks.append(test_array.pick_random())
	print("DEBUG GDScript pick_random test: %s" % str(gd_picks))

	# Re-seed for actual solve
	seed(test_seed)

	# Track first few divergence decisions
	var gd_first_solved = []
	var before_entropy = gd_solver.current_state.cell_solution_or_entropy.duplicate()
	for i in range(5):
		gd_solver.solve_step()
		var after_entropy = gd_solver.current_state.cell_solution_or_entropy
		for j in range(after_entropy.size()):
			if before_entropy[j] < 0 and after_entropy[j] >= 0:
				gd_first_solved.append({"step": i+1, "cell": j, "value": after_entropy[j]})
				break
		before_entropy = after_entropy.duplicate()
		if gd_solver.current_state.is_all_solved():
			break
	print("DEBUG GDScript first solved: %s" % str(gd_first_solved))

	# Continue solving
	var gd_start_time = Time.get_ticks_msec()
	var gd_state = gd_solver.solve()
	var gd_end_time = Time.get_ticks_msec()

	var gd_solutions = gd_state.cell_solution_or_entropy.duplicate()
	print("GDScript solver completed in %d ms" % (gd_end_time - gd_start_time))
	print("GDScript unsolved cells: %d" % gd_state.unsolved_cells)

	# === Run Native Solver ===
	print("")
	print("Running Native solver...")
	seed(test_seed)

	# Convert rules to native format
	var native_rules = _convert_rules_to_native(rules)

	# Create native problem
	var native_problem = WFC2DProblemNative.new()
	native_problem.initialize(native_rules, rect)

	# Setup preconditions on native problem
	_setup_native_preconditions(native_problem, target, rect, precondition, mapper, rules)

	# Verify native rule matrices
	var native_matrix_hash = 0
	var native_matrices = native_rules.get_axis_matrices()
	for axis_idx in range(native_matrices.size()):
		var matrix = native_matrices[axis_idx]
		for i in range(matrix.get_height()):
			var row = matrix.get_row(i)
			for j in row.iterator():
				native_matrix_hash = (native_matrix_hash * 31 + axis_idx * 1000 + i * 100 + j) % 1000000007
	print("Native rule matrix hash: %d" % native_matrix_hash)

	var native_settings = WFCSolverSettingsNative.new()
	native_settings.set_force_ac3(true)
	native_settings.set_allow_backtracking(false)

	var native_solver = WFCSolverNative.new()
	native_solver.initialize(native_problem, native_settings)

	# Debug: Check first random call from native
	print("DEBUG Native: First debug_randi_range(0, 99) = %d" % native_problem.debug_randi_range(0, 99))

	# Debug: Check state after initialization (before solving)
	var native_init_state = native_solver.get_current_state()
	print("DEBUG Native after init: unsolved=%d, total=%d" % [native_init_state.get_unsolved_cells(), native_init_state.get_cell_solution_or_entropy().size()])

	# Check changed_cells
	var native_changed = native_init_state.get_changed_cells()
	print("DEBUG Native changed_cells count: %d, first 5: %s" % [native_changed.size(), str(Array(native_changed).slice(0, 5))])

	# Check entropy distribution
	var native_entropy_dist = {}
	var native_se = native_init_state.get_cell_solution_or_entropy()
	for i in range(native_se.size()):
		var e = native_se[i]
		if e < 0:  # unsolved
			var entropy = -e
			native_entropy_dist[entropy] = native_entropy_dist.get(entropy, 0) + 1
	print("DEBUG Native entropy distribution: %s" % str(native_entropy_dist))

	# Test pick_random behavior (use native debug method)
	seed(test_seed)
	var native_picks = []
	for i in range(5):
		# Simulate pick_random by using randi_range
		native_picks.append([100, 200, 300, 400, 500][native_problem.debug_randi_range(0, 4)])
	print("DEBUG Native pick_random test: %s" % str(native_picks))

	# Re-seed for actual solve
	seed(test_seed)

	# Track first few divergence decisions
	var native_first_solved = []
	var native_before_entropy = native_solver.get_current_state().get_cell_solution_or_entropy().duplicate()
	for i in range(5):
		native_solver.solve_step()
		var native_after_entropy = native_solver.get_current_state().get_cell_solution_or_entropy()
		for j in range(native_after_entropy.size()):
			if native_before_entropy[j] < 0 and native_after_entropy[j] >= 0:
				native_first_solved.append({"step": i+1, "cell": j, "value": native_after_entropy[j]})
				break
		native_before_entropy = native_after_entropy.duplicate()
		if native_solver.get_current_state().is_all_solved():
			break
	print("DEBUG Native first solved: %s" % str(native_first_solved))

	# Continue solving
	var native_start_time = Time.get_ticks_msec()
	while not native_solver.solve_step():
		pass
	var native_end_time = Time.get_ticks_msec()

	var native_state = native_solver.get_current_state()
	var native_solutions = native_state.get_cell_solution_or_entropy()
	print("Native solver completed in %d ms" % (native_end_time - native_start_time))
	print("Native unsolved cells: %d" % native_state.get_unsolved_cells())

	# === Compare and Output ===
	print("")
	_output_comparison(gd_solutions, native_solutions, rect)

	demo.queue_free()

func _convert_rules_to_native(gd_rules: WFCRules2D) -> WFCRules2DNative:
	var native_rules = WFCRules2DNative.new()
	var axes: Array[Vector2i] = []
	for axis in gd_rules.axes:
		axes.append(axis)
	native_rules.initialize(gd_rules.mapper.size(), axes)

	for axis_idx in range(gd_rules.axis_matrices.size()):
		var matrix = gd_rules.axis_matrices[axis_idx]
		for i in range(matrix.height):
			for j in matrix.rows[i].iterator():
				# Note: matrix.rows[i] contains column indices (j) where bit is set
				# set_rule(axis, tile1, tile2) means tile1 can be next to tile2
				# In the matrix: rows[y].bit(x) means x can follow y along this axis
				# So we need set_rule(axis_idx, j, i, true) - j (column) can follow i (row)
				native_rules.set_rule(axis_idx, j, i, true)

	# IMPORTANT: Copy probabilities!
	native_rules.set_probabilities_enabled(gd_rules.probabilities_enabled)
	if gd_rules.probabilities_enabled:
		native_rules.set_probabilities(gd_rules.probabilities)

	return native_rules

func _setup_native_preconditions(native_problem, target: Node, rect: Rect2i, precondition, mapper, rules: WFCRules2D):
	# Note: GDScript problem does NOT directly read tiles from target - it only reads from
	# init_read_rects (which is empty for non-split problems). All tiles come via precondition.
	for x in range(rect.size.x):
		for y in range(rect.size.y):
			var pos = Vector2i(x, y)
			var abs_pos = rect.position + pos
			var cell_id = rect.size.x * y + x

			# Only use precondition - don't read directly from target (matching GDScript behavior)
			var domain = precondition.read_domain(abs_pos)
			if domain != null:
				var native_domain = WFCBitSetNative.new()
				native_domain.initialize(domain.size, false)
				for bit in domain.iterator():
					native_domain.set_bit(bit, true)
				native_problem.set_precondition_domain(cell_id, native_domain)

func _output_comparison(gd: PackedInt64Array, native: PackedInt64Array, rect: Rect2i):
	var width = rect.size.x
	var height = rect.size.y

	print("=== COMPARISON RESULTS ===")
	print("Grid: %d x %d = %d cells" % [width, height, gd.size()])

	if gd.size() != native.size():
		print("ERROR: Size mismatch! gd=%d native=%d" % [gd.size(), native.size()])
		return

	var differences = []
	var gd_unsolved = 0
	var native_unsolved = 0

	for i in range(gd.size()):
		if gd[i] < 0:
			gd_unsolved += 1
		if native[i] < 0:
			native_unsolved += 1
		if gd[i] != native[i]:
			@warning_ignore("integer_division")
			var x = i % width
			@warning_ignore("integer_division")
			var y = i / width
			differences.append({"cell": i, "x": x, "y": y, "gd": gd[i], "native": native[i]})

	print("GDScript unsolved/failed cells: %d" % gd_unsolved)
	print("Native unsolved/failed cells: %d" % native_unsolved)
	print("")
	print("Different cells: %d (%.2f%%)" % [differences.size(), 100.0 * differences.size() / gd.size()])

	if differences.size() > 0:
		print("")
		print("First 30 differences:")
		for i in range(min(30, differences.size())):
			var d = differences[i]
			print("  Cell %d (%d,%d): gd=%d native=%d" % [d.cell, d.x, d.y, d.gd, d.native])

		# Find first divergence (first cell where they differ)
		var first_diff = differences[0]
		print("")
		print("First divergence at cell %d (x=%d, y=%d)" % [first_diff.cell, first_diff.x, first_diff.y])

		# Analyze divergence pattern
		var clustered = _analyze_clustering(differences, width)
		print("")
		print("Divergence pattern analysis:")
		print("  Clustered around first difference: %s" % ("Yes" if clustered else "No (scattered)"))

	# Write full grids to files
	var user_dir = OS.get_user_data_dir()
	_write_grid_to_file(gd, rect, user_dir + "/gd_output.txt")
	_write_grid_to_file(native, rect, user_dir + "/native_output.txt")
	_write_diff_grid(gd, native, rect, user_dir + "/diff_output.txt")

	print("")
	print("Full grids written to:")
	print("  %s/gd_output.txt" % user_dir)
	print("  %s/native_output.txt" % user_dir)
	print("  %s/diff_output.txt" % user_dir)

func _analyze_clustering(differences: Array, width: int) -> bool:
	if differences.size() < 2:
		return true

	# Check if differences are clustered near the first one
	var first = differences[0]
	var near_first = 0
	var radius = 10

	for d in differences:
		var dx = abs(d.x - first.x)
		var dy = abs(d.y - first.y)
		if dx <= radius and dy <= radius:
			near_first += 1

	# If more than 50% are within radius, consider it clustered
	return float(near_first) / differences.size() > 0.5

func _write_grid_to_file(solutions: PackedInt64Array, rect: Rect2i, path: String):
	var file = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		print("ERROR: Could not open %s for writing" % path)
		return

	var width = rect.size.x
	for y in range(rect.size.y):
		var row = ""
		for x in range(width):
			var cell_id = y * width + x
			var val = solutions[cell_id]
			row += "%3d " % val
		file.store_line(row)
	file.close()

func _write_diff_grid(gd: PackedInt64Array, native: PackedInt64Array, rect: Rect2i, path: String):
	var file = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		print("ERROR: Could not open %s for writing" % path)
		return

	var width = rect.size.x
	for y in range(rect.size.y):
		var row = ""
		for x in range(width):
			var cell_id = y * width + x
			if gd[cell_id] == native[cell_id]:
				row += "  . "
			else:
				row += "  X "
		file.store_line(row)
	file.close()

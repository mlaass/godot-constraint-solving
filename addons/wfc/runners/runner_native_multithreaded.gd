class_name WFCNativeMultithreadedSolverRunner
## A [WFCSolverRunner] that uses the native C++ multithreaded solver implementation.
##
## Splits the problem and runs sub-problems in parallel using native threads.
extends WFCSolverRunner

## Settings for multithreaded runner.
var runner_settings: WFCMultithreadedRunnerSettings = WFCMultithreadedRunnerSettings.new()

var _native_runner = null  # WFCMultithreadedRunnerNative
var _native_problems: Array = []  # Array of WFC2DProblemNative
var _gd_problem: WFC2DProblem = null  # Keep for rendering via mapper
var _interrupted: bool = false
var _started: bool = false
var _needs_final_update: bool = true

## See [member WFCSolverRunner.start].
func start(problem_: WFCProblem):
  assert(not is_started())
  assert(problem_ is WFC2DProblem)

  _gd_problem = problem_ as WFC2DProblem

  # Convert to native
  var native_problem = _convert_problem_to_native(_gd_problem)

  # Split into sub-problems FIRST
  var concurrency = runner_settings.get_max_threads()
  var sub_problems = native_problem.split(concurrency)
  print("start %d threads -> %d problems" % [concurrency,sub_problems.size()])
  # Setup preconditions on EACH sub-problem (after split)
  _native_problems.clear()
  for i in range(sub_problems.size()):
    var sub = sub_problems[i]
    var native_sub_problem = sub.get_problem()
    # Setup preconditions using absolute coordinates for this sub-problem's rect
    _setup_preconditions_for_subproblem(native_sub_problem, _gd_problem)
    _native_problems.append(native_sub_problem)

  # Create native settings
  var native_settings = WFCSolverSettingsNative.new()
  native_settings.set_allow_backtracking(solver_settings.allow_backtracking)
  native_settings.set_force_ac3(solver_settings.force_ac3)

  # Create and start native multithreaded runner
  _native_runner = WFCMultithreadedRunnerNative.new()
  _native_runner.start(sub_problems, native_settings, concurrency)
  _started = true

## See [member WFCSolverRunner.update].
func update():
  if not is_started():
    return
  if _interrupted:
    return

  var done = _native_runner.update()

  if done:
    _needs_final_update = false
    _render_all_to_map()
    sub_problem_solved.emit(_gd_problem, null)
    all_solved.emit()
  else:
    partial_solution.emit(_gd_problem, null)

## See [member WFCSolverRunner.is_running].
func is_running() -> bool:
  if not is_started():
    return false
  if _interrupted:
    return false
  # Check if native runner is still running
  # Note: We need to keep running until we've processed the completion
  var native_running = _native_runner.is_running()
  if not native_running:
    # Runner finished - check if we need one more update to emit signals
    # The update() method will handle the final emission
    return _needs_final_update
  return true

## See [member WFCSolverRunner.is_started].
func is_started() -> bool:
  return _started

## See [member WFCSolverRunner.interrupt].
func interrupt():
  _interrupted = true
  if _native_runner != null:
    _native_runner.interrupt()

## See [member WFCSolverRunner.get_progress].
func get_progress() -> float:
  if not is_started():
    return 0.0
  if not is_running():
    return 1.0
  return _native_runner.get_progress()

func _convert_problem_to_native(gd_problem: WFC2DProblem):
  var gd_rules = gd_problem.rules
  var native_rules = WFCRules2DNative.new()

  var axes: Array[Vector2i] = []
  for axis in gd_rules.axes:
    axes.append(axis)
  native_rules.initialize(gd_rules.mapper.size(), axes)

  for axis_idx in range(gd_rules.axis_matrices.size()):
    var matrix = gd_rules.axis_matrices[axis_idx]
    for i in range(matrix.height):
      for j in matrix.rows[i].iterator():
        # Matrix stores rows[y].bit(x) meaning "x can follow y"
        # So we need set_rule(axis_idx, j, i, true) - j (column) can follow i (row)
        native_rules.set_rule(axis_idx, j, i, true)

  # Copy probabilities for weighted selection
  native_rules.set_probabilities_enabled(gd_rules.probabilities_enabled)
  if gd_rules.probabilities_enabled:
    native_rules.set_probabilities(gd_rules.probabilities)

  var native_problem = WFC2DProblemNative.new()
  native_problem.initialize(native_rules, gd_problem.rect)
  return native_problem

func _setup_preconditions_for_subproblem(native_problem, gd_problem: WFC2DProblem):
  # Get this sub-problem's rect (may be different from gd_problem.rect)
  var rect = native_problem.get_rect()
  var precondition = gd_problem.precondition
  if precondition == null:
    return

  for x in range(rect.size.x):
    for y in range(rect.size.y):
      # Use ABSOLUTE coordinates for precondition query
      var abs_pos = rect.position + Vector2i(x, y)
      var cell_id = rect.size.x * y + x

      # Apply precondition domain using absolute coordinates
      var domain = precondition.read_domain(abs_pos)
      if domain != null:
        # Convert WFCBitSet to native format
        var native_domain = WFCBitSetNative.new()
        native_domain.initialize(domain.size, false)
        for bit in domain.iterator():
          native_domain.set_bit(bit, true)
        native_problem.set_precondition_domain(cell_id, native_domain)

func _render_all_to_map():
  var mapper = _gd_problem.rules.mapper
  var main_rect = _gd_problem.rect

  # Render each sub-problem's results
  for native_problem in _native_problems:
    var sub_rect = native_problem.get_renderable_rect()

    # Get solver state for this sub-problem - we need to get it from the runner
    # For now, we'll read the solutions directly from each problem's solver
    # Note: The multithreaded runner stores completed states internally

    # Actually, we need to access the task snapshots from the runner
    # But since all tasks are done, let's query each task's final state
    pass

  # Alternative approach: The native runner tracks all tasks,
  # but we need a way to get their final states
  # For now, let's request snapshots and render from those
  _native_runner.request_snapshots()

  for i in range(_native_runner.get_task_count()):
    var snapshot = _native_runner.get_task_snapshot(i)
    if snapshot == null:
      continue

    var native_problem = _native_problems[i]
    var sub_rect = native_problem.get_rect()
    var renderable_rect = native_problem.get_renderable_rect()
    var solutions = snapshot.get_cell_solution_or_entropy()

    # Render only within renderable_rect to avoid duplicates at boundaries
    for cell_id in range(solutions.size()):
      var solution = solutions[cell_id]
      if solution >= 0:
        @warning_ignore("integer_division")
        var local_coord = Vector2i(cell_id % sub_rect.size.x, cell_id / sub_rect.size.x)
        var abs_coord = local_coord + sub_rect.position

        # Only render if within renderable rect
        if renderable_rect.has_point(abs_coord):
          mapper.write_cell(_gd_problem.map, abs_coord, solution)

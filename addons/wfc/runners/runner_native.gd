class_name WFCNativeSolverRunner
## A [WFCSolverRunner] that uses the native C++ solver implementation.
##
## Runs on main thread with incremental progress using [code]solve_step()[/code].
extends WFCSolverRunner

## Settings for runner timing.
var runner_settings: WFCMainThreadRunnerSettings = WFCMainThreadRunnerSettings.new()

var _native_problem = null  # WFC2DProblemNative
var _native_solver = null   # WFCSolverNative
var _gd_problem: WFC2DProblem = null  # Keep for rendering via mapper
var _interrupted: bool = false

## See [member WFCSolverRunner.start].
func start(problem_: WFCProblem):
  assert(not is_started())
  assert(problem_ is WFC2DProblem)

  _gd_problem = problem_ as WFC2DProblem

  # Convert to native
  _native_problem = _convert_problem_to_native(_gd_problem)

  # Setup preconditions on native problem BEFORE solver init
  # (solver's initialize() calls populate_initial_state() which applies these)
  _setup_preconditions(_native_problem, _gd_problem)

  var native_settings = WFCSolverSettingsNative.new()
  native_settings.set_allow_backtracking(solver_settings.allow_backtracking)
  native_settings.set_force_ac3(solver_settings.force_ac3)

  _native_solver = WFCSolverNative.new()
  _native_solver.initialize(_native_problem, native_settings)

## See [member WFCSolverRunner.update].
func update():
  assert(is_running())

  var start_ticks: int = Time.get_ticks_msec()
  var state = _native_solver.get_current_state()

  while state.get_unsolved_cells() > 0:
    var done = _native_solver.solve_step()

    state = _native_solver.get_current_state()
    if done or state.get_unsolved_cells() == 0:
      _render_to_map()
      sub_problem_solved.emit(_gd_problem, null)
      all_solved.emit()
      return

    if (Time.get_ticks_msec() - start_ticks) >= runner_settings.max_ms_per_frame:
      break

  partial_solution.emit(_gd_problem, null)

## See [member WFCSolverRunner.is_running].
func is_running() -> bool:
  if not is_started():
    return false
  if _interrupted:
    return false
  var state = _native_solver.get_current_state()
  return state != null and state.get_unsolved_cells() > 0

## See [member WFCSolverRunner.is_started].
func is_started() -> bool:
  return _native_solver != null

## See [member WFCSolverRunner.interrupt].
func interrupt():
  _interrupted = true

## See [member WFCSolverRunner.get_progress].
func get_progress() -> float:
  if not is_started():
    return 0.0
  if not is_running():
    return 1.0
  var state = _native_solver.get_current_state()
  return 1.0 - (float(state.get_unsolved_cells()) / float(_native_problem.get_cell_count()))

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
        # Note: matrix.rows[i] contains column indices (j) where bit is set
        # set_rule(axis, tile1, tile2) stores rows[tile2].set_bit(tile1)
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

func _setup_preconditions(native_problem, gd_problem: WFC2DProblem):
  # Setup preconditions on native problem BEFORE solver init
  # The solver's initialize() will call populate_initial_state() which applies these
  var rect = gd_problem.rect
  var precondition = gd_problem.precondition

  for x in range(rect.size.x):
    for y in range(rect.size.y):
      var pos = Vector2i(x, y)
      var abs_pos = rect.position + pos
      var cell_id = rect.size.x * y + x

      # Check for existing tiles from init_read_rects
      var from_map = gd_problem._read_from_target(abs_pos)
      if from_map >= 0:
        native_problem.set_precondition_solution(cell_id, from_map)
      else:
        # Apply precondition domain
        var domain = precondition.read_domain(abs_pos)
        if domain != null:
          # Convert WFCBitSet to native format
          var native_domain = WFCBitSetNative.new()
          native_domain.initialize(domain.size, false)
          for bit in domain.iterator():
            native_domain.set_bit(bit, true)
          native_problem.set_precondition_domain(cell_id, native_domain)

func _render_to_map():
  var state = _native_solver.get_current_state()
  var solutions = state.get_cell_solution_or_entropy()
  var mapper = _gd_problem.rules.mapper
  var rect = _gd_problem.rect

  for cell_id in range(solutions.size()):
    var solution = solutions[cell_id]
    if solution >= 0:
      @warning_ignore("integer_division")
      var coord = Vector2i(cell_id % rect.size.x, cell_id / rect.size.x)
      var map_coord = coord + rect.position
      mapper.write_cell(_gd_problem.map, map_coord, solution)

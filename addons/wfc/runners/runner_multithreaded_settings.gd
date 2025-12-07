extends Resource
## Settings for [code]WFCMultithreadedSolverRunner[/code].
##
## See [WFCMultithreadedSolverRunner].
class_name WFCMultithreadedRunnerSettings

## Maximum number of threads to use.
## [br]
## When set to non-positive value (default) the number of threads will be chosen based on number of
## available CPU cores (processor_count - 1).
## [br]
## When set to [code]1[/code], the runner will always run a single solver in single [Thread].
@export
var max_threads: int = -1

## Calculates actual number of allowed threads.
func get_max_threads() -> int:
	if max_threads < 1:
		return max(OS.get_processor_count() - 1, 1)
	return max_threads

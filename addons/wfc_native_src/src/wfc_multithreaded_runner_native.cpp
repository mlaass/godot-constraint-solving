#include "wfc_multithreaded_runner_native.h"
#include "wfc_2d_problem_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

namespace godot {

// WFCMultithreadedRunnerNative implementation

void WFCMultithreadedRunnerNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start", "sub_problems", "settings", "max_threads"), &WFCMultithreadedRunnerNative::start, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("update"), &WFCMultithreadedRunnerNative::update);
    ClassDB::bind_method(D_METHOD("interrupt"), &WFCMultithreadedRunnerNative::interrupt);
    ClassDB::bind_method(D_METHOD("get_progress"), &WFCMultithreadedRunnerNative::get_progress);
    ClassDB::bind_method(D_METHOD("is_running"), &WFCMultithreadedRunnerNative::is_running);
    ClassDB::bind_method(D_METHOD("is_started"), &WFCMultithreadedRunnerNative::is_started);
    ClassDB::bind_method(D_METHOD("get_task_snapshot", "task_index"), &WFCMultithreadedRunnerNative::get_task_snapshot);
    ClassDB::bind_method(D_METHOD("request_snapshots"), &WFCMultithreadedRunnerNative::request_snapshots);
    ClassDB::bind_method(D_METHOD("get_task_count"), &WFCMultithreadedRunnerNative::get_task_count);

    ClassDB::bind_method(D_METHOD("get_max_threads"), &WFCMultithreadedRunnerNative::get_max_threads);
    ClassDB::bind_method(D_METHOD("set_max_threads", "val"), &WFCMultithreadedRunnerNative::set_max_threads);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_threads"), "set_max_threads", "get_max_threads");
}

WFCMultithreadedRunnerNative::WFCMultithreadedRunnerNative() {
    // Default to number of hardware threads minus 1, capped at 4
    int hw_threads = std::thread::hardware_concurrency();
    max_threads_ = std::min(std::max(hw_threads - 1, 1), 4);
}

WFCMultithreadedRunnerNative::~WFCMultithreadedRunnerNative() {
    interrupt();
}

void WFCMultithreadedRunnerNative::thread_main(int task_index) {
    Task* task = tasks_[task_index].get();

    // Create solver for this task
    task->solver.instantiate();
    task->solver->initialize(task->problem, task->settings);

    Ref<WFCSolverStateNative> state = task->solver->get_current_state();

    // Solver loop
    while (!interrupted_.load() && state->get_unsolved_cells() > 0) {
        bool done = task->solver->solve_step();

        state = task->solver->get_current_state();
        task->unsolved_cells.store(state->get_unsolved_cells());

        // Handle snapshot requests
        if (task->snapshot_requested.load()) {
            std::lock_guard<std::mutex> lock(task->snapshot_mutex);
            task->state_snapshot = state->make_snapshot();
            task->snapshot_requested.store(false);
        }

        if (done || state->get_unsolved_cells() == 0) {
            break;
        }
    }

    // Store final state snapshot before unlinking
    {
        std::lock_guard<std::mutex> lock(task->snapshot_mutex);
        task->state_snapshot = state->make_snapshot();
    }

    // Free backtracking history
    state->unlink_from_previous();

    task->completed.store(true);
}

bool WFCMultithreadedRunnerNative::is_task_blocked(int task_index) const {
    const Task* task = tasks_[task_index].get();

    for (int i = 0; i < task->dependencies.size(); i++) {
        int dep_index = task->dependencies[i];
        if (dep_index >= 0 && dep_index < static_cast<int>(tasks_.size())) {
            if (!tasks_[dep_index]->completed.load()) {
                return true;
            }
        }
    }

    return false;
}

void WFCMultithreadedRunnerNative::copy_boundary_solutions(
    WFC2DProblemNative* target_problem,
    WFC2DProblemNative* source_problem,
    const Ref<WFCSolverStateNative>& source_state,
    const Rect2i& read_rect) {

    if (!read_rect.has_area()) return;

    PackedInt64Array solutions = source_state->get_cell_solution_or_entropy();
    Rect2i source_rect = source_problem->get_rect();
    Rect2i target_rect = target_problem->get_rect();

    // Copy solutions within the read_rect
    for (int x = read_rect.position.x; x < read_rect.position.x + read_rect.size.x; x++) {
        for (int y = read_rect.position.y; y < read_rect.position.y + read_rect.size.y; y++) {
            Vector2i pos(x, y);

            // Check if this point is within source problem's rect
            if (!source_rect.has_point(pos)) continue;

            // Convert absolute coord to source cell_id
            int source_local_x = x - source_rect.position.x;
            int source_local_y = y - source_rect.position.y;
            int source_cell_id = source_local_y * source_rect.size.x + source_local_x;

            if (source_cell_id < 0 || source_cell_id >= solutions.size()) continue;

            int solution = solutions[source_cell_id];
            if (solution >= 0) {  // Has a valid solution
                // Check if point is within target rect
                if (!target_rect.has_point(pos)) continue;

                // Convert to target cell_id
                int target_local_x = x - target_rect.position.x;
                int target_local_y = y - target_rect.position.y;
                int target_cell_id = target_local_y * target_rect.size.x + target_local_x;

                target_problem->set_precondition_solution(target_cell_id, solution);
            }
        }
    }
}

int WFCMultithreadedRunnerNative::start_available_tasks() {
    int started = 0;
    int running_count = 0;

    // Count currently running tasks
    for (const auto& task : tasks_) {
        if (task->started.load() && !task->completed.load()) {
            running_count++;
        }
    }

    // Start tasks up to max_threads
    for (size_t i = 0; i < tasks_.size(); i++) {
        if (running_count >= max_threads_) {
            break;
        }

        Task* task = tasks_[i].get();

        if (!task->started.load() && !is_task_blocked(static_cast<int>(i))) {
            // Copy boundary solutions from completed dependencies before starting
            WFC2DProblemNative* problem_2d = Object::cast_to<WFC2DProblemNative>(task->problem.ptr());
            if (problem_2d) {
                TypedArray<Rect2i> read_rects = problem_2d->get_init_read_rects();

                for (int j = 0; j < task->dependencies.size(); j++) {
                    int dep_index = task->dependencies[j];
                    if (dep_index >= 0 && dep_index < static_cast<int>(tasks_.size())) {
                        Task* dep_task = tasks_[dep_index].get();
                        WFC2DProblemNative* dep_problem = Object::cast_to<WFC2DProblemNative>(dep_task->problem.ptr());

                        std::lock_guard<std::mutex> lock(dep_task->snapshot_mutex);
                        if (dep_problem && dep_task->state_snapshot.is_valid() && j < read_rects.size()) {
                            // read_rects[j] corresponds to dependencies[j]
                            Rect2i read_rect = read_rects[j];
                            copy_boundary_solutions(problem_2d, dep_problem, dep_task->state_snapshot, read_rect);
                        }
                    }
                }
            }

            task->started.store(true);
            task->thread = std::make_unique<std::thread>(&WFCMultithreadedRunnerNative::thread_main, this, static_cast<int>(i));
            started++;
            running_count++;
        }
    }

    return started;
}

void WFCMultithreadedRunnerNative::start(const TypedArray<WFCProblemSubProblemNative>& sub_problems,
                                          const Ref<WFCSolverSettingsNative>& settings,
                                          int max_threads) {
    // Interrupt any existing tasks
    interrupt();

    // Reset state
    tasks_.clear();
    interrupted_.store(false);
    all_done_.store(false);

    if (max_threads > 0) {
        max_threads_ = max_threads;
    }

    solver_settings_ = settings;
    if (solver_settings_.is_null()) {
        solver_settings_.instantiate();
    }

    // Create tasks from sub-problems
    for (int i = 0; i < sub_problems.size(); i++) {
        Ref<WFCProblemSubProblemNative> sub_problem = sub_problems[i];
        if (sub_problem.is_null()) continue;

        auto task = std::make_unique<Task>();
        task->problem = sub_problem->get_problem();
        task->dependencies = sub_problem->get_dependencies();
        task->settings = solver_settings_;
        task->started.store(false);
        task->completed.store(false);
        task->unsolved_cells.store(task->problem.is_valid() ? task->problem->get_cell_count() : 0);

        tasks_.push_back(std::move(task));
    }

    // Start initial batch of tasks
    start_available_tasks();
}

bool WFCMultithreadedRunnerNative::update() {
    if (tasks_.empty()) {
        return true;
    }

    // Check for completed tasks and join their threads
    bool any_just_completed = false;
    for (auto& task : tasks_) {
        if (task->started.load() && task->completed.load() && task->thread && task->thread->joinable()) {
            task->thread->join();
            task->thread.reset();
            any_just_completed = true;
        }
    }

    // If any tasks just completed, try to start more
    if (any_just_completed) {
        start_available_tasks();
    }

    // Check if all tasks are complete
    bool all_complete = true;
    for (const auto& task : tasks_) {
        if (!task->completed.load()) {
            all_complete = false;
            break;
        }
    }

    all_done_.store(all_complete);
    return all_complete;
}

void WFCMultithreadedRunnerNative::interrupt() {
    interrupted_.store(true);

    // Wait for all threads to finish
    for (auto& task : tasks_) {
        if (task->thread && task->thread->joinable()) {
            task->thread->join();
            task->thread.reset();
        }
    }
}

float WFCMultithreadedRunnerNative::get_progress() const {
    if (tasks_.empty()) {
        return 1.0f;
    }

    int total_cells = 0;
    int unsolved_cells = 0;

    for (const auto& task : tasks_) {
        if (task->problem.is_valid()) {
            total_cells += task->problem->get_cell_count();
            unsolved_cells += task->unsolved_cells.load();
        }
    }

    if (total_cells == 0) {
        return 1.0f;
    }

    return 1.0f - (static_cast<float>(unsolved_cells) / static_cast<float>(total_cells));
}

bool WFCMultithreadedRunnerNative::is_running() const {
    if (tasks_.empty()) {
        return false;
    }

    for (const auto& task : tasks_) {
        if (task->started.load() && !task->completed.load()) {
            return true;
        }
    }

    return false;
}

bool WFCMultithreadedRunnerNative::is_started() const {
    return !tasks_.empty();
}

Ref<WFCSolverStateNative> WFCMultithreadedRunnerNative::get_task_snapshot(int task_index) {
    if (task_index < 0 || task_index >= static_cast<int>(tasks_.size())) {
        return Ref<WFCSolverStateNative>();
    }

    Task* task = tasks_[task_index].get();
    std::lock_guard<std::mutex> lock(task->snapshot_mutex);
    return task->state_snapshot;
}

void WFCMultithreadedRunnerNative::request_snapshots() {
    for (auto& task : tasks_) {
        if (task->started.load() && !task->completed.load()) {
            task->snapshot_requested.store(true);
        }
    }
}

} // namespace godot

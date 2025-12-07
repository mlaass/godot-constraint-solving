#ifndef WFC_MULTITHREADED_RUNNER_NATIVE_H
#define WFC_MULTITHREADED_RUNNER_NATIVE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include "wfc_problem_native.h"
#include "wfc_solver_native.h"
#include "wfc_solver_state_native.h"
#include "wfc_solver_settings_native.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

namespace godot {

// Forward declaration
class WFC2DProblemNative;

// Multithreaded runner using std::thread
class WFCMultithreadedRunnerNative : public RefCounted {
    GDCLASS(WFCMultithreadedRunnerNative, RefCounted)

private:
    // Task structure for thread management
    struct Task {
        Ref<WFCProblemNative> problem;
        Ref<WFCSolverNative> solver;
        Ref<WFCSolverSettingsNative> settings;
        PackedInt64Array dependencies;

        std::unique_ptr<std::thread> thread;
        std::atomic<bool> started{false};
        std::atomic<bool> completed{false};
        std::atomic<int> unsolved_cells{0};

        // Thread-safe state snapshot
        std::mutex snapshot_mutex;
        Ref<WFCSolverStateNative> state_snapshot;
        std::atomic<bool> snapshot_requested{false};

        Task() = default;
        ~Task() = default;

        // Non-copyable
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        // Non-movable (due to mutex and atomics)
        Task(Task&&) = delete;
        Task& operator=(Task&&) = delete;
    };

    // Use unique_ptr since Task is non-copyable/movable
    std::vector<std::unique_ptr<Task>> tasks_;
    std::atomic<bool> interrupted_{false};
    std::atomic<bool> all_done_{false};
    int max_threads_ = 4;
    Ref<WFCSolverSettingsNative> solver_settings_;

    // Thread main function
    void thread_main(int task_index);

    // Check if task dependencies are satisfied
    bool is_task_blocked(int task_index) const;

    // Start tasks that can run
    int start_available_tasks();

    // Copy boundary solutions from source to target problem's preconditions
    void copy_boundary_solutions(
        WFC2DProblemNative* target_problem,
        WFC2DProblemNative* source_problem,
        const Ref<WFCSolverStateNative>& source_state,
        const Rect2i& read_rect);

protected:
    static void _bind_methods();

public:
    WFCMultithreadedRunnerNative();
    ~WFCMultithreadedRunnerNative();

    // Initialize with sub-problems (accepts result from WFC2DProblemNative::split())
    void start(const TypedArray<WFCProblemSubProblemNative>& sub_problems,
               const Ref<WFCSolverSettingsNative>& settings,
               int max_threads = 0);

    // Update - call each frame to check progress and start new tasks
    // Returns true when all tasks are complete
    bool update();

    // Interrupt all running tasks
    void interrupt();

    // Get overall progress (0.0 to 1.0)
    float get_progress() const;

    // Check if running
    bool is_running() const;
    bool is_started() const;

    // Get state snapshot for a specific task (thread-safe)
    Ref<WFCSolverStateNative> get_task_snapshot(int task_index);

    // Request snapshots from all running tasks
    void request_snapshots();

    // Get number of tasks
    int get_task_count() const { return static_cast<int>(tasks_.size()); }

    // Settings
    int get_max_threads() const { return max_threads_; }
    void set_max_threads(int val) { max_threads_ = val; }
};

} // namespace godot

#endif // WFC_MULTITHREADED_RUNNER_NATIVE_H

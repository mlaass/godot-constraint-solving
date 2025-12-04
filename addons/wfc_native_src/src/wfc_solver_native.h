#ifndef WFC_SOLVER_NATIVE_H
#define WFC_SOLVER_NATIVE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "wfc_solver_settings_native.h"
#include "wfc_solver_state_native.h"
#include "wfc_problem_native.h"

namespace godot {

class WFCSolverNative : public RefCounted {
    GDCLASS(WFCSolverNative, RefCounted)

private:
    Ref<WFCSolverSettingsNative> settings_;
    Ref<WFCProblemNative> problem_;
    bool backtracking_enabled_ = true;
    int backtracking_count_ = 0;
    bool ac4_enabled_ = false;
    TypedArray<WFCProblemAC4BinaryConstraintNative> ac4_constraints_;
    Ref<WFCSolverStateNative> current_state_;
    Ref<WFCSolverStateNative> best_state_;

    Ref<WFCSolverStateNative> make_initial_state(int num_cells, const Ref<WFCBitSetNative>& initial_domain);
    bool propagate_constraints_ac3();
    bool propagate_constraints_ac4();
    bool propagate_constraints();
    void continue_without_backtracking();
    bool try_backtrack();
    bool should_keep_previous_state(const Ref<WFCSolverStateNative>& state) const;

protected:
    static void _bind_methods();

public:
    WFCSolverNative();
    ~WFCSolverNative();

    void initialize(const Ref<WFCProblemNative>& problem, const Ref<WFCSolverSettingsNative>& settings = Ref<WFCSolverSettingsNative>());

    // Property accessors
    Ref<WFCSolverSettingsNative> get_settings() const { return settings_; }
    void set_settings(const Ref<WFCSolverSettingsNative>& val) { settings_ = val; }

    Ref<WFCProblemNative> get_problem() const { return problem_; }
    void set_problem(const Ref<WFCProblemNative>& val) { problem_ = val; }

    bool get_backtracking_enabled() const { return backtracking_enabled_; }
    void set_backtracking_enabled(bool val) { backtracking_enabled_ = val; }

    int get_backtracking_count() const { return backtracking_count_; }
    void set_backtracking_count(int val) { backtracking_count_ = val; }

    bool get_ac4_enabled() const { return ac4_enabled_; }
    void set_ac4_enabled(bool val) { ac4_enabled_ = val; }

    Ref<WFCSolverStateNative> get_current_state() const { return current_state_; }
    void set_current_state(const Ref<WFCSolverStateNative>& val) { current_state_ = val; }

    Ref<WFCSolverStateNative> get_best_state() const { return best_state_; }
    void set_best_state(const Ref<WFCSolverStateNative>& val) { best_state_ = val; }

    // Methods
    bool solve_step();
    Ref<WFCSolverStateNative> solve();
};

} // namespace godot

#endif // WFC_SOLVER_NATIVE_H

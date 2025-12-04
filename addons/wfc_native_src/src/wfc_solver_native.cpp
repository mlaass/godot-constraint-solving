#include "wfc_solver_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void WFCSolverNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "problem", "settings"), &WFCSolverNative::initialize, DEFVAL(Ref<WFCSolverSettingsNative>()));

    ClassDB::bind_method(D_METHOD("get_settings"), &WFCSolverNative::get_settings);
    ClassDB::bind_method(D_METHOD("set_settings", "val"), &WFCSolverNative::set_settings);

    ClassDB::bind_method(D_METHOD("get_problem"), &WFCSolverNative::get_problem);
    ClassDB::bind_method(D_METHOD("set_problem", "val"), &WFCSolverNative::set_problem);

    ClassDB::bind_method(D_METHOD("get_backtracking_enabled"), &WFCSolverNative::get_backtracking_enabled);
    ClassDB::bind_method(D_METHOD("set_backtracking_enabled", "val"), &WFCSolverNative::set_backtracking_enabled);

    ClassDB::bind_method(D_METHOD("get_backtracking_count"), &WFCSolverNative::get_backtracking_count);
    ClassDB::bind_method(D_METHOD("set_backtracking_count", "val"), &WFCSolverNative::set_backtracking_count);

    ClassDB::bind_method(D_METHOD("get_ac4_enabled"), &WFCSolverNative::get_ac4_enabled);
    ClassDB::bind_method(D_METHOD("set_ac4_enabled", "val"), &WFCSolverNative::set_ac4_enabled);

    ClassDB::bind_method(D_METHOD("get_current_state"), &WFCSolverNative::get_current_state);
    ClassDB::bind_method(D_METHOD("set_current_state", "val"), &WFCSolverNative::set_current_state);

    ClassDB::bind_method(D_METHOD("get_best_state"), &WFCSolverNative::get_best_state);
    ClassDB::bind_method(D_METHOD("set_best_state", "val"), &WFCSolverNative::set_best_state);

    ClassDB::bind_method(D_METHOD("solve_step"), &WFCSolverNative::solve_step);
    ClassDB::bind_method(D_METHOD("solve"), &WFCSolverNative::solve);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "settings", PROPERTY_HINT_RESOURCE_TYPE, "WFCSolverSettingsNative"), "set_settings", "get_settings");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "problem", PROPERTY_HINT_RESOURCE_TYPE, "WFCProblemNative"), "set_problem", "get_problem");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "backtracking_enabled"), "set_backtracking_enabled", "get_backtracking_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "backtracking_count"), "set_backtracking_count", "get_backtracking_count");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ac4_enabled"), "set_ac4_enabled", "get_ac4_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "current_state", PROPERTY_HINT_RESOURCE_TYPE, "WFCSolverStateNative"), "set_current_state", "get_current_state");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "best_state", PROPERTY_HINT_RESOURCE_TYPE, "WFCSolverStateNative"), "set_best_state", "get_best_state");
}

WFCSolverNative::WFCSolverNative() {
}

WFCSolverNative::~WFCSolverNative() {
}

Ref<WFCSolverStateNative> WFCSolverNative::make_initial_state(int num_cells, const Ref<WFCBitSetNative>& initial_domain) {
    Ref<WFCSolverStateNative> state;
    state.instantiate();

    TypedArray<WFCBitSetNative> cell_domains;
    cell_domains.resize(num_cells);
    for (int i = 0; i < num_cells; i++) {
        cell_domains[i] = initial_domain;
    }
    state->set_cell_domains(cell_domains);

    int64_t entropy = -(initial_domain->count_set_bits() - 1);
    PackedInt64Array solution_or_entropy;
    solution_or_entropy.resize(num_cells);
    for (int i = 0; i < num_cells; i++) {
        solution_or_entropy[i] = entropy;
    }
    state->set_cell_solution_or_entropy(solution_or_entropy);

    state->set_unsolved_cells(num_cells);
    state->set_observations_count(0);

    return state;
}

void WFCSolverNative::initialize(const Ref<WFCProblemNative>& problem, const Ref<WFCSolverSettingsNative>& settings) {
    if (settings.is_valid()) {
        settings_ = settings;
    } else {
        settings_.instantiate();
    }

    backtracking_enabled_ = settings_->get_allow_backtracking();
    problem_ = problem;
    ac4_enabled_ = (!settings_->get_force_ac3()) && problem_->supports_ac4();

    current_state_ = make_initial_state(
        problem_->get_cell_count(),
        problem_->get_default_domain()
    );
    best_state_ = current_state_;

    if (ac4_enabled_) {
        ac4_constraints_ = problem_->get_ac4_binary_constraints();
    }

    problem_->populate_initial_state(current_state_);
}

bool WFCSolverNative::propagate_constraints_ac3() {
    while (true) {
        PackedInt64Array changed = current_state_->extract_changed_cells();

        if (changed.is_empty()) {
            return false;
        }

        Dictionary related;

        // Use get_related_cells to find neighbors of changed cells
        for (int i = 0; i < changed.size(); i++) {
            int cell_id = changed[i];

            PackedInt64Array related_cells = problem_->get_related_cells(cell_id);

            for (int j = 0; j < related_cells.size(); j++) {
                int related_cell_id = related_cells[j];
                if (!current_state_->is_cell_solved(related_cell_id)) {
                    related[related_cell_id] = true;
                }
            }
        }

        Array related_keys = related.keys();
        for (int i = 0; i < related_keys.size(); i++) {
            int related_cell_id = related_keys[i];

            Ref<WFCBitSetNative> new_domain = problem_->compute_cell_domain(
                current_state_, related_cell_id
            );

            bool should_backtrack = current_state_->set_domain(
                related_cell_id,
                new_domain
            );

            if (should_backtrack && backtracking_enabled_) {
                return true;
            }
        }
    }

    return false;
}

bool WFCSolverNative::propagate_constraints_ac4() {
    Ref<WFCSolverStateNative> state = current_state_;

    state->ensure_ac4_state(problem_, ac4_constraints_);

    while (true) {
        PackedInt64Array changed_cells = state->extract_changed_cells();
        if (changed_cells.is_empty()) {
            return false;
        }

        TypedArray<WFCBitSetNative> cell_domains = state->get_cell_domains();
        TypedArray<WFCBitSetNative> ac4_acknowledged_domains = state->get_ac4_acknowledged_domains();

        for (int c = 0; c < changed_cells.size(); c++) {
            int cell_id = changed_cells[c];
            Ref<WFCBitSetNative> new_domain = cell_domains[cell_id];
            Ref<WFCBitSetNative> prev_acknowledged_domain = ac4_acknowledged_domains[cell_id];

            if (new_domain->equals(prev_acknowledged_domain)) {
                continue;
            }

            ac4_acknowledged_domains[cell_id] = new_domain;
            state->set_ac4_acknowledged_domains(ac4_acknowledged_domains);

            Ref<WFCBitSetNative> delta_bitset = new_domain->xor_with(prev_acknowledged_domain);
            PackedInt64Array delta = delta_bitset->to_array();

            for (int constraint_id = 0; constraint_id < ac4_constraints_.size(); constraint_id++) {
                Ref<WFCProblemAC4BinaryConstraintNative> constraint = ac4_constraints_[constraint_id];
                int dependent_cell = constraint->get_dependent(cell_id);
                if (dependent_cell < 0) {
                    continue;
                }

                Ref<WFCBitSetNative> dependent_domain = cell_domains[dependent_cell];
                bool dependent_domain_changed = false;

                for (int d = 0; d < delta.size(); d++) {
                    int this_removed = delta[d];
                    PackedInt64Array allowed = constraint->get_allowed(this_removed);

                    for (int a = 0; a < allowed.size(); a++) {
                        int dependent_removed = allowed[a];

                        if (state->decrement_ac4_counter(dependent_cell, constraint_id, dependent_removed)) {
                            if (dependent_domain->get_bit(dependent_removed)) {
                                if (!dependent_domain_changed) {
                                    dependent_domain = dependent_domain->copy();
                                    dependent_domain_changed = true;
                                }
                                dependent_domain->set_bit(dependent_removed, false);
                            }
                        }
                    }
                }

                if (dependent_domain_changed) {
                    if (dependent_domain->is_empty()) {
                        if (backtracking_enabled_) {
                            return true;
                        }
                        // TODO: Handle contradiction in non-backtracking mode
                    }

                    state->set_domain(dependent_cell, dependent_domain);

                    // Refresh cell_domains since state may have modified it
                    cell_domains = state->get_cell_domains();
                }
            }
        }
    }

    return false;
}

bool WFCSolverNative::propagate_constraints() {
    if (ac4_enabled_) {
        return propagate_constraints_ac4();
    } else {
        return propagate_constraints_ac3();
    }
}

void WFCSolverNative::continue_without_backtracking() {
    current_state_ = best_state_;
    backtracking_enabled_ = false;
    current_state_->unlink_from_previous();
}

bool WFCSolverNative::try_backtrack() {
    if (settings_->get_backtracking_limit() > 0 && backtracking_count_ > settings_->get_backtracking_limit()) {
        // Debug: Backtracking limit exceeded
        continue_without_backtracking();
        return false;
    }

    current_state_ = current_state_->backtrack(problem_);

    if (current_state_.is_null()) {
        if (!settings_->get_require_backtracking()) {
            // Debug: Restarting from best state without backtracking
            continue_without_backtracking();
        } else {
            // Debug: Backtracking is required but failed - terminating with failure
            return true;
        }
    }

    backtracking_count_ += 1;
    return false;
}

bool WFCSolverNative::should_keep_previous_state(const Ref<WFCSolverStateNative>& state) const {
    if (!backtracking_enabled_) {
        return false;
    }

    if (!settings_->is_sparse_history_enabled()) {
        return true;
    }

    int observations_count = state->get_observations_count();

    if (observations_count < settings_->get_sparse_history_start()) {
        return true;
    }

    if ((observations_count - settings_->get_sparse_history_start()) % settings_->get_sparse_history_interval() == 0) {
        return true;
    }

    return false;
}

bool WFCSolverNative::solve_step() {
    if (current_state_->is_all_solved()) {
        return true;
    }

    bool backtrack = propagate_constraints();

    if (backtrack) {
        return try_backtrack();
    }

    if (current_state_->is_all_solved()) {
        return true;
    } else if (current_state_->get_unsolved_cells() < best_state_->get_unsolved_cells()) {
        best_state_ = current_state_;
    }

    current_state_->prepare_divergence();

    if (should_keep_previous_state(current_state_)) {
        Ref<WFCSolverStateNative> next_state = current_state_->diverge(problem_);

        if (next_state.is_null()) {
            return try_backtrack();
        } else {
            current_state_ = next_state;
        }
    } else {
        current_state_->diverge_in_place(problem_);
    }

    return false;
}

Ref<WFCSolverStateNative> WFCSolverNative::solve() {
    while (!solve_step()) {
        // Continue solving
    }

    return current_state_;
}

} // namespace godot

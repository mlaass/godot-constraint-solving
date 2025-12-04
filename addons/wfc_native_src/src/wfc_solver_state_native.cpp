#include "wfc_solver_state_native.h"
#include "wfc_problem_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void WFCSolverStateNative::_bind_methods() {
    // Property accessors
    ClassDB::bind_method(D_METHOD("get_previous"), &WFCSolverStateNative::get_previous);
    ClassDB::bind_method(D_METHOD("set_previous", "val"), &WFCSolverStateNative::set_previous);

    ClassDB::bind_method(D_METHOD("get_cell_domains"), &WFCSolverStateNative::get_cell_domains);
    ClassDB::bind_method(D_METHOD("set_cell_domains", "val"), &WFCSolverStateNative::set_cell_domains);

    ClassDB::bind_method(D_METHOD("get_cell_solution_or_entropy"), &WFCSolverStateNative::get_cell_solution_or_entropy);
    ClassDB::bind_method(D_METHOD("set_cell_solution_or_entropy", "val"), &WFCSolverStateNative::set_cell_solution_or_entropy);

    ClassDB::bind_method(D_METHOD("get_unsolved_cells"), &WFCSolverStateNative::get_unsolved_cells);
    ClassDB::bind_method(D_METHOD("set_unsolved_cells", "val"), &WFCSolverStateNative::set_unsolved_cells);

    ClassDB::bind_method(D_METHOD("get_observations_count"), &WFCSolverStateNative::get_observations_count);
    ClassDB::bind_method(D_METHOD("set_observations_count", "val"), &WFCSolverStateNative::set_observations_count);

    ClassDB::bind_method(D_METHOD("get_changed_cells"), &WFCSolverStateNative::get_changed_cells);
    ClassDB::bind_method(D_METHOD("set_changed_cells", "val"), &WFCSolverStateNative::set_changed_cells);

    ClassDB::bind_method(D_METHOD("get_divergence_cell"), &WFCSolverStateNative::get_divergence_cell);
    ClassDB::bind_method(D_METHOD("set_divergence_cell", "val"), &WFCSolverStateNative::set_divergence_cell);

    ClassDB::bind_method(D_METHOD("get_divergence_options"), &WFCSolverStateNative::get_divergence_options);
    ClassDB::bind_method(D_METHOD("set_divergence_options", "val"), &WFCSolverStateNative::set_divergence_options);

    ClassDB::bind_method(D_METHOD("get_divergence_candidates"), &WFCSolverStateNative::get_divergence_candidates);
    ClassDB::bind_method(D_METHOD("set_divergence_candidates", "val"), &WFCSolverStateNative::set_divergence_candidates);

    ClassDB::bind_method(D_METHOD("get_ac4_counters"), &WFCSolverStateNative::get_ac4_counters);
    ClassDB::bind_method(D_METHOD("set_ac4_counters", "val"), &WFCSolverStateNative::set_ac4_counters);

    ClassDB::bind_method(D_METHOD("get_ac4_counter_index_coefficients"), &WFCSolverStateNative::get_ac4_counter_index_coefficients);
    ClassDB::bind_method(D_METHOD("set_ac4_counter_index_coefficients", "val"), &WFCSolverStateNative::set_ac4_counter_index_coefficients);

    ClassDB::bind_method(D_METHOD("get_ac4_acknowledged_domains"), &WFCSolverStateNative::get_ac4_acknowledged_domains);
    ClassDB::bind_method(D_METHOD("set_ac4_acknowledged_domains", "val"), &WFCSolverStateNative::set_ac4_acknowledged_domains);

    // Methods
    ClassDB::bind_method(D_METHOD("is_cell_solved", "cell_id"), &WFCSolverStateNative::is_cell_solved);
    ClassDB::bind_method(D_METHOD("get_cell_solution", "cell_id"), &WFCSolverStateNative::get_cell_solution);
    ClassDB::bind_method(D_METHOD("is_all_solved"), &WFCSolverStateNative::is_all_solved);
    ClassDB::bind_method(D_METHOD("store_solution", "cell_id", "solution"), &WFCSolverStateNative::store_solution);
    ClassDB::bind_method(D_METHOD("set_solution", "cell_id", "solution"), &WFCSolverStateNative::set_solution);
    ClassDB::bind_method(D_METHOD("set_domain", "cell_id", "domain", "entropy"), &WFCSolverStateNative::set_domain, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("extract_changed_cells"), &WFCSolverStateNative::extract_changed_cells);
    ClassDB::bind_method(D_METHOD("backtrack", "problem"), &WFCSolverStateNative::backtrack);
    ClassDB::bind_method(D_METHOD("make_next"), &WFCSolverStateNative::make_next);
    ClassDB::bind_method(D_METHOD("make_snapshot"), &WFCSolverStateNative::make_snapshot);
    ClassDB::bind_method(D_METHOD("unlink_from_previous"), &WFCSolverStateNative::unlink_from_previous);
    ClassDB::bind_method(D_METHOD("pick_divergence_cell"), &WFCSolverStateNative::pick_divergence_cell);
    ClassDB::bind_method(D_METHOD("prepare_divergence"), &WFCSolverStateNative::prepare_divergence);
    ClassDB::bind_method(D_METHOD("diverge", "problem"), &WFCSolverStateNative::diverge);
    ClassDB::bind_method(D_METHOD("diverge_in_place", "problem"), &WFCSolverStateNative::diverge_in_place);
    ClassDB::bind_method(D_METHOD("get_ac4_counter_offset", "cell_id", "constraint_id", "tile_id"), &WFCSolverStateNative::get_ac4_counter_offset);
    ClassDB::bind_method(D_METHOD("decrement_ac4_counter", "cell_id", "constraint_id", "tile_id"), &WFCSolverStateNative::decrement_ac4_counter);
    ClassDB::bind_method(D_METHOD("ensure_ac4_state", "problem", "binary_constraints"), &WFCSolverStateNative::ensure_ac4_state);

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "previous", PROPERTY_HINT_RESOURCE_TYPE, "WFCSolverStateNative"), "set_previous", "get_previous");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "cell_domains"), "set_cell_domains", "get_cell_domains");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "cell_solution_or_entropy"), "set_cell_solution_or_entropy", "get_cell_solution_or_entropy");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "unsolved_cells"), "set_unsolved_cells", "get_unsolved_cells");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "observations_count"), "set_observations_count", "get_observations_count");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "changed_cells"), "set_changed_cells", "get_changed_cells");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "divergence_cell"), "set_divergence_cell", "get_divergence_cell");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "divergence_options"), "set_divergence_options", "get_divergence_options");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "divergence_candidates"), "set_divergence_candidates", "get_divergence_candidates");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "ac4_counters"), "set_ac4_counters", "get_ac4_counters");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "ac4_counter_index_coefficients"), "set_ac4_counter_index_coefficients", "get_ac4_counter_index_coefficients");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ac4_acknowledged_domains"), "set_ac4_acknowledged_domains", "get_ac4_acknowledged_domains");

    // Constants
    BIND_CONSTANT(CELL_SOLUTION_FAILED);
}

WFCSolverStateNative::WFCSolverStateNative() {
}

WFCSolverStateNative::~WFCSolverStateNative() {
}

bool WFCSolverStateNative::is_cell_solved(int cell_id) const {
    return cell_solution_or_entropy_[cell_id] >= 0;
}

int64_t WFCSolverStateNative::get_cell_solution(int cell_id) const {
    return cell_solution_or_entropy_[cell_id];
}

bool WFCSolverStateNative::is_all_solved() const {
    return unsolved_cells_ == 0;
}

void WFCSolverStateNative::store_solution(int cell_id, int64_t solution) {
    cell_solution_or_entropy_[cell_id] = solution;
    unsolved_cells_ -= 1;
    divergence_candidates_.erase(cell_id);
}

void WFCSolverStateNative::set_solution(int cell_id, int64_t solution) {
    Ref<WFCBitSetNative> first_domain = cell_domains_[0];
    Ref<WFCBitSetNative> bs;
    bs.instantiate();
    bs->initialize(first_domain->get_size());
    bs->set_bit(solution, true);
    set_domain(cell_id, bs, 0);
}

bool WFCSolverStateNative::set_domain(int cell_id, const Ref<WFCBitSetNative>& domain, int entropy) {
    bool should_backtrack = false;

    Ref<WFCBitSetNative> current_domain = cell_domains_[cell_id];

    if (current_domain->equals(domain)) {
        return should_backtrack;
    }

    changed_cells_.append(cell_id);

    int only_bit = domain->get_only_set_bit();

    if (only_bit == WFCBitSetNative::ONLY_BIT_NO_BITS_SET) {
        store_solution(cell_id, CELL_SOLUTION_FAILED);
        entropy = 0;
        should_backtrack = true;
    } else if (only_bit != WFCBitSetNative::ONLY_BIT_MORE_BITS_SET) {
        store_solution(cell_id, only_bit);
        entropy = 0;
    } else {
        if (entropy < 0) {
            entropy = domain->count_set_bits() - 1;
        }

        cell_solution_or_entropy_[cell_id] = -entropy;
        divergence_candidates_[cell_id] = true;
    }

    cell_domains_[cell_id] = domain;

    return should_backtrack;
}

PackedInt64Array WFCSolverStateNative::extract_changed_cells() {
    PackedInt64Array res;
    res.resize(changed_cells_.size());
    for (int i = 0; i < changed_cells_.size(); i++) {
        res[i] = changed_cells_[i];
    }
    changed_cells_.clear();
    return res;
}

Ref<WFCSolverStateNative> WFCSolverStateNative::backtrack(const Ref<WFCProblemNative>& problem) {
    if (previous_.is_null()) {
        return Ref<WFCSolverStateNative>();
    }

    Ref<WFCSolverStateNative> state = previous_->diverge(problem);

    if (state.is_valid()) {
        return state;
    }

    return previous_->backtrack(problem);
}

Ref<WFCSolverStateNative> WFCSolverStateNative::make_next() {
    Ref<WFCSolverStateNative> new_state;
    new_state.instantiate();

    // Duplicate cell_domains
    TypedArray<WFCBitSetNative> domains_copy;
    domains_copy.resize(cell_domains_.size());
    for (int i = 0; i < cell_domains_.size(); i++) {
        domains_copy[i] = cell_domains_[i];
    }
    new_state->cell_domains_ = domains_copy;

    // Duplicate cell_solution_or_entropy
    PackedInt64Array entropy_copy;
    entropy_copy.resize(cell_solution_or_entropy_.size());
    for (int i = 0; i < cell_solution_or_entropy_.size(); i++) {
        entropy_copy[i] = cell_solution_or_entropy_[i];
    }
    new_state->cell_solution_or_entropy_ = entropy_copy;

    new_state->unsolved_cells_ = unsolved_cells_;

    // Duplicate divergence_candidates
    new_state->divergence_candidates_ = divergence_candidates_.duplicate();

    // AC4 state is transferred to next state, without copying
    new_state->ac4_counters_ = ac4_counters_;
    ac4_counters_ = PackedInt32Array();
    new_state->ac4_counter_index_coefficients_ = ac4_counter_index_coefficients_;
    new_state->ac4_acknowledged_domains_ = ac4_acknowledged_domains_;
    ac4_acknowledged_domains_ = TypedArray<WFCBitSetNative>();

    new_state->observations_count_ = observations_count_;
    new_state->previous_ = Ref<WFCSolverStateNative>(this);

    return new_state;
}

Ref<WFCSolverStateNative> WFCSolverStateNative::make_snapshot() const {
    Ref<WFCSolverStateNative> new_state;
    new_state.instantiate();

    // Duplicate cell_domains
    TypedArray<WFCBitSetNative> domains_copy;
    domains_copy.resize(cell_domains_.size());
    for (int i = 0; i < cell_domains_.size(); i++) {
        domains_copy[i] = cell_domains_[i];
    }
    new_state->cell_domains_ = domains_copy;

    // Duplicate cell_solution_or_entropy
    PackedInt64Array entropy_copy;
    entropy_copy.resize(cell_solution_or_entropy_.size());
    for (int i = 0; i < cell_solution_or_entropy_.size(); i++) {
        entropy_copy[i] = cell_solution_or_entropy_[i];
    }
    new_state->cell_solution_or_entropy_ = entropy_copy;

    new_state->unsolved_cells_ = unsolved_cells_;

    return new_state;
}

void WFCSolverStateNative::unlink_from_previous() {
    previous_ = Ref<WFCSolverStateNative>();
}

int WFCSolverStateNative::pick_divergence_cell() {
    TypedArray<int> options;
    int64_t target_entropy = MAX_INT_VAL;

    Array candidates = divergence_candidates_.keys();

    if (candidates.is_empty()) {
        for (int i = 0; i < cell_solution_or_entropy_.size(); i++) {
            candidates.append(i);
        }
    }

    for (int i = 0; i < candidates.size(); i++) {
        int cell_id = candidates[i];
        int64_t entropy = -cell_solution_or_entropy_[cell_id];

        if (entropy <= 0) {
            continue;
        }

        if (entropy == target_entropy) {
            options.append(cell_id);
        } else if (entropy < target_entropy) {
            options.clear();
            options.append(cell_id);
            target_entropy = entropy;
        }
    }

    // Use Array's pick_random() to match GDScript behavior exactly
    Variant picked = ((Array)options).pick_random();
    return static_cast<int>(static_cast<int64_t>(picked));
}

void WFCSolverStateNative::prepare_divergence() {
    divergence_cell_ = pick_divergence_cell();
    divergence_candidates_.erase(divergence_cell_);
    divergence_options_.clear();

    Ref<WFCBitSetNative> domain = cell_domains_[divergence_cell_];
    PackedInt64Array bits = domain->iterator();
    for (int i = 0; i < bits.size(); i++) {
        divergence_options_.append(bits[i]);
    }
}

Ref<WFCSolverStateNative> WFCSolverStateNative::diverge(const Ref<WFCProblemNative>& problem) {
    if (divergence_options_.is_empty()) {
        return Ref<WFCSolverStateNative>();
    }

    Ref<WFCSolverStateNative> next_state = make_next();

    int solution = problem->pick_divergence_option(divergence_options_);

    next_state->set_solution(divergence_cell_, solution);
    next_state->observations_count_ += 1;

    return next_state;
}

void WFCSolverStateNative::diverge_in_place(const Ref<WFCProblemNative>& problem) {
    int solution = problem->pick_divergence_option(divergence_options_);

    set_solution(divergence_cell_, solution);

    divergence_options_.clear();
    divergence_cell_ = -1;
    observations_count_ += 1;
}

int WFCSolverStateNative::get_ac4_counter_offset(int cell_id, int constraint_id, int tile_id) const {
    Vector3i v = ac4_counter_index_coefficients_ * Vector3i(cell_id, constraint_id, tile_id);
    return v.x + v.y + v.z;
}

bool WFCSolverStateNative::decrement_ac4_counter(int cell_id, int constraint_id, int tile_id) {
    int index = get_ac4_counter_offset(cell_id, constraint_id, tile_id);
    int32_t value = ac4_counters_[index];
    value -= 1;
    ac4_counters_[index] = value;

    return value == 0;
}

void WFCSolverStateNative::ensure_ac4_state(const Ref<WFCProblemNative>& problem, const TypedArray<WFCProblemAC4BinaryConstraintNative>& binary_constraints) {
    if (ac4_counters_.size() > 0) {
        return;
    }

    int total_cells = problem->get_cell_count();
    Ref<WFCBitSetNative> default_domain = problem->get_default_domain();
    int domain_size = default_domain->get_size();
    int total_constraints = binary_constraints.size();

    ac4_acknowledged_domains_.resize(total_cells);
    for (int i = 0; i < total_cells; i++) {
        ac4_acknowledged_domains_[i] = default_domain;
    }

    int counters_size = total_cells * total_constraints * domain_size;
    ac4_counters_.resize(counters_size);
    ac4_counter_index_coefficients_ = Vector3i(
        total_constraints * domain_size,
        domain_size,
        1
    );

    for (int constraint_id = 0; constraint_id < binary_constraints.size(); constraint_id++) {
        Ref<WFCProblemAC4BinaryConstraintNative> constraint = binary_constraints[constraint_id];
        PackedInt64Array initial_constraint_counters;
        initial_constraint_counters.resize(domain_size);

        PackedInt64Array tiles = default_domain->iterator();
        for (int t = 0; t < tiles.size(); t++) {
            int tile = tiles[t];
            PackedInt64Array allowed = constraint->get_allowed(tile);
            for (int a = 0; a < allowed.size(); a++) {
                int allowed_tile = allowed[a];
                initial_constraint_counters[allowed_tile] += 1;
            }
        }

        for (int cell_id = 0; cell_id < total_cells; cell_id++) {
            for (int tile_id = 0; tile_id < domain_size; tile_id++) {
                ac4_counters_[get_ac4_counter_offset(cell_id, constraint_id, tile_id)] =
                    initial_constraint_counters[tile_id];
            }
        }
    }

    changed_cells_.clear();
    for (int cell_id = 0; cell_id < cell_domains_.size(); cell_id++) {
        Ref<WFCBitSetNative> cell_domain = cell_domains_[cell_id];
        if (!default_domain->equals(cell_domain)) {
            for (int c = 0; c < binary_constraints.size(); c++) {
                Ref<WFCProblemAC4BinaryConstraintNative> constraint = binary_constraints[c];
                if (!is_cell_solved(constraint->get_dependent(cell_id))) {
                    changed_cells_.append(cell_id);
                    break;
                }
            }
        }
    }
}

} // namespace godot

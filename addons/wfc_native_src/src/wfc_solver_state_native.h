#ifndef WFC_SOLVER_STATE_NATIVE_H
#define WFC_SOLVER_STATE_NATIVE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include "wfc_bitset_native.h"

namespace godot {

// Forward declarations
class WFCProblemNative;
class WFCProblemAC4BinaryConstraintNative;

class WFCSolverStateNative : public RefCounted {
    GDCLASS(WFCSolverStateNative, RefCounted)

public:
    static const int64_t MAX_INT_VAL = 9223372036854775807LL;
    static const int64_t CELL_SOLUTION_FAILED = MAX_INT_VAL;

private:
    // Previous state for backtracking
    Ref<WFCSolverStateNative> previous_;

    // Current domains of all cells
    TypedArray<WFCBitSetNative> cell_domains_;

    // Solution or entropy for each cell
    PackedInt64Array cell_solution_or_entropy_;

    // Number of unsolved cells
    int unsolved_cells_ = 0;

    // Number of observations made
    int observations_count_ = 0;

    // Changed cells
    PackedInt64Array changed_cells_;

    // Divergence state
    int divergence_cell_ = -1;
    TypedArray<int> divergence_options_;

    // Divergence candidates
    Dictionary divergence_candidates_;

    // AC4 state
    PackedInt32Array ac4_counters_;
    Vector3i ac4_counter_index_coefficients_;
    TypedArray<WFCBitSetNative> ac4_acknowledged_domains_;

protected:
    static void _bind_methods();

public:
    WFCSolverStateNative();
    ~WFCSolverStateNative();

    // Property accessors
    Ref<WFCSolverStateNative> get_previous() const { return previous_; }
    void set_previous(const Ref<WFCSolverStateNative>& val) { previous_ = val; }

    TypedArray<WFCBitSetNative> get_cell_domains() const { return cell_domains_; }
    void set_cell_domains(const TypedArray<WFCBitSetNative>& val) { cell_domains_ = val; }

    PackedInt64Array get_cell_solution_or_entropy() const { return cell_solution_or_entropy_; }
    void set_cell_solution_or_entropy(const PackedInt64Array& val) { cell_solution_or_entropy_ = val; }

    int get_unsolved_cells() const { return unsolved_cells_; }
    void set_unsolved_cells(int val) { unsolved_cells_ = val; }

    int get_observations_count() const { return observations_count_; }
    void set_observations_count(int val) { observations_count_ = val; }

    PackedInt64Array get_changed_cells() const { return changed_cells_; }
    void set_changed_cells(const PackedInt64Array& val) { changed_cells_ = val; }

    int get_divergence_cell() const { return divergence_cell_; }
    void set_divergence_cell(int val) { divergence_cell_ = val; }

    TypedArray<int> get_divergence_options() const { return divergence_options_; }
    void set_divergence_options(const TypedArray<int>& val) { divergence_options_ = val; }

    Dictionary get_divergence_candidates() const { return divergence_candidates_; }
    void set_divergence_candidates(const Dictionary& val) { divergence_candidates_ = val; }

    PackedInt32Array get_ac4_counters() const { return ac4_counters_; }
    void set_ac4_counters(const PackedInt32Array& val) { ac4_counters_ = val; }

    Vector3i get_ac4_counter_index_coefficients() const { return ac4_counter_index_coefficients_; }
    void set_ac4_counter_index_coefficients(const Vector3i& val) { ac4_counter_index_coefficients_ = val; }

    TypedArray<WFCBitSetNative> get_ac4_acknowledged_domains() const { return ac4_acknowledged_domains_; }
    void set_ac4_acknowledged_domains(const TypedArray<WFCBitSetNative>& val) { ac4_acknowledged_domains_ = val; }

    // Methods
    bool is_cell_solved(int cell_id) const;
    int64_t get_cell_solution(int cell_id) const;
    bool is_all_solved() const;

    void store_solution(int cell_id, int64_t solution);
    void set_solution(int cell_id, int64_t solution);
    bool set_domain(int cell_id, const Ref<WFCBitSetNative>& domain, int entropy = -1);

    PackedInt64Array extract_changed_cells();

    Ref<WFCSolverStateNative> backtrack(const Ref<WFCProblemNative>& problem);
    Ref<WFCSolverStateNative> make_next();
    Ref<WFCSolverStateNative> make_snapshot() const;
    void unlink_from_previous();

    int pick_divergence_cell();
    void prepare_divergence();
    Ref<WFCSolverStateNative> diverge(const Ref<WFCProblemNative>& problem);
    void diverge_in_place(const Ref<WFCProblemNative>& problem);

    // AC4 methods
    int get_ac4_counter_offset(int cell_id, int constraint_id, int tile_id) const;
    bool decrement_ac4_counter(int cell_id, int constraint_id, int tile_id);
    void ensure_ac4_state(const Ref<WFCProblemNative>& problem, const TypedArray<WFCProblemAC4BinaryConstraintNative>& binary_constraints);
};

} // namespace godot

#endif // WFC_SOLVER_STATE_NATIVE_H

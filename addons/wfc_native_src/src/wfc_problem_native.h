#ifndef WFC_PROBLEM_NATIVE_H
#define WFC_PROBLEM_NATIVE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <functional>
#include "wfc_bitset_native.h"
#include "wfc_solver_state_native.h"

namespace godot {

// AC4 Binary Constraint abstract class
class WFCProblemAC4BinaryConstraintNative : public RefCounted {
    GDCLASS(WFCProblemAC4BinaryConstraintNative, RefCounted)

protected:
    static void _bind_methods();

public:
    WFCProblemAC4BinaryConstraintNative();
    virtual ~WFCProblemAC4BinaryConstraintNative();

    virtual int get_dependent(int cell_id);
    virtual int get_dependency(int cell_id);
    virtual PackedInt64Array get_allowed(int dependency_variant);
};

// SubProblem class
class WFCProblemNative; // Forward declaration

class WFCProblemSubProblemNative : public RefCounted {
    GDCLASS(WFCProblemSubProblemNative, RefCounted)

private:
    Ref<WFCProblemNative> problem_;
    PackedInt64Array dependencies_;

protected:
    static void _bind_methods();

public:
    WFCProblemSubProblemNative();
    ~WFCProblemSubProblemNative();

    void initialize(const Ref<WFCProblemNative>& problem, const PackedInt64Array& dependencies);

    Ref<WFCProblemNative> get_problem() const { return problem_; }
    void set_problem(const Ref<WFCProblemNative>& val) { problem_ = val; }
    PackedInt64Array get_dependencies() const { return dependencies_; }
    void set_dependencies(const PackedInt64Array& val) { dependencies_ = val; }
};

// Main WFCProblem abstract class
class WFCProblemNative : public RefCounted {
    GDCLASS(WFCProblemNative, RefCounted)

protected:
    static void _bind_methods();

public:
    WFCProblemNative();
    virtual ~WFCProblemNative();

    // Virtual methods to be overridden
    virtual int get_cell_count();
    virtual Ref<WFCBitSetNative> get_default_domain();
    virtual void populate_initial_state(const Ref<WFCSolverStateNative>& state);
    virtual Ref<WFCBitSetNative> compute_cell_domain(const Ref<WFCSolverStateNative>& state, int cell_id);
    virtual void mark_related_cells(int changed_cell_id, const Callable& mark_cell);
    virtual TypedArray<WFCProblemSubProblemNative> split(int concurrency_limit);
    virtual int pick_divergence_option(TypedArray<int> options);
    virtual bool supports_ac4();
    virtual TypedArray<WFCProblemAC4BinaryConstraintNative> get_ac4_binary_constraints();

    // Returns cell IDs that depend on the changed cell (for AC3 propagation)
    virtual PackedInt64Array get_related_cells(int changed_cell_id);

    // C++ specific: Internal version with std::function for performance
    void mark_related_cells_internal(int changed_cell_id, std::function<void(int)> mark_cell);

    // Debug methods
    int debug_randi_range(int from, int to);
    String debug_array_contents(TypedArray<int> arr);
};

} // namespace godot

#endif // WFC_PROBLEM_NATIVE_H

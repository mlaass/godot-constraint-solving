#include "wfc_problem_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// AC4BinaryConstraint implementation
void WFCProblemAC4BinaryConstraintNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_dependent", "cell_id"), &WFCProblemAC4BinaryConstraintNative::get_dependent);
    ClassDB::bind_method(D_METHOD("get_dependency", "cell_id"), &WFCProblemAC4BinaryConstraintNative::get_dependency);
    ClassDB::bind_method(D_METHOD("get_allowed", "dependency_variant"), &WFCProblemAC4BinaryConstraintNative::get_allowed);
}

WFCProblemAC4BinaryConstraintNative::WFCProblemAC4BinaryConstraintNative() {
}

WFCProblemAC4BinaryConstraintNative::~WFCProblemAC4BinaryConstraintNative() {
}

int WFCProblemAC4BinaryConstraintNative::get_dependent(int cell_id) {
    return -1; // To be overridden
}

int WFCProblemAC4BinaryConstraintNative::get_dependency(int cell_id) {
    return -1; // To be overridden
}

PackedInt64Array WFCProblemAC4BinaryConstraintNative::get_allowed(int dependency_variant) {
    return PackedInt64Array(); // To be overridden
}

// SubProblem implementation
void WFCProblemSubProblemNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "problem", "dependencies"), &WFCProblemSubProblemNative::initialize);
    ClassDB::bind_method(D_METHOD("get_problem"), &WFCProblemSubProblemNative::get_problem);
    ClassDB::bind_method(D_METHOD("set_problem", "val"), &WFCProblemSubProblemNative::set_problem);
    ClassDB::bind_method(D_METHOD("get_dependencies"), &WFCProblemSubProblemNative::get_dependencies);
    ClassDB::bind_method(D_METHOD("set_dependencies", "val"), &WFCProblemSubProblemNative::set_dependencies);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "problem", PROPERTY_HINT_RESOURCE_TYPE, "WFCProblemNative"), "set_problem", "get_problem");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "dependencies"), "set_dependencies", "get_dependencies");
}

WFCProblemSubProblemNative::WFCProblemSubProblemNative() {
}

WFCProblemSubProblemNative::~WFCProblemSubProblemNative() {
}

void WFCProblemSubProblemNative::initialize(const Ref<WFCProblemNative>& problem, const PackedInt64Array& dependencies) {
    problem_ = problem;
    dependencies_ = dependencies;
}

// WFCProblem implementation
void WFCProblemNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_cell_count"), &WFCProblemNative::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_default_domain"), &WFCProblemNative::get_default_domain);
    ClassDB::bind_method(D_METHOD("populate_initial_state", "state"), &WFCProblemNative::populate_initial_state);
    ClassDB::bind_method(D_METHOD("compute_cell_domain", "state", "cell_id"), &WFCProblemNative::compute_cell_domain);
    ClassDB::bind_method(D_METHOD("mark_related_cells", "changed_cell_id", "mark_cell"), &WFCProblemNative::mark_related_cells);
    ClassDB::bind_method(D_METHOD("split", "concurrency_limit"), &WFCProblemNative::split);
    ClassDB::bind_method(D_METHOD("pick_divergence_option", "options"), &WFCProblemNative::pick_divergence_option);
    ClassDB::bind_method(D_METHOD("supports_ac4"), &WFCProblemNative::supports_ac4);
    ClassDB::bind_method(D_METHOD("get_ac4_binary_constraints"), &WFCProblemNative::get_ac4_binary_constraints);
    ClassDB::bind_method(D_METHOD("get_related_cells", "changed_cell_id"), &WFCProblemNative::get_related_cells);
    ClassDB::bind_method(D_METHOD("debug_randi_range", "from", "to"), &WFCProblemNative::debug_randi_range);
    ClassDB::bind_method(D_METHOD("debug_array_contents", "arr"), &WFCProblemNative::debug_array_contents);
}

WFCProblemNative::WFCProblemNative() {
}

WFCProblemNative::~WFCProblemNative() {
}

int WFCProblemNative::get_cell_count() {
    return -1; // To be overridden
}

Ref<WFCBitSetNative> WFCProblemNative::get_default_domain() {
    Ref<WFCBitSetNative> empty;
    empty.instantiate();
    empty->initialize(0);
    return empty; // To be overridden
}

void WFCProblemNative::populate_initial_state(const Ref<WFCSolverStateNative>& state) {
    // Default: do nothing
}

Ref<WFCBitSetNative> WFCProblemNative::compute_cell_domain(const Ref<WFCSolverStateNative>& state, int cell_id) {
    Ref<WFCBitSetNative> empty;
    empty.instantiate();
    empty->initialize(0);
    return empty; // To be overridden
}

void WFCProblemNative::mark_related_cells(int changed_cell_id, const Callable& mark_cell) {
    // To be overridden
}

void WFCProblemNative::mark_related_cells_internal(int changed_cell_id, std::function<void(int)> mark_cell) {
    // Internal C++ version - to be overridden by derived classes
}

PackedInt64Array WFCProblemNative::get_related_cells(int changed_cell_id) {
    // Default: return empty array (subclasses override)
    return PackedInt64Array();
}

TypedArray<WFCProblemSubProblemNative> WFCProblemNative::split(int concurrency_limit) {
    TypedArray<WFCProblemSubProblemNative> result;
    Ref<WFCProblemSubProblemNative> sub;
    sub.instantiate();
    sub->initialize(Ref<WFCProblemNative>(this), PackedInt64Array());
    result.append(sub);
    return result;
}

int WFCProblemNative::pick_divergence_option(TypedArray<int> options) {
    if (options.size() == 0) return -1;

    int index = UtilityFunctions::randi_range(0, options.size() - 1);
    // Explicit Variant conversion to avoid issues with TypedArray operator[]
    Variant v = options[index];
    int result = static_cast<int>(static_cast<int64_t>(v));
    options.remove_at(index);
    return result;
}

// Debug methods
int WFCProblemNative::debug_randi_range(int from, int to) {
    return UtilityFunctions::randi_range(from, to);
}

String WFCProblemNative::debug_array_contents(TypedArray<int> arr) {
    String s = "size=" + String::num_int64(arr.size()) + " [";
    for (int i = 0; i < arr.size(); i++) {
        Variant v = arr[i];
        s += String::num_int64(static_cast<int64_t>(v));
        if (i < arr.size() - 1) s += ", ";
    }
    s += "]";
    return s;
}

bool WFCProblemNative::supports_ac4() {
    return false;
}

TypedArray<WFCProblemAC4BinaryConstraintNative> WFCProblemNative::get_ac4_binary_constraints() {
    return TypedArray<WFCProblemAC4BinaryConstraintNative>(); // To be overridden
}

} // namespace godot

#include "wfc_2d_problem_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// WFC2DAC4BinaryConstraintNative implementation

void WFC2DAC4BinaryConstraintNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "axis", "size", "axis_matrix"), &WFC2DAC4BinaryConstraintNative::initialize);
    ClassDB::bind_method(D_METHOD("get_cell_id", "pos"), &WFC2DAC4BinaryConstraintNative::get_cell_id);
    ClassDB::bind_method(D_METHOD("get_cell_pos", "cell_id"), &WFC2DAC4BinaryConstraintNative::get_cell_pos);
}

WFC2DAC4BinaryConstraintNative::WFC2DAC4BinaryConstraintNative() {
}

WFC2DAC4BinaryConstraintNative::~WFC2DAC4BinaryConstraintNative() {
}

void WFC2DAC4BinaryConstraintNative::initialize(const Vector2i& axis, const Vector2i& size, const Ref<WFCBitMatrixNative>& axis_matrix) {
    axis_ = axis;
    problem_size_ = Rect2i(Vector2i(0, 0), size);

    allowed_tiles_.clear();
    for (int i = 0; i < axis_matrix->get_height(); i++) {
        Ref<WFCBitSetNative> row = axis_matrix->get_row(i);
        if (row.is_valid()) {
            allowed_tiles_.append(row->to_array());
        } else {
            allowed_tiles_.append(PackedInt64Array());
        }
    }
}

int WFC2DAC4BinaryConstraintNative::get_cell_id(const Vector2i& pos) const {
    if (problem_size_.has_point(pos)) {
        return pos.x + pos.y * problem_size_.size.x;
    }
    return -1;
}

Vector2i WFC2DAC4BinaryConstraintNative::get_cell_pos(int cell_id) const {
    int szx = problem_size_.size.x;
    return Vector2i(cell_id % szx, cell_id / szx);
}

int WFC2DAC4BinaryConstraintNative::get_dependent(int cell_id) {
    return get_cell_id(get_cell_pos(cell_id) - axis_);
}

int WFC2DAC4BinaryConstraintNative::get_dependency(int cell_id) {
    return get_cell_id(get_cell_pos(cell_id) + axis_);
}

PackedInt64Array WFC2DAC4BinaryConstraintNative::get_allowed(int dependency_variant) {
    if (dependency_variant >= 0 && dependency_variant < allowed_tiles_.size()) {
        return allowed_tiles_[dependency_variant];
    }
    return PackedInt64Array();
}

// WFC2DProblemNative implementation

void WFC2DProblemNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "rules", "rect"), &WFC2DProblemNative::initialize);

    ClassDB::bind_method(D_METHOD("get_rules"), &WFC2DProblemNative::get_rules);
    ClassDB::bind_method(D_METHOD("set_rules", "val"), &WFC2DProblemNative::set_rules);

    ClassDB::bind_method(D_METHOD("get_rect"), &WFC2DProblemNative::get_rect);
    ClassDB::bind_method(D_METHOD("set_rect", "val"), &WFC2DProblemNative::set_rect);

    ClassDB::bind_method(D_METHOD("get_renderable_rect"), &WFC2DProblemNative::get_renderable_rect);
    ClassDB::bind_method(D_METHOD("set_renderable_rect", "val"), &WFC2DProblemNative::set_renderable_rect);

    ClassDB::bind_method(D_METHOD("get_edges_rect"), &WFC2DProblemNative::get_edges_rect);
    ClassDB::bind_method(D_METHOD("set_edges_rect", "val"), &WFC2DProblemNative::set_edges_rect);

    ClassDB::bind_method(D_METHOD("get_axes"), &WFC2DProblemNative::get_axes);
    ClassDB::bind_method(D_METHOD("get_axis_matrices"), &WFC2DProblemNative::get_axis_matrices);

    ClassDB::bind_method(D_METHOD("coord_to_id", "coord"), &WFC2DProblemNative::coord_to_id);
    ClassDB::bind_method(D_METHOD("id_to_coord", "id"), &WFC2DProblemNative::id_to_coord);

    // Precondition methods
    ClassDB::bind_method(D_METHOD("set_precondition_domain", "cell_id", "domain"), &WFC2DProblemNative::set_precondition_domain);
    ClassDB::bind_method(D_METHOD("set_precondition_solution", "cell_id", "solution"), &WFC2DProblemNative::set_precondition_solution);
    ClassDB::bind_method(D_METHOD("clear_preconditions"), &WFC2DProblemNative::clear_preconditions);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rules", PROPERTY_HINT_RESOURCE_TYPE, "WFCRules2DNative"), "set_rules", "get_rules");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "rect"), "set_rect", "get_rect");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "renderable_rect"), "set_renderable_rect", "get_renderable_rect");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "edges_rect"), "set_edges_rect", "get_edges_rect");
}

WFC2DProblemNative::WFC2DProblemNative() {
}

WFC2DProblemNative::~WFC2DProblemNative() {
}

void WFC2DProblemNative::initialize(const Ref<WFCRules2DNative>& rules, const Rect2i& rect) {
    rules_ = rules;
    rect_ = rect;
    renderable_rect_ = rect;
    edges_rect_ = rect;
    tile_count_ = rules->get_tile_count();

    // Build axes and axis_matrices (including reverse directions)
    axes_.clear();
    axis_matrices_.clear();

    TypedArray<Vector2i> rule_axes = rules->get_axes();
    TypedArray<WFCBitMatrixNative> rule_matrices = rules->get_axis_matrices();

    for (int i = 0; i < rule_axes.size(); i++) {
        Vector2i axis = rule_axes[i];
        Ref<WFCBitMatrixNative> matrix = rule_matrices[i];

        // Forward direction
        axes_.append(axis);
        axis_matrices_.append(matrix);

        // Reverse direction
        axes_.append(-axis);
        axis_matrices_.append(matrix->transpose());
    }
}

int WFC2DProblemNative::coord_to_id(const Vector2i& coord) const {
    return rect_.size.x * coord.y + coord.x;
}

Vector2i WFC2DProblemNative::id_to_coord(int id) const {
    int szx = rect_.size.x;
    return Vector2i(id % szx, id / szx);
}

int WFC2DProblemNative::get_cell_count() {
    return rect_.get_area();
}

Ref<WFCBitSetNative> WFC2DProblemNative::get_default_domain() {
    Ref<WFCBitSetNative> domain;
    domain.instantiate();
    domain->initialize(tile_count_, true);
    return domain;
}

void WFC2DProblemNative::set_precondition_domain(int cell_id, const Ref<WFCBitSetNative>& domain) {
    int cell_count = get_cell_count();

    // Initialize arrays if needed
    if (precondition_domains_.size() != cell_count) {
        precondition_domains_.resize(cell_count);
        // All elements are null by default (no precondition)
    }
    if (precondition_solutions_.size() != cell_count) {
        precondition_solutions_.resize(cell_count);
        precondition_solutions_.fill(-1);  // -1 = not pre-solved
    }

    if (cell_id >= 0 && cell_id < cell_count) {
        precondition_domains_[cell_id] = domain;
    }
}

void WFC2DProblemNative::set_precondition_solution(int cell_id, int solution) {
    int cell_count = get_cell_count();

    // Initialize arrays if needed
    if (precondition_domains_.size() != cell_count) {
        precondition_domains_.resize(cell_count);
    }
    if (precondition_solutions_.size() != cell_count) {
        precondition_solutions_.resize(cell_count);
        precondition_solutions_.fill(-1);
    }

    if (cell_id >= 0 && cell_id < cell_count) {
        precondition_solutions_[cell_id] = solution;
    }
}

void WFC2DProblemNative::clear_preconditions() {
    precondition_domains_.clear();
    precondition_solutions_.clear();
}

void WFC2DProblemNative::populate_initial_state(const Ref<WFCSolverStateNative>& state) {
    int cell_count = get_cell_count();
    int width = rect_.size.x;
    int height = rect_.size.y;

    // IMPORTANT: Iterate in same order as GDScript (for x ... for y)
    // to ensure changed_cells is populated in identical order.
    // GDScript iterates: for x in range(rect.size.x): for y in range(rect.size.y)
    // which visits cells in column-major order.
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int cell_id = y * width + x;

            // Check for pre-solved cells first
            if (precondition_solutions_.size() > cell_id && precondition_solutions_[cell_id] >= 0) {
                state->set_solution(cell_id, precondition_solutions_[cell_id]);
            }
            // Otherwise check for domain constraints
            else if (precondition_domains_.size() > cell_id) {
                Ref<WFCBitSetNative> domain = precondition_domains_[cell_id];
                if (domain.is_valid() && !domain->is_empty()) {
                    state->set_domain(cell_id, domain);
                }
            }
        }
    }
}

Ref<WFCBitSetNative> WFC2DProblemNative::compute_cell_domain(const Ref<WFCSolverStateNative>& state, int cell_id) {
    TypedArray<WFCBitSetNative> cell_domains = state->get_cell_domains();
    Ref<WFCBitSetNative> current_domain = cell_domains[cell_id];
    Ref<WFCBitSetNative> res = current_domain->copy();

    Vector2i pos = id_to_coord(cell_id);
    PackedInt64Array solution_or_entropy = state->get_cell_solution_or_entropy();

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        Vector2i other_pos = pos + axis;

        if (!rect_.has_point(other_pos + rect_.position)) {
            continue;
        }

        int other_id = coord_to_id(other_pos);

        if (solution_or_entropy[other_id] == WFCSolverStateNative::CELL_SOLUTION_FAILED) {
            continue;
        }

        Ref<WFCBitSetNative> other_domain = cell_domains[other_id];
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[i];
        Ref<WFCBitSetNative> transformed = matrix->transform(other_domain);
        res->intersect_in_place(transformed);
    }

    return res;
}

void WFC2DProblemNative::mark_related_cells(int changed_cell_id, const Callable& mark_cell) {
    Vector2i pos = id_to_coord(changed_cell_id);

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        Vector2i other_pos = pos + axis;
        if (rect_.has_point(other_pos + rect_.position)) {
            mark_cell.call(coord_to_id(other_pos));
        }
    }
}

PackedInt64Array WFC2DProblemNative::get_related_cells(int changed_cell_id) {
    PackedInt64Array result;
    Vector2i pos = id_to_coord(changed_cell_id);

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        Vector2i other_pos = pos + axis;
        if (rect_.has_point(other_pos + rect_.position)) {
            result.append(coord_to_id(other_pos));
        }
    }

    return result;
}

int WFC2DProblemNative::pick_divergence_option(TypedArray<int> options) {
    if (rules_.is_null() || !rules_->get_probabilities_enabled()) {
        return WFCProblemNative::pick_divergence_option(options);
    }

    if (options.size() == 0) return -1;
    if (options.size() == 1) {
        // Explicit Variant conversion to avoid issues with TypedArray operator[]
        Variant v = options[0];
        int result = static_cast<int>(static_cast<int64_t>(v));
        options.remove_at(0);
        return result;
    }

    PackedFloat32Array probabilities = rules_->get_probabilities();
    float probabilities_sum = 0.0f;

    for (int i = 0; i < options.size(); i++) {
        Variant v = options[i];
        int option = static_cast<int>(static_cast<int64_t>(v));
        if (option >= 0 && option < probabilities.size()) {
            probabilities_sum += probabilities[option];
        }
    }

    float value = UtilityFunctions::randf_range(0.0f, probabilities_sum);
    probabilities_sum = 0.0f;
    int chosen_index = 0;

    for (int i = 0; i < options.size(); i++) {
        Variant v = options[i];
        int option = static_cast<int>(static_cast<int64_t>(v));
        if (option >= 0 && option < probabilities.size()) {
            probabilities_sum += probabilities[option];
        }
        if (probabilities_sum > value) {
            chosen_index = i;
            break;
        }
    }

    Variant v = options[chosen_index];
    int result = static_cast<int>(static_cast<int64_t>(v));
    options.remove_at(chosen_index);
    return result;
}

bool WFC2DProblemNative::supports_ac4() {
    return true;
}

TypedArray<WFCProblemAC4BinaryConstraintNative> WFC2DProblemNative::get_ac4_binary_constraints() {
    TypedArray<WFCProblemAC4BinaryConstraintNative> constraints;

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[i];

        Ref<WFC2DAC4BinaryConstraintNative> constraint;
        constraint.instantiate();
        constraint->initialize(axis, rect_.size, matrix);
        constraints.append(constraint);
    }

    return constraints;
}

} // namespace godot

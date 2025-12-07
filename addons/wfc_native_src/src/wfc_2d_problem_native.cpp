#include "wfc_2d_problem_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

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

    ClassDB::bind_method(D_METHOD("get_init_read_rects"), &WFC2DProblemNative::get_init_read_rects);
    ClassDB::bind_method(D_METHOD("set_init_read_rects", "val"), &WFC2DProblemNative::set_init_read_rects);

    ClassDB::bind_method(D_METHOD("get_axes"), &WFC2DProblemNative::get_axes);
    ClassDB::bind_method(D_METHOD("get_axis_matrices"), &WFC2DProblemNative::get_axis_matrices);

    ClassDB::bind_method(D_METHOD("coord_to_id", "coord"), &WFC2DProblemNative::coord_to_id);
    ClassDB::bind_method(D_METHOD("id_to_coord", "id"), &WFC2DProblemNative::id_to_coord);

    ClassDB::bind_method(D_METHOD("get_dependencies_range"), &WFC2DProblemNative::get_dependencies_range);
    ClassDB::bind_method(D_METHOD("split", "concurrency_limit"), &WFC2DProblemNative::split);

    // Precondition methods
    ClassDB::bind_method(D_METHOD("set_precondition_domain", "cell_id", "domain"), &WFC2DProblemNative::set_precondition_domain);
    ClassDB::bind_method(D_METHOD("set_precondition_solution", "cell_id", "solution"), &WFC2DProblemNative::set_precondition_solution);
    ClassDB::bind_method(D_METHOD("clear_preconditions"), &WFC2DProblemNative::clear_preconditions);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rules", PROPERTY_HINT_RESOURCE_TYPE, "WFCRules2DNative"), "set_rules", "get_rules");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "rect"), "set_rect", "get_rect");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "renderable_rect"), "set_renderable_rect", "get_renderable_rect");
    ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "edges_rect"), "set_edges_rect", "get_edges_rect");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "init_read_rects"), "set_init_read_rects", "get_init_read_rects");
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

Vector2i WFC2DProblemNative::get_dependencies_range() const {
    int rx = 0;
    int ry = 0;

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        rx = std::max(rx, std::abs(axis.x));
        ry = std::max(ry, std::abs(axis.y));
    }

    return Vector2i(rx, ry);
}

PackedInt64Array WFC2DProblemNative::split_range(int first, int size, int partitions, int min_partition_size) {
    if (partitions <= 0) {
        return PackedInt64Array();
    }

    int approx_partition_size = size / partitions;

    if (approx_partition_size < min_partition_size) {
        if (partitions <= 2) {
            PackedInt64Array res;
            res.append(first);
            res.append(first + size);
            return res;
        }
        return split_range(first, size, partitions - 1, min_partition_size);
    }

    PackedInt64Array res;
    for (int partition = 0; partition < partitions; partition++) {
        res.append(first + (size * partition) / partitions);
    }
    res.append(first + size);

    return res;
}

TypedArray<WFCProblemSubProblemNative> WFC2DProblemNative::split(int concurrency_limit) {
    TypedArray<WFCProblemSubProblemNative> empty_result;

    if (concurrency_limit < 2) {
        // Return single sub-problem with no dependencies
        Ref<WFCProblemSubProblemNative> sub;
        sub.instantiate();

        // Create a copy of this problem
        Ref<WFC2DProblemNative> problem_copy;
        problem_copy.instantiate();
        problem_copy->initialize(rules_, rect_);
        problem_copy->set_renderable_rect(renderable_rect_);
        problem_copy->set_edges_rect(edges_rect_);

        sub->initialize(problem_copy, PackedInt64Array());
        empty_result.append(sub);
        return empty_result;
    }

    TypedArray<Rect2i> rects;

    Vector2i dependency_range = get_dependencies_range();
    Vector2i overlap_min = dependency_range / 2;
    Vector2i overlap_max = overlap_min + Vector2i(dependency_range.x % 2, dependency_range.y % 2);

    Vector2i influence_range = rules_->get_influence_range();
    Vector2i extra_overlap(0, 0);

    bool may_split_x = influence_range.x < rect_.size.x;
    bool may_split_y = influence_range.y < rect_.size.y;

    int split_x_overhead = influence_range.x * rect_.size.y;
    int split_y_overhead = influence_range.y * rect_.size.x;

    if (may_split_x && (!may_split_y || split_x_overhead <= split_y_overhead)) {
        // Split along X axis
        extra_overlap.x = influence_range.x * 2;

        PackedInt64Array partitions = split_range(
            rect_.position.x,
            rect_.size.x,
            concurrency_limit * 2,
            dependency_range.x + extra_overlap.x * 2
        );

        for (int i = 0; i < partitions.size() - 1; i++) {
            Rect2i sub_rect(
                partitions[i],
                rect_.position.y,
                partitions[i + 1] - partitions[i],
                rect_.size.y
            );
            rects.append(sub_rect);
        }
    } else if (may_split_y && (!may_split_x || split_y_overhead <= split_x_overhead)) {
        // Split along Y axis
        extra_overlap.y = influence_range.y * 2;

        PackedInt64Array partitions = split_range(
            rect_.position.y,
            rect_.size.y,
            concurrency_limit * 2,
            dependency_range.y + extra_overlap.y * 2
        );

        for (int i = 0; i < partitions.size() - 1; i++) {
            Rect2i sub_rect(
                rect_.position.x,
                partitions[i],
                rect_.size.x,
                partitions[i + 1] - partitions[i]
            );
            rects.append(sub_rect);
        }
    } else {
        UtilityFunctions::print_verbose("Could not split the problem. influence_range=(",
            influence_range.x, ",", influence_range.y, "), overhead_x=", split_x_overhead,
            ", overhead_y=", split_y_overhead);
        // Return single sub-problem
        Ref<WFCProblemSubProblemNative> sub;
        sub.instantiate();
        Ref<WFC2DProblemNative> problem_copy;
        problem_copy.instantiate();
        problem_copy->initialize(rules_, rect_);
        problem_copy->set_renderable_rect(renderable_rect_);
        problem_copy->set_edges_rect(edges_rect_);
        sub->initialize(problem_copy, PackedInt64Array());
        empty_result.append(sub);
        return empty_result;
    }

    if (rects.size() < 3) {
        UtilityFunctions::print_verbose("Could not split problem. produced_rects=", rects.size());
        // Return single sub-problem
        Ref<WFCProblemSubProblemNative> sub;
        sub.instantiate();
        Ref<WFC2DProblemNative> problem_copy;
        problem_copy.instantiate();
        problem_copy->initialize(rules_, rect_);
        problem_copy->set_renderable_rect(renderable_rect_);
        problem_copy->set_edges_rect(edges_rect_);
        sub->initialize(problem_copy, PackedInt64Array());
        empty_result.append(sub);
        return empty_result;
    }

    TypedArray<WFCProblemSubProblemNative> result;

    for (int i = 0; i < rects.size(); i++) {
        Rect2i base_rect = rects[i];

        // Calculate sub_renderable_rect with overlap
        Rect2i sub_renderable_rect = base_rect;
        sub_renderable_rect.position.x -= overlap_min.x;
        sub_renderable_rect.position.y -= overlap_min.y;
        sub_renderable_rect.size.x += overlap_min.x + overlap_max.x;
        sub_renderable_rect.size.y += overlap_min.y + overlap_max.y;
        sub_renderable_rect = sub_renderable_rect.intersection(rect_);

        Rect2i sub_rect = sub_renderable_rect;

        // Even-indexed sub-problems get extended rects
        if ((i & 1) == 0) {
            sub_rect.position.x -= extra_overlap.x;
            sub_rect.position.y -= extra_overlap.y;
            sub_rect.size.x += extra_overlap.x * 2;
            sub_rect.size.y += extra_overlap.y * 2;
            sub_rect = sub_rect.intersection(rect_);
        }

        // Create sub-problem
        Ref<WFC2DProblemNative> sub_problem;
        sub_problem.instantiate();
        sub_problem->initialize(rules_, sub_rect);
        sub_problem->set_renderable_rect(sub_renderable_rect);
        sub_problem->set_edges_rect(edges_rect_);

        // Set dependencies for odd-indexed sub-problems
        PackedInt64Array dependencies;
        if ((i & 1) == 1) {
            dependencies.append(i - 1);
            if (i < rects.size() - 1) {
                dependencies.append(i + 1);
            }
        }

        Ref<WFCProblemSubProblemNative> sub;
        sub.instantiate();
        sub->initialize(sub_problem, dependencies);
        result.append(sub);
    }

    // Set up init_read_rects for dependent sub-problems
    for (int i = 0; i < result.size(); i++) {
        if (i & 1) {
            Ref<WFCProblemSubProblemNative> current_sub = result[i];
            Ref<WFC2DProblemNative> cur_problem = Object::cast_to<WFC2DProblemNative>(current_sub->get_problem().ptr());

            Ref<WFCProblemSubProblemNative> dep1_sub = result[i - 1];
            Ref<WFC2DProblemNative> dependency1 = Object::cast_to<WFC2DProblemNative>(dep1_sub->get_problem().ptr());

            TypedArray<Rect2i> read_rects;
            read_rects.append(cur_problem->get_rect().intersection(dependency1->get_renderable_rect()));

            if ((i + 1) < result.size()) {
                Ref<WFCProblemSubProblemNative> dep2_sub = result[i + 1];
                Ref<WFC2DProblemNative> dependency2 = Object::cast_to<WFC2DProblemNative>(dep2_sub->get_problem().ptr());
                read_rects.append(cur_problem->get_rect().intersection(dependency2->get_renderable_rect()));
            }

            cur_problem->set_init_read_rects(read_rects);
        }
    }

    return result;
}

} // namespace godot

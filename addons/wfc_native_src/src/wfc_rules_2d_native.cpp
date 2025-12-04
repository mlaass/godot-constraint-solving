#include "wfc_rules_2d_native.h"
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void WFCRules2DNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "tile_count", "axes"), &WFCRules2DNative::initialize);

    ClassDB::bind_method(D_METHOD("get_complete_matrices"), &WFCRules2DNative::get_complete_matrices);
    ClassDB::bind_method(D_METHOD("set_complete_matrices", "val"), &WFCRules2DNative::set_complete_matrices);

    ClassDB::bind_method(D_METHOD("get_axes"), &WFCRules2DNative::get_axes);
    ClassDB::bind_method(D_METHOD("set_axes", "val"), &WFCRules2DNative::set_axes);

    ClassDB::bind_method(D_METHOD("get_axis_matrices"), &WFCRules2DNative::get_axis_matrices);
    ClassDB::bind_method(D_METHOD("set_axis_matrices", "val"), &WFCRules2DNative::set_axis_matrices);

    ClassDB::bind_method(D_METHOD("get_probabilities"), &WFCRules2DNative::get_probabilities);
    ClassDB::bind_method(D_METHOD("set_probabilities", "val"), &WFCRules2DNative::set_probabilities);

    ClassDB::bind_method(D_METHOD("get_edge_domain"), &WFCRules2DNative::get_edge_domain);
    ClassDB::bind_method(D_METHOD("set_edge_domain", "val"), &WFCRules2DNative::set_edge_domain);

    ClassDB::bind_method(D_METHOD("get_probabilities_enabled"), &WFCRules2DNative::get_probabilities_enabled);
    ClassDB::bind_method(D_METHOD("set_probabilities_enabled", "val"), &WFCRules2DNative::set_probabilities_enabled);

    ClassDB::bind_method(D_METHOD("get_tile_count"), &WFCRules2DNative::get_tile_count);
    ClassDB::bind_method(D_METHOD("set_tile_count", "val"), &WFCRules2DNative::set_tile_count);

    ClassDB::bind_method(D_METHOD("set_rule", "axis_index", "tile1", "tile2", "allowed"), &WFCRules2DNative::set_rule, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_rule", "axis_index", "tile1", "tile2"), &WFCRules2DNative::get_rule);
    ClassDB::bind_method(D_METHOD("complete_all_matrices"), &WFCRules2DNative::complete_all_matrices);
    ClassDB::bind_method(D_METHOD("is_ready"), &WFCRules2DNative::is_ready);
    ClassDB::bind_method(D_METHOD("get_influence_range"), &WFCRules2DNative::get_influence_range);
    ClassDB::bind_method(D_METHOD("format"), &WFCRules2DNative::format);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "complete_matrices"), "set_complete_matrices", "get_complete_matrices");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "axes"), "set_axes", "get_axes");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "axis_matrices"), "set_axis_matrices", "get_axis_matrices");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "probabilities"), "set_probabilities", "get_probabilities");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "edge_domain", PROPERTY_HINT_RESOURCE_TYPE, "WFCBitSetNative"), "set_edge_domain", "get_edge_domain");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "probabilities_enabled"), "set_probabilities_enabled", "get_probabilities_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_count"), "set_tile_count", "get_tile_count");
}

WFCRules2DNative::WFCRules2DNative() {
    // Default axes: (0,1) and (1,0)
    axes_.append(Vector2i(0, 1));
    axes_.append(Vector2i(1, 0));
}

WFCRules2DNative::~WFCRules2DNative() {
}

void WFCRules2DNative::initialize(int tile_count, const TypedArray<Vector2i>& axes) {
    tile_count_ = tile_count;
    axes_ = axes;

    axis_matrices_.clear();
    for (int i = 0; i < axes_.size(); i++) {
        Ref<WFCBitMatrixNative> matrix;
        matrix.instantiate();
        matrix->initialize(tile_count, tile_count);
        axis_matrices_.append(matrix);
    }
}

void WFCRules2DNative::set_rule(int axis_index, int tile1, int tile2, bool allowed) {
    if (axis_index >= 0 && axis_index < axis_matrices_.size()) {
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[axis_index];
        matrix->set_bit(tile1, tile2, allowed);
    }
}

bool WFCRules2DNative::get_rule(int axis_index, int tile1, int tile2) const {
    if (axis_index >= 0 && axis_index < axis_matrices_.size()) {
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[axis_index];
        return matrix->get_bit(tile1, tile2);
    }
    return false;
}

void WFCRules2DNative::complete_all_matrices() {
    for (int i = 0; i < axis_matrices_.size(); i++) {
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[i];
        matrix->complete();
    }
}

bool WFCRules2DNative::is_ready() const {
    return tile_count_ > 0 &&
           axis_matrices_.size() == axes_.size() &&
           (!probabilities_enabled_ || probabilities_.size() == tile_count_);
}

Vector2i WFCRules2DNative::get_influence_range() const {
    Vector2i res(0, 0);

    for (int a = 0; a < axes_.size(); a++) {
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[a];
        Vector2i axis = axes_[a];

        int forward_path = matrix->get_longest_path();

        if (forward_path <= 0) {
            if (axis.x != 0) res.x = MAX_INT_32;
            if (axis.y != 0) res.y = MAX_INT_32;
            continue;
        }

        Ref<WFCBitMatrixNative> transposed = matrix->transpose();
        int backward_path = transposed->get_longest_path();

        if (backward_path <= 0) {
            if (axis.x != 0) res.x = MAX_INT_32;
            if (axis.y != 0) res.y = MAX_INT_32;
            continue;
        }

        int longest_path = forward_path > backward_path ? forward_path : backward_path;

        int x_contribution = abs(axis.x) * longest_path;
        int y_contribution = abs(axis.y) * longest_path;

        if (x_contribution > res.x) res.x = x_contribution;
        if (y_contribution > res.y) res.y = y_contribution;
    }

    return res;
}

String WFCRules2DNative::format() const {
    String res = "";

    for (int i = 0; i < axes_.size(); i++) {
        Vector2i axis = axes_[i];
        res += "Axis " + String::num_int64(i) + " (" + String::num_int64(axis.x) + "," + String::num_int64(axis.y) + "):\n";
        Ref<WFCBitMatrixNative> matrix = axis_matrices_[i];
        res += matrix->format_bits();
        res += "\n";
    }

    return res;
}

} // namespace godot

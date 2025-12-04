#ifndef WFC_RULES_2D_NATIVE_H
#define WFC_RULES_2D_NATIVE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include "wfc_bitset_native.h"
#include "wfc_bitmatrix_native.h"

namespace godot {

class WFCRules2DNative : public Resource {
    GDCLASS(WFCRules2DNative, Resource)

public:
    static constexpr int MAX_INT_32 = 2147483647;

private:
    bool complete_matrices_ = true;
    TypedArray<Vector2i> axes_;
    TypedArray<WFCBitMatrixNative> axis_matrices_;
    PackedFloat32Array probabilities_;
    Ref<WFCBitSetNative> edge_domain_;
    bool probabilities_enabled_ = false;
    int tile_count_ = 0;

protected:
    static void _bind_methods();

public:
    WFCRules2DNative();
    ~WFCRules2DNative();

    void initialize(int tile_count, const TypedArray<Vector2i>& axes);

    // Property accessors
    bool get_complete_matrices() const { return complete_matrices_; }
    void set_complete_matrices(bool val) { complete_matrices_ = val; }

    TypedArray<Vector2i> get_axes() const { return axes_; }
    void set_axes(const TypedArray<Vector2i>& val) { axes_ = val; }

    TypedArray<WFCBitMatrixNative> get_axis_matrices() const { return axis_matrices_; }
    void set_axis_matrices(const TypedArray<WFCBitMatrixNative>& val) { axis_matrices_ = val; }

    PackedFloat32Array get_probabilities() const { return probabilities_; }
    void set_probabilities(const PackedFloat32Array& val) { probabilities_ = val; }

    Ref<WFCBitSetNative> get_edge_domain() const { return edge_domain_; }
    void set_edge_domain(const Ref<WFCBitSetNative>& val) { edge_domain_ = val; }

    bool get_probabilities_enabled() const { return probabilities_enabled_; }
    void set_probabilities_enabled(bool val) { probabilities_enabled_ = val; }

    int get_tile_count() const { return tile_count_; }
    void set_tile_count(int val) { tile_count_ = val; }

    // Methods
    void set_rule(int axis_index, int tile1, int tile2, bool allowed = true);
    bool get_rule(int axis_index, int tile1, int tile2) const;
    void complete_all_matrices();
    bool is_ready() const;
    Vector2i get_influence_range() const;
    String format() const;
};

} // namespace godot

#endif // WFC_RULES_2D_NATIVE_H

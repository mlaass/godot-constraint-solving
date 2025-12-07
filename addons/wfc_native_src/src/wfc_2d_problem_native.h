#ifndef WFC_2D_PROBLEM_NATIVE_H
#define WFC_2D_PROBLEM_NATIVE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "wfc_problem_native.h"
#include "wfc_rules_2d_native.h"

namespace godot {

// AC4 Binary Constraint for 2D problems
class WFC2DAC4BinaryConstraintNative : public WFCProblemAC4BinaryConstraintNative {
    GDCLASS(WFC2DAC4BinaryConstraintNative, WFCProblemAC4BinaryConstraintNative)

private:
    Vector2i axis_;
    Rect2i problem_size_;
    TypedArray<PackedInt64Array> allowed_tiles_;

protected:
    static void _bind_methods();

public:
    WFC2DAC4BinaryConstraintNative();
    ~WFC2DAC4BinaryConstraintNative();

    void initialize(const Vector2i& axis, const Vector2i& size, const Ref<WFCBitMatrixNative>& axis_matrix);

    int get_cell_id(const Vector2i& pos) const;
    Vector2i get_cell_pos(int cell_id) const;

    virtual int get_dependent(int cell_id) override;
    virtual int get_dependency(int cell_id) override;
    virtual PackedInt64Array get_allowed(int dependency_variant) override;
};

// Main 2D WFC Problem implementation
class WFC2DProblemNative : public WFCProblemNative {
    GDCLASS(WFC2DProblemNative, WFCProblemNative)

private:
    Ref<WFCRules2DNative> rules_;
    Rect2i rect_;
    Rect2i renderable_rect_;
    Rect2i edges_rect_;
    TypedArray<Vector2i> axes_;
    TypedArray<WFCBitMatrixNative> axis_matrices_;
    int tile_count_ = 0;

    // Precondition data (set from GDScript before solver init)
    TypedArray<WFCBitSetNative> precondition_domains_;  // Per-cell domains (null = full domain)
    PackedInt64Array precondition_solutions_;            // Pre-solved cells (-1 = not solved)

    // For multithreaded solving - rects to read from completed neighbors
    TypedArray<Rect2i> init_read_rects_;

    // Helper for split()
    static PackedInt64Array split_range(int first, int size, int partitions, int min_partition_size);

protected:
    static void _bind_methods();

public:
    WFC2DProblemNative();
    ~WFC2DProblemNative();

    void initialize(const Ref<WFCRules2DNative>& rules, const Rect2i& rect);

    // Property accessors
    Ref<WFCRules2DNative> get_rules() const { return rules_; }
    void set_rules(const Ref<WFCRules2DNative>& val) { rules_ = val; }

    Rect2i get_rect() const { return rect_; }
    void set_rect(const Rect2i& val) { rect_ = val; }

    Rect2i get_renderable_rect() const { return renderable_rect_; }
    void set_renderable_rect(const Rect2i& val) { renderable_rect_ = val; }

    Rect2i get_edges_rect() const { return edges_rect_; }
    void set_edges_rect(const Rect2i& val) { edges_rect_ = val; }

    TypedArray<Rect2i> get_init_read_rects() const { return init_read_rects_; }
    void set_init_read_rects(const TypedArray<Rect2i>& val) { init_read_rects_ = val; }

    TypedArray<Vector2i> get_axes() const { return axes_; }
    TypedArray<WFCBitMatrixNative> get_axis_matrices() const { return axis_matrices_; }

    // Coordinate conversion
    int coord_to_id(const Vector2i& coord) const;
    Vector2i id_to_coord(int id) const;

    // Maximal distances along X and Y axes between a cell and its immediate dependencies
    Vector2i get_dependencies_range() const;

    // Split problem into sub-problems for parallel solving
    virtual TypedArray<WFCProblemSubProblemNative> split(int concurrency_limit) override;

    // Precondition methods (call before solver initialize)
    void set_precondition_domain(int cell_id, const Ref<WFCBitSetNative>& domain);
    void set_precondition_solution(int cell_id, int solution);
    void clear_preconditions();

    // WFCProblemNative overrides
    virtual int get_cell_count() override;
    virtual Ref<WFCBitSetNative> get_default_domain() override;
    virtual void populate_initial_state(const Ref<WFCSolverStateNative>& state) override;
    virtual Ref<WFCBitSetNative> compute_cell_domain(const Ref<WFCSolverStateNative>& state, int cell_id) override;
    virtual void mark_related_cells(int changed_cell_id, const Callable& mark_cell) override;
    virtual PackedInt64Array get_related_cells(int changed_cell_id) override;
    virtual int pick_divergence_option(TypedArray<int> options) override;
    virtual bool supports_ac4() override;
    virtual TypedArray<WFCProblemAC4BinaryConstraintNative> get_ac4_binary_constraints() override;
};

} // namespace godot

#endif // WFC_2D_PROBLEM_NATIVE_H

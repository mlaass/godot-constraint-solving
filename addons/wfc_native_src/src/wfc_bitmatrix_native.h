#ifndef WFC_BITMATRIX_NATIVE_H
#define WFC_BITMATRIX_NATIVE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include "wfc_bitset_native.h"

namespace godot {

class WFCBitMatrixNative : public Resource {
    GDCLASS(WFCBitMatrixNative, Resource)

private:
    TypedArray<WFCBitSetNative> rows;
    int width_ = 0;
    int height_ = 0;

protected:
    static void _bind_methods();

public:
    WFCBitMatrixNative();
    ~WFCBitMatrixNative();

    void initialize(int w, int h);

    Ref<WFCBitMatrixNative> copy() const;
    bool get_bit(int x, int y) const;
    void set_bit(int x, int y, bool value = true);
    Ref<WFCBitMatrixNative> transpose() const;
    Ref<WFCBitSetNative> transform(const Ref<WFCBitSetNative>& input) const;
    void complete();
    String format_bits() const;
    int get_longest_path() const;

    // Property getters/setters
    TypedArray<WFCBitSetNative> get_rows() const { return rows; }
    void set_rows(const TypedArray<WFCBitSetNative>& val) { rows = val; }
    int get_width() const { return width_; }
    void set_width(int val) { width_ = val; }
    int get_height() const { return height_; }
    void set_height(int val) { height_ = val; }

    // Direct row access
    Ref<WFCBitSetNative> get_row(int index) const;
};

} // namespace godot

#endif // WFC_BITMATRIX_NATIVE_H

#include "wfc_bitmatrix_native.h"
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void WFCBitMatrixNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "w", "h"), &WFCBitMatrixNative::initialize);
    ClassDB::bind_method(D_METHOD("copy"), &WFCBitMatrixNative::copy);
    ClassDB::bind_method(D_METHOD("get_bit", "x", "y"), &WFCBitMatrixNative::get_bit);
    ClassDB::bind_method(D_METHOD("set_bit", "x", "y", "value"), &WFCBitMatrixNative::set_bit, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("transpose"), &WFCBitMatrixNative::transpose);
    ClassDB::bind_method(D_METHOD("transform", "input"), &WFCBitMatrixNative::transform);
    ClassDB::bind_method(D_METHOD("complete"), &WFCBitMatrixNative::complete);
    ClassDB::bind_method(D_METHOD("format_bits"), &WFCBitMatrixNative::format_bits);
    ClassDB::bind_method(D_METHOD("get_longest_path"), &WFCBitMatrixNative::get_longest_path);

    // Properties
    ClassDB::bind_method(D_METHOD("get_rows"), &WFCBitMatrixNative::get_rows);
    ClassDB::bind_method(D_METHOD("set_rows", "val"), &WFCBitMatrixNative::set_rows);
    ClassDB::bind_method(D_METHOD("get_width"), &WFCBitMatrixNative::get_width);
    ClassDB::bind_method(D_METHOD("set_width", "val"), &WFCBitMatrixNative::set_width);
    ClassDB::bind_method(D_METHOD("get_height"), &WFCBitMatrixNative::get_height);
    ClassDB::bind_method(D_METHOD("set_height", "val"), &WFCBitMatrixNative::set_height);
    ClassDB::bind_method(D_METHOD("get_row", "index"), &WFCBitMatrixNative::get_row);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rows", PROPERTY_HINT_ARRAY_TYPE, "WFCBitSetNative"), "set_rows", "get_rows");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
}

WFCBitMatrixNative::WFCBitMatrixNative() : width_(0), height_(0) {
}

WFCBitMatrixNative::~WFCBitMatrixNative() {
}

void WFCBitMatrixNative::initialize(int w, int h) {
    width_ = w;
    height_ = h;
    rows.clear();

    for (int i = 0; i < height_; i++) {
        Ref<WFCBitSetNative> row;
        row.instantiate();
        row->initialize(width_);
        rows.append(row);
    }
}

Ref<WFCBitMatrixNative> WFCBitMatrixNative::copy() const {
    Ref<WFCBitMatrixNative> res;
    res.instantiate();

    res->width_ = width_;
    res->height_ = height_;

    for (int i = 0; i < height_; i++) {
        Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[i]);
        if (row.is_valid()) {
            res->rows.append(row->copy());
        }
    }

    return res;
}

bool WFCBitMatrixNative::get_bit(int x, int y) const {
    if (y < 0 || y >= height_) return false;

    Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[y]);
    if (row.is_valid()) {
        return row->get_bit(x);
    }
    return false;
}

void WFCBitMatrixNative::set_bit(int x, int y, bool value) {
    if (y < 0 || y >= height_) return;

    Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[y]);
    if (row.is_valid()) {
        row->set_bit(x, value);
    }
}

Ref<WFCBitMatrixNative> WFCBitMatrixNative::transpose() const {
    Ref<WFCBitMatrixNative> res;
    res.instantiate();
    res->initialize(height_, width_);

    for (int y = 0; y < height_; y++) {
        Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[y]);
        if (row.is_valid()) {
            // Use for_each_set_bit to avoid to_array() allocation
            row->for_each_set_bit([&](int x) {
                res->set_bit(y, x, true);
            });
        }
    }

    return res;
}

Ref<WFCBitSetNative> WFCBitMatrixNative::transform(const Ref<WFCBitSetNative>& input) const {
    Ref<WFCBitSetNative> res;
    res.instantiate();
    res->initialize(width_);

    if (input.is_null()) return res;

    // Use for_each_set_bit to avoid to_array() allocation
    input->for_each_set_bit([&](int y) {
        if (y >= 0 && y < height_) {
            Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[y]);
            if (row.is_valid()) {
                res->union_in_place(row);
            }
        }
    });

    return res;
}

void WFCBitMatrixNative::complete() {
    for (int i = 0; i < height_; i++) {
        Ref<WFCBitSetNative> ri = Object::cast_to<WFCBitSetNative>(rows[i]);
        if (ri.is_null()) continue;

        for (int j = 0; j < height_; j++) {
            if (i != j) {
                Ref<WFCBitSetNative> rj = Object::cast_to<WFCBitSetNative>(rows[j]);
                if (rj.is_valid() && ri->intersects_with(rj)) {
                    rj->union_in_place(ri);
                }
            }
        }
    }
}

String WFCBitMatrixNative::format_bits() const {
    String res = "(";

    for (int i = 0; i < height_; i++) {
        res += "\n\t";
        Ref<WFCBitSetNative> row = Object::cast_to<WFCBitSetNative>(rows[i]);
        if (row.is_valid()) {
            res += row->format_bits();
        }
        res += ",";
    }

    res += "\n)";
    return res;
}

int WFCBitMatrixNative::get_longest_path() const {
    if (width_ != height_) return -1;

    Ref<WFCBitSetNative> all_set;
    all_set.instantiate();
    all_set->initialize(width_, true);

    int longest_known_path = -1;

    for (int start = 0; start < width_; start++) {
        Ref<WFCBitSetNative> cur;
        cur.instantiate();
        cur->initialize(width_);
        cur->set_bit(start, true);

        for (int path_len = 1; path_len < width_; path_len++) {
            cur = transform(cur);
            if (cur->equals(all_set)) {
                if (path_len > longest_known_path) {
                    longest_known_path = path_len;
                }
                break;
            }
        }
    }

    return longest_known_path;
}

Ref<WFCBitSetNative> WFCBitMatrixNative::get_row(int index) const {
    if (index < 0 || index >= rows.size()) {
        return Ref<WFCBitSetNative>();
    }
    return Object::cast_to<WFCBitSetNative>(rows[index]);
}

} // namespace godot

#include "wfc_bitset_native.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>

namespace godot {

void WFCBitSetNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "size_val", "default_val"), &WFCBitSetNative::initialize, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("set_all"), &WFCBitSetNative::set_all);
    ClassDB::bind_method(D_METHOD("copy"), &WFCBitSetNative::copy);
    ClassDB::bind_method(D_METHOD("equals", "other"), &WFCBitSetNative::equals);

    ClassDB::bind_method(D_METHOD("union_in_place", "other"), &WFCBitSetNative::union_in_place);
    ClassDB::bind_method(D_METHOD("union_with", "other"), &WFCBitSetNative::union_with);
    ClassDB::bind_method(D_METHOD("intersect_in_place", "other"), &WFCBitSetNative::intersect_in_place);
    ClassDB::bind_method(D_METHOD("intersect", "other"), &WFCBitSetNative::intersect);
    ClassDB::bind_method(D_METHOD("xor_in_place", "other"), &WFCBitSetNative::xor_in_place);
    ClassDB::bind_method(D_METHOD("xor_with", "other"), &WFCBitSetNative::xor_with);

    ClassDB::bind_method(D_METHOD("invert"), &WFCBitSetNative::invert);
    ClassDB::bind_method(D_METHOD("is_superset_of", "subset"), &WFCBitSetNative::is_superset_of);

    ClassDB::bind_method(D_METHOD("get_bit", "bit_num"), &WFCBitSetNative::get_bit);
    ClassDB::bind_method(D_METHOD("set_bit", "bit_num", "value"), &WFCBitSetNative::set_bit, DEFVAL(true));

    ClassDB::bind_method(D_METHOD("get_only_set_bit"), &WFCBitSetNative::get_only_set_bit);
    ClassDB::bind_method(D_METHOD("is_empty"), &WFCBitSetNative::is_empty);
    ClassDB::bind_method(D_METHOD("intersects_with", "other"), &WFCBitSetNative::intersects_with);

    ClassDB::bind_method(D_METHOD("get_elem", "n"), &WFCBitSetNative::get_elem);
    ClassDB::bind_method(D_METHOD("to_array"), &WFCBitSetNative::to_array);
    ClassDB::bind_method(D_METHOD("iterator"), &WFCBitSetNative::iterator);
    ClassDB::bind_method(D_METHOD("count_set_bits", "pass_if_more_than"), &WFCBitSetNative::count_set_bits, DEFVAL(MAX_INT_VAL));
    ClassDB::bind_method(D_METHOD("format_bits"), &WFCBitSetNative::format_bits);

    // Properties
    ClassDB::bind_method(D_METHOD("get_data0"), &WFCBitSetNative::get_data0);
    ClassDB::bind_method(D_METHOD("set_data0", "val"), &WFCBitSetNative::set_data0);
    ClassDB::bind_method(D_METHOD("get_data1"), &WFCBitSetNative::get_data1);
    ClassDB::bind_method(D_METHOD("set_data1", "val"), &WFCBitSetNative::set_data1);
    ClassDB::bind_method(D_METHOD("get_dataX"), &WFCBitSetNative::get_dataX);
    ClassDB::bind_method(D_METHOD("set_dataX", "val"), &WFCBitSetNative::set_dataX);
    ClassDB::bind_method(D_METHOD("get_size"), &WFCBitSetNative::get_size);
    ClassDB::bind_method(D_METHOD("set_size", "val"), &WFCBitSetNative::set_size);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "data0"), "set_data0", "get_data0");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "data1"), "set_data1", "get_data1");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "dataX"), "set_dataX", "get_dataX");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "size"), "set_size", "get_size");

    // Constants
    BIND_CONSTANT(ONLY_BIT_MORE_BITS_SET);
    BIND_CONSTANT(ONLY_BIT_NO_BITS_SET);
}

WFCBitSetNative::WFCBitSetNative() : data0(0), data1(0), size_(0) {
}

WFCBitSetNative::~WFCBitSetNative() {
}

int64_t WFCBitSetNative::n_bits_set(int n) {
    int64_t res = 0;
    for (int i = 0; i < n; i++) {
        res |= (1LL << i);
    }
    return res;
}

int WFCBitSetNative::get_first_set_bit_index(int64_t bits) {
    if (bits == 0) return -1;
    // Use __builtin_ctzll for O(1) first set bit lookup (compiles to BSF/TZCNT instruction)
    return __builtin_ctzll(static_cast<uint64_t>(bits));
}

bool WFCBitSetNative::is_pot(int64_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

int WFCBitSetNative::count_bits_helper(int64_t value, int initial, int pass_if_more_than) const {
    // Use compiler builtin for fast, correct bit counting (compiles to POPCNT instruction)
    int count = __builtin_popcountll(static_cast<uint64_t>(value));
    return initial + count;
}

void WFCBitSetNative::initialize(int size_val, bool default_val) {
    if (size_val == 0) {
        size_ = 0;
        data0 = 0;
        data1 = 0;
        dataX.clear();
        return;
    }

    size_ = size_val;

    int fullElems = size_val / BITS_PER_INT;
    int lastElemBits = size_val % BITS_PER_INT;

    int totalElems = fullElems;
    if (lastElemBits > 0) {
        totalElems += 1;
    }

    if (totalElems > STATIC_ELEMS) {
        dataX.resize(totalElems - STATIC_ELEMS);
    } else {
        dataX.clear();
    }

    data0 = 0;
    data1 = 0;

    if (default_val) {
        set_all();
    }
}

void WFCBitSetNative::set_all() {
    int fullElems = size_ / BITS_PER_INT;
    int lastElemBits = size_ % BITS_PER_INT;

    if (fullElems == 0) {
        data0 = n_bits_set(lastElemBits);
        return;
    } else {
        data0 = ALL_SET;
        fullElems -= 1;
    }

    if (fullElems == 0) {
        data1 = n_bits_set(lastElemBits);
        return;
    } else {
        data1 = ALL_SET;
        fullElems -= 1;
    }

    for (int i = 0; i < fullElems; i++) {
        dataX.set(i, ALL_SET);
    }

    if (lastElemBits > 0) {
        dataX.set(fullElems, n_bits_set(lastElemBits));
    }
}

Ref<WFCBitSetNative> WFCBitSetNative::copy() const {
    Ref<WFCBitSetNative> res;
    res.instantiate();

    res->data0 = data0;
    res->data1 = data1;

    if (size_ > STATIC_BITS) {
        res->dataX.resize(dataX.size());
        for (int i = 0; i < dataX.size(); i++) {
            res->dataX.set(i, dataX[i]);
        }
    }

    res->size_ = size_;

    return res;
}

bool WFCBitSetNative::equals(const Ref<WFCBitSetNative>& other) const {
    if (other.is_null()) return false;

    if (other->size_ != size_) return false;
    if (other->data0 != data0) return false;
    if (other->data1 != data1) return false;

    for (int i = 0; i < dataX.size(); i++) {
        if (dataX[i] != other->dataX[i]) {
            return false;
        }
    }

    return true;
}

void WFCBitSetNative::union_in_place(const Ref<WFCBitSetNative>& other) {
    if (other.is_null()) return;

    data0 |= other->data0;
    data1 |= other->data1;

    if (size_ > STATIC_BITS) {
        for (int i = 0; i < other->dataX.size() && i < dataX.size(); i++) {
            dataX.set(i, dataX[i] | other->dataX[i]);
        }
    }
}

Ref<WFCBitSetNative> WFCBitSetNative::union_with(const Ref<WFCBitSetNative>& other) const {
    if (other.is_null()) return copy();

    if (other->size_ > size_) {
        return other->union_with(Ref<WFCBitSetNative>(const_cast<WFCBitSetNative*>(this)));
    }

    Ref<WFCBitSetNative> res = copy();
    res->union_in_place(other);
    return res;
}

void WFCBitSetNative::intersect_in_place(const Ref<WFCBitSetNative>& other) {
    if (other.is_null()) return;

    data0 &= other->data0;
    data1 &= other->data1;

    if (size_ > STATIC_BITS) {
        for (int i = 0; i < dataX.size(); i++) {
            if (i < other->dataX.size()) {
                dataX.set(i, dataX[i] & other->dataX[i]);
            } else {
                dataX.set(i, 0);
            }
        }
    }
}

Ref<WFCBitSetNative> WFCBitSetNative::intersect(const Ref<WFCBitSetNative>& other) const {
    if (other.is_null()) return copy();

    if (other->size_ < size_) {
        return other->intersect(Ref<WFCBitSetNative>(const_cast<WFCBitSetNative*>(this)));
    }

    Ref<WFCBitSetNative> res = copy();
    res->intersect_in_place(other);
    return res;
}

void WFCBitSetNative::xor_in_place(const Ref<WFCBitSetNative>& other) {
    if (other.is_null()) return;

    data0 ^= other->data0;
    data1 ^= other->data1;

    if (size_ > STATIC_BITS) {
        for (int i = 0; i < dataX.size() && i < other->dataX.size(); i++) {
            dataX.set(i, dataX[i] ^ other->dataX[i]);
        }
    }
}

Ref<WFCBitSetNative> WFCBitSetNative::xor_with(const Ref<WFCBitSetNative>& other) const {
    Ref<WFCBitSetNative> res = copy();
    res->xor_in_place(other);
    return res;
}

Ref<WFCBitSetNative> WFCBitSetNative::invert() const {
    Ref<WFCBitSetNative> res;
    res.instantiate();
    res->initialize(size_, true);
    res->xor_in_place(Ref<WFCBitSetNative>(const_cast<WFCBitSetNative*>(this)));
    return res;
}

bool WFCBitSetNative::is_superset_of(const Ref<WFCBitSetNative>& subset) const {
    if (subset.is_null()) return true;

    if ((data0 & subset->data0) != subset->data0) return false;
    if ((data1 & subset->data1) != subset->data1) return false;

    for (int i = 0; i < dataX.size(); i++) {
        int64_t sub_val = (i < subset->dataX.size()) ? subset->dataX[i] : 0;
        if ((dataX[i] & sub_val) != sub_val) {
            return false;
        }
    }

    return true;
}

bool WFCBitSetNative::get_bit(int bit_num) const {
    if (bit_num < 0 || bit_num >= size_) return false;

    if (bit_num < STATIC_BITS) {
        if (bit_num < BITS_PER_INT) {
            return (data0 & (1LL << bit_num)) != 0;
        } else {
            return (data1 & (1LL << (bit_num - BITS_PER_INT))) != 0;
        }
    }

    int adjusted = bit_num - STATIC_BITS;
    int el_num = adjusted / BITS_PER_INT;
    int el_bit_num = adjusted % BITS_PER_INT;

    if (el_num >= dataX.size()) return false;

    return (dataX[el_num] & (1LL << el_bit_num)) != 0;
}

void WFCBitSetNative::set_bit(int bit_num, bool value) {
    if (bit_num < 0 || bit_num >= size_) return;

    if (bit_num < STATIC_BITS) {
        if (bit_num < BITS_PER_INT) {
            if (value) {
                data0 |= (1LL << bit_num);
            } else {
                data0 &= ~(1LL << bit_num);
            }
        } else {
            int shifted = bit_num - BITS_PER_INT;
            if (value) {
                data1 |= (1LL << shifted);
            } else {
                data1 &= ~(1LL << shifted);
            }
        }
        return;
    }

    int adjusted = bit_num - STATIC_BITS;
    int el_index = adjusted / BITS_PER_INT;
    int bit_index = adjusted % BITS_PER_INT;

    if (el_index >= dataX.size()) return;

    if (value) {
        dataX.set(el_index, dataX[el_index] | (1LL << bit_index));
    } else {
        dataX.set(el_index, dataX[el_index] & ~(1LL << bit_index));
    }
}

int WFCBitSetNative::get_only_set_bit() const {
    int elem_index = -1;

    if (data0 != 0) {
        if (!is_pot(data0)) return ONLY_BIT_MORE_BITS_SET;
        elem_index = 0;
    }

    if (data1 != 0) {
        if (elem_index >= 0 || !is_pot(data1)) return ONLY_BIT_MORE_BITS_SET;
        elem_index = 1;
    }

    if (size_ > STATIC_BITS) {
        for (int i = 0; i < dataX.size(); i++) {
            int64_t el = dataX[i];
            if (el != 0) {
                if (elem_index >= 0 || !is_pot(el)) return ONLY_BIT_MORE_BITS_SET;
                elem_index = i + STATIC_ELEMS;
            }
        }
    }

    if (elem_index < 0) return ONLY_BIT_NO_BITS_SET;

    return get_first_set_bit_index(get_elem(elem_index)) + elem_index * BITS_PER_INT;
}

bool WFCBitSetNative::is_empty() const {
    if (data0 != 0 || data1 != 0) return false;

    if (size_ > STATIC_BITS) {
        for (int i = 0; i < dataX.size(); i++) {
            if (dataX[i] != 0) return false;
        }
    }

    return true;
}

bool WFCBitSetNative::intersects_with(const Ref<WFCBitSetNative>& other) const {
    if (other.is_null()) return false;

    if ((data0 & other->data0) != 0) return true;
    if ((data1 & other->data1) != 0) return true;

    int min_size = std::min((int)dataX.size(), (int)other->dataX.size());
    for (int i = 0; i < min_size; i++) {
        if ((dataX[i] & other->dataX[i]) != 0) return true;
    }

    return false;
}

int64_t WFCBitSetNative::get_elem(int n) const {
    switch (n) {
        case 0: return data0;
        case 1: return data1;
        default:
            if (n - STATIC_ELEMS < dataX.size()) {
                return dataX[n - STATIC_ELEMS];
            }
            return 0;
    }
}

PackedInt64Array WFCBitSetNative::to_array() const {
    PackedInt64Array res;
    // Pre-estimate capacity based on popcount for fewer reallocations
    int estimated = count_set_bits(size_);
    res.resize(estimated);
    int count = 0;

    // Use optimized bit iteration with __builtin_ctzll
    for_each_set_bit([&](int bit) {
        if (count < estimated) {
            res.set(count, bit);
        } else {
            res.append(bit);
        }
        count++;
    });

    // Resize to actual count (in case estimate was wrong)
    if (count < estimated) {
        res.resize(count);
    }

    return res;
}

PackedInt64Array WFCBitSetNative::iterator() const {
    return to_array();
}

int WFCBitSetNative::count_set_bits(int pass_if_more_than) const {
    int res = count_bits_helper(data0, 0, pass_if_more_than);

    // Fixed: Unconditionally check data1 when size > 64
    if (size_ > 64) {
        int data1_bits = __builtin_popcountll(static_cast<uint64_t>(data1));
        res = res + data1_bits;
    }

    // Check dataX for sizes > 128
    if (size_ > 128 && res <= pass_if_more_than) {
        for (int i = 0; i < dataX.size(); i++) {
            res = count_bits_helper(dataX[i], res, pass_if_more_than);
            if (res > pass_if_more_than) break;
        }
    }

    return res;
}

String WFCBitSetNative::format_bits() const {
    String res = "(";

    for (int i = 0; i < size_; i++) {
        if (get_bit(i)) {
            res += "1, ";
        } else {
            res += "0, ";
        }
    }

    res += ")";
    return res;
}

} // namespace godot

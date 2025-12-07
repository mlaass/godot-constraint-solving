#ifndef WFC_BITSET_NATIVE_H
#define WFC_BITSET_NATIVE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <cstdint>

namespace godot {

class WFCBitSetNative : public Resource {
    GDCLASS(WFCBitSetNative, Resource)

public:
    static constexpr int64_t BITS_PER_INT = 64;
    static constexpr int64_t MAX_INT_VAL = 9223372036854775807LL;
    static constexpr int64_t ALL_SET = ~0LL;
    static constexpr int STATIC_ELEMS = 2;
    static constexpr int STATIC_BITS = BITS_PER_INT * STATIC_ELEMS;

    static constexpr int ONLY_BIT_MORE_BITS_SET = -2;
    static constexpr int ONLY_BIT_NO_BITS_SET = -1;

private:
    int64_t data0 = 0;
    int64_t data1 = 0;
    PackedInt64Array dataX;
    int size_ = 0;

    static int64_t n_bits_set(int n);
    static int get_first_set_bit_index(int64_t bits);
    static bool is_pot(int64_t x);
    int count_bits_helper(int64_t value, int initial, int pass_if_more_than) const;

protected:
    static void _bind_methods();

public:
    WFCBitSetNative();
    ~WFCBitSetNative();

    void initialize(int size_val, bool default_val = false);

    void set_all();
    Ref<WFCBitSetNative> copy() const;
    bool equals(const Ref<WFCBitSetNative>& other) const;

    void union_in_place(const Ref<WFCBitSetNative>& other);
    Ref<WFCBitSetNative> union_with(const Ref<WFCBitSetNative>& other) const;

    void intersect_in_place(const Ref<WFCBitSetNative>& other);
    Ref<WFCBitSetNative> intersect(const Ref<WFCBitSetNative>& other) const;

    void xor_in_place(const Ref<WFCBitSetNative>& other);
    Ref<WFCBitSetNative> xor_with(const Ref<WFCBitSetNative>& other) const;

    Ref<WFCBitSetNative> invert() const;
    bool is_superset_of(const Ref<WFCBitSetNative>& subset) const;

    bool get_bit(int bit_num) const;
    void set_bit(int bit_num, bool value = true);

    int get_only_set_bit() const;
    bool is_empty() const;
    bool intersects_with(const Ref<WFCBitSetNative>& other) const;

    int64_t get_elem(int n) const;
    PackedInt64Array to_array() const;
    PackedInt64Array iterator() const;
    int count_set_bits(int pass_if_more_than = 2147483647) const;
    String format_bits() const;

    // High-performance bit iteration (template for internal C++ use)
    // Calls callback(int bit_index) for each set bit, using __builtin_ctzll
    template<typename Func>
    void for_each_set_bit(Func&& callback) const {
        // Process data0
        uint64_t word = static_cast<uint64_t>(data0);
        while (word) {
            int bit = __builtin_ctzll(word);  // Count trailing zeros - O(1)
            callback(bit);
            word &= word - 1;  // Clear lowest set bit
        }

        // Process data1 if size > 64
        if (size_ > 64) {
            word = static_cast<uint64_t>(data1);
            while (word) {
                int bit = __builtin_ctzll(word);
                callback(64 + bit);
                word &= word - 1;
            }
        }

        // Process dataX if size > 128
        if (size_ > 128) {
            for (int i = 0; i < dataX.size(); i++) {
                word = static_cast<uint64_t>(dataX[i]);
                int base = 128 + i * 64;
                while (word) {
                    int bit = __builtin_ctzll(word);
                    callback(base + bit);
                    word &= word - 1;
                }
            }
        }
    }

    // Property getters/setters
    int64_t get_data0() const { return data0; }
    void set_data0(int64_t val) { data0 = val; }
    int64_t get_data1() const { return data1; }
    void set_data1(int64_t val) { data1 = val; }
    PackedInt64Array get_dataX() const { return dataX; }
    void set_dataX(const PackedInt64Array& val) { dataX = val; }
    int get_size() const { return size_; }
    void set_size(int val) { size_ = val; }
};

} // namespace godot

#endif // WFC_BITSET_NATIVE_H

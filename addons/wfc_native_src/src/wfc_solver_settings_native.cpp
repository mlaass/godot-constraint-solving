#include "wfc_solver_settings_native.h"
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void WFCSolverSettingsNative::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_sparse_history_enabled"), &WFCSolverSettingsNative::is_sparse_history_enabled);

    // Property getters/setters
    ClassDB::bind_method(D_METHOD("get_allow_backtracking"), &WFCSolverSettingsNative::get_allow_backtracking);
    ClassDB::bind_method(D_METHOD("set_allow_backtracking", "val"), &WFCSolverSettingsNative::set_allow_backtracking);

    ClassDB::bind_method(D_METHOD("get_require_backtracking"), &WFCSolverSettingsNative::get_require_backtracking);
    ClassDB::bind_method(D_METHOD("set_require_backtracking", "val"), &WFCSolverSettingsNative::set_require_backtracking);

    ClassDB::bind_method(D_METHOD("get_backtracking_limit"), &WFCSolverSettingsNative::get_backtracking_limit);
    ClassDB::bind_method(D_METHOD("set_backtracking_limit", "val"), &WFCSolverSettingsNative::set_backtracking_limit);

    ClassDB::bind_method(D_METHOD("get_sparse_history_start"), &WFCSolverSettingsNative::get_sparse_history_start);
    ClassDB::bind_method(D_METHOD("set_sparse_history_start", "val"), &WFCSolverSettingsNative::set_sparse_history_start);

    ClassDB::bind_method(D_METHOD("get_sparse_history_interval"), &WFCSolverSettingsNative::get_sparse_history_interval);
    ClassDB::bind_method(D_METHOD("set_sparse_history_interval", "val"), &WFCSolverSettingsNative::set_sparse_history_interval);

    ClassDB::bind_method(D_METHOD("get_force_ac3"), &WFCSolverSettingsNative::get_force_ac3);
    ClassDB::bind_method(D_METHOD("set_force_ac3", "val"), &WFCSolverSettingsNative::set_force_ac3);

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_backtracking"), "set_allow_backtracking", "get_allow_backtracking");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "require_backtracking"), "set_require_backtracking", "get_require_backtracking");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "backtracking_limit"), "set_backtracking_limit", "get_backtracking_limit");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sparse_history_start"), "set_sparse_history_start", "get_sparse_history_start");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sparse_history_interval"), "set_sparse_history_interval", "get_sparse_history_interval");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "force_ac3"), "set_force_ac3", "get_force_ac3");
}

WFCSolverSettingsNative::WFCSolverSettingsNative() {
}

WFCSolverSettingsNative::~WFCSolverSettingsNative() {
}

bool WFCSolverSettingsNative::is_sparse_history_enabled() const {
    return sparse_history_start_ > 0;
}

} // namespace godot

#ifndef WFC_SOLVER_SETTINGS_NATIVE_H
#define WFC_SOLVER_SETTINGS_NATIVE_H

#include <godot_cpp/classes/resource.hpp>

namespace godot {

class WFCSolverSettingsNative : public Resource {
    GDCLASS(WFCSolverSettingsNative, Resource)

private:
    bool allow_backtracking_ = true;
    bool require_backtracking_ = false;
    int backtracking_limit_ = -1;
    int sparse_history_start_ = 10;
    int sparse_history_interval_ = 10;
    bool force_ac3_ = true;

protected:
    static void _bind_methods();

public:
    WFCSolverSettingsNative();
    ~WFCSolverSettingsNative();

    bool is_sparse_history_enabled() const;

    // Property getters/setters
    bool get_allow_backtracking() const { return allow_backtracking_; }
    void set_allow_backtracking(bool val) { allow_backtracking_ = val; }

    bool get_require_backtracking() const { return require_backtracking_; }
    void set_require_backtracking(bool val) { require_backtracking_ = val; }

    int get_backtracking_limit() const { return backtracking_limit_; }
    void set_backtracking_limit(int val) { backtracking_limit_ = val; }

    int get_sparse_history_start() const { return sparse_history_start_; }
    void set_sparse_history_start(int val) { sparse_history_start_ = val; }

    int get_sparse_history_interval() const { return sparse_history_interval_; }
    void set_sparse_history_interval(int val) { sparse_history_interval_ = val; }

    bool get_force_ac3() const { return force_ac3_; }
    void set_force_ac3(bool val) { force_ac3_ = val; }
};

} // namespace godot

#endif // WFC_SOLVER_SETTINGS_NATIVE_H

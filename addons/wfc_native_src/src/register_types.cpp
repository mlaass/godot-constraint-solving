#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "wfc_bitset_native.h"
#include "wfc_bitmatrix_native.h"
#include "wfc_solver_settings_native.h"
#include "wfc_solver_state_native.h"
#include "wfc_problem_native.h"
#include "wfc_solver_native.h"
#include "wfc_rules_2d_native.h"
#include "wfc_2d_problem_native.h"

using namespace godot;

void initialize_wfc_solver_native_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<WFCBitSetNative>();
    ClassDB::register_class<WFCBitMatrixNative>();
    ClassDB::register_class<WFCSolverSettingsNative>();
    ClassDB::register_class<WFCSolverStateNative>();
    ClassDB::register_class<WFCProblemAC4BinaryConstraintNative>();
    ClassDB::register_class<WFCProblemSubProblemNative>();
    ClassDB::register_class<WFCProblemNative>();
    ClassDB::register_class<WFCSolverNative>();
    ClassDB::register_class<WFCRules2DNative>();
    ClassDB::register_class<WFC2DAC4BinaryConstraintNative>();
    ClassDB::register_class<WFC2DProblemNative>();
}

void uninitialize_wfc_solver_native_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT wfc_solver_native_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_wfc_solver_native_module);
        init_obj.register_terminator(uninitialize_wfc_solver_native_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}

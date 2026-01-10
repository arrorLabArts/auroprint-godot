#include "register_types.h"

#include "auroprint.h"
#include "auroprint_result.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_plugin_module(ModuleInitializationLevel p_level) {
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register Auroprint classes
    ClassDB::register_class<AuroprintResult>();
    ClassDB::register_class<Auroprint>();

    // Initialize and register singleton with Engine
    Engine::get_singleton()->register_singleton("Auroprint", Auroprint::get_singleton());
}

void uninitialize_plugin_module(ModuleInitializationLevel p_level) {
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Unregister singleton
    Engine::get_singleton()->unregister_singleton("Auroprint");
}

extern "C" {
// Initialization
GDExtensionBool GDE_EXPORT plugin_library_init(GDExtensionInterfaceGetProcAddress
                                                p_get_proc_address,
                                                const GDExtensionClassLibraryPtr p_library,
                                                GDExtensionInitialization
                                                *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_plugin_module);
    init_obj.register_terminator(uninitialize_plugin_module);

    // TODO: Update to the proper ModuleInitializationLevel level for your plugin
    init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}

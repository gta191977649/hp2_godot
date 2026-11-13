#include "presentation/race_presentation.hpp"

#include <godot_cpp/godot.hpp>

extern "C" GDExtensionBool GDE_EXPORT hp2_track12_init(
    GDExtensionInterfaceGetProcAddress get_proc_address, GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
    godot::GDExtensionBinding::InitObject entry(get_proc_address, library, initialization);
    entry.register_initializer([](godot::ModuleInitializationLevel level) {
        if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE)
            godot::ClassDB::register_class<hp2::RacePresentation>();
    });
    entry.register_terminator([](godot::ModuleInitializationLevel) {});
    entry.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return entry.init();
}

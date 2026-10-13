#include "port_core.hpp"
#include "main_loop_observer.hpp"
#include "frontend_menu_observer.hpp"
#include "ps2_texture_loader.hpp"
#include "ps2_frontend_layout.hpp"
#include "ps2_movie_player.hpp"
#include "ps2_render_host.hpp"
#include "platform/not_implemented_dialog.hpp"
#include <godot_cpp/godot.hpp>

void initialize_port(godot::ModuleInitializationLevel level) {
    if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        // One call, one place: opt-in debugger reaction for untranslated recovered paths.
        hp2::platform::install_not_implemented_dialog();
        godot::ClassDB::register_class<port::godot_bridge::PortCore>();
        godot::ClassDB::register_class<port::godot_bridge::MainLoopObserver>();
        godot::ClassDB::register_class<port::godot_bridge::FrontEndMenuObserver>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2TextureLoader>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2FrontEndLayout>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2MoviePlayer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2RenderHost>();
    }
}
void terminate_port(godot::ModuleInitializationLevel) {}

extern "C" {
GDExtensionBool GDE_EXPORT ps2_port_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
        GDExtensionClassLibraryPtr library, GDExtensionInitialization* initialization) {
    godot::GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize_port);
    init.register_terminator(terminate_port);
    init.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}
}

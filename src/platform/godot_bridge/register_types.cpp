#include "port_core.hpp"
#include "main_loop_observer.hpp"
#include "frontend_menu_observer.hpp"
#include "ps2_texture_loader.hpp"
#include "ps2_frontend_layout.hpp"
#include "ps2_movie_player.hpp"
#include "ps2_render_host.hpp"
#include "ps2_car_viewer.hpp"
#include "ps2_nis_renderer.hpp"
#include "ps2_driving_lab.hpp"
#include "ps2_track_viewer.hpp"
#include "platform/not_implemented_dialog.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include <godot_cpp/godot.hpp>

void initialize_port(godot::ModuleInitializationLevel level) {
    if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        hp2::set_not_implemented_observer(+[](const char* address,const char* reason,const char*) {
            port::diagnostics::warn(address,reason,"recovered call stopped; hp2::NotImplemented propagated to owning boundary");
        });
        // One call, one place: opt-in debugger reaction for untranslated recovered paths.
        hp2::platform::install_not_implemented_dialog();
        godot::ClassDB::register_class<port::godot_bridge::PortCore>();
        godot::ClassDB::register_class<port::godot_bridge::MainLoopObserver>();
        godot::ClassDB::register_class<port::godot_bridge::FrontEndMenuObserver>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2TextureLoader>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2FrontEndLayout>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2MoviePlayer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2RenderHost>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2VehicleRenderer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2NisRenderer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2CarViewer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2DrivingRuntime>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2DrivingLab>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2TrackRenderer>();
        godot::ClassDB::register_class<port::godot_bridge::Ps2TrackViewer>();
    }
}
void terminate_port(godot::ModuleInitializationLevel level) {
    if(level==godot::MODULE_INITIALIZATION_LEVEL_SCENE){
        port::godot_bridge::Ps2DrivingRuntime::shutdown_loader();
        port::godot_bridge::Ps2VehicleRenderer::shutdown_loader();
        port::diagnostics::print_warning_summary();
        // Source workers are joined before their diagnostics code is removed.
        hp2::set_not_implemented_observer(nullptr);
    }
}

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

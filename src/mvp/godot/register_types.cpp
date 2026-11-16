#include "presentation/race_presentation.hpp"

#include "systems/audio/godot_speech_output.hpp"
#include "systems/audio/godot_audio_device.hpp"
#include "systems/camera/race_post_presentation.hpp"
#include "systems/frontend/original_video_stream.hpp"
#include <godot_cpp/godot.hpp>

extern "C" GDExtensionBool GDE_EXPORT hp2_blackbox_init(
    GDExtensionInterfaceGetProcAddress get_proc_address, GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
    godot::GDExtensionBinding::InitObject entry(get_proc_address, library, initialization);
    entry.register_initializer([](godot::ModuleInitializationLevel level) {
        if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
            godot::ClassDB::register_internal_class<hp2::OriginalSpeechPlayback>();
            godot::ClassDB::register_internal_class<hp2::OriginalSpeechStream>();
            godot::ClassDB::register_internal_class<hp2::DevicePcmPlayback>();
            godot::ClassDB::register_internal_class<hp2::DevicePcmStream>();
            godot::ClassDB::register_internal_class<hp2::OriginalVideoPlayback>();
            godot::ClassDB::register_internal_class<hp2::OriginalVideoStream>();
            godot::ClassDB::register_internal_class<hp2::RacePostPresentation>();
            godot::ClassDB::register_class<hp2::RacePresentation>();
        }
    });
    entry.register_terminator([](godot::ModuleInitializationLevel) {});
    entry.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return entry.init();
}

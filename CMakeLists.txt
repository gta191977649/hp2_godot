cmake_minimum_required(VERSION 3.24)
project(PS2PCPort VERSION 0.1.0 LANGUAGES CXX)

# Port-only standalone copy baseline; excluded from normal builds and CTest.
add_executable(port_memory_copy_benchmark EXCLUDE_FROM_ALL
  src/port/diagnostics/memory_copy_benchmark.cpp
  src/game_core/port/driving/memory.cpp src/game_core/port/driving/original_image.cpp)
target_include_directories(port_memory_copy_benchmark PRIVATE src)
target_compile_features(port_memory_copy_benchmark PRIVATE cxx_std_17)

include(CTest)
option(PORT_BUILD_GODOT "Build the optional Godot 4 bridge" OFF)
option(PORT_ENABLE_GPU_TESTS "Run windowed vehicle shader RGB checks with configured original assets" OFF)
set(_console_default OFF)
if(PORT_BUILD_GODOT AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_console_default ON)
endif()
option(HP2_ENABLE_DEBUG_CONSOLE "Enable the port-side F8 debug console" ${_console_default})
add_library(port_debug_viewers STATIC src/game_core/extensions/viewer_session.cpp)
target_include_directories(port_debug_viewers PUBLIC src)
target_compile_features(port_debug_viewers PUBLIC cxx_std_17)
set_target_properties(port_debug_viewers PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(HP2_ENABLE_DEBUG_CONSOLE OR BUILD_TESTING OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_console_vendor "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-console")
  set(_imgui "${_console_vendor}/example/thirdparty/imgui")
  add_library(port_debug_console STATIC src/game_core/extensions/debug_console.cpp
    src/game_core/extensions/debug_ui.cpp src/game_core/extensions/viewer_ui.cpp
    src/game_core/extensions/viewer_commands.cpp
    "${_console_vendor}/src/imgui_console.cpp" "${_imgui}/imgui.cpp"
    "${_imgui}/imgui_draw.cpp" "${_imgui}/imgui_widgets.cpp")
  target_include_directories(port_debug_console PUBLIC src "${_console_vendor}/include"
    "${_console_vendor}/include/imgui_console" "${_imgui}")
  target_compile_features(port_debug_console PUBLIC cxx_std_17)
  target_link_libraries(port_debug_console PUBLIC port_debug_viewers)
  set_target_properties(port_debug_console PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
set(PORT_GODOT_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/godot-cpp" CACHE PATH "Pinned godot-cpp checkout")

add_library(game_core STATIC
  src/game_core/indep/src/aerodynamics.cpp
  src/game_core/indep/src/carinfo.cpp
  src/game_core/indep/src/carstate.cpp
  src/game_core/indep/src/engine.cpp
  src/game_core/indep/src/rigidbody.cpp
  src/game_core/indep/src/suspension.cpp
  src/game_core/indep/src/table.cpp
  src/game_core/indep/src/timer.cpp
  src/game_core/indep/src/track.cpp
  src/game_core/indep/src/volumetree.cpp
  src/game_core/src/ecstasy/elighte.cpp
  src/game_core/src/skye.cpp src/game_core/src/sune.cpp src/game_core/src/ecstasy/eenvmape.cpp
  src/game_core/src/ecstasy/eenveffectse.cpp
  src/game_core/port/track/postprocess.cpp
  src/game_core/indep/src/weatherman.cpp
  src/game_core/indep/src/wheel.cpp
  src/game_core/port/driving/adapters.cpp
  src/game_core/port/driving/byte_view.cpp
  src/game_core/port/driving/unresolved_entries.cpp
  src/game_core/src/bMath.cpp
  src/game_core/src/bMatrix.cpp
  src/game_core/src/ecstasy/emathe.cpp src/game_core/port/driving/session.cpp


  src/game_core/port/driving/memory.cpp src/game_core/port/driving/original_image.cpp



  src/game_core/port/driving/native_backend.cpp
  src/game_core/port/driving/camera.cpp
  src/game_core/port/driving/water_effects.cpp
  src/game_core/port/driving/splash_audio.cpp
  src/game_core/port/driving/vehicle_flares.cpp
  src/game_core/port/driving/tire_audio.cpp
  src/game_core/port/driving/collision_audio.cpp
  src/game_core/port/driving/collision_events.cpp
  src/game_core/port/driving/vehicle_audio.cpp
  src/game_core/port/driving/surface.cpp
  src/game_core/port/driving/track_contact.cpp
  src/game_core/port/driving/static_collisions.cpp
  src/game_core/port/driving/skid_services.cpp

  src/game_core/port/scaffold.cpp src/game_core/port/police/cop_control.cpp
  src/game_core/port/engine/engine_shell.cpp src/game_core/port/engine/world_connections.cpp
  src/game_core/port/frontend/screen_manager.cpp src/game_core/indep/src/fmenus.cpp
  src/game_core/port/frontend/selection_camera.cpp
  src/game_core/port/frontend/selection_locked.cpp
  src/game_core/port/frontend/composition.cpp
  src/game_core/indep/src/frontendscreens/options.cpp
  src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp
  src/game_core/port/frontend/frontendscreens/registry_0016adb0.cpp
  src/game_core/port/frontend/frontendscreens/modeselect.cpp
  src/game_core/indep/src/eventtree.cpp
  src/game_core/port/input/joyevents.cpp
  src/game_core/indep/src/sound.cpp
  src/game_core/bSound/src/bJukebox.cpp
  src/game_core/memorycard/src/memorycard.cpp
  src/game_core/port/movieplayer/movie_audio.cpp
  src/game_core/port/assets/elab_chunks.cpp src/game_core/port/assets/hp2_tpk_texture.cpp
  src/game_core/src/ecstasy/estripdatae.cpp
  src/platform/ps2_asset_root.cpp src/platform/ps2_profile_store.cpp)
target_include_directories(game_core PUBLIC src)
include(reverse/classes/template_sources.cmake)
target_sources(game_core PRIVATE ${HP2_CLASS_TEMPLATE_SOURCES})
target_compile_features(game_core PUBLIC cxx_std_17)
set_target_properties(game_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# TASK-0018 / D008: platform-independent port runtime. Produces render::Frame and owns the
# original-asset texture catalog. Links game_core only; never godot-cpp or a backend.
add_library(port_runtime STATIC src/port/render/frame.cpp src/port/assets/catalog.cpp
  src/port/runtime/race_launch_catalog.cpp
  src/game_core/port/track/assets.cpp src/game_core/port/track/runtime.cpp src/game_core/port/track/environment.cpp src/game_core/port/track/particles.cpp src/game_core/port/driving/track_environment.cpp
  src/game_core/port/driving/car_effects.cpp
  src/port/vehicle/textures.cpp
  src/port/vehicle/catalog.cpp
  src/port/assets/vehicle_geometry.cpp src/port/vehicle/model.cpp
  src/port/runtime/frame_builder.cpp src/port/runtime/frontend_runtime.cpp
  src/port/runtime/event_tree_view.cpp src/port/runtime/selection_page.cpp)
target_include_directories(port_runtime PUBLIC src)
target_compile_features(port_runtime PUBLIC cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(port_runtime PUBLIC game_core Threads::Threads)
set_target_properties(port_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(port_runtime PRIVATE /W4)
else()
  target_compile_options(port_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Host-side MV30 decoder. Codec/runtime assets stay outside Git; original AVI is read at runtime.
set(PORT_FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/local/tools/ffmpeg/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1" CACHE PATH "Pinned LGPL FFmpeg 8.1 host SDK")
if(WIN32 AND EXISTS "${PORT_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
  add_library(port_media STATIC src/game_core/indep/src/movieplayer/movieplayer.cpp)
  target_include_directories(port_media PUBLIC src PRIVATE "${PORT_FFMPEG_ROOT}/include")
  target_compile_features(port_media PUBLIC cxx_std_17)
  foreach(component avcodec avutil swscale)
    if(MSVC)
      target_link_libraries(port_media PUBLIC "${PORT_FFMPEG_ROOT}/lib/${component}.lib")
    else()
      target_link_libraries(port_media PUBLIC "${PORT_FFMPEG_ROOT}/lib/lib${component}.dll.a")
    endif()
  endforeach()
  file(GLOB PORT_FFMPEG_DLLS "${PORT_FFMPEG_ROOT}/bin/*.dll")
elseif(NOT WIN32)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PORT_FFMPEG QUIET IMPORTED_TARGET libavcodec>=62 libavutil>=60 libswscale>=9)
    if(PORT_FFMPEG_FOUND)
      add_library(port_media STATIC src/game_core/indep/src/movieplayer/movieplayer.cpp)
      target_include_directories(port_media PUBLIC src)
      target_compile_features(port_media PUBLIC cxx_std_17)
      target_link_libraries(port_media PUBLIC PkgConfig::PORT_FFMPEG)
    endif()
  endif()
endif()
if(PORT_BUILD_GODOT AND NOT TARGET port_media)
  message(FATAL_ERROR "Front-end movies require the FFmpeg 8.1 host SDK with MV30. Set PORT_FFMPEG_ROOT; see docs/DEVELOPMENT.md.")
endif()
if(TARGET port_media)
  set_target_properties(port_media PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
if(MSVC)
  target_compile_options(game_core PRIVATE /W4)
else()
  target_compile_options(game_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
  add_executable(track_collision_geometry_tests src/tests/track_collision_geometry_tests.cpp)
  target_link_libraries(track_collision_geometry_tests PRIVATE game_core)
  add_test(NAME track_collision_geometry COMMAND track_collision_geometry_tests)
  add_executable(static_collision_tests src/tests/static_collision_tests.cpp)
  target_link_libraries(static_collision_tests PRIVATE port_runtime)
  add_test(NAME static_collisions COMMAND static_collision_tests)
  set_tests_properties(static_collisions PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(driving_contact_tests src/tests/driving_contact_tests.cpp)
  target_link_libraries(driving_contact_tests PRIVATE game_core)
  add_test(NAME driving_contacts COMMAND driving_contact_tests)
  set_tests_properties(driving_contacts PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(driving_track_tests src/tests/driving_track_tests.cpp)
  target_link_libraries(driving_track_tests PRIVATE port_runtime)
  add_test(NAME driving_tracks COMMAND driving_track_tests)
  set_tests_properties(driving_tracks PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(track_asset_tests src/tests/track_asset_tests.cpp)
  target_link_libraries(track_asset_tests PRIVATE port_runtime)
  add_test(NAME track_assets COMMAND track_asset_tests)
  add_executable(track_topology_tests src/tests/track_topology_tests.cpp)
  target_link_libraries(track_topology_tests PRIVATE game_core)
  add_test(NAME track_topology COMMAND track_topology_tests)
  add_executable(driving_engine_tests src/tests/driving_engine_tests.cpp)
  target_link_libraries(driving_engine_tests PRIVATE game_core)
  add_test(NAME driving_engine COMMAND driving_engine_tests)
  add_executable(driving_tire_tests src/tests/driving_tire_tests.cpp)
  target_link_libraries(driving_tire_tests PRIVATE game_core)
  add_test(NAME driving_tire COMMAND driving_tire_tests)
  add_executable(driving_constructor_tests src/tests/driving_constructor_tests.cpp)
  target_link_libraries(driving_constructor_tests PRIVATE game_core)
  add_test(NAME driving_constructor COMMAND driving_constructor_tests)
  add_executable(driving_schedule_tests src/tests/driving_schedule_tests.cpp)
  target_link_libraries(driving_schedule_tests PRIVATE game_core)
  add_test(NAME driving_schedule COMMAND driving_schedule_tests)
  add_executable(driving_debug_timing_tests src/tests/driving_debug_timing_tests.cpp)
  target_link_libraries(driving_debug_timing_tests PRIVATE game_core)
  add_test(NAME driving_debug_timing COMMAND driving_debug_timing_tests)
  add_executable(driving_surface_tests src/tests/driving_surface_tests.cpp)
  target_link_libraries(driving_surface_tests PRIVATE port_runtime)
  add_test(NAME driving_surfaces COMMAND driving_surface_tests)
  set_tests_properties(driving_surfaces PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(driving_body_tests src/tests/driving_body_tests.cpp)
  target_link_libraries(driving_body_tests PRIVATE game_core)
  add_test(NAME driving_body COMMAND driving_body_tests)
  add_executable(driving_mover_tests src/tests/driving_mover_tests.cpp)
  target_link_libraries(driving_mover_tests PRIVATE game_core)
  add_test(NAME driving_mover COMMAND driving_mover_tests)
  add_executable(driving_player_tests src/tests/driving_player_tests.cpp)
  target_link_libraries(driving_player_tests PRIVATE port_runtime)
  add_test(NAME driving_player COMMAND driving_player_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_player PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_skid_tests src/tests/driving_skid_tests.cpp)
  target_link_libraries(driving_skid_tests PRIVATE game_core)
  add_test(NAME driving_skids COMMAND driving_skid_tests)
  add_executable(driving_terrain_effects_tests src/tests/driving_terrain_effects_tests.cpp)
  target_link_libraries(driving_terrain_effects_tests PRIVATE port_runtime)
  add_test(NAME driving_terrain_effects COMMAND driving_terrain_effects_tests)
  add_executable(vehicle_blur_absence_tests src/tests/vehicle_blur_absence_tests.cpp)
  target_link_libraries(vehicle_blur_absence_tests PRIVATE port_runtime)
  add_test(NAME vehicle_blur_absence COMMAND vehicle_blur_absence_tests)
  set_tests_properties(vehicle_blur_absence PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(water_contact_tests src/tests/water_contact_tests.cpp)
  target_link_libraries(water_contact_tests PRIVATE port_runtime)
  add_test(NAME water_contacts COMMAND water_contact_tests)
  set_tests_properties(water_contacts PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(track12_particle_tests src/tests/track12_particle_tests.cpp)
  target_link_libraries(track12_particle_tests PRIVATE port_runtime)
  add_test(NAME track12_particles COMMAND track12_particle_tests)
  set_tests_properties(track12_particles PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(gpu_particle_emission_tests src/tests/gpu_particle_emission_tests.cpp)
  target_link_libraries(gpu_particle_emission_tests PRIVATE port_runtime)
  add_test(NAME gpu_particle_emission COMMAND gpu_particle_emission_tests)
  set_tests_properties(gpu_particle_emission PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(driving_particle_tests src/tests/driving_particle_tests.cpp)
  target_link_libraries(driving_particle_tests PRIVATE port_runtime)
  add_test(NAME driving_particles COMMAND driving_particle_tests)
  add_test(NAME driving_particles_original COMMAND driving_particle_tests --original-image)
  add_test(NAME driving_wheel_particles_assets COMMAND driving_particle_tests --original-image --wheel-assets)
  set_tests_properties(driving_wheel_particles_assets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  set_tests_properties(driving_particles_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(driving_backend_tests src/tests/driving_backend_tests.cpp)
  target_link_libraries(driving_backend_tests PRIVATE port_runtime)
  add_test(NAME driving_backend COMMAND driving_backend_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(driving_camera_tests src/tests/driving_camera_tests.cpp)
  target_link_libraries(driving_camera_tests PRIVATE port_runtime)
  add_test(NAME driving_camera COMMAND driving_camera_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_camera PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(water_effects_tests src/tests/water_effects_tests.cpp)
  target_link_libraries(water_effects_tests PRIVATE port_runtime)
  add_test(NAME water_effects COMMAND water_effects_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(water_effects PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle_wiper_tests src/tests/vehicle_wiper_tests.cpp)
  target_link_libraries(vehicle_wiper_tests PRIVATE port_runtime)
  add_test(NAME vehicle_wipers COMMAND vehicle_wiper_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(vehicle_wipers PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_tire_audio_tests src/tests/driving_tire_audio_tests.cpp)
  target_link_libraries(driving_tire_audio_tests PRIVATE port_runtime)
  add_test(NAME driving_tire_audio COMMAND driving_tire_audio_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_tire_audio PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_collision_events_tests src/tests/driving_collision_events_tests.cpp)
  target_link_libraries(driving_collision_events_tests PRIVATE port_runtime)
  add_test(NAME driving_collision_events COMMAND driving_collision_events_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_collision_events PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_collision_audio_tests src/tests/driving_collision_audio_tests.cpp)
  target_link_libraries(driving_collision_audio_tests PRIVATE port_runtime)
  add_test(NAME driving_collision_audio COMMAND driving_collision_audio_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_collision_audio PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_audio_tests src/tests/driving_audio_tests.cpp)
  target_link_libraries(driving_audio_tests PRIVATE port_runtime)
  add_test(NAME driving_audio COMMAND driving_audio_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(driving_audio PROPERTIES SKIP_RETURN_CODE 77)
  set_tests_properties(driving_backend PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(driving_init_tests src/tests/driving_init_tests.cpp)
  target_link_libraries(driving_init_tests PRIVATE game_core)
  add_test(NAME driving_init COMMAND driving_init_tests)
  add_executable(driving_solver_tests src/tests/driving_solver_tests.cpp)
  target_link_libraries(driving_solver_tests PRIVATE game_core)
  add_test(NAME driving_solver COMMAND driving_solver_tests)
  add_executable(driving_drivetrain_tests src/tests/driving_drivetrain_tests.cpp)
  target_link_libraries(driving_drivetrain_tests PRIVATE game_core)
  add_test(NAME driving_drivetrain COMMAND driving_drivetrain_tests)
  add_executable(vehicle_catalog_tests src/tests/vehicle_catalog_tests.cpp)
  target_link_libraries(vehicle_catalog_tests PRIVATE port_runtime)
  add_test(NAME vehicle_catalog COMMAND vehicle_catalog_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(vehicle_catalog PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle_base_material_tests src/tests/vehicle_base_material_tests.cpp)
  target_link_libraries(vehicle_base_material_tests PRIVATE game_core)
  add_test(NAME vehicle_base_material COMMAND vehicle_base_material_tests)
  add_executable(vehicle_marker_tests src/tests/vehicle_marker_tests.cpp)
  target_link_libraries(vehicle_marker_tests PRIVATE port_runtime)
  add_test(NAME vehicle_markers COMMAND vehicle_marker_tests)
  set_tests_properties(vehicle_markers PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(vehicle_flare_tests src/tests/vehicle_flare_tests.cpp)
  target_link_libraries(vehicle_flare_tests PRIVATE port_runtime)
  add_test(NAME vehicle_flares COMMAND vehicle_flare_tests)
  set_tests_properties(vehicle_flares PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(vehicle_model_tests src/tests/vehicle_model_tests.cpp)
  target_link_libraries(vehicle_model_tests PRIVATE port_runtime)
  add_test(NAME vehicle_model COMMAND vehicle_model_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(vehicle_model PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle_geometry_tests src/tests/vehicle_geometry_tests.cpp)
  target_link_libraries(vehicle_geometry_tests PRIVATE port_runtime)
  add_test(NAME vehicle_geometry COMMAND vehicle_geometry_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(car_render_binding_tests src/tests/car_render_binding_tests.cpp)
  target_link_libraries(car_render_binding_tests PRIVATE game_core)
  add_test(NAME car_render_bindings COMMAND car_render_binding_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(vehicle_texture_tests src/tests/vehicle_texture_tests.cpp)
  target_link_libraries(vehicle_texture_tests PRIVATE port_runtime)
  add_test(NAME vehicle_textures COMMAND vehicle_texture_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(driving_lab_tests src/tests/driving_lab_tests.cpp)
  target_link_libraries(driving_lab_tests PRIVATE port_runtime)
  add_test(NAME driving_lab COMMAND driving_lab_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(viewer_session_tests src/tests/viewer_session_tests.cpp)
  target_link_libraries(viewer_session_tests PRIVATE port_debug_viewers)
  add_test(NAME viewer_session COMMAND viewer_session_tests)
  add_executable(debug_console_tests src/tests/debug_console_tests.cpp)
  target_link_libraries(debug_console_tests PRIVATE port_debug_console)
  add_test(NAME debug_console COMMAND debug_console_tests)
  add_executable(demo_runner src/tests/demo_runner.cpp)
  target_link_libraries(demo_runner PRIVATE game_core)
  add_executable(core_tests src/tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE game_core)
  add_test(NAME core_scaffold COMMAND core_tests)
  add_executable(police_control_tests src/tests/police_control_tests.cpp)
  target_link_libraries(police_control_tests PRIVATE game_core)
  add_test(NAME police_control COMMAND police_control_tests)
  add_executable(class_template_tests src/tests/class_template_tests.cpp)
  target_link_libraries(class_template_tests PRIVATE game_core)
  add_test(NAME class_template_gaps COMMAND class_template_tests)
  add_executable(engine_shell_tests src/tests/engine_shell_tests.cpp)
  target_link_libraries(engine_shell_tests PRIVATE game_core)
  add_test(NAME engine_shell COMMAND engine_shell_tests)
  add_executable(screen_manager_tests src/tests/screen_manager_tests.cpp)
  target_link_libraries(screen_manager_tests PRIVATE game_core)
  add_test(NAME screen_manager COMMAND screen_manager_tests)
  add_executable(fmenus_menu_tests src/tests/fmenus_menu_tests.cpp)
  target_link_libraries(fmenus_menu_tests PRIVATE game_core)
  add_test(NAME fmenus_menu COMMAND fmenus_menu_tests)
  add_executable(eventtree_tests src/tests/eventtree_tests.cpp)
  target_link_libraries(eventtree_tests PRIVATE game_core)
  add_test(NAME eventtree COMMAND eventtree_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(event_name_catalog_tests src/tests/event_name_catalog_tests.cpp)
  target_link_libraries(event_name_catalog_tests PRIVATE game_core)
  add_test(NAME event_name_catalog COMMAND event_name_catalog_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

  # TASK-0021: the recovered save container and the 0x114E0 player profile. Needs no original
  # asset and no save file -- every fixture is synthetic -- so it has no WORKING_DIRECTORY.
  add_executable(memorycard_tests src/tests/memorycard_tests.cpp)
  target_link_libraries(memorycard_tests PRIVATE game_core)
  add_test(NAME memorycard COMMAND memorycard_tests)
  add_executable(frontend_modeselect_tests src/tests/frontend_modeselect_tests.cpp)
  target_link_libraries(frontend_modeselect_tests PRIVATE game_core)
  add_test(NAME frontend_modeselect COMMAND frontend_modeselect_tests)
  add_executable(frontend_actions_tests src/tests/frontend_actions_tests.cpp)
  target_link_libraries(frontend_actions_tests PRIVATE game_core)
  add_test(NAME frontend_actions COMMAND frontend_actions_tests)
  add_executable(frontend_exit_tests src/tests/frontend_exit_tests.cpp)
  target_link_libraries(frontend_exit_tests PRIVATE game_core)
  add_test(NAME frontend_exit COMMAND frontend_exit_tests)
  add_executable(frontend_scroll_tests src/tests/frontend_scroll_tests.cpp)
  target_link_libraries(frontend_scroll_tests PRIVATE game_core)
  add_test(NAME frontend_scroll COMMAND frontend_scroll_tests)
  add_executable(frontend_back_tests src/tests/frontend_back_tests.cpp)
  target_link_libraries(frontend_back_tests PRIVATE game_core)
  add_test(NAME frontend_back COMMAND frontend_back_tests)
  add_executable(frontend_viewport_tests src/tests/frontend_viewport_tests.cpp)
  target_link_libraries(frontend_viewport_tests PRIVATE game_core)
  add_test(NAME frontend_viewport COMMAND frontend_viewport_tests)
  add_executable(sound_asset_tests src/tests/sound_asset_tests.cpp)
  target_link_libraries(sound_asset_tests PRIVATE game_core)
  add_test(NAME sound_assets COMMAND sound_asset_tests)
  set_tests_properties(sound_assets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(jukebox_tests src/tests/jukebox_tests.cpp)
  target_link_libraries(jukebox_tests PRIVATE game_core)
  add_test(NAME jukebox COMMAND jukebox_tests)
  add_executable(jukebox_screen_tests src/tests/jukebox_screen_tests.cpp)
  target_link_libraries(jukebox_screen_tests PRIVATE game_core)
  add_test(NAME jukebox_screen COMMAND jukebox_screen_tests)
  add_executable(frontend_jukebox_tests src/tests/frontend_jukebox_tests.cpp)
  target_link_libraries(frontend_jukebox_tests PRIVATE port_runtime)
  add_test(NAME frontend_jukebox COMMAND frontend_jukebox_tests)
  set_tests_properties(frontend_jukebox PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frontend_audio_tests src/tests/frontend_audio_tests.cpp)
  target_link_libraries(frontend_audio_tests PRIVATE port_runtime)
  add_test(NAME frontend_audio COMMAND frontend_audio_tests)
  set_tests_properties(frontend_audio PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(ultimate_event_tests src/tests/ultimate_event_tests.cpp)
  add_executable(race_parameters_tests src/tests/race_parameters_tests.cpp)
  add_executable(world_participants_tests src/tests/world_participants_tests.cpp)
  target_link_libraries(world_participants_tests PRIVATE port_runtime)
  add_test(NAME world_participants COMMAND world_participants_tests)
  target_link_libraries(race_parameters_tests PRIVATE port_runtime)
  add_test(NAME race_parameters COMMAND race_parameters_tests)
  set_tests_properties(race_parameters PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(ultimate_event_tests PRIVATE port_runtime)
  add_test(NAME ultimate_event COMMAND ultimate_event_tests)
  set_tests_properties(ultimate_event PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(ultimate_selection_tests src/tests/ultimate_selection_tests.cpp)
  target_link_libraries(ultimate_selection_tests PRIVATE port_runtime)
  add_test(NAME ultimate_selection COMMAND ultimate_selection_tests)
  set_tests_properties(ultimate_selection PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(ultimate_selection_camera_tests src/tests/ultimate_selection_camera_tests.cpp)
  target_link_libraries(ultimate_selection_camera_tests PRIVATE game_core)
  add_test(NAME ultimate_selection_camera COMMAND ultimate_selection_camera_tests)
  set_tests_properties(ultimate_selection_camera PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(ultimate_selection_widgets_tests src/tests/ultimate_selection_widgets_tests.cpp)
  target_link_libraries(ultimate_selection_widgets_tests PRIVATE port_runtime)
  add_test(NAME ultimate_selection_widgets COMMAND ultimate_selection_widgets_tests)
  set_tests_properties(ultimate_selection_widgets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(ultimate_selection_lifecycle_tests src/tests/ultimate_selection_lifecycle_tests.cpp)
  target_link_libraries(ultimate_selection_lifecycle_tests PRIVATE game_core)
  add_test(NAME ultimate_selection_lifecycle COMMAND ultimate_selection_lifecycle_tests)
  set_tests_properties(ultimate_selection_lifecycle PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(selection_locked_tests src/tests/selection_locked_tests.cpp)
  target_link_libraries(selection_locked_tests PRIVATE game_core)
  add_test(NAME selection_locked COMMAND selection_locked_tests)
  set_tests_properties(selection_locked PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frontend_composition_tests src/tests/frontend_composition_tests.cpp)
  target_link_libraries(frontend_composition_tests PRIVATE game_core)
  add_test(NAME frontend_composition COMMAND frontend_composition_tests)
  add_executable(selection_page_tests src/tests/selection_page_tests.cpp)
  target_link_libraries(selection_page_tests PRIVATE port_runtime)
  add_test(NAME selection_page COMMAND selection_page_tests)
  set_tests_properties(selection_page PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frontend_effects_tests src/tests/frontend_effects_tests.cpp)
  target_link_libraries(frontend_effects_tests PRIVATE port_runtime)
  add_test(NAME frontend_effects COMMAND frontend_effects_tests)
  set_tests_properties(frontend_effects PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frontend_name_entry_tests src/tests/frontend_name_entry_tests.cpp)
  target_link_libraries(frontend_name_entry_tests PRIVATE port_runtime)
  add_test(NAME frontend_name_entry COMMAND frontend_name_entry_tests)
  set_tests_properties(frontend_name_entry PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frontend_display_tests src/tests/frontend_display_tests.cpp)
  target_link_libraries(frontend_display_tests PRIVATE port_runtime)
  add_test(NAME frontend_display COMMAND frontend_display_tests)
  set_tests_properties(frontend_display PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(jukebox PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  if(TARGET port_media)
    add_executable(movieplayer_tests src/tests/movieplayer_tests.cpp)
    target_link_libraries(movieplayer_tests PRIVATE port_media game_core)
    add_test(NAME movieplayer COMMAND movieplayer_tests)
    add_executable(movie_audio_tests src/tests/movie_audio_tests.cpp)
    target_link_libraries(movie_audio_tests PRIVATE port_media game_core)
    add_test(NAME movie_audio COMMAND movie_audio_tests)
    set_tests_properties(movie_audio PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    if(WIN32)
      add_custom_command(TARGET movie_audio_tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_FFMPEG_DLLS} "$<TARGET_FILE_DIR:movie_audio_tests>")
      add_custom_command(TARGET movieplayer_tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_FFMPEG_DLLS} "$<TARGET_FILE_DIR:movieplayer_tests>")
    endif()
  endif()
  add_executable(frontend_runtime_tests src/tests/frontend_runtime_tests.cpp)
  target_link_libraries(frontend_runtime_tests PRIVATE port_runtime)
  # TASK-0021: this test drives the runtime against the player's own ZZDATA, so it needs the
  # repo root as its working directory to find config/ps2_assets.local.json. Without it the
  # whole suite ran against an unresolved asset root and reported success having asserted
  # almost nothing.
  add_test(NAME frontend_runtime COMMAND frontend_runtime_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(frame_builder_tests src/tests/frame_builder_tests.cpp)
  target_link_libraries(frame_builder_tests PRIVATE port_runtime)
  add_test(NAME frame_builder COMMAND frame_builder_tests)
  add_executable(frontend_quad_tests src/tests/frontend_quad_tests.cpp)
  target_link_libraries(frontend_quad_tests PRIVATE port_runtime)
  add_test(NAME frontend_quad COMMAND frontend_quad_tests)
  add_executable(particle_alpha_bounds_tests src/tests/particle_alpha_bounds_tests.cpp)
  target_include_directories(particle_alpha_bounds_tests PRIVATE src)
  add_test(NAME particle_alpha_bounds COMMAND particle_alpha_bounds_tests)
  add_executable(track_effects_tests src/tests/track_effects_tests.cpp)
  target_link_libraries(track_effects_tests PRIVATE game_core)
  add_test(NAME track_effects COMMAND track_effects_tests)
  add_executable(track_environment_tests src/tests/track_environment_tests.cpp)
  target_link_libraries(track_environment_tests PRIVATE port_runtime)
  add_test(NAME track_environment COMMAND track_environment_tests)
  add_executable(track_postprocess_tests src/tests/track_postprocess_tests.cpp)
  target_link_libraries(track_postprocess_tests PRIVATE game_core)
  add_test(NAME track_postprocess COMMAND track_postprocess_tests)
  add_executable(track_world_batches_tests src/tests/track_world_batches_tests.cpp)
  target_link_libraries(track_world_batches_tests PRIVATE game_core)
  add_test(NAME track_world_batches COMMAND track_world_batches_tests)
  add_executable(parameter_staging_registry_tests src/tests/parameter_staging_registry_tests.cpp)
  add_executable(depth_sync_tests src/tests/depth_sync_tests.cpp)
  target_include_directories(depth_sync_tests PRIVATE src)
  target_compile_features(depth_sync_tests PRIVATE cxx_std_17)
  add_test(NAME depth_sync_native COMMAND depth_sync_tests)
  add_executable(mesh_prepare_tests src/tests/mesh_prepare_tests.cpp)
  target_include_directories(mesh_prepare_tests PRIVATE src)
  target_compile_features(mesh_prepare_tests PRIVATE cxx_std_17)
  add_test(NAME mesh_prepare COMMAND mesh_prepare_tests)
  target_include_directories(parameter_staging_registry_tests PRIVATE src)
  add_test(NAME parameter_staging_registry COMMAND parameter_staging_registry_tests)
  add_executable(track_submission_digest_tests src/tests/track_submission_digest_tests.cpp)
  target_link_libraries(track_submission_digest_tests PRIVATE game_core)
  add_test(NAME track_submission_digest COMMAND track_submission_digest_tests)
  add_executable(track_submission_cache_tests src/tests/track_submission_cache_tests.cpp)
  target_link_libraries(track_submission_cache_tests PRIVATE game_core)
  add_test(NAME track_submission_cache COMMAND track_submission_cache_tests)
  add_executable(track_command_batches_tests src/tests/track_command_batches_tests.cpp)
  target_link_libraries(track_command_batches_tests PRIVATE game_core)
  add_test(NAME track_command_batches COMMAND track_command_batches_tests)
  add_executable(track_instance_batches_tests src/tests/track_instance_batches_tests.cpp)
  target_link_libraries(track_instance_batches_tests PRIVATE game_core)
  add_test(NAME track_instance_batches COMMAND track_instance_batches_tests)
  # TASK0063 CPU-only asset census; never a runtime FPS benchmark.
  add_executable(track_draw_inventory src/tests/track_draw_inventory.cpp)
  target_link_libraries(track_draw_inventory PRIVATE port_runtime)
  add_test(NAME track_draw_inventory_self_test COMMAND track_draw_inventory --self-test)
  add_executable(track_runtime_tests src/tests/track_runtime_tests.cpp)
  add_executable(static_lods_tests src/tests/static_lods_tests.cpp)
  target_link_libraries(static_lods_tests PRIVATE port_runtime)
  add_test(NAME static_lods COMMAND static_lods_tests)
  add_test(NAME static_lods_assets COMMAND static_lods_tests --assets)
  set_tests_properties(static_lods_assets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 240)
  target_link_libraries(track_runtime_tests PRIVATE port_runtime)
  add_test(NAME track_runtime COMMAND track_runtime_tests)
  add_test(NAME track_runtime_assets COMMAND track_runtime_tests --assets)
  set_tests_properties(track_runtime_assets PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(track_debug_geometry_tests src/tests/track_debug_geometry_tests.cpp)
  target_link_libraries(track_debug_geometry_tests PRIVATE game_core)
  add_test(NAME track_debug_geometry COMMAND track_debug_geometry_tests)
  add_executable(track_texture_tests src/tests/track_texture_tests.cpp)
  target_link_libraries(track_texture_tests PRIVATE game_core)
  add_test(NAME track_texture_orientation COMMAND track_texture_tests)
  add_executable(hp2_tpk_texture_tests src/tests/hp2_tpk_texture_tests.cpp)
  target_link_libraries(hp2_tpk_texture_tests PRIVATE game_core)
  add_test(NAME hp2_tpk_texture COMMAND hp2_tpk_texture_tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME track_profiler_source_review_rules COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/audit_task0069_review.py")
  add_test(NAME driving_effect_source_review_rules COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/audit_task0075_review.py")
  add_test(NAME driving_effect_review_contracts COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0075_source_review.py)
  add_test(NAME track_runtime_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0051_source_review.py)
  add_test(NAME track_regression_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0053_source_review.py)
  add_test(NAME track_optimization_performance_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0055_performance.py)
  add_test(NAME native_profile_performance_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0072_performance.py)
  add_test(NAME native_render_workers_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0072_workers.py)
  add_test(NAME track_optimization_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0055_source_review.py)
  add_test(NAME particle_boundary_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0059_source_review.py)
  add_test(NAME track_memory_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0060_source_review.py)
  add_test(NAME ui_dpi_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0061_source_review.py)
  add_test(NAME track_global_matrix_source_review_rules COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/audit_task0066_review.py")
  add_test(NAME track11_matrix_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0065_source_review.py)
  add_test(NAME track_performance_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0063_source_review.py)
  add_test(NAME original_track_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0058_source_review.py)
  add_test(NAME track_hlod_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0056_source_review.py)
  add_test(NAME track_projection_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0054_source_review.py)
  add_test(NAME track_effects_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0052_source_review.py)
  add_test(NAME track_debug_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0049_source_review.py)
  add_test(NAME track_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0048_source_review.py)
  add_test(NAME class_template_coverage COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/generate_class_templates.py" --check)
  add_test(NAME class_template_generator COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_class_templates.py)
  add_test(NAME debug_reference_matching COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts" -p test_debug_reference_matching.py)
  add_test(NAME debug_reference_types COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/extract_debug_reference_types.py" --self-test)
  add_test(NAME debug_reference_vtables COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/match_debug_vtables.py" --self-test)
  add_test(NAME debug_callchain_evidence COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_debug_callchain_evidence.py)
  add_test(NAME projection_partition_preservation COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_projection_partitions.py)
  add_test(NAME debug_callchain_adoption COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0042_adoption.py)
  add_test(NAME complete_class_members COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/complete_class_members.py")
  add_test(NAME complete_member_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/tests" -p test_complete_members.py)
  add_test(NAME confirmed_identifier_adoption COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/adopt_task0043_identifiers.py" --check)
  add_test(NAME identifier_entity_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0043_identifiers.py)
  add_test(NAME existing_body_preservation COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/audit_task0043_preservation.py" --renames "${CMAKE_CURRENT_SOURCE_DIR}/reverse/naming/task0043-identifier-renames.json")
  add_test(NAME existing_body_audit_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0043_preservation.py)
  add_test(NAME debug_driving_migration_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0044_migration.py)
  add_test(NAME debug_driving_single_chain COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_debug_driving_chain.py)
  add_test(NAME grip_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0045_source_review.py)
  add_test(NAME player_source_review_rules COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_task0046_source_review.py)
  add_test(NAME current_projection_bindings COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/refresh_task0043_projection_bindings.py")
  add_test(NAME main_loop_catalog COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/validate_main_loop.py" --metadata-only)
  # docs/PROGRESS.md's tables are generated from reverse/symbols.csv, the task files and the
  # not_implemented call sites. A stale tracker is a tracker that misleads the next session,
  # so staleness is a test failure, not a style nit.
  add_test(NAME progress_tables COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/generate_progress.py" --check)
  add_test(NAME recovery_progress_contracts COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/reverse/tests" -p test_recovery_progress.py)
  add_test(NAME differential_synthetic COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/pipeline.py" verify --runner $<TARGET_FILE:demo_runner>)
  add_test(NAME pipeline_contracts COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tools/tests)
  set_tests_properties(pipeline_contracts PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

  # The test executables otherwise resolve libstdc++-6.dll / libgcc_s_seh-1.dll from PATH, and a
  # mismatched copy makes them exit with STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139) before main
  # runs. ps2_port already links those runtimes statically for the same reason; do the same here
  # so a test result reflects the test rather than whatever DLL the host happened to find first.
  if(MINGW)
    get_property(_port_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target ${_port_test_targets})
      get_target_property(_type ${_target} TYPE)
      if(_type STREQUAL "EXECUTABLE")
        target_link_options(${_target} PRIVATE -static -static-libgcc -static-libstdc++)
      endif()
    endforeach()
  endif()
endif()

if(PORT_BUILD_GODOT)
  # Port test staging allows validation while another process holds the live DLL.
  set(PORT_GODOT_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/godot" CACHE PATH "Godot host project for builds and tests")
  set(PORT_GODOT_BIN_DIR "${PORT_GODOT_PROJECT_DIR}/bin" CACHE PATH "Godot extension output directory")
  # Ship shader text in the binary; exports do not depend on source-tree paths.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_base.gdshader"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_modulate.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_wireframe.gdshader")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_base.gdshader" HP2_CAR_BASE)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_modulate.glsl" HP2_CAR_MODULATE)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_wireframe.gdshader" HP2_CAR_WIREFRAME)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_sampling.glsl")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_sampling.glsl" HP2_TRACK_SAMPLING)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_fragment.glsl")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_fragment.glsl" HP2_TRACK_FRAGMENT)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_alpha_vertex.glsl")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_alpha_vertex.glsl" HP2_TRACK_ALPHA_VERTEX)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_depth.glsl")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_depth.glsl" HP2_TRACK_DEPTH)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_effects.glsl")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_effects.glsl" HP2_TRACK_EFFECTS)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_debug_fill.gdshader")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_debug_fill.gdshader" HP2_TRACK_DEBUG_FILL)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_debug_edges.gdshader")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/track_debug_edges.gdshader" HP2_TRACK_DEBUG_EDGES)
  configure_file(src/platform/shaders/embedded_shaders.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/hp2_embedded_shaders.hpp" @ONLY)
  if(NOT EXISTS "${PORT_GODOT_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Missing godot-cpp. Follow docs/DEVELOPMENT.md to fetch the pinned checkout.")
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_subdirectory("${PORT_GODOT_CPP_DIR}" "${CMAKE_BINARY_DIR}/godot-cpp")
  if(BUILD_TESTING)
    add_executable(submit_math_tests src/tests/submit_math_tests.cpp)
    target_include_directories(submit_math_tests PRIVATE src)
    target_compile_features(submit_math_tests PRIVATE cxx_std_17)
    target_link_libraries(submit_math_tests PRIVATE godot-cpp)
    if(NOT MSVC)
      set_source_files_properties(src/tests/submit_math_tests.cpp PROPERTIES COMPILE_OPTIONS "-fexceptions;-fno-fast-math;-ffp-contract=off")
    endif()
    add_test(NAME submit_math_native COMMAND submit_math_tests)
  endif()
  add_library(ps2_port SHARED src/platform/godot_bridge/port_core.cpp src/platform/godot_bridge/register_types.cpp
    src/platform/godot_bridge/main_loop_observer.cpp src/platform/godot_bridge/frontend_menu_observer.cpp
    src/platform/godot_bridge/ps2_texture_loader.cpp src/platform/godot_bridge/ps2_frontend_layout.cpp
    src/platform/godot_bridge/ps2_car_viewer.cpp src/platform/godot_bridge/ps2_vehicle_renderer.cpp
    src/platform/godot_bridge/ps2_track_renderer.cpp
    src/platform/godot_bridge/track_environment_renderer.cpp
    src/platform/godot_bridge/track_postprocess.cpp
    src/platform/godot_bridge/track_reflection_renderer.cpp
    src/platform/godot_bridge/track_shadow_renderer.cpp
    src/platform/godot_bridge/track_sun_renderer.cpp
    src/platform/godot_bridge/particle_billboard_renderer.cpp
    src/platform/godot_bridge/gpu_particle_renderer.cpp
    src/platform/godot_bridge/track_particle_renderer.cpp
    src/platform/godot_bridge/driving_particle_display.cpp
    src/platform/godot_bridge/profile_debug_ui.cpp
    src/platform/godot_bridge/track_mesh_batches.cpp
    src/platform/godot_bridge/track_instance_pool.cpp
    src/platform/shaders/track_shader.cpp
    src/platform/godot_bridge/ps2_track_viewer.cpp
    src/platform/godot_bridge/ps2_driving_runtime.cpp
    src/platform/godot_bridge/ps2_driving_lab.cpp
    src/platform/godot_bridge/ps2_render_host.cpp src/platform/not_implemented_dialog.cpp)
  target_sources(ps2_port PRIVATE src/platform/godot_bridge/ps2_movie_player.cpp)
  target_include_directories(ps2_port PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_compile_definitions(ps2_port PRIVATE HP2_DEBUG_BUILD=$<IF:$<CONFIG:Debug>,1,0>
    HP2_ENABLE_DEBUG_CONSOLE=$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>)
  option(PORT_OPTIMIZE_DEBUG_PORT_UI "Optimize port-side Debug ImGui code; preserve symbols/assertions and recovered code flags" OFF)
  set(PORT_DEBUG_PORT_UI_OPTIMIZED 0)
  if(PORT_OPTIMIZE_DEBUG_PORT_UI AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      message(FATAL_ERROR "PORT_OPTIMIZE_DEBUG_PORT_UI requires GCC or Clang")
    endif()
    set(PORT_DEBUG_PORT_UI_OPTIMIZED 1)
    if(TARGET port_debug_console)
      target_compile_options(port_debug_console PRIVATE -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
    endif()
  endif()
  option(PORT_OPTIMIZE_DEBUG_RENDER_SELECTION "Optimize Debug rendering selection only; retain symbols/assertions and physics Debug flags" OFF)
  set(PORT_DEBUG_RENDER_SELECTION_OPTIMIZED 0)
  if(PORT_OPTIMIZE_DEBUG_RENDER_SELECTION AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      message(FATAL_ERROR "PORT_OPTIMIZE_DEBUG_RENDER_SELECTION requires GCC or Clang")
    endif()
    set(PORT_DEBUG_RENDER_SELECTION_OPTIMIZED 1)
    # Rendering-only units. Physics, driving, assets and the scheduler retain
    # existing Debug flags. Preserve floating point ordering and alias semantics.
    set_property(SOURCE
      src/game_core/port/track/runtime.cpp
      src/game_core/port/track/environment.cpp
      src/game_core/indep/src/scenery.cpp
      src/game_core/src/ecstasy/eenveffectse.cpp
      APPEND PROPERTY COMPILE_OPTIONS -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
  endif()
  set_property(SOURCE src/platform/godot_bridge/ps2_track_renderer.cpp APPEND PROPERTY COMPILE_DEFINITIONS HP2_DEBUG_RENDER_SELECTION_OPTIMIZED=${PORT_DEBUG_RENDER_SELECTION_OPTIMIZED} HP2_DEBUG_PORT_UI_OPTIMIZED=${PORT_DEBUG_PORT_UI_OPTIMIZED})
  option(PORT_OPTIMIZE_DEBUG_RENDER_HOST "Optimize only the Debug rendering adapters; retain debug symbols, assertions and the original core build" OFF)
  set(PORT_DEBUG_RENDER_HOST_OPTIMIZED 0)
  if(PORT_OPTIMIZE_DEBUG_RENDER_HOST AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      message(FATAL_ERROR "PORT_OPTIMIZE_DEBUG_RENDER_HOST currently requires GCC or Clang")
    endif()
    set(PORT_DEBUG_RENDER_HOST_OPTIMIZED 1)
    set(PORT_DEBUG_RENDER_SOURCES
      src/platform/godot_bridge/ps2_track_renderer.cpp
      src/platform/godot_bridge/ps2_vehicle_renderer.cpp
      src/platform/godot_bridge/ps2_render_host.cpp
      src/platform/godot_bridge/profile_debug_ui.cpp
      src/platform/godot_bridge/track_sun_renderer.cpp
      src/platform/godot_bridge/track_postprocess.cpp
      src/platform/godot_bridge/track_environment_renderer.cpp
      src/platform/godot_bridge/particle_billboard_renderer.cpp
    src/platform/godot_bridge/gpu_particle_renderer.cpp
    src/platform/godot_bridge/track_particle_renderer.cpp
      src/platform/godot_bridge/driving_particle_display.cpp
      src/platform/godot_bridge/track_shadow_renderer.cpp
      src/platform/godot_bridge/track_reflection_renderer.cpp)
  endif()
  set_property(SOURCE src/platform/godot_bridge/ps2_track_renderer.cpp APPEND PROPERTY COMPILE_DEFINITIONS HP2_DEBUG_RENDER_HOST_OPTIMIZED=${PORT_DEBUG_RENDER_HOST_OPTIMIZED})
  if(TARGET port_debug_console)
    target_link_libraries(ps2_port PRIVATE port_debug_console)
  endif()
  # Godot-cpp 4.4 propagates -fno-exceptions. Port adapters catch recovered NotImplemented
  # locally; enable exceptions in these source files without changing Godot's own code.
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_sources(ps2_port PRIVATE src/platform/godot_bridge/track_mesh_batches_tests.cpp src/platform/godot_bridge/track_instance_pool_tests.cpp)
  endif()
  get_target_property(PORT_BRIDGE_SOURCES ps2_port SOURCES)
  if(MSVC)
    set_source_files_properties(${PORT_BRIDGE_SOURCES} PROPERTIES COMPILE_OPTIONS "/EHsc;/U_HAS_EXCEPTIONS")
  else()
    set_source_files_properties(${PORT_BRIDGE_SOURCES} PROPERTIES COMPILE_OPTIONS "-fexceptions")
  endif()
  if(NOT MSVC)
    set_property(SOURCE src/platform/godot_bridge/track_mesh_batches.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
  endif()
  if(PORT_DEBUG_RENDER_HOST_OPTIMIZED)
    # Keep recovered algorithms, physics, godot-cpp and ImGui at their existing
    # Debug flags. Do not reassociate floating point operations in host math.
    set_property(SOURCE ${PORT_DEBUG_RENDER_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
      -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
  endif()
  target_link_libraries(ps2_port PRIVATE game_core port_runtime godot-cpp port_media)
  target_link_libraries(ps2_port PRIVATE port_debug_viewers)
  if(WIN32)
    add_custom_command(TARGET ps2_port POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_FFMPEG_DLLS} "$<TARGET_FILE_DIR:ps2_port>")
  endif()
  set_target_properties(ps2_port PROPERTIES PREFIX "" OUTPUT_NAME "ps2_port"
    RUNTIME_OUTPUT_DIRECTORY "${PORT_GODOT_BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PORT_GODOT_BIN_DIR}")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(ps2_port PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${PORT_GODOT_BIN_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_${config} "${PORT_GODOT_BIN_DIR}")
  endforeach()
  # Keep the smoke scene self-contained on Windows with MinGW runtime DLLs.
  if(MINGW)
    target_link_options(ps2_port PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  set(PORT_GODOT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
      CACHE FILEPATH "Godot 4.7.2 executable for host smoke tests")
  if(BUILD_TESTING AND EXISTS "${PORT_GODOT_EXECUTABLE}")
    add_test(NAME profile_async_export_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_profile_export_smoke.gd)
    # The fixture deliberately injects write/Object errors and verifies retry.
    set_tests_properties(profile_async_export_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "TASK0072_PROFILE_EXPORT_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;TASK0072_PROFILE_EXPORT_FAILED")
    add_test(NAME driving_performance_contract_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_driving_harness_smoke.gd)
    add_test(NAME render_worker_configuration_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_worker_config_smoke.gd -- --render-prepare-workers=4 --render-thread-requested=safe)
    add_test(NAME render_worker_invalid_configuration_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_worker_config_smoke.gd -- --render-prepare-workers=3 --expect-startup-error)
    add_test(NAME render_thread_mismatch_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --render-thread safe --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_worker_config_smoke.gd -- --render-prepare-workers=0 --render-thread-requested=separate --expect-startup-error)

    set_tests_properties(driving_performance_contract_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "TASK0072_DRIVING_HARNESS_CONTRACT_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME profile_debug_contract_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/profile_debug_contract_smoke.gd)
    set_tests_properties(profile_debug_contract_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "PROFILE_DEBUG_CONTRACT_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_test(NAME driving_camera_track_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/driving_camera_smoke.gd -- --camera-track)
      set_tests_properties(driving_camera_track_host PROPERTIES TIMEOUT 120
        PASS_REGULAR_EXPRESSION "DRIVING_CAMERA_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_camera_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/driving_camera_smoke.gd)
      set_tests_properties(driving_camera_host PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "DRIVING_CAMERA_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME collision_audio_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/runtime/collision_audio_transport_smoke.gd)
      set_tests_properties(collision_audio_host PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "COLLISION_AUDIO_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_audio_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/runtime/driving_audio_smoke.gd)
      set_tests_properties(driving_audio_host PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "DRIVING_AUDIO_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME profile_history_cache_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/task0072_profile_cache_smoke.gd -- --expect-debug=true)
      set_tests_properties(profile_history_cache_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "TASK0072_PROFILE_CACHE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    endif()
    add_test(NAME track_profiler_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track_profiler_smoke.gd -- --track-profile)
    add_test(NAME track_profiler_hud_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track_profiler_hud_smoke.gd -- --track-profile)
    set_tests_properties(track_profiler_hud_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "PROFILE_DEBUG_IMGUI_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    set_tests_properties(track_profiler_host PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "TRACK_PROFILER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    if(PORT_ENABLE_GPU_TESTS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_test(NAME water_effects_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/water_effects_smoke.gd
          -- --water-screenshot=${CMAKE_CURRENT_SOURCE_DIR}/local/task0089/wiper-host.png)
      set_tests_properties(water_effects_gpu PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "WATER_EFFECTS_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track12_driving_gaps_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track12_driving_gaps_smoke.gd -- --particle-backend=godot_gpu)
      set_tests_properties(track12_driving_gaps_gpu PROPERTIES TIMEOUT 180
          PASS_REGULAR_EXPRESSION "TRACK12_GAPS_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME gpu_particle_lifecycle_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/gpu_particle_lifecycle_smoke.gd -- --particle-backend=godot_gpu)
      set_tests_properties(gpu_particle_lifecycle_gpu PROPERTIES TIMEOUT 240
        PASS_REGULAR_EXPRESSION "GPU_PARTICLE_LIFECYCLE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME vehicle_flares_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/vehicle_flares_smoke.gd)
      set_tests_properties(vehicle_flares_gpu PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "VEHICLE_FLARES_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME vehicle_flares_live_exit_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/vehicle_flares_smoke.gd -- --flare-track --flare-exit-live)
      set_tests_properties(vehicle_flares_live_exit_gpu PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "VEHICLE_FLARES_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME vehicle_flares_track_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/vehicle_flares_smoke.gd -- --flare-track)
      set_tests_properties(vehicle_flares_track_gpu PROPERTIES TIMEOUT 180
          PASS_REGULAR_EXPRESSION "VEHICLE_FLARES_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME gpu_particle_state_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/gpu_particle_state_smoke.gd -- --particle-backend=godot_gpu)
      set_tests_properties(gpu_particle_state_gpu PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "GPU_PARTICLE_STATE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME gpu_particle_visual_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/gpu_particle_visual_smoke.gd -- --particle-backend=godot_gpu)
      set_tests_properties(gpu_particle_visual_gpu PROPERTIES TIMEOUT 120
        PASS_REGULAR_EXPRESSION "GPU_PARTICLE_VISUAL_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME sun_counter_transport_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/task0072_sun_counter_smoke.tscn --
        "--sun-counter-source=${CMAKE_CURRENT_SOURCE_DIR}/src/platform/godot_bridge/track_sun_renderer.cpp")
      set_tests_properties(sun_counter_transport_gpu PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TASK0072_SUN_COUNTER_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME profile_bulk_upload_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/task0072_profile_upload_smoke.tscn --
        "--profile-upload-output=${CMAKE_CURRENT_SOURCE_DIR}/local/task0072/ctest-profile-upload")
      set_tests_properties(profile_bulk_upload_gpu PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TASK0072_PROFILE_UPLOAD_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME encoded_transport_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/encoded_transport_gpu_smoke.tscn --quit-after 300)
      set_tests_properties(encoded_transport_gpu PROPERTIES TIMEOUT 30 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "ENCODED_TRANSPORT_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_camera_motion_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track_camera_motion_smoke.gd -- --track-load=11)
      set_tests_properties(track_camera_motion_gpu PROPERTIES TIMEOUT 180 PASS_REGULAR_EXPRESSION "TRACK_CAMERA_MOTION_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME native_track_culling_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/runtime/track_native_culling_smoke.gd)
      set_tests_properties(native_track_culling_gpu PROPERTIES TIMEOUT 180 PASS_REGULAR_EXPRESSION "NATIVE_TRACK_CULLING_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME race_runtime_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/race_scene.tscn -- --track=11 --car=ELISE --race-runtime-smoke)
      set_tests_properties(race_runtime_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "RACE_RUNTIME_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_mask_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_mask_smoke.tscn)
      set_tests_properties(track_mask_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_MASK_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_fusion_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_fusion_gpu_smoke.tscn)
      set_tests_properties(track_fusion_gpu PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_FUSION_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_effects_ui_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_effects_ui_smoke.tscn)
      set_tests_properties(track_effects_ui_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_EFFECTS_UI_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_particles_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_particles_gpu_smoke.tscn)
      set_tests_properties(track_particles_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_PARTICLES_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_optimization_reflection_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_optimization_reflection_smoke.tscn -- --optimization-mode=optimized)
      set_tests_properties(track_optimization_reflection_gpu PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77 PASS_REGULAR_EXPRESSION "TRACK_OPTIMIZATION_REFLECTION_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_optimization_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_optimization_smoke.tscn -- --optimization-mode=optimized)
      set_tests_properties(track_optimization_gpu PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77 PASS_REGULAR_EXPRESSION "TRACK_OPTIMIZATION_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME particle_mesh_reuse_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/particle_mesh_reuse_gpu_smoke.tscn)
      set_tests_properties(particle_mesh_reuse_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77 PASS_REGULAR_EXPRESSION "PARTICLE_MESH_REUSE_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_sun_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_sun_gpu_smoke.tscn -- --track-sun-gpu-resident=false)
      set_tests_properties(track_sun_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_SUN_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME sky_depth_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/sky_depth_gpu_smoke.tscn)
      set_tests_properties(sky_depth_gpu PROPERTIES TIMEOUT 30 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "SKY_DEPTH_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_postprocess_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_postprocess_gpu_smoke.tscn)
      set_tests_properties(track_postprocess_gpu PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_POSTPROCESS_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_postprocess_lifecycle_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_postprocess_lifecycle_gpu_smoke.tscn)
      set_tests_properties(track_postprocess_lifecycle_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_POSTPROCESS_LIFECYCLE_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_pipeline_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_pipeline_smoke.tscn -- --track-parity-gpu)
      set_tests_properties(track_pipeline_gpu PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_PIPELINE_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME car_material_gpu COMMAND "${PORT_GODOT_EXECUTABLE}" --audio-driver Dummy
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/car_material_gpu_smoke.tscn --quit-after 300 -- --material-gpu-smoke)
      set_tests_properties(car_material_gpu PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "CAR_MATERIAL_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    endif()
    add_test(NAME race_runtime_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/race_scene.tscn -- --track=11 --car=ELISE --race-runtime-smoke)
    set_tests_properties(race_runtime_host PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
      PASS_REGULAR_EXPRESSION "RACE_RUNTIME_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME track_controls_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/runtime/track_controls_smoke.gd)
    set_tests_properties(track_controls_host PROPERTIES TIMEOUT 180
      PASS_REGULAR_EXPRESSION "TRACK_CONTROLS_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_test(NAME track_render_diagnostics_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track_render_diagnostics_smoke.gd)
    set_tests_properties(track_render_diagnostics_host PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "TRACK_RENDER_DIAGNOSTICS_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME track_mesh_batch_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_mesh_batch_smoke.tscn)
      set_tests_properties(track_mesh_batch_host PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "TRACK_MESH_BATCH_CACHE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_pipeline_parity COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/track_pipeline_smoke.tscn)
      set_tests_properties(track_pipeline_parity PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_PIPELINE_PARITY_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_viewer_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/track_viewer.tscn -- --track-load=11 --track-smoke)
      set_tests_properties(track_viewer_host PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "TRACK_VIEWER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_input_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/driving_input_smoke.gd)
      set_tests_properties(driving_input_host PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "DRIVING_INPUT_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track12_driving_gaps_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/track12_driving_gaps_smoke.gd)
      set_tests_properties(track12_driving_gaps_host PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "TRACK12_GAPS_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME water_effects_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/water_effects_smoke.gd)
      set_tests_properties(water_effects_host PROPERTIES TIMEOUT 120
          PASS_REGULAR_EXPRESSION "WATER_EFFECTS_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_particles_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/debug/driving_particles_smoke.gd)
      set_tests_properties(driving_particles_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
          PASS_REGULAR_EXPRESSION "DRIVING_PARTICLE_HOST_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_render_retention_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
          --path "${PORT_GODOT_PROJECT_DIR}" --script res://scripts/runtime/track_retention_smoke.gd)
      set_tests_properties(track_render_retention_host PROPERTIES TIMEOUT 60
          PASS_REGULAR_EXPRESSION "TRACK_RENDER_RETENTION_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME track_default_projection_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
          --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/track_viewer.tscn --quit-after 1000 -- --track-smoke --track-projection-only)
      set_tests_properties(track_default_projection_host PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
          PASS_REGULAR_EXPRESSION "TRACK_DEFAULT_PROJECTION_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_asset_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/car_viewer.tscn --quit-after 600 -- --driving-asset-smoke)
      set_tests_properties(driving_asset_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "DRIVING_ASSET_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_lab_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/car_viewer.tscn --quit-after 600 -- --driving-smoke)
      set_tests_properties(driving_lab_host PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "DRIVING_LAB_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    endif()
    add_test(NAME car_surface_batches COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/car_surface_batch_smoke.tscn)
    set_tests_properties(car_surface_batches PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
      PASS_REGULAR_EXPRESSION "CAR_SURFACE_BATCH_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME car_render_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/viewer_frontend_fixture.tscn -- --car-render-smoke
      --expect-debug=$<IF:$<CONFIG:Debug>,true,false>)
    set_tests_properties(car_render_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
      PASS_REGULAR_EXPRESSION "CAR_RENDER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME debug_console_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug_console_smoke.tscn --
      --expect-debug=$<IF:$<CONFIG:Debug>,true,false>
      --expect-console=$<IF:$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>,true,false>)
    set_tests_properties(debug_console_host PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "DEBUG_CONSOLE_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME debug_viewers_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/debug/viewer_frontend_fixture.tscn --
      --viewer-smoke --expect-debug=$<IF:$<CONFIG:Debug>,true,false>
      --expect-console=$<IF:$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>,true,false>)
    set_tests_properties(debug_viewers_host PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "VIEWER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
  endif()
endif()

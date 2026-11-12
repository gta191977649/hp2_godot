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

# TASK-0110 E-0110-A01: scoped Resample FPU rounding is observable. Only the explicitly scoped Resample/CopMover
# translation units opt out of contraction/rounding assumptions; ordinary game
# functions retain their existing compiler/numerical model.
set_source_files_properties(src/game_core/indep/src/drivetarget.cpp src/game_core/indep/src/copdriver.cpp PROPERTIES
  COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU,Clang>:-frounding-math;-ffp-contract=off>;$<$<CXX_COMPILER_ID:MSVC>:/fp:strict>")
# E-0110-R01/RM01/F19: preserve separate original scalar Euler and VU ACC stages.
set_source_files_properties(src/game_core/indep/src/componentphysics.cpp src/game_core/indep/src/rigidbody.cpp src/game_core/indep/src/car.cpp src/game_core/indep/src/collisionbody.cpp src/game_core/src/ecstasy/emathe.cpp PROPERTIES
  COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>;$<$<CXX_COMPILER_ID:MSVC>:/fp:strict>")
add_library(game_core STATIC
  src/game_core/indep/src/zonecameras.cpp
  src/game_core/port/race/nis_services.cpp
  src/game_core/port/race/nis_resources.cpp
  src/game_core/port/race/nis_render_packet.cpp
  src/game_core/indep/src/speech/conversions.cpp
  src/game_core/NFSTaskParameters.cpp
  src/game_core/indep/src/aerodynamics.cpp
  src/game_core/indep/src/carinfo.cpp
  src/game_core/indep/src/carstate.cpp
  src/game_core/indep/src/engine.cpp
  src/game_core/indep/src/rigidbody.cpp
  src/game_core/indep/src/suspension.cpp
  src/game_core/indep/src/table.cpp
  src/game_core/indep/src/eventmanager.cpp
  src/game_core/indep/src/timer.cpp
  src/game_core/indep/src/clans.cpp
  src/game_core/indep/src/fernflattener.cpp
  src/game_core/indep/src/copmgr.cpp
  src/game_core/port/race/cop_speech.cpp
  src/game_core/port/race/damage_resources.cpp
  src/game_core/port/race/car_render_resources.cpp
  src/game_core/indep/src/ecstasy.cpp
  src/game_core/indep/src/speech/mgr.cpp
  src/game_core/indep/src/speech/speechcontext.cpp
  src/game_core/indep/src/speechnoise.cpp
  src/game_core/indep/src/terraintype.cpp
  src/game_core/indep/src/scenery.cpp
  src/game_core/indep/src/track.cpp
  src/game_core/indep/src/volumetree.cpp
  src/game_core/indep/src/polyhedra.cpp
  src/game_core/indep/src/animateddriver.cpp
  src/game_core/indep/src/characterinfo.cpp
  src/game_core/indep/src/camera.cpp
  src/game_core/indep/src/debugtopology.cpp
  src/game_core/indep/src/eanimations.cpp
  src/game_core/src/ecstasy/elighte.cpp
  src/game_core/src/skye.cpp src/game_core/src/sune.cpp src/game_core/src/ecstasy/eenvmape.cpp
  src/game_core/src/ecstasy/eenveffectse.cpp
  src/game_core/port/track/postprocess.cpp
  src/game_core/port/track/position_markers.cpp
  src/game_core/indep/src/weatherman.cpp
  src/game_core/indep/src/wheel.cpp
  src/game_core/port/driving/adapters.cpp
  src/game_core/port/driving/byte_view.cpp
  src/game_core/port/driving/unresolved_entries.cpp
  src/game_core/src/bMath.cpp
  src/game_core/src/bMatrix.cpp
  src/game_core/src/bFunkPlat.cpp
  src/game_core/src/EspClient.cpp
  src/game_core/src/bInventory.cpp
  src/game_core/indep/src/drivinglocation.cpp
  src/game_core/port/race/pcm_command_device.cpp
  src/game_core/indep/src/soundcollision.cpp
  src/game_core/port/race/ai_drive_target.cpp
  src/game_core/indep/src/joystick.cpp
  src/game_core/port/race/flailer_resources.cpp
  src/game_core/port/race/player_resources.cpp
  src/game_core/port/race/memory_availability.cpp
  src/game_core/indep/src/nis.cpp
  src/game_core/indep/src/NISSoundTable.cpp
  src/game_core/indep/src/joylog.cpp
  src/game_core/indep/src/brepeat.cpp
  src/game_core/indep/src/contact.cpp
  src/game_core/src/bVector.cpp
  src/game_core/src/joysticke.cpp
  src/game_core/src/bDriverPS2.cpp
  src/game_core/src/bDriverLowPS2.cpp
  src/game_core/src/bVoice.cpp
  src/game_core/src/bList.cpp
  src/game_core/src/bRegion.cpp
  src/game_core/port/race/car_sound_resources.cpp
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
  # TASK-0107 build repair: existing TASK-0099 source owners must not depend
  # on manual additions to the regenerated Retail template source list.
  src/game_core/indep/src/barricade.cpp
  src/game_core/indep/src/copdriver.cpp
  src/game_core/indep/src/coputil.cpp
  src/game_core/indep/src/gameflow.cpp
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
  src/port/runtime/world_session.cpp
  src/port/runtime/race_menu_host.cpp
  src/port/runtime/loading_presentation.cpp
  src/port/runtime/free_run_launch.cpp
  src/port/runtime/event_world_launch.cpp
  src/port/runtime/event_runtime_continuation.cpp
  src/port/runtime/event_profile_codec.cpp
  src/port/runtime/event_settings_codec.cpp
  src/port/runtime/event_score_codec.cpp
  src/port/runtime/event_reward_codec.cpp
  src/platform/ps2_reward_store.cpp
  src/port/runtime/race_rules_view.cpp
  src/port/runtime/selection_event_resources.cpp
  src/port/runtime/prepared_event_launch.cpp
  src/port/runtime/source_event_status.cpp
  src/game_core/port/track/assets.cpp src/game_core/port/track/runtime.cpp src/game_core/port/track/environment.cpp src/game_core/port/track/particles.cpp src/game_core/port/driving/track_environment.cpp
  src/game_core/port/driving/car_effects.cpp
  src/port/vehicle/textures.cpp
  src/port/vehicle/catalog.cpp
  src/port/assets/vehicle_geometry.cpp src/port/vehicle/model.cpp
  src/port/runtime/frame_builder.cpp src/port/runtime/frontend_runtime.cpp
  src/port/runtime/event_tree_view.cpp src/port/runtime/selection_page.cpp src/port/runtime/selection_runtime.cpp)
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
  add_executable(world_dependencies_tests src/tests/world_dependencies_tests.cpp)
  add_executable(world_construction_tests src/tests/world_construction_tests.cpp)
  add_executable(world_loading_tests src/tests/world_loading_tests.cpp)
  target_link_libraries(world_loading_tests PRIVATE port_runtime)
  add_test(NAME world_loading_boundaries COMMAND world_loading_tests)
  add_test(NAME world_loading_reverse COMMAND world_loading_tests --reverse)
  set_tests_properties(world_loading_boundaries world_loading_reverse PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(world_texture_bindings_tests src/tests/world_texture_bindings_tests.cpp)
  target_link_libraries(world_texture_bindings_tests PRIVATE port_runtime)
  add_test(NAME world_texture_bindings COMMAND world_texture_bindings_tests)
  set_tests_properties(world_texture_bindings PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(world_construction_tests PRIVATE port_runtime)
  add_test(NAME world_construction COMMAND world_construction_tests)
  set_tests_properties(world_construction PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(world_emitter_bindings_tests src/tests/world_emitter_bindings_tests.cpp)
  target_link_libraries(world_emitter_bindings_tests PRIVATE port_runtime)
  add_test(NAME world_emitter_bindings COMMAND world_emitter_bindings_tests)
  set_tests_properties(world_emitter_bindings PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(world_dependencies_tests PRIVATE port_runtime)
  add_test(NAME world_dependencies COMMAND world_dependencies_tests)
  add_test(NAME world_dependencies_original COMMAND world_dependencies_tests --original)
  add_test(NAME world_dependencies_original_reverse COMMAND world_dependencies_tests --original-reverse)
  set_tests_properties(world_dependencies world_dependencies_original world_dependencies_original_reverse PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(world_dependencies_original world_dependencies_original_reverse PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(async_asset_cache_tests src/tests/async_asset_cache_tests.cpp)
  target_link_libraries(async_asset_cache_tests PRIVATE port_runtime)
  add_test(NAME async_asset_cache COMMAND async_asset_cache_tests)
  set_tests_properties(async_asset_cache PROPERTIES TIMEOUT 10)
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
    src/platform/godot_bridge/ps2_nis_renderer.cpp
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
    src/platform/godot_bridge/ps2_render_host.cpp src/platform/godot_bridge/frontend_gpu_executor.cpp src/platform/not_implemented_dialog.cpp)
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
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # The port-side selection packet expands and shades the vehicle every frame.
    # Retain Debug symbols/assertions and the original floating-point order.
    set_property(SOURCE src/port/runtime/selection_runtime.cpp APPEND PROPERTY COMPILE_OPTIONS
      -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
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
        PASS_REGULAR_EXPRESSION "WORLD_DEBUG_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
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
      PASS_REGULAR_EXPRESSION "WORLD_DEBUG_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
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
      # TASK-0107 current public driving contract: source World, not a plane fixture.
      add_test(NAME world_debug_session_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${PORT_GODOT_PROJECT_DIR}" res://scenes/race_scene.tscn -- --world-debug-smoke)
      set_tests_properties(world_debug_session_host PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "WORLD_DEBUG_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
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

# TASK-0099 E-0099-W16: actual Debug speech startup/queue/voice regression.
if(BUILD_TESTING)
  add_executable(cop_speech_tests src/tests/cop_speech_tests.cpp)
  target_link_libraries(cop_speech_tests PRIVATE game_core)
  add_test(NAME cop_speech COMMAND cop_speech_tests)
  set_tests_properties(cop_speech PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 30 SKIP_RETURN_CODE 77)
endif()

# TASK-0099 E-0099-W17: independent cop structure rule/source corpus checks.
if(BUILD_TESTING)
  add_executable(cop_resources_tests src/tests/cop_resources_tests.cpp)
  target_link_libraries(cop_resources_tests PRIVATE port_runtime)
  add_test(NAME cop_resources COMMAND cop_resources_tests)
  add_test(NAME cop_resources_original COMMAND cop_resources_tests --original)
  set_tests_properties(cop_resources cop_resources_original PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  set_tests_properties(cop_resources_original PROPERTIES TIMEOUT 300)
endif()

# TASK-0099 E-0099-W22: SHA-checked actual damage/marker corpus and Car constructors.
if(BUILD_TESTING)
  add_executable(car_state_damage_tests src/tests/car_state_damage_tests.cpp)
  target_link_libraries(car_state_damage_tests PRIVATE game_core)
  add_test(NAME car_state_damage COMMAND car_state_damage_tests)
  set_tests_properties(car_state_damage PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()

# TASK-0099 actual participant foundation lifetimes and constructor semantics.
if(BUILD_TESTING)
  add_executable(collision_body_lifecycle_tests src/tests/collision_body_lifecycle_tests.cpp)
  target_link_libraries(collision_body_lifecycle_tests PRIVATE game_core)
  add_test(NAME collision_body_lifecycle COMMAND collision_body_lifecycle_tests)
  add_executable(car_control_tests src/tests/car_control_tests.cpp)
  target_link_libraries(car_control_tests PRIVATE game_core)
  add_test(NAME car_control COMMAND car_control_tests)
  set_tests_properties(collision_body_lifecycle car_control PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(control_construction_tests src/tests/control_construction_tests.cpp)
  target_link_libraries(control_construction_tests PRIVATE game_core)
  add_test(NAME control_construction COMMAND control_construction_tests)
  set_tests_properties(control_construction PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(collision_geometry_bindings_tests src/tests/collision_geometry_bindings_tests.cpp)
  target_link_libraries(collision_geometry_bindings_tests PRIVATE port_runtime)
  add_test(NAME collision_geometry_bindings COMMAND collision_geometry_bindings_tests)
  set_tests_properties(collision_geometry_bindings PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(coordinate_memory_tests src/tests/coordinate_memory_tests.cpp)
  target_link_libraries(coordinate_memory_tests PRIVATE port_runtime)
  add_test(NAME coordinate_memory COMMAND coordinate_memory_tests)
  add_test(NAME coordinate_memory_original COMMAND coordinate_memory_tests --original)
  set_tests_properties(coordinate_memory coordinate_memory_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  set_tests_properties(coordinate_memory_original PROPERTIES TIMEOUT 300)
  add_executable(car_audio_foundation_tests src/tests/car_audio_foundation_tests.cpp)
  target_link_libraries(car_audio_foundation_tests PRIVATE game_core)
  add_test(NAME car_audio_foundation COMMAND car_audio_foundation_tests)
  set_tests_properties(car_audio_foundation PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

# TASK-0099 E-0099-W28: actual nonnull character/artwork constructor lifetime.
if(BUILD_TESTING)
  add_executable(animated_driver_tests src/tests/animated_driver_tests.cpp)
  target_link_libraries(animated_driver_tests PRIVATE port_runtime)
  add_test(NAME animated_driver COMMAND animated_driver_tests)
  set_tests_properties(animated_driver PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 300)
endif()

if(BUILD_TESTING)
  add_executable(event_manager_tests src/tests/event_manager_tests.cpp)
  target_link_libraries(event_manager_tests PRIVATE game_core)
  add_test(NAME event_manager COMMAND event_manager_tests)
  set_tests_properties(event_manager PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(debug_transport_tests src/tests/debug_transport_tests.cpp)
  target_link_libraries(debug_transport_tests PRIVATE game_core)
  add_test(NAME debug_transport COMMAND debug_transport_tests)
  set_tests_properties(debug_transport PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(car_render_tests src/tests/car_render_tests.cpp)
  target_link_libraries(car_render_tests PRIVATE game_core)
  add_test(NAME car_render COMMAND car_render_tests)
  set_tests_properties(car_render PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(car_construction_tests src/tests/car_construction_tests.cpp)
  target_link_libraries(car_construction_tests PRIVATE port_runtime)
  add_test(NAME car_construction_original COMMAND car_construction_tests)
  add_test(NAME car_construction_original_reverse COMMAND car_construction_tests --reverse)
  set_tests_properties(car_construction_original car_construction_original_reverse PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 300)
  add_test(NAME event_manager_original COMMAND event_manager_tests --original)
  set_tests_properties(event_manager_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 300)
  target_link_libraries(event_manager_tests PRIVATE port_runtime)
endif()

if(BUILD_TESTING)
  add_executable(physics_construction_tests src/tests/physics_construction_tests.cpp)
  target_link_libraries(physics_construction_tests PRIVATE game_core)
  add_test(NAME physics_construction COMMAND physics_construction_tests)
  set_tests_properties(physics_construction PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(grid_start_tests src/tests/grid_start_tests.cpp)
  target_link_libraries(grid_start_tests PRIVATE port_runtime)
  add_test(NAME grid_start_original COMMAND grid_start_tests)
  set_tests_properties(grid_start_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
endif()

if(BUILD_TESTING)
  add_executable(racing_car_tests src/tests/racing_car_tests.cpp)
  target_link_libraries(racing_car_tests PRIVATE game_core)
  add_test(NAME racing_car_original COMMAND racing_car_tests)
  set_tests_properties(racing_car_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()

if(BUILD_TESTING)
  add_test(NAME world_participants_original COMMAND car_construction_tests --participants)
  set_tests_properties(world_participants_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(traffic_construction_tests src/tests/traffic_construction_tests.cpp)
  target_link_libraries(traffic_construction_tests PRIVATE game_core)
  add_test(NAME traffic_construction COMMAND traffic_construction_tests)
  set_tests_properties(traffic_construction PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(car_sound_factories_tests src/tests/car_sound_factories_tests.cpp)
  target_link_libraries(car_sound_factories_tests PRIVATE port_runtime)
  add_test(NAME car_sound_factories_original COMMAND car_sound_factories_tests)
  set_tests_properties(car_sound_factories_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
endif()

if(BUILD_TESTING)
  add_executable(car_voice_pool_tests src/tests/car_voice_pool_tests.cpp)
  target_link_libraries(car_voice_pool_tests PRIVATE game_core)
  add_test(NAME car_voice_pool COMMAND car_voice_pool_tests)
  set_tests_properties(car_voice_pool PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(shared_collision_foundation_tests src/tests/shared_collision_foundation_tests.cpp)
  target_link_libraries(shared_collision_foundation_tests PRIVATE game_core)
  add_test(NAME shared_collision_foundation COMMAND shared_collision_foundation_tests)
  set_tests_properties(shared_collision_foundation PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(replay_construction_tests src/tests/replay_construction_tests.cpp)
  target_link_libraries(replay_construction_tests PRIVATE game_core)
  add_test(NAME replay_construction COMMAND replay_construction_tests)
  set_tests_properties(replay_construction PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(ai_drive_target_tests src/tests/ai_drive_target_tests.cpp)
  target_link_libraries(ai_drive_target_tests PRIVATE game_core)
  add_test(NAME ai_drive_target COMMAND ai_drive_target_tests)
  set_tests_properties(ai_drive_target PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(repeat_bitstream_tests src/tests/repeat_bitstream_tests.cpp)
  target_link_libraries(repeat_bitstream_tests PRIVATE game_core)
  add_test(NAME repeat_bitstream COMMAND repeat_bitstream_tests)
  set_tests_properties(repeat_bitstream PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_test(NAME car_construction_sound_original COMMAND car_construction_tests --sound)
  set_tests_properties(car_construction_sound_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

if(BUILD_TESTING)
  add_executable(pcm_command_device_tests src/tests/pcm_command_device_tests.cpp)
  target_link_libraries(pcm_command_device_tests PRIVATE port_runtime)
  add_test(NAME pcm_command_device_original COMMAND pcm_command_device_tests)
  set_tests_properties(pcm_command_device_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(collision_sound_tests src/tests/collision_sound_tests.cpp)
  target_link_libraries(collision_sound_tests PRIVATE port_runtime)
  add_test(NAME collision_sound_original COMMAND collision_sound_tests)
  set_tests_properties(collision_sound_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(driver_animation_queue_tests src/tests/driver_animation_queue_tests.cpp)
  target_link_libraries(driver_animation_queue_tests PRIVATE port_runtime)
  add_test(NAME driver_animation_queue_original COMMAND driver_animation_queue_tests)
  set_tests_properties(driver_animation_queue_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(camera_view_tests src/tests/camera_view_tests.cpp)
  target_link_libraries(camera_view_tests PRIVATE port_runtime)
  add_test(NAME camera_view_original COMMAND camera_view_tests)
  set_tests_properties(camera_view_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(drive_camera_construction_tests src/tests/drive_camera_construction_tests.cpp)
  target_link_libraries(drive_camera_construction_tests PRIVATE port_runtime)
  add_test(NAME drive_camera_construction_original COMMAND drive_camera_construction_tests)
  set_tests_properties(drive_camera_construction_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(drive_camera_update_tests src/tests/drive_camera_update_tests.cpp)
  target_link_libraries(drive_camera_update_tests PRIVATE port_runtime)
  add_test(NAME drive_camera_update_original COMMAND drive_camera_update_tests)
  set_tests_properties(drive_camera_update_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
  add_executable(camera_constraints_tests src/tests/camera_constraints_tests.cpp)
  target_link_libraries(camera_constraints_tests PRIVATE port_runtime)
  add_test(NAME camera_constraints COMMAND camera_constraints_tests)
  add_test(NAME camera_constraints_original COMMAND camera_constraints_tests --original)
  set_tests_properties(camera_constraints camera_constraints_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 90)
endif()

if(BUILD_TESTING)
  add_executable(ai_driver_tests src/tests/ai_driver_tests.cpp)
  target_link_libraries(ai_driver_tests PRIVATE port_runtime game_core)
  add_test(NAME ai_driver COMMAND ai_driver_tests)
  set_tests_properties(ai_driver PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(joylog_tests src/tests/joylog_tests.cpp)
  target_link_libraries(joylog_tests PRIVATE port_runtime game_core)
  add_test(NAME joylog_original COMMAND joylog_tests)
  set_tests_properties(joylog_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

if(BUILD_TESTING)
  add_executable(player_dependency_tests src/tests/player_dependency_tests.cpp)
  target_link_libraries(player_dependency_tests PRIVATE port_runtime game_core)
  add_test(NAME player_dependencies_original COMMAND player_dependency_tests)
  set_tests_properties(player_dependencies_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(player_construction_tests src/tests/player_construction_tests.cpp)
  target_link_libraries(player_construction_tests PRIVATE port_runtime game_core)
  add_test(NAME player_construction_original COMMAND player_construction_tests)
  set_tests_properties(player_construction_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(real_controller_tests src/tests/real_controller_tests.cpp)
  target_link_libraries(real_controller_tests PRIVATE port_runtime game_core)
  add_test(NAME real_controller_original COMMAND real_controller_tests)
  add_test(NAME real_driver_frame_original COMMAND real_controller_tests --driver)
  set_tests_properties(real_controller_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  set_tests_properties(real_driver_frame_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

if(BUILD_TESTING)
  add_executable(flailer_resources_tests src/tests/flailer_resources_tests.cpp)
  target_link_libraries(flailer_resources_tests PRIVATE port_runtime game_core)
  add_test(NAME flailer_resources_original COMMAND flailer_resources_tests)
  set_tests_properties(flailer_resources_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME world_player_movement_original COMMAND car_construction_tests --participants --player --collision-stage)
  set_tests_properties(world_player_movement_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME race_start_original COMMAND car_construction_tests --participants --player --coordinator-init --collision-stage)
  set_tests_properties(race_start_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME race_coordinator_original COMMAND car_construction_tests --participants --player --coordinator-owner --collision-stage)
  set_tests_properties(race_coordinator_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(race_start_tests src/tests/race_start_tests.cpp)
  target_link_libraries(race_start_tests PRIVATE port_runtime game_core)
  add_test(NAME race_start COMMAND race_start_tests)
  set_tests_properties(race_start PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(race_finish_tests src/tests/race_finish_tests.cpp)
  target_link_libraries(race_finish_tests PRIVATE port_runtime game_core)
  add_test(NAME race_finish_original COMMAND race_finish_tests)
  set_tests_properties(race_finish_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 60)
  add_executable(frontend_finish_tests src/tests/frontend_finish_tests.cpp)
  target_link_libraries(frontend_finish_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_finish_original COMMAND frontend_finish_tests)
  set_tests_properties(frontend_finish_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 60)
  add_executable(cop_frame_tests src/tests/cop_frame_tests.cpp)
  target_link_libraries(cop_frame_tests PRIVATE game_core)
  add_test(NAME cop_frame COMMAND cop_frame_tests)
  set_tests_properties(cop_frame PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(memory_budget_tests src/tests/memory_budget_tests.cpp)
  target_link_libraries(memory_budget_tests PRIVATE game_core)
  add_test(NAME memory_budget COMMAND memory_budget_tests)
  add_executable(fern_shaker_tests src/tests/fern_shaker_tests.cpp)
  target_link_libraries(fern_shaker_tests PRIVATE port_runtime game_core)
  add_test(NAME fern_shaker COMMAND fern_shaker_tests)
  set_tests_properties(fern_shaker PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME fern_shaker_original COMMAND fern_shaker_tests --original)
  set_tests_properties(fern_shaker_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 600)
  add_executable(raw_acidfx_tests src/tests/raw_acidfx_tests.cpp)
  target_link_libraries(raw_acidfx_tests PRIVATE port_runtime game_core)
  add_test(NAME raw_acidfx COMMAND raw_acidfx_tests)
  set_tests_properties(raw_acidfx PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME raw_acidfx_original COMMAND raw_acidfx_tests --original)
  set_tests_properties(raw_acidfx_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()


if(BUILD_TESTING)
  add_executable(smokeable_think_tests src/tests/smokeable_think_tests.cpp)
  target_link_libraries(smokeable_think_tests PRIVATE port_runtime game_core)
  add_test(NAME smokeable_think COMMAND smokeable_think_tests)
  set_tests_properties(smokeable_think PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()


# TASK-0099 W57: exact original WorldObject source recovered from .mdebug.
target_sources(game_core PRIVATE src/game_core/indep/src/worldobject.cpp)
target_sources(game_core PRIVATE src/game_core/port/race/frontend_event_resources.cpp)
target_sources(game_core PRIVATE src/game_core/port/race/frontend_language_resources.cpp)
if(BUILD_TESTING)
  add_executable(frontend_event_resources_tests src/tests/frontend_event_resources_tests.cpp)
  target_link_libraries(frontend_event_resources_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_event_resources_original COMMAND frontend_event_resources_tests)
  set_tests_properties(frontend_event_resources_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(frontend_result_tests src/tests/frontend_result_tests.cpp)
  target_link_libraries(frontend_result_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_result_original COMMAND frontend_result_tests)
  set_tests_properties(frontend_result_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 60)
endif()
target_sources(game_core PRIVATE src/game_core/indep/src/speech/speechfn.cpp)
target_sources(game_core PRIVATE src/game_core/src/bDebug.cpp)

if(BUILD_TESTING)
  add_executable(player_feedback_tests src/tests/player_feedback_tests.cpp)
  target_link_libraries(player_feedback_tests PRIVATE port_runtime game_core)
  add_test(NAME player_feedback_original COMMAND player_feedback_tests)
  set_tests_properties(player_feedback_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

if(BUILD_TESTING)
  add_test(NAME player_game_state_original COMMAND player_construction_tests --frame)
  set_tests_properties(player_game_state_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

if(BUILD_TESTING)
  add_executable(car_terrain_effects_tests src/tests/car_terrain_effects_tests.cpp)
  target_link_libraries(car_terrain_effects_tests PRIVATE port_runtime game_core)
  add_test(NAME car_terrain_effects COMMAND car_terrain_effects_tests)
  add_test(NAME car_terrain_effects_original COMMAND car_terrain_effects_tests --original)
  set_tests_properties(car_terrain_effects_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  set_tests_properties(car_terrain_effects PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

if(BUILD_TESTING)
  add_executable(world_frame_tests src/tests/world_frame_tests.cpp)
  target_link_libraries(world_frame_tests PRIVATE port_runtime game_core)
  add_test(NAME world_frame COMMAND world_frame_tests)
  set_tests_properties(world_frame PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME world_frame_original COMMAND car_construction_tests --participants --player --coordinator-owner --world-frame)
  set_tests_properties(world_frame_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(countdown_frame_tests src/tests/countdown_frame_tests.cpp)
  add_executable(hud_animation_tests src/tests/hud_animation_tests.cpp)
  target_link_libraries(hud_animation_tests PRIVATE port_runtime game_core)
  add_test(NAME hud_animation_original COMMAND hud_animation_tests)
  set_tests_properties(hud_animation_original PROPERTIES SKIP_RETURN_CODE 77 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(countdown_frame_tests PRIVATE port_runtime game_core)
  add_test(NAME countdown_frame_original COMMAND countdown_frame_tests)
  set_tests_properties(countdown_frame_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(shared_skid_tests src/tests/shared_skid_tests.cpp)
  target_link_libraries(shared_skid_tests PRIVATE port_runtime game_core)
  add_test(NAME shared_skids COMMAND shared_skid_tests)
  set_tests_properties(shared_skids PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(world_clock_tests src/tests/world_clock_tests.cpp)
  target_link_libraries(world_clock_tests PRIVATE port_runtime game_core)
  add_test(NAME world_clock COMMAND world_clock_tests)
  set_tests_properties(world_clock PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(cop_snapshot_tests src/tests/cop_snapshot_tests.cpp)
  target_link_libraries(cop_snapshot_tests PRIVATE game_core)
  add_test(NAME cop_snapshot COMMAND cop_snapshot_tests)
  set_tests_properties(cop_snapshot PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(replay_snapshot_tests src/tests/replay_snapshot_tests.cpp)
  target_link_libraries(replay_snapshot_tests PRIVATE port_runtime game_core)
  add_test(NAME replay_snapshot_original COMMAND replay_snapshot_tests)
  set_tests_properties(replay_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(world_snapshot_tests src/tests/world_snapshot_tests.cpp)
  target_link_libraries(world_snapshot_tests PRIVATE port_runtime game_core)
  add_test(NAME world_snapshot COMMAND world_snapshot_tests)
  set_tests_properties(world_snapshot PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME snapshot_tree_original COMMAND car_construction_tests --participants --player --coordinator-owner --snapshot-tree)
  set_tests_properties(snapshot_tree_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

# TASK-0099 W74 typed AI snapshot source-consistency checks.
if(BUILD_TESTING)
  add_executable(ai_snapshot_tests src/tests/ai_snapshot_tests.cpp)
  target_link_libraries(ai_snapshot_tests PRIVATE game_core)
  add_test(NAME ai_snapshot COMMAND ai_snapshot_tests)
  set_tests_properties(ai_snapshot PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W77 original input registration prerequisites.
if(BUILD_TESTING)
  add_executable(input_registration_tests src/tests/input_registration_tests.cpp)
  target_link_libraries(input_registration_tests PRIVATE game_core)
  add_test(NAME input_registration_original COMMAND input_registration_tests)
  set_tests_properties(input_registration_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W79 original shared Smokeable creator/family ownership fixtures.
if(BUILD_TESTING)
  add_executable(smokeable_create_tests src/tests/smokeable_create_tests.cpp)
  target_link_libraries(smokeable_create_tests PRIVATE game_core)
  add_test(NAME smokeable_create COMMAND smokeable_create_tests)
  set_tests_properties(smokeable_create PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W82 raw original goal/status dependency.
if(BUILD_TESTING)
  add_executable(frontend_status_tests src/tests/frontend_status_tests.cpp)
  target_link_libraries(frontend_status_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_status_original COMMAND frontend_status_tests)
  set_tests_properties(frontend_status_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W85 concrete real-time Joylog preparation.
if(BUILD_TESTING)
  add_executable(real_world_clock_tests src/tests/real_world_clock_tests.cpp)
  target_link_libraries(real_world_clock_tests PRIVATE port_runtime game_core)
  add_test(NAME real_world_clock_original COMMAND real_world_clock_tests)
  set_tests_properties(real_world_clock_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME world_clock_frame_original COMMAND car_construction_tests --participants --player --coordinator-owner --clock-frame)
  set_tests_properties(world_clock_frame_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME world_clock_audio_frame_original COMMAND car_construction_tests --participants --player --coordinator-owner --clock-frame --sound --world-frames 2)
  add_test(NAME world_clock_countdown_frame_original COMMAND car_construction_tests --participants --player --coordinator-owner --clock-frame --world-frames 2)
  set_tests_properties(world_clock_audio_frame_original world_clock_countdown_frame_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

# TASK-0099 W84 original stats/record tables and reviewed catalog fields.
if(BUILD_TESTING)
  add_executable(frontend_records_tests src/tests/frontend_records_tests.cpp)
  target_link_libraries(frontend_records_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_records_original COMMAND frontend_records_tests)
  set_tests_properties(frontend_records_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W83 actual virgin conversion and explicit auxiliary-lane binding.
if(BUILD_TESTING)
  add_executable(smokeable_conversion_tests src/tests/smokeable_conversion_tests.cpp)
  target_link_libraries(smokeable_conversion_tests PRIVATE port_runtime game_core)
  add_test(NAME smokeable_conversion_original COMMAND smokeable_conversion_tests)
  add_executable(smokeable_moveability_tests src/tests/smokeable_moveability_tests.cpp)
  target_link_libraries(smokeable_moveability_tests PRIVATE game_core)
  add_test(NAME smokeable_moveability_original COMMAND smokeable_moveability_tests)
  set_tests_properties(smokeable_conversion_original smokeable_moveability_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W86 raw authored-event StartRace request, not transition execution.
if(BUILD_TESTING)
  add_executable(frontend_start_tests src/tests/frontend_start_tests.cpp)
  target_link_libraries(frontend_start_tests PRIVATE port_runtime game_core)
  add_test(NAME frontend_start_original COMMAND frontend_start_tests)
  set_tests_properties(frontend_start_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W89 original active-AI process resources.
if(BUILD_TESTING)
  add_executable(active_ai_bootstrap_tests src/tests/active_ai_bootstrap_tests.cpp)
  target_link_libraries(active_ai_bootstrap_tests PRIVATE port_runtime game_core)
  add_test(NAME active_ai_bootstrap_original COMMAND active_ai_bootstrap_tests)
  set_tests_properties(active_ai_bootstrap_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W90 source active-action replacement and cursor regression.
if(BUILD_TESTING)
  add_executable(ai_action_selection_tests src/tests/ai_action_selection_tests.cpp)
  target_link_libraries(ai_action_selection_tests PRIVATE port_runtime game_core)
  add_test(NAME ai_action_selection_original COMMAND ai_action_selection_tests)
  set_tests_properties(ai_action_selection_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W92 original active catchup/boost and reviewed difficulty graphs.
if(BUILD_TESTING)
  add_executable(ai_boost_tests src/tests/ai_boost_tests.cpp)
  target_link_libraries(ai_boost_tests PRIVATE port_runtime game_core)
  add_test(NAME ai_boost_original COMMAND ai_boost_tests)
  set_tests_properties(ai_boost_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

  # TASK-0099 W115 actual authored HP roster,240 source clock/audio steps.
  if(BUILD_TESTING)
    add_test(NAME authored_hp_rolling_original COMMAND car_construction_tests --authored-event 0 --world-frames 240)
    set_tests_properties(authored_hp_rolling_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W115 source-proven boolean consumer, no guessed SQRT encoding.
  if(BUILD_TESTING)
    add_executable(track_polygon_consumer_tests src/tests/track_polygon_consumer_tests.cpp)
    target_link_libraries(track_polygon_consumer_tests PRIVATE game_core)
    add_test(NAME track_polygon_consumer_original COMMAND track_polygon_consumer_tests)
    set_tests_properties(track_polygon_consumer_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W111 actual authored first HP roster, complete source clock frame.
  if(BUILD_TESTING)
    add_test(NAME authored_hp_clock_original COMMAND car_construction_tests --authored-event 0)
    set_tests_properties(authored_hp_clock_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W111 original TrafficCar movement/CRT and bounded zero normalization.
  if(BUILD_TESTING)
    add_executable(traffic_move_tests src/tests/traffic_move_tests.cpp)
    target_link_libraries(traffic_move_tests PRIVATE game_core)
    add_test(NAME traffic_move_original COMMAND traffic_move_tests)
    set_tests_properties(traffic_move_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W110 original Belt/QuickSpline snapshot and unknown heap-W guard.
  if(BUILD_TESTING)
    add_executable(belt_snapshot_tests src/tests/belt_snapshot_tests.cpp)
    target_link_libraries(belt_snapshot_tests PRIVATE game_core)
    add_test(NAME belt_snapshot_original COMMAND belt_snapshot_tests)
    set_tests_properties(belt_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W109 original Chopper Fly prelude and actual effect-list cleanup.
  if(BUILD_TESTING)
    add_executable(chopper_fly_tests src/tests/chopper_fly_tests.cpp)
    target_link_libraries(chopper_fly_tests PRIVATE game_core)
    add_test(NAME chopper_fly_original COMMAND chopper_fly_tests)
    set_tests_properties(chopper_fly_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W106 original Belt movement helpers, cubic transport and RPM.
  if(BUILD_TESTING)
    add_executable(belt_move_tests src/tests/belt_move_tests.cpp)
    target_link_libraries(belt_move_tests PRIVATE game_core)
    add_test(NAME belt_move_original COMMAND belt_move_tests)
    set_tests_properties(belt_move_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W108 original CopDriver Think prefix and actual virtual target.
  if(BUILD_TESTING)
    add_executable(cop_driver_think_tests src/tests/cop_driver_think_tests.cpp)
    target_link_libraries(cop_driver_think_tests PRIVATE game_core)
    add_test(NAME cop_driver_think_original COMMAND cop_driver_think_tests)
    set_tests_properties(cop_driver_think_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W107 actual authored HP forest and source lifetime, no host frames.
  if(BUILD_TESTING)
    add_test(NAME authored_hp_snapshot_original COMMAND car_construction_tests --authored-event 0 --snapshot-tree --snapshot-only)
    set_tests_properties(authored_hp_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W107 actual AI-cop/parent snapshot and pointer codecs.
  if(BUILD_TESTING)
    add_executable(cop_active_snapshot_tests src/tests/cop_active_snapshot_tests.cpp)
    target_link_libraries(cop_active_snapshot_tests PRIVATE game_core)
    add_test(NAME cop_active_snapshot_original COMMAND cop_active_snapshot_tests)
    set_tests_properties(cop_active_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W119 actual pursuit lifetime and snapshot forest.
  if(BUILD_TESTING)
    add_executable(pursuit_snapshot_tests src/tests/pursuit_snapshot_tests.cpp)
    target_link_libraries(pursuit_snapshot_tests PRIVATE game_core)
    add_test(NAME pursuit_snapshot_original COMMAND pursuit_snapshot_tests)
    set_tests_properties(pursuit_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME player_car_reset_original COMMAND car_construction_tests --authored-event 0 --event-tree 0 --snapshot-tree --snapshot-only --route-reset)
    set_tests_properties(player_car_reset_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME authored_hp_pullover_original COMMAND car_construction_tests --authored-event 0 --event-tree 1 --snapshot-tree --snapshot-only --pullover-release)
    set_tests_properties(authored_hp_pullover_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME authored_championship_pullover_leaves_original COMMAND car_construction_tests --authored-event 0 --event-tree 0 --snapshot-tree --snapshot-only --pullover-release)
    set_tests_properties(authored_championship_pullover_leaves_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  endif()

  # TASK-0099 W103 original traffic scheduling/actions and authored speed leaves.
  if(BUILD_TESTING)
    add_executable(traffic_think_tests src/tests/traffic_think_tests.cpp)
    target_link_libraries(traffic_think_tests PRIVATE game_core)
    add_test(NAME traffic_think_original COMMAND traffic_think_tests)
    set_tests_properties(traffic_think_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W105 original TrafficCar mover snapshot and empty base body.
  if(BUILD_TESTING)
    add_executable(trafficcar_snapshot_tests src/tests/trafficcar_snapshot_tests.cpp)
    target_link_libraries(trafficcar_snapshot_tests PRIVATE game_core)
    add_test(NAME trafficcar_snapshot_original COMMAND trafficcar_snapshot_tests)
    set_tests_properties(trafficcar_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W102 original BeltMover route/spline coordinate setup.
  if(BUILD_TESTING)
    add_executable(belt_set_tests src/tests/belt_set_tests.cpp)
    target_link_libraries(belt_set_tests PRIVATE game_core)
    add_test(NAME belt_set_original COMMAND belt_set_tests)
    set_tests_properties(belt_set_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W104 original TrafficDriver and inherited AI snapshot codecs.
  if(BUILD_TESTING)
    add_executable(trafficdriver_snapshot_tests src/tests/trafficdriver_snapshot_tests.cpp)
    target_link_libraries(trafficdriver_snapshot_tests PRIVATE port_runtime game_core)
    add_test(NAME trafficdriver_snapshot_original COMMAND trafficdriver_snapshot_tests)
    set_tests_properties(trafficdriver_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W96 original active police descriptor dispatch and initial states.
  if(BUILD_TESTING)
    add_executable(cop_think_tests src/tests/cop_think_tests.cpp)
    target_link_libraries(cop_think_tests PRIVATE game_core)
    add_test(NAME cop_think_original COMMAND cop_think_tests)
    set_tests_properties(cop_think_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W101 original CopDriver and inherited source snapshot codecs.
  if(BUILD_TESTING)
    add_executable(copdriver_snapshot_tests src/tests/copdriver_snapshot_tests.cpp)
    target_link_libraries(copdriver_snapshot_tests PRIVATE port_runtime game_core)
    add_test(NAME copdriver_snapshot_original COMMAND copdriver_snapshot_tests)
    set_tests_properties(copdriver_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W100 original BeltMover constructor and owned-child teardown.
  if(BUILD_TESTING)
    add_executable(belt_construction_tests src/tests/belt_construction_tests.cpp)
    target_link_libraries(belt_construction_tests PRIVATE game_core)
    add_test(NAME belt_construction_original COMMAND belt_construction_tests)
    set_tests_properties(belt_construction_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W98 original active and idle Chopper snapshot/resource effects.
  if(BUILD_TESTING)
    add_executable(chopper_snapshot_tests src/tests/chopper_snapshot_tests.cpp)
    target_link_libraries(chopper_snapshot_tests PRIVATE port_runtime game_core)
    add_test(NAME chopper_snapshot_original COMMAND chopper_snapshot_tests)
    set_tests_properties(chopper_snapshot_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W88/W95 authored first Championship roster, continuous source clock.
  if(BUILD_TESTING)
    add_test(NAME authored_championship_clock_original COMMAND car_construction_tests --authored-event 0 --event-tree 0 --world-frames 240)
    set_tests_properties(authored_championship_clock_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W95 original active DrivingChoice selection.
  if(BUILD_TESTING)
    add_executable(ai_new_choice_tests src/tests/ai_new_choice_tests.cpp)
    target_link_libraries(ai_new_choice_tests PRIVATE game_core)
    add_test(NAME ai_new_choice_original COMMAND ai_new_choice_tests)
    set_tests_properties(ai_new_choice_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W94 original Chopper and shared ChopperSound lifetime.
  if(BUILD_TESTING)
    add_executable(chopper_construction_tests src/tests/chopper_construction_tests.cpp)
    target_link_libraries(chopper_construction_tests PRIVATE port_runtime game_core)
    add_test(NAME chopper_construction_original COMMAND chopper_construction_tests)
    set_tests_properties(chopper_construction_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W93 original active AvoidCars and honk scheduling.
  if(BUILD_TESTING)
    add_executable(ai_avoid_tests src/tests/ai_avoid_tests.cpp)
    target_link_libraries(ai_avoid_tests PRIVATE port_runtime game_core)
    add_test(NAME ai_avoid_original COMMAND ai_avoid_tests)
    set_tests_properties(ai_avoid_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  endif()

  # TASK-0099 W91 original traffic/CopControl participant construction.
if(BUILD_TESTING)
  add_executable(ambient_participant_tests src/tests/ambient_participant_tests.cpp)
  target_link_libraries(ambient_participant_tests PRIVATE port_runtime game_core)
  add_test(NAME ambient_participants_original COMMAND ambient_participant_tests)
  set_tests_properties(ambient_participants_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

# TASK-0099 W81 actual replay special-event source consumers and positive time scan.
if(BUILD_TESTING)
  add_executable(replay_events_tests src/tests/replay_events_tests.cpp)
  target_link_libraries(replay_events_tests PRIVATE port_runtime game_core)
  add_test(NAME replay_events_original COMMAND replay_events_tests)
  set_tests_properties(replay_events_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()


# TASK-0107: one real source World behind every driving debug entry.
if(BUILD_TESTING)
  add_executable(world_particle_snapshot_tests src/tests/world_particle_snapshot_tests.cpp)
  target_link_libraries(world_particle_snapshot_tests PRIVATE port_runtime game_core)
  add_test(NAME world_particle_snapshot COMMAND world_particle_snapshot_tests)
  add_executable(player_hud_presentation_tests src/tests/player_hud_presentation_tests.cpp)
  add_executable(event_reward_codec_tests src/tests/event_reward_codec_tests.cpp)
  add_executable(cop_driver_pursuit_tests src/tests/cop_driver_pursuit_tests.cpp)
  target_link_libraries(cop_driver_pursuit_tests PRIVATE game_core)
  add_test(NAME cop_driver_pursuit_original COMMAND cop_driver_pursuit_tests)
  set_tests_properties(cop_driver_pursuit_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  target_link_libraries(event_reward_codec_tests PRIVATE port_runtime game_core)
  add_test(NAME event_reward_codec COMMAND event_reward_codec_tests)
  add_executable(reward_store_tests src/tests/reward_store_tests.cpp)
  target_link_libraries(reward_store_tests PRIVATE port_runtime game_core)
  add_test(NAME reward_store COMMAND reward_store_tests)
  target_link_libraries(player_hud_presentation_tests PRIVATE port_runtime game_core)
  add_test(NAME player_hud_presentation_original COMMAND player_hud_presentation_tests)
  set_tests_properties(player_hud_presentation_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(track_map_tests src/tests/track_map_tests.cpp)
  target_link_libraries(track_map_tests PRIVATE game_core)
  add_test(NAME track_map_original COMMAND track_map_tests)
  set_tests_properties(track_map_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(end_game_screens_tests src/tests/end_game_screens_tests.cpp)
  add_executable(game_core_nis_override_tests src/tests/nis_override_tests.cpp)
  target_link_libraries(game_core_nis_override_tests PRIVATE game_core)
  add_test(NAME nis_override_original COMMAND game_core_nis_override_tests)
  set_tests_properties(nis_override_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(loading_screen_tests src/tests/loading_screen_tests.cpp)
  target_link_libraries(loading_screen_tests PRIVATE port_runtime game_core)
  add_test(NAME loading_screen_original COMMAND loading_screen_tests)
  set_tests_properties(loading_screen_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(nis_resource_tests src/tests/nis_resource_tests.cpp)
  add_executable(nis_render_packet_tests src/tests/nis_render_packet_tests.cpp)
  target_link_libraries(nis_render_packet_tests PRIVATE game_core)
  add_test(NAME nis_render_packet_original COMMAND nis_render_packet_tests)
  set_tests_properties(nis_render_packet_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  target_link_libraries(nis_resource_tests PRIVATE port_runtime game_core)
  add_test(NAME nis_resources_original COMMAND nis_resource_tests)
  set_tests_properties(nis_resources_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  target_link_libraries(end_game_screens_tests PRIVATE port_runtime game_core)
  add_test(NAME end_game_screens_original COMMAND end_game_screens_tests)
  set_tests_properties(end_game_screens_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(race_menu_tests src/tests/race_menu_tests.cpp)
  target_link_libraries(race_menu_tests PRIVATE port_runtime game_core)
  add_test(NAME race_menu_original COMMAND race_menu_tests)
  set_tests_properties(race_menu_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(world_session_tests src/tests/world_session_tests.cpp)
  target_link_libraries(world_session_tests PRIVATE port_runtime game_core)
  add_test(NAME world_session_original COMMAND world_session_tests)
  set_tests_properties(world_session_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(free_run_launch_tests src/tests/free_run_launch_tests.cpp)
  add_executable(event_world_tests src/tests/event_world_tests.cpp)
  add_executable(cop_mover_tests src/tests/cop_mover_tests.cpp)
  target_link_libraries(cop_mover_tests PRIVATE game_core)
  add_test(NAME cop_mover_original COMMAND cop_mover_tests)
  set_tests_properties(cop_mover_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 120)
  target_link_libraries(event_world_tests PRIVATE port_runtime game_core)
  add_test(NAME event_world_original COMMAND event_world_tests)
  add_test(NAME event_world_language_mode COMMAND event_world_tests --language 0)
  set_tests_properties(event_world_language_mode PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  set_tests_properties(event_world_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  target_link_libraries(free_run_launch_tests PRIVATE port_runtime game_core)
  add_test(NAME free_run_launch_original COMMAND free_run_launch_tests)
  add_executable(minimal_driving_tests src/tests/minimal_driving_tests.cpp)
  target_link_libraries(minimal_driving_tests PRIVATE port_runtime game_core)
  add_test(NAME minimal_driving_original COMMAND minimal_driving_tests)
  set_tests_properties(minimal_driving_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(runtime_warning_tests src/tests/runtime_warning_tests.cpp)
  add_executable(event_profile_codec_tests src/tests/event_profile_codec_tests.cpp)
  add_executable(race_rules_view_tests src/tests/race_rules_view_tests.cpp)
  add_executable(selection_event_resources_tests src/tests/selection_event_resources_tests.cpp)
  add_executable(source_event_status_tests src/tests/source_event_status_tests.cpp)
  add_executable(generic_event_entry_tests src/tests/generic_event_entry_tests.cpp)
  target_link_libraries(generic_event_entry_tests PRIVATE port_runtime game_core)
  add_test(NAME generic_event_entry COMMAND generic_event_entry_tests)
  set_tests_properties(generic_event_entry PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 600)
  add_executable(event_settings_codec_tests src/tests/event_settings_codec_tests.cpp)
  target_link_libraries(event_settings_codec_tests PRIVATE port_runtime game_core)
  add_test(NAME event_settings_codec COMMAND event_settings_codec_tests)
  set_tests_properties(event_settings_codec PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(event_score_runtime_tests src/tests/event_score_runtime_tests.cpp)
  target_link_libraries(event_score_runtime_tests PRIVATE port_runtime game_core)
  add_test(NAME event_score_runtime COMMAND event_score_runtime_tests)
  set_tests_properties(event_score_runtime PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 600)
  add_executable(event_score_codec_tests src/tests/event_score_codec_tests.cpp)
  target_link_libraries(event_score_codec_tests PRIVATE port_runtime game_core)
  add_test(NAME event_score_codec COMMAND event_score_codec_tests)
  set_tests_properties(event_score_codec PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(cop_hot_pursuit_tests src/tests/cop_hot_pursuit_tests.cpp)
  target_link_libraries(cop_hot_pursuit_tests PRIVATE game_core)
  add_test(NAME cop_hot_pursuit COMMAND cop_hot_pursuit_tests)
  set_tests_properties(cop_hot_pursuit PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(cop_teleport_tests src/tests/cop_teleport_tests.cpp)
  target_link_libraries(cop_teleport_tests PRIVATE game_core)
  add_test(NAME cop_teleport COMMAND cop_teleport_tests)
  set_tests_properties(cop_teleport PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(cop_mode_transition_tests src/tests/cop_mode_transition_tests.cpp)
  add_executable(car_average_air_tests src/tests/car_average_air_tests.cpp)
  target_link_libraries(car_average_air_tests PRIVATE game_core)
  add_test(NAME car_average_air COMMAND car_average_air_tests)
  set_tests_properties(car_average_air PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(cop_mode_transition_tests PRIVATE game_core)
  add_test(NAME cop_mode_transition COMMAND cop_mode_transition_tests)
  set_tests_properties(cop_mode_transition PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(pursuit_update_tests src/tests/pursuit_update_tests.cpp)
  target_link_libraries(pursuit_update_tests PRIVATE game_core)
  add_test(NAME pursuit_update COMMAND pursuit_update_tests)
  set_tests_properties(pursuit_update PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  add_executable(cop_admission_tests src/tests/cop_admission_tests.cpp)
  add_executable(result_actions_tests src/tests/result_actions_tests.cpp)
  target_link_libraries(result_actions_tests PRIVATE game_core)
  add_test(NAME result_actions COMMAND result_actions_tests)
  set_tests_properties(result_actions PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(cop_admission_tests PRIVATE game_core)
  add_test(NAME cop_admission COMMAND cop_admission_tests)
  set_tests_properties(cop_admission PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(source_event_status_tests PRIVATE port_runtime game_core)
  add_test(NAME source_event_status COMMAND source_event_status_tests)
  set_tests_properties(source_event_status PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(selection_event_resources_tests PRIVATE port_runtime game_core)
  add_test(NAME selection_event_resources COMMAND selection_event_resources_tests)
  set_tests_properties(selection_event_resources PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(race_rules_view_tests PRIVATE port_runtime game_core)
  add_test(NAME race_rules_view COMMAND race_rules_view_tests)
  set_tests_properties(race_rules_view PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
  target_link_libraries(event_profile_codec_tests PRIVATE port_runtime)
  add_test(NAME event_profile_codec COMMAND event_profile_codec_tests)
  set_tests_properties(event_profile_codec PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(route_observation_tests src/tests/route_observation_tests.cpp)
  target_link_libraries(route_observation_tests PRIVATE game_core)
  add_test(NAME route_observation COMMAND route_observation_tests)
  add_executable(ee_scalar_carrier_tests src/tests/ee_scalar_carrier_tests.cpp)
  target_include_directories(ee_scalar_carrier_tests PRIVATE src)
  target_compile_features(ee_scalar_carrier_tests PRIVATE cxx_std_17)
  target_link_libraries(ee_scalar_carrier_tests PRIVATE Threads::Threads)
  add_test(NAME ee_scalar_carriers COMMAND ee_scalar_carrier_tests)
  target_include_directories(runtime_warning_tests PRIVATE src)
  target_compile_features(runtime_warning_tests PRIVATE cxx_std_17)
  target_link_libraries(runtime_warning_tests PRIVATE Threads::Threads)
  add_test(NAME runtime_warnings COMMAND runtime_warning_tests)
  set_tests_properties(runtime_warnings PROPERTIES TIMEOUT 30)
  set_tests_properties(free_run_launch_original PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()

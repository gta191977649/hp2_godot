cmake_minimum_required(VERSION 3.24)
project(PS2PCPort VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
option(PORT_BUILD_GODOT "Build the optional Godot 4.4 bridge" OFF)
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
  src/game_core/indep/src/wheel.cpp
  src/game_core/port/driving/adapters.cpp
  src/game_core/port/driving/byte_view.cpp
  src/game_core/port/driving/unresolved_entries.cpp
  src/game_core/src/bMath.cpp
  src/game_core/src/bMatrix.cpp
  src/game_core/src/ecstasy/emathe.cpp src/game_core/port/driving/session.cpp


  src/game_core/port/driving/memory.cpp src/game_core/port/driving/original_image.cpp



  src/game_core/port/driving/native_backend.cpp

  src/game_core/port/scaffold.cpp src/game_core/port/police/cop_control.cpp
  src/game_core/port/engine/engine_shell.cpp src/game_core/port/engine/world_connections.cpp
  src/game_core/port/frontend/screen_manager.cpp src/game_core/indep/src/fmenus.cpp
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
  src/port/vehicle/textures.cpp
  src/port/vehicle/catalog.cpp
  src/port/assets/vehicle_geometry.cpp src/port/vehicle/model.cpp
  src/port/runtime/frame_builder.cpp src/port/runtime/frontend_runtime.cpp
  src/port/runtime/event_tree_view.cpp)
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
  add_executable(driving_body_tests src/tests/driving_body_tests.cpp)
  target_link_libraries(driving_body_tests PRIVATE game_core)
  add_test(NAME driving_body COMMAND driving_body_tests)
  add_executable(driving_mover_tests src/tests/driving_mover_tests.cpp)
  target_link_libraries(driving_mover_tests PRIVATE game_core)
  add_test(NAME driving_mover COMMAND driving_mover_tests)
  add_executable(driving_backend_tests src/tests/driving_backend_tests.cpp)
  target_link_libraries(driving_backend_tests PRIVATE port_runtime)
  add_test(NAME driving_backend COMMAND driving_backend_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
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
  add_executable(hp2_tpk_texture_tests src/tests/hp2_tpk_texture_tests.cpp)
  target_link_libraries(hp2_tpk_texture_tests PRIVATE game_core)
  add_test(NAME hp2_tpk_texture COMMAND hp2_tpk_texture_tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
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
  add_test(NAME current_projection_bindings COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/refresh_task0043_projection_bindings.py")
  add_test(NAME main_loop_catalog COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/validate_main_loop.py" --metadata-only)
  # docs/PROGRESS.md's tables are generated from reverse/symbols.csv, the task files and the
  # not_implemented call sites. A stale tracker is a tracker that misleads the next session,
  # so staleness is a test failure, not a style nit.
  add_test(NAME progress_tables COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/reverse/scripts/generate_progress.py" --check)
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
  # Ship shader text in the binary; exports do not depend on source-tree paths.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_base.gdshader"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_modulate.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_wireframe.gdshader")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_base.gdshader" HP2_CAR_BASE)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_modulate.glsl" HP2_CAR_MODULATE)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/shaders/car_wireframe.gdshader" HP2_CAR_WIREFRAME)
  configure_file(src/platform/shaders/embedded_shaders.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/hp2_embedded_shaders.hpp" @ONLY)
  if(NOT EXISTS "${PORT_GODOT_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Missing godot-cpp. Follow docs/DEVELOPMENT.md to fetch the pinned checkout.")
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_subdirectory("${PORT_GODOT_CPP_DIR}" "${CMAKE_BINARY_DIR}/godot-cpp")
  add_library(ps2_port SHARED src/platform/godot_bridge/port_core.cpp src/platform/godot_bridge/register_types.cpp
    src/platform/godot_bridge/main_loop_observer.cpp src/platform/godot_bridge/frontend_menu_observer.cpp
    src/platform/godot_bridge/ps2_texture_loader.cpp src/platform/godot_bridge/ps2_frontend_layout.cpp
    src/platform/godot_bridge/ps2_car_viewer.cpp src/platform/godot_bridge/ps2_vehicle_renderer.cpp
    src/platform/godot_bridge/ps2_driving_lab.cpp
    src/platform/godot_bridge/ps2_render_host.cpp src/platform/not_implemented_dialog.cpp)
  target_sources(ps2_port PRIVATE src/platform/godot_bridge/ps2_movie_player.cpp)
  target_include_directories(ps2_port PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_compile_definitions(ps2_port PRIVATE HP2_DEBUG_BUILD=$<IF:$<CONFIG:Debug>,1,0>
    HP2_ENABLE_DEBUG_CONSOLE=$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>)
  if(TARGET port_debug_console)
    target_link_libraries(ps2_port PRIVATE port_debug_console)
  endif()
  # Godot-cpp 4.4 propagates -fno-exceptions. Port adapters catch recovered NotImplemented
  # locally; enable exceptions in these source files without changing Godot's own code.
  get_target_property(PORT_BRIDGE_SOURCES ps2_port SOURCES)
  if(MSVC)
    set_source_files_properties(${PORT_BRIDGE_SOURCES} PROPERTIES COMPILE_OPTIONS "/EHsc;/U_HAS_EXCEPTIONS")
  else()
    set_source_files_properties(${PORT_BRIDGE_SOURCES} PROPERTIES COMPILE_OPTIONS "-fexceptions")
  endif()
  target_link_libraries(ps2_port PRIVATE game_core port_runtime godot-cpp port_media)
  target_link_libraries(ps2_port PRIVATE port_debug_viewers)
  if(WIN32)
    add_custom_command(TARGET ps2_port POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_FFMPEG_DLLS} "$<TARGET_FILE_DIR:ps2_port>")
  endif()
  set_target_properties(ps2_port PROPERTIES PREFIX "" OUTPUT_NAME "ps2_port"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin")
  foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(ps2_port PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin"
      LIBRARY_OUTPUT_DIRECTORY_${config} "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin")
  endforeach()
  # Keep the smoke scene self-contained on Windows with MinGW runtime DLLs.
  if(MINGW)
    target_link_options(ps2_port PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  set(PORT_GODOT_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe"
      CACHE FILEPATH "Godot 4.4.1 executable for asset-free host smoke tests")
  if(BUILD_TESTING AND EXISTS "${PORT_GODOT_EXECUTABLE}")
    if(PORT_ENABLE_GPU_TESTS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_test(NAME car_material_gpu COMMAND "${PORT_GODOT_EXECUTABLE}"
        --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/car_material_gpu_smoke.tscn --quit-after 300 -- --material-gpu-smoke)
      set_tests_properties(car_material_gpu PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "CAR_MATERIAL_GPU_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_test(NAME driving_asset_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/car_viewer.tscn --quit-after 600 -- --driving-asset-smoke)
      set_tests_properties(driving_asset_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
        PASS_REGULAR_EXPRESSION "DRIVING_ASSET_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
      add_test(NAME driving_lab_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
        --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/car_viewer.tscn --quit-after 600 -- --driving-smoke)
      set_tests_properties(driving_lab_host PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "DRIVING_LAB_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    endif()
    add_test(NAME car_surface_batches COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/car_surface_batch_smoke.tscn)
    set_tests_properties(car_surface_batches PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77
      PASS_REGULAR_EXPRESSION "CAR_SURFACE_BATCH_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME car_render_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/viewer_frontend_fixture.tscn -- --car-render-smoke
      --expect-debug=$<IF:$<CONFIG:Debug>,true,false>)
    set_tests_properties(car_render_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77
      PASS_REGULAR_EXPRESSION "CAR_RENDER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME debug_console_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug_console_smoke.tscn --
      --expect-debug=$<IF:$<CONFIG:Debug>,true,false>
      --expect-console=$<IF:$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>,true,false>)
    set_tests_properties(debug_console_host PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "DEBUG_CONSOLE_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
    add_test(NAME debug_viewers_host COMMAND "${PORT_GODOT_EXECUTABLE}" --headless
      --path "${CMAKE_CURRENT_SOURCE_DIR}/godot" res://scenes/debug/viewer_frontend_fixture.tscn --
      --viewer-smoke --expect-debug=$<IF:$<CONFIG:Debug>,true,false>
      --expect-console=$<IF:$<BOOL:${HP2_ENABLE_DEBUG_CONSOLE}>,true,false>)
    set_tests_properties(debug_viewers_host PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "VIEWER_SMOKE_OK" FAIL_REGULAR_EXPRESSION "SCRIPT ERROR;ERROR:")
  endif()
endif()

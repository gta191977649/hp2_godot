cmake_minimum_required(VERSION 3.24)
project(hp2 LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "The HP2 native runtime currently targets Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

add_subdirectory(src/game_core)
add_executable(mvp_race_catalog EXCLUDE_FROM_ALL tools/test_mvp_race_catalog.cpp)
target_link_libraries(mvp_race_catalog PRIVATE game_core)

add_library(mvp_entity_bridge STATIC src/mvp/bridge/entity_state_bridge.cpp)
target_include_directories(mvp_entity_bridge PUBLIC src/mvp)
target_link_libraries(mvp_entity_bridge PUBLIC game_core)

add_subdirectory(third_party/godot-cpp EXCLUDE_FROM_ALL)
file(GLOB local_ffmpeg_sdks LIST_DIRECTORIES true
  "${CMAKE_SOURCE_DIR}/local/tools/ffmpeg/*")
set(HP2_FFMPEG_SDK "" CACHE PATH "FFmpeg 8 SDK directory")
find_path(hp2_ffmpeg_include libavformat/avformat.h
  HINTS "${HP2_FFMPEG_SDK}" ${local_ffmpeg_sdks} PATH_SUFFIXES include REQUIRED)
get_filename_component(hp2_ffmpeg_root "${hp2_ffmpeg_include}" DIRECTORY)
set(hp2_ffmpeg_dlls)
foreach(component avformat avcodec avutil)
  find_library(hp2_ffmpeg_${component} NAMES ${component}
    PATHS "${hp2_ffmpeg_root}/lib" NO_DEFAULT_PATH REQUIRED)
endforeach()
foreach(dll avformat-62 avcodec-62 avutil-60 swresample-6)
  find_file(hp2_${dll} "${dll}.dll" PATHS "${hp2_ffmpeg_root}/bin" NO_DEFAULT_PATH REQUIRED)
  list(APPEND hp2_ffmpeg_dlls "${hp2_${dll}}")
endforeach()

add_library(hp2_blackbox SHARED
  src/mvp/godot/register_types.cpp
  src/mvp/presentation/race_presentation.cpp
  src/mvp/assets/mesh_converter.cpp
  src/mvp/assets/race_diagnostics.cpp
  src/mvp/systems/audio/godot_speech_output.cpp
  src/mvp/systems/frontend/frontend_presentation.cpp
  src/mvp/systems/frontend/frontend_video.cpp
  src/mvp/systems/frontend/original_video_stream.cpp
  src/mvp/systems/track/track_presentation.cpp
  src/mvp/systems/effects/race_effects_presentation.cpp
  src/mvp/systems/vehicle/vehicle_presentation.cpp
  src/mvp/systems/vehicle/vehicle_reflection.cpp
  src/mvp/systems/cinematic/nis_presentation.cpp
  src/mvp/systems/camera/race_post_presentation.cpp
  src/mvp/systems/camera/camera_presentation.cpp)
# Godot's exception setting is a usage requirement. Link its library without
# exporting that compiler flag to the exception-using HP2 boundary.
target_link_libraries(hp2_blackbox PRIVATE mvp_entity_bridge $<LINK_ONLY:godot::cpp>
  "${hp2_ffmpeg_avformat}" "${hp2_ffmpeg_avcodec}" "${hp2_ffmpeg_avutil}")
target_include_directories(hp2_blackbox SYSTEM PRIVATE "${hp2_ffmpeg_include}")
target_include_directories(hp2_blackbox SYSTEM PRIVATE
  $<TARGET_PROPERTY:godot-cpp,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(hp2_blackbox PRIVATE
  $<TARGET_PROPERTY:godot-cpp,INTERFACE_COMPILE_DEFINITIONS>)
set_target_properties(hp2_blackbox PROPERTIES PREFIX "")
set(imgui_console_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-console")
set(imgui_dir "${imgui_console_dir}/example/thirdparty/imgui")
target_sources(hp2_blackbox PRIVATE
  "$<$<CONFIG:Debug>:src/mvp/assets/imgui_overlay.cpp>"
  "$<$<CONFIG:Debug>:src/mvp/assets/frontend_debug_menu.cpp>"
  "$<$<CONFIG:Debug>:src/mvp/assets/trackviewer/track_viewer.cpp>"
  "$<$<CONFIG:Debug>:src/mvp/assets/trackviewer/track_viewer_scene.cpp>"
  "$<$<CONFIG:Debug>:src/mvp/tests/pursuit_test_driver.cpp>")
target_compile_definitions(hp2_blackbox PRIVATE "$<$<CONFIG:Debug>:HP2_MVP_IMGUI>")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES)
  add_library(mvp_imgui STATIC EXCLUDE_FROM_ALL
    "${imgui_dir}/imgui.cpp"
    "${imgui_dir}/imgui_draw.cpp"
    "${imgui_dir}/imgui_widgets.cpp"
    "${imgui_console_dir}/src/imgui_console.cpp")
  target_include_directories(mvp_imgui SYSTEM PUBLIC "${imgui_dir}"
    "${imgui_console_dir}/include" "${imgui_console_dir}/include/imgui_console")
  target_link_libraries(hp2_blackbox PRIVATE "$<$<CONFIG:Debug>:mvp_imgui>")
endif()
if(MINGW)
  # Keep Debug symbols and checks while avoiding -O0 simulation and draw preparation.
  foreach(target game_core mvp_entity_bridge hp2_blackbox mvp_imgui)
    if(TARGET ${target})
      target_compile_options(${target} PRIVATE "$<$<CONFIG:Debug>:-O2>")
    endif()
  endforeach()
  target_link_options(hp2_blackbox PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set(project_dir "${CMAKE_BINARY_DIR}/mvp-project")
file(GENERATE OUTPUT "${project_dir}/.godot/extension_list.cfg" CONTENT "res://hp2_blackbox.gdextension\n")
file(GENERATE OUTPUT "${project_dir}/override.cfg" CONTENT
  "[hp2]\n\nasset_config=\"${CMAKE_SOURCE_DIR}/config/ps2_assets.local.json\"\ndebug_mode=$<IF:$<CONFIG:Debug>,true,false>\n")
add_custom_target(stage_mvp ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${project_dir}/bin"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${project_dir}/licenses"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/third_party/notices" "${project_dir}/licenses"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/third_party/THIRD_PARTY_NOTICES.md" "${project_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/third_party/godot-cpp/LICENSE.md" "${project_dir}/licenses/godot-cpp-LICENSE.md"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${imgui_dir}/LICENSE.txt" "${project_dir}/licenses/Dear-ImGui-LICENSE.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${imgui_console_dir}/LICENSE.txt" "${project_dir}/licenses/ImGui-Console-LICENSE.txt"
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${project_dir}/shader"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/mvp/shader" "${project_dir}/shader"
  COMMAND ${CMAKE_COMMAND} -E rm -f
    "${project_dir}/ps2_mvp.gdextension"
    "${project_dir}/ps2_mvp.gdextension.uid"
    "${project_dir}/bin/ps2_mvp.dll"
    "${project_dir}/hp2_track12.gdextension"
    "${project_dir}/hp2_track12.gdextension.uid"
    "${project_dir}/bin/hp2_track12.dll"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/godot_mvp/project.godot"
    "${CMAKE_SOURCE_DIR}/godot_mvp/export_presets.cfg"
    "${CMAKE_SOURCE_DIR}/godot_mvp/native_game.gd"
    "${CMAKE_SOURCE_DIR}/godot_mvp/native_game.tscn"
    "${CMAKE_SOURCE_DIR}/godot_mvp/hp2_blackbox.gdextension"
    "${CMAKE_SOURCE_DIR}/godot_mvp/hp2_blackbox.gdextension.uid"
    "${project_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:hp2_blackbox>" "${project_dir}/bin"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${hp2_ffmpeg_dlls} "${project_dir}/bin"
  DEPENDS hp2_blackbox VERBATIM)

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
add_library(mvp_entity_bridge STATIC src/mvp/bridge/entity_state_bridge.cpp)
target_include_directories(mvp_entity_bridge PUBLIC src/mvp)
target_link_libraries(mvp_entity_bridge PUBLIC game_core)

add_subdirectory(third_party/godot-cpp EXCLUDE_FROM_ALL)
add_library(hp2_track12 SHARED
  src/mvp/godot/register_types.cpp
  src/mvp/presentation/race_presentation.cpp
  src/mvp/assets/mesh_converter.cpp
  src/mvp/systems/frontend/frontend_presentation.cpp
  src/mvp/systems/frontend/frontend_video.cpp
  src/mvp/systems/track/track_presentation.cpp
  src/mvp/systems/vehicle/vehicle_presentation.cpp
  src/mvp/systems/camera/camera_presentation.cpp)
# Godot's exception setting is a usage requirement. Link its library without
# exporting that compiler flag to the exception-using HP2 boundary.
target_link_libraries(hp2_track12 PRIVATE mvp_entity_bridge $<LINK_ONLY:godot::cpp>)
target_include_directories(hp2_track12 SYSTEM PRIVATE
  $<TARGET_PROPERTY:godot-cpp,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(hp2_track12 PRIVATE
  $<TARGET_PROPERTY:godot-cpp,INTERFACE_COMPILE_DEFINITIONS>)
set_target_properties(hp2_track12 PROPERTIES PREFIX "")
if(MINGW)
  target_link_options(hp2_track12 PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set(project_dir "${CMAKE_BINARY_DIR}/mvp-project")
file(GENERATE OUTPUT "${project_dir}/.godot/extension_list.cfg" CONTENT "res://hp2_track12.gdextension\n")
add_custom_target(stage_mvp ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${project_dir}/bin"
  COMMAND ${CMAKE_COMMAND} -E rm -f
    "${project_dir}/ps2_mvp.gdextension"
    "${project_dir}/ps2_mvp.gdextension.uid"
    "${project_dir}/bin/ps2_mvp.dll"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/godot_mvp/project.godot"
    "${CMAKE_SOURCE_DIR}/godot_mvp/native_game.gd"
    "${CMAKE_SOURCE_DIR}/godot_mvp/native_game.tscn"
    "${CMAKE_SOURCE_DIR}/godot_mvp/hp2_track12.gdextension"
    "${project_dir}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:hp2_track12>" "${project_dir}/bin"
  DEPENDS hp2_track12 VERBATIM)

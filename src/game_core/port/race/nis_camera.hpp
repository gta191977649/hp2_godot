#pragma once
// TASK-0112 E-0112-NC01; explicit native startup/resource bridge, no original ABI.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
void initialize_source_nis_camera_state(::port::driving::Memory&);
int LoaderCameras(::port::driving::Memory&,std::uint32_t); //00197D38/00197770.
int UnloaderCameras(::port::driving::Memory&,std::uint32_t); //00197AA8 original CameraLoader::Unloader wrapper.
}

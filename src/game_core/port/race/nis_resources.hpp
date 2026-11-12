#pragma once
// TASK-0112 port I/O ownership around original BIN/CAM chunk consumers.
#include "nis_services.hpp"
#include "port/assets/catalog.hpp"
#include <map>
#include <vector>
namespace port::race {
class NisResources {
 struct File {std::uint32_t base=0;std::vector<std::uint32_t> animations,cameras;};
 driving::Memory& memory_;assets::Source& source_;
 std::function<int(driving::Memory&,std::uint32_t)> load_camera_,unload_camera_;
 std::map<std::uint32_t,File> files_;
public:
 NisResources(driving::Memory&,assets::Source&,
  std::function<int(driving::Memory&,std::uint32_t)> camera_loader,
  std::function<int(driving::Memory&,std::uint32_t)> camera_unloader);
 std::uint32_t load(const std::string&); //Fully consumes actual chunks before returning opaque owner token.
 void unload(std::uint32_t);
 void close(); //Only after source NisManager/camera borrowers are destroyed.
 bool empty()const{return files_.empty();}
};
std::optional<NisClock> host_nis_clock(); //Explicit host local RTC device input, not PS2 clock equivalence.
}
namespace hp2::debug {
void Sub_001A53E8(::port::driving::Memory&); //Original animation-solid list initializer.
int LoaderAnimations(::port::driving::Memory&,std::uint32_t); //001A4D50.
int UnloaderAnimations(::port::driving::Memory&,std::uint32_t); //001A4E68.
std::uint32_t eFindAnimationSolid(::port::driving::Memory&,std::uint32_t); //001A4EA8.
}

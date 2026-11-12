#pragma once
// TASK-0112 E-0112-P03. Port-owned decoded source pause-screen state;
// not a PS2 C++ ABI. Source entrypoints remain in original pausemenu.cpp.
#include "pause_services.hpp"
#include <memory>
namespace port::race {
class PauseSubmenus {
public:
 explicit PauseSubmenus(driving::Memory&);
 ~PauseSubmenus();
 bool dispatch(std::uint32_t screen,hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&,const PauseServices&);
 void close(const PauseServices&);
 void annotate(std::vector<hp2::frontend::FeScreen>&) const; //Port copies source rectangle draw metadata.
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}

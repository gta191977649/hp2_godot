#pragma once
// E-0112-H01. Original ../indep/inc/Font.hpp referenced by captured glyph consumers.
// Explicit logical-memory view; no host/PS2 C++ ABI overlay.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
std::uint32_t next_power_of_2(std::uint32_t); //0013FD50.
std::uint32_t FindFont(::port::driving::Memory&,std::uint32_t); //0013FC08.
int LoaderFontInfo(::port::driving::Memory&,std::uint32_t); //0013FC58.
int UnloaderFontInfo(::port::driving::Memory&,std::uint32_t); //0013FCE8.
class Font:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_0013FD98(std::uint32_t data); //Original constructor; extent0x428.
 void Sub_00146468(int flags);
 std::uint32_t GetTextureInfo(); //0013FEE8.
 float GetWidestCharacter(std::uint32_t charset); //0013FFC8.
};
}

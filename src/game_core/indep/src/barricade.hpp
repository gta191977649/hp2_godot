#pragma once
// Port companion declarations; actual Debug definitions in barricade.cpp.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {class tBarricadeManager:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_00248418();void Sub_00248468(int);void DoSnapshot(std::uint32_t);void Update(float); /*002486F8,E-0100-PU02*/};}

#pragma once
// TASK-0113 E-0113-L03. Original Debug class, logical Memory binding only.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class tLoadingScreenAnimation:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_0016ECB0(int screen_hash);
 void Sub_0016ED80(int flags);
 void Tick(); //0016EDD8, actual source GameFlow/manual-transmission gates.
};
}

#pragma once
// TASK-0099 E-0099-W50. Original Camera.hpp spelling recovered from inline
// markers; explicit logical Memory view, never native PS2 object ABI.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class Camera:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001566E0();
 void SetCameraMatrix(std::uint32_t matrix,float time); //00156868.
};
}

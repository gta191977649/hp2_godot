#pragma once
#include "game_core/port/driving/object_view.hpp"
namespace hp2::recovered {
// TASK-0044 E-0044-E01, Debug SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
class Aerodynamics : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(); // excluded replay path, throws
 void ResolveForces(std::uint32_t output=0); //001AB3F0
 float GetDownforceAtSpeed(float speed); //001AB638
};
}

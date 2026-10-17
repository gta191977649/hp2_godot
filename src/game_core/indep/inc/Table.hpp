#pragma once
#include "game_core/port/driving/object_view.hpp"
// TASK-0044 E-0044-E01. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Explicit address view; native host layout is port-side.
namespace hp2::recovered {
class Table : public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    void Sub_0018D810(std::uint32_t values,std::uint32_t count,float minimum,float maximum);
    void SetMinMax(float minimum,float maximum); // 001872D0
    void CalcIndexMultiplier(); // 00187318
    float GetValue(float value); // 0018D840
    float InverseLookup(float value); // 0018D8D8
};
}

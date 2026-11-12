#pragma once
// Port-side memory-view companion; original source is src/bRegion.cpp.
#include "game_core/port/driving/object_view.hpp"
#include <string>
namespace hp2::debug {
class bRegion:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 // Debug00269D68 /0026AFE0, E-0099-W31.
 std::uint32_t Sample(const std::string&)const;std::uint32_t GetName()const{return address_;}
};
}

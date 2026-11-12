#pragma once
// TASK-0099 E-0099-W57. Original WorldObject.hpp/source class spelling; explicit
// logical shared-Memory view only, no host original-object ABI assertion.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class WorldObject:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001CA730(std::uint32_t name_hash,std::uint32_t matrix,bool add_lighting);
 void Sub_001CA7F8(int flags);
 static int DoesArtExist(::port::driving::Memory&,std::uint32_t name_hash); //001CA8A0 E-0099-W79.
 std::uint32_t GetNameHash()const; //001CA870 E-0099-W76.
};
void Sub_001CACF0(::port::driving::Memory&); // original process-static list
void InitWorldObjects(::port::driving::Memory&); //001CA8E8
void CloseWorldObjects(::port::driving::Memory&); //001CA918
}

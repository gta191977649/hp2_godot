#pragma once
// TASK-0099 E-0099-W109. Original bWare/inc/bList.hpp definition owner.
// Logical Memory APIs only; no original C++ ABI or host node layout asserted.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class bPNode {
public:
 static std::uint32_t Malloc(::port::driving::Memory&);
 static void Free(::port::driving::Memory&,std::uint32_t);
};
template<class T> class bPList:public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug001BCDE8 for T=AcidEmitter. Original bPNode dtor14E8C0 calls
 // nonowning bTNode dtor(flags0), then delete14E918 -> actual Free249AB8.
 void Remove(std::uint32_t node){const auto next=m_.word(node),prev=m_.word(node+4);m_.word(prev,next);m_.word(next+4,prev);bPNode::Free(m_,node);}
};
}

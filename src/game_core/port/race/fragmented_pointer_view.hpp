#pragma once
// Port-only views of confirmed bFragmentedVector2<T*> data, E-0099-W31.
// Original long fields are64-bit; source-owned objects retain the raw layout.
#include "game_core/port/driving/memory.hpp"
#include <stdexcept>
namespace port::race {
inline std::uint64_t fragmented_count(const ::port::driving::Memory& m,std::uint32_t at){return std::uint64_t(m.word(at))|(std::uint64_t(m.word(at+4))<<32);}
inline void fragmented_count(::port::driving::Memory& m,std::uint32_t at,std::uint64_t value){m.word(at,std::uint32_t(value));m.word(at+4,std::uint32_t(value>>32));}
inline std::uint32_t fragmented_pointer(const ::port::driving::Memory& m,std::uint32_t vector,std::uint64_t index){
 if(index>=fragmented_count(m,vector+8))throw std::out_of_range("Original fragmented pointer index outside vector");
 for(auto node=m.word(vector);node;node=m.word(node)){const auto start=fragmented_count(m,node+8),size=fragmented_count(m,node+16);
  if(index>=start && index-start<size)return m.word(m.word(node+4)+std::uint32_t(index-start)*4);}
 throw std::out_of_range("Original fragmented pointer index has no source array node");
}
}

// TASK-0099 E-0099-W31. Original src/bRegion.cpp; namespace/view storage port-side.
#include "game_core/src/bRegion.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/port/race/fragmented_pointer_view.hpp"
#include <cstdio>
namespace hp2::debug {
// Debug00269C68. Original first ordered case-insensitive name match. Failure
// returns the original null result with a diagnostic, never a default sample.
std::uint32_t bRegion::Sample(const std::string& name)const{
 const auto upper=[](std::uint8_t c){return std::uint32_t(c-'a')<26?std::uint8_t(c&0x5f):c;};
 for(std::uint64_t i=0;i<::port::race::fragmented_count(m_,address_+0x30);++i){const auto sample=::port::race::fragmented_pointer(m_,address_+0x28,i);
  const auto text=bSample(m_,sample).GetName();for(std::size_t j=0;;++j){const auto a=upper(m_.byte(text+std::uint32_t(j))),b=j<name.size()?upper(std::uint8_t(name[j])):std::uint8_t(0);
   if(a!=b)break;if(!a)return sample;}}
 std::fputs("Original bRegion cannot find requested sample\n",stderr);return 0;
}
}

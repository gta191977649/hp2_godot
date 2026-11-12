// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include <string>
namespace {
std::string port_campaign_text(const ::port::driving::Memory& m,std::uint32_t at){std::string text;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return text;text+=char(c);}throw std::out_of_range("Unterminated original campaign text");}
void port_campaign_copy(::port::driving::Memory& m,std::uint32_t to,const std::string& text){for(unsigned i=0;i<=text.size();++i)m.byte(to+i,i<text.size()?std::uint8_t(text[i]):0);}
}
namespace hp2::debug {
void tUnlockMap::Reset(::port::driving::Memory& m,std::uint32_t a){m.word(a+0x30,0);}
void tUnlockMap::AddGoal(::port::driving::Memory& m,std::uint32_t a,std::uint32_t goal,int event){const int count=m.signed_word(a+0x30);if(count<4){const auto node=a+std::uint32_t(count)*12;m.word(node+8,std::uint32_t(event));m.copy(node,goal,8);m.word(a+0x30,std::uint32_t(count)+1);}}
void tEventLeaf::BuildUnlockList(::port::driving::Memory& m,std::uint32_t a,std::uint32_t map){
 tUnlockMap::Reset(m,map);const auto list=a+0x28;
 for(auto node=m.word(list);node!=list;node=m.word(node)){const auto leaf=m.word(node+8);(void)EventInfo::GetObjective(m,m.word(a+0x14));
  for(int i=0;i<m.signed_byte(m.word(leaf+0x14)+0x3d);++i){const auto info=m.word(leaf+0x14),goal=EventInfo::GetGoal(m,info,i);if(int(m.half(goal+2))==m.signed_byte(m.word(a+0x14)+0x28))tUnlockMap::AddGoal(m,map,goal,m.signed_byte(m.word(leaf+0x14)+0x28));}
 }
}
// TASK-0099 E-0099-W72. Original case-sensitive prefix search and source token
// order. Unknown $ strings are copied literally by the caller, not discarded.
std::uint32_t GetEventDescriptionToken(const ::port::driving::Memory& m,std::uint32_t input){for(unsigned i=0;i<8;++i){const auto token=0x3128d0+8*i;const auto pattern=port_campaign_text(m,m.word(token+4));bool match=true;for(unsigned j=0;j<pattern.size();++j)if(m.byte(input+j)!=std::uint8_t(pattern[j])){match=false;break;}if(match)return token;}return 0;}
void FormatEventDescriptionToken(::port::driving::Memory& m,std::uint32_t tree,std::uint32_t info,std::uint32_t token,std::uint32_t out){std::string text;switch(m.word(token)){
 case 0:{const auto goal=m.word(tree+0x24)+std::int32_t(m.signed_half(info+0x42))*8;if(m.half(goal)==2){Timer timer{m.signed_word(goal+4)};timer.PrintToString(m,out,0);return;}text=port_campaign_text(m,0x328778);break;}
 case 1:{const auto track=m.word(tree+0x1c)+std::int32_t(m.signed_half(info+0x6c))*4;text=std::to_string((m.word(track)>>14)&15);break;}
 case 2:text=std::to_string(m.signed_byte(info+0x3f));break;
 case 3:text=std::to_string(EventInfo::GetRaceCount(m,info));break;
 // These literal placeholders are present in the original implementation;
 // they are not port-generated replacements for missing game behavior.
 case 4:text=port_campaign_text(m,0x2b2380);break;case 5:text=port_campaign_text(m,0x328788);break;case 6:text=port_campaign_text(m,0x2b23a0);break;case 7:text=port_campaign_text(m,0x2b23b8);break;
 default:hp2::not_implemented("0x001F1FD0","original unrecognized description token leaves output unwritten");
 }port_campaign_copy(m,out,text);
}
void FormatEventDescription(::port::driving::Memory& m,std::uint32_t tree,std::uint32_t info,std::uint32_t text,std::uint32_t out,int length){::port::driving::Memory::Temporary token_buffer(m,64);unsigned written=0;while(m.byte(text)&&std::int64_t(written)<length){if(m.byte(text)=='$'){if(const auto token=GetEventDescriptionToken(m,text)){text+=std::uint32_t(port_campaign_text(m,m.word(token+4)).size());FormatEventDescriptionToken(m,tree,info,token,token_buffer.address());for(unsigned i=0;m.byte(token_buffer.address()+i)&&std::int64_t(written)<length;++i)m.byte(out+written++,m.byte(token_buffer.address()+i));continue;}}
  m.byte(out+written++,m.byte(text++));
 }m.byte(out+written,0);
}
}

namespace hp2::recovered {
// ELF 0x001F5D00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f5d00.md.
void DataEventSelectScreen::GetSelectedEvent() {
    hp2::not_implemented("0x001F5D00", "DataEventSelectScreen::GetSelectedEvent (signature and full body unresolved)");
}

// ELF 0x001F5D28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f5d28.md.
void DataEventSelectScreen::SetSelectedEvent() {
    hp2::not_implemented("0x001F5D28", "DataEventSelectScreen::SetSelectedEvent (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F8C10; E-0041-R05; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f8c10.md.
void tCarLogoAnimation::Sub_001F8C10() {
    hp2::not_implemented("0x001F8C10", "tCarLogoAnimation::Sub_001F8C10 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/eventtree.hpp"

namespace hp2::frontend {
// ELF 0x001F7FE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f7fe8.md.
void tEventGroup::IsMember() {
    hp2::not_implemented("0x001F7FE8", "tEventGroup::IsMember (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001F7FA8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f7fa8.md.
void tEventLeaf::CalculateGeneration() {
    hp2::not_implemented("0x001F7FA8", "tEventLeaf::CalculateGeneration (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001FA840; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fa840.md.
void tEventTreeView::FindLeafByEventNumber() {
    hp2::not_implemented("0x001FA840", "tEventTreeView::FindLeafByEventNumber (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::recovered {
// ELF 0x001F8AD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f8ad8.md.
void tParallaxer::Sub_001F8AD8() {
    hp2::not_implemented("0x001F8AD8", "tParallaxer::Sub_001F8AD8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F90F0; E-0041-R06; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f90f0.md.
void tTrackLogoAnimation::Sub_001F90F0() {
    hp2::not_implemented("0x001F90F0", "tTrackLogoAnimation::Sub_001F90F0 (signature and full body unresolved)");
}

// ELF 0x001F91D0; E-0042-C151; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f91d0.md.
void tTrackLogoAnimation::Show() {
    hp2::not_implemented("0x001F91D0", "tTrackLogoAnimation::Show (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F65C8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001F65C8.md.
void tTreeReward::HandleMessages() {
    hp2::not_implemented("0x001F65C8", "tTreeReward::HandleMessages (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F72B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f72b0.md.
void tUnlockMap::AddGoal() {
    hp2::not_implemented("0x001F72B0", "tUnlockMap::AddGoal (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug001F1F00, E-0099-W86; actual source owner campaigntrees.cpp.
int DataEventSelectScreen::GetSelectedEvent(const ::port::driving::Memory& m,std::uint32_t a,int type){if(type==0)return m.signed_word(a+8);if(type==1)return m.signed_word(a+4);return 1;}
}

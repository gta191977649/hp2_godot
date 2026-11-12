// Exact Debug .mdebug original owner ..\indep\src\ecstasy.cpp, TASK-0099 E-0099-W22.
// Debug SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/damage_resources.hpp"
namespace hp2::debug {
// Debug001A5758; E-0099-W22. Null filter, previous marker, intrusive original list.
std::uint32_t eFindPositionMarker(Memory&m,std::uint32_t name,std::uint32_t type,std::uint32_t previous){
 if(name==0&&type==0)return 0;
 for(auto p=previous?m.word(previous):m.word(0x327df0);p!=0x327df0;p=m.word(p))
  if((name==0||m.word(p+8)==name)&&(type==0||m.word(p+0xc)==type))return p;
 return 0;
}
// Debug001A5670; E-0099-W22. Chunk address alignment is significant.
int LoaderPositionMarkers(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34013)return 0;
 const auto first=(chunk+0x17)&~15u,count=(m.word(chunk+4)-(first-(chunk+8)))>>5;
 for(unsigned i=0;i<count;++i){const auto p=first+32*i,tail=m.word(0x327df4);
  m.word(tail,p);m.word(p+4,tail);m.word(0x327df4,p);m.word(p,0x327df0);}
 return 1;
}
// Debug001A56E8; E-0099-W22.
int UnloaderPositionMarkers(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34013)return 0;
 const auto first=(chunk+0x17)&~15u,count=(m.word(chunk+4)-(first-(chunk+8)))>>5;
 for(unsigned i=0;i<count;++i){const auto p=first+32*i,next=m.word(p),prev=m.word(p+4);m.word(prev,next);m.word(next+4,prev);}
 return 1;
}
}
namespace port::race {
// Required PositionMarkerList member of original static initializer001A5950;
// E-0099-W22. Other dynamic frame lists belong to separate frame-memory startup.
void initialize_position_marker_list(port::driving::Memory&m){m.word(0x327df4,0x327df0);m.word(0x327df0,0x327df0);}
}

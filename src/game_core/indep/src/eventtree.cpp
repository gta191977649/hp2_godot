// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\eventtree.cpp`
// (string at 0x002BA9F8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include "game_core/port/race/frontend_event_resources.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/CarInfo.hpp"
#include "game_core/indep/inc/EndGameScreens.hpp"
#include "game_core/indep/inc/Language.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"
#include "game_core/indep/src/raceparameters.hpp"

namespace {
std::string port_event_text(const ::port::driving::Memory& m,std::uint32_t a){std::string text;for(unsigned i=0;i<4096;++i){const auto c=m.byte(a+i);if(!c)return text;text+=char(c);}throw std::out_of_range("Unterminated original EventTree string");}
bool port_event_equal(const std::string& a,const std::string& b){if(a.size()!=b.size())return false;const auto fold=[](unsigned char c){return c>='A'&&c<='Z'?c+32:c;};for(unsigned i=0;i<a.size();++i)if(fold(a[i])!=fold(b[i]))return false;return true;}
}
namespace hp2::debug {
// Debug001FC618 ->001FB968/001FBBB0, E-0101-PL01. LH/LB/LB retain signed
// extraction. Non-multitrack source leaves all three output operands untouched.
void EventInfo::GetSelectedCar(const ::port::driving::Memory& m,std::uint32_t a,int& car,int& skin,int& dash){
 const auto type=m.signed_byte(a+0x29);if(type!=3&&type!=4)return;
 const auto state=m.word(a+4),offset=type==3?4u:8u;
 car=m.signed_half(state+offset);skin=m.signed_byte(state+offset+2);dash=m.signed_byte(state+offset+3);
}
// TASK-0099 E-0099-W82. Debug001FB4A8; original definition eventtree.cpp.
// Passed explicitly supplies its goal, so the target does not recurse through
// its own unlock dependencies. Missing event numbers return false in source.
bool EventGoal::Passed(const ::port::driving::Memory& m,std::uint32_t goal,std::uint32_t tree){
    const auto event=EventTree::GetEventNumber(m,tree,m.half(goal+2));
    return event&&EventInfo::GetStatus(m,event,goal)==eEVENT_PASSED;
}
// Debug001FC2A8; E-0099-W82. Original global003287E8 and signed field reads
// confirmed by fresh symbol/reference/listing captures. Source evaluates all
// dependency goals and ORs their results; do not replace that with all-of or a
// short-circuit search. An explicit goal bypasses the unlock checks entirely.
eEventStatus EventInfo::GetStatus(const ::port::driving::Memory& m,std::uint32_t event,std::uint32_t goal){
    if(m.word(0x3287e8))return eEVENT_PASSED;
    if(!goal){
        if(m.signed_byte(event+0x28)==32){
            const auto pursuit=EventTree::GetEventNumber(m,m.word(0x328800),31);
            const auto ultimate=EventTree::GetEventNumber(m,m.word(0x3287fc),31);
            if(pursuit&&GetStatus(m,pursuit)!=eEVENT_PASSED)return eEVENT_LOCKED;
            if(ultimate&&GetStatus(m,ultimate)!=eEVENT_PASSED)return eEVENT_LOCKED;
        }else if(m.signed_byte(event+0x3d)>0){
            bool passed=false;
            for(int i=0;i<m.signed_byte(event+0x3d);++i){
                const auto current=GetGoal(m,event,i);
                const bool result=EventGoal::Passed(m,current,m.word(event));
                passed=passed|result;
            }
            if(!passed)return eEVENT_LOCKED;
        }
        goal=m.word(m.word(event)+0x24)+8*std::uint32_t(std::int32_t(m.signed_half(event+0x42)));
    }
    const auto state=m.word(event+4);
    switch(m.signed_byte(event+0x29)){
    case 0:case 2:return EventRankState::GetStatus(m,state,goal);
    case 1:case 5:case 6:return EventTimeState::GetStatus(m,state,goal);
    case 3:return EventKoState::GetStatus(m,state,goal);
    case 4:return EventTournamentState::GetStatus(m,state,goal,m.signed_byte(event+0x3c),m.signed_byte(event+0x3f));
    default:return eEVENT_UNTRIED;
    }
}
// Debug001FC1E8; E-0099-W82. Prediction reads actual current/opposite event31
// states and never changes a result, unlock flag, tree or profile field.
bool EventInfo::WillSuperBonusEventBeUnlockedIfThisEventIsWon(const ::port::driving::Memory& m,std::uint32_t event){
    if(m.signed_byte(event+0x28)!=31)return false;
    const auto tree=m.word(event);
    const auto opposite=m.word(m.word(tree+8)==1?0x3287fc:0x328800);
    if(!opposite)return false;
    const auto current=EventTree::GetEventNumber(m,tree,31);
    const auto other=EventTree::GetEventNumber(m,opposite,31);
    if(!current||!other)return false;
    const auto current_status=GetStatus(m,current);
    const auto other_status=GetStatus(m,other);
    return current_status!=eEVENT_PASSED&&other_status==eEVENT_PASSED;
}
// TASK-0099 E-0099-W78. Original primitive result-state lifecycle.
void EventRankState::Reset(::port::driving::Memory& m,std::uint32_t a){m.byte(a,0);}
eEventStatus EventRankState::GetStatus(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t goal){const auto rank=m.byte(a);return !rank?eEVENT_UNTRIED:rank<=m.word(goal+4)?eEVENT_PASSED:eEVENT_FAILED;}
void EventTimeState::Reset(::port::driving::Memory& m,std::uint32_t a){m.word(a,0);}
eEventStatus EventTimeState::GetStatus(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t goal){const int time=m.signed_word(a);return time==0||time==0x7fffffff?eEVENT_UNTRIED:time<=m.signed_word(goal+4)?eEVENT_PASSED:eEVENT_FAILED;}
void EventKoState::Reset(::port::driving::Memory& m,std::uint32_t a){m.word(a,0);}
eEventStatus EventKoState::GetStatus(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t goal){if(!GetRaceIndex(m,a))return eEVENT_UNTRIED;const unsigned rank=m.byte(a)&15u;return !rank?eEVENT_INPROGRESS:rank<=m.word(goal+4)?eEVENT_PASSED:eEVENT_FAILED;}
void EventTournamentState::Reset(::port::driving::Memory& m,std::uint32_t a){for(int i=7;i>=0;--i)m.byte(a+std::uint32_t(i),0);}
unsigned EventTournamentState::RacerRank(const ::port::driving::Memory& m,std::uint32_t a,int index){unsigned rank=1;for(int i=0;i<8;++i)if(i!=index){const auto own=m.byte(a+std::uint32_t(index)),other=m.byte(a+std::uint32_t(i));if(own<other||(own==other&&i<index))++rank;}return rank;}
unsigned EventTournamentState::UserRank(const ::port::driving::Memory& m,std::uint32_t a){return RacerRank(m,a,0);}
eEventStatus EventTournamentState::GetStatus(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t goal,int races,int opponents){const int index=GetRaceIndex(m,a,opponents);if(!index)return eEVENT_UNTRIED;if(index<races)return eEVENT_INPROGRESS;return UserRank(m,a)<=m.word(goal+4)?eEVENT_PASSED:eEVENT_FAILED;}
std::uint32_t RaceEvent::GetRankState(const ::port::driving::Memory& m,std::uint32_t a){const int type=m.signed_half(a+0x6a);return type==0||type==2?m.word(a+4):0;}
std::uint32_t RaceEvent::GetTournamentState(const ::port::driving::Memory& m,std::uint32_t a){return m.signed_half(a+0x6a)==4?m.word(a+4):0;}
std::uint32_t RaceEvent::GetKoState(const ::port::driving::Memory& m,std::uint32_t a){return m.signed_half(a+0x6a)==3?m.word(a+4):0;}
std::uint32_t RaceEvent::GetTimeState(const ::port::driving::Memory& m,std::uint32_t a){const int type=m.signed_half(a+0x6a);return type==1||type==5||type==6?m.word(a+4):0;}
eEventStatus RaceEvent::GetStatus(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t state){const auto goal=a+0x7c;switch(m.signed_half(a+0x6a)){case 0:case 2:return EventRankState::GetStatus(m,state,goal);case 1:case 5:case 6:return EventTimeState::GetStatus(m,state,goal);case 3:return EventKoState::GetStatus(m,state,goal);case 4:return EventTournamentState::GetStatus(m,state,goal,m.signed_half(a+0x70),m.signed_half(a+0x74));default:return eEVENT_UNTRIED;}}
eEventStatus RaceEvent::GetStatus(const ::port::driving::Memory& m,std::uint32_t a){return GetStatus(m,a,m.word(a+4));}
eEventStatus RaceEvent::GetOldStatus(const ::port::driving::Memory& m,std::uint32_t a){return GetStatus(m,a,a+8);}
bool RaceEvent::ResultShouldBeDiscarded(const ::port::driving::Memory& m,std::uint32_t a){
    const auto old=GetOldStatus(m,a),current=GetStatus(m,a);if(current<old)return true;if(old!=current)return false;
    switch(m.signed_half(a+0x6a)){
    case 0:case 2:{const auto state=GetRankState(m,a);if(!state)return false;const auto previous=m.byte(a+8);if(!previous)return false;const auto rank=m.byte(state);return !rank||previous<rank;}
    case 1:case 5:case 6:{const auto state=GetTimeState(m,a);if(!state)return false;const int previous=m.signed_word(a+8);if(previous==0||previous==0x7fffffff)return false;const int time=m.signed_word(state);return time==0||time==0x7fffffff||previous<time;}
    case 3:{if(!FrontendDatabase::UseTreeEvent(m,0x2f3890))return false;const auto state=GetKoState(m,a);if(!state)return false;const int previous=EventKoState::GetRank(m,a+8,0),rank=EventKoState::GetRank(m,state,0);if(!previous)return rank!=0;if(!rank)return false;return previous<rank;}
    case 4:{const auto state=GetTournamentState(m,a);return state&&old>eEVENT_UNTRIED&&EventTournamentState::UserRank(m,a+8)<EventTournamentState::UserRank(m,state);}
    default:return false;
    }
}
void RaceEvent::MaybeDiscardResult(::port::driving::Memory& m,std::uint32_t a){if(ResultShouldBeDiscarded(m,a))m.copy(m.word(a+4),a+8,std::uint32_t(GetStateSize(m,a)));}
int RaceEvent::GetRaceIndex(const ::port::driving::Memory& m,std::uint32_t a){const auto state=m.word(a+4);if(!state)return 0;switch(m.signed_half(a+0x6a)){case 3:return EventKoState::GetRaceIndex(m,state);case 4:return EventTournamentState::GetRaceIndex(m,state,m.signed_half(a+0x74));default:return 0;}}
void RaceEvent::ResetState(::port::driving::Memory& m,std::uint32_t a){const auto state=m.word(a+4);if(!state)return;switch(m.signed_half(a+0x6a)){case 0:case 2:EventRankState::Reset(m,state);break;case 1:case 5:case 6:EventTimeState::Reset(m,state);break;case 3:EventKoState::Reset(m,state);break;case 4:EventTournamentState::Reset(m,state);break;default:break;}}
int EventKoState::GetRank(const ::port::driving::Memory& m,std::uint32_t a,int index){return std::uint32_t(index)<8?int((m.word(a)>>(std::uint32_t(index)*4))&15):0;}
bool RaceEvent::RacerEliminated(const ::port::driving::Memory& m,std::uint32_t a,int index){return m.word(0x311f98)==4&&EventKoState::GetRank(m,m.word(a+4),index)!=0;}
bool RaceEvent::DriverEliminated(const ::port::driving::Memory& m,std::uint32_t a,int number){return RacerEliminated(m,a,RacerIndexFromDriverNumber(m,a,number));}
void EventTrack::SetRace(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race){
    m.word(race,m.byte(a));m.word(race+0x2c,(m.word(a)>>14)&15);m.word(race+4,(m.word(a)>>10)&3);m.word(race+8,(m.word(a)>>12)&3);m.word(race+12,(m.word(a)>>9)&1);
    m.scalar(race+0x10,float((m.word(a)>>18)&127)*m.scalar(0x325d10));const auto number=m.byte(a);m.scalar(race+0x14,float(m.word(a)>>25)*m.scalar(0x325d14));const auto track=hp2::debug::GetTrackInfo(m,number);if(track)m.word(race+0x28,std::uint32_t(m.signed_byte(track+0x43)));
}
// TASK-0099 E-0099-W72. Sparse raw-memory source overloads, no host ABI cast.
int EventInfo::GetStateSize(const ::port::driving::Memory& m,std::uint32_t a){switch(m.signed_byte(a+0x29)){case 1:case 5:case 6:return 4;case 3:return 12;case 4:return 16;default:return 1;}}
bool EventTree::SetName(::port::driving::Memory& m,std::uint32_t a,std::uint32_t name){if(const auto previous=m.word(a+12))m.release(previous);if(!name){m.word(a+12,0);return false;}const auto text=port_event_text(m,name);const auto copy=m.allocate(std::uint32_t(text.size()+1));for(unsigned i=0;i<=text.size();++i)m.byte(copy+i,i<text.size()?std::uint8_t(text[i]):0);m.word(a+12,copy);
 const bool e3=m.word(0x326e98)!=0;if(port_event_equal(text,port_event_text(m,e3?0x2b27c8:0x2b27d8))){m.word(0x328800,a);m.word(a+8,1);return true;}if(port_event_equal(text,port_event_text(m,e3?0x2b27e8:0x2b27f8))){m.word(0x3287fc,a);m.word(a+8,0);return true;}return false;
}
void EventTree::Sub_001F8688(::port::driving::Memory& m,std::uint32_t a,std::uint32_t name,std::optional<std::uint32_t> center_w){m.word(a+12,0);SetName(m,a,name);
 for(auto offset:{0x10u,0x8u,0x14u,0x18u,0x1cu,0x20u,0x24u,0x28u,0x2cu,0x30u,0x3cu,0x40u,0x44u,0x48u,0x4cu,0x50u,0x54u,0x58u,0x5cu})m.word(a+offset,0);
 if(!center_w){hp2::not_implemented("0x001F872C","original vCenter.W comes from unwritten stack word; explicit port operand required");}m.word(a+0x60,0);m.word(a+0x64,0);m.word(a+0x68,0);m.word(a+0x6c,*center_w);m.word(a+0x70,1);m.matrix(a+0x80,hp2::recovered::bIdentity());m.matrix(a+0xc0,hp2::recovered::bIdentity());for(unsigned i=0;i<8;++i)m.word(a+0x100+4*i,0);for(unsigned i=0;i<6;++i)m.word(a+0x100+4*i,GetTextureInfo(m,m.word(0x312fc0+4*i),1,0));
}
void EventTree::Sub_001F87A8(::port::driving::Memory& m,std::uint32_t a,int flags){if(const auto name=m.word(a+12))m.release(name);if(m.word(a+0x40))hp2::not_implemented("0x001F87A8","active original EventLeaf array destruction is required");if(flags&1)m.release(a);}
void EventTree::SetEvents(::port::driving::Memory& m,std::uint32_t a,std::uint32_t events,int bytes){m.word(a+0x3c,events);const int count=bytes/0xbc;m.word(a+0x5c,events);m.word(a+0x44,std::uint32_t(count));for(int i=0;i<count;++i)m.word(events+std::uint32_t(i)*0xbc,a);}
void EventTree::CalcStatePointers(::port::driving::Memory& m,std::uint32_t a,std::uint32_t buffer,int size,int alignment){(void)size;m.word(a+0x48,0);
 const auto assign=[&](std::uint32_t event,bool aligned){if(m.signed_byte(event+0x28)==32&&m.word(0x2f3890+0x384)){m.word(event+4,m.word(0x2f3890+0x384));return;}
  m.word(event+4,buffer+m.word(a+0x48));const int bytes=EventInfo::GetStateSize(m,event),old=m.signed_word(a+0x48);int next=old+bytes;if(aligned){if(!alignment)hp2::not_implemented("0x001F9F88","original divide-by-zero trap in event-state alignment");next=((old+bytes-1+alignment)/alignment)*alignment;}m.word(a+0x48,std::uint32_t(next));if(m.signed_byte(event+0x28)==32)m.word(0x2f3890+0x384,m.word(event+4));};
 for(int pass=0;pass<2;++pass)for(int i=0;i<m.signed_word(a+0x44);++i){const auto event=m.word(a+0x3c)+std::uint32_t(i)*0xbc;const int bytes=EventInfo::GetStateSize(m,event);if((pass==0&&bytes>=alignment)||(pass==1&&bytes<alignment))assign(event,pass==0);}
}
std::uint32_t EventTree::GetEventNumber(const ::port::driving::Memory& m,std::uint32_t a,int number){for(int i=0;i<m.signed_word(a+0x44);++i){const auto event=m.word(a+0x3c)+std::uint32_t(i)*0xbc;if(m.signed_byte(event+0x28)==number)return event;}return 0;}
std::uint32_t GetEventTree(const ::port::driving::Memory& m,const std::string& name){for(auto tree=m.word(0x3287f0);tree!=0x3287f0;tree=m.word(tree))if(port_event_equal(name,port_event_text(m,m.word(tree+12))))return tree;return 0;}
int LoaderEventTree(::port::driving::Memory& m,std::uint32_t chunk,std::optional<std::uint32_t> center_w){if(m.word(chunk)!=0x80034a00)return 0;const auto tree=m.allocate(0x120);EventTree::Sub_001F8688(m,tree,0,center_w);const auto tail=m.word(0x3287f4);m.word(tail,tree);m.word(tree+4,tail);m.word(0x3287f4,tree);m.word(tree,0x3287f0);
 const std::uint64_t end=std::uint64_t(chunk)+8+m.word(chunk+4);for(auto child=chunk+8;child<end;){const auto tag=m.word(child),bytes=m.word(child+4),payload=child+8;if(std::uint64_t(child)+8+bytes>end)throw std::out_of_range("Original EventTree child exceeds retained resource");
  switch(tag){case 1:if(!EventTree::SetName(m,tree,payload)){const auto next=m.word(tree),previous=m.word(tree+4);m.word(previous,next);m.word(next+4,previous);EventTree::Sub_001F87A8(m,tree,3);hp2::not_implemented("0x001FB248","unrecognized source EventTree name follows original deleted-tree branch");}break;
   case 0x34a01:m.word(tree+0x14,payload);m.word(tree+0x18,bytes>>2);break;case 0x34a02:m.word(tree+0x1c,payload);m.word(tree+0x20,bytes>>2);break;
   case 0x34a03:m.word(tree+0x24,payload);m.word(tree+0x28,bytes>>3);break;case 0x34a04:m.word(tree+0x2c,payload);m.word(tree+0x30,bytes>>3);break;
   case 0x34a06:m.word(tree+0x34,payload);m.word(tree+0x38,bytes>>2);break;
   case 0x34a05:EventTree::SetEvents(m,tree,payload,int(bytes));if(m.word(tree+8)==0)EventTree::CalcStatePointers(m,tree,0x2f3890+0x184,256,4);else if(m.word(tree+8)==1)EventTree::CalcStatePointers(m,tree,0x2f3890+0x284,256,4);break;
   default:break;
  }child+=bytes+8;
 }return 1;
}
int UnloaderEventTree(::port::driving::Memory& m,std::uint32_t chunk){if(m.word(chunk)!=0x80034a00)return 0;m.word(0x3287f8,0);m.word(0x3287fc,0);m.word(0x328800,0);while(m.word(0x3287f0)!=0x3287f0){const auto tree=m.word(0x3287f0),next=m.word(tree),previous=m.word(tree+4);m.word(0x3287f0,tree);m.word(previous,next);m.word(next+4,previous);EventTree::Sub_001F87A8(m,tree,3);}m.word(0x3287f0,0x3287f0);return 1;}
}

namespace {
std::uint32_t port_raw_event_item(const ::port::driving::Memory& m,std::uint32_t info,int index,unsigned count,unsigned indices,unsigned table,unsigned stride){if(index<0||index>=m.signed_byte(info+count))return 0;return m.word(m.word(info)+table)+std::int32_t(m.signed_half(info+indices+2*std::uint32_t(index)))*stride;}
void port_event_strncpy(::port::driving::Memory& m,std::uint32_t to,std::uint32_t from,unsigned count){bool ended=false;for(unsigned i=0;i<count;++i){const auto c=ended?0:m.byte(from+i);m.byte(to+i,std::uint8_t(c));if(!c)ended=true;}}
void port_event_strcpy(::port::driving::Memory& m,std::uint32_t to,std::uint32_t from){for(unsigned i=0;i<4096;++i){const auto c=m.byte(from+i);m.byte(to+i,c);if(!c)return;}throw std::out_of_range("Unterminated original selected event text");}
}
namespace hp2::debug {
// E-0099-W72. Original raw EventInfo table getters retain null index paths.
std::uint32_t EventInfo::GetCar(const ::port::driving::Memory& m,std::uint32_t a,int i){return port_raw_event_item(m,a,i,0x3b,0x44,0x14,4);}
std::uint32_t EventInfo::GetGoal(const ::port::driving::Memory& m,std::uint32_t a,int i){return port_raw_event_item(m,a,i,0x3d,0x7c,0x24,8);}
std::uint32_t EventInfo::GetTrack(const ::port::driving::Memory& m,std::uint32_t a,int i){return port_raw_event_item(m,a,i,0x3c,0x6c,0x1c,4);}
std::uint32_t EventInfo::GetReward(const ::port::driving::Memory& m,std::uint32_t a,int i){return port_raw_event_item(m,a,i,0x3e,0x8c,0x2c,8);}
std::uint32_t EventInfo::GetOpponent(const ::port::driving::Memory& m,std::uint32_t a,int i){return port_raw_event_item(m,a,i,0x3f,0x9c,0x34,4);}
std::uint32_t EventInfo::GetObjective(const ::port::driving::Memory& m,std::uint32_t a){return m.word(m.word(a)+0x24)+std::int32_t(m.signed_half(a+0x42))*8;}
int EventInfo::GetRaceCount(const ::port::driving::Memory& m,std::uint32_t a){const auto type=m.signed_byte(a+0x29);return type==3?m.signed_byte(a+0x3f):type==4?m.signed_byte(a+0x3c):1;}
void EventResult::Reset(::port::driving::Memory& m,std::uint32_t a){m.word(a+8,0);m.word(a,0);m.word(a+4,0);}
void RaceEvent::ResetResults(::port::driving::Memory& m,std::uint32_t a){for(unsigned i=0;i<8;++i)EventResult::Reset(m,a+0x150+12*i);}
int EventKoState::GetRaceIndex(const ::port::driving::Memory& m,std::uint32_t a){const auto ranks=m.word(a);int count=0;for(unsigned i=0;i<8;++i)count+=((ranks>>(4*i))&15)!=0;return count;}
int EventKoState::GetRandomSeed(const ::port::driving::Memory& m,std::uint32_t a){return m.signed_word(a+8);}
void EventKoState::NewRandomSeed(::port::driving::Memory& m,std::uint32_t a){m.word(a+8,hp2::recovered::bRandom(m,-1));}
int EventTournamentState::GetRaceIndex(const ::port::driving::Memory& m,std::uint32_t a,int opponents){int possible=0,earned=0;for(int i=0;i<=opponents;++i)possible+=m.byte(0x328830+std::uint32_t(i));for(unsigned i=0;i<8;++i)earned+=m.byte(a+i);if(!possible)hp2::not_implemented("0x001FB9F0","original tournament points division trap");return earned/possible;}
int EventTournamentState::GetRandomSeed(const ::port::driving::Memory& m,std::uint32_t a){return m.signed_word(a+12);}
void EventTournamentState::NewRandomSeed(::port::driving::Memory& m,std::uint32_t a){m.word(a+12,hp2::recovered::bRandom(m,-1));}
// Debug001FA378. Complete source selected-event producer, including actual
// language/reward/description dependencies. No successful game callbacks or
// blank objective/reward substitutes are accepted by this raw overload.
void EventTree::SetRace(::port::driving::Memory& m,std::uint32_t tree,std::uint32_t race,std::uint32_t info){constexpr auto event=0x312cd0u;m.word(tree+0x5c,info);m.word(0x2f3890+0x10730,std::uint32_t(int(m.signed_byte(info+0x28))));
 for(unsigned i=0;i<32;++i){m.byte(event+0x28+i,m.byte(info+8+i));}const int bytes=EventInfo::GetStateSize(m,info);for(int i=0;i<bytes;++i)m.byte(event+8+std::uint32_t(i),m.byte(m.word(info+4)+std::uint32_t(i)));m.word(event,m.word(info));m.word(event+4,m.word(info+4));
 const unsigned fields[][2]={{0x68,0x28},{0x6a,0x29},{0x6c,0x2a},{0x6e,0x3b},{0x70,0x3c},{0x72,0x3e}};for(const auto& field:fields)m.half(event+field[0],std::uint16_t(std::int16_t(m.signed_byte(info+field[1]))));m.half(event+0x76,1);m.half(event+0x74,std::uint16_t(std::int16_t(m.signed_byte(info+0x3f))));
 const auto objective=EventInfo::GetObjective(m,info);m.word(event+0x7c,m.word(objective));m.word(event+0x80,m.word(objective+4));
 for(int i=0;i<m.signed_byte(info+0x3b);++i)m.word(event+0x84+4*std::uint32_t(i),m.word(EventInfo::GetCar(m,info,i)));
 for(int i=0;i<m.signed_byte(info+0x3c);++i)m.word(event+0xd4+4*std::uint32_t(i),m.word(EventInfo::GetTrack(m,info,i)));
 for(int i=0;i<m.signed_byte(info+0x3e);++i){const auto reward=EventInfo::GetReward(m,info,i),to=event+0xf4+8*std::uint32_t(i);m.word(to,m.word(reward));m.word(to+4,m.word(reward+4));}
 for(int i=0;i<m.signed_byte(info+0x3f);++i)m.word(event+0x134+4*std::uint32_t(i),m.word(EventInfo::GetOpponent(m,info,i)));
 if(m.signed_byte(info+0x3e)<1)port_event_strcpy(m,event+0x48,GetTranslatedString(m,std::uint32_t(0x2b2818)));
 else{std::uint32_t name;const auto reward=event+0xf4;if(m.byte(reward)==0){const auto type=m.byte(reward+1);const auto full=CarTypeInfo::GetFullName(m,m.word(0x3278e0)+std::uint32_t(type)*0x560);name=GetTranslatedString(m,full);}
  else if(m.byte(reward)==1)name=GetTranslatedString(m,GetTheRightTrackLanguageID(m,int(m.byte(reward+1)),int(m.byte(reward+2))));
  else{port_event_strcpy(m,event+0x48,GetTranslatedString(m,std::uint32_t(0x2b2808)));name=0;}
  if(name)port_event_strncpy(m,event+0x48,name,32);
 }
 FormatEventDescription(m,tree,info,GetTranslatedString(m,m.signed_word(info+0xb4)),race+0x54,512);m.word(race+0x50,1);RaceEvent::ResetResults(m,event);m.word(race+0x27c,2);m.word(race+0x274,0);
 const int type=m.signed_half(event+0x6a);switch(type){case 1:m.word(race+0x20,2);m.word(race+0x24,m.word(event+0x80));break;case 2:m.word(race+0x20,3);break;
  case 3:{const auto state=m.word(info+4);m.word(race+0x20,4);if(!EventKoState::GetRaceIndex(m,state))EventKoState::NewRandomSeed(m,state);m.word(race+0x30c,std::uint32_t(EventKoState::GetRandomSeed(m,state)));break;}
  case 4:{const auto state=m.word(info+4);m.word(race+0x20,5);if(!EventTournamentState::GetRaceIndex(m,state,m.signed_byte(info+0x3f)))EventTournamentState::NewRandomSeed(m,state);m.word(race+0x30c,std::uint32_t(EventTournamentState::GetRandomSeed(m,state)));break;}
  case 5:m.word(race+0x28,1);m.word(race+0x20,1);m.word(race+0x24,m.word(event+0x80));break;
  case 6:m.word(race+0x27c,0);m.word(race+0x274,1);m.word(race+0x278,0);m.word(race+0x20,1);m.word(race+0x24,m.word(event+0x80));break;
  default:m.word(race+0x20,1);break;
 }
 m.word(race+0x30,1);m.word(race+0x34,std::uint32_t(int(m.signed_byte(info+0x3f))));m.word(race+0x25c,m.word(tree+8)==1?6:0);m.word(race+0x38,std::uint32_t(int(m.signed_byte(info+0x2b))));m.word(race+0x288,m.word(info+0x30));
 const unsigned flags[][2]={{0x260,0x37},{0x264,0x34},{0x268,0x36},{0x26c,0x35},{0x270,0x38},{0x280,0x39}};for(const auto& flag:flags)m.word(race+flag[0],std::uint32_t(int(m.signed_byte(info+flag[1]))));m.word(race+0x4c,0);m.word(race+0x284,std::uint32_t(int(m.signed_byte(info+0x3a))));m.word(race+0x40,m.word(info+0xb8)!=0);m.byte(race+0x3c,std::uint8_t(m.byte(info+0x3f)+1));m.word(race+0x2fc,m.word(info+0xac));m.word(race+0x300,m.word(info+0xac));
}
}

namespace hp2::frontend {
namespace {

// Port-side bounds checks around recovered slice readers; not original failure behavior.
namespace port {
const std::uint8_t* profile_slice(const EventTree_001FFC40& tree,
                                const hp2::memorycard::Profile_002FC050& profile, int index) {
    const auto layout = build_profile_layout_001FE988(tree);
    const auto* block = profile.EventBlock_001FE988(tree.type_0x08);
    if (!block || index < 0 || static_cast<std::size_t>(index) >= layout.offsets.size())
        throw std::out_of_range("invalid event profile slice");
    const auto offset = layout.offsets[static_cast<std::size_t>(index)];
    if (offset < 0 || offset + EventSaveSize_00200DE8(tree.records[index].goal_type_0x29) > 256)
        throw std::out_of_range("event profile slice exceeds block");
    return block + offset;
}
} // namespace port

int GetRaceIndex(std::uint32_t value) { // E-0022-D02, ELF 002001A8
    int count = 0;
    for (int i = 0; i < 8; ++i) count += ((value >> (i * 4)) & 15) != 0;
    return count;
}
int Sub_002006A0(const std::uint8_t* bytes, int count) { // E-0022-D02, ELF 002006A0
    constexpr std::uint8_t points_003317E8[8] = {10, 6, 4, 3, 2, 1, 0, 0};
    if (count < 0 || count >= 8)
        hp2::not_implemented("0x002006A0", "point-count table index outside recovered eight bytes");
    int divisor = 0, sum = 0;
    for (int i = 0; i <= count; ++i) divisor += points_003317E8[i];
    for (int i = 0; i < 8; ++i) sum += bytes[i];
    return sum / divisor;
}
int RacerRank(const std::uint8_t* bytes, int index) { // E-0022-D02, ELF 00200710
    int rank = 1;
    for (int i = 0; i < 8; ++i)
        if (i != index && (bytes[i] > bytes[index] || (bytes[i] == bytes[index] && i < index)))
            ++rank;
    return rank;
}

// Port-side safe representation of the recovered sprintf calls. Formats in the
// language data are parsed rather than forwarded as a variadic C format string.
namespace port {
std::string format_operands(const std::string& format, const std::vector<std::string>& operands) {
    std::string out;
    std::size_t operand = 0;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%') { out += format[i]; continue; }
        if (++i == format.size()) throw std::runtime_error("truncated event text format");
        if (format[i] == '%') { out += '%'; continue; }
        if ((format[i] != 'd' && format[i] != 'i' && format[i] != 's') || operand >= operands.size())
            hp2::not_implemented("0x00251398", "event localized format conversion not implemented");
        out += operands[operand++];
    }
    return out;
}
} // namespace port

std::uint32_t read_u32(const std::uint8_t* d, std::size_t off) {
    return static_cast<std::uint32_t>(d[off]) | (static_cast<std::uint32_t>(d[off + 1]) << 8) |
           (static_cast<std::uint32_t>(d[off + 2]) << 16) |
           (static_cast<std::uint32_t>(d[off + 3]) << 24);
}

std::uint16_t read_u16(const std::uint8_t* d, std::size_t off) {
    return static_cast<std::uint16_t>(d[off] | (d[off + 1] << 8));
}

std::string read_fixed_string(const std::uint8_t* d, std::size_t off, std::size_t max_len) {
    std::size_t len = 0;
    while (len < max_len && d[off + len] != 0) ++len;
    return std::string(reinterpret_cast<const char*>(d + off), len);
}

void collect(const std::vector<hp2::assets::Chunk>& chunks, std::uint32_t tag,
             std::vector<const hp2::assets::Chunk*>& out) {
    for (const auto& chunk : chunks) {
        if (chunk.tag == tag) out.push_back(&chunk);
        if (chunk.is_container()) collect(chunk.children, tag, out);
    }
}

const hp2::assets::Chunk* child(const hp2::assets::Chunk& parent, std::uint32_t tag) {
    for (const auto& c : parent.children) {
        if (c.tag == tag) return &c;
    }
    return nullptr;
}

} // namespace

const char* EventPanelTexture_001F75F0(int tree_type, int status) {
    // FUN_001F75F0: the switch has no arm for any other status, and leaves the texture unset.
    if (tree_type == 0) {
        switch (status) {
            case kEventStatusLocked: return kSpeedPanelLocked_002BA768;
            case kEventStatusUnlocked:
            case kEventStatusUnlocked2:
            case kEventStatusUnlocked3: return kSpeedPanelUnlocked_002BA788;
            case kEventStatusWon: return kSpeedPanelWon_002BA7A8;
            default: return nullptr;
        }
    }
    if (tree_type == 1) {
        switch (status) {
            case kEventStatusLocked: return kPursuitPanelLocked_002BA7C0;
            case kEventStatusUnlocked:
            case kEventStatusUnlocked2:
            case kEventStatusUnlocked3: return kPursuitPanelUnlocked_002BA7E0;
            case kEventStatusWon: return kPursuitPanelWon_002BA800;
            default: return nullptr;
        }
    }
    return nullptr;
}

int EventSaveSize_00200DE8(std::uint8_t goal_type) {
    switch (goal_type) {
        case 0:
        case 2: return 1;
        case 1:
        case 5:
        case 6: return 4;
        case 3:
        case 4: return 0x10;
        default: return 1; // the original's own default arm
    }
}

const char* EventTree_001FFC40::layout_map() const {
    if (type_0x08 == 0) return kLayoutChampionship_0031B3C0;
    if (type_0x08 == 1) return kLayoutHotPursuit_0031B490;
    return nullptr;
}

int EventTree_001FFC40::FindLeafByEventNumber(std::uint8_t number) const {
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (records[i].number_0x28 == number) return static_cast<int>(i);
    }
    return -1;
}

int EventTree_001FFC40::StatusWithoutProfile_002010B8(int record_index) const {
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) {
        return kEventStatusLocked;
    }
    // TASK-0018's reduction, retained for callers that hold no profile. An event with no
    // requirement rows reports Unlocked, everything else Locked.
    const auto& record = records[static_cast<std::size_t>(record_index)];
    if (record.number_0x28 == 0x1f || record.number_0x28 == 0x20) return kEventStatusLocked;
    if (record.nGoals < 1) return kEventStatusUnlocked;
    return kEventStatusLocked;
}

const EventLink_002008B8* EventTree_001FFC40::GetGoal(int record_index, int i) const {
    // FUN_002008B8, verbatim: the bound is record+0x3D and the row index comes from the s16
    // array at record+0x7C. The original returns 0 for an out-of-range `i`.
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) return nullptr;
    const auto& record = records[static_cast<std::size_t>(record_index)];
    if (i < 0 || i >= static_cast<int>(record.nGoals)) return nullptr;
    if (static_cast<std::size_t>(i) >= record.aGoals.size()) return nullptr;
    const std::int16_t row = record.aGoals[static_cast<std::size_t>(i)];
    if (row < 0 || static_cast<std::size_t>(row) >= links.size()) return nullptr;
    return &links[static_cast<std::size_t>(row)];
}

int EventTree_001FFC40::Status_002010B8(const hp2::memorycard::Profile_002FC050& profile,
                                        int record_index,
                                        const EventLink_002008B8* row) const {
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) {
        return kEventStatusLocked;
    }
    const auto& record = records[static_cast<std::size_t>(record_index)];

    // FUN_002010B8 opens with `if (iGpffffbeb0 == 0)` and answers 4 for every event otherwise.
    // That global is an unlock-all debug switch whose producer is not traced, so the port holds
    // it at its released value, zero, and the branch is not reachable here. Recorded as
    // TASK-0021 unresolved question 2 rather than exposed as a port-side cheat.

    // The two special cases. `bVar3` in the decompile is event 0x1F's gate; the original ORs it
    // into the final answer, so a locked 0x1F still reports 1 when either completion global is
    // set. Neither global is recovered, so reaching this at all is an untranslated path.
    if (record.number_0x28 == 0x1f) {
        hp2::not_implemented("0x002010B8",
                             "event 31's gate reads DAT_0031D7F4+0x180 / +0x1B8, which are not "
                             "recovered");
    }

    if (row == nullptr) {
        // The original's `param_2 == 0` entry: run the prerequisite test, then pick this
        // record's own requirement row from record+0x42 and judge the slice against it.
        if (record.number_0x28 == 0x20) {
            hp2::not_implemented("0x002010B8",
                                 "event 32 requires events 1..31 of BOTH trees to be Won, which "
                                 "needs the other tree's view; not translated");
        }
        if (record.nGoals >= 1) {
            bool satisfied = false;
            for (int i = 0; i < static_cast<int>(record.nGoals); ++i) {
                const EventLink_002008B8* requirement = GetGoal(record_index, i);
                if (requirement == nullptr) continue;
                // FUN_001FFFB0(row, tree): look the prerequisite up by number and ask for its
                // status judged against THIS row's threshold.
                const int prerequisite = FindLeafByEventNumber(
                    static_cast<std::uint8_t>(requirement->prerequisite_number_0x02));
                if (prerequisite < 0) continue;
                if (Status_002010B8(profile, prerequisite, requirement) == kEventStatusWon) {
                    satisfied = true;
                }
            }
            if (!satisfied) return kEventStatusLocked;
        }
        if (static_cast<std::size_t>(record.index_0x42) >= links.size()) return kEventStatusLocked;
        row = &links[static_cast<std::size_t>(record.index_0x42)];
    }

    // The GoalType switch on record+0x29. (E-0021-11)
    const std::uint8_t* block = profile.EventBlock_001FE988(type_0x08);
    if (block == nullptr) return kEventStatusLocked;
    const EventProfileLayout_001FE988 layout = build_profile_layout_001FE988(*this);
    if (static_cast<std::size_t>(record_index) >= layout.offsets.size()) {
        return kEventStatusLocked;
    }
    const int slice_offset = layout.offsets[static_cast<std::size_t>(record_index)];
    if (slice_offset < 0 ||
        slice_offset + EventSaveSize_00200DE8(record.goal_type_0x29) >
            static_cast<int>(hp2::memorycard::kEventBlockSize_001FE988)) {
        return kEventStatusLocked;
    }
    const std::uint8_t* slice = block + slice_offset;

    switch (record.goal_type_0x29) {
    case 0:
    case 2:
        return hp2::memorycard::EventRankState::GetStatus(slice[0], row->goal_threshold_0x04);
    case 1:
    case 5:
    case 6:
        return hp2::memorycard::EventTimeState::GetStatus(
            static_cast<std::int32_t>(read_u32(slice, 0)),
            static_cast<std::int32_t>(row->goal_threshold_0x04));
    case 3:
        // E-0022-D02, 002003F8: any nonzero nibble means progress; low nibble is result.
        if (GetRaceIndex(read_u32(slice, 0)) == 0) return 1;
        if ((slice[0] & 15) == 0) return 2;
        return (slice[0] & 15) <= row->goal_threshold_0x04 ? 4 : 3;
    case 4:
        // E-0022-D02, 002007D0: completed rounds from sum/points, result from ranking.
        {
            const int progress = Sub_002006A0(slice, record.Field_0x3F);
            if (progress == 0) return 1;
            if (progress < record.nTracks) return 2;
            return static_cast<std::uint32_t>(RacerRank(slice, 0)) <= row->goal_threshold_0x04 ? 4 : 3;
        }
    default:
        // The original's switch has no default arm, so `uVar7` keeps its initialiser, 1.
        return kEventStatusUnlocked;
    }
}

int EventTree_001FFC40::WalkSameDepth_001FA880(int record_index, int direction) const {
    // FUN_001FA880, verbatim. It walks the view's whole leaf list, skips the current leaf, keeps
    // only leaves whose +0x1C (depth) equals the current leaf's, and tracks the nearest X on the
    // requested side. The initial "best" values are the literals at gp-0x6CBC and gp-0x6CB8, and
    // the comparisons are strict, so an exact X tie is not a candidate and there is no wrap.
    if (record_index < 0 || record_index >= static_cast<int>(leaves.size())) return -1;
    if (direction != -1 && direction != 1) return -1;
    const auto& current = leaves[static_cast<std::size_t>(record_index)];

    float best = direction == -1 ? kWalkWorstLeft_gp_0x6CBC : kWalkWorstRight_gp_0x6CB8;
    int found = -1;
    for (std::size_t i = 0; i < leaves.size(); ++i) {
        if (static_cast<int>(i) == record_index) continue;
        const auto& candidate = leaves[i];
        if (candidate.depth_0x1C != current.depth_0x1C) continue;
        if (direction == -1) {
            if (candidate.x < current.x && best < candidate.x) {
                best = candidate.x;
                found = static_cast<int>(i);
            }
        } else {
            if (current.x < candidate.x && candidate.x < best) {
                best = candidate.x;
                found = static_cast<int>(i);
            }
        }
    }
    return found;
}

int EventTree_001FFC40::WalkAdjacent_001FA978(int record_index, int direction) const {
    // FUN_001FA978, verbatim: the candidate with the smallest |x - current.x| on the chosen
    // adjacency list, starting from gp-0x6CB4 == 999999.0f, strict comparison so the first of
    // equal distances wins. FUN_001FA9C8 passes leaf+0x20 for +2 and leaf+0x28 for -2;
    // FUN_001F96B8 threads +0x20 with this leaf's prerequisites and +0x28 with the leaves that
    // require it, so +2 walks towards the root.
    if (record_index < 0 || record_index >= static_cast<int>(leaves.size())) return -1;
    if (direction != 2 && direction != -2) return -1;
    const auto& current = leaves[static_cast<std::size_t>(record_index)];
    const std::vector<int>& list = direction == 2 ? current.incoming : current.outgoing;

    float best = kWalkWorstDistance_gp_0x6CB4;
    int found = -1;
    for (int candidate : list) {
        if (candidate < 0 || candidate >= static_cast<int>(leaves.size())) continue;
        const float distance =
            std::fabs(leaves[static_cast<std::size_t>(candidate)].x - current.x);
        if (distance < best) {
            best = distance;
            found = candidate;
        }
    }
    return found;
}

int EventTree_001FFC40::MoveCursor_001FA9C8(int record_index, int direction) const {
    // FUN_001FA9C8's own dispatch, including its quirk: the switch tests -1 first, then <0 for
    // -2, then 1, then 2, and the -1 arm is also what an unrecognised positive value falls into.
    // Only the four values the handler ever passes are modelled.
    if (direction == -1 || direction == 1) return WalkSameDepth_001FA880(record_index, direction);
    if (direction == 2 || direction == -2) return WalkAdjacent_001FA978(record_index, direction);
    return -1;
}

std::uint32_t TitleStringId_001F96B8(int selected_mode) {
    // FUN_001F96B8's three-way on DAT_0030C77C. (E-0021-16)
    if (selected_mode == 8) return kTitleStringChampionship_001F96B8;
    if (selected_mode == 9) return kTitleStringHotPursuit_001F96B8;
    return 0;
}

std::vector<EventTree_001FFC40> parse_event_trees(const std::vector<std::uint8_t>& buffer) {
    std::vector<EventTree_001FFC40> trees;
    auto chunks = hp2::assets::parse_chunks_top(buffer);
    std::vector<const hp2::assets::Chunk*> containers;
    collect(chunks, kEventTreeChunkId_80034A00, containers);

    for (const auto* container : containers) {
        EventTree_001FFC40 tree;

        if (const auto* name = child(*container, kEventTreeNameChunkId_00000001)) {
            tree.name = read_fixed_string(buffer.data(), name->payload_offset(), name->size);
        }
        // FUN_001FDEF8 compares the parsed name against the four known tree names and stores the
        // type; anything else leaves the tree untyped, which is what -1 means here.
        if (tree.name == "Championship" || tree.name == "Championship E3") tree.type_0x08 = 0;
        if (tree.name == "Hot Pursuit" || tree.name == "Hot Pursuit E3") tree.type_0x08 = 1;

        if (const auto* rows = child(*container, kEventTreeChunk_00034A01)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 4 <= rows->size; off += 4)
                tree.pCars.push_back(read_u32(d, off));
        }

        if (const auto* rows = child(*container, kEventTreeChunk_00034A02)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 4 <= rows->size; off += 4)
                tree.pTracks.push_back(read_u32(d, off));
        }
        if (const auto* rows = child(*container, kEventTreeChunk_00034A04)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 8 <= rows->size; off += 8) {
                std::array<std::uint8_t, 8> row{};
                std::memcpy(row.data(), d + off, 8);
                tree.pRewards.push_back(row);
            }
        }

        if (const auto* links = child(*container, kEventTreeChunk_00034A03)) {
            const std::uint8_t* d = buffer.data() + links->payload_offset();
            const std::size_t count = links->size / kEventLinkSize_002008B8;
            tree.links.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t off = i * kEventLinkSize_002008B8;
                EventLink_002008B8 link;
                link.field_0x00 = read_u16(d, off + 0);
                link.prerequisite_number_0x02 = read_u16(d, off + 2);
                link.goal_threshold_0x04 = read_u32(d, off + 4);
                tree.links.push_back(link);
            }
        }

        const auto* blob = child(*container, kEventTreeChunk_00034A05);
        if (blob == nullptr) continue;
        const std::uint8_t* d = buffer.data() + blob->payload_offset();
        const std::size_t count = blob->size / kEventRecordSize_001FFF60;
        tree.records.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t off = i * kEventRecordSize_001FFF60;
            EventInfo record;
            record.name_0x08 = read_fixed_string(d, off + 0x08, 0x20);
            record.number_0x28 = d[off + 0x28];
            record.goal_type_0x29 = d[off + 0x29];
            record.nGoals = d[off + 0x3d];
            record.index_0x42 = read_u16(d, off + 0x42);
            record.string_id_0xB0 = read_u32(d, off + 0xb0);
            record.string_id_0xB4 = read_u32(d, off + 0xb4);
            record.nCars = d[off + 0x3b];
            for (int k = 0; k < record.nCars; ++k) {
                if (0x44 + k * 2 + 2 > static_cast<int>(kEventRecordSize_001FFF60))
                    throw std::runtime_error("event car indices exceed record bounds");
                record.aCars.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x44 + k * 2)));
            }
            record.nTracks = d[off + 0x3c];
            record.nRewards = d[off + 0x3e];
            record.Field_0x3F = d[off + 0x3f];
            for (int k = 0; k < record.nTracks; ++k) {
                if (k >= 8) throw std::runtime_error("event track indices overlap requirement indices");
                record.aTracks.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x6c + k * 2)));
            }
            for (int k = 0; k < record.nRewards; ++k) {
                if (0x8c + k * 2 + 2 > static_cast<int>(kEventRecordSize_001FFF60))
                    throw std::runtime_error("event reward indices exceed record bounds");
                record.aRewards.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x8c + k * 2)));
            }
            for (int k = 0; k < static_cast<int>(record.nGoals); ++k) {
                // FUN_002008B8 indexes a s16 array at record+0x7C and multiplies by the 8-byte
                // link row size.
                const std::size_t slot = off + 0x7c + static_cast<std::size_t>(k) * 2;
                record.aGoals.push_back(
                    static_cast<std::int16_t>(read_u16(d, slot)));
            }
            tree.records.push_back(std::move(record));
        }

        // --- the view's node graph (FUN_001F96B8)
        tree.leaves.resize(tree.records.size());
        for (std::size_t i = 0; i < tree.records.size(); ++i) {
            tree.leaves[i].record_index = static_cast<int>(i);
        }
        for (std::size_t i = 0; i < tree.records.size(); ++i) {
            for (std::int16_t row : tree.records[i].aGoals) {
                if (row < 0 || static_cast<std::size_t>(row) >= tree.links.size()) continue;
                const int other = tree.FindLeafByEventNumber(
                    static_cast<std::uint8_t>(tree.links[static_cast<std::size_t>(row)]
                                                  .prerequisite_number_0x02));
                if (other < 0) continue;
                tree.leaves[i].incoming.push_back(other);
                tree.leaves[static_cast<std::size_t>(other)].outgoing.push_back(
                    static_cast<int>(i));
            }
        }

        // Depth: FUN_001F7FA8 follows the first incoming link back to a root and counts the hops.
        for (auto& leaf : tree.leaves) {
            int depth = 0;
            int walk = leaf.record_index;
            while (!tree.leaves[static_cast<std::size_t>(walk)].incoming.empty() &&
                   depth <= static_cast<int>(tree.leaves.size())) {
                walk = tree.leaves[static_cast<std::size_t>(walk)].incoming.front();
                ++depth;
            }
            leaf.depth_0x1C = depth;
            leaf.y = static_cast<float>(depth) * kRowSpacing_gp_0x41B4;
        }

        // X: walk the ASCII map counting non-space characters until the event number is reached,
        // then take 130.0f * (position % 20).
        if (const char* map = tree.layout_map()) {
            const std::size_t length = std::strlen(map);
            for (auto& leaf : tree.leaves) {
                int remaining = tree.records[static_cast<std::size_t>(leaf.record_index)].number_0x28;
                for (std::size_t pos = 0; pos < length; ++pos) {
                    if (map[pos] != ' ') --remaining;
                    if (remaining == 0) {
                        leaf.x = kColumnSpacing_gp_0x41B0 *
                                 static_cast<float>(pos % kLayoutColumns_001F96B8);
                        break;
                    }
                }
            }
        }

        // Groups: FUN_001F96B8 puts every leaf reachable through the link lists into one
        // `tEventGroup`, i.e. the connected components of the undirected graph.
        int group = 0;
        for (auto& leaf : tree.leaves) {
            if (leaf.group != -1) continue;
            std::vector<int> stack{leaf.record_index};
            while (!stack.empty()) {
                const int at = stack.back();
                stack.pop_back();
                auto& node = tree.leaves[static_cast<std::size_t>(at)];
                if (node.group != -1) continue;
                node.group = group;
                for (int next : node.incoming) stack.push_back(next);
                for (int next : node.outgoing) stack.push_back(next);
            }
            ++group;
        }

        trees.push_back(std::move(tree));
    }
    return trees;
}

EventProfileLayout_001FE988 build_profile_layout_001FE988(const EventTree_001FFC40& tree) {
    // FUN_001FE988 makes TWO passes over the records, split on whether an event's slice is
    // smaller than the alignment:
    //   pass 1 takes every event whose size is >= 4 and rounds the cursor up to 4 after it;
    //   pass 2 takes the 1-byte events and packs them with no rounding at all.
    // An event numbered 0x20 reuses the slice cached in the global DAT_002FC3D8 instead of taking
    // one of its own. That global is shared between the two trees in the original; this function
    // models it per tree, which is the only scope a single call can speak for.
    EventProfileLayout_001FE988 layout;
    layout.offsets.assign(tree.records.size(), -1);
    int used = 0;
    int shared = -1;

    auto assign = [&](std::size_t i, bool rounded) {
        const auto& record = tree.records[i];
        const int size = EventSaveSize_00200DE8(record.goal_type_0x29);
        if (record.number_0x28 == 0x20 && shared >= 0) {
            layout.offsets[i] = shared;
            return;
        }
        layout.offsets[i] = used;
        used = rounded ? ((used + size - 1 + layout.kAlignment) / layout.kAlignment) *
                             layout.kAlignment
                       : used + size;
        if (record.number_0x28 == 0x20) shared = layout.offsets[i];
    };

    for (std::size_t i = 0; i < tree.records.size(); ++i) {
        if (EventSaveSize_00200DE8(tree.records[i].goal_type_0x29) >= layout.kAlignment) {
            assign(i, true);
        }
    }
    for (std::size_t i = 0; i < tree.records.size(); ++i) {
        if (EventSaveSize_00200DE8(tree.records[i].goal_type_0x29) < layout.kAlignment) {
            assign(i, false);
        }
    }

    layout.used = used;
    if (used > static_cast<int>(layout.kBlockSize)) {
        hp2::not_implemented("0x001FE988",
                            "event profile slices overflow the 0x100-byte block, which the "
                            "original's two passes are sized to avoid");
    }
    return layout;
}

const std::uint32_t* EventTree_001FFC40::GetTrack(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.nTracks) return nullptr;
    const auto row = record.aTracks.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= pTracks.size())
        throw std::out_of_range("event track row outside table");
    return &pTracks[static_cast<std::size_t>(row)];
}
const std::uint32_t* EventTree_001FFC40::GetCar(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.nCars) return nullptr;
    const auto row = record.aCars.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= pCars.size())
        throw std::out_of_range("event car row outside table");
    return &pCars[static_cast<std::size_t>(row)];
}
const std::array<std::uint8_t, 8>* EventTree_001FFC40::GetReward(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.nRewards) return nullptr;
    const auto row = record.aRewards.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= pRewards.size())
        throw std::out_of_range("event reward row outside table");
    return &pRewards[static_cast<std::size_t>(row)];
}
int EventTree_001FFC40::Sub_002012D8(const hp2::memorycard::Profile_002FC050& profile,
                                  int record_index) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (record.goal_type_0x29 == 3)
        return GetRaceIndex(read_u32(port::profile_slice(*this, profile, record_index), 0));
    if (record.goal_type_0x29 == 4)
        return Sub_002006A0(port::profile_slice(*this, profile, record_index), record.Field_0x3F);
    return 0;
}
int GetRaceCount(const EventInfo& record) {
    if (record.goal_type_0x29 == 3) return record.Field_0x3F;
    if (record.goal_type_0x29 == 4) return record.nTracks;
    return 1;
}
std::string Sub_001A3B98(std::int32_t value, int locale, unsigned flags) {
    // E-0022-D07. Integer ticks are 1/4000 second; INT_MAX and zero format as zero.
    if (value == INT32_MIN)
        hp2::not_implemented("0x001A3B98", "INT_MIN timer negation behavior not modeled");
    int hours = 0, minutes = 0, seconds = 0, hundredths = 0;
    if (value != 0 && value != INT32_MAX) {
        if (value < 0) { value = -value; flags |= 1; }
        const int whole_seconds = value / 4000;
        hours = whole_seconds / 3600;
        minutes = (whole_seconds % 3600) / 60;
        seconds = whole_seconds % 60;
        hundredths = (value % 4000) / 40;
    }
    const bool quotes = locale == 1 || locale == 3;
    const bool colon = locale == 2 || locale == 4 || locale == 5;
    std::string out;
    auto digits = [&](std::size_t offset, int number, bool zeros) {
        for (int pos = 1; pos >= 0; --pos) {
            out[offset + static_cast<std::size_t>(pos)] =
                (!zeros && number % 10 == 0 && number <= 9) ? ' ' : char('0' + number % 10);
            number /= 10;
        }
    };
    int leading = 0;
    if (hours) {
        out = quotes ? "--h:--'--\"" : "--h:--:--";
        digits(0, hours, false); digits(4, minutes, true); digits(7, seconds, true);
        leading = hours;
    } else if (minutes) {
        out = quotes ? "--'--\"--" : colon ? "--:--:--" : "--:--.--";
        digits(0, minutes, false); digits(3, seconds, true); digits(6, hundredths, true);
        leading = minutes;
    } else {
        out = seconds ? (quotes ? "   --\"--" : colon ? "   --:--" : "   --.--")
                      : (quotes ? "    0\"--" : colon ? "    0:--" : "    0.--");
        if (seconds) digits(3, seconds, false);
        digits(6, hundredths, true);
    }
    if ((flags & 3) && leading > 9) out = " " + out.substr(0, out.size() - 1);
    if (flags & 1) out[0] = '-'; else if (flags & 2) out[0] = '+';
    return out;
}
std::uint32_t Sub_001F60E8(int status) {
    if (status < 0 || status > 4) throw std::out_of_range("event status outside original text table");
    return 0xf230u + static_cast<std::uint32_t>(status);
}
std::string Sub_001F60E8(int status, const std::string& format, int progress, int total,
                       const std::string& result) {
    (void)Sub_001F60E8(status);
    if (status <= 1) return format;
    if (status == 2) return port::format_operands(format, {std::to_string(progress), std::to_string(total)});
    return port::format_operands(format, {result});
}
std::string Sub_001F6220(const std::array<std::uint8_t, 8>* reward,
                       const std::string& localized_101D8,
                       const std::function<std::string(int)>& car_name,
                       const std::function<std::string(int, int)>& track_name) {
    if (!reward) return localized_101D8;
    if ((*reward)[0] == 0) {
        if (!car_name) hp2::not_implemented("0x00187EB8", "original car database name lookup missing");
        return car_name((*reward)[1]);
    }
    if ((*reward)[0] == 1) {
        if (!track_name) hp2::not_implemented("0x001DC5A8", "original track database name lookup missing");
        return track_name((*reward)[1], (*reward)[2]);
    }
    hp2::not_implemented("0x001F6220", "reward kind has no text-producing arm in original");
}
std::string Sub_001F5F88(const EventTree_001FFC40& tree, int record_index,
                       const std::string& source,
                       const std::function<std::string(std::int32_t)>& timer) {
    constexpr const char* tokens_0031B200[8] = {"$TARGET_TIME", "$NUM_LAPS", "$NUM_OPPONENTS",
        "$NUM_RACES", "$EVENTS_UNLOCKED", "$COMPLETION_TIME", "$EVENT_UNLOCKED", "$TRACK_NAME"};
    const auto& record = tree.records.at(static_cast<std::size_t>(record_index));
    std::string out;
    for (std::size_t i = 0; i < source.size() && out.size() < 512;) {
        int token = -1;
        if (source[i] == '$')
            for (int j = 0; j < 8; ++j)
                if (source.compare(i, std::strlen(tokens_0031B200[j]), tokens_0031B200[j]) == 0) {
                    token = j; break;
                }
        if (token < 0) { out += source[i++]; continue; }
        i += std::strlen(tokens_0031B200[token]);
        std::string substitution;
        switch (token) {
        case 0: {
            const auto& row = tree.links.at(record.index_0x42);
            if (row.field_0x00 == 2) {
                if (!timer) hp2::not_implemented("0x001A3B98", "original timer formatter missing");
                substitution = timer(static_cast<std::int32_t>(row.goal_threshold_0x04));
                const auto first = substitution.find_first_not_of(' ');
                substitution = first == std::string::npos ? "" : substitution.substr(first);
            } else substitution = "zz:zz";
            break;
        }
        case 1: {
            const auto* row = tree.GetTrack(record_index, 0);
            if (!row) hp2::not_implemented("0x001F5DD0", "NUM_LAPS requires first track row");
            substitution = std::to_string((*row >> 14) & 15); break;
        }
        case 2: substitution = std::to_string(record.Field_0x3F); break;
        case 3: substitution = std::to_string(GetRaceCount(record)); break;
        case 4: substitution = "events x,y,z are Unlocked!"; break;
        case 5: substitution = "ff:ggg"; break;
        case 6: substitution = "event x is unlocked"; break;
        case 7: substitution = "TRACK_NAME_HERE"; break;
        }
        out.append(substitution, 0, 512 - out.size());
    }
    return out;
}
std::string Sub_00200E78(const EventTree_001FFC40& tree,
                       const hp2::memorycard::Profile_002FC050& profile, int record_index,
                       int status, const std::function<std::string(std::uint32_t)>& localize,
                       const std::function<std::string(std::int32_t)>& timer) {
    if (status < 3) return "";
    const auto& record = tree.records.at(static_cast<std::size_t>(record_index));
    const auto* slice = port::profile_slice(tree, profile, record_index);
    int result = 0;
    switch (record.goal_type_0x29) {
    case 0: case 2: result = slice[0]; break;
    case 1: case 5: case 6:
        if (!timer) hp2::not_implemented("0x001A3B98", "original timer formatter missing");
        return timer(static_cast<std::int32_t>(read_u32(slice, 0)));
    case 3: result = slice[0] & 15; break; // 00200220 case 0, listing 00200248..250
    case 4: result = RacerRank(slice, 0); break; // 00200768 delegates with index 0
    default: hp2::not_implemented("0x00200E78", "goal type has no result-text arm in original");
    }
    // The table is nine words in the original: placing 0..8.
    if (result > 8) hp2::not_implemented("0x00200E78", "result index beyond nine recovered placing text IDs");
    if (!localize) hp2::not_implemented("0x0021E560", "result localization lookup missing");
    return localize(0xee48u + static_cast<std::uint32_t>(result));
}

void tCarLogoAnimation::Bind_001F8C10(FeScreen& screen, std::uint32_t now_ticks) {
    Field_0x00 = nullptr;
    for (auto& widget : screen.widgets)
        if (widget.name_hash == 0xc6398c48u) { Field_0x00 = &widget; break; }
    if (!Field_0x00) hp2::not_implemented("0x001F8C10", "car logo widget C6398C48 missing");
    Field_0x04 = now_ticks;
    Field_0x08.clear();
    Field_0x2C = Field_0x30 = Field_0x38 = Field_0x3C = 0;
    Field_0x34 = Field_0x00->a;
}
void tCarLogoAnimation::Sub_001F8D20(bool randomize, std::uint32_t& rng) {
    if (!Field_0x00) hp2::not_implemented("0x001F8D20", "car logo animator not bound");
    if (Field_0x2C < static_cast<int>(Field_0x08.size()))
        Field_0x00->texture_hash = Field_0x08[static_cast<std::size_t>(Field_0x2C)];
    if (randomize) {
        Field_0x38 = (Field_0x38 + static_cast<int>(bRandom(3, rng)) + 1) % 4;
        Field_0x3C = (Field_0x3C + static_cast<int>(bRandom(2, rng))) & 1;
    }
    constexpr float x_0031B5B0[4] = {70, 150, 210, 260};
    constexpr float y_00331760[2] = {140, 280};
    Field_0x00->x = x_0031B5B0[Field_0x38];
    Field_0x00->y = y_00331760[Field_0x3C];
    Field_0x00->dirty_0x2b = 1;
}
void tCarLogoAnimation::Sub_001F8E18(const std::vector<std::uint32_t>& logos,
                               std::uint32_t now_ticks) {
    Field_0x08.assign(logos.begin(), logos.begin() + static_cast<std::ptrdiff_t>(std::min<std::size_t>(8, logos.size())));
    Field_0x2C = Field_0x38 = Field_0x3C = 0;
    Field_0x30 = 1;
    std::uint32_t unused = 0;
    Sub_001F8D20(false, unused);
    Field_0x04 = now_ticks - 22000u; // 00136DD0(5.5) at 4000 ticks/second
    Field_0x00->visible = 1; // 001F8CF0
}
void tCarLogoAnimation::Sub_001F8FB0(std::uint32_t now_ticks, std::uint32_t& rng) {
    if (!Field_0x00) hp2::not_implemented("0x001F8FB0", "car logo animator not bound");
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x04)) * 0.00025f;
    if (elapsed > 5.0f) {
        if (elapsed < 5.5f) {
            const float fraction = (elapsed - 5.0f) / 0.5f; // 001F8F68
            Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x34 * (1.0f - fraction)));
            Field_0x30 = 0;
        } else if (elapsed < 6.0f) {
            if (Field_0x30 == 0) {
                if (++Field_0x2C >= static_cast<int>(Field_0x08.size())) Field_0x2C = 0;
                Sub_001F8D20(true, rng);
                Field_0x30 = 1;
            }
            const float fraction = (elapsed - 5.5f) / 0.5f;
            Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x34 * fraction));
        } else Field_0x04 = now_ticks;
        Field_0x00->dirty_0x2b = 1;
    }
}
void tCarLogoAnimation::Sub_001F8CF0() { if (Field_0x00) Field_0x00->visible = 1; }
void tCarLogoAnimation::Sub_001F8D08() { if (Field_0x00) Field_0x00->visible = 0; }
void tTrackLogoAnimation::Bind_001F90F0(FeScreen& screen, std::uint32_t now_ticks) {
    Field_0x00 = nullptr;
    for (auto& widget : screen.widgets)
        if (widget.name_hash == 0xf60114e4u) { Field_0x00 = &widget; break; }
    if (!Field_0x00) hp2::not_implemented("0x001F90F0", "track logo widget F60114E4 missing");
    Field_0x40 = Field_0x00->x;
    Field_0x48 = Field_0x00->a;
    Field_0x44 = 0; // constructor's original clock field is zero, not current time
    Field_0x4C = false;
    Field_0x38 = 0;
    Field_0x04.clear();
    Field_0x00->visible = 1; // 001F91D0
    (void)now_ticks;
}
void tTrackLogoAnimation::Sub_001F91F0(const std::vector<std::uint32_t>& logos,
                               std::uint32_t now_ticks) {
    if (!Field_0x00) hp2::not_implemented("0x001F91F0", "track logo animator not bound");
    Field_0x04.assign(logos.begin(), logos.begin() + static_cast<std::ptrdiff_t>(std::min<std::size_t>(12, logos.size())));
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x44)) * 0.00025f;
    if (elapsed < 0.35f) Field_0x00->x = 640.0f;
    Field_0x38 = 0;
    Field_0x44 = now_ticks;
    Field_0x00->visible = 1;
    Field_0x00->dirty_0x2b = 1;
}
void tTrackLogoAnimation::Sub_001F9318(std::uint32_t now_ticks) {
    if (!Field_0x00) hp2::not_implemented("0x001F9318", "track logo animator not bound");
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x44)) * 0.00025f;
    constexpr float fade_end = 0.35f; // 0032EC30
    constexpr float slide_end = 0.35f + 0.27f; // gp9330 + gpBE78
    if (elapsed < fade_end) {
        const float fraction = elapsed < 0.0f ? 0.0f : elapsed / fade_end;
        Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x48 * (1.0f - fraction)));
        Field_0x4C = false;
    } else if (elapsed <= fade_end || elapsed > slide_end) {
        if (elapsed > slide_end) Field_0x00->x = Field_0x40;
    } else {
        if (!Field_0x4C) {
            // Original zeroed twelve-word list yields texture zero for an empty list.
            Field_0x00->texture_hash = Field_0x04.empty() ? 0 : Field_0x04[static_cast<std::size_t>(Field_0x38)];
            Field_0x4C = true;
        }
        const float fraction = (elapsed - fade_end) / (slide_end - fade_end);
        Field_0x00->x = static_cast<float>(static_cast<int>(fraction * (Field_0x40 - 640.0f) + 640.0f));
        Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x48));
    }
    Field_0x00->dirty_0x2b = 1;
}
void tTrackLogoAnimation::Show() { if (Field_0x00) Field_0x00->visible = 1; }
void tTrackLogoAnimation::Sub_001F91E0() { if (Field_0x00) Field_0x00->visible = 0; }

} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001F6FA0; E-0018-02; E-0028-C03; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f6fa0.md.
void tEventLeaf::Sub_001F6FA0() {
    hp2::not_implemented("0x001F6FA0", "tEventLeaf::Sub_001F6FA0 (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::recovered {
// ELF 0x00200878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200878.md.
void EventInfo::GetCar() {
    hp2::not_implemented("0x00200878", "EventInfo::GetCar (signature and full body unresolved)");
}

// ELF 0x002008B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002008b8.md.
void EventInfo::GetGoal() {
    hp2::not_implemented("0x002008B8", "EventInfo::GetGoal (signature and full body unresolved)");
}

// ELF 0x002008F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002008f8.md.
void EventInfo::GetTrack() {
    hp2::not_implemented("0x002008F8", "EventInfo::GetTrack (signature and full body unresolved)");
}

// ELF 0x00200938; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200938.md.
void EventInfo::GetReward() {
    hp2::not_implemented("0x00200938", "EventInfo::GetReward (signature and full body unresolved)");
}

// ELF 0x00200978; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200978.md.
void EventInfo::GetOpponent() {
    hp2::not_implemented("0x00200978", "EventInfo::GetOpponent (signature and full body unresolved)");
}

// ELF 0x00200D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d48.md.
void EventInfo::GetRankState() {
    hp2::not_implemented("0x00200D48", "EventInfo::GetRankState (signature and full body unresolved)");
}

// ELF 0x00200D70; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d70.md.
void EventInfo::GetTournamentState() {
    hp2::not_implemented("0x00200D70", "EventInfo::GetTournamentState (signature and full body unresolved)");
}

// ELF 0x00200D90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d90.md.
void EventInfo::GetKoState() {
    hp2::not_implemented("0x00200D90", "EventInfo::GetKoState (signature and full body unresolved)");
}

// ELF 0x00200DB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200db0.md.
void EventInfo::GetTimeState() {
    hp2::not_implemented("0x00200DB0", "EventInfo::GetTimeState (signature and full body unresolved)");
}

// ELF 0x00201350; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00201350.md.
void EventInfo::GetRaceCount() {
    hp2::not_implemented("0x00201350", "EventInfo::GetRaceCount (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x002001A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002001a8.md.
void EventKoState::GetRaceIndex() {
    hp2::not_implemented("0x002001A8", "EventKoState::GetRaceIndex (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001FC580; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fc580.md.
void EventLeaf::SetScrollNotch() {
    hp2::not_implemented("0x001FC580", "EventLeaf::SetScrollNotch (signature and full body unresolved)");
}

// ELF 0x001FCC88; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fcc88.md.
void EventLeaf::FillVerts() {
    hp2::not_implemented("0x001FCC88", "EventLeaf::FillVerts (signature and full body unresolved)");
}

// ELF 0x001FD050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fd050.md.
void EventLeaf::GetNeighbour() {
    hp2::not_implemented("0x001FD050", "EventLeaf::GetNeighbour (signature and full body unresolved)");
}

// ELF 0x001FD0B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fd0b0.md.
void EventLeaf::GetReward() {
    hp2::not_implemented("0x001FD0B0", "EventLeaf::GetReward (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00200018; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200018.md.
void EventRankState::GetStatus() {
    hp2::not_implemented("0x00200018", "EventRankState::GetStatus (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00200068; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200068.md.
void EventTimeState::GetStatus() {
    hp2::not_implemented("0x00200068", "EventTimeState::GetStatus (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00200638; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200638.md.
void EventTournamentState::Reset() {
    hp2::not_implemented("0x00200638", "EventTournamentState::Reset (signature and full body unresolved)");
}

// ELF 0x00200710; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200710.md.
void EventTournamentState::RacerRank() {
    hp2::not_implemented("0x00200710", "EventTournamentState::RacerRank (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001FDFE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fdfe8.md.
void EventTree::GetEventNumber() {
    hp2::not_implemented("0x001FDFE8", "EventTree::GetEventNumber (signature and full body unresolved)");
}

// ELF 0x001FFF60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fff60.md.
void EventTree::SetEvents() {
    hp2::not_implemented("0x001FFF60", "EventTree::SetEvents (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00201558; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201558.md.
void RaceEvent::RacerIndexFromDriverNumber() {
    hp2::not_implemented("0x00201558", "RaceEvent::RacerIndexFromDriverNumber (signature and full body unresolved)");
}

// ELF 0x00201578; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201578.md.
void RaceEvent::DriverNumberFromRacerIndex() {
    hp2::not_implemented("0x00201578", "RaceEvent::DriverNumberFromRacerIndex (signature and full body unresolved)");
}

// ELF 0x00201858; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201858.md.
void RaceEvent::GetRankState() {
    hp2::not_implemented("0x00201858", "RaceEvent::GetRankState (signature and full body unresolved)");
}

// ELF 0x00201880; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201880.md.
void RaceEvent::GetTournamentState() {
    hp2::not_implemented("0x00201880", "RaceEvent::GetTournamentState (signature and full body unresolved)");
}

// ELF 0x002018A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002018a0.md.
void RaceEvent::GetKoState() {
    hp2::not_implemented("0x002018A0", "RaceEvent::GetKoState (signature and full body unresolved)");
}

// ELF 0x002018C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002018c0.md.
void RaceEvent::GetTimeState() {
    hp2::not_implemented("0x002018C0", "RaceEvent::GetTimeState (signature and full body unresolved)");
}

// ELF 0x00202088; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00202088.md.
void RaceEvent::GetRaceCount() {
    hp2::not_implemented("0x00202088", "RaceEvent::GetRaceCount (signature and full body unresolved)");
}
} // namespace hp2::recovered


#include "game_core/indep/inc/EventTree.hpp"
#include <cstring>
#include <stdexcept>

namespace port::race::event_reader {
std::uint32_t word(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
std::int16_t half(const std::uint8_t* p){return static_cast<std::int16_t>(std::uint16_t(p[0])|(std::uint16_t(p[1])<<8));}
std::string text(const std::uint8_t* p,std::size_t size){std::size_t n=0;while(n<size && p[n])++n;return {reinterpret_cast<const char*>(p),n};}
template<class T> const T* table(const std::vector<T>& values,int index,const char* address){
    if(index<0 || static_cast<std::size_t>(index)>=values.size())hp2::not_implemented(address,"Debug event table index outside original loaded data");
    return &values[static_cast<std::size_t>(index)];
}
void collect(const std::vector<hp2::assets::Chunk>& chunks,std::vector<const hp2::assets::Chunk*>& trees){
    for(const auto& c:chunks){if(c.tag==0x80034a00)trees.push_back(&c);else if(c.is_container())collect(c.children,trees);}
}
}
namespace port::race {
// Debug001FB138/001FB458 and EventInfo type, E-0097-D01. Host parser only;
// original pointer relocation/global list insertion/progress binding are excluded.
std::vector<hp2::debug::EventTree> parse_debug_event_tables(const std::vector<std::uint8_t>& bytes){
    using namespace event_reader;
    std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
    if(consumed!=bytes.size())throw std::runtime_error("Incomplete event bundle chunk consumption");
    std::vector<const hp2::assets::Chunk*> found;collect(chunks,found);
    std::vector<hp2::debug::EventTree> result;
    for(const auto* container:found){
        hp2::debug::EventTree tree;
        for(const auto& c:container->children){
            const auto* data=bytes.data()+c.payload_offset();
            if(c.tag==1){tree.sName=text(data,c.size);continue;}
            if(c.tag==0x34a01 || c.tag==0x34a02 || c.tag==0x34a06){
                if(c.size%4)throw std::runtime_error("Misaligned Debug event four-byte table");
                auto& target=c.tag==0x34a01?tree.pCars:c.tag==0x34a02?tree.pTracks:tree.pOpponents;
                for(std::size_t i=0;i<c.size;i+=4)target.push_back(word(data+i));
            }else if(c.tag==0x34a03){
                if(c.size%8)throw std::runtime_error("Misaligned Debug event goal table");
                for(std::size_t i=0;i<c.size;i+=8)tree.pGoals.push_back({static_cast<std::uint16_t>(half(data+i)),static_cast<std::uint16_t>(half(data+i+2)),word(data+i+4)});
            }else if(c.tag==0x34a04){
                if(c.size%8)throw std::runtime_error("Misaligned Debug event reward table");
                for(std::size_t i=0;i<c.size;i+=8){hp2::debug::EventReward reward;std::copy_n(data+i,8,reward.Field_0x00.begin());tree.pRewards.push_back(reward);}
            }else if(c.tag==0x34a05){
                if(c.size%0xbc)throw std::runtime_error("Misaligned Debug EventInfo table");
                for(std::size_t i=0;i<c.size;i+=0xbc){
                    const auto* r=data+i;hp2::debug::EventInfo e;
                    e.sName=text(r+8,32);std::copy_n(reinterpret_cast<const char*>(r+8),32,e.port_name_bytes.begin());
                    e.nNumber=static_cast<std::int8_t>(r[0x28]);e.nType=static_cast<std::int8_t>(r[0x29]);
                    e.nClass=static_cast<std::int8_t>(r[0x2a]);e.nOpponentSelect=static_cast<std::int8_t>(r[0x2b]);e.nCops=static_cast<std::int8_t>(r[0x2c]);
                    e.eCopStrength=static_cast<std::int32_t>(word(r+0x30));
                    e.bRoadBlocks=static_cast<std::int8_t>(r[0x34]);e.bSpikeStrips=static_cast<std::int8_t>(r[0x35]);e.bBarricadeSmokeables=static_cast<std::int8_t>(r[0x36]);
                    e.bTickets=static_cast<std::int8_t>(r[0x37]);e.bHelicopter=static_cast<std::int8_t>(r[0x38]);
                    e.nNumBeACopRoadBlocks=static_cast<std::int8_t>(r[0x39]);e.nNumBeACopChoppers=static_cast<std::int8_t>(r[0x3a]);
                    e.nCars=static_cast<std::int8_t>(r[0x3b]);e.nTracks=static_cast<std::int8_t>(r[0x3c]);e.nGoals=static_cast<std::int8_t>(r[0x3d]);
                    e.nRewards=static_cast<std::int8_t>(r[0x3e]);e.nOpponents=static_cast<std::int8_t>(r[0x3f]);e.bZoneEnabled=static_cast<std::int8_t>(r[0x40]);e.nObjective=half(r+0x42);
                    for(std::size_t j=0;j<20;++j)e.aCars[j]=half(r+0x44+j*2);
                    for(std::size_t j=0;j<8;++j){e.aTracks[j]=half(r+0x6c+j*2);e.aGoals[j]=half(r+0x7c+j*2);e.aRewards[j]=half(r+0x8c+j*2);}
                    for(std::size_t j=0;j<7;++j)e.aOpponents[j]=half(r+0x9c+j*2);
                    const auto boost=word(r+0xac);std::memcpy(&e.fOpponentBoostScale,&boost,4);
                    e.mTitleString=static_cast<std::int32_t>(word(r+0xb0));e.mDescriptionString=static_cast<std::int32_t>(word(r+0xb4));e.mDamage=static_cast<std::int32_t>(word(r+0xb8));
                    if(e.nCars<0 || e.nCars>20 || e.nTracks<0 || e.nTracks>8 || e.nGoals<0 || e.nGoals>8 || e.nRewards<0 || e.nRewards>8 || e.nOpponents<0 || e.nOpponents>7)
                        throw std::runtime_error("Debug EventInfo count exceeds original inline array");
                    tree.pEvents.push_back(e);
                }
            }
        }
        // Debug001F9450 compares case-insensitively; E3/global registration and
        // pointer ownership remain outside this non-E3 asset adapter.
        auto folded=tree.sName;
        for(auto& c:folded)if(c>='A' && c<='Z')c=static_cast<char>(c+('a'-'A'));
        if(folded=="championship")tree.nType=0;
        else if(folded=="hot pursuit")tree.nType=1;
        else hp2::not_implemented("0x001F9450","unknown original Debug event tree name");
        result.push_back(std::move(tree));
    }
    return result;
}
}
namespace hp2::debug {
// Exact Debug getter entries; E-0097-D02. Caller index rejection is original.
// Debug0x001FBBC8, E-0097-D02; explicit host context, original valid-domain body.
const std::uint32_t* EventInfo::GetCar(const EventTree& tree,int n)const {
    if(n<0 || n>=nCars)return nullptr;
    return ::port::race::event_reader::table(tree.pCars,aCars.at(n),"0x001FBBC8");
}
// Debug0x001FBC08, E-0097-D02; explicit host context, original valid-domain body.
const EventGoal* EventInfo::GetGoal(const EventTree& tree,int n)const {
    if(n<0 || n>=nGoals)return nullptr;
    return ::port::race::event_reader::table(tree.pGoals,aGoals.at(n),"0x001FBC08");
}
// Debug0x001FBC48, E-0097-D02; explicit host context, original valid-domain body.
const std::uint32_t* EventInfo::GetTrack(const EventTree& tree,int n)const {
    if(n<0 || n>=nTracks)return nullptr;
    return ::port::race::event_reader::table(tree.pTracks,aTracks.at(n),"0x001FBC48");
}
// Debug0x001FBC88, E-0097-D02; explicit host context, original valid-domain body.
const EventReward* EventInfo::GetReward(const EventTree& tree,int n)const {
    if(n<0 || n>=nRewards)return nullptr;
    return ::port::race::event_reader::table(tree.pRewards,aRewards.at(n),"0x001FBC88");
}
// Debug0x001FBCC8, E-0097-D02; explicit host context, original valid-domain body.
const std::uint32_t* EventInfo::GetOpponent(const EventTree& tree,int n)const {
    if(n<0 || n>=nOpponents)return nullptr;
    return ::port::race::event_reader::table(tree.pOpponents,aOpponents.at(n),"0x001FBCC8");
}
// Debug0x001FBD08, E-0097-D02; explicit host context, original valid-domain body.
const EventGoal* EventInfo::GetObjective(const EventTree& tree)const {
    return ::port::race::event_reader::table(tree.pGoals,nObjective,"0x001FBD08");
}
// Debug001FC028 E-0097-D02. Preserve the original default arm.
int EventInfo::GetStateSize()const {
    switch(nType){case 0:case 2:return 1;case 1:case 5:case 6:return 4;case 3:return 12;case 4:return 16;default:return 1;}
}
// Debug001FA178 E-0097-D03. Constants00325D10/14 both bits3C23D70A.
void EventTrack::SetRace(std::uint32_t packed,::port::driving::init::ByteView race,
    const std::function<std::optional<int>(int)>& GetTrackInfo){
    race.require(0,0x310);
    if(!GetTrackInfo)hp2::not_implemented("0x001FA178","original GetTrackInfo service missing");
    race.word(0,packed&0xff);race.word(0x2c,(packed>>14)&15);
    race.word(4,(packed>>10)&3);race.word(8,(packed>>12)&3);race.word(12,(packed>>9)&1);
    const std::uint32_t bits=0x3c23d70a;float factor;std::memcpy(&factor,&bits,4);
    race.scalar(0x10,static_cast<float>((packed>>18)&127)*factor);
    race.scalar(0x14,static_cast<float>(packed>>25)*factor);
    const auto info=GetTrackInfo(static_cast<int>(packed&255));
    if(info)race.word(0x28,static_cast<std::uint32_t>(*info));
}
}

#include "game_core/port/race/launch.hpp"
namespace hp2::debug {
// Debug001FC648, E-0098-RP04. Original packed-time/rank/reason reset.
void EventResult::Reset(){nRank=0;tTime.PackedTime=0;nReason=0;}
// Debug001FC678, E-0098-RP04. Original resets all eight result slots.
void RaceEvent::ResetResults(){for(auto& result:aResults)result.Reset();}
// Debug001FB540, E-0098-RP04. Original four-byte persistent state reset.
void EventTimeState::Reset(::port::driving::init::ByteView state){state.word(0,0);}
// Debug001FB560, E-0098-RP04. Packed time and objective comparisons are signed32.
eEventStatus EventTimeState::GetStatus(::port::driving::init::ByteView state,const EventGoal& goal){
    const auto time=static_cast<std::int32_t>(state.word(0));
    if(time==0 || time==0x7fffffff)return eEVENT_UNTRIED;
    return time<=static_cast<std::int32_t>(goal.nData)?eEVENT_PASSED:eEVENT_FAILED;
}
// Debug001FD708, E-0098-RP04/E-0101-PL01. Original resets only progress;
// selected car/skin/dash/color seed survive KO/tournament reset.
void RaceEvent::ResetState(){
    if(!pState.data)return;
    if(nType==0 || nType==2){pState.require(0,1);pState.data[0]=0;return;} //001FB4F8.
    if(nType==1 || nType==5 || nType==6){EventTimeState::Reset(pState);return;}
    if(nType==3){pState.word(0,0);return;} // Original EventKoState.Reset001FB5A0.
    if(nType==4){pState.require(0,8);for(int i=7;i>=0;--i)pState.data[i]=0;return;} //001FB988.
}
// Debug001FD118, E-0098-RP04. Non-multitrack types always return zero.
int RaceEvent::GetRaceIndex() const{
    if(nType!=3 && nType!=4)return 0;
    if(!pState.data)return 0;
    if(nType==3){const auto ranks=pState.word(0);int count=0;for(unsigned i=0;i<8;++i)count+=((ranks>>(4*i))&15)!=0;return count;} //001FB698.
    if(!port_points_at_00328830)
        hp2::not_implemented("0x001FB9F0","decoded tournament requires the original points-table binding");
    pState.require(0,8);int possible=0,earned=0;
    for(int i=0;i<=nOpponents;++i)possible+=port_points_at_00328830(unsigned(i));
    for(unsigned i=0;i<8;++i)earned+=pState.data[i];
    if(!possible)hp2::not_implemented("0x001FB9F0","original tournament points division trap");
    return earned/possible;
}
// Debug001FC1E8, E-0098-RP04. Ordinary event numbers are an original immediate false.
bool EventInfo::WillSuperBonusEventBeUnlockedIfThisEventIsWon() const{
    if(nNumber!=31)return false;
    hp2::not_implemented("0x001FC1E8","original event31 cross-tree unlock prediction not translated");
}
// Debug001FA378, E-0098-RP04/E-0110-E02. First HP delivery/Championship branches with explicit original
// state/localization bindings. Unknown branches fail BEFORE touching prior settings.
void EventTree::SetRace(::port::driving::init::ByteView race,const EventInfo& event,RaceEvent& active,
    ::port::driving::init::ByteView state,const ::port::race::EventLaunchServices& services){
    race.require(0,0x310);
    const bool delivery=nType==1&&event.nNumber==1&&event.nType==5&&event.nOpponents==0;
    const bool championship=nType==0&&event.nNumber==1&&event.nType==0&&event.nOpponents>=0&&event.nOpponents<=7;
    if(!delivery&&!championship)
        hp2::not_implemented("0x001FA378","event binding outside reviewed first Championship/delivery branches");
    if(!services.translated_key || !services.translated_id || !services.format_description)
        hp2::not_implemented("0x001FA378","original reward/objective localization bindings missing");
    const int size=event.GetStateSize();state.require(0,size);
    pCurrentEvent=&event;services.fred.nCurrentEvent=event.nNumber;
    active.sName=event.port_name_bytes;
    std::copy_n(state.data,size,active.aOldState.begin());active.pTree=this;active.pState=state;
    active.nNumber=event.nNumber;active.nType=event.nType;active.nClass=event.nClass;
    active.nCars=event.nCars;active.nTracks=event.nTracks;active.nRewards=event.nRewards;
    active.nInitialPlayers=1;active.nOpponents=event.nOpponents;
    const auto* objective=event.GetObjective(*this);
    if(!objective)hp2::not_implemented("0x001FA378","original event objective record missing");
    active.Objective=*objective;
    for(int i=0;i<event.nCars;++i)active.aCars.at(i)=*event.GetCar(*this,i);
    for(int i=0;i<event.nTracks;++i)active.aTracks.at(i)=*event.GetTrack(*this,i);
    for(int i=0;i<event.nRewards;++i)active.aRewards.at(i)=*event.GetReward(*this,i);
    for(int i=0;i<event.nOpponents;++i)active.aOpponents.at(i)=*event.GetOpponent(*this,i);
    std::string reward;
    if(event.nRewards<1)reward=services.translated_key("No reward");
    else{
        const auto& first=active.aRewards[0].Field_0x00;
        if(first[0]==0){
            if(!services.car_full_name)hp2::not_implemented("0x001FA378","original reward car GetFullName binding missing");
            reward=services.translated_key(services.car_full_name(first[1]));
        }else if(first[0]==1){
            if(!services.track_language_id)hp2::not_implemented("0x001FA378","original reward track language binding missing");
            reward=services.translated_id(services.track_language_id(first[1],first[2]));
        }else reward=services.translated_key("Unhandled type");
    }
    if(reward.size()>=active.sRewardName.size())hp2::not_implemented("0x001FA378","original reward string outside bounded host buffer domain");
    std::copy(reward.begin(),reward.end(),active.sRewardName.begin());active.sRewardName[reward.size()]=0;
    const auto description=services.translated_id(event.mDescriptionString);
    const auto formatted=services.format_description(*this,event,description);
    if(formatted.size()>=512)hp2::not_implemented("0x001FA378","original description outside bounded host buffer domain");
    std::copy(formatted.begin(),formatted.end(),race.data+0x54);race.data[0x54+formatted.size()]=0;
    race.word(0x50,1);active.ResetResults();
    race.word(0x27c,2);race.word(0x274,0);race.word(0x20,1);
    if(delivery){race.word(0x28,1);race.word(0x24,active.Objective.nData);}
    race.word(0x30,1);race.word(0x34,static_cast<std::uint32_t>(event.nOpponents));
    race.word(0x25c,nType==1?6:0);race.word(0x38,static_cast<std::uint32_t>(event.nOpponentSelect));
    race.word(0x288,static_cast<std::uint32_t>(event.eCopStrength));
    race.word(0x260,static_cast<std::uint32_t>(event.bTickets));race.word(0x264,static_cast<std::uint32_t>(event.bRoadBlocks));
    race.word(0x268,static_cast<std::uint32_t>(event.bBarricadeSmokeables));race.word(0x26c,static_cast<std::uint32_t>(event.bSpikeStrips));
    race.word(0x270,static_cast<std::uint32_t>(event.bHelicopter));race.word(0x280,static_cast<std::uint32_t>(event.nNumBeACopRoadBlocks));
    race.word(0x284,static_cast<std::uint32_t>(event.nNumBeACopChoppers));race.word(0x4c,0);race.word(0x40,event.mDamage!=0);
    race.data[0x3c]=static_cast<std::uint8_t>(event.nOpponents+1);
    race.scalar(0x2fc,event.fOpponentBoostScale);race.scalar(0x300,event.fOpponentBoostScale);
}
}

#include "game_core/port/frontend/event_entry.hpp"

namespace hp2::debug {
// Debug001F2BD8, E-0098-E01, reverse/task0098/production-entry.md.
// Host services bind current leaf/unlock map and original global storage.
void tTreeDetails::StartEvent(ScreenCallbackData& data,const ::port::selection::EventEntryServices& s){
    if(!pTree || !LastKnownEvent || !s.set_race || !s.build_unlock_list)
        hp2::not_implemented("0x001F2BD8","event entry requires tree/event/race/unlock bindings");
    s.race.require(0x254,4);
    s.challenge_race_mode=1;
    s.race.word(0x254,1); // RACE_MODE_NFS_EXTREME, original001F2C20.
    s.fred.nCurrentEvent=LastKnownEvent->nNumber;
    s.set_race(*pTree,*LastKnownEvent);
    s.build_unlock_list();
    // Original IsMultiTrack001FC5D8: only event types3 and4 (E-0110-F01).
    if(LastKnownEvent->nType==3 || LastKnownEvent->nType==4){
        if(!s.get_status)hp2::not_implemented("0x001FC2A8","event status binding missing");
        if(s.get_status(*LastKnownEvent)==eEVENT_INPROGRESS){
            if(data.Port==0){ // JOYSTICK_PORT1; only this continuation branch gates Port.
                if(!s.push_screen)hp2::not_implemented("0x001F1E28","event continuation history binding missing");
                s.push_screen("View Event Tree");
                if(!s.continue_event)hp2::not_implemented("0x001828E8","multitrack continuation binding missing");
                s.continue_event();
            }
            return;
        }
    }
    s.fred.GameType=pTree->nType==0?8:9;
    s.fred.MakeEventCarsAvailable(*pTree,*LastKnownEvent,s.cars,s.language);
    if(!s.change_screen)hp2::not_implemented("0x0014C978","production Select Car transition binding missing");
    s.change_screen("Select Car",true);
}
// Debug001F2D20, E-0098-E01. START13 is NOT an alias for SELECT1 here.
void tTreeDetails::HandleMessages(ScreenMessage message,ScreenCallbackData& data,
    const ::port::selection::EventEntryServices& s){
    const int msg=static_cast<int>(message);
    if(msg>=10)return;
    if(msg>=6){data.Noise=0;return;}
    if(message!=MENUITEM_SELECT)return;
    if(!LastKnownEvent || !s.get_status)
        hp2::not_implemented("0x001FC2A8","event selection status binding missing");
    if(static_cast<int>(s.get_status(*LastKnownEvent))>0){
        data.Noise=0x16;StartEvent(data,s);
    }else data.Noise=0x17;
}
}

namespace hp2::debug {
// Debug001FC078; E-0099-W56. Original event-state byte sizes by nType.
int RaceEvent::GetStateSize(const ::port::driving::Memory& m,std::uint32_t address){switch(m.signed_half(address+0x6a)){case 0:case 2:return 1;case 1:case 5:case 6:return 4;case 3:return 12;case 4:return 16;default:return 1;}}
// Debug001FCB90. Requires the actual native event-state owner to publish pState;
// a decoded frontend/profile structure is not an original raw state allocation.
void RaceEvent::SavePreRaceState(::port::driving::Memory& m,std::uint32_t address){const int size=GetStateSize(m,address);const auto source=m.word(address+4);if(!source)hp2::not_implemented("0x001FCB90","actual original RaceEvent state owner is not bound");m.copy(address+0x18,source,static_cast<unsigned>(size));}
}

namespace hp2::debug {
// E-0099-W70, original eventtree.cpp. Raw Memory consumer, not decoded profile
// state. RecordResults does not consume pTree/pState or the pRace argument.
int RaceEvent::RacerIndexFromDriverNumber(const ::port::driving::Memory& m,std::uint32_t a,int number){return number>99?m.signed_half(a+0x76)+number-100:number;}
void RaceEvent::RecordResults(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race,std::uint32_t racing,int rank){
 (void)race;const int index=RacerIndexFromDriverNumber(m,a,m.signed_half(m.word(racing)+0xa));
 if(index<0||index>=8)hp2::not_implemented("0x001FC6F8","original raw result index lies outside eight entries; source event/roster producer is required");
 const auto reason=m.signed_half(racing+0x12);std::uint32_t timer=racing+0xb4;
 if(m.signed_word(0x311f98)==2)timer=RacingCar(m,racing).GetBestLapTime();
 const auto time=m.word(timer),result=a+0x150+12*static_cast<unsigned>(index);
 m.word(result,static_cast<std::uint32_t>(rank));m.word(result+4,time);m.word(result+8,static_cast<std::uint32_t>(static_cast<std::int32_t>(reason)));
}
}

namespace hp2::debug {
// Debug001FB958/001FBB98, E-0099-W86. Raw source selected-car fields;
// low-byte/halfword truncation is deliberate, and other progress/seed stays.
void EventKoState::SetSelectedCar(::port::driving::Memory& m,std::uint32_t a,int car,int skin,int dash){m.byte(a+7,std::uint8_t(dash));m.half(a+4,std::uint16_t(car));m.byte(a+6,std::uint8_t(skin));}
void EventTournamentState::SetSelectedCar(::port::driving::Memory& m,std::uint32_t a,int car,int skin,int dash){m.byte(a+11,std::uint8_t(dash));m.half(a+8,std::uint16_t(car));m.byte(a+10,std::uint8_t(skin));}
// Debug001FC5E8, E-0099-W86. Only source KO/tournament types write fields.
void EventInfo::SetSelectedCar(::port::driving::Memory& m,std::uint32_t a,int car,int skin,int dash){const auto type=m.signed_byte(a+0x29);if(type==3)EventKoState::SetSelectedCar(m,m.word(a+4),car,skin,dash);else if(type==4)EventTournamentState::SetSelectedCar(m,m.word(a+4),car,skin,dash);}
}

// TASK-0110 E-0110-F01. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Raw original state consumers; records/queries: reverse/task0110/finish.md.
namespace hp2::debug {
// Debug001FB7E8; E-0110-F01. The jump table writes one four-bit rank in the first word.
void EventKoState::SetRank(::port::driving::Memory& m,std::uint32_t a,int index,int rank){
 if(static_cast<unsigned>(index)>=8)return;
 const auto shift=4*static_cast<unsigned>(index);
 m.word(a,(m.word(a)&~(15u<<shift))|((static_cast<std::uint32_t>(rank)&15u)<<shift));
}
// Debug001FC5D8; E-0110-F01. LHU followed by ADDIU and unsigned SLTIU.
bool RaceEvent::IsMultiTrack(const ::port::driving::Memory& m,std::uint32_t a){return static_cast<std::uint32_t>(m.signed_half(a+0x6a)-3)<2;}
// Debug001FC658; E-0110-F01. The event's own state is not the abortion authority.
bool RaceEvent::RaceWasAborted(const ::port::driving::Memory& m,std::uint32_t){return FrontendDatabase::RaceWasAborted(m,0x2f3890);}
// Debug001FC6D8; E-0110-F01. Human numbers precede the AI range starting at100.
int RaceEvent::DriverNumberFromRacerIndex(const ::port::driving::Memory& m,std::uint32_t a,int index){const int players=m.signed_half(a+0x76);return index<players?index:static_cast<std::int32_t>(static_cast<std::uint32_t>(index)-static_cast<std::uint32_t>(players)+100u);}
// Debug001FC7D0; E-0110-F01. DriverInfo absence/elimination is tested in the real roster.
bool RaceEvent::DriverInTheRace(::port::driving::Memory& m,std::uint32_t,int number){const auto driver=RaceParameters(m,0x311f78).GetDriverInfoByDriverNumber(number);return driver&&m.signed_byte(driver)==0;}
// Debug001FC7A0; E-0110-F01.
bool RaceEvent::RacerInTheRace(::port::driving::Memory& m,std::uint32_t a,int index){return DriverInTheRace(m,a,DriverNumberFromRacerIndex(m,a,index));}
// Debug001FC870; E-0110-F01. Strict comparison keeps the first index on a tie.
int RaceEvent::GetLastPlace(::port::driving::Memory& m,std::uint32_t a){int chosen=0,rank=0;for(int i=0;i<8;++i)if(RacerInTheRace(m,a,i)){const int next=m.signed_word(a+0x150+12*i);if(rank<next){chosen=i;rank=next;}}return chosen;}
// Debug001FC900; E-0110-F01. The initial comparison bound8 and default index0 are original.
int RaceEvent::GetFirstPlace(::port::driving::Memory& m,std::uint32_t a){int chosen=0,rank=8;for(int i=0;i<8;++i)if(RacerInTheRace(m,a,i)){const int next=m.signed_word(a+0x150+12*i);if(next<rank){chosen=i;rank=next;}}return chosen;}
// Debug001FCA30; E-0110-F01. Actual rank/time/KO/tournament state publication. This does
// not commit a PC profile or claim original save-device completion.
void RaceEvent::PostResults(::port::driving::Memory& m,std::uint32_t a){
 if(RaceWasAborted(m,a))return;
 const auto state=m.word(a+4);
 switch(m.signed_half(a+0x6a)){
 case 0:case 2:if(state)m.byte(state,static_cast<std::uint8_t>(m.word(a+0x150)));break;
 case 1:case 5:case 6:if(state)m.word(state,m.word(a+0x154));break;
 case 3:if(state){const int last=GetLastPlace(m,a);EventKoState::SetRank(m,state,last,m.signed_word(a+0x150+12*last));if(static_cast<std::int32_t>(m.word(0x311fa8)+m.word(0x311fac))<3){const int first=GetFirstPlace(m,a);EventKoState::SetRank(m,state,first,m.signed_word(a+0x150+12*first));}}break;
 case 4:if(state)for(unsigned i=0;i<8;++i){const auto rank=m.word(a+0x150+12*i);if(rank)m.byte(state+i,static_cast<std::uint8_t>(m.byte(state+i)+m.byte(0x32882fu+rank)));}break;
 }
}
// Debug001FD190; E-0110-F01.
int RaceEvent::GetRaceCount(const ::port::driving::Memory& m,std::uint32_t a){const int type=m.signed_half(a+0x6a);return type==3?m.signed_half(a+0x74):type==4?m.signed_half(a+0x70):1;}
// Debug001FD490; E-0110-F01. KO's last-place driver-number test is unsigned; a tournament
// continues at the original <= boundary, not a host-derived race-count rule.
bool RaceEvent::IsThereAnotherRace(::port::driving::Memory& m,std::uint32_t a){
 const int type=m.signed_half(a+0x6a);
 if(type==3){int remaining=0;for(int i=0;i<m.signed_word(0x2f3890+0x10734);++i)if(DriverInTheRace(m,a,i))++remaining;if(static_cast<std::uint32_t>(DriverNumberFromRacerIndex(m,a,GetLastPlace(m,a)))<2)--remaining;if(remaining>0)return static_cast<std::int32_t>(m.word(0x311fa8)+m.word(0x311fac))>2;}
 else if(type==4)return GetRaceIndex(m,a)+1<=GetRaceCount(m,a);
 return false;
}
// Debug001FCBD0; E-0110-F01. The two exact copies intentionally have different sources.
// Missing pState remains an explicit source-owner gap rather than a no-op retry.
void RaceEvent::RestartRace(::port::driving::Memory& m,std::uint32_t a){
 const bool preserve=!IsMultiTrack(m,a)||!IsThereAnotherRace(m,a);
 const auto state=m.word(a+4);
 if(!state)hp2::not_implemented("0x001FCBD0","original persistent event state is required for retry");
 if(preserve)m.copy(a+8,state,static_cast<unsigned>(GetStateSize(m,a)));
 m.copy(state,a+0x18,static_cast<unsigned>(GetStateSize(m,a)));
}
// Debug001FCC48, E-0101-RA02. Reset the event without recreating its roster,
// selected ride or RNG. Caller sends RC_NEXT_RACE with Param0 afterward.
void RaceEvent::RestartEvent(::port::driving::Memory& m,std::uint32_t a){
 if(!IsThereAnotherRace(m,a)&&!ResultShouldBeDiscarded(m,a))
  m.copy(a+8,m.word(a+4),static_cast<unsigned>(GetStateSize(m,a)));
 ResetState(m,a);ResetResults(m,a);
 RaceParameters parameters(m,0x311f78);parameters.ClearEliminatedDrivers();
 parameters.ResetStartingPositions();FrontendDatabase::CalculateStartingPositions(m);
 EventTrack::SetRace(m,a+0xd4,0x311f78);
}
// Debug001FD1C0, E-0101-RA02. Result publication already advances the source
// progress. Preserve current DriverInfo entries and settings; no fresh entry.
bool RaceEvent::SetupNextRace(::port::driving::Memory& m,std::uint32_t a){
 const bool next=IsThereAnotherRace(m,a);if(!next)return false;
 constexpr auto rp=0x311f78u;
 m.copy(rp+0x28c,0x2f3890+0x10738+0x644,48);
 RaceParameters parameters(m,rp);const int type=m.signed_half(a+0x6a);
 if(type==3){
  const int last=GetLastPlace(m,a);
  parameters.EliminateDriver(DriverNumberFromRacerIndex(m,a,last),m.signed_word(a+0x150+12*last));
  m.byte(rp+0x3c,std::uint8_t(m.word(a+0x150)));m.byte(rp+0x3d,std::uint8_t(m.word(a+0x15c)));
  for(int i=0;i<8;++i){const auto d=parameters.GetDriverInfoByDriverNumber(DriverNumberFromRacerIndex(m,a,i));
   if(d&&!m.signed_byte(d))m.byte(d+4,std::uint8_t(m.word(a+0x150+12*i)));}
 }else if(type==4){
  const auto state=GetTournamentState(m,a);m.byte(rp+0x3c,std::uint8_t(EventTournamentState::UserRank(m,state)));
  for(int i=0;i<8;++i){const auto d=parameters.GetDriverInfoByDriverNumber(DriverNumberFromRacerIndex(m,a,i));
   if(d&&!m.signed_byte(d))m.byte(d+4,std::uint8_t(EventTournamentState::RacerRank(m,state,i)));}
 }
 if(type==3||type==4){
  const int index=GetRaceIndex(m,a),tracks=m.signed_half(a+0x70);
  if(!tracks)hp2::not_implemented("0x001FD1C0","original SetupNextRace division-by-zero break7");
  if(index==std::numeric_limits<std::int32_t>::min()&&tracks==-1)
   hp2::not_implemented("0x001FD1C0","original SetupNextRace signed division overflow domain");
  EventTrack::SetRace(m,a+0xd4+std::uint32_t(index%tracks)*4,rp);
 }
 return next;
}
}

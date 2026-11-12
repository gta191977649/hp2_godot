// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\fred.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/tRandomlyPickableTrackInfo.hpp"

#include "game_core/indep/src/fred.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/character_resources.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/port/race/frontend_event_resources.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/src/bMath.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>

namespace hp2::debug {
// Debug001828E8, E-0101-PL01. Exact original continuation producer. The host
// service is the existing port boundary for StartRace, not a replacement race.
void FrontendDatabase::ContinueMultiTrackEvent(::port::driving::Memory& m,std::uint32_t a,const std::function<void()>& start_race){
 if(!UseTreeEvent(m,a))return;
 const auto tree=GetEventTree(m,a);
 if(!tree)hp2::not_implemented("0x001828E8","original continuation EventTree owner is missing");
 const auto selected=DataEventSelectScreen::GetSelectedEvent(m,a+0x160,m.signed_word(tree+8));
 const auto info=EventTree::GetEventNumber(m,tree,selected);
 if(!info)hp2::not_implemented("0x001828E8","original continuation selected EventInfo is missing");
 int car=0,skin=0,dash=0; // Outputs are defined by the source multitrack state.
 if(m.signed_byte(info+0x29)!=3&&m.signed_byte(info+0x29)!=4)
  hp2::not_implemented("0x001FC618","original continuation receives undefined selected-car operands for a non-multitrack event");
 EventInfo::GetSelectedCar(m,info,car,skin,dash);
 FrontendPlayerInfo::AssignCarOfType(m,a+0x10738,m.signed_word(0x2f3890+0x1072c),car,std::uint32_t(skin),dash);
 if(start_race)start_race();else StartRace(m,a);
}
// TASK-0099 E-0099-W72. Original raw-memory constructor and ride producers.
// ../indep/src/fred.cpp, Debug NFSHP2.ELF SHA815b60c7...332f1b.
void FrontendDatabase::Sub_00181BE8(::port::driving::Memory& m,std::uint32_t a){
    for(unsigned i=0;i<32;++i)for(unsigned j=0;j<3;++j)m.byte(a+0xac+i*4+j,0);
    m.byte(a+0x13f,0);
    for(unsigned i=0;i<8;++i){
        const auto stats=a+0x394+i*100;
        m.word(stats+0xc,0);m.word(stats+0x10,0);
        for(unsigned lap=0;lap<11;++lap)m.word(stats+0x34+lap*4,0);
    }
    for(unsigned i=0;i<7200;++i)m.word(a+0x6b4+i*8,0);
    for(unsigned i=0;i<800;++i)m.word(a+0xe7b4+i*8,0);
    for(unsigned player=0;player<2;++player){
        const auto p=a+0x10738+player*0x6d0;
        for(unsigned car=0;car<64;++car){m.word(p+car*24,3);m.word(p+car*24+0x10,0);m.word(p+car*24+0x14,0);}
        m.word(p+0x668,0);m.word(p+0x664,0);
        for(unsigned mode=0;mode<11;++mode){const auto ride=p+0x674+mode*8;m.word(ride,0xffffffffu);for(unsigned j=4;j<8;++j)m.byte(ride+j,0);}
    }
}
void CarStatus::AddAllSkins(::port::driving::Memory& m,std::uint32_t a,std::uint32_t info){
    const int count=CarTypeInfo::GetNumAvailableSkins(m,info);m.word(a+0x10,0);
    for(int index=0;index<count;++index){const int skin=CarTypeInfo::GetAvailableSkin(m,info,index);const auto n=m.signed_word(a+0x10);if(n<10)m.byte(a+4+std::uint32_t(n),std::uint8_t(skin));m.word(a+0x10,std::uint32_t(n)+1);}
}
void FrontendPlayerInfo::FlushAvailableCars(::port::driving::Memory& m,std::uint32_t a){for(unsigned i=0;i<64;++i)m.word(a+i*24,3);}
std::uint32_t FrontendPlayerInfo::GetCarStatus(const ::port::driving::Memory&,std::uint32_t a,int type){return a+std::uint32_t(type)*24;}
void FrontendPlayerInfo::SetCarAvailability(::port::driving::Memory& m,std::uint32_t a,eCarStatus status,int type,int skin,bool dash){
    const auto car=a+std::uint32_t(type)*24;m.word(car,std::uint32_t(status));m.word(car+0x14,dash?1:0);m.word(car+0x10,0);
    if(skin==-1){CarStatus::AddAllSkins(m,car,m.word(0x3278e0)+std::uint32_t(type)*0x560);return;}
    m.byte(car+4,std::uint8_t(skin));m.word(car+0x10,1);
}
void FrontendPlayerInfo::BuildRideInfo(::port::driving::Memory& m,std::uint32_t a,std::uint32_t ride,int type,std::uint32_t skin,int use_dash){
    const auto car=a+std::uint32_t(type)*24;const int count=m.signed_word(car+0x10);int index=0;
    while(index<count&&skin!=m.byte(car+4+std::uint32_t(index)))++index;
    if(index==count)skin=m.byte(car+4);
    const bool dash=use_dash!=0&&m.word(car+0x14)!=0&&m.byte(m.word(0x3278e0)+std::uint32_t(type)*0x560+0x80)!=0;
    m.word(ride,std::uint32_t(type));m.byte(ride+5,std::uint8_t(skin));m.byte(ride+6,dash?1:0);m.byte(ride+7,0);m.byte(ride+4,0);
}
void FrontendPlayerInfo::AssignCarOfType(::port::driving::Memory& m,std::uint32_t a,int mode,int type,std::uint32_t skin,int dash){BuildRideInfo(m,a,a+0x674+std::uint32_t(mode)*8,type,skin,dash);}
std::uint32_t FrontendPlayerInfo::GetSelectedRideInfo(const ::port::driving::Memory&,std::uint32_t a,int mode){return a+0x674+std::uint32_t(mode)*8;}
int PickCharacterNumberThatIsntRacing(::port::driving::Memory& m,std::uint32_t array,int length,int gender){
    for(int i=0;i<length;++i){const auto slot=array+std::uint32_t(i)*4;const int number=m.signed_word(slot);if(number==-1)continue;
        bool matches=gender==-1;
        if(!matches){const auto info=GetCharacterInfo(m,number);const auto upper=[](std::uint8_t c){return std::uint32_t(c-'a')<26?std::uint8_t(c&0x5f):c;};bool equal=true;
            for(unsigned j=0;;++j){const auto a=upper(m.byte(info+0x24+j)),b=upper(m.byte(0x3278d0+j));if(a!=b){equal=false;break;}if(!a)break;}
            matches=equal?gender==0:gender==1;
        }
        if(matches){m.word(slot,0xffffffffu);return number;}
    }return 0;
}
void FrontendDatabase::CalculateStartingPositions(::port::driving::Memory& m){
    constexpr auto parameters=0x311f78u;RaceParameters race(m,parameters);
    const int count=m.signed_word(parameters+0x308);if(count<0||count>14)hp2::not_implemented("0x00184390","raw DriverInfo count exceeds original14-entry storage");
    std::array<std::uint32_t,14> selected{};std::array<float,14> power{};unsigned n=0;
    for(int i=0;i<count;++i){const auto d=race.GetDriverInfo(i);const int type=m.signed_word(d+8);if(!m.signed_byte(d)&&!m.signed_byte(d+4)&&type>0&&type<3)selected[n++]=d;}
    for(unsigned i=0;i<n;++i){const auto car=m.word(0x3278e0)+m.word(selected[i]+12)*0x560;float value=0;
        for(unsigned j=0;j<9;++j){const float torque=m.scalar(car+0x2c0+j*4);if(!std::isfinite(torque))hp2::not_implemented("0x001844B8","nonfinite MAX.S torque operand outside scoped native finite projection");if(value<torque)value=torque;}
        const float mass=m.scalar(car+0x1e0),radius=m.scalar(car+0x18c);if(mass==0||radius==0)hp2::not_implemented("0x001844DC","original zero-divisor starting-position power operand requires EE numerical policy");
        value=value/mass;value=value*m.scalar(car+0x298);value=value*m.scalar(car+0x27c);power[i]=value/radius;
    }
    for(unsigned pass=0;pass<n;++pass)for(unsigned i=0;i+1<n;++i)if(power[i]<power[i+1]){std::swap(power[i],power[i+1]);std::swap(selected[i],selected[i+1]);}
    for(unsigned i=0;i<n;++i){int position=1;for(;position<=8;++position){bool occupied=false;for(int j=0;j<count;++j)if(m.signed_byte(race.GetDriverInfo(j)+4)==position){occupied=true;break;}if(!occupied)break;}m.byte(selected[i]+4,std::uint8_t(position));}
}
std::uint32_t FrontendDatabase::GetEventTree(const ::port::driving::Memory& m,std::uint32_t a){
    const int type=m.signed_word(a+0x1072c);if(type!=8&&type!=9)return 0;
    const auto name=type==8?(m.word(0x326e98)?0x2abfc0u:0x2abfd0u):(m.word(0x326e98)?0x2abfe0u:0x2abff0u);std::string text;
    for(unsigned j=0;;++j){const auto c=m.byte(name+j);if(!c)break;text+=char(c);}return hp2::debug::GetEventTree(m,text);
}
void FrontendDatabase::MakeEventCarsAvailable(::port::driving::Memory& m,std::uint32_t a,std::uint32_t tree,std::uint32_t event){
    for(unsigned p=0;p<2;++p)FrontendPlayerInfo::FlushAvailableCars(m,a+0x10738+p*0x6d0);
    for(int i=0;i<m.signed_byte(event+0x3b);++i){const auto car=m.word(tree+0x14)+std::uint32_t(m.signed_half(event+0x44+std::uint32_t(i)*2))*4;if(!car)continue;const auto packed=m.word(car);const int skin=(packed>>24)&15;
        for(unsigned p=0;p<2;++p)FrontendPlayerInfo::SetCarAvailability(m,a+0x10738+p*0x6d0,eCAR_UNLOCKED_FULLY_VISIBLE,m.byte(car+2),skin?skin:-1,((packed>>30)&1)!=0);
    }
}
int PickCopCharacterNumber(::port::driving::Memory& m,int){
    const auto track=hp2::debug::GetTrackInfo(m,m.signed_word(0x311f78));auto hash=m.word(track+0x54);
    if(!hash){std::string name;for(unsigned j=0;;++j){const auto c=m.byte(0x3278d8+j);if(!c)break;name+=char(c);}hash=hp2::frontend::bStringHash(name.c_str());}
    const auto character=GetCharacterInfoFromNameHash(m,hash);return character?m.signed_word(character):-1;
}
int AreThereAnyCarsOfType(::port::driving::Memory& m,int type){
    RaceParameters race(m,0x311f78);for(int i=0;i<m.signed_word(0x312280);++i){const auto d=race.GetDriverInfo(i);if(!m.signed_byte(d)&&m.signed_word(d+12)==type)return 1;}return 0;
}
int AreThereAnyCarsOfColour(::port::driving::Memory& m,int type,int skin){
    const auto paint=m.word(::port::race::GetSkinInfo(m,type,skin,1)+0x28);RaceParameters race(m,0x311f78);
    for(int i=0;i<m.signed_word(0x312280);++i){const auto d=race.GetDriverInfo(i);if(m.signed_byte(d))continue;const auto info=::port::race::GetSkinInfo(m,m.signed_word(d+12),m.signed_byte(d+17),1);if(m.word(info+0x28)==paint)return 1;}return 0;
}
int PickRandomSkinOfUniqueColour(::port::driving::Memory& m,int type){
    int skin=1;const auto info=m.word(0x3278e0)+std::uint32_t(type)*0x560;for(int i=0;i<100;++i){skin=CarTypeInfo::GetRandomSkin(m,info);if(!AreThereAnyCarsOfColour(m,type,skin))break;}return skin;
}
void FrontendDatabase::SetupCarsForRace(::port::driving::Memory& m,std::uint32_t a){
    constexpr auto parameters=0x311f78u,fred=0x2f3890u,event=0x312cd0u;RaceParameters race(m,parameters);race.ClearEliminatedDrivers();
    hp2::recovered::bSetRandomSeed(m,m.word(parameters+0x30c),0x329060);
    ::port::driving::Memory::Temporary characters(m,32),ride(m,8),driver(m,24);
    for(unsigned i=0;i<8;++i)m.word(characters.address()+i*4,i);
    m.word(parameters+0x308,0);int active_players=m.signed_word(parameters+0x30);
    if(!m.word(parameters+0x18)){active_players=0;
        for(int i=0;i<m.signed_word(parameters+0x30);++i){const auto player=fred+0x10738+std::uint32_t(i)*0x6d0;int character=PickCharacterNumberThatIsntRacing(m,characters.address(),8,m.signed_byte(player+0x64b));
            if(m.word(parameters+0x274)&&i==0)character=PickCopCharacterNumber(m,m.signed_word(parameters));
            const auto selected=FrontendPlayerInfo::GetSelectedRideInfo(m,player,m.signed_word(fred+0x1072c));m.byte(selected+7,1);
            DriverInfo::Sub_001867C0(m,driver.address(),i,1,selected,character,-2,m.signed_byte(parameters+0x3c+std::uint32_t(i)));race.AddDriverInfo(driver.address());
            if(UseTheRaceEvent(m,a)&&RaceEvent::DriverEliminated(m,event,i)){const int index=RaceEvent::RacerIndexFromDriverNumber(m,event,i);race.EliminateDriver(i,EventKoState::GetRank(m,m.word(event+4),index));}else ++active_players;
        }
    }m.word(parameters+0x30,std::uint32_t(active_players));
    // Original class reads/RNG precede AI construction even on explicit events.
    if(!m.word(parameters+0x18)){
        const auto selected=FrontendPlayerInfo::GetSelectedRideInfo(m,fred+0x10738,m.signed_word(a+0x1072c));(void)m.signed_byte(m.word(0x3278e0)+m.word(selected)*0x560+0xe6);
        if(m.signed_word(parameters+0x30)!=1){const auto second=FrontendPlayerInfo::GetSelectedRideInfo(m,fred+0x10e08,m.signed_word(a+0x1072c));(void)m.signed_byte(m.word(0x3278e0)+m.word(second)*0x560+0xe6);}
    }else (void)hp2::recovered::bRandom(m,5);
    int active_ai=0;
    for(int i=0;i<m.signed_word(parameters+0x34);++i){const int character=PickCharacterNumberThatIsntRacing(m,characters.address(),8,-1),number=100+i;
        m.word(ride.address(),0xffffffffu);m.word(ride.address()+4,0);
        if(m.word(parameters+0x38)!=2)hp2::not_implemented("0x00183E2C","nonexplicit roster requires reviewed Debug single versus Retail police-selected dual skill distributions and original random uniqueness path");
        const auto tree=GetEventTree(m,a),info=EventTree::GetEventNumber(m,tree,m.signed_word(a+0x10730));const auto opponent=m.word(tree+0x34)+std::uint32_t(m.signed_half(info+0x9c+std::uint32_t(i)*2))*4;
        // MIPS183DF0..183E10 retains this signed negative-skin branch; the
        // decompiler incorrectly marks183DFC unreachable and omits the call.
        int skin=m.signed_byte(opponent+1);if(skin<0)skin=PickRandomSkinOfUniqueColour(m,m.byte(opponent));
        m.byte(ride.address()+5,std::uint8_t(skin));m.byte(ride.address()+4,0);m.byte(ride.address()+6,0);m.byte(ride.address()+7,0);const int skill=m.byte(opponent+2);m.word(ride.address(),m.byte(opponent));
        DriverInfo::Sub_001867C0(m,driver.address(),number,2,ride.address(),character,skill,0);race.AddDriverInfo(driver.address());
        if(UseTheRaceEvent(m,a)&&RaceEvent::DriverEliminated(m,event,number)){const int index=RaceEvent::RacerIndexFromDriverNumber(m,event,number);race.EliminateDriver(number,EventKoState::GetRank(m,m.word(event+4),index));}else ++active_ai;
    }m.word(parameters+0x34,std::uint32_t(active_ai));
    const int traffic=std::abs(m.scalar(parameters+0x10))<=m.scalar(0x3253fc)?0:6;
    for(int i=0;i<traffic;++i){m.word(ride.address(),0xffffffffu);m.word(ride.address()+4,0);
        for(int tries=0;tries<100;++tries){GenerateRandomRideInfo(m,ride.address(),CAR_USAGE_TYPE_TRAFFIC,-1);m.byte(ride.address()+5,0xff);if(!AreThereAnyCarsOfType(m,m.signed_word(ride.address())))break;}
        DriverInfo::Sub_001867C0(m,driver.address(),50+i,3,ride.address(),-1,-1,0);race.AddDriverInfo(driver.address());
    }
    std::uint64_t minimum_class=5;
    for(int i=0;i<m.signed_word(parameters+0x308);++i){const auto d=race.GetDriverInfo(i);const auto car=m.word(0x3278e0)+m.word(d+12)*0x560;const auto car_class=std::uint64_t(std::int64_t(m.signed_byte(car+0xe6)));if(car_class<minimum_class)minimum_class=car_class;}
    if(8<m.signed_word(parameters+0x30)+m.signed_word(parameters+0x34)+m.signed_word(parameters+0x25c))m.word(parameters+0x25c,m.word(parameters+0x30)+m.word(parameters+0x34));
    const int cops=m.signed_word(parameters+0x25c);
    for(int i=0;i<cops;++i){const int character=PickCopCharacterNumber(m,m.signed_word(parameters));int selected=-1,closest_class=0;
        for(int type=0;type<64;++type){const auto info=m.word(0x3278e0)+std::uint32_t(type)*0x560;if(m.word(info+0x538)!=1||!m.byte(info+0x40))continue;const int car_class=m.signed_byte(info+0xe6);if(selected==-1||std::abs(int(minimum_class)-closest_class)>std::abs(int(minimum_class)-car_class)){selected=type;closest_class=car_class;}}
        m.word(ride.address(),std::uint32_t(selected));m.byte(ride.address()+5,1);m.byte(ride.address()+6,0);m.byte(ride.address()+7,0);m.byte(ride.address()+4,0);
        DriverInfo::Sub_001867C0(m,driver.address(),10+i,4,ride.address(),character,-1,0);race.AddDriverInfo(driver.address());
    }CalculateStartingPositions(m);
}
// Debug00185CD8, E-0098-S01. Original count increments even past ten storage slots.
void CarStatus::AddAllSkins(const CarTypeInfo& info,int language){
    const int count=info.GetNumAvailableSkins();nSkins=0;
    for(int i=0;i<count;++i){
        const auto skin=info.GetAvailableSkin(i,language);
        if(nSkins<10)aSkins[nSkins]=static_cast<std::uint8_t>(skin);
        ++nSkins;
    }
}
// Debug00185AF8, E-0098-S01. Deliberately leaves skins and dashboard state intact.
void FrontendPlayerInfo::FlushAvailableCars(){for(auto& car:aCarStatus)car.nStatus=eCAR_LOCKED_INVISIBLE;}
// Debug00185B28, E-0098-S01; host bounds guard replaces out-of-domain native access.
CarStatus& FrontendPlayerInfo::GetCarStatus(int type){
    if(type<0 || type>=64)hp2::not_implemented("0x00185B28","car identity outside original64 statuses");
    return aCarStatus[type];
}
// Debug00185AB0, E-0098-S01.
void FrontendPlayerInfo::SetCarAvailability(eCarStatus status,int type,int skin_number,bool dash_available,
    const std::array<CarTypeInfo,64>& cars,int language){
    if(type<0 || type>=64)hp2::not_implemented("0x00185AB0","car identity outside original64 statuses");
    auto& car=aCarStatus[type];car.nStatus=status;car.bDashAvailable=dash_available;car.nSkins=0;
    if(skin_number==-1)car.AddAllSkins(cars[type],language);
    else{car.aSkins[0]=static_cast<std::uint8_t>(skin_number);car.nSkins=1;}
}
// Debug00185B88, E-0098-S01. Zero count still selects stored slot0 in the original.
void FrontendPlayerInfo::BuildRideInfo(RideInfo& ri,int type,std::uint32_t skin_number,
    int use_dash_if_available,const std::array<CarTypeInfo,64>& cars) const{
    if(type<0 || type>=64)hp2::not_implemented("0x00185B88","invalid car identity");
    const auto& car=aCarStatus[type];
    if(car.nSkins<0 || car.nSkins>10)hp2::not_implemented("0x00185B88","skin list exceeds decoded storage");
    int i=0;while(i<car.nSkins && skin_number!=car.aSkins[i])++i;
    if(i==car.nSkins)skin_number=car.aSkins[0];
    ri.Type=type;ri.SkinNumber=static_cast<std::uint8_t>(skin_number);
    ri.HasDash=use_dash_if_available && car.bDashAvailable && cars[type].DashGeometryFilename[0];
    ri.CanBeVertexDamaged=0;ri.InstanceIndex=0;
}
// Debug00185C38, E-0098-S01. No Retail profile layout is bound here.
void FrontendPlayerInfo::AssignCarOfType(int game_type,int type,std::uint32_t skin_number,
    int use_dash_if_available,const std::array<CarTypeInfo,64>& cars){
    if(game_type<0 || game_type>=11)hp2::not_implemented("0x00185C38","invalid original game type");
    BuildRideInfo(RideInfoByGameMode[game_type],type,skin_number,use_dash_if_available,cars);
}
// Debug00185CC8, E-0098-S01.
RideInfo& FrontendPlayerInfo::GetSelectedRideInfo(int game_type){
    if(game_type<0 || game_type>=11)hp2::not_implemented("0x00185CC8","invalid original game type");
    return RideInfoByGameMode[game_type];
}
// Debug001822F0, E-0098-S01. Skin0 means all; dashboard is packed bit30.
void FrontendDatabase::MakeEventCarsAvailable(const EventTree& tree,const EventInfo& event,
    const std::array<CarTypeInfo,64>& cars,int language){
    for(auto& player:FrontendPlayers)player.FlushAvailableCars();
    for(int i=0;i<event.nCars;++i){
        const auto* packed=event.GetCar(tree,i);
        if(!packed)continue;
        const int type=(*packed>>16)&255,skin=(*packed>>24)&15;
        if(type>=64)hp2::not_implemented("0x001822F0","event car identity outside original table");
        for(auto& player:FrontendPlayers)player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,
            type,skin?skin:-1,((*packed>>30)&1)!=0,cars,language);
    }
}
}

namespace hp2::recovered {
// ELF 0x001841A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001841a8.md.
void FrontendDatabase::UseTheRaceEvent() {
    hp2::not_implemented("0x001841A8", "FrontendDatabase::UseTheRaceEvent (signature and full body unresolved)");
}

// ELF 0x001855F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001855f0.md.
void FrontendDatabase::IsChallengeGame() {
    hp2::not_implemented("0x001855F0", "FrontendDatabase::IsChallengeGame (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00187928; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00187928.md.
void FrontendPlayerInfo::FlushAvailableCars() {
    hp2::not_implemented("0x00187928", "FrontendPlayerInfo::FlushAvailableCars (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00187080; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00187080.md.
void TransientStats::GetDriver() {
    hp2::not_implemented("0x00187080", "TransientStats::GetDriver (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/launch.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>
namespace hp2::debug {
// Debug00182610, E-0098-RP02. First unused gender-compatible character; no random draw.
int PickCharacterNumberThatIsntRacing(std::array<int,8>& available,int gender,
    const std::function<bool(int)>& is_male){
    for(auto& number:available){
        if(number==-1)continue;
        bool match=gender==-1;
        if(!match){
            if(!is_male)hp2::not_implemented("0x00182610","original character body-type binding missing");
            const bool male=is_male(number);match=male?gender==0:gender==1;
        }
        if(match){const int result=number;number=-1;return result;}
    }
    return 0;
}
// Debug00183A08, E-0098-RP02. Original argument ignored; global race track is read.
int PickCopCharacterNumber(::port::driving::Memory& memory,std::uint32_t race,int unused,
    const ::port::race::ParticipantServices& services){
    (void)unused;
    if(!services.track_cop_hash || !services.character_number_from_hash)
        hp2::not_implemented("0x00183A08","original track/character binding missing");
    const auto hash=services.track_cop_hash(memory.signed_word(race));
    if(!hash)hp2::not_implemented("0x00183A08","original GetTrackInfo missing record");
    const auto character=services.character_number_from_hash(*hash?*hash:hp2::frontend::bStringHash("Cop"));
    return character?*character:-1;
}
// Debug00184390, E-0098-RP02. Bounded no-unassigned-racers branch used by first HP event.
// Power/weight ordering for unassigned racers remains explicitly untranslated.
void FrontendDatabase::CalculateStartingPositions(::port::driving::Memory& memory,std::uint32_t address){
    RaceParameters race(memory,address);const int count=memory.signed_word(address+0x308);
    if(count<0 || count>14)hp2::not_implemented("0x00184390","original driver count outside host table");
    for(int i=0;i<count;++i){const auto driver=race.GetDriverInfo(i);const int type=memory.signed_word(driver+8);
        if(memory.signed_byte(driver)==0 && memory.signed_byte(driver+4)==0 && type>0 && type<3)
            hp2::not_implemented("0x00184390","original unassigned racer power/weight ordering not translated");
    }
}
// Debug00183A58, E-0098-RP02. First single-player HP delivery path; other event/AI
// branches fail before mutation. Explicit services bind original resource globals.
void FrontendDatabase::SetupCarsForRace(::port::driving::Memory& memory,std::uint32_t address,
    const EventInfo& event,const ::port::race::ParticipantServices& services){
    if(GameType!=9 || event.nNumber!=1 || event.nType!=5 || event.nOpponents!=0
        || memory.word(address+0x30)!=1 || memory.word(address+0x34)!=0
        || memory.word(address+0x18)!=0 || memory.word(address+0x274)!=0 || memory.word(address+0x20)!=1)
        hp2::not_implemented("0x00183A58","non-first-delivery SetupCarsForRace branch not translated");
    if(!services.character_address || !services.skin_info)
        hp2::not_implemented("0x00183A58","original character/skin providers missing");
    RaceParameters race(memory,address);race.ClearEliminatedDrivers();
    hp2::recovered::bSetRandomSeed(memory,memory.word(address+0x30c),0x329060);
    std::array<int,8> characters{0,1,2,3,4,5,6,7};memory.word(address+0x308,0);
    ::port::driving::Memory::Scratch scratch(memory);const auto driver=memory.allocate(24);
    const int character=PickCharacterNumberThatIsntRacing(characters,FrontendPlayers[0].mPlayerSettings.PreferredGender,services.character_is_male);
    auto& selected=FrontendPlayers[0].GetSelectedRideInfo(GameType);
    selected.CanBeVertexDamaged=1;
    DriverInfo(memory,driver,0,1,selected,character,-2,memory.signed_byte(address+0x3c),services.character_address);
    race.AddDriverInfo(driver,services.skin_info);
    // RacerEliminated001FC808 is false for the guarded SINGLE_RACE branch.
    memory.word(address+0x30,1);memory.word(address+0x34,0);
    const int traffic=std::abs(memory.scalar(address+0x10))<=memory.scalar(0x3253fc)?0:6;
    const auto already_present=[&](int type){
        for(int i=0;i<memory.signed_word(address+0x308);++i){const auto entry=race.GetDriverInfo(i);
            if(memory.signed_byte(entry)==0 && memory.signed_word(entry+12)==type)return true;}
        return false;
    };
    for(int i=0;i<traffic;++i){
        RideInfo ride;ride.Init(-1,0,0,0);
        for(int attempt=0;attempt<100;++attempt){
            GenerateRandomRideInfo(memory,ride,CAR_USAGE_TYPE_TRAFFIC,-1,services.cars,race.GetLocationIndex(),services.language);
            ride.SkinNumber=255; // Original overwrites AFTER the skin RNG draw.
            if(!already_present(ride.Type))break;
        }
        DriverInfo(memory,driver,i+50,3,ride,-1,-1,0,services.character_address);race.AddDriverInfo(driver,services.skin_info);
    }
    int minimum_class=5; //0018420C uses signed SLT; decompiler's ulong is misleading.
    for(int i=0;i<memory.signed_word(address+0x308);++i){const auto type=memory.signed_word(race.GetDriverInfo(i)+12);
        if(type<0 || type>=64)hp2::not_implemented("0x00183A58","generated car outside original catalog domain");
        const int car_class=services.cars[type].CarClass;
        if(car_class<minimum_class)minimum_class=car_class;
    }
    int cops=memory.signed_word(address+0x25c);
    if(8<memory.signed_word(address+0x30)+memory.signed_word(address+0x34)+cops){
        cops=memory.signed_word(address+0x30)+memory.signed_word(address+0x34);memory.word(address+0x25c,cops);
    }
    for(int i=0;i<cops;++i){
        const int cop_character=PickCopCharacterNumber(memory,address,memory.signed_word(address),services);
        int type=-1,closest_class=0;
        for(int n=0;n<64;++n){const auto& car=services.cars[n];
            if(car.UsageType!=CAR_USAGE_TYPE_COP || !car.GeometryFilename[0])continue;
            if(type==-1 || std::abs(int(minimum_class)-car.CarClass)<std::abs(int(minimum_class)-closest_class)){
                type=n;closest_class=car.CarClass;
            }
        }
        RideInfo ride;ride.Init(type,1,0,0);
        DriverInfo(memory,driver,i+10,4,ride,cop_character,-1,0,services.character_address);race.AddDriverInfo(driver,services.skin_info);
    }
    CalculateStartingPositions(memory,address);
}
}

namespace hp2::debug {
// Debug00183378, E-0098-RP05. First HP delivery branch; requests original GameFlow
// unload/load transition, never directly starts a single-car fixture scene.
void FrontendDatabase::StartRace(::port::driving::Memory& memory,std::uint32_t address,
    RaceEvent& active,const ::port::race::StartServices& services){
    if(GameType!=9 || active.nType!=5 || active.nNumber!=1 || nCurrentEvent!=1
        || !active.pTree || active.pTree->nType!=1 || !active.pState.data || services.demo_replay_active)
        hp2::not_implemented("0x00183378","non-first-delivery or demo-replay StartRace branch not translated");
    if(!services.track_point_to_point || !services.copy_profile_settings || !services.set_record_times || !services.change_game_flow)
        hp2::not_implemented("0x00183378","required original track/profile/records/GameFlow binding missing");
    const auto event=std::find_if(active.pTree->pEvents.begin(),active.pTree->pEvents.end(),
        [&](const auto& info){return info.nNumber==nCurrentEvent;});
    if(event==active.pTree->pEvents.end())hp2::not_implemented("0x00183378","current original event not found in owning tree");
    // GetStatus(time-state) never returns INPROGRESS; keep status lookup before reset.
    const auto status=EventTimeState::GetStatus(active.pState,active.Objective);
    if(status!=eEVENT_INPROGRESS)active.ResetState();
    if(active.nTracks<1 || active.nTracks>8)hp2::not_implemented("0x00183378","original track-index division/table domain invalid");
    const int track_index=active.GetRaceIndex()%active.nTracks;
    EventTrack::SetRace(active.aTracks[track_index],memory.view(address,0x310),services.track_point_to_point);
    active.bWillSuperBonusEventBeUnlockedIfThisEventIsWon=0;
    active.bWillSuperBonusEventBeUnlockedIfThisEventIsWon=event->WillSuperBonusEventBeUnlockedIfThisEventIsWon();
    if(services.maximum_ai_cars<memory.signed_word(address+0x34))memory.word(address+0x34,services.maximum_ai_cars);
    if(8<memory.signed_word(address+0x34)+memory.signed_word(address+0x30))memory.word(address+0x34,8-memory.word(address+0x30));
    if(memory.word(address+0x28)==0){
        const auto track=services.track_point_to_point(memory.signed_word(address));
        if(!track)hp2::not_implemented("0x00183378","original track record required for point-to-point correction");
        if(*track)memory.word(address+0x28,1);
    }
    SetupCarsForRace(memory,address,*event,services.participants);
    services.copy_profile_settings(memory,address);
    for(int player=0;player<memory.signed_word(address+0x30);++player)services.set_record_times(player);
    RaceParameters(memory,address).ChooseHudTextures();
    // MaybeSetSelectedCarForMultiTrackEvent00182818 -> EventInfo::SetSelectedCar001FC5E8
    // makes no writes for the guarded type5; KO/tournament branches are not substituted.
    memory.word(address+0x258,1);
    using ::port::race::Flow;
    services.change_game_flow(services.current_flow==Flow::in_frontend?Flow::unloading_frontend:Flow::loading_track_common,Flow::none);
    bRaceWasAborted=0;
}
}

#include "game_core/indep/src/fred.hpp"
#include "game_core/indep/src/eventtree.hpp"
namespace hp2::debug {
// Debug00182718; E-0099-W56. Original GameTypes enum4/8/9; no decoded
// frontend/profile state is substituted for this original shared-memory field.
bool FrontendDatabase::UseTheRaceEvent(const ::port::driving::Memory& m,std::uint32_t address){const auto type=m.signed_word(address+0x1072c);return type==4||type==8||type==9;}
// Debug00185888. pRace is not consumed by this source body.
void FrontendDatabase::TheRaceIsOn(::port::driving::Memory& m,std::uint32_t address,std::uint32_t race){(void)race;if(UseTheRaceEvent(m,address))RaceEvent::SavePreRaceState(m,0x312cd0);}
}
namespace hp2::debug {
bool FrontendDatabase::UseTreeEvent(const ::port::driving::Memory& m,std::uint32_t a){const int type=m.signed_word(a+0x1072c);return type<10&&type>7;}
void FrontendDatabase::TheRaceIsLeaving(::port::driving::Memory& m,std::uint32_t address){if(UseTheRaceEvent(m,address))RaceEvent::MaybeDiscardResult(m,0x312cd0);}
}

namespace hp2::debug {
// E-0099-W84; Debug00184A78. Canonical track indices are assigned before
// aliases, with the original16-table overflow-to-zero behavior. Reset only
// the two source record arrays, then publish source INT_MAX sentinel times.
void HighScoreTable::InitStatsTables(::port::driving::Memory& m,std::uint32_t a){
 int previous=0,next=1;bool available=true;
 for(int number=1;number<=m.signed_word(0x3273a0);++number){
  const auto track=GetTrackInfo(m,number);
  if(track&&m.signed_byte(track+0x40)&&m.signed_byte(track+0x41)==m.signed_byte(track+0x42)){
   m.half(track+0x46,std::uint16_t(available?previous:0));previous=next;available=next<16;++next;
  }
 }
 for(int number=1;number<=m.signed_word(0x3273a0);++number){
  const auto track=GetTrackInfo(m,number);
  if(track&&m.signed_byte(track+0x40)&&m.signed_byte(track+0x41)!=m.signed_byte(track+0x42)){
   const auto original=GetTrackInfo(m,m.signed_byte(track+0x42));
   if(!original)hp2::not_implemented("0x00184B50","source valid aliased track requires its original SameAsTrackNumber record");
   m.half(track+0x46,m.half(original+0x46));
  }
 }
 std::memset(m.view(a,0xe100).data,0,0xe100);
 std::memset(m.view(a+0xe100,0x1900).data,0,0x1900);
 for(unsigned i=0;i<7200;++i)m.word(a+8*i,0x7fffffffu);
 for(unsigned i=0;i<800;++i)m.word(a+0xe100+8*i,0x7fffffffu);
}
// Debug00184C10/00184C48; E-0099-W84. Signed original fields and authored
// nine-entry lap table; unknown lap count deliberately maps to source index0.
int HighScoreTable::GetUniqueTrackIndex(::port::driving::Memory& m,int number){const auto track=GetTrackInfo(m,number);return track&&m.signed_byte(track+0x40)?m.signed_half(track+0x46):0;}
int HighScoreTable::GetUniqueLapIndex(const ::port::driving::Memory& m,int laps){for(unsigned i=0;i<9;++i)if(laps==m.signed_byte(0x2ac028+i))return int(i);return 0;}
// Debug00184CA8/00184D40; E-0099-W84. Unsigned host arithmetic explicitly
// retains original32-bit multiplication/addition wrap and nonzero direction.
int HighScoreTable::GetTotalTimesIndex(::port::driving::Memory& m,int track,int car_class,int direction,int laps){return std::int32_t(std::uint32_t(GetUniqueTrackIndex(m,track))*450u+std::uint32_t(GetUniqueLapIndex(m,laps))*5u+(direction!=0?45u:0u)+std::uint32_t(car_class)*90u);}
int HighScoreTable::GetLapTimesIndex(::port::driving::Memory& m,int track,int car_class,int direction){return std::int32_t(std::uint32_t(GetUniqueTrackIndex(m,track))*50u+std::uint32_t(car_class)*10u+(direction!=0?5u:0u));}
// Debug00185108/00185140; E-0099-W84. Real Fred ChallengeHighScores arrays;
// no decoded profile/high-score replacement or implicit fresh initialization.
std::uint32_t FrontendDatabase::GetBestTotalTimesTable(::port::driving::Memory& m,std::uint32_t a,int track,int car_class,int direction,int laps){return a+0x6b4+8u*std::uint32_t(HighScoreTable::GetTotalTimesIndex(m,track,car_class,direction,laps));}
std::uint32_t FrontendDatabase::GetBestLapTimesTable(::port::driving::Memory& m,std::uint32_t a,int track,int car_class,int direction){return a+0xe7b4+8u*std::uint32_t(HighScoreTable::GetLapTimesIndex(m,track,car_class,direction));}
// Debug00182990; E-0099-W84. Original uses global Fred.GameType even when
// this's FrontendPlayers/ChallengeHighScores are accessed through another a.
void FrontendDatabase::SetRecordTimesToBeat(::port::driving::Memory& m,std::uint32_t a,int player){
 if(player<0||player>=2)hp2::not_implemented("0x00182990","source player number outside mapped two-player record domain");
 const auto ride=FrontendPlayerInfo::GetSelectedRideInfo(m,a+0x10738+std::uint32_t(player)*0x6d0,m.signed_word(0x2f3890+0x1072c));
 const auto track=m.signed_word(0x311f78),direction=m.signed_word(0x311f84),laps=m.signed_word(0x311fa4);
 const int car_class=m.signed_byte(m.word(0x3278e0)+m.word(ride)*0x560+0xe6);
 const auto settings=0x312204u+std::uint32_t(player)*0x30;
 if(laps)m.word(settings+0x20,m.word(GetBestTotalTimesTable(m,a,track,car_class,direction,laps)));
 m.word(settings+0x24,m.word(GetBestLapTimesTable(m,a,track,car_class,direction)));
}
}

#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"
#include "game_core/indep/src/gameflow.hpp"
namespace hp2::debug {
// Debug001839B8/001839E0, E-0099-W86. Source signed GameType interval tests.
bool FrontendDatabase::IsChallengeGame(const ::port::driving::Memory& m,std::uint32_t a){const auto type=m.signed_word(a+0x1072c);return type<8&&type>2;}
bool FrontendDatabase::IsEventGame(const ::port::driving::Memory& m,std::uint32_t a){const auto type=m.signed_word(a+0x1072c);return type<10&&type>7;}
// Debug00182818, E-0099-W86. Actual selected UI event can differ from
// nCurrentEvent. Global Fred.GameType selects this's first player's ride.
void FrontendDatabase::MaybeSetSelectedCarForMultiTrackEvent(::port::driving::Memory& m,std::uint32_t a){
 if(!UseTreeEvent(m,a))return;
 const auto ride=FrontendPlayerInfo::GetSelectedRideInfo(m,a+0x10738,m.signed_word(0x2f3890+0x1072c));
 const auto tree=GetEventTree(m,a);
 if(!tree)hp2::not_implemented("0x0018288C","source event-game selected-car consumer requires its original EventTree");
 const auto selected=DataEventSelectScreen::GetSelectedEvent(m,a+0x160,m.signed_word(tree+8));
 const auto info=EventTree::GetEventNumber(m,tree,selected);
 if(!info)hp2::not_implemented("0x001FC5E8","source selected event requires its actual authored EventInfo");
 EventInfo::SetSelectedCar(m,info,m.signed_word(ride),m.signed_byte(ride+5),m.byte(ride+6)!=0);
}
// Debug00183378, E-0099-W86. Raw source StartRace pipeline and actual pending
// GameFlow request. Full Fred defaults/ServiceGameFlow execution are separate.
// Challenge fake-event/boost and active demo file restore retain precise guards.
void FrontendDatabase::StartRace(::port::driving::Memory& m,std::uint32_t a){
 constexpr auto event=0x312cd0u,parameters=0x311f78u,fred=0x2f3890u;
 if(m.signed_word(a+0x1072c)==4)hp2::not_implemented("0x00182E00","actual SetupFakeRaceEventForChallengeKnockout is required");
 if(UseTheRaceEvent(m,a)){if(RaceEvent::GetStatus(m,event)!=eEVENT_INPROGRESS)RaceEvent::ResetState(m,event);}
 else m.half(event+0x76,std::uint16_t(m.word(a+0x10734)));
 if(UseTheRaceEvent(m,a)){
  const int index=RaceEvent::GetRaceIndex(m,event),tracks=m.signed_half(event+0x70);
  if(!tracks)hp2::not_implemented("0x00183440","original StartRace division-by-zero break7");
  if(index==std::numeric_limits<std::int32_t>::min()&&tracks==-1)hp2::not_implemented("0x00183438","exceptional original signed DIV overflow domain");
  EventTrack::SetRace(m,event+0xd4+std::uint32_t(index%tracks)*4,parameters);
  if(m.signed_half(event+0x6a)==3)m.byte(parameters+0x3c,std::uint8_t(m.byte(parameters+0x3c)-RaceEvent::GetRaceIndex(m,event)));
  if(m.signed_half(event+0x6a)==4&&RaceEvent::GetStatus(m,event)==eEVENT_INPROGRESS)m.byte(parameters+0x3c,std::uint8_t(EventTournamentState::UserRank(m,RaceEvent::GetTournamentState(m,event))));
 }
 m.word(event+0x78,0);
 if(UseTheRaceEvent(m,a))if(const auto tree=GetEventTree(m,a)){
  const auto info=EventTree::GetEventNumber(m,tree,m.signed_word(a+0x10730));
  if(!info)hp2::not_implemented("0x001FC1E8","source bonus prediction requires the current authored event");
  m.word(event+0x78,EventInfo::WillSuperBonusEventBeUnlockedIfThisEventIsWon(m,info)?1:0);
 }
 if(IsChallengeGame(m,a)||m.signed_word(a+0x1072c)==2){
  // E-0107-F01 recovers this shared copy only for the reviewed FreeRun producer.
  if(m.signed_word(a+0x1072c)!=FRED_CHALLENGE_GAME_FREE_RUN)hp2::not_implemented("0x001835C4","non-FreeRun challenge/express profile and difficulty producer remains outside recovered scope");
  for(int player=0;player<m.signed_word(parameters+0x30);++player){
   if(player>=2)hp2::not_implemented("0x001835C4","source BoostScale player index exceeds the two original outputs");
   const auto ride=FrontendPlayerInfo::GetSelectedRideInfo(m,fred+0x10738+std::uint32_t(player)*0x6d0,m.signed_word(fred+0x1072c));const int skill=m.signed_word(parameters+0x2f4),car_class=m.signed_byte(m.word(0x3278e0)+m.word(ride)*0x560+0xe6);
   if(skill<0||skill>=3||car_class<0||car_class>=5)hp2::not_implemented("0x001835C4","original challenge BoostScale index exceeds reviewed three-by-five source table");
   m.word(parameters+0x2fc+4*std::uint32_t(player),m.word(0x2e7980+0x288+20*std::uint32_t(skill)+4*std::uint32_t(car_class)));
  }
 }else if(!IsEventGame(m,a)&&m.signed_word(a+0x1072c)==1){m.scalar(parameters+0x300,1);m.scalar(parameters+0x2fc,1);}
 const int maximum=m.signed_word(0x326f08);
 if(maximum<m.signed_word(parameters+0x34))m.word(parameters+0x34,std::uint32_t(maximum));
 if(std::int32_t(m.word(parameters+0x34)+m.word(parameters+0x30))>8)m.word(parameters+0x34,8u-m.word(parameters+0x30));
 if(!m.word(parameters+0x28)){
  const auto track=GetTrackInfo(m,m.signed_word(parameters));
  if(!track)hp2::not_implemented("0x00183670","source Point2Point correction requires original TrackInfo");
  if(m.signed_byte(track+0x43))m.word(parameters+0x28,1);
 }
 SetupCarsForRace(m,a);
 m.copy(parameters+0x2ec,a+0x148,8);
 for(unsigned player=0;player<2;++player)m.copy(parameters+0x28c+player*0x30,fred+0x10738+player*0x6d0+0x644,0x30);
 for(int player=0;player<m.signed_word(parameters+0x30);++player)SetRecordTimesToBeat(m,a,player);
 RaceParameters(m,parameters).ChooseHudTextures();
 MaybeSetSelectedCarForMultiTrackEvent(m,a);
 if(m.word(0x2e81f0))hp2::not_implemented("0x0013F010","actual DemoModeReplay RestoreRaceParametersSnapshot and UnloadResourceFile are required");
 const auto type=m.signed_word(fred+0x1072c);m.word(parameters+0x258,type==8||type==9?1:0);
 ChangeGameFlow(m,m.word(0x3275ac)==GAMEFLOW_STATE_IN_FRONTEND?GAMEFLOW_STATE_UNLOADING_FRONTEND:GAMEFLOW_STATE_LOADING_TRACK_COMMON,GAMEFLOW_STATE_NONE);
 m.word(a+0x10728,0);
}
}


namespace hp2::debug {
// Debug00184640, E-0107-F01. Exact original FreeRun register selection and
// final stores; other game modes remain outside this recovered branch scope.
void FrontendDatabase::SetupRaceParametersAfterRaceSetup(::port::driving::Memory& m,std::uint32_t a){
 constexpr std::uint32_t fred=0x2f3890,rp=0x311f78;
 if(m.signed_word(fred+0x1072c)!=FRED_CHALLENGE_GAME_FREE_RUN)hp2::not_implemented("0x00184640","SetupRaceParametersAfterRaceSetup non-FreeRun branches are not recovered here");
 const auto police=m.word(a+0x1c);const int players=m.signed_word(a+0x10734),traffic=m.signed_word(a+0x50+(police?4:0)),damage=m.signed_word(a+0x58);
 if(traffic<0||traffic>=4)hp2::not_implemented("0x00184970","original FreeRun traffic table index lies outside its four source words");
 const std::int32_t ai=std::min<std::int32_t>(0,std::int32_t(8u-std::uint32_t(players)));
 m.word(rp+0x25c,police?6:0);m.word(rp+0x26c,1);m.word(rp+0x270,police);m.word(rp+0x34,std::uint32_t(ai));m.word(rp+0x10,m.word(0x2ac000+4*std::uint32_t(traffic)));
 m.byte(rp+0x3c,std::uint8_t(ai+1));m.byte(rp+0x3d,std::uint8_t(ai+2));m.word(rp+0x2f8,0);m.word(rp+0x288,m.word(fred+0x14));m.word(rp+0x38,0);m.word(rp+0x40,damage!=0);
 m.word(rp+0x280,0);m.word(rp+0x284,0);m.word(rp+0x27c,2);m.word(rp+0x278,1);m.word(rp+0x48,m.signed_byte(fred+0x14f)!=0);m.word(rp+0x44,m.signed_byte(fred+0x14e)!=0);
 m.word(rp+0x264,police);m.word(rp+0x268,police);m.scalar(rp+0x14,.5f);m.word(rp+0x2f4,m.word(fred+0x14));m.word(rp+0x274,0);m.word(rp+0x4c,0);m.word(rp+0x24,0);m.word(rp+0x50,0);m.byte(rp+0x54,0);
}
}

#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/NFSTaskParameters.hpp"
namespace hp2::debug {
// Debug00185830; E-0110-F02. Both original index slots are inspected even if
// NumPlayers differs. The original clears then stores the whole abort word.
void FrontendDatabase::CheckForAbortion(::port::driving::Memory& m,std::uint32_t a){
 m.word(a+0x10728,0);
 for(unsigned i=0;i<2;++i)if(const auto player=m.word(0x327778+4*i))if(!m.word(player+0x4c))m.word(a+0x10728,1);
}
// Debug00185878; E-0110-F02. Caller001FCA50 branches on the unmasked LW return;
// decompiler SUB41(bool) is counterevidence, not a low-byte instruction here.
bool FrontendDatabase::RaceWasAborted(const ::port::driving::Memory& m,std::uint32_t a){return m.word(a+0x10728)!=0;}
// Debug00185400; E-0110-F02. All48 named settings bytes are assigned in source.
// DisplayOptions intentionally targets global Fred, even for a different this.
void FrontendDatabase::GetOptionsSetFromRace(::port::driving::Memory& m,std::uint32_t a){
 for(int i=0;i<m.signed_word(0x327780);++i){const auto p=m.word(0x327778+4u*std::uint32_t(i));const auto n=m.word(p+0x10);m.copy(a+0x10d7c+n*0x6d0,p+0x1c,0x30);m.copy(0x312204+n*0x30,p+0x1c,0x30);}
 switch(m.signed_word(a+0x1072c)){
 case 3:m.word(a+0x38,m.word(0x311fb8));break;
 case 4:m.word(a+0x74,m.word(0x311fb8));break;
 case 5:m.word(a+0x8c,m.word(0x311fb8));break;
 case 6:m.word(a+0x58,m.word(0x311fb8));break;
 case 7:m.word(a+0x4c,m.word(0x311fb8));break;
 }
 m.copy(0x2f39d8,0x312264,8);
}
// Debug00185690; E-0110-F02. Only the original first-player/racer-count gates
// qualify for the actual Player.GetFinishBonus scoring-table conversion.
int FrontendDatabase::FigureOutBonusPoints(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race,std::uint32_t racing){
 (void)a;if(m.word(0x3121ec))return 0;const auto player=RacingCar(m,racing).GetPlayer();if(!player||m.word(player+0x10))return 0;
 const int count=m.signed_word(race+0x18),rank=m.signed_byte(racing+0xc);
 const bool eligible=count==1?false:count==2?rank==1:count==3?std::uint32_t(rank-1)<=1:true;
 return eligible?Player(m,player).GetFinishBonus(rank):0;
}
// Debug00185750; E-0110-F02. Unsigned sums preserve source32-bit wrap.
void FrontendDatabase::AwardPointsToPlayer(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race,std::uint32_t racing){
 const auto player=RacingCar(m,racing).GetPlayer();if(!player)return;
 std::uint32_t cop_points=0;if(m.word(0x3121ec)){const auto cop=m.word(0x328c1c);const float seconds=cop?m.scalar(cop+0x28f0):0;
  if(!std::isfinite(seconds)||seconds<-2147483648.f||seconds>=2147483648.f)hp2::not_implemented("0x001857A8","exceptional EE BeACop score conversion operand");
  cop_points=std::uint32_t(::port::driving::cvt_w_s(seconds))*100u;
 }
 const auto info=a+0x10738+m.word(player+0x10)*0x6d0;if(!info)return;
 m.word(info+0x6cc,m.word(info+0x6cc)+m.word(player+0x5e8));
 const auto bonus=FigureOutBonusPoints(m,a,race,racing);m.word(info+0x6cc,m.word(info+0x6cc)+std::uint32_t(bonus)+cop_points);
}
// Debug00184DC0; E-0110-F02. First empty/matching slot; copy exactly11 bytes
// into the source12-byte zeroed slot, and return -1 at the128-slot limit.
int FrontendDatabase::InsertPlayerName(::port::driving::Memory& m,std::uint32_t a,std::uint32_t name){
 for(unsigned i=0;i<128;++i){const auto row=a+0x100b4+12*i;if(!m.byte(row)){for(unsigned j=0;j<12;++j)m.byte(row+j,0);m.copy(row,name,11);return int(i);}
  unsigned j=0;while(m.byte(name+j)==m.byte(row+j)){if(!m.byte(name+j))return int(i);++j;}
 }return -1;
}
// Debug00184E60; E-0110-F02. Strictly faster scores insert before the first
// slower entry, shifting the remaining4..0 records. Equal times keep order.
// The source's computed lap rank is dead; only total rank low byte is returned.
int HighScoreTable::MaybeRecordHighScore(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race,std::uint32_t racing){
 const auto player=RacingCar(m,racing).GetPlayer(),car=m.word(racing),type=m.word(car+0xc);m.word(player+0x9ec,0);m.word(player+0x9e8,0);
 const int track=m.signed_word(0x311f78),direction=m.signed_word(0x311f84),car_class=m.signed_byte(type+0xe6);
 const auto total=a+8u*std::uint32_t(GetTotalTimesIndex(m,track,car_class,direction,m.signed_word(0x311fa4)));
 int recorded=-1;
 if(m.signed_byte(racing+9)==m.signed_word(race+0x644)){
  const auto time=m.word(racing+0xb4);
  for(unsigned i=0;i<5;++i){const auto row=total+8*i;if(m.signed_word(row)<=std::int32_t(time))continue;m.copy(row+8,row,32-8*i);m.word(row,time);m.half(row+6,std::uint16_t(m.word(type+0x534)));const int name=FrontendDatabase::InsertPlayerName(m,0x2f3890,m.word(car+0x18)+0x30);m.word(player+0x9ec,row);m.half(row+4,std::uint16_t(name<0?0:name));recorded=int(i);break;}
 }
 const auto lap=a+0xe100+8u*std::uint32_t(GetLapTimesIndex(m,track,car_class,direction));const auto time=m.word(RacingCar(m,racing).GetBestLapTime());
 for(unsigned i=0;i<5;++i){const auto row=lap+8*i;if(m.signed_word(row)<=std::int32_t(time))continue;m.copy(row+8,row,32-8*i);m.half(row+6,std::uint16_t(m.word(type+0x534)));const int name=FrontendDatabase::InsertPlayerName(m,0x2f3890,m.word(car+0x18)+0x30);m.word(player+0x9e8,row);m.word(row,time);m.half(row+4,std::uint16_t(name<0?0:name));break;}
 return recorded&0xff;
}
// Debug00185178; E-0110-F02. Original transient layout and actual participant
// getters; no host-derived finish status or score is introduced.
void TransientStatsElement::Construct(::port::driving::Memory& m,std::uint32_t a,std::uint32_t racing){
 const auto player=RacingCar(m,racing).GetPlayer(),car=m.word(racing);
 m.word(a,std::uint32_t(m.signed_byte(racing+0xa)));m.word(a+4,std::uint32_t(m.signed_half(car+0xa)));m.word(a+8,std::uint32_t(m.signed_half(racing+0x12)));m.word(a+0xc,m.word(racing+0xb4));m.word(a+0x10,m.word(RacingCar(m,racing).GetBestLapTime()));m.word(a+0x28,m.word(racing+0xb8));m.word(a+0x2c,m.word(racing+0xbc));m.word(a+0x30,0);
 const auto cops=m.word(0x328c1c);
 m.word(a+0x20,cops?std::uint32_t(tCopManager(m,cops).GetNumTickets(car)):0);
 if(cops&&m.word(0x3121ec)&&player&&!m.word(player+0x10))m.word(a+0x30,m.word(cops+0x28f0));
 if(player){m.word(a+0x14,m.word(player+0x5e8));m.word(a+0x18,m.word(0x2f3890+0x10738+m.word(player+0x10)*0x6d0+0x6cc));m.word(a+0x1c,std::uint32_t(FrontendDatabase::FigureOutBonusPoints(m,0x2f3890,m.word(0x327258),racing)));m.word(a+0x24,std::uint32_t(m.signed_byte(player+0x1e)));}
 else{m.word(a+0x14,0);m.word(a+0x18,0);m.word(a+0x1c,0);m.word(a+0x24,0xffffffff);}
 for(unsigned i=0;i<11;++i)m.word(a+0x34+4*i,0);
 const int laps=m.signed_byte(racing+9);m.word(a+0x60,std::uint32_t(laps));for(int i=1;i<=laps&&i<=10;++i)m.word(a+0x34+4u*std::uint32_t(i),m.word(racing+0x88+4u*std::uint32_t(i)));
}
// Debug00185320; E-0110-F02. Original source order and count publication.
void TransientStats::Construct(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race){
 m.word(a,0);for(int i=0;i<m.signed_word(race+0x18);++i){TransientStatsElement::Construct(m,a+12+100u*std::uint32_t(i),m.word(race+0x624+4u*std::uint32_t(i)));m.word(a,m.word(a)+1);}
}
// Debug001858B8; E-0110-F02. Original options -> player records/points ->
// transient stats -> event results -> task/point rewards order. This consumer
// is not an outcome transaction; caller/lifetime must prevent duplicate delivery.
void FrontendDatabase::TheRaceIsOver(::port::driving::Memory& m,std::uint32_t a,std::uint32_t race){
 GetOptionsSetFromRace(m,a);
 for(int i=0;i<m.signed_word(race+0x18);++i){const auto racing=m.word(race+0x624+4u*std::uint32_t(i));if(RacingCar(m,racing).GetPlayer()){if(m.signed_word(0x2f3890+0x1072c)!=FRED_CHALLENGE_GAME_BE_A_COP)HighScoreTable::MaybeRecordHighScore(m,a+0x6b4,race,racing);AwardPointsToPlayer(m,a,race,racing);}}
 TransientStats::Construct(m,a+0x388,race);
 if(UseTheRaceEvent(m,a))RaceEvent::PostResults(m,0x312cd0);
 CheckForTaskAndPointRewards(m,race);
}
// Debug00182780/001827B8/00185A50; E-0110-F04. Note Fred.IsMultiTrack
// reads RaceType4/5, distinct from RaceEvent's original event-type3/4 test.
bool FrontendDatabase::IsThereAnotherRace(::port::driving::Memory& m,std::uint32_t a){return UseTheRaceEvent(m,a)&&RaceEvent::IsThereAnotherRace(m,0x312cd0);}
bool FrontendDatabase::IsMultiTrack(const ::port::driving::Memory& m,std::uint32_t){const int type=m.signed_word(0x311f98);return type>3&&type<6;}
// Debug001827E0, E-0101-RA02. Active postrace continuation deliberately does
// not enter StartRace/SetupCarsForRace or reseed the shared random stream.
bool FrontendDatabase::SetupNextRace(::port::driving::Memory& m,std::uint32_t a){return UseTheRaceEvent(m,a)&&RaceEvent::SetupNextRace(m,0x312cd0);}
void FrontendDatabase::TheRaceWasRestarted(::port::driving::Memory& m,std::uint32_t a){if(UseTheRaceEvent(m,a))RaceEvent::RestartRace(m,0x312cd0);}
// Debug001859F0, E-0110-F04. Original pause-menu retirement preserves the
// distinction from full race completion: no scores or transient finish rows.
void FrontendDatabase::TheRaceWasQuit(::port::driving::Memory& m,std::uint32_t a){
 CheckForAbortion(m,a);GetOptionsSetFromRace(m,a);
 if(UseTheRaceEvent(m,a)){RaceEvent::PostResults(m,0x312cd0);RaceEvent::MaybeDiscardResult(m,0x312cd0);}
}
}

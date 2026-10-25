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

#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
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

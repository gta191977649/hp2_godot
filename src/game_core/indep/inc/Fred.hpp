#pragma once
// TASK-0098 E-0098-S01. Debug NFSHP2.ELF SHA815b60c7...332f1b.
// Recovered ../indep/inc/Fred.hpp names; explicit host storage, NOT PS2 ABI.
#include "CarInfo.hpp"
#include "EventTree.hpp"
namespace hp2::debug {
enum eCarStatus { eCAR_UNLOCKED_FULLY_VISIBLE=0,eCAR_LOCKED_FULLY_VISIBLE=1,
    eCAR_LOCKED_PARTLY_VISIBLE=2,eCAR_LOCKED_INVISIBLE=3,eCAR_LOCKED_CLOAKED=4 };
struct RideInfo {
    int Type=0;
    std::uint8_t InstanceIndex=0,SkinNumber=0,HasDash=0,CanBeVertexDamaged=0;
};
struct CarStatus {
    eCarStatus nStatus=eCAR_LOCKED_INVISIBLE;
    std::array<std::uint8_t,10> aSkins{};
    int nSkins=0;
    bool bDashAvailable=false;
    void AddAllSkins(const CarTypeInfo&,int language);
};
// Partial decoded settings. Other profile fields remain in existing Retail storage.
struct cPlayerSettings { std::uint8_t DrivingStyle=0,TransmissionType=0; };
struct FrontendPlayerInfo {
    std::array<CarStatus,64> aCarStatus{};
    std::array<RideInfo,11> RideInfoByGameMode{};
    cPlayerSettings mPlayerSettings;
    void FlushAvailableCars();
    CarStatus& GetCarStatus(int type);
    void SetCarAvailability(eCarStatus,int type,int skin_number,bool dash_available,
                            const std::array<CarTypeInfo,64>&,int language);
    void BuildRideInfo(RideInfo&,int type,std::uint32_t skin_number,int use_dash_if_available,
                       const std::array<CarTypeInfo,64>&) const;
    void AssignCarOfType(int game_type,int type,std::uint32_t skin_number,int use_dash_if_available,
                         const std::array<CarTypeInfo,64>&);
    RideInfo& GetSelectedRideInfo(int game_type);
};
struct FrontendDatabase {
    std::array<FrontendPlayerInfo,2> FrontendPlayers{};
    int GameType=0,NumPlayerCars=1;
    void MakeEventCarsAvailable(const EventTree&,const EventInfo&,
                                const std::array<CarTypeInfo,64>&,int language);
};
}

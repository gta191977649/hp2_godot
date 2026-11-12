#pragma once
// Port companion declarations; original implementation source is copmgr.cpp.
#include "game_core/port/race/world_construction.hpp"
namespace port::race {struct CarConstructionServices;struct PhysicsConstructionServices;struct CarResetServices;}
namespace port::race {struct SnapshotSpeedTrapPointer{std::uint32_t address;};struct SnapshotPursuitPointer{std::uint32_t address;};}
namespace port::race {struct SnapshotRoadBlockPointer{std::uint32_t address;};struct SnapshotCopPointer{std::uint32_t address;};}
namespace hp2::debug {
class tCopManager {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    // Host view only; call Sub_0021DE80 for original construction.
    tCopManager(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug0021DE80, E-0099-W11. Required loaded-data/speech callees do not
    // default to success. This is not a complete police gameplay implementation.
    void Sub_0021DE80(const ::port::race::CopConstructionServices&);
    // Debug0021E270, E-0099-W17. Constructed pre-simulation manager teardown;
    // active pursuit/cop/spike virtual destruction remains explicitly guarded.
    void Sub_0021E270(int in_chrg);
    // Debug00220738/00221218, E-0099-W17. Actual source records/routes/topology.
    void InitRoadBlockStructures(const ::port::race::CopConstructionServices&);
    void InitSpeedTrapStructures(const ::port::race::CopConstructionServices&);
    // Debug0021F1E0/0021FE30/00223368, E-0099-W11.
    float GetBaseSpeedTrapSpawnTime();
    std::uint32_t ReserveCopSpeaker();
    std::uint32_t GetCurrentCopSpeaker(); //0021FD60,E-0100-PA01.
    void NextCopSpeaker(); //0021FDC8,E-0100-PA01.
    std::int8_t GetNextPursuitID(); //0021ED28,E-0100-PA01.
    bool RequestPursuit(std::uint32_t cop,std::uint32_t target,::port::race::PhysicsConstructionServices* =nullptr); //0021F8F0,E-0100-PA01.
    void PossibleCallSupervisor(std::uint32_t pursuit,bool disable_speech=false); //0021FC60,E-0100-PA01.
    void ReleaseCopSpeaker(std::uint32_t); //0021FE88,W119.
    std::uint32_t CreatePursuit(std::int8_t id,std::uint32_t car); //0021ED68,W119.
    void DeletePursuit(std::uint32_t); //0021EDE8,W119.
    std::uint32_t GetArrestingOfficer(std::uint32_t pursuit); //E-0112-CAP01.
    void CreatePullover(std::uint32_t pursuit,std::uint32_t car,bool,::port::race::PhysicsConstructionServices* =nullptr);
    void RemoveSpeedTrapsForCar(std::uint32_t); //E-0112-CAP01.
    bool IsChopperAvailable()const; //0021F748,E-0112-ESC01.
    std::uint32_t RequestChopper(); //0021F778.
    std::uint32_t RequestRoadBlock(std::uint32_t pursuit,float direction,::port::race::PhysicsConstructionServices* =nullptr); //00220B08.
    void InitRoadClosures();
    // Debug00221B28/00221EE0, E-0099-W91; actual constructor resource operands.
    void CreateCop(std::uint32_t,bool,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&);
    void CalculateCopCarDimensions();
    std::uint32_t IsBeingPursued(std::uint32_t car); //00221C78, E-0099-W53.
    int GetNumActiveRoadBlocks()const; //002211D8, E-0099-W47.
    int GetNumTickets(std::uint32_t car)const; //00222DA8,E-0110-F02.
    std::uint32_t GetActiveRoadBlock(int index)const; //00220AA8, E-0099-W47.
    void MonitorImpact(std::uint32_t car,float magnitude,bool two_car); //00222BA8, E-0099-W38.
    std::uint32_t GetCopByCar(std::uint32_t car); //0021F7E8; E-0099-W50.
    std::uint32_t GetCopByChase(std::uint32_t car); //0021F840; E-0099-W50.
    // E-0099-W61: actual frame order and source timer/list prefixes. Active
    // pursuit/spike/spawn/cop child boundaries remain explicit at their calls.
    void DoSnapshot(std::uint32_t snapshot); //0021E3F8, E-0099-W76. Active children remain explicit boundaries.
    void DoPointerSnapshot(std::uint32_t,::port::race::SnapshotSpeedTrapPointer); //0021EC80, W107.
    void DoPointerSnapshot(std::uint32_t,::port::race::SnapshotPursuitPointer); //0021EE40, W107.
    void DoPointerSnapshot(std::uint32_t,::port::race::SnapshotRoadBlockPointer); //0021EBE0,W119.
    void DoPointerSnapshot(std::uint32_t,::port::race::SnapshotCopPointer); //0021EED0,W119.
    void Think(::port::race::PhysicsConstructionServices* =nullptr,const ::port::race::CarResetServices* =nullptr); //0021EF58; W96 actual cop resource context
    void TimeBasedSpeech(); //0021FED8
    void SupervisorActivate(); //0021FB80
    void PursuitManagement(::port::race::PhysicsConstructionServices* =nullptr); //00220440,E-0100-PU02.
    bool IsCopNearby(std::uint32_t,float); //002205A0, ordinary fleet path.
    bool DidRecentlyCollideWithCop(std::uint32_t); //00222F40,E-0100-PU03.
    void SpawnSpeedTraps(::port::race::PhysicsConstructionServices* =nullptr,const ::port::race::CarResetServices* =nullptr); //0021F2A8,E-0110-P01.
    std::uint32_t FindCopForSpeedTrap(); //0021F0C0,E-0110-P01.
    std::uint32_t GetSpeedTrapFromLoadedDataIndex(int); //00221420,E-0110-P01.
    void ResetAllDamage(std::uint32_t car); //00222FD8,E-0110-P01.
    bool ShouldDisplayBustedText()const; //00223280,E-0110-P01.
    void ReclaimPatrollers(); //0021F688
    void MonitorSpikeBelts(); //00221F98
    void EndPullover(int,const ::port::race::CarResetServices* =nullptr); //002200F0,W121.
    void MonitorPullovers(const ::port::race::CarResetServices* =nullptr); //002216E0
    void RenderDebugInfo(); //00222480, actual empty body
    void LookForDanglyPointers(int); //0021ED20, actual empty body
    std::uint32_t FindPlayerPursuit(); //00220058
};
// Debug002225B0, E-0099-W91; source free wrapper.
void AddCopCarToRace(::port::driving::Memory&,std::uint32_t,bool,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&);
void SnapshotCops(::port::driving::Memory&,std::uint32_t snapshot); //00222510 E-0099-W76.
// Port callback bridge for the original pullover/NIS callback context.
void EndPulloverBridge(::port::driving::Memory&,int,const ::port::race::CarResetServices* =nullptr);
void GlobalCopThink(::port::driving::Memory&,float,::port::race::PhysicsConstructionServices* =nullptr,const ::port::race::CarResetServices* =nullptr); //00222530 E-0099-W61
std::uint32_t FindLandmark(::port::driving::Memory&,const std::array<float,2>&); //002232E0 E-0099-W61
}

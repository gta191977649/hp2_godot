// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/RaceCoordinator.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/snapshot_services.hpp"
namespace port::race {struct CarConstructionServices;struct PhysicsConstructionServices;struct AiDriveTargetServices;struct RaceStartOperands;class CarSoundResources;}
namespace port::race {struct CoordinatorConstructionServices;}
namespace hp2::debug {
class RaceCoordinator : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_001495B0(::port::race::CoordinatorConstructionServices&); //W62 original constructor.
 void Sub_001496C8(int flags,::port::race::CoordinatorConstructionServices&); //Original destructor.
 void CleanupAfterRace(); //001499F8, construction/prestart components.
 void CleanupPlayers(); //00149BE0.
 void CleanUpAutoReplay(); //0014BE48, null/base/DriveCamera cleanup component.
 void DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& ={}); //00149768, W74.
 void InitForRace(::port::race::CoordinatorConstructionServices&); //00149800.
 void PostMessage(int msg,int param); //00149550.
 void SendMessage(int msg,int param,::port::race::CoordinatorConstructionServices&); //001494C8.
 void ExecuteQueuedMessages(::port::race::CoordinatorConstructionServices&); //001493F0.
 void ChangeState(int state,::port::race::CoordinatorConstructionServices&); //00149398.
 int RCStateNoneCallback(int msg,int param,::port::race::CoordinatorConstructionServices&); //0014A4A8,E-0101-RC01; non-demo transition requests/restart.
 int RCStateRaceInProgressCallback(int msg,int param,::port::race::CoordinatorConstructionServices&); //0014AC60.
 int RCStatePauseCallback(int msg,int param,::port::race::CoordinatorConstructionServices&); //0014B1B8,E-0112-P02.
 int RCStatePostRaceCallback(int msg,int param,::port::race::CoordinatorConstructionServices&); //0014BBA8,E-0110-F04/E-0101-RC01; source result actions, explicit minimal presentation.
 void TheRaceHasFinished(::port::race::CoordinatorConstructionServices&); //0014BAB0,E-0110-F04; mandatory native data, strict presentation guard.
 void DisplayCountdownNumber(int); //0014B040.
 // Debug0014A430, E-0099-W34. Actual Car unlink/destruction; live Chopper
 // destruction remains explicit until its original owner is reconstructed.
 void CleanupCars();
 void InitCars(const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&,
               const ::port::race::AiDriveTargetServices&,const ::port::race::CarSoundResources&,
               const ::port::race::RaceStartOperands&); //0014A128, E-0099-W56.
 void InitPlayers(); //00149B28, W56; calls actual W55 Player.InitForRace.
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class RaceCoordinator {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

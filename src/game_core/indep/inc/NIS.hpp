// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/NIS.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class NisManager {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
#include <string>
#include <array>
namespace hp2::debug {
void Sub_001B1100(::port::driving::Memory&); //E-0099-W51, original static initializer.
class NisManager:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001AE5C0(const std::string&); //Original constructor, explicit lifetime.
 void Sub_001AE6F0(int); //Original constructed-state destructor; loaded ResourceFile consumer remains explicit.
 void InitSoundEvents(); //001AE5B0, instruction-proven empty.
 int AllowDebugLooping()const; //001AE9C8, W69; instruction-proven return0.
 int GetCurrentNisNumber()const; //001AEC68,E-0100-PA01.
 void DoSnapshot(std::uint32_t snapshot); //Debug001AE7D8, E-0099-W74.
 void StopAnimating(); //001AFF20.
 void Load(int); //001AEB18, actual retained BIN/CAM ownership.
 void NisLoadedCallback(); //001AEAF0.
 void EnsureNisIsLoaded(); //001AEC10.
 void Start(int,int,std::uint32_t,std::uint32_t); //001AE8F8.
 void Stop(); //001AE968.
 void SetupCarsBeforeNIS();void RestoreCarsAfterNIS(); //001AFC18/001AFCC0.
 void StartAnimating(); //001AFD40.
 void SetupForRendering(); //001AE9D0.
 int GetFrameNumber()const; //001B0118.
 void PlaySoundEvents(int); //001AE5B8, original empty.
 std::uint32_t CreateNisCameraCoordinator(int); //001AFFA0.
 void DeleteNisCameraCoordinator(std::uint32_t); //001B0038.
 void EnableNisCameraCoordinator(std::uint32_t);void DisableNisCameraCoordinator(std::uint32_t); //001B00C8/001B0078.
 void EnableOverrideRendering(int,std::uint32_t,std::uint32_t,std::uint32_t); //001AF220.
 void DisableOverrideRendering(int,std::uint32_t,std::uint32_t); //001AF9E8.
 void SetOverrideViews(int,std::uint32_t,std::uint32_t,bool); //001AFA48.
 bool AnythingInTheWay(std::uint32_t,std::uint32_t); //001AEE28.
 void ComputePossiblePlayerCarPosition(std::uint32_t,std::array<float,4>&,std::uint32_t,std::uint16_t,float,float); //001AF020.
 void ComputePossibleOtherCarPosition(std::array<float,4>&,std::uint32_t,const std::array<float,4>&,std::uint16_t,float,std::uint16_t); //001AF0D0.

};
}

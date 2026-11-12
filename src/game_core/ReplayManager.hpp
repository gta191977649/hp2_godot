// TASK-0046: Debug0013E070 is canonical; E-0046-C01. Historical template
// provenance below does not describe this typed Debug member or its PS2 ABI.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/snapshot_services.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayManager::BeginFrame() [0x002AEF30]
// Diagnostic text (not a callable signature): ReplayManager::HandleSnapshot() [0x002AEFF8]
namespace port::race {struct ReplayMemoryServices;}
namespace hp2::recovered {
class ReplayManager : public ::port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    //Debug0013D248/0013D3C0, E-0099-W41. Explicit original lifetime, including
    //demo list/borrowed-repeat transfer; constructor does not publish3271A8.
    void Sub_0013D248();
    void Sub_0013D3C0(int in_chrg);
    std::uint32_t GetData(int bits,int channel); //Debug0013E978; active repeat remains explicit.
    void AddData(std::int32_t,int bits,int channel); //Debug0013EA10.
    void BeginFrame(const ::port::race::SnapshotServices& ={}); //0013D680,W74.
    void EndFrame(); //0013DE08.
    bool WasSnapshotTakenAtThisTime(int); //0013D7B8.
    std::uint32_t FindSnapshot(const char* name,int number); //0013E3B8; host C-string operand binding.
    void DeleteSnapshot(std::uint32_t snapshot); //0013E5C8.
    int DeleteSnapshot(const char* name); //0013E580.
    std::uint32_t TakeSnapshot(const char* name,const ::port::race::SnapshotServices& ={}); //0013E7E0.
    int RestoreSnapshot(const char* name,const ::port::race::SnapshotServices& ={}); //0013E538,W74.
    void RestoreSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& ={}); //0013E928.
    void HandleSnapshot(std::uint32_t,const ::port::race::SnapshotServices& ={}); //0013E5F8.
    void DoEndSnapshot(std::uint32_t); //0013E6C8.
    void MaybeTakeDummySnapshot(const ::port::race::SnapshotServices&); //0013D820.
    void MaybeTakeInstantReplaySnapshot(const ::port::race::SnapshotServices&); //0013D8F0.
    void MaybeTakeLapSnapshot(const ::port::race::SnapshotServices&); //0013DCD8.
    void MaybeDoChecksumSnapshot(const ::port::race::SnapshotServices&); //0013DB00.
    void DoChecksumSnapshot(const char* name,const ::port::race::SnapshotServices&); //0013DB88.
    void AddSpecialEvent(int,int,int); //0013E1E8,E-0112-P03.
    void HandleSpecialEvent(std::uint32_t info); //0013E298 thunk->0016F4D8, E-0099-W81.
    void CheckForSpecialEvents(); //0013E2A0; E-0099-W81 positive source event/time scan.
    void MarkTheEndOfTime(); //Debug0013D668.
    void MarkTheBeginningOfTime(const ::port::race::ReplayMemoryServices&); //Debug0013D4F8.
    std::int32_t GetSignedData(int bits,int channel); //Debug0013E9D8: logical shift, despite name.
    // Debug0013E070; E-0046-C01; replay.cpp. No host ABI claim.
    void RegisterInterestingEvent(float interesting_value,const char* name);
    void InvalidateInterestingEvents(); //0013E0F0,W122.
};
} // namespace hp2::recovered

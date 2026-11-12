// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Replay.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class ReplaySnapshot {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0013CD68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013cd68.md.
    // Original Debug reference: ReplaySnapshot::GetChecksum(void *, int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetChecksum();
    // ELF 0x0013CE78; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013ce78.md.
    // Original Debug reference: ReplaySnapshot::ReadData(void *, int, int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ReadData();
};
} // namespace hp2::recovered

// TASK-0099 E-0099-W74. Explicit logical Debug snapshot view; no native C++ ABI.
// Original fields: timers+28/+2C; capture/restore/checksum/verify bytes+3C..3F;
// depth+40, overflow+41, sectionNumber+42, dataPosition+150,size+154,data+158.
#include "game_core/port/driving/object_view.hpp"
namespace port::race {
// Port operand carrier disambiguates original pointer-type MoveData overloads.
struct SnapshotMatrix {std::uint32_t address;};
}
namespace hp2::debug {
// Debug enum recovered verbatim; E-0099-W81, type-eReplaySpecialEvent.json.
enum eReplaySpecialEvent {REPLAY_SPECIAL_EVENT_FORCE_RACE_FINISH=0,REPLAY_SPECIAL_EVENT_DRIVER_MODE=1,REPLAY_SPECIAL_EVENT_DAMAGE_MODE=2,REPLAY_SPECIAL_EVENT_TRANSMISSION_TYPE=3,REPLAY_SPECIAL_EVENT_JUMP_CAM=4,REPLAY_SPECIAL_EVENT_ZONE_CAM=5,NUM_REPLAY_SPECIAL_EVENTS=6};
class ReplaySnapshot:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Init(std::uint32_t name); //0013CAE8; original logical C-string operand.
 std::uint32_t GetChecksum(std::uint32_t pdata,int data_size,int memory_size)const; //0013CB68.
 void MoveData(std::uint32_t pdata,int data_size,int memory_size=-1,int sign_extend=0); //0013CD08.
 void MoveData(::port::race::SnapshotMatrix matrix,int data_size); //0013CF00; data_size unused by source.
 void BeginSection(std::uint32_t object,std::uint32_t name); //0013D088.
 void BeginSection(std::uint32_t name){BeginSection(0,name);} //Original header overload emitted001369F8.
 void EndSection(); //0013D158.
 void BeginNamedSection(std::uint32_t name){BeginSection(0,name);} //0013EB20 original inline.
 void EndNamedSection(){EndSection();} //0013EB18 original inline.
 void VerifyData(std::uint32_t pdata,int data_size,int memory_size=-1,int sign_extend=0); //0013CED0.
 void ReadData(std::uint32_t pdata,int data_size,int memory_size,int sign_extend);
 void WriteData(std::uint32_t pdata,int data_size,int memory_size,int sign_extend);
 void VerifyMoveDataInSync(std::uint32_t section_name,std::uint32_t func_name);
};
}

#include "game_core/port/race/snapshot_services.hpp"
namespace hp2::debug {
class DemoModeReplay:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void ServiceRecording(bool took_snapshot,const ::port::race::SnapshotServices& ={}); //0013F2F0,W74; active file operations explicit.
};
}

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/debug_transport.hpp"
namespace hp2::debug {
// E-0099-W41, Debug NFSHP2.ELF815b...332f1b. Original brepeat.cpp;
// explicit logical storage including the variable-length trailing stream.
class bRepeat : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot,bool add_data_position); //0013C990,W74.
 void PrepareToAddData(int bytes,int channels,int enable_codeine);
 void PrepareToRemoveData();
 void TellCodeineWeAreFinishedRemovingData(const ::port::race::MonitorSyscall& ={}); //001E7AC0, E-0099-W49.
 void WriteToLocation(std::int32_t location,std::int32_t data,int bits);
 std::int32_t ReadFromLocation(std::int32_t location,int bits)const;
 int AddData(std::int32_t,int bits,int channel,const ::port::race::MonitorSyscall& monitor={});
 std::int32_t RemoveData(int bits,int channel);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class bRepeat {
public:
    // ELF 0x001EB050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb050.md.
    // Original Debug reference: bRepeat::WriteToLocation(int, int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void WriteToLocation();
    // ELF 0x001EB0C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb0c0.md.
    // Original Debug reference: bRepeat::ReadFromLocation(int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ReadFromLocation();
    // ELF 0x001EB498; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eb498.md.
    // Original Debug reference: bRepeat::PrepareToRemoveData(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PrepareToRemoveData();
};
} // namespace hp2::recovered

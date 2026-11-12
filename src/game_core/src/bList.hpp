// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include "game_core/bWare/inc/bList.hpp"
namespace hp2::debug {
// Debug00249A28/00249A60 and00249A88/00249AB8; E-0099-W34.
void bPListInit(::port::driving::Memory&,int expected_nodes);
void bPListClose(::port::driving::Memory&);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class bList {
public:
    // ELF 0x002503F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002503f8.md.
    // Original Debug reference: bList::GetNode(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetNode();
    // ELF 0x00250428; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/containers/00250428.md.
    // Original Debug reference: bList::TraversebList(bNode *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void TraversebList();
    // ELF 0x00250458; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/containers/00250458.md.
    // Original Debug reference: bList::AddTail(bList *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AddTail();
};
} // namespace hp2::recovered
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class bList : public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 // Debug002498C0, E-0099-W38. Comparator is an original function address;
 // unknown reached comparators remain explicit gaps.
 std::uint32_t AddSorted(std::uint32_t comparator,std::uint32_t node);
};
}

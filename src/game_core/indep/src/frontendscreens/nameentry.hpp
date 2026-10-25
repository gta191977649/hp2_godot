// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/indep/src/frontendscreens/options.hpp"
#include <array>

namespace port::frontend {
// Host storage for Retail0024D408/0024C458. No original ABI/layout claim.
struct NameEntryState {
    std::array<std::array<char,12>,2> names{},initial_names{};
    std::array<std::uint8_t,2> flags{},initial_flags{};
    bool active=false,uppercase=false;
    int player=0,row=0,column=0,cursor=0;
    std::array<char,8> buffer{};
    ScalarPath grid_x,grid_y,panel_x;
    float panel_authored_x=0,panel_authored_y=0,caret_phase=0;
};
}
namespace hp2::frontend {
// Retail0024C708/0024D088/0024CC00/0024CCB0, E-0095-N01.
void Sub_0024C708(::port::frontend::NameEntryState&,int player);
// Returns false only when keyboard commits/closes. Event contains original sound.
bool Sub_0024D088(::port::frontend::NameEntryState&,int message,MenuEvent_0016A458&);
// Retail0024D660, E-0095-N01; caller passes only gender row messages6/7.
void Sub_0024D660(::port::frontend::NameEntryState&,int message,MenuEvent_0016A458&);
// Retail0024D370, E-0095-N01, compares initial names and flags.
bool Sub_0024D370(const ::port::frontend::NameEntryState&);
// Retail0024D408/0024C5D0/0024CF50/0024CD40, E-0095-N02.
void Sub_0024D408(::port::frontend::NameEntryState&,FeScreen&,
    const std::function<std::string(std::uint32_t)>&);
// Retail0024C948/0024CA60, E-0095-N03. Caller detects original wrap snap.
void Sub_0024C948(::port::frontend::NameEntryState&,FeScreen&,const WidgetMeasure&,bool snap);
void Sub_0024CA60(::port::frontend::NameEntryState&,FeScreen&,float delta,
    const WidgetMeasure&,const std::function<std::string(std::uint32_t)>&);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tKeyboard {
public:
    // ELF 0x0024D048; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024d048.md.
    // Original Debug reference: tKeyboard::PlayerRight(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PlayerRight();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/raceparameters.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001EDC50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001edc50.md.
void RaceParameters::ClearEliminatedDrivers() {
    hp2::not_implemented("0x001EDC50", "RaceParameters::ClearEliminatedDrivers (signature and full body unresolved)");
}

// ELF 0x001EDCD0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001edcd0.md.
void RaceParameters::GetNumEliminated() {
    hp2::not_implemented("0x001EDCD0", "RaceParameters::GetNumEliminated (signature and full body unresolved)");
}

// ELF 0x001EDDB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eddb8.md.
void RaceParameters::GetLocationNumber() {
    hp2::not_implemented("0x001EDDB8", "RaceParameters::GetLocationNumber (signature and full body unresolved)");
}

// ELF 0x001EE390; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee390.md.
void RaceParameters::IsFreeRun() {
    hp2::not_implemented("0x001EE390", "RaceParameters::IsFreeRun (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001EE1F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee1f8.md.
void cPlayerSettings::ToggleGauges() {
    hp2::not_implemented("0x001EE1F8", "cPlayerSettings::ToggleGauges (signature and full body unresolved)");
}

// ELF 0x001EE230; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee230.md.
void cPlayerSettings::ToggleLapInfo() {
    hp2::not_implemented("0x001EE230", "cPlayerSettings::ToggleLapInfo (signature and full body unresolved)");
}

// ELF 0x001EE258; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee258.md.
void cPlayerSettings::CycleSplitTime() {
    hp2::not_implemented("0x001EE258", "cPlayerSettings::CycleSplitTime (signature and full body unresolved)");
}

// ELF 0x001EE2B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee2b0.md.
void cPlayerSettings::ToggleRacePosition() {
    hp2::not_implemented("0x001EE2B0", "cPlayerSettings::ToggleRacePosition (signature and full body unresolved)");
}

// ELF 0x001EE2D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee2d8.md.
void cPlayerSettings::ToggleLookback() {
    hp2::not_implemented("0x001EE2D8", "cPlayerSettings::ToggleLookback (signature and full body unresolved)");
}

// ELF 0x001EE300; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee300.md.
void cPlayerSettings::CycleAlternateCamera() {
    hp2::not_implemented("0x001EE300", "cPlayerSettings::CycleAlternateCamera (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001EE120; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee120.md.
void tGlobalDisplaySettings::CycleLeaderboardMode() {
    hp2::not_implemented("0x001EE120", "tGlobalDisplaySettings::CycleLeaderboardMode (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Fred.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/src/bMath.hpp"
#include <array>
#include <algorithm>

namespace port::race {
// Port safety around the recovered valid fourteen-entry logical table domain.
void require_driver_count(int count,const char* address){
    if(count<0 || count>14)hp2::not_implemented(address,"DriverInfo count outside original table domain");
}
void copy_driver(::port::driving::Memory& memory,std::uint32_t destination,std::uint32_t source){
    std::array<std::uint8_t,24> value{};
    for(unsigned i=0;i<value.size();++i)value[i]=memory.byte(source+i);
    for(unsigned i=0;i<value.size();++i)memory.byte(destination+i,value[i]);
}
}
namespace hp2::debug {
// Debug001EA330, E-0098-RP01. Original defaults do not clear trailing PlayerName bytes.
void cPlayerSettings::ResetToDefaults(::port::driving::Memory& m,std::uint32_t a,
    const ::port::race::DefaultSettings& globals){
    m.byte(a,0);m.byte(a+1,1);m.byte(a+2,0);m.byte(a+3,0);
    m.byte(a+4,globals.skip_frontend?globals.pov_index:1);
    m.byte(a+5,0);m.byte(a+6,0);m.byte(a+7,0);
    m.word(a+8,0);m.word(a+12,1);m.word(a+16,0x355);
    for(unsigned i=0;i<12;++i){const auto ch=m.byte(0x328690+i);m.byte(a+20+i,ch);
        if(!ch)break;
        if(i==11)hp2::not_implemented("0x001EA330","original PLAYER string exceeds PlayerName domain");}
    m.word(a+44,1);m.word(a+40,1);m.word(a+32,0);m.word(a+36,0);
}
// Debug001EA440, E-0098-RP01. Current language predicate is explicitly supplied.
void tGlobalDisplaySettings::ResetToDefaults(::port::driving::Memory& m,std::uint32_t a,
    const ::port::race::DefaultSettings& globals){
    m.byte(a,globals.skip_frontend?globals.rear_view:1);
    m.byte(a+1,globals.skip_frontend?globals.speedo_mode:!globals.english_language);
    m.byte(a+3,7);m.byte(a+4,8);m.byte(a+5,2);m.byte(a+2,0);
    m.byte(a+6,globals.skip_frontend?globals.jump_camera:1);
    m.byte(a+7,globals.skip_frontend?globals.zone_camera:1);
}
// Debug001E9968, E-0098-RP01. Called at frontend/main-menu baseline, NOT each EventTree::SetRace.
void RaceParameters::InitWithDefaults(const ::port::race::DefaultSettings& globals){
    auto& m=memory_;const auto a=address_;
    for(unsigned i=0;i<0x310;++i)m.byte(a+i,0);
    m.word(a,11);m.word(a+8,3);m.scalar(a+0x10,1);m.scalar(a+0x14,.5f);
    m.word(a+0x2c,2);m.word(a+0x30,1);m.word(a+0x34,7);
    m.byte(a+0x3c,7);m.byte(a+0x3d,8);m.word(a+0x254,1);
    for(unsigned offset=0x260;offset<=0x270;offset+=4)m.word(a+offset,1);
    m.word(a+0x278,1);m.word(a+0x27c,2);m.word(a+0x2f8,1);
    m.scalar(a+0x2fc,1);m.scalar(a+0x300,1);
    for(unsigned i=0;i<0x150;++i)m.byte(0x312288+i,0);
    m.word(a+0x304,0x312288);m.word(a+0x308,0);
    cPlayerSettings::ResetToDefaults(m,a+0x28c,globals);
    cPlayerSettings::ResetToDefaults(m,a+0x2bc,globals);
    m.word(a+0x28,0);tGlobalDisplaySettings::ResetToDefaults(m,a+0x2ec,globals);
    m.word(a+0x4c,0);m.word(a+0x50,0);
    m.word(a+0x30c,hp2::recovered::bRandom(m,-1));
}
// Debug001E9DE0, E-0098-RP01. Full table is an original no-op, with no skin lookup.
void RaceParameters::AddDriverInfo(std::uint32_t driver,const ::port::race::SkinLookup& skin){
    auto& m=memory_;const int before=m.signed_word(address_+0x308);
    ::port::race::require_driver_count(before,"0x001E9DE0");if(before>=14)return;
    if(!skin)hp2::not_implemented("0x001E9DE0","original GetSkinInfo binding missing");
    skin(m.signed_word(driver+12),m.signed_byte(driver+17),1);
    const int count=m.signed_word(address_+0x308); //001E9E14 reloads after GetSkinInfo.
    if(count<0 || count>=14)hp2::not_implemented("0x001E9DE0","driver table count changed outside original valid append domain");
    ::port::race::copy_driver(m,m.word(address_+0x304)+24u*count,driver);
    m.word(address_+0x308,count+1);
}
// Debug001E9E78, E-0098-RP01. Set does not change count or invoke GetSkinInfo.
void RaceParameters::SetDriverInfo(std::uint32_t driver,int index){
    if(index>=14)return;
    if(index<0)hp2::not_implemented("0x001E9E78","negative original table index outside bounded host domain");
    ::port::race::copy_driver(memory_,GetDriverInfo(index),driver);
}
// Debug001E9EC8, E-0098-RP01. Original permits table slots beyond populated count.
std::uint32_t RaceParameters::GetDriverInfo(int index) const{
    if(index<0 || index>=14)hp2::not_implemented("0x001E9EC8","original DriverInfo pointer outside bounded table domain");
    return memory_.word(address_+0x304)+24u*index;
}
// Debug001E9EE0, E-0098-RP01. Signed UniqueDriverNumber comparison; first match.
std::uint32_t RaceParameters::GetDriverInfoByDriverNumber(int number) const{
    const int count=memory_.signed_word(address_+0x308);::port::race::require_driver_count(count,"0x001E9EE0");
    for(int i=0;i<count;++i){const auto driver=GetDriverInfo(i);if(memory_.signed_byte(driver+1)==number)return driver;}
    return 0;
}
// Debug001E9F68, E-0098-RP01. Rank narrowing and nonpositive-rank repeat behavior retained.
std::uint32_t RaceParameters::EliminateDriver(int number,int rank){
    auto& m=memory_;const auto driver=GetDriverInfoByDriverNumber(number);
    if(driver && m.signed_byte(driver)<1){
        m.byte(driver,static_cast<std::uint8_t>(rank));const auto type=m.word(driver+8);
        if(type==1)m.word(address_+0x30,m.word(address_+0x30)-1);
        else if(type==2)m.word(address_+0x34,m.word(address_+0x34)-1);
    }
    return driver;
}
// Debug001E9FF0, E-0098-RP01. Only positive eliminated ranks are restored.
void RaceParameters::ClearEliminatedDrivers(){
    auto& m=memory_;const int count=m.signed_word(address_+0x308);::port::race::require_driver_count(count,"0x001E9FF0");
    for(int i=0;i<count;++i){const auto driver=GetDriverInfo(i);if(m.signed_byte(driver)>0){
        const auto type=m.word(driver+8);
        if(type==1)m.word(address_+0x30,m.word(address_+0x30)+1);
        else if(type==2)m.word(address_+0x34,m.word(address_+0x34)+1);
        m.byte(driver,0);
    }}
}
// Debug001EA070, E-0098-RP01.
int RaceParameters::GetNumEliminated() const{
    const int count=memory_.signed_word(address_+0x308);::port::race::require_driver_count(count,"0x001EA070");int result=0;
    for(int i=0;i<count;++i)result+=memory_.signed_byte(GetDriverInfo(i))>0;
    return result;
}
// Debug001EA0B8, E-0098-RP01. Driver number, not driver type, selects player positions.
void RaceParameters::ResetStartingPositions(){
    auto& m=memory_;const auto ai=m.byte(address_+0x34);m.byte(address_+0x3c,std::uint8_t(ai+1));m.byte(address_+0x3d,std::uint8_t(ai+2));
    const int count=m.signed_word(address_+0x308);::port::race::require_driver_count(count,"0x001EA0B8");
    for(int i=0;i<count;++i){const auto driver=GetDriverInfo(i);const int number=m.signed_byte(driver+1);
        m.byte(driver+4,number>=0 && number<2?m.byte(address_+0x3c+number):0);}
}
// Debug001EA7A8, E-0098-RP01. BeACop overrides the zero-lap single-race condition.
bool RaceParameters::IsFreeRun() const{return memory_.word(address_+0x20)==1 && memory_.word(address_+0x2c)==0 && memory_.word(address_+0x274)==0;}
// Debug001EA158, E-0098-RP01. Signed division truncates toward zero.
int RaceParameters::GetLocationNumber() const{return memory_.signed_word(address_)/10;}
// Debug001EA178, E-0098-RP01. Location6 shares the fifth probability slot.
int RaceParameters::GetLocationIndex() const{const auto number=GetLocationNumber();return number==6?4:number-1;}
}

namespace hp2::debug {
// Debug001EA7A0, E-0098-RP05. Confirmed original empty function, not a missing HUD implementation.
void RaceParameters::ChooseHudTextures(){}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/include/bVoice.h"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0027C5C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c5c0.md.
void bVoice::Release() {
    hp2::not_implemented("0x0027C5C0", "bVoice::Release (signature and full body unresolved)");
}

// ELF 0x0027C5E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c5e8.md.
void bVoice::Set() {
    hp2::not_implemented("0x0027C5E8", "bVoice::Set (signature and full body unresolved)");
}

// ELF 0x0027C6F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c6f8.md.
void bVoice::Sub_0027C6F8() {
    hp2::not_implemented("0x0027C6F8", "bVoice::Sub_0027C6F8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// TASK-0099 E-0099-W40. Live MCP FDR/SYMR/source-marker/path corroborates
// original src/bVoice.cpp. Debug815b60c7... revision unknown.
// Debug00274550: leave original voice ID and padding untouched.
void bVoice::Release(){m_.word(address_+4,0);}
// Debug00274578, E-0099-W40.
void bVoice::Set(std::uint16_t id,std::uint32_t sample){m_.half(address_,id);m_.word(address_+4,sample);}
// Debug002745B8/002745F0, E-0099-W40.
std::uint32_t bVoice::GetSample()const{return m_.word(address_+4);}
std::uint16_t bVoice::GetId()const{return m_.half(address_);}
// Debug00272C90, E-0099-W40.48 embedded voices at actual global31ED48.
void bVoiceMgr::Initialize(::port::driving::Memory& m){for(unsigned i=0;i<48;++i)bVoice(m,0x31ed48+i*8).Release();}
// Debug00272D10, E-0099-W40. Duplicate-ID assertion is an explicit original
// debug assertion policy dependency; no replacement/steal is silently performed.
std::uint32_t bVoiceMgr::GetVoice(::port::driving::Memory& m,std::uint16_t id,std::uint32_t sample){
 for(unsigned i=0;i<48;++i){bVoice voice(m,0x31ed48+i*8);if(voice.GetSample()&&voice.GetId()==id)hp2::not_implemented("0x00272D10","original duplicate voice-ID bAssertFail policy is not bound");}
 for(unsigned i=0;i<48;++i){const auto at=0x31ed48+i*8;bVoice voice(m,at);if(!voice.GetSample()){voice.Set(id,sample);return at;}}
 return 0;
}
// Debug00272F28, E-0099-W40.
void bVoiceMgr::ReleaseVoice(::port::driving::Memory& m,std::uint32_t voice){bVoice(m,voice).Release();}
}

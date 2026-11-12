// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/src/bSound.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/fragmented_pointer_view.hpp"
#include "game_core/src/bRegion.hpp"

namespace hp2::debug {
// Debug0026FAB8, E-0099-W31. Region names use bStrCmp, not bStrICmp.
std::uint32_t bSound::Region(const std::string& name)const{
 for(std::uint64_t i=0;i<::port::race::fragmented_count(m_,address_+0x160);++i){const auto region=::port::race::fragmented_pointer(m_,address_+0x158,i);
  for(std::size_t j=0;;++j){const auto a=m_.byte(region+std::uint32_t(j)),b=j<name.size()?std::uint8_t(name[j]):std::uint8_t(0);if(a!=b)break;if(!a)return region;}}
 return 0;
}
}

namespace hp2::recovered {
// ELF 0x00277DC0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/audio/00277dc0.md.
void bSound::GetCategoryVolume() {
    hp2::not_implemented("0x00277DC0", "bSound::GetCategoryVolume (signature and full body unresolved)");
}

// ELF 0x00277EA0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00277ea0.md.
void bSound::GetStatus() {
    hp2::not_implemented("0x00277EA0", "bSound::GetStatus (signature and full body unresolved)");
}

// ELF 0x00278320; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00278320.md.
void bSound::EnablePrintf() {
    hp2::not_implemented("0x00278320", "bSound::EnablePrintf (signature and full body unresolved)");
}

// ELF 0x00278368; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00278368.md.
void bSound::DisablePrintf() {
    hp2::not_implemented("0x00278368", "bSound::DisablePrintf (signature and full body unresolved)");
}

// ELF 0x0027A6D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027a6d8.md.
void bSound::getPendingRegion() {
    hp2::not_implemented("0x0027A6D8", "bSound::getPendingRegion (signature and full body unresolved)");
}

// ELF 0x0027A720; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027a720.md.
void bSound::getPendingRegionsCount() {
    hp2::not_implemented("0x0027A720", "bSound::getPendingRegionsCount (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug0026FE38, E-0099-W40. Original out-of-range categories have full gain.
float bSound::GetCategoryVolume(std::uint32_t category)const{return category<16?m_.scalar(address_+12+category*4):1.0f;}
// Debug0026FF50, E-0099-W40. A nonnull original callback needs its real binding.
void bSound::notifyClient(std::uint32_t message,std::uint32_t object){if(m_.word(address_+76))hp2::not_implemented("0x0026FF50","original bSound client callback target is not bound");(void)message;(void)object;}
}

// E-0099-W62. Original bSound.Printf002702C8 channel gate; PC stdout binding
// only for caller-supplied no-varargs literals. Not the PS2 formatter/terminal.
#include <cstdio>
namespace port::race {
// PC output primitive: preserve bytes; terminal channel multiplexing is not
// reconstructed. The source bSound gate is applied before this call.
static void write_literal_diagnostic(std::uint8_t channel,const std::string& literal){
 (void)channel;
 if(std::fwrite(literal.data(),1,literal.size(),stdout)!=literal.size())throw std::runtime_error("PC diagnostic output failed");
}
}
namespace hp2::debug {
void bSound::Printf(std::uint32_t format)const{
 if(m_.word(address_)==0xffffffffu)return;
 std::string literal;for(unsigned i=0;;++i){if(i==4096)throw std::out_of_range("Unterminated original diagnostic literal");const auto c=m_.byte(format+i);if(!c)break;if(c=='%')hp2::not_implemented("0x002702C8","nonliteral original bSound printf format requires varargs formatter");literal.push_back(char(c));}
 ::port::race::write_literal_diagnostic(static_cast<std::uint8_t>(m_.word(address_)),literal);
}
}

#include "game_core/include/bVoice.h"
#include "game_core/src/bSample.hpp"
namespace hp2::debug {
// Debug0026FD28; E-0112-P03. Preserve all48 live source voice updates.
void bSound::SetCategoryVolume(std::uint32_t category,float volume){
 if(category>=16)return;m_.scalar(address_+12+category*4,volume);
 for(unsigned i=0;i<48;++i){const auto sample=bVoice(m_,0x31ed48+8*i).GetSample();if(sample&&m_.word(sample+0x84)==category)bSample(m_,sample).SetVolume(m_.scalar(sample+0x18));}
}
}

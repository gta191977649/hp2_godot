// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/language.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/fmenus.hpp"

namespace hp2::debug {
// TASK-0099 E-0099-W72. Actual source raw-language component. No decoded
// translation table/profile is published as original-memory state.
void LanguageManager::Sub_00218BB8(){m_.word(address_+4,0);m_.word(address_,0xffffffff);m_.word(address_+8,0);m_.word(address_+0x10,0);m_.word(address_+0x14,0);}
int LanguageManager::Loader(std::uint32_t chunk){if(m_.word(chunk)!=0x39000)return 0;const auto hashes=m_.word(chunk+0x54),ids=m_.word(chunk+0x58),eng=chunk+0x6c,id=eng+8*hashes,text=id+8*ids;m_.word(address_+4,chunk+8);m_.word(chunk+0x60,eng);m_.word(chunk+0x68,text);m_.word(chunk+0x64,id);return 1;}
int LanguageManager::Unloader(std::uint32_t chunk){if(m_.word(chunk)!=0x39000)return 0;m_.word(address_+4,0);return 1;}
// E-0112-H01: const logical-memory transport of the same original read-only lookup.
std::uint32_t LanguageManager::GetString(const ::port::driving::Memory& m,std::uint32_t address,int wanted,int requested,bool english_hash){(void)requested;const auto description=m.word(address+4);if(!description)return 0;const auto table=m.word(description+(english_hash?0x58:0x5c));int high=m.signed_word(description+0x50),low=0,index=high/2;
 // Original inclusive upper bound and its final probe are retained. This is
 // deliberately not replaced by the decoded asset reader's lower_bound.
 auto entry=table+std::uint32_t(index)*8;bool found=m.signed_word(entry)==wanted;
 if(!found){if(high>=0){do{const int key=m.signed_word(entry);if(key<wanted)low=index+1;else if(wanted<key)high=index-1;index=(low+high)/2;entry=table+std::uint32_t(index)*8;if(m.signed_word(entry)==wanted){found=true;break;}}while(low<=high);}if(!found&&m.signed_word(entry)==wanted)found=true;}
 return found?m.word(description+0x60)+m.word(table+std::uint32_t(index)*8+4):0;
}
std::uint32_t LanguageManager::GetString(int wanted,int requested,bool english_hash){return GetString(m_,address_,wanted,requested,english_hash);}
int LoaderLanguage(::port::driving::Memory& m,std::uint32_t chunk){return LanguageManager(m,0x313898).Loader(chunk)!=0;}
std::uint32_t GetTranslatedString(const ::port::driving::Memory& m,std::uint32_t english){std::string text;for(unsigned i=0;;++i){if(i>=4096)throw std::out_of_range("Unterminated original translation input");const auto c=m.byte(english+i);if(!c)break;text+=char(c);}const auto hash=hp2::frontend::bStringHash(text.c_str());const auto translated=LanguageManager::GetString(m,0x313898,std::int32_t(hash),m.signed_word(0x313898),true);return translated?translated:english;}
std::uint32_t GetTranslatedString(const ::port::driving::Memory& m,int id){const auto translated=LanguageManager::GetString(m,0x313898,id,m.signed_word(0x313898),false);return translated?translated:0x2b4518;}
}

namespace hp2::recovered {
// ELF 0x0021E2E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/0021e2e8.md.
void LanguageManager::GetString() {
    hp2::not_implemented("0x0021E2E8", "LanguageManager::GetString (signature and full body unresolved)");
}

// ELF 0x0021E3D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021e3d0.md.
void LanguageManager::Loader() {
    hp2::not_implemented("0x0021E3D0", "LanguageManager::Loader (signature and full body unresolved)");
}

// ELF 0x0021E428; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021e428.md.
void LanguageManager::Unloader() {
    hp2::not_implemented("0x0021E428", "LanguageManager::Unloader (signature and full body unresolved)");
}
} // namespace hp2::recovered

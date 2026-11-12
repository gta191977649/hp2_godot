#pragma once
// E-0099-W72: original ../indep/inc/Language.hpp marker corroborated through
// SetCurrLanguage00100828. Logical views are port-side, never PS2 host ABI.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class LanguageManager:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_00218BB8();
 void SetCurrLanguage(int language){m_.word(address_,std::uint32_t(language));} //00100828.
 int Loader(std::uint32_t); //00218FE0.
 int Unloader(std::uint32_t); //00219038.
 static std::uint32_t GetString(const ::port::driving::Memory&,std::uint32_t owner,int id,int requested_language,bool english_hash); //E-0112-H01 const transport; same source lookup.
 std::uint32_t GetString(int id,int requested_language,bool english_hash); //00218EF8; requested_language unconsumed.
};
int LoaderLanguage(::port::driving::Memory&,std::uint32_t); //002190C8.
std::uint32_t GetTranslatedString(const ::port::driving::Memory&,std::uint32_t english); //00219118.
std::uint32_t GetTranslatedString(const ::port::driving::Memory&,int id); //00219170.
}

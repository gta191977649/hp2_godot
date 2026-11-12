#pragma once
// TASK-0101 E-0101-PL01. Port asset metadata, not an original loader function.
// Debug e_Languages and the owned FE_*.BIN header+50 independently agree:
// l_EN0/l_FR1/l_DE2/l_IT3/l_ES4/l_SE5. No US/UK-English reinterpretation.
#include <stdexcept>
namespace port::runtime {
inline const char* event_language_asset(int language){
 static constexpr const char* files[]={"LANGUAGES/FE_ENGLISH.BIN","LANGUAGES/FE_FRENCH.BIN",
  "LANGUAGES/FE_GERMAN.BIN","LANGUAGES/FE_ITALIAN.BIN","LANGUAGES/FE_SPANISH.BIN","LANGUAGES/FE_SWEDISH.BIN"};
 if(language<0||language>=6)throw std::invalid_argument("Event language is outside the reviewed six selected frontend locales");
 return files[language];
}
}

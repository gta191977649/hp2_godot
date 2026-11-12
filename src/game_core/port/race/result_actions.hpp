#pragma once
#include "game_core/port/driving/memory.hpp"
#include "game_core/indep/inc/Fred.hpp"

namespace port::race {
// Explicit PC view of final EndGameMenu visibility, not an original class or
// coordinator ContinueRaceType. E-0101-RA01/RA02; Debug SHA815b60c7...332f1b.
struct ResultActionAvailability {
 bool next_race=false;
 bool restart_race=false;
 bool restart_event=false;
 bool quit=true;
};
inline ResultActionAvailability result_action_availability(::port::driving::Memory& memory,
                                                          std::uint32_t fred){
 using hp2::debug::FrontendDatabase;
 ResultActionAvailability out;
 // EndGameMenu.SeeNextRace001DD970; keep the source short circuit.
 out.next_race=FrontendDatabase::IsMultiTrack(memory,fred)&&FrontendDatabase::IsThereAnotherRace(memory,fred);
 // EndGameMenu.SeeRestartRace001DD9C0.
 const int type=memory.signed_word(fred+0x1072c);
 out.restart_race=(type!=hp2::debug::FRED_ULTIMATE_GAME&&type!=hp2::debug::FRED_HOTPURSUIT_GAME)||FrontendDatabase::IsMultiTrack(memory,fred);
 // EndGameMenu.SeeRestartEvent001DDA18 and SeeQuit001DDA70.
 out.restart_event=FrontendDatabase::IsMultiTrack(memory,fred)||FrontendDatabase::UseTheRaceEvent(memory,fred);
 return out;
}
}

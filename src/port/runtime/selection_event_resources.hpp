#pragma once
// TASK-0101 E-0101-PL01. Port prerequisite owner for the original raw menu
// SetRace. This does not call SetRace/StartRace or enable additional events.
#include "event_launch.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/race/frontend_language_resources.hpp"
#include "game_core/port/race/frontend_event_resources.hpp"
#include "game_core/port/track/texture_bindings.hpp"
namespace port::runtime {
class SelectionEventResources {
 driving::Memory& memory_;
 race::CarRenderResources cars_;
 race::FrontendLanguageResources language_;
 race::FrontendEventResources events_;
 track::TextureBindings textures_;
 bool loaded_=false;
 EventProfileLayout retail_{EventProfileVersion::Retail,{},{{0,0}}};
 EventProfileLayout debug_{EventProfileVersion::Debug,{},{{0,0}}};
 EventProfilePayload retail_baseline_;
public:
 explicit SelectionEventResources(driving::Memory&);
 SelectionEventResources(const SelectionEventResources&)=delete;
 SelectionEventResources& operator=(const SelectionEventResources&)=delete;
 // Inputs are the actual owned GLOBAL/FRONTB/language assets and explicit
 // both-tree profile. Original resource loaders run; profile fields are coded
 // individually. No whole-profile overlay and no source RNG draw.
 void load(const std::vector<std::uint8_t>& global,const std::vector<std::uint8_t>& front,
           const std::vector<std::uint8_t>& language,const race::EventProfileTransfer&);
 EventProfilePayload export_profile()const;
 void import_profile(const EventProfilePayload&);
 const race::FrontendEventResources& events()const{return events_;}
 // All raw RaceEvent borrowers must have ended. No destructor swallows an
 // untranslated original path; caller explicitly closes before Memory dies.
 void close();
};
}

#pragma once
// TASK-0110 port semantic transport to original authored-event producers.
#include "free_run_launch.hpp"
#include "game_core/port/race/frontend_event_resources.hpp"
namespace port::race {
void prepare_event_world_launch(driving::Memory&,WorldLaunchRequest&,const LaunchCatalog&,FrontendEventResources&);
std::vector<std::uint8_t> export_event_progress(const driving::Memory&,const EventLaunchRequest&,const FrontendEventResources&);
std::optional<::port::runtime::EventProfilePayload> export_event_profile(const driving::Memory&,const EventLaunchRequest&,const FrontendEventResources&);
std::optional<::port::runtime::EventScoreProfile> export_event_scores(const driving::Memory&,const EventLaunchRequest&);
std::optional<::port::runtime::EventSettingsTransfer> export_event_settings(const driving::Memory&,const EventLaunchRequest&);
}

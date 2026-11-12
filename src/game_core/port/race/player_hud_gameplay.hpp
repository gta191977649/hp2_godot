#pragma once
// TASK-0110 E-0110-F09. Port-only logical counterpart of original score text
// animation when Flailer presentation is explicitly disabled. Own in WorldImpl;
// reset on restart. Player score/strings/clean flags remain original Memory.

#include <optional>
#include <cstdint>
#include <functional>
#include "game_core/port/driving/memory.hpp"
namespace port::race {
// Semantic result of the PC presentation camera selecting the actual POV.
// No recovered pointer/enum/gauge bit is fabricated by this service.
struct PlayerHudCameraContext {bool selected_pov_available=false;bool outside_pov=false;int selected_pov_kind=0;};
// Explicit platform/resource seams for the original Player HUD. Each callback
// implements the named source callee; an absent reached callback is a guard.
struct PlayerHudPresentationServices {
 std::function<void(driving::Memory&,std::uint32_t,bool)> track_map; //00177E68 construction/deletion +001774B0 mode.
 std::function<void(driving::Memory&,std::uint32_t,bool)> mirror; //00174FF8.
 std::function<void(driving::Memory&,std::uint32_t,bool)> letterbox; //original eView.LetterBox field.
 std::function<void(driving::Memory&,std::uint32_t)> race_over; //0017FBB8 + RaceFinishedSpeech.
 std::function<void(driving::Memory&,std::uint32_t,bool)> nis_visibility; //00180FB0.
 std::function<void(driving::Memory&,std::uint32_t)> nis_update; //00180C48.
 std::function<void(driving::Memory&,std::uint32_t,int)> score_sound; //PlayScoreSound.
 std::function<void(driving::Memory&,std::uint32_t)> radar_beep; //bSample.Play.
 std::function<void(driving::Memory&,std::uint32_t)> timer_warning; //TriggerTimerWarningSound.
};
PlayerHudPresentationServices source_player_hud_services(); // Native bindings; map geometry supplied by World host.
struct PlayerHudGameplayState {
 bool source_hud=false;
 const PlayerHudPresentationServices* presentation=nullptr;
 bool score_active=false;
 int score_phase=0;
 float score_start_time=0;
 std::optional<PlayerHudCameraContext> camera;
 // Logical original full HUD feature mask; changes flush pending source scores.
 std::uint32_t hud_features=0;
};
}

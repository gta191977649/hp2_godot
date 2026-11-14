#pragma once
#include <cstdint>

struct WorldSimulation;
namespace godot { class Dictionary; class PackedVector3Array; }

namespace hp2 {

struct MainMenuScreenServices;
struct FrontendStateView;
struct RaceStateView;
using FrontendReady = void (*)(MainMenuScreenServices&, const void*);
godot::PackedVector3Array race_photo_route(const RaceStateView& race);

struct CampaignRaceDiagnostic {
    int tree{-1};
    int event{};
};

// The context is borrowed from the host session through the startup callback.
void start_campaign_race_diagnostic(MainMenuScreenServices& menus, const void* context);
void start_quick_race_diagnostic(MainMenuScreenServices& menus, const void* context);
void configure_race_diagnostics(bool unlockRaces);
bool profile_shared_world_reflections();
FrontendReady reflection_profile_frontend_ready();
FrontendReady race_profile_frontend_ready();
bool profile_legacy_primary_scenery();
void collect_police_diagnostics(godot::Dictionary& result, const RaceStateView& race,
    const FrontendStateView& frontend);

#ifdef HP2_MVP_IMGUI
struct RaceDiagnosticChecks {
    bool pursuitDriver{};
    bool arrestCinematic{};
    bool supportVisuals{};
    bool trackEffectsCamera{};
    std::uint64_t pursuitGeneration{};

    void advance(const WorldSimulation& simulation, int track, int raceState,
        std::uint64_t generation, std::uint32_t step);
};
#endif

}

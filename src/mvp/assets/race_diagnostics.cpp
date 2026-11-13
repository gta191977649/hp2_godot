#include "race_diagnostics.hpp"

#include "bware/inc/bWare.hpp"
#include "indep/inc/EventTree.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/World.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <algorithm>
#include <stdexcept>

#ifdef HP2_MVP_IMGUI
#include "tests/pursuit_test_driver.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Chopper.hpp"
#include "indep/inc/Munitions.hpp"
#include "indep/inc/spikestrip.hpp"
#include "bware/inc/bMath.hpp"
#include <cstdio>
#endif

namespace hp2 {

void configure_race_diagnostics(bool unlockRaces) {
    applicationConfigureTesting(false, unlockRaces);
}

void collect_police_diagnostics(godot::Dictionary& result, const RaceStateView& race,
    const FrontendStateView& frontend) {
    if (!race.beTheCop || !race.simulation || !race.simulation->pursuit) return;
    const auto& pursuit = *race.simulation->pursuit;
    const auto* selected = pursuitManagerSelectedEntry(&pursuit);
    const auto* group = selected ? selected->group : nullptr;
    const auto* players = race.simulation->players;
    const auto* player = players ? players->playersByNumber[0] : nullptr;
    const auto clock = hashString("BETHECOP_TIME");
    result["police_hud"] = frontend.layout && std::any_of(frontend.layout->layers.begin(),
        frontend.layout->layers.end(), [clock](const auto& layer) {
            return std::any_of(layer.begin(), layer.end(),
                [clock](const FlailerStrip* strip) { return strip->sourceObject == clock; });
        });
    result["player_is_pursuing_officer"] = player && player->car && selected && selected->movement == player->car;
    result["police_seconds"] = pursuit.remainingSeconds;
    result["police_roadblocks"] = pursuit.roadBlocksRemaining;
    result["police_helicopters"] = pursuit.chopperRequestsRemaining;
    result["police_target_active"] = group && group->target;
    result["police_support_flags"] = group ? group->requestFlags : 0u;
    result["police_busts"] = pursuit.completedPullovers;
}

void start_quick_race_diagnostic(MainMenuScreenServices& menus, const void* context) {
    const auto track = *static_cast<const int*>(context);
    if (!menus.raceSetup->tracks->Find(track))
        throw std::invalid_argument("Track is absent from the authored scenario catalogue");
    StartQuickRace(menus, 0, track);
}

void start_campaign_race_diagnostic(MainMenuScreenServices& menus, const void* context) {
    const auto& request = *static_cast<const CampaignRaceDiagnostic*>(context);
    const auto& setup = *menus.raceSetup;
    auto* tree = setup.events->PrimaryTree(static_cast<std::size_t>(request.tree));
    auto* selected = tree ? tree->GetEventNumber(request.event) : nullptr;
    if (!selected) throw std::invalid_argument("Event is absent from the authored campaign tree");

    auto& frontend = *menus.frontend;
    auto& parameters = *setup.parameters;
    raceParametersInitializeDefaults(&parameters, setup.applicationOptions, &setup.language->currentLanguage);
    frontend.gameType = request.tree == 0 ? 8 : 9;
    frontend.playerCount = 1;
    frontend.copsEnabled = request.tree == 1;
    frontend.challengeSetup.raceOption254 = 1;
    frontend.eventSelection.treeType = request.tree;
    frontend.eventSelection.SetSelectedEvent(request.tree, request.event);
    parameters.authoredRaceOption254 = 1;
    tree->SetRace(parameters, *selected, {&frontend, setup.currentEvent, setup.cars, setup.tracks, setup.language});
    frontend.MakeEventCarsAvailable(*tree, *selected);

    const auto* authoredCar = selected->GetCar(0);
    Require(authoredCar, "Diagnostic event has no authored player car");
    const auto model = static_cast<int>((static_cast<std::uint32_t>(*authoredCar) >> 16) & 255u);
    auto& profile = *frontend.players[0];
    const auto& available = profile.availability.at(static_cast<std::size_t>(model));
    Require(available.skinCount > 0, "Diagnostic event has no authored player skin");
    profile.AssignCarOfType(frontend.gameType, model, available.skinNumbers[0], true);
    menus.screens->stack.Push("View Event Tree");
    frontend.StartRace(setup);
}

#ifdef HP2_MVP_IMGUI
void RaceDiagnosticChecks::advance(const WorldSimulation& simulation, int track, int raceState,
    std::uint64_t generation, std::uint32_t step) {
    if (raceState != RC_STATE_RACE_IN_PROGRESS) return;
    if (trackEffectsCamera && (track == 12 || track == 21)) {
        // Views beside two source 35020 emitters. The normal emitter
        // scheduler still decides when to emit using the actual view eye.
        const float forest[3] = {-654.92f, 190.07f, -53.85f};
        const float desert[3] = {1517.87f, 206.27f, 15.37f};
        const float* target = track == 12 ? forest : desert;
        auto& view = simulation.particles->cameras->screens[1].view->current;
        std::copy_n(target, 3, view.eye);
        view.eye[0] -= 35.0f;
        view.eye[1] -= 18.0f;
        view.eye[2] += 195.0f;
        view.forward[0] = 0.176f;
        view.forward[1] = 0.0905f;
        view.forward[2] = -0.9803f;
        view.matrix[1] = 0.0f;
        view.matrix[5] = 0.0f;
        view.matrix[9] = -1.0f;
    }
    if (supportVisuals && step >= 35) {
        const auto* manager = simulation.pursuit;
        auto* chopper = simulation.chopper;
        if (!manager || !chopper || manager->groupCount == 0) return;
        const auto* selected = pursuitManagerSelectedEntry(manager);
        if (!selected || !selected->group) return;
        auto* barricades = selected->group->owner->barricades;
        Require(barricades, "Police support test requires the selected pursuit's barricade owner");
        const auto* player = simulation.players->playersByNumber[0];
        const auto& car = *player->car->bindings.state;
        const float point[2] = {
            car.centerPosition[0] + car.orientation[0] * 18.0f,
            car.centerPosition[1] + car.orientation[1] * 18.0f
        };
        float ground[4];
        pursuitGroundPoint(barricades->topology, ground, point);
        std::int16_t barrel = -1;
        Require(pursuitBarricadesCreateAt(barricades, ground, &barrel) == WORLD_SERVICES_OK && barrel != -1,
            "Police support test could not create the authored roadblock barrel");
        auto* registry = chopper->environment.munitions;
        auto* bomb = chopperSmokeBombCreate(registry);
        Require(bomb, "Police support test could not create the authored smoke bomb");
        float pose[16];
        rotationMatrixIdentity(pose);
        std::copy_n(ground, 3, pose + 12);
        pose[14] += 2.0f;
        const float velocity[4]{};
        chopperMunitionSetTransform(bomb, pose, velocity);
        chopperMunitionDetonate(bomb);
        Require(registry->result == WORLD_SERVICES_OK, "Police support test smoke detonation failed");
        chopperMunitionDestroy(bomb);
        // A second original drop stays live long enough to inspect its mesh
        // regardless of how the AI approaches the selected target.
        auto* drop = chopperSmokeBombCreate(registry);
        Require(drop, "Police support test could not create the live smoke bomb");
        pose[14] += 8.0f;
        chopperMunitionSetTransform(drop, pose, velocity);
        supportVisuals = false;
        std::fprintf(stderr, "HP2_MVP_TRACE stage=support_visual_fixture barrel=%d\n", barrel);
    }
    if (track != 13) return;
    if (arrestCinematic && step >= 60) {
        start_arrest_cinematic_test(simulation);
        arrestCinematic = false;
    }
    if (pursuitDriver && pursuitGeneration != generation) {
        attach_pursuit_test_driver(*simulation.world, *simulation.players->playersByNumber[0]);
        pursuitGeneration = generation;
        std::fprintf(stderr, "HP2_MVP_TRACE stage=pursuit_test_driver track=13 generation=%llu\n",
            static_cast<unsigned long long>(generation));
    }
}
#endif

}

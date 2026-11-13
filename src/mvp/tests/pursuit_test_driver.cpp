#include "pursuit_test_driver.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/World.hpp"

#include <stdexcept>

namespace hp2 {

void start_arrest_cinematic_test(const WorldSimulation& simulation) {
    auto& player = *simulation.players->playersByNumber[0];
    auto* car = player.car->bindings.owner;
    auto* pursuit = simulation.pursuit;
    PursuitCollisionEntry* officer{};
    if (!pursuit || !pursuit->parameters || car->tableIndex >= 8 ||
        pursuitManagerSelectArrestingOfficer(pursuit, nullptr, &officer) != WORLD_SERVICES_OK ||
        !officer || !officer->movement)
        throw std::runtime_error("Arrest cinematic test needs the original player and an officer");
    auto& row = pursuit->participantReset[car->tableIndex];
    row.pulloverCount = pursuit->parameters->pulloverLimit;
    if (!carFramePlayerLoadNextNis(&player, player.car, officer->movement, row.pulloverCount) ||
        !carFramePlayerPlayNextNis(&player, player.car, officer->movement, CAR_PLAYER_NIS_COMPLETION_NONE, -1))
        throw std::runtime_error("The donor could not start its arrest cinematic");
}

// Test-only takeover of the live Player's car. The donor still owns driving,
// the World and the Player, including the camera, HUD and pursuit target.
void attach_pursuit_test_driver(World& world, const CarFramePlayerState& player) {
    for (int slot = 0; slot < world.totalCars; ++slot) {
        auto* car = world.carTable[slot];
        if (car->Movement() != player.car) continue;
        auto* driver = car->Driver();
        if (!driver || driver->kind != CAR_DRIVER_REAL ||
            carDriverChange(driver, car, CAR_DRIVER_AI) != CAR_DRIVER_OK)
            throw std::runtime_error("Pursuit test could not attach the donor AI to the player car");
        auto& ai = driver->owner.ai->base;
        driverAIRefreshRoad(&ai);
        if (ai.result != CAR_DRIVER_OK)
            throw std::runtime_error("Pursuit test AI could not locate the authored road");
        worldRegisterAiCar(&world, car);
        return;
    }
    throw std::runtime_error("Pursuit test could not find the player car");
}

}

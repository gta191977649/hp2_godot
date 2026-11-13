#pragma once

struct World;
struct WorldSimulation;
struct CarFramePlayerState;

namespace hp2 {

void attach_pursuit_test_driver(World& world, const CarFramePlayerState& player);
void start_arrest_cinematic_test(const WorldSimulation& simulation);

}

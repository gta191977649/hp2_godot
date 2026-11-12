#pragma once
// TASK-0107 E-0107-F01. Port debug inputs -> original FreeRun launch producers.
#include "world_session.hpp"
#include "race_launch_catalog.hpp"
namespace port::race {
// Requires InitCarInfo/CharacterResources, language/TrackInfo projections,
// and the reviewed AiPlanningResources difficulty table in this same Memory.
// Does not unlock events/cars or write a persistent profile. Resolves exact IDs.
void prepare_free_run_launch(driving::Memory&, WorldLaunchRequest&, const LaunchCatalog&);
}

#pragma once

#include "bridge/presentation_snapshot.hpp"
#include "indep/inc/RaceEffects.hpp"

struct CarCinematicViewFrame;

namespace hp2 {

struct RaceStateView;
class ResourceRegistry;
RaceCameraState source_main_camera(const CarCinematicViewFrame& camera, bool ntsc);
// Borrowed texture bindings for synchronous scene creation.
std::vector<RaceLightEffect> stage_track_coronas(ResourceRegistry& resources);

class EntityStateBridge {
public:
    const EntityFrame& stage(const RaceStateView& race);
    void publish() noexcept;
    const EntityFrame& current() const noexcept { return current_; }

private:
    EntityFrame current_;
    EntityFrame pending_;
};

}

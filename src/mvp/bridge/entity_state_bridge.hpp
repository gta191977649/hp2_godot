#pragma once

#include "bridge/presentation_snapshot.hpp"

struct CarCinematicViewFrame;

namespace hp2 {

struct RaceStateView;
RaceCameraState source_main_camera(const CarCinematicViewFrame& camera, bool ntsc);

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

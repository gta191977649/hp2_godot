#pragma once

#include "bridge/presentation_snapshot.hpp"

namespace hp2 {

struct RaceStateView;

class EntityStateBridge {
public:
    EntityFrame stage(const RaceStateView& race) const;
    void publish(EntityFrame frame) noexcept;
    const EntityFrame& current() const noexcept { return current_; }

private:
    EntityFrame current_;
};

}

#pragma once

#include "bridge/presentation_snapshot.hpp"

namespace hp2 {

struct RaceStateView;

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

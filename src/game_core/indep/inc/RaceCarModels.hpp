#pragma once

#include <array>

struct Car;
struct EditorMarkerModel;

namespace hp2 {

// Asset borrow for synchronous scene construction. Pose is published separately.
std::array<const EditorMarkerModel*, 7> RaceCarModels(Car& car, int worldFrames);

}

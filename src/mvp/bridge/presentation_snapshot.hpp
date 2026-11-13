#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace hp2 {

struct VehicleEntityState {
    std::uint64_t handle{};
    int model{}, skin{};
    bool player{}, visible{};
    std::array<float, 16> transform{};
    std::array<std::array<float, 16>, 4> wheels{};
    float speed{};
};

struct RaceCameraState {
    std::array<float, 3> eye{}, forward{}, up{};
    float verticalFov{}, nearClip{}, farClip{};
};

struct EntityFrame {
    std::uint64_t generation{};
    std::uint32_t step{};
    int track{-1};
    RaceCameraState camera;
    std::vector<VehicleEntityState> vehicles;
};

}

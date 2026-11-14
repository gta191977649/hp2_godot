#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

struct EditorMarkerModel;

namespace hp2 {

struct WheelBlurState {
    std::array<float, 16> transform{};
    float alpha{};
};

struct VehicleEntityState {
    std::uint64_t handle{};
    int model{}, skin{};
    bool player{}, visible{}, rearVisible{};
    std::array<float, 16> transform{};
    std::array<std::array<float, 16>, 4> wheels{};
    std::array<WheelBlurState, 4> wheelBlur{}, rearWheelBlur{};
    float speed{};
};

struct WorldObjectEntityState {
    std::uint64_t handle{};
    // Borrowed only during synchronous presentation of the staged frame.
    const EditorMarkerModel* model{};
    bool visible{};
    std::array<float, 16> transform{};
};

struct RaceCameraState {
    std::array<float, 3> eye{}, forward{}, up{};
    float verticalFov{}, nearClip{}, farClip{};
};

struct EntityFrame {
    std::uint64_t generation{};
    std::uint64_t replayEpoch{};
    std::uint32_t step{};
    int track{-1};
    bool reverse{};
    RaceCameraState camera;
    std::optional<RaceCameraState> rearCamera;
    std::vector<VehicleEntityState> vehicles;
    std::vector<WorldObjectEntityState> worldObjects;
};

}

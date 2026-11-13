#pragma once

// Individually proved eEnvMapE.obj, including the constructor's original
// eEnvMap.hpp N_SOL records. PAL owns all behavior and table values.
#include "indep/inc/Ecstasy.hpp"
#include <array>
#include <cstddef>

struct World;
struct CarFramePlayerRegistry;

namespace hp2 {
struct EnvironmentReflectionSample {
    std::array<float,3> direction{};
    std::array<float,2> mappedST{};
    std::array<float,2> faceST{};
};

// The one original six-Camera owner. Views and native GPU images keep their
// existing owners; source samples replace only the console packet transport.
class EnvironmentMap final {
public:
    EnvironmentMap(CarCinematicServices& views,EcstasyRenderTarget* targets,std::size_t count);
    ~EnvironmentMap();
    EnvironmentMap(const EnvironmentMap&)=delete;
    EnvironmentMap& operator=(const EnvironmentMap&)=delete;
    void UpdateCameras(const float viewEye[3],const float reflectionOrigin[3],
        std::int32_t applicationState,float realTimeElapsed);
    void CreatePrimitives(EcstasyFrame& frame) const;
    const std::array<float,16>& BodyMatrix() const noexcept {return bodyMatrix_;}
    EcstasyRenderTarget& MappedTarget() const noexcept {return *mappedTarget_;}
    const EnvironmentReflectionSample& Sample(std::size_t face,std::size_t row,std::size_t column) const;
    const EnvironmentReflectionSample& BackSample(std::size_t patch,std::size_t row,std::size_t column) const;
    std::array<float,3> eyeOffset{}; // Released EnvMapEyeOffset2E0260.
    std::int32_t enabled{1}; // Released EnableUpdateEnvMapCameras3306A0.
private:
    void CreateReflectionSamples();
    void CreateFacePrimitive(EcstasyFrame& frame,const CarCinematicScreen& view,
        const EnvironmentReflectionSample* samples) const;
    CarCinematicServices& views_;
    std::array<CarCinematicView,6> cameras_{};
    EcstasyRenderTarget* mappedTarget_{};
    std::array<float,16> bodyMatrix_{};
    // Native elapsed-time scheduling, independent of presentation count.
    // Twelve covers both authored periods (four and six); no catch-up
    // image is rendered more than once in one PC frame.
    double scheduleRemainder_{};
    std::uint32_t schedulePhase_{};
    bool scheduleInitialized_{},previousPrimary_{};
    std::array<float,6> cameraElapsed_{};
    std::array<std::array<EnvironmentReflectionSample,49>,6> samples_{};
    std::array<std::array<EnvironmentReflectionSample,49>,4> backSamples_{};
};

// PAL123BA0: actual top camera GetCarToFollow, player1, first World car,
// then the source FrontEndRenderingCar list. No second car pose is retained.
void UpdateEnvironmentMapCameras(EnvironmentMap& environment,CarCinematicServices& views,
    World* world,const CarFramePlayerRegistry* players,std::int32_t applicationState,
    float realTimeElapsed);
}

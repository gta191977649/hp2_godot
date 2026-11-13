#pragma once

#include "indep/inc/PhysicsCar.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_PERFORMANCE_SAMPLES 32

/* Copyable configuration owns its sample values. Views are rebuilt on demand
 * so moving a decoded car definition cannot leave a pointer into its old copy. */
typedef struct CarPerformance {
    float samples[CAR_PERFORMANCE_SAMPLES];
    float maximumSpeed;
} CarPerformance;

typedef struct CarPerformanceTuning {
    float defaultMaximumSpeed;
    float maximumSampleSpeed;
    float sampleSpeedStep;
    float angularVelocityToRpm;
    DriveTrainTuning shift;
} CarPerformanceTuning;

extern const CarPerformanceTuning carPerformanceDefaultTuning;

CarPerformance* carPerformanceInitialize(CarPerformance* performance);
int32_t carPerformanceBuild(CarPerformance* performance, const PhysicsCarInfo* info,
                             const CarPerformanceTuning* tuning);
SampledCurve carPerformanceCurve(const CarPerformance* performance);


#ifdef __cplusplus
}

#include "bware/inc/bChunk.hpp"
#include "bware/inc/bFile.hpp"
#include "indep/inc/Engine.hpp"
#include "indep/inc/RigidBody.hpp"
#include "indep/inc/Suspension.hpp"
#include "indep/inc/Aerodynamics.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/RaceParameters.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct EditorMarkerTexture;
struct CarViewMoverRegistry;

namespace hp2 {
extern "C++" {
class LanguageManager;
// The proven original header definition is provided out of line in its
// independently proved compilation unit, fred.cpp. No beta layout is used.
void InitializeRideInfo(CarAppearanceSelection& ride,std::int32_t model,
    std::int32_t skin,std::int32_t dashboard,std::int32_t reserved);
}

// One typed projection of the final fifty-entry language descriptor table.
// The canonical catalogue retains these records; cars borrow their selected
// record rather than copying a language ID or using the technical model name.
struct CarLanguageInfo {
    std::int32_t modelCode{};
    std::int32_t shortNameLanguageId{};
    std::int32_t fullNameLanguageId{};
    // Four actual final-PAL descriptor IDs, not recomputed specifications.
    std::int32_t accelerationLanguageId{},topSpeedMphLanguageId{},topSpeedKmhLanguageId{},powerLanguageId{};
};

// Wheel configuration from the original car type record.
struct CarWheel {
    std::array<float, 3> position{};
    float radius = 0;
    float halfWidth = 0;
    std::uint32_t meshIndex = 0;
    float sideValue = 0;
    float unknown = 0;
};

// Immutable gauge configuration decoded from final CarInfo+4D0. The selected
// car and every HUD borrower use this one retained GLOBALB record.
struct CarGaugeInfo {
    std::int32_t combined{};
    std::array<float,2> tachometerAngles{};
    // Original order is MPH/KMH. Each retained pair contains the minimum
    // and maximum of one authored angle or speed interval.
    std::array<std::array<float,2>,2> speedometerAngles{};
    std::array<std::array<float,2>,2> speedometerRange{};
    float genericMaximum{};
    std::array<std::int16_t,2> tachometerCenter{};
    std::array<std::int16_t,2> speedometerCenter{};
    std::array<std::int16_t,2> speedTextPosition{};
    std::array<std::int16_t,2> unitsPosition{};
};

struct CarDefinition {
    std::string name;
    std::string baseName;
    std::string manufacturer;
    std::string geometryPath;
    std::string geometryCompressedPath;
    std::string engineSound;
    std::array<CarWheel, 4> wheels{};  // Front left, front right, rear right, rear left.
    DriveTrainInfo driveTrainInfo{};
    RigidBodyInfo rigidBodyInfo{};
    SteeringGeometry steeringGeometry{};
    AerodynamicsInfo aerodynamicsInfo{};
    std::array<WheelTorqueInfo, 4> wheelInfo{};
    std::array<WheelTireInfo, 2> tireInfo{};
    WheelBrakeInfo brakeInfo{};
    std::array<SuspensionInfo, 2> suspensionInfo{};
    std::array<float, 4> centerOfMass{};
    EngineInfo engineInfo{};
    CarPerformance performance{};
    float forwardSpeedLimitMph = 0;
    float ordinaryYawLimit = 0;
    float pursuitYawLimit = 0;
    float preloadDownforceScale = 0;
    float yawBaseCoefficient = 0;
    float yawCollisionCoefficient = 0;
    float steeringRangeDegrees = 0;
    bool pursuitConfiguration = false;
    std::int32_t modelCode = 0;
    std::int32_t bodyCategory = 0;
    // Final CarInfo+53C/+540, read independently by challenge/be-a-cop
    // availability. They are authored status values, not unlock booleans.
    std::int32_t challengeAvailability = 0;
    std::int32_t pursuitAvailability = 0;
    std::int8_t routeVehicleClass = 0;
    CarCameraCatalogue cameraCatalogue{};
    std::uint32_t recordOffset = 0;
    float cameraTargetClearance = 0;
    bool supportsDashboardTextures = false;
    std::array<float,5> selectionWeights{};
    std::array<float,4> driverAnimationOffset{};
    const CarLanguageInfo* languageInfo{};
    // Physical CarInfo source+354, consumed by the AI engine sample name.
    std::uint16_t aiEngineSampleIndex{};
    // Authored CarInfo source+1E4/+1E8; no half-extent interpretation.
    std::array<float,2> roadBlockDimensions{};
    // PlayerEngine consumes these independently from the AI sample index.
    // The authored prefix occupies356..35D; pitchOrigin starts at35E.
    std::array<char,8> playerEngineSamplePrefix{};
    std::uint16_t enginePitchOrigin{};
    std::array<std::int8_t,4> engineThrottleRamp{};
    std::int16_t engineDetuneScale{};
    std::int16_t engineDetuneBias{};
    std::array<std::int16_t,12> engineVoiceDetune{};
    std::array<float,12> engineVoiceGain{};
    std::int16_t engineParameter3B0{};
    std::int16_t engineParameter3B2{};
    // Separate dashboard geometry filename, authored CarInfo+A0.
    std::string dashboardGeometryPath;
    std::int8_t selectionSortOrder{};
    std::int32_t defaultSkinNumber{};
    CarGaugeInfo gauges{};
    PhysicsCarInfo PhysicsInfo() const noexcept;
    const char* GetShortName(const LanguageManager& language) const;
    const char* GetFullName(const LanguageManager& language) const;
};


// The application supplies the actual content policy. Resource loading does
// not infer it from a file name, the selected car or an absent demo manager.
enum class CarInfoAvailability { FullGame, DemoDisc };

struct CarSkin {
    std::int32_t modelCode = 0;
    std::int32_t skinNumber = 0;
    std::string displayName;
    std::uint32_t selectionMask = 0;
    std::uint32_t lightMaterialKey{}; // final SkinInfo+2C, not a texture
    std::uint32_t authoredWord30{};
};

struct CarSkinChoices {
    std::array<std::int8_t, 8> numbers{};
    std::uint8_t count = 0;
};

// This owns the decoded GLOBALB car/skin resources. Definitions and visual
// descriptors have stable addresses. All borrowers must finish before a
// resource is unloaded or replaced; the owner itself cannot be moved.
class CarInfoResource final {
public:
    static constexpr std::size_t CarCount = 64;
    static constexpr std::uint32_t CarChunk = 0x34600;
    static constexpr std::uint32_t SkinChunk = 0x34601;

    CarInfoResource();
    CarInfoResource(const GameResources& resources, CarInfoAvailability availability);
    CarInfoResource(const std::vector<std::uint8_t>& package, CarInfoAvailability availability);
    CarInfoResource(const CarInfoResource&) = delete;
    CarInfoResource& operator=(const CarInfoResource&) = delete;
    CarInfoResource(CarInfoResource&&) = delete;
    CarInfoResource& operator=(CarInfoResource&&) = delete;

    // Chunk events preserve the separate source car/skin lifetimes. Unknown
    // chunk kinds return false and leave both resources unchanged.
    bool LoadChunk(const std::vector<std::uint8_t>& package, const Chunk& chunk,
                   CarInfoAvailability availability);
    bool UnloadChunk(std::uint32_t kind) noexcept;
    void LoadPackage(const std::vector<std::uint8_t>& package, CarInfoAvailability availability);

    bool CarsLoaded() const noexcept { return carsLoaded_; }
    bool SkinsLoaded() const noexcept { return skinsLoaded_; }
    const std::vector<CarDefinition>& Cars() const noexcept;
    const CarDefinition& Car(std::size_t modelCode) const;
    // Cop construction changes this same shared per-model physical record.
    // Existing bodies refresh their caches at their original refresh calls.
    RigidBodyInfo* MutableRigidBodyInfo(std::size_t modelCode);
    const CarDefinition& Find(const std::string& name) const;
    bool IsAvailable(std::size_t modelCode) const;

    const CarDriverVisualVehicleInfo& VisualInfo(std::size_t modelCode) const;
    const CarDriverVisualVehicleInfo& VisualInfo(const std::string& name) const;
    const CarSkinChoices& AuthoredSkins(std::size_t modelCode) const;
    const std::vector<CarSkin>& Skins() const noexcept { return skins_; }
    const CarSkin* FindSkin(std::int32_t modelCode, std::int32_t skinNumber,
                            bool allowFirstFallback) const noexcept;
    std::int32_t AvailableSkinCount(std::size_t modelCode) const;
    std::int32_t AvailableSkinNumber(std::size_t modelCode, std::int32_t selection,
                                     bool alternatePursuitSkin) const;
    std::int32_t RandomSkin(std::size_t modelCode, bool alternatePursuitSkin) const;
    bool WeightedPickRide(CarAppearanceSelection& output, std::int32_t bodyCategory,
        std::int32_t routeVehicleClass, const RaceParameters& parameters,
        const LanguageManager& language) const;
    CarLanguageInfo* FindCarLanguageInfo(std::int32_t modelCode);
    void InitCarInfo();

    // Original acceleration storage is independent of the resource pointer:
    // unloading cars does not erase its 64 constructed sample tables.
    const CarPerformance& Acceleration(std::size_t modelCode) const;
    SampledCurve AccelerationCurve(std::size_t modelCode) const;

private:
    struct Appearance {
        CarDriverVisualVehicleInfo visual{};
        CarSkinChoices skins{};
    };
    void LoadCars(const std::vector<std::uint8_t>& package, const Chunk& chunk,
                  CarInfoAvailability availability);
    void LoadSkins(const std::vector<std::uint8_t>& package, const Chunk& chunk);

    std::vector<CarDefinition> cars_;
    std::array<CarLanguageInfo,50> languageInfo_{};
    std::array<Appearance, CarCount> appearance_{};
    std::vector<CarSkin> skins_;
    bool carsLoaded_ = false;
    bool skinsLoaded_ = false;
};

struct CarUsedTextureInfo {
    std::array<std::uint32_t,16> usedKeys{};
    std::uint32_t count = 0;
    std::array<std::uint32_t,2> defaultSkinKeys{};
    std::uint32_t defaultDashboardKey = 0;
    std::array<std::uint32_t,2> headlightSideKeys{};
    std::array<std::uint32_t,2> brakeSideKeys{};
    std::uint32_t reverseLightKey = 0;
    std::uint32_t defaultTireKey = 0;
    std::uint32_t defaultTireBlurKey = 0;
    std::uint32_t licensePlateKey = 0;
    std::array<std::uint32_t,2> skinKeys{};
    std::uint32_t dashboardKey = 0;
    std::array<std::uint32_t,9> lightingKeys{};
    std::uint32_t tireKey = 0;
    std::uint32_t tireBlurKey = 0;
    std::uint32_t shadowKey = 0;
};

// The renderer and every car borrow this same mutable naming policy. Its
// initial value is the actual game setting, not a per-car copy.
struct CarTextureNamingPolicy { std::string tireTextureFormat = "%s_TIRE"; };

CarUsedTextureInfo BuildCarTextureInfo(const CarInfoResource& info,
    const CarAppearanceSelection& selection, int frontend,
    const CarTextureNamingPolicy& naming);

// Texture metadata/pixels live only in the canonical ResourceRegistry/TexturePack.
class ResourceRegistry;

} // namespace hp2

// Native presentation ownership adapters; they do not add original ELF
// functions. The car retains one live DriverInfo selection and borrows its
// immutable resources. The texture catalog and naming policy must outlive it.

void carAppearanceDispose(Car* car);
const hp2::CarUsedTextureInfo* carAppearanceTextures(const Car* car);
const EditorMarkerTexture* carAppearanceReplacement(const Car* car, std::size_t slot);

struct EditorMarkerModel;
struct PositionMarkerRegistry;
struct WorldParticleSystem;
struct TerrainEffectEmitters;
struct ChopperEffectGroup;

namespace hp2 {
struct CarEffectPosition {
    std::array<float,3> value{};
};
// InitCarEffects11DB90 / CloseCarEffects11DC50: one World owns the350
// position slots and26 final effect-name keys shared by every live Car.
// The World releases its cars before this owner; slots are never per-car pools.
class CarEffectsResources final {
public:
    CarEffectsResources();
    ~CarEffectsResources();
    CarEffectsResources(const CarEffectsResources&)=delete;
    CarEffectsResources& operator=(const CarEffectsResources&)=delete;
    static constexpr std::size_t Capacity=350;
    std::size_t UsedPositions() const noexcept;
    std::uint32_t EffectKey(std::size_t effect) const;
    const std::array<float,16>& Identity() const noexcept;
    const std::array<std::uint32_t,12>& MaterialWords() const noexcept;
    CarEffectPosition* AcquirePosition(const std::array<float,3>& position);
    void ReleasePosition(CarEffectPosition* position) noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}

// The body handle, marker list, terrain owner slot and particle pool are the
// same live owners used by the model and world services. They must outlive
// this binding. This binds the CPU portion of InitRenderInfo; model creation
// and rendering retain their own original responsibilities.
CarEffectResult carAppearanceBindEffects(Car* car,const EditorMarkerModel* body,
    const PositionMarkerRegistry* markers,WorldParticleSystem* particles,
    TerrainEffectEmitters* const* terrain,const ChopperEffectGroup* groups,std::size_t groupCount,
    hp2::CarEffectsResources* resources);
void carAppearanceReleaseEffects(Car* car);
const float* carAppearanceEffectPosition(const Car* car,std::size_t position,std::size_t index);

struct ReplaySnapshot;
struct CarDriverAppearanceCatalogue;
namespace hp2 {
void SnapshotRideInfo(CarAppearanceSelection& ride,ReplaySnapshot& snapshot);
void SnapshotDriverInfo(RaceDriverInfo& driver,ReplaySnapshot& snapshot,
    const CarDriverAppearanceCatalogue& characters);
}

namespace hp2 {
const char* RaceDriverName(const RaceDriverInfo& driver);
}

// The original CarRenderInfo owns all26 live models and replacement tables.
// Configuration/resources are borrowed, including the same application mode
// and shared effect-texture slot written by the actual startup owners.
struct CarPartCuller {
    float position[4]{};
    std::uint32_t visibleQueries{},culledQueries{};
    std::int32_t culled{};
};
struct CarPartCullingPlaneInfo {
    std::int32_t combine{},count{};
    float normals[3][4]{};
    float offsets[4]{};
};
extern CarPartCullingPlaneInfo carPartCullingPlanes[14];
extern std::int32_t carPartCullingNormalizeOnQuery;
void carPartCullerInitialize(CarPartCuller* parts,std::int32_t part,const float position[4]);
void carPartCullerCull(CarPartCuller* parts,const float eye[4]);

struct CarLightFlare {
    std::uint32_t markerKey{};
    std::uint8_t active{1},kind{},flags{},state{};
    float position[4]{},normal[4]{};
};
struct TrackScenarioDefinition;
extern std::int32_t carRenderForceHeadlightsOn;
extern std::int32_t carRenderForceBrakelightsOn;
// PAL121128/121148 and121ACC: the model-scale matrix and driver visibility
// switch are shared by the original Stomper descriptors, not per-car bounds.
extern float CarScaleMatrix[16];
extern std::int32_t carRenderDriverVisibilityOverride;
// Individually matched PAL/beta CarE globals. -1 selects normal detail;
// body and tire overrides are independent, with tires clamped to level C.
extern std::int32_t ForceCarLOD;
extern std::int32_t ForceTireLOD;

namespace hp2 {
class EnvironmentMap;
struct CarRenderBindings {
    const CarInfoResource* cars{};
    // Borrow the current GLOBALB scenario; null is the original frontend/no-track branch.
    const ::TrackScenarioDefinition* scenario{};
    ResourceRegistry* resources{};
    const CarTextureNamingPolicy* naming{};
    const PositionMarkerRegistry* markers{};
    WorldParticleSystem* particles{};
    TerrainEffectEmitters* const* terrain{};
    const ChopperEffectGroup* effectGroups{};
    std::size_t effectGroupCount{};
    const std::int32_t* applicationState{};
    const CarDriverAppearanceCatalogue* characters{};
    CarDriverVisualBindings visual{};
    EditorMarkerTexture** sharedEffectTexture{};
    CarEffectsResources* carEffects{};
};

// Original frontend render owner (PAL11E5C0/11E758/11E7D8). The copied
// RideInfo is intentional source state; the 26 model handles share the same
// CarRenderInfo implementation as a race car, whose Car pointer is null here.
// Resources and the application-state cell must outlive this owner. Previous
// GPU submission must have completed before ReInit or destruction.
struct FrontendFrame;
void CollectFrontendCarModels(FrontendFrame& frame);
class FrontEndRenderingCar final {
public:
    FrontEndRenderingCar(const CarRenderBindings& bindings,
        const CarAppearanceSelection* selection,std::int32_t view);
    ~FrontEndRenderingCar();
    FrontEndRenderingCar(const FrontEndRenderingCar&)=delete;
    FrontEndRenderingCar& operator=(const FrontEndRenderingCar&)=delete;
    void ReInit(const CarAppearanceSelection* selection);

    CarAppearanceSelection ride{};
    std::int32_t viewNumber{};
    float position[4]{},orientation[16]{},wheels[4][16]{},brakes[4][16]{};
    // Source transfers ownership of this optional model to the render car.
    EditorMarkerModel* overrideModel{};
    bool enabled{true};
    std::uint32_t drawFlags{1};
    bool reflected{};
    std::int32_t headlights{},policeLights{};
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    friend void CollectFrontendCarModels(FrontendFrame&);
    friend void RenderFrontEndCars(const CarCinematicScreen&,EcstasyFrame&,
        std::int32_t,std::int32_t,bool,bool,bool,bool);
    friend void UpdateEnvironmentMapCameras(EnvironmentMap&,CarCinematicServices&,
        World*,const CarFramePlayerRegistry*,std::int32_t,float);
};
void RenderFrontEndCars(const CarCinematicScreen& view,EcstasyFrame& frame,
    std::int32_t worldFrames,std::int32_t presentationLayout,
    bool normal,bool reflected,bool opaque,bool transparent);
}
void carInitRenderInfo(Car* car,const hp2::CarRenderBindings* bindings);
void carRenderUpdateReplacementTextures(Car* car);
void carRenderCreateLightFlares(Car* car,const PositionMarkerRegistry* markers);
const EditorMarkerModel* carRenderModel(const Car* car,std::size_t slot);
const CarPartCuller* carRenderPartCullers(const Car* car);
const CarLightFlare* carRenderLightFlares(const Car* car,std::size_t* count);
void carRenderBounds(const Car* car,float minimum[4],float maximum[4]);
// PC-only quality selector over original body LOD0..3. All source thresholds
// and explicit frontend forced LOD take precedence where authored.
std::size_t carRenderNativeDetailLod(const EditorMarkerModel* models,
    std::int32_t originalPixels) noexcept;
std::uint32_t carRenderBrokenLightMask(const Car* car);
// Original CarRenderInfo geometry pass. The same model/damage/texture owner
// supplies body parts and the physical suspension supplies wheel transforms.
// Lighting, flares and terrain-projected shadow completion remain tracked
// separately; this declaration carries no complete-body acceptance claim.
std::size_t carRenderScene(Car& car,const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    const float position[3],const float orientation[16],const float wheels[4][16],
    const float brakes[4][16],std::int32_t worldFrames,std::int32_t presentationLayout);
void RenderCars(World& world,const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    const GameTime& clock,std::int32_t presentationLayout,
    const hp2::CarDamageDisplayBindings* damageDisplay,CarViewMoverRegistry* views);
// PAL1242A8/123570: the attached camera's authored DASHGEOM pass, after
// rear-view composition and before fog. Geometry retains its loaded owner.
std::size_t RenderDashboards(const CarCinematicScreen& view,hp2::EcstasyFrame& frame,
    std::int32_t worldFrames);
#endif

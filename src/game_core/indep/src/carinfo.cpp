#include "../inc/CarInfo.hpp"
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "Language.hpp"
#include "indep/inc/Fred.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <math.h>
#include <cstring>

namespace hp2 {
void SnapshotRideInfo(CarAppearanceSelection& ride,ReplaySnapshot& snapshot)
{
    replaySnapshotBeginSection(&snapshot,nullptr,"RideInfo");
    replaySnapshotMoveData(&snapshot,&ride.modelCode,1,-1,0);
    replaySnapshotMoveData(&snapshot,&ride.skinNumber,1,1,1);
    replaySnapshotMoveData(&snapshot,&ride.dashboardTextures,1,1,1);
    replaySnapshotEndSection(&snapshot);
}

void SnapshotDriverInfo(RaceDriverInfo& driver,ReplaySnapshot& snapshot,
    const CarDriverAppearanceCatalogue& characters)
{
    replaySnapshotBeginSection(&snapshot,nullptr,"DriverInfo");
    replaySnapshotMoveData(&snapshot,&driver.eliminated,1,1,1);
    replaySnapshotMoveData(&snapshot,&driver.driverNumber,1,1,1);
    replaySnapshotMoveData(&snapshot,&driver.behaviorProfile,1,1,1);
    replaySnapshotMoveData(&snapshot,&driver.startingPosition,1,1,1);
    replaySnapshotMoveData(&snapshot,&driver.driverClass,1,-1,0);
    SnapshotRideInfo(driver.appearance,snapshot);
    replaySnapshotMoveData(&snapshot,&driver.characterIndex,1,1,1);
    if (driver.characterIndex!=-1 && snapshot.restoring)
        driver.character=carDriverCharacterAt(&characters,driver.characterIndex);
    replaySnapshotEndSection(&snapshot);
}
} // namespace hp2
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace hp2 {
namespace {
constexpr std::size_t kRecordSize = 0x560;
constexpr std::size_t kGeometryPathOffset = 0x40;
constexpr std::size_t kGeometryPathBytes = 0x20;

std::string Text(const std::uint8_t* p, std::size_t capacity) {
    const auto* end = std::find(p, p + capacity, std::uint8_t{0});
    return {reinterpret_cast<const char*>(p), static_cast<std::size_t>(end - p)};
}

float Float(const std::uint8_t* p) {
    const auto raw = ReadU32(p);
    float value;
    static_assert(sizeof(float) == 4, "The game stores IEEE-754 binary32 values");
    std::memcpy(&value, &raw, sizeof(value));
    if (!std::isfinite(value)) throw std::runtime_error("Non-finite PS2 car field");
    return value;
}

std::string Upper(std::string name) {
    for (auto& c:name) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return name;
}

bool GeometryPath(const std::uint8_t* p) {
    const auto name=Upper(Text(p,kGeometryPathBytes));
    return name.size()>19 && name.compare(0,5,"CARS\\")==0 &&
           name.size()>=12 && name.compare(name.size()-12,12,"GEOMETRY.BIN")==0;
}

CarDefinition ReadCar(const std::uint8_t* bytes, std::uint32_t offset) {
    CarDefinition car;
    car.recordOffset = offset;
    car.name=Text(bytes,0x20);
    car.baseName=Text(bytes+0x20,0x20);
    car.geometryPath=Text(bytes+0x40,0x20);
    car.geometryCompressedPath=Text(bytes+0x60,0x60);
    car.dashboardGeometryPath=Text(bytes+0xA0,0x20);
    std::memcpy(&car.selectionSortOrder,bytes+0xE4,1);
    car.defaultSkinNumber=static_cast<std::int32_t>(ReadU32(bytes+0x554));
    car.gauges.combined=static_cast<std::int32_t>(ReadU32(bytes+0x4d0));
    for (std::size_t axis=0;axis<2;++axis) {
        car.gauges.tachometerAngles[axis]=Float(bytes+0x4d4+axis*4);
        for (std::size_t units=0;units<2;++units) {
            car.gauges.speedometerAngles[units][axis]=Float(bytes+0x4e8+units*8+axis*4);
            car.gauges.speedometerRange[units][axis]=Float(bytes+0x4f8+units*8+axis*4);
        }
    }
    car.gauges.genericMaximum=Float(bytes+0x4e0);
    for (unsigned axis=0;axis<2;++axis) {
        car.gauges.tachometerCenter[axis]=static_cast<std::int16_t>(ReadU16(bytes+0x4e4+axis*2));
        car.gauges.speedometerCenter[axis]=static_cast<std::int16_t>(ReadU16(bytes+0x508+axis*2));
        car.gauges.speedTextPosition[axis]=static_cast<std::int16_t>(ReadU16(bytes+0x50c+axis*2));
        car.gauges.unitsPosition[axis]=static_cast<std::int16_t>(ReadU16(bytes+0x510+axis*2));
    }
    car.manufacturer=Text(bytes+0xC0,0x20);
    car.engineSound=Text(bytes+0x330,0x20);
    car.aiEngineSampleIndex=ReadU16(bytes+0x354);
    std::memcpy(car.playerEngineSamplePrefix.data(),bytes+0x356,8);
    car.enginePitchOrigin=ReadU16(bytes+0x35E);
    std::memcpy(car.engineThrottleRamp.data(),bytes+0x360,4);
    car.engineDetuneScale=static_cast<std::int16_t>(ReadU16(bytes+0x364));
    car.engineDetuneBias=static_cast<std::int16_t>(ReadU16(bytes+0x366));
    for (unsigned i=0;i<12;++i) {
        car.engineVoiceDetune[i]=static_cast<std::int16_t>(ReadU16(bytes+0x368+2*i));
        car.engineVoiceGain[i]=Float(bytes+0x380+4*i);
    }
    car.engineParameter3B0=static_cast<std::int16_t>(ReadU16(bytes+0x3B0));
    car.engineParameter3B2=static_cast<std::int16_t>(ReadU16(bytes+0x3B2));
    car.driveTrainInfo.shiftRpmFraction = Float(bytes + 0x270);
    car.driveTrainInfo.frontFinalDrive = Float(bytes + 0x278);
    car.driveTrainInfo.rearFinalDrive = Float(bytes + 0x27C);
    car.driveTrainInfo.rearDriveFraction = Float(bytes + 0x280);
    car.driveTrainInfo.clutchStiffness = Float(bytes + 0x284);
    car.driveTrainInfo.forwardGearCount = static_cast<int32_t>(ReadU32(bytes + 0x288));
    car.driveTrainInfo.clutchRate = Float(bytes + 0x28C);
    for (unsigned i = 0; i < 8; ++i)
        car.driveTrainInfo.gearRatios[i] = Float(bytes + 0x290 + 4 * i);
    car.rigidBodyInfo.mass = Float(bytes + 0x1E0);
    car.cameraTargetClearance = Float(bytes + 0x1EC);
    car.roadBlockDimensions = {Float(bytes + 0x1E4), Float(bytes + 0x1E8)};
    car.supportsDashboardTextures = bytes[0x80] != 0; // LB followed by a nonzero test.
    for (unsigned i = 0; i < car.selectionWeights.size(); ++i)
        car.selectionWeights[i] = Float(bytes + 0x520 + 4 * i);
    for (unsigned i = 0; i < car.driverAnimationOffset.size(); ++i)
        car.driverAnimationOffset[i] = Float(bytes + 0xF0 + 4 * i);
    for (unsigned i = 0; i < 16; ++i)
        car.rigidBodyInfo.inertia[i] = Float(bytes + 0x1F0 + 4 * i);
    for (unsigned i = 0; i < 4; ++i)
        car.centerOfMass[i] = Float(bytes + 0x110 + 4 * i);
    car.steeringGeometry = {Float(bytes + 0x120), Float(bytes + 0x180),
                            Float(bytes + 0x124), Float(bytes + 0x144)};
    car.aerodynamicsInfo = {Float(bytes + 0x2F0), Float(bytes + 0x2F4), Float(bytes + 0x2F8)};
    car.engineInfo.idleRpm = Float(bytes + 0x2B0);
    car.engineInfo.redlineRpm = Float(bytes + 0x2B4);
    car.engineInfo.maximumRpm = Float(bytes + 0x2B8);
    car.engineInfo.momentOfInertia = Float(bytes + 0x2BC);
    for (unsigned i = 0; i < 9; ++i)
        car.engineInfo.torqueCurve[i] = Float(bytes + 0x2C0 + 4 * i);
    for (unsigned i = 0; i < 3; ++i)
        car.engineInfo.brakingCurve[i] = Float(bytes + 0x2E4 + 4 * i);
    car.forwardSpeedLimitMph=Float(bytes+0x320);
    car.ordinaryYawLimit = Float(bytes + 0x318);
    car.pursuitYawLimit = Float(bytes + 0x31C);
    car.preloadDownforceScale = Float(bytes + 0x2FC);
    car.yawBaseCoefficient = Float(bytes + 0x310);
    car.yawCollisionCoefficient = Float(bytes + 0x314);
    car.steeringRangeDegrees = Float(bytes + 0x300);
    car.pursuitConfiguration = bytes[0xE4] != 0;
    car.modelCode = static_cast<std::int32_t>(ReadU32(bytes + 0x534));
    car.bodyCategory = static_cast<std::int32_t>(ReadU32(bytes + 0x538));
    car.challengeAvailability = static_cast<std::int32_t>(ReadU32(bytes + 0x53c));
    car.pursuitAvailability = static_cast<std::int32_t>(ReadU32(bytes + 0x540));
    std::memcpy(&car.routeVehicleClass, bytes + 0xE6, sizeof(car.routeVehicleClass));
    Require(car.routeVehicleClass >= 0 && car.routeVehicleClass <= 9,
            "Car type has an invalid authored road class");
    for (unsigned layout = 0; layout < 2; ++layout) {
        auto& count = car.cameraCatalogue.viewCount[layout];
        count = static_cast<std::int32_t>(ReadU32(bytes + 0x3C0 + layout * 4));
        Require(count >= 0 && count <= 4, "Car type has an invalid authored camera count");
        for (std::int32_t index = 0; index < count; ++index) {
            const auto* record = bytes + 0x3D0 + layout * 128 + index * 32;
            auto& view = car.cameraCatalogue.views[layout][index];
            view.kind = static_cast<std::int32_t>(ReadU32(record));
            for (unsigned axis = 0; axis < 3; ++axis)
                view.offset[axis] = Float(record + 4 + axis * 4);
            for (unsigned parameter = 0; parameter < 8; ++parameter)
                view.parameters[parameter] = ReadU16(record + 16 + parameter * 2);
        }
    }
    car.brakeInfo = {Float(bytes + 0x304), Float(bytes + 0x308)};
    for (unsigned axle = 0; axle < 2; ++axle) {
        const auto* tire = bytes + 0x1A0 + axle * 0x20;
        car.tireInfo[axle] = {Float(tire), Float(tire + 4), Float(tire + 8),
                              Float(tire + 12), Float(tire + 16)};
        const auto* suspension = bytes + 0x230 + axle * 0x20;
        car.suspensionInfo[axle] = {
            Float(suspension + 0x14), Float(suspension + 0x1C),
            Float(suspension), Float(suspension + 4), Float(suspension + 8),
            Float(suspension + 0x0C), Float(suspension + 0x10), Float(suspension + 0x18)
        };
    }
    for (unsigned i=0;i<4;++i) {
        const auto* w=bytes+0x120+i*0x20;
        auto& wheel=car.wheels[i];
        for (unsigned a=0;a<3;++a) wheel.position[a]=Float(w+a*4);
        wheel.radius=Float(w+0x0C);
        wheel.halfWidth=Float(w+0x10);
        wheel.meshIndex=ReadU32(w+0x14);
        wheel.sideValue=Float(w+0x18);
        wheel.unknown=Float(w+0x1C);
        auto& info = car.wheelInfo[i];
        info.reactionScale = wheel.radius;
        std::copy(wheel.position.begin(), wheel.position.end(), info.mountPosition);
        info.mountPosition[3] = wheel.radius;
        info.halfWidth = wheel.halfWidth;
        info.index = static_cast<int32_t>(wheel.meshIndex);
        if (wheel.radius<=0 || wheel.radius>5 || wheel.halfWidth<=0 || wheel.halfWidth>5)
            throw std::runtime_error("Malformed PS2 wheel dimensions: "+car.name);
    }
    if (car.name.empty() || car.baseName.empty() || car.driveTrainInfo.forwardGearCount<=0 ||
        car.driveTrainInfo.forwardGearCount>6 ||
        !GeometryPath(bytes+kGeometryPathOffset))
        throw std::runtime_error("Malformed PS2 GLOBALB car definition");
    carPerformanceInitialize(&car.performance);
    const auto physics = car.PhysicsInfo();
    if (!carPerformanceBuild(&car.performance, &physics, &carPerformanceDefaultTuning))
        throw std::runtime_error("Invalid original car performance configuration: " + car.name);
    return car;
}

constexpr std::array<std::int32_t, 12> kDemoCars = {
    45, 23, 47, 21, 25, 11, 32, 50, 51, 53, 55, 57
};

std::int8_t SignedByte(const std::uint8_t* data) noexcept {
    std::int8_t value;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

void CheckChunk(const std::vector<std::uint8_t>& data, const Chunk& chunk) {
    Require(Inside(chunk.headerOffset, 8, data.size()), "CarInfo chunk header lies outside package");
    Require(chunk.payloadOffset == chunk.headerOffset + 8 &&
            chunk.endOffset == chunk.payloadOffset + chunk.size &&
            Inside(chunk.payloadOffset, chunk.size, data.size()),
            "CarInfo chunk extent is inconsistent");
    Require(ReadU32(data.data() + chunk.headerOffset) == chunk.id &&
            ReadU32(data.data() + chunk.headerOffset + 4) == chunk.size,
            "CarInfo chunk descriptor differs from resource header");
}

void LoadChildren(CarInfoResource& owner, const std::vector<std::uint8_t>& data,
                  std::size_t begin, std::size_t length,
                  CarInfoAvailability availability, unsigned depth) {
    Require(depth < 32, "CarInfo resource containers are nested too deeply");
    for (const auto& chunk : ReadChunks(data, begin, length)) {
        if (chunk.IsContainer())
            LoadChildren(owner, data, chunk.payloadOffset, chunk.size, availability, depth + 1);
        else
            owner.LoadChunk(data, chunk, availability);
    }
}
} // namespace

CarInfoResource::CarInfoResource() : cars_(CarCount) {
    // Final PAL30D578: the two distinct full/short IDs are retained with the model key.
    // This owner is mutable; ELF write flags are not treated as immutability.
    languageInfo_={{
        {-1,41300,41100,42002,43002,43102,44002},
        {0,41300,41100,42002,43002,43102,44002},
        {2,41301,41101,42003,43003,43103,44003},
        {4,41302,41102,42004,43004,43104,44004},
        {5,41303,41103,42006,43006,43106,44006},
        {7,41304,41104,42007,43007,43107,44007},
        {9,41305,41105,42008,43008,43108,44008},
        {11,41306,41106,42009,43009,43109,44009},
        {13,41307,41107,42010,43010,43110,44010},
        {15,41308,41108,42011,43011,43111,44011},
        {17,41309,41109,42012,43012,43112,44012},
        {19,41310,41110,42013,43013,43113,44013},
        {20,41311,41111,42014,43014,43114,44014},
        {21,41312,41112,42015,43015,43115,44015},
        {23,41313,41113,42016,43016,43116,44016},
        {27,41314,41114,42017,43017,43117,44017},
        {29,41315,41115,42018,43018,43118,44018},
        {31,41316,41116,42019,43019,43119,44019},
        {32,41317,41117,42020,43020,43120,44020},
        {33,41318,41118,42021,43021,43121,44021},
        {34,41319,41119,42022,43022,43122,44022},
        {35,41320,41120,42023,43023,43123,44023},
        {37,41321,41121,42024,43024,43124,44024},
        {39,41322,41122,42025,43025,43125,44025},
        {41,41323,41123,42026,43026,43126,44026},
        {43,41324,41124,42027,43027,43127,44027},
        {45,41325,41125,42028,43028,43128,44028},
        {47,41326,41126,42029,43029,43129,44029},
        {25,41327,41127,42041,43041,43141,44041},
        {1,41400,41200,42002,43002,43102,44002},
        {3,41401,41201,42003,43003,43103,44003},
        {6,41402,41202,42006,43006,43106,44006},
        {8,41403,41203,42007,43007,43107,44007},
        {10,41404,41204,42008,43008,43108,44008},
        {12,41405,41205,42009,43009,43109,44009},
        {14,41406,41206,42010,43010,43110,44010},
        {16,41407,41207,42011,43011,43111,44011},
        {18,41408,41208,42012,43012,43112,44012},
        {24,41412,41212,42016,43016,43116,44016},
        {28,41413,41213,42017,43017,43117,44017},
        {30,41414,41214,42018,43018,43118,44018},
        {36,41415,41215,42023,43023,43123,44023},
        {38,41416,41216,42024,43024,43124,44024},
        {40,41417,41217,42025,43025,43125,44025},
        {42,41418,41218,42026,43026,43126,44026},
        {44,41419,41219,42027,43027,43127,44027},
        {46,41420,41220,42028,43028,43128,44028},
        {48,41421,41221,42029,43029,43129,44029},
        {26,41422,41222,42041,43041,43141,44041},
        {22,41411,41211,42042,43042,43142,44042}
    }};
    for (auto& car : cars_) carPerformanceInitialize(&car.performance);
    InitCarInfo();
}

CarInfoResource::CarInfoResource(const GameResources& resources,
                               CarInfoAvailability availability)
    : CarInfoResource(resources.Load("GLOBAL/GLOBALB.LZC"), availability) {}

CarInfoResource::CarInfoResource(const std::vector<std::uint8_t>& package,
                               CarInfoAvailability availability)
    : CarInfoResource() {
    LoadPackage(package, availability);
}

void CarInfoResource::LoadPackage(const std::vector<std::uint8_t>& package,
                                 CarInfoAvailability availability) {
    LoadChildren(*this, package, 0, package.size(), availability, 0);
    Require(carsLoaded_, "GLOBALB contains no CarInfo chunk");
    Require(skinsLoaded_, "GLOBALB contains no SkinInfo chunk");
}

bool CarInfoResource::LoadChunk(const std::vector<std::uint8_t>& package,
                               const Chunk& chunk, CarInfoAvailability availability) {
    if (chunk.id != CarChunk && chunk.id != SkinChunk) return false;
    CheckChunk(package, chunk);
    if (chunk.id == CarChunk) LoadCars(package, chunk, availability);
    else LoadSkins(package, chunk);
    return true;
}

void CarInfoResource::LoadCars(const std::vector<std::uint8_t>& package,
                              const Chunk& chunk, CarInfoAvailability availability) {
    Require(availability == CarInfoAvailability::FullGame || availability == CarInfoAvailability::DemoDisc,
            "CarInfo availability policy is not bound");
    Require(chunk.headerOffset <= std::numeric_limits<std::size_t>::max() - 23,
            "CarInfo alignment overflows");
    const auto begin = (chunk.headerOffset + 23) & ~std::size_t{15};
    Require(begin >= chunk.payloadOffset && begin <= chunk.endOffset &&
            chunk.endOffset - begin == CarCount * kRecordSize,
            "CarInfo chunk must contain exactly 64 aligned records");
    Require(begin <= std::numeric_limits<std::uint32_t>::max() - CarCount * kRecordSize,
            "CarInfo resource offset does not fit the authored offset domain");

    // Decode/build all records before applying the source demo geometry gate.
    // Temporary staging also leaves existing resource state intact on a bad record.
    std::vector<CarDefinition> definitions;
    definitions.reserve(CarCount);
    std::array<Appearance, CarCount> appearances{};
    std::unordered_set<std::string> names;
    for (std::size_t index = 0; index < CarCount; ++index) {
        const auto offset = begin + index * kRecordSize;
        const auto* record = package.data() + offset;
        auto car = ReadCar(record, static_cast<std::uint32_t>(offset));
        Require(car.modelCode == static_cast<std::int32_t>(index),
                "CarInfo model code differs from its array position");
        Require(names.insert(Upper(car.name)).second, "Duplicate CarInfo model name");
        auto& appearance = appearances[index];
        appearance.visual.modelCode = car.modelCode;
        appearance.visual.driverAnimationCode = SignedByte(record + 0xE7);
        const auto skinCount = SignedByte(record + 0xE5);
        Require(skinCount >= 0 && skinCount <= 8, "CarInfo skin choice count is invalid");
        appearance.skins.count = static_cast<std::uint8_t>(skinCount);
        for (std::size_t choice = 0; choice < appearance.skins.numbers.size(); ++choice)
            appearance.skins.numbers[choice] = SignedByte(record + 0x544 + choice);
        definitions.push_back(std::move(car));
    }
    if (availability == CarInfoAvailability::DemoDisc) {
        for (std::size_t index = 0; index < CarCount; ++index)
            if (std::find(kDemoCars.begin(), kDemoCars.end(), static_cast<std::int32_t>(index)) == kDemoCars.end())
                definitions[index].geometryPath.clear();
    }
    for (std::size_t index = 0; index < CarCount; ++index) {
        cars_[index] = std::move(definitions[index]);
        appearance_[index] = appearances[index];
    }
    carsLoaded_ = true;
    InitCarInfo();
}

CarLanguageInfo* CarInfoResource::FindCarLanguageInfo(std::int32_t modelCode)
{
    for (auto& descriptor:languageInfo_)if (descriptor.modelCode==modelCode)return &descriptor;
    return &languageInfo_.front();
}

void CarInfoResource::InitCarInfo()
{
    for (std::size_t index=0;index<CarCount;++index)
        cars_[index].languageInfo=FindCarLanguageInfo(static_cast<std::int32_t>(index));
    // The original tail188CA8 queries skin counts, without writing any state.
    for (std::size_t index=0;index<CarCount;++index)
        if (!cars_[index].geometryPath.empty())(void)AvailableSkinCount(index);
}

const char* CarDefinition::GetShortName(const LanguageManager& language) const
{
    // The shipped language table has no traffic names. Its legacy fallback
    // points at the first racer; the native player option must identify the
    // actual selected traffic model instead of displaying that other car.
    if(bodyCategory==2)return name.c_str();
    Require(languageInfo,"Car short name has no canonical language descriptor");
    return language.GetString(languageInfo->shortNameLanguageId);
}

const char* CarDefinition::GetFullName(const LanguageManager& language) const
{
    if(bodyCategory==2)return name.c_str();
    Require(languageInfo,"Car full name has no canonical language descriptor");
    return language.GetString(languageInfo->fullNameLanguageId);
}

void CarInfoResource::LoadSkins(const std::vector<std::uint8_t>& package,
                               const Chunk& chunk) {
    constexpr std::size_t recordSize = 64;
    Require(chunk.size % recordSize == 0, "SkinInfo chunk has a partial record");
    std::vector<CarSkin> skins;
    skins.reserve(chunk.size / recordSize);
    std::unordered_set<std::uint64_t> keys;
    for (std::size_t offset = chunk.payloadOffset; offset < chunk.endOffset; offset += recordSize) {
        const auto* record = package.data() + offset;
        CarSkin skin;
        skin.modelCode = static_cast<std::int32_t>(ReadU32(record));
        skin.skinNumber = static_cast<std::int32_t>(ReadU32(record + 4));
        skin.displayName = Text(record + 8, 32);
        skin.selectionMask = ReadU32(record + 40);
        skin.lightMaterialKey = ReadU32(record + 44);
        skin.authoredWord30 = ReadU32(record + 48);
        Require(skin.modelCode >= 0 && skin.modelCode < static_cast<std::int32_t>(CarCount) &&
                skin.skinNumber >= 0 && !skin.displayName.empty(), "SkinInfo identifier is invalid");
        Require(ReadU32(record + 52) == 0 && ReadU32(record + 56) == 0 && ReadU32(record + 60) == 0,
                "SkinInfo contains unsupported nonzero reserved file words");
        const auto key = std::uint64_t(static_cast<std::uint32_t>(skin.modelCode)) << 32 |
                         static_cast<std::uint32_t>(skin.skinNumber);
        Require(keys.insert(key).second, "Duplicate SkinInfo model/skin identifier");
        skins.push_back(std::move(skin));
    }
    skins_.swap(skins);
    skinsLoaded_ = true;
}

bool CarInfoResource::UnloadChunk(std::uint32_t kind) noexcept {
    if (kind == CarChunk) {
        carsLoaded_ = false;
        // The original static acceleration tables survive this unbind.
        return true;
    }
    if (kind == SkinChunk) {
        skins_.clear();
        skinsLoaded_ = false;
        return true;
    }
    return false;
}

const std::vector<CarDefinition>& CarInfoResource::Cars() const noexcept {
    static const std::vector<CarDefinition> empty;
    return carsLoaded_ ? cars_ : empty;
}

const CarDefinition& CarInfoResource::Car(std::size_t modelCode) const {
    Require(carsLoaded_, "CarInfo is not loaded");
    Require(modelCode < CarCount, "CarInfo model code is outside 0..63");
    return cars_[modelCode];
}

RigidBodyInfo* CarInfoResource::MutableRigidBodyInfo(std::size_t modelCode) {
    Require(carsLoaded_ && cars_.size() == CarCount, "Physical properties require the loaded canonical CarInfo owner");
    Require(modelCode < CarCount, "Physical property model code is outside 0..63");
    return &cars_[modelCode].rigidBodyInfo;
}

const CarDefinition& CarInfoResource::Find(const std::string& name) const {
    const auto wanted = Upper(name);
    for (const auto& car : Cars())
        if (Upper(car.name) == wanted) return car;
    throw std::runtime_error("Car not present in CarInfo resource: " + name);
}

bool CarInfoResource::IsAvailable(std::size_t modelCode) const {
    return !Car(modelCode).geometryPath.empty();
}

const CarDriverVisualVehicleInfo& CarInfoResource::VisualInfo(std::size_t modelCode) const {
    (void)Car(modelCode);
    return appearance_[modelCode].visual;
}

const CarDriverVisualVehicleInfo& CarInfoResource::VisualInfo(const std::string& name) const {
    return VisualInfo(static_cast<std::size_t>(Find(name).modelCode));
}

const CarSkinChoices& CarInfoResource::AuthoredSkins(std::size_t modelCode) const {
    (void)Car(modelCode);
    return appearance_[modelCode].skins;
}

const CarSkin* CarInfoResource::FindSkin(std::int32_t modelCode, std::int32_t skinNumber,
                                       bool allowFirstFallback) const noexcept {
    for (const auto& skin : skins_)
        if (skin.modelCode == modelCode && skin.skinNumber == skinNumber) return &skin;
    return allowFirstFallback && !skins_.empty() ? &skins_.front() : nullptr;
}

std::int32_t CarInfoResource::AvailableSkinCount(std::size_t modelCode) const {
    return Car(modelCode).bodyCategory == 1 ? 1 : appearance_[modelCode].skins.count;
}

std::int32_t CarInfoResource::AvailableSkinNumber(std::size_t modelCode, std::int32_t selection,
                                                bool alternatePursuitSkin) const {
    if (Car(modelCode).bodyCategory == 1) return alternatePursuitSkin ? 2 : 1;
    const auto& choices = appearance_[modelCode].skins;
    Require(selection >= 0 && choices.count != 0, "Invalid CarInfo skin selection");
    return choices.numbers[static_cast<std::size_t>(selection) % choices.count];
}

std::int32_t CarInfoResource::RandomSkin(std::size_t modelCode, bool alternatePursuitSkin) const {
    if (Car(modelCode).bodyCategory == 1)
        return AvailableSkinNumber(modelCode, 0, alternatePursuitSkin);
    const auto& choices = appearance_[modelCode].skins;
    if (choices.count == 0) return 0;
    return choices.numbers[vectorDefaultRandomBounded(choices.count)];
}

bool CarInfoResource::WeightedPickRide(CarAppearanceSelection& output, std::int32_t bodyCategory,
    std::int32_t routeVehicleClass, const RaceParameters& parameters,
    const LanguageManager& language) const {
    Require(carsLoaded_ && cars_.size() == CarCount, "Weighted car selection requires its canonical CarInfo owner");
    const std::int32_t category = parameters.selectedScenario / 10;
    const std::int32_t weight = category == 6 ? 4 : category - 1;
    Require(weight >= 0 && weight < 5, "Race selection has no valid authored car-weight column");
    const auto eligible = [&](const CarDefinition& car) {
        return car.bodyCategory == bodyCategory && !car.geometryPath.empty() &&
            (routeVehicleClass == -1 || car.routeVehicleClass == routeVehicleClass) &&
            AvailableSkinCount(static_cast<std::size_t>(car.modelCode)) > 0;
    };
    float total = 0.0f;
    for (const auto& car : cars_)
        if (eligible(car)) total = total + car.selectionWeights[static_cast<std::size_t>(weight)];
    if (std::fabs(total) <= 0.0010000000474974513f) return false;
    float remaining = vectorDefaultRandomFloat(total);
    for (const auto& car : cars_) {
        if (!eligible(car)) continue;
        remaining = remaining - car.selectionWeights[static_cast<std::size_t>(weight)];
        if (remaining <= 0.0f) {
            output.skinNumber = static_cast<std::int8_t>(RandomSkin(static_cast<std::size_t>(car.modelCode),
                language.currentLanguage != 0));
            output.modelCode = car.modelCode;
            output.reserved = 0;
            output.variant = 0;
            output.dashboardTextures = 0;
            return true;
        }
    }
    return false;
}

const CarPerformance& CarInfoResource::Acceleration(std::size_t modelCode) const {
    Require(modelCode < CarCount, "Acceleration table model code is outside 0..63");
    return cars_[modelCode].performance;
}

SampledCurve CarInfoResource::AccelerationCurve(std::size_t modelCode) const {
    return carPerformanceCurve(&Acceleration(modelCode));
}

PhysicsCarInfo CarDefinition::PhysicsInfo() const noexcept {
    PhysicsCarInfo info{};
    info.body = &rigidBodyInfo;
    info.centerOfMass = centerOfMass.data();
    info.steering = &steeringGeometry;
    info.engine = &engineInfo;
    info.driveTrain = &driveTrainInfo;
    info.aerodynamics = &aerodynamicsInfo;
    info.wheels = wheelInfo.data();
    info.tires = tireInfo.data();
    info.brakes = &brakeInfo;
    info.suspension = suspensionInfo.data();
    for (unsigned i = 0; i < 4; ++i) info.wheelSideOffset[i] = wheels[i].sideValue;
    info.forwardSpeedLimit = forwardSpeedLimitMph;
    info.ordinaryYawLimit = ordinaryYawLimit;
    info.pursuitYawLimit = pursuitYawLimit;
    info.preloadDownforceScale = preloadDownforceScale;
    info.yawBaseCoefficient = yawBaseCoefficient;
    info.yawCollisionCoefficient = yawCollisionCoefficient;
    info.pursuitConfiguration = pursuitConfiguration;
    info.modelCode = modelCode;
    info.routeVehicleClass = routeVehicleClass;
    info.steeringRangeDegrees = steeringRangeDegrees;
    info.bodyCategory = bodyCategory;
    info.driverAnimationOffset = driverAnimationOffset.data();
    return info;
}
} // namespace hp2

namespace hp2 {



RaceDriverInfo* InitializeDriverInfo(RaceDriverInfo& driver,std::int32_t number,
    std::int32_t driverClass,const CarAppearanceSelection& ride,
    std::int32_t characterIndex,std::int32_t behaviorProfile,std::int32_t startingPosition,
    const CarDriverAppearanceCatalogue& characters)
{
    driver.appearance.modelCode=-1;
    driver.appearance.reserved=0;driver.appearance.variant=0;
    driver.appearance.skinNumber=0;driver.appearance.dashboardTextures=0;
    const auto numberByte=static_cast<std::uint8_t>(number);
    const auto positionByte=static_cast<std::uint8_t>(startingPosition);
    std::memcpy(&driver.driverNumber,&numberByte,1);
    std::memcpy(&driver.startingPosition,&positionByte,1);
    driver.driverClass=driverClass;driver.eliminated=0;
    // Clearing precedes copying in PAL. A reference to this same appearance
    // therefore sees the cleared values; do not snapshot ride on entry.
    driver.appearance=ride;
    const auto characterByte=static_cast<std::uint8_t>(characterIndex);
    std::memcpy(&driver.characterIndex,&characterByte,1);
    driver.character=carDriverCharacterAt(&characters,characterIndex);
    const auto behaviorByte=static_cast<std::uint8_t>(behaviorProfile);
    std::memcpy(&driver.behaviorProfile,&behaviorByte,1);
    return &driver;
}

} // namespace hp2


int32_t carPerformanceBuild(CarPerformance* performance, const PhysicsCarInfo* info,
                             const CarPerformanceTuning* tuning)
{
    if (!performance || !info || !info->body || !info->driveTrain || !info->engine || !info->wheels ||
        !tuning || info->driveTrain->forwardGearCount < 1 || info->driveTrain->forwardGearCount > 6 ||
        !(info->body->mass > 0.0f) || !(tuning->sampleSpeedStep > 0.0f) ||
        !(tuning->maximumSampleSpeed > 0.0f)) return 0;
    const DriveTrainInfo* train = info->driveTrain;
    const float rearFraction = train->rearDriveFraction;
    const float frontFraction = 1.0f - rearFraction;
    const float frontDrive = frontFraction * train->frontFinalDrive;
    const float rearDrive = rearFraction * train->rearFinalDrive;
    const float finalDrive = frontDrive + rearDrive;
    const float frontRadius = frontFraction * info->wheels[0].reactionScale;
    const float rearRadius = rearFraction * info->wheels[2].reactionScale;
    const float radius = frontRadius + rearRadius;
    if (!(radius > 0.0f)) return 0;
    float upshift[9] = {0}, downshift[9] = {0};
    driveTrainBuildShiftTables(train, info->engine, upshift, downshift, &tuning->shift);
    SampledCurve torque;
    sampledCurveInitialize(&torque, info->engine->torqueCurve, 9, info->engine->idleRpm, info->engine->maximumRpm);

    /* The final PAL configuration has 101 temporary speed samples. A fixed
     * bound makes malformed tuning fail before it can overrun that storage. */
    float acceleration[101];
    int32_t count = 0;
    int32_t gear = upshift[2] <= 0.0f ? 3 : 2;
    float speed = 0.0f, rpm = 0.0f;
    if (gear >= train->forwardGearCount + 2) return 0;
    for (;;) {
        if (count == 101) return 0;
        const float outputTorque = sampledCurveEvaluate(&torque, rpm);
        speed = speed + tuning->sampleSpeedStep;
        const float axleTorque = outputTorque * finalDrive;
        const float gearTorque = axleTorque * train->gearRatios[gear];
        const float force = gearTorque / radius;
        acceleration[count++] = force / info->body->mass;
        if (!(speed < tuning->maximumSampleSpeed)) break;
        const float wheelVelocity = speed / radius;
        const float driveVelocity = wheelVelocity * finalDrive;
        const float engineVelocity = driveVelocity * train->gearRatios[gear];
        rpm = engineVelocity * tuning->angularVelocityToRpm;
        if (upshift[gear] <= rpm) {
            ++gear;
            if (gear >= train->forwardGearCount + 2) break;
        }
    }
    acceleration[count - 1] = 0.0f;
    SampledCurve generated;
    sampledCurveInitialize(&generated, acceleration, count, acceleration[count - 1], speed);
    for (int32_t index = 0; index < CAR_PERFORMANCE_SAMPLES; ++index) {
        const float scaled = (float)index * speed;
        const float coordinate = 0.0f + scaled / (float)(CAR_PERFORMANCE_SAMPLES - 1);
        performance->samples[index] = sampledCurveEvaluate(&generated, coordinate);
        if (!isfinite(performance->samples[index])) return 0;
    }
    performance->maximumSpeed = speed;
    return 1;
}

namespace hp2 {
namespace {
std::string TireName(const std::string& format, const std::string& name)
{
    std::string result;
    bool substituted = false;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%') { result.push_back(format[i]); continue; }
        Require(++i < format.size(), "Truncated tire texture format");
        if (format[i] == '%') result.push_back('%');
        else {
            Require(format[i] == 's' && !substituted, "Tire texture format requires one string");
            result += name; substituted = true;
        }
    }
    Require(substituted, "Tire texture format has no model name");
    return result;
}
}

CarUsedTextureInfo BuildCarTextureInfo(const CarInfoResource& info,
    const CarAppearanceSelection& selection, int frontend,
    const CarTextureNamingPolicy& naming)
{
    const auto& car = info.Car(static_cast<std::size_t>(selection.modelCode));
    CarUsedTextureInfo result;
    const auto key = [&car](const std::string& suffix) {
        return hashString((car.baseName + suffix).c_str());
    };
    const auto append = [&result](std::uint32_t value) {
        Require(result.count < result.usedKeys.size(), "Car texture list capacity exceeded");
        result.usedKeys[result.count++] = value;
    };
    result.defaultSkinKeys = {key("_SKIN1"), key("_SKIN1B")};
    const auto number = std::to_string(static_cast<int>(selection.skinNumber));
    result.skinKeys[0] = key("_SKIN" + number);
    append(result.skinKeys[0]);
    if (!frontend) {
        result.skinKeys[1] = key("_SKIN" + number + "B");
        append(result.skinKeys[1]);
    }
    if (selection.dashboardTextures) {
        result.defaultDashboardKey = key("_DASHSKIN1");
        result.dashboardKey = key("_DASHSKIN" + number);
        append(result.dashboardKey);
    }
    result.headlightSideKeys = {key("_HEADLIGHT_LEFT"), key("_HEADLIGHT_RIGHT")};
    result.brakeSideKeys = {key("_BRAKELIGHT_LEFT"), key("_BRAKELIGHT_RIGHT")};
    result.reverseLightKey = key("_REVERSELIGHT");
    const std::array<const char*,9> lighting = {"_HEADLIGHT", "_HEADLIGHT_ON", "_HEADLIGHT_DAMAGE0",
        "_BRAKELIGHT", "_BRAKELIGHT_ON", "_BRAKELIGHT_DAMAGE0", "_REVERSELIGHT", "_REVERSELIGHT_ON", "_REVERSELIGHT"};
    for (std::size_t index = 0; index < lighting.size(); ++index) {
        result.lightingKeys[index] = key(lighting[index]);
        append(result.lightingKeys[index]);
    }
    result.shadowKey = key(frontend ? "_SHADOWFE" : "_SHADOWIG");
    append(result.shadowKey);
    result.defaultTireKey = hashString(TireName(naming.tireTextureFormat, car.baseName).c_str());
    result.tireKey = hashString(TireName(naming.tireTextureFormat, car.name).c_str());
    result.defaultTireBlurKey = key("_WHEEL_BLUR");
    result.tireBlurKey = hashString((car.name + "_WHEEL_BLUR").c_str());
    append(result.tireKey); append(result.tireBlurKey);
    result.licensePlateKey = key("_LICENSE_PLATE");
    append(result.licensePlateKey);
    return result;
}
} // namespace hp2

namespace hp2 {
const char* RaceDriverName(const RaceDriverInfo& driver)
{
    // Complete final DriverInfo::GetName188770. Final330878 is empty.
    return driver.character?driver.character->name:"";
}
}

const CarPerformanceTuning carPerformanceDefaultTuning = {
    55.5555572509765625f, 138.8889007568359375f, 1.38888895511627197f, 9.54930400848388672f,
    {1, 0.949999988079071f, 0.949999988079071f,
     0.3499999940395355f, 300.0f, 50.0f, 1.2000000476837158f}
};

CarPerformance* carPerformanceInitialize(CarPerformance* performance)
{
    for (int32_t index = 0; index < CAR_PERFORMANCE_SAMPLES - 1; ++index)
        performance->samples[index] = 4.0f;
    performance->samples[CAR_PERFORMANCE_SAMPLES - 1] = 0.0f;
    performance->maximumSpeed = carPerformanceDefaultTuning.defaultMaximumSpeed;
    return performance;
}

SampledCurve carPerformanceCurve(const CarPerformance* performance)
{
    SampledCurve curve;
    sampledCurveInitialize(&curve, performance->samples, CAR_PERFORMANCE_SAMPLES, 0.0f, performance->maximumSpeed);
    return curve;
}

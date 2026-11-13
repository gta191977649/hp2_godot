#ifndef NFSHP2_INDEP_CAR_DAMAGE_HPP
#define NFSHP2_INDEP_CAR_DAMAGE_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t physicsGetPacked2Bit(uint32_t value, uint32_t index);

enum { BODY_DAMAGE_SECTORS = 32, BODY_DAMAGE_MODELS = 4, CAR_DAMAGE_ITEMS = 15 };

struct EditorMarkerModel;
struct ReplaySnapshot;
struct BodyDamage;
struct Car;
struct PositionMarkerRegistry;
struct RaceParameters;

typedef struct BodyDamageInfo {
    float minimum[2];
    float maximum[2];
    struct BodyDamageInfo* previous;
    struct BodyDamageInfo* next;
    uint32_t key;
    size_t vertexCount;
    const uint32_t* runs;
    size_t runCount;
} BodyDamageInfo;

/* The original model slot borrows its live eModel and loaded damage record.
 * Positions and RGB555 remain in the same geometry used by rendering. */
typedef struct BodyDamageModel {
    struct BodyDamage* owner;
    struct EditorMarkerModel* model;
    const BodyDamageInfo* info;
    size_t vertexCount;
    int32_t deformationDisabled;
    uint32_t repeat;
    size_t runIndex;
} BodyDamageModel;

typedef struct BodyDamage {
    struct BodyDamage* previous;
    struct BodyDamage* next;
    uint8_t sectors[BODY_DAMAGE_SECTORS];
    uint16_t total;
    uint8_t appliedSectors[BODY_DAMAGE_SECTORS];
    uint16_t appliedTotal;
    const BodyDamageInfo* info;
    BodyDamageModel models[BODY_DAMAGE_MODELS];
    size_t modelCount;
    /* Final31A8D8 is RaceParameters+40. Every query reads that same live
     * owner; no per-Car enabled copy can go stale after a settings event. */
    const struct RaceParameters* settings;
#ifdef __cplusplus
    explicit BodyDamage(const char* name = nullptr);
    ~BodyDamage();
    BodyDamage(const BodyDamage&) = delete;
    BodyDamage& operator=(const BodyDamage&) = delete;
    BodyDamage(BodyDamage&&) = delete;
    BodyDamage& operator=(BodyDamage&&) = delete;
#endif
} BodyDamage;

typedef struct BodyDamageList {
    BodyDamage* first;
    BodyDamage* last;
} BodyDamageList;
extern BodyDamageList bodyDamageList;

typedef struct CarDamageItemInfo {
    uint32_t attachmentKey;
    float thresholds[4];
    /* PAL313B80 row+18 gates keyed environment mapping; row+1C is CarFX. */
    int32_t changesEnvironmentMapping;
    int32_t effectId;
} CarDamageItemInfo;

typedef struct CarDamageItemState {
    uint8_t level;
    int32_t environmentMappingEnabled;
    uint64_t revision;
} CarDamageItemState;

typedef struct CarDamageEvent {
    uint8_t item;
    uint8_t level;
    int32_t detached;
    int32_t effectId;
    float strength;
    uint64_t revision;
} CarDamageEvent;

typedef struct CarDamage {
    BodyDamage body;
    uint32_t carType;
    int8_t itemSectors[CAR_DAMAGE_ITEMS];
    uint32_t levels;
    uint32_t appliedLevels;
    CarDamageItemState items[CAR_DAMAGE_ITEMS];
    CarDamageEvent events[CAR_DAMAGE_ITEMS];
    uint64_t revision;
    struct Car* car;
    uint32_t markerGroupKey;
#ifdef __cplusplus
    CarDamage();
    CarDamage(struct Car* owner,const char* modelName);
    ~CarDamage();
    CarDamage(const CarDamage&) = delete;
    CarDamage& operator=(const CarDamage&) = delete;
#endif
} CarDamage;

extern const CarDamageItemInfo carDamageDefaultItems[CAR_DAMAGE_ITEMS];
void carDamageInitSectorPositions(CarDamage* damage,const struct PositionMarkerRegistry* markers);
void carDamageSnapshot(CarDamage* damage,struct ReplaySnapshot* snapshot);

int32_t bodyDamageLoadInfo(uint32_t chunkKind, BodyDamageInfo* info);
int32_t bodyDamageUnloadInfo(uint32_t chunkKind, BodyDamageInfo* info);
const BodyDamageInfo* bodyDamageFindInfo(uint32_t key);
void bodyDamageSetALevelInfo(BodyDamage* damage, const char* name);
void bodyDamageModelInitialize(BodyDamageModel* state, BodyDamage* damage,
    struct EditorMarkerModel* model, const char* name, int32_t deformationDisabled);
void bodyDamageModelApply(BodyDamageModel* model);
void bodyDamageAddModel(BodyDamage* damage, struct EditorMarkerModel* model,
    const char* name, int32_t deformationDisabled);
void bodyDamageSnapshot(BodyDamage* damage, struct ReplaySnapshot* snapshot);
void bodyDamageSetEnvironmentMapping(BodyDamage* damage, uint32_t textureKey, int32_t enabled);
void bodyDamageSetSector(BodyDamage* damage, int32_t sector, float amount);
int32_t bodyDamageFindSector(const BodyDamage* damage, const float point[2]);
float bodyDamageAtPoint(const BodyDamage* damage, const float point[2]);
int32_t bodyDamageEnabled(const BodyDamage* damage);
float bodyDamageVisibleAtPoint(const BodyDamage* damage, const float point[2]);
float bodyDamageVisibleSector(const BodyDamage* damage, int32_t sector);
uint32_t bodyDamageCompressedSector(const BodyDamage* damage, int32_t sector);
float bodyDamageVisibleTotal(const BodyDamage* damage);
void bodyDamageApply(BodyDamage* damage);
void bodyDamageRepair(BodyDamage* damage);
int32_t bodyDamageDeformVertex(BodyDamageModel* model, int32_t vertexIndex,
    float position[3], uint16_t* color, float* textureCoordinates, int32_t damageFlag);
CarDamage* carDamageInitialize(CarDamage* damage, const BodyDamageInfo* info,
                               uint32_t carType, const struct RaceParameters* settings);
void carDamageSetItemPositions(CarDamage* damage, const float positions[CAR_DAMAGE_ITEMS][2],
                                const uint8_t present[CAR_DAMAGE_ITEMS]);
uint32_t carDamageRecalculateItems(const CarDamage* damage);
void carDamageAddToSector(CarDamage* damage, int32_t sector, float amount, float impact);
void carDamageAdd(CarDamage* damage, const float point[2], float amount);
void carDamageApply(CarDamage* damage);
void carDamageRethink(CarDamage* damage);
void carDamageSetFull(CarDamage* damage);
void carDamageSetZero(CarDamage* damage);
uint32_t carDamageItemLevel(uint32_t levels, uint32_t item);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cstdint>
#include <string>
#include <vector>

namespace hp2 {

class CarDamageData final {
public:
    explicit CarDamageData(const std::vector<std::uint8_t>& geometry);
    ~CarDamageData();
    const BodyDamageInfo* Find(const std::string& name) const noexcept;
    std::size_t DefinitionCount() const noexcept { return definitions_.size(); }
    CarDamageData(const CarDamageData&) = delete;
    CarDamageData& operator=(const CarDamageData&) = delete;
    CarDamageData(CarDamageData&&) = delete;
    CarDamageData& operator=(CarDamageData&&) = delete;

private:
    struct Definition {
        std::string name;
        BodyDamageInfo bounds{};
        std::vector<std::uint32_t> runs;
    };
    std::vector<Definition> definitions_;
};

} // namespace hp2

#endif
#endif

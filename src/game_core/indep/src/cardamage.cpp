#include "indep/inc/CarDamage.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>
#include <string.h>
#include "indep/inc/Car.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/Collision.hpp"

CarDamage::CarDamage()
    : body(nullptr),carType(0),itemSectors{},levels(0),appliedLevels(0),
      items{},events{},revision(0),car(nullptr),markerGroupKey(0)
{
    // Native storage initialization for callers that subsequently bind their
    // existing damage owner; no original constructor is claimed for this API.
    memset(itemSectors,-1,sizeof(itemSectors));
    for (auto& item:items)item.environmentMappingEnabled=1;
}

CarDamage::CarDamage(Car* owner,const char* modelName)
    : body(modelName),carType(0),itemSectors{},levels(0),appliedLevels(0),
      items{},events{},revision(0),car(owner),markerGroupKey(0)
{
    // PAL1B54B0 constructs its actual BodyDamage and retains the same Car.
    hp2::Require(owner && owner->driverInfo && modelName,"CarDamage construction has no Car/type owner");
    markerGroupKey=hashString(modelName);
    // Existing accepted item rules use the decoded model index rather than
    // repeatedly following the console Car->CarInfo pointer.
    carType=static_cast<uint32_t>(owner->driverInfo->appearance.modelCode);
    memset(itemSectors,-1,sizeof(itemSectors));
    for (auto& item:items)item.environmentMappingEnabled=1;
}

CarDamage::~CarDamage()
{
    // PAL1B5538 first publishes undamaged breakables, then BodyDamage's real
    // destructor repairs the model geometry and removes its live-list node.
    levels=0;
    carDamageApply(this);
}

void carDamageInitSectorPositions(CarDamage* damage,const PositionMarkerRegistry* markers)
{
    // PAL1B5598 uses actual34013 markers, independently of skeletal34007.
    hp2::Require(damage && markers,"Damage sectors have no actual marker registry");
    for (std::size_t item=0;item<CAR_DAMAGE_ITEMS;++item) {
        damage->itemSectors[item]=-1;
        const auto* marker=positionMarkerFind(markers,carDamageDefaultItems[item].attachmentKey,
            damage->markerGroupKey,nullptr);
        if (marker)damage->itemSectors[item]=static_cast<int8_t>(bodyDamageFindSector(&damage->body,marker->position));
    }
}

// Complete PAL1B5638..1B56AC: the live model damage and detachable-item
// state remain one owner, and restoration reapplies their visual changes.
void carDamageSnapshot(CarDamage* damage,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,damage,"CarDamage");
    bodyDamageSnapshot(&damage->body,snapshot);
    replaySnapshotMoveUnsigned(snapshot,&damage->levels,4);
    if(snapshot->restoring)carDamageApply(damage);
    replaySnapshotEndSection(snapshot);
}

const CarDamageItemInfo carDamageDefaultItems[CAR_DAMAGE_ITEMS] = {
    {0x05f63788u,{0,.25f,.5f,.75f},0,12},
    {0xc52bf01bu,{0,.25f,.5f,.75f},0,13},
    {0x725758bfu,{0,.25f,.5f,.75f},0,14},
    {0x494ed280u,{0,.25f,.5f,.75f},0,15},
    {0xa7e6ea53u,{0,.25f,.3499999940395355f,.5f},0,16},
    {0xa532fc46u,{0,.25f,.3499999940395355f,.5f},0,17},
    {0xd947f346u,{0,.25f,.44999998807907104f,.6499999761581421f},0,18},
    {0x02b52399u,{0,.25f,.44999998807907104f,.6499999761581421f},0,19},
    {0x7b220ddfu,{0,.25f,.5f,.75f},1,20},
    {0x1b0763a0u,{0,.25f,.5f,.75f},1,21},
    {0xe7e4ef49u,{0,.25f,.5f,.75f},1,22},
    {0x4cdebfcau,{0,.25f,.5f,.75f},1,23},
    {0x60f8b13cu,{0,.25f,.5f,.75f},1,24},
    {0x0ab88f5du,{0,.25f,.5f,.75f},1,25},
    {0x68100815u,{0,.699999988079071f,.8500000238418579f,.949999988079071f},0,6}
};

CarDamage* carDamageInitialize(CarDamage* damage, const BodyDamageInfo* info,
                               uint32_t carType, const RaceParameters* settings)
{
    // Bind the one real race policy before Repair consults visibility.
    // This is native owner initialization, not an additional ELF body.
    hp2::Require(damage && settings && (!damage->body.settings || damage->body.settings==settings),
        "Damage initialization must retain its canonical RaceParameters owner");
    damage->body.settings = settings;
    bodyDamageRepair(&damage->body);
    damage->body.info = info;
    damage->carType = carType;
    damage->levels = damage->appliedLevels = 0;
    memset(damage->items, 0, sizeof(damage->items));
    memset(damage->events, 0, sizeof(damage->events));
    damage->revision = 0;
    memset(damage->itemSectors, -1, sizeof(damage->itemSectors));
    for (int item = 0; item < CAR_DAMAGE_ITEMS; ++item) damage->items[item].environmentMappingEnabled = 1;
    return damage;
}

void carDamageSetItemPositions(CarDamage* damage, const float positions[CAR_DAMAGE_ITEMS][2],
                                const uint8_t present[CAR_DAMAGE_ITEMS])
{
    for (int item = 0; item < CAR_DAMAGE_ITEMS; ++item)
        damage->itemSectors[item] = present[item] ? (int8_t)bodyDamageFindSector(&damage->body, positions[item]) : -1;
}

uint32_t carDamageRecalculateItems(const CarDamage* damage)
{
    uint32_t result = 0;
    for (int item = 0; item < CAR_DAMAGE_ITEMS; ++item) {
        const int32_t sector = damage->itemSectors[item];
        if (sector < 0) continue;
        const float amount = bodyDamageVisibleSector(&damage->body, sector);
        const CarDamageItemInfo* info = &carDamageDefaultItems[item];
        int32_t level = 3;
        if (amount < info->thresholds[3]) {
            while (--level > 0 && amount < info->thresholds[level]) {}
        }
        result |= (uint32_t)level << (item * 2);
    }
    if (damage->carType == 19 || damage->carType == 20 || damage->carType == 31) result &= ~0x03ff0000u;
    return result;
}

void carDamageApply(CarDamage* damage)
{
    bodyDamageApply(&damage->body);
    if (damage->levels == damage->appliedLevels) return;
    for (uint32_t item = 0; item < CAR_DAMAGE_ITEMS; ++item) {
        const uint32_t level = carDamageItemLevel(damage->levels, item);
        if (level == carDamageItemLevel(damage->appliedLevels, item)) continue;
        CarDamageItemState* state = &damage->items[item];
        state->level = (uint8_t)level;
        if (carDamageDefaultItems[item].changesEnvironmentMapping) {
            // PAL1B5B24..1B5B3C: the same authored row key used by markers
            // selects the material. This does not hide a whole body mesh.
            state->environmentMappingEnabled = level == 0;
            bodyDamageSetEnvironmentMapping(&damage->body,
                carDamageDefaultItems[item].attachmentKey,state->environmentMappingEnabled);
        }
        state->revision = ++damage->revision;
        damage->appliedLevels = (damage->appliedLevels & ~(3u << (item * 2))) | (level << (item * 2));
    }
}

void carDamageAddToSector(CarDamage* damage, int32_t sector, float amount, float impact)
{
    const float previous = (float)damage->body.sectors[sector] * 0.0078125f;
    amount = minimumFinite(1.0f, maximumFinite(0.0f, previous + amount));
    if (amount == previous) return;
    bodyDamageSetSector(&damage->body, sector, amount);
    const uint32_t levels = carDamageRecalculateItems(damage);
    if (levels == damage->levels) return;
    if (impact < 0.20000000298023224f) {
        bodyDamageSetSector(&damage->body, sector, previous);
        return;
    }
    static const float strengths[4] = {0, 0.10000000149011612f, 0.4000000059604645f, 1};
    for (uint32_t item = 0; item < CAR_DAMAGE_ITEMS; ++item) {
        const uint32_t level = carDamageItemLevel(levels, item);
        if (level == carDamageItemLevel(damage->levels, item)) continue;
        CarDamageEvent* event = &damage->events[item];
        event->item = (uint8_t)item;
        event->level = (uint8_t)level;
        event->detached = level == 1;
        event->effectId = carDamageDefaultItems[item].effectId;
        event->strength = strengths[level];
        event->revision = ++damage->revision;
        // PAL1B59FC..1B5A30 emits these effects immediately at the logical
        // level transition. Rendering and snapshot Apply never replay them.
        // The native unbound damage helper has no Car; real instances keep
        // their actual movement/collision/audio and particle owners.
        if (damage->car) {
            auto* movement=damage->car->Movement();
            hp2::Require(movement && movement->bindings.state && movement->bindings.damage==damage,
                "Damage transition lost its canonical Car movement owner");
            if (level==1) {
                auto* collisions=movement->collisionSystem;
                hp2::Require(collisions && collisions->contactAudio,
                    "Damage glass effect lost its actual collision audio owner");
                hp2::Require(collisionAudioPlayGlass(collisions->contactAudio,
                    movement->bindings.state->position)==WORLD_AUDIO_OK,
                    "Damage glass effect failed on its actual positional audio owner");
            }
            if (event->effectId)
                hp2::Require(carTriggerEffect(movement,event->effectId,event->strength,0,nullptr)==CAR_EFFECT_OK,
                    "Damage burst failed on its actual CarFX owner");
        }
    }
    damage->levels = levels;
}

void carDamageAdd(CarDamage* damage, const float point[2], float amount)
{
    if (!bodyDamageEnabled(&damage->body) || amount < 0.05000000074505806f) return;
    const int32_t sector = bodyDamageFindSector(&damage->body, point);
    if (sector < 0) return;
    carDamageAddToSector(damage, sector, amount, amount);
    float neighboring = amount;
    for (int offset = 1; offset < 5; ++offset) {
        neighboring = neighboring * 0.5f;
        carDamageAddToSector(damage, (sector + offset) & 31, neighboring, amount);
        carDamageAddToSector(damage, (sector - offset) & 31, neighboring, amount);
    }
    carDamageApply(damage);
}

void carDamageRethink(CarDamage* damage)
{
    damage->levels = carDamageRecalculateItems(damage);
    carDamageApply(damage);
}

void carDamageSetFull(CarDamage* damage)
{
    for (int sector = 0; sector < BODY_DAMAGE_SECTORS; ++sector) carDamageAddToSector(damage, sector, 1.0f, 9999.0f);
    carDamageRethink(damage);
}

void carDamageSetZero(CarDamage* damage)
{
    bodyDamageRepair(&damage->body);
    carDamageRethink(damage);
}

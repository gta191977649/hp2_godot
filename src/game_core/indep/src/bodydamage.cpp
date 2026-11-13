#include "indep/inc/CarDamage.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Replay.hpp"

#include "bware/inc/bWare.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <math.h>
#include <string.h>

namespace {
BodyDamageInfo* firstDamageInfo{};
BodyDamageInfo* lastDamageInfo{};
}

BodyDamageList bodyDamageList{};

int32_t bodyDamageLoadInfo(uint32_t chunkKind, BodyDamageInfo* info)
{
    if (chunkKind != 0x34030u) return 0;
    info->previous = lastDamageInfo;
    info->next = nullptr;
    if (lastDamageInfo) lastDamageInfo->next = info;
    else firstDamageInfo = info;
    lastDamageInfo = info;
    return 1;
}

int32_t bodyDamageUnloadInfo(uint32_t chunkKind, BodyDamageInfo* info)
{
    if (chunkKind != 0x34030u) return 0;
    if (info->previous) info->previous->next = info->next;
    else firstDamageInfo = info->next;
    if (info->next) info->next->previous = info->previous;
    else lastDamageInfo = info->previous;
    return 1;
}

const BodyDamageInfo* bodyDamageFindInfo(uint32_t key)
{
    for (auto* info = firstDamageInfo; info; info = info->next)
        if (info->key == key) return info;
    return nullptr;
}

BodyDamage::BodyDamage(const char*)
    : previous(bodyDamageList.last), next(nullptr), sectors{}, total(0),
      appliedSectors{}, appliedTotal(0), info(nullptr), models{}, modelCount(0), settings(nullptr)
{
    if (previous) previous->next = this;
    else bodyDamageList.first = this;
    bodyDamageList.last = this;
}

BodyDamage::~BodyDamage()
{
    bodyDamageRepair(this);
    if (previous) previous->next = next;
    else bodyDamageList.first = next;
    if (next) next->previous = previous;
    else bodyDamageList.last = previous;
}

void bodyDamageSetALevelInfo(BodyDamage* damage, const char* name)
{
    damage->info = bodyDamageFindInfo(hashString(name));
}

void bodyDamageModelInitialize(BodyDamageModel* state, BodyDamage* damage,
    EditorMarkerModel* model, const char* name, int32_t deformationDisabled)
{
    state->info = nullptr;
    state->owner = damage;
    state->deformationDisabled = deformationDisabled;
    state->model = model;
    state->vertexCount = static_cast<size_t>(ecstasyModelNumVertices(model));
    if (!state->vertexCount) return;
    state->info = bodyDamageFindInfo(hashString(name));
    if (!state->info) return;
    if (state->info->vertexCount != state->vertexCount) {
        state->info = nullptr;
        return;
    }
    bodyDamageModelApply(state);
}

void bodyDamageModelApply(BodyDamageModel* model)
{
    if (!model->info) return;
    (void)ecstasyModelNumVertices(model->model);
    model->repeat = 0;
    model->runIndex = 0;
    ecstasyCallDamageVertexFunction(model->model, model, bodyDamageDeformVertex);
}

void bodyDamageAddModel(BodyDamage* damage, EditorMarkerModel* model,
    const char* name, int32_t deformationDisabled)
{
    hp2::Require(damage->modelCount < BODY_DAMAGE_MODELS,
        "BodyDamage's four original model slots are full");
    auto& state = damage->models[damage->modelCount++];
    bodyDamageModelInitialize(&state, damage, model, name, deformationDisabled);
}

void bodyDamageSnapshot(BodyDamage* damage, ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot, nullptr, "BodyDamage");
    replaySnapshotMoveData(snapshot, damage->sectors, BODY_DAMAGE_SECTORS, BODY_DAMAGE_SECTORS, 0);
    replaySnapshotMoveData(snapshot, &damage->total, 2, 2, 0);
    if (snapshot->restoring) bodyDamageApply(damage);
    replaySnapshotEndSection(snapshot);
}

void bodyDamageSetEnvironmentMapping(BodyDamage* damage, uint32_t textureKey, int32_t enabled)
{
    for (size_t index = 0; index < damage->modelCount; ++index)
        ecstasyModelSetEnvironmentMapping(damage->models[index].model, textureKey, enabled);
}

void bodyDamageSetSector(BodyDamage* damage, int32_t sector, float amount)
{
    if (1.0f < amount) amount = 1.0f;
    if (amount < 0.0f) amount = 0.0f;
    const uint8_t encoded = (uint8_t)convertFloatToInt32(amount * 128.0f);
    damage->total = (uint16_t)(damage->total + encoded - damage->sectors[sector]);
    damage->sectors[sector] = encoded;
}

int32_t bodyDamageFindSector(const BodyDamage* damage, const float point[2])
{
    if (!damage->info) return -1;
    const BodyDamageInfo* info = damage->info;
    const float x = finiteRatio(point[0] - info->minimum[0], info->maximum[0] - info->minimum[0]);
    const float y = finiteRatio(point[1] - info->minimum[1], info->maximum[1] - info->minimum[1]);
    int32_t ix = convertFloatToInt32(x * 8.0f), iy = convertFloatToInt32(y * 8.0f);
    if (ix < 0) ix = 0;
    if (iy < 0) iy = 0;
    if (7 < ix) ix = 7;
    if (7 < iy) iy = 7;
    if (y < x) return 1.0f - y < x ? 8 - iy : 16 - ix;
    return 1.0f - y < x ? 24 + ix : 16 + iy;
}

float bodyDamageAtPoint(const BodyDamage* damage, const float point[2])
{
    const int32_t sector = bodyDamageFindSector(damage, point);
    return sector < 0 ? 0.0f : (float)damage->sectors[sector] * 0.0078125f;
}

int32_t bodyDamageEnabled(const BodyDamage* damage)
{
    hp2::Require(damage && damage->settings,"BodyDamage has no canonical RaceParameters damage policy");
    return damage->settings->authoredRaceOption40;
}

float bodyDamageVisibleAtPoint(const BodyDamage* damage, const float point[2])
{
    return bodyDamageEnabled(damage) ? bodyDamageAtPoint(damage, point) : 0.0f;
}

float bodyDamageVisibleSector(const BodyDamage* damage, int32_t sector)
{
    return bodyDamageEnabled(damage) ? (float)damage->sectors[sector] * 0.0078125f : 0.0f;
}

uint32_t bodyDamageCompressedSector(const BodyDamage* damage, int32_t sector)
{
    return bodyDamageEnabled(damage) ? damage->sectors[sector] : 0;
}

float bodyDamageVisibleTotal(const BodyDamage* damage)
{
    return bodyDamageEnabled(damage) ? (float)damage->total * 0.000244140625f : 0.0f;
}

static int32_t signedSix(uint32_t bits)
{
    const int32_t value = (int32_t)(bits & 63u);
    return value < 32 ? value : value - 64;
}

static int32_t floorEighth(int32_t value)
{
    return value >= 0 ? value / 8 : -(int32_t)(((uint32_t)(-(int64_t)value) + 7u) / 8u);
}

int32_t bodyDamageDeformVertex(BodyDamageModel* model, int32_t,
    float position[3], uint16_t* color, float*, int32_t damageFlag)
{
    hp2::Require(model->runIndex < model->info->runCount,
        "BodyDamage's RLE cursor exceeds its loaded definition");
    const uint32_t run = model->info->runs[model->runIndex];
    if (model->repeat == (run & 7u)) { model->repeat = 0; ++model->runIndex; }
    else ++model->repeat;
    const auto* damage = model->owner;
    const int32_t sector = (int32_t)((run & 255u) >> 3);
    const float previous = (float)damage->appliedSectors[sector] * 0.0078125f;
    const float current = bodyDamageVisibleSector(damage, sector);
    const float difference = current - previous;
    if (difference != 0.0f && !model->deformationDisabled) {
        if (run & 0xffffc000u) {
            for (int axis = 0; axis < 3; ++axis) {
                const float displacement = (float)signedSix(run >> (14 + axis * 6)) * 0.019999999552965164f;
                position[axis] = position[axis] + displacement * difference;
            }
        }
        const int32_t colorScale = signedSix(run >> 8) * 8;
        if (colorScale) {
            const int32_t oldColor = floorEighth(convertFloatToInt32((float)colorScale * previous));
            const int32_t newColor = floorEighth(convertFloatToInt32((float)colorScale * current));
            const int32_t change = newColor - oldColor;
            *color = (uint16_t)((int32_t)*color - change * 1057);
        }
    }
    if (damageFlag) {
        int32_t level = convertFloatToInt32(current * 256.0f);
        if (level > 255) level = 255;
        if (level < 0) level = 0;
        if (128 < level) *color |= 0x8000u;
        else *color &= 0x7fffu;
    }
    return current != 0.0f;
}

void bodyDamageApply(BodyDamage* damage)
{
    const float current = bodyDamageVisibleTotal(damage);
    const float previous = (float)damage->appliedTotal * 0.000244140625f;
    if (current == previous || (!(0.019999999552965164f < fabsf(current - previous)) && current != 0.0f)) return;
    for (size_t index = 0; index < damage->modelCount; ++index) {
        bodyDamageModelApply(&damage->models[index]);
    }
    uint32_t total = 0;
    for (int sector = 0; sector < BODY_DAMAGE_SECTORS; ++sector) {
        const uint32_t value = bodyDamageCompressedSector(damage, sector);
        damage->appliedSectors[sector] = (uint8_t)value;
        total += value;
    }
    damage->appliedTotal = (uint16_t)total;
}

void bodyDamageRepair(BodyDamage* damage)
{
    for (int sector = 0; sector < BODY_DAMAGE_SECTORS; ++sector) bodyDamageSetSector(damage, sector, 0.0f);
    damage->total = 0;
    bodyDamageApply(damage);
}


namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Body damage definition contains a non-finite bound");
    return value;
}
} // namespace

CarDamageData::CarDamageData(const std::vector<std::uint8_t>& geometry)
{
    std::set<std::uint32_t> keys;
    const auto visit = [&](const auto& self, std::size_t start, std::size_t size, unsigned depth) -> void {
        Require(depth <= 32, "Body damage chunk nesting exceeds 32 levels");
        for (const auto& chunk : ReadChunks(geometry, start, size)) {
            if (chunk.id == 0x34030u) {
                Require(chunk.size >= 84 && (chunk.size - 84) % 4 == 0,
                        "Body damage definition contains a partial record");
                const auto* data = geometry.data() + chunk.payloadOffset;
                const auto* end = std::find(data + 8, data + 40, std::uint8_t(0));
                Require(end != data + 40, "Body damage model name is not terminated");
                Definition definition;
                definition.name.assign(reinterpret_cast<const char*>(data + 8), static_cast<std::size_t>(end - data - 8));
                definition.bounds.key = ReadU32(data + 40);
                Require(keys.insert(definition.bounds.key).second && hashString(definition.name.c_str()) == definition.bounds.key,
                        "Body damage definition repeats a key or has a name/hash mismatch");
                for (int lane = 0; lane < 2; ++lane) {
                    definition.bounds.minimum[lane] = real(data + 44 + lane * 4);
                    definition.bounds.maximum[lane] = real(data + 52 + lane * 4);
                    Require(definition.bounds.minimum[lane] < definition.bounds.maximum[lane],
                            "Body damage definition has a degenerate bound");
                }
                definition.bounds.vertexCount = ReadU32(data + 68);
                const std::size_t count = ReadU32(data + 72);
                Require(count == (chunk.size - 84) / 4, "Body damage run count does not match its payload");
                definition.runs.reserve(count);
                std::size_t covered = 0;
                for (std::size_t index = 0; index < count; ++index) {
                    const auto code = ReadU32(data + 84 + index * 4);
                    definition.runs.push_back(code);
                    covered += (code & 7u) + 1;
                }
                Require(covered == definition.bounds.vertexCount, "Body damage runs do not cover the declared vertices");
                definitions_.push_back(std::move(definition));
            } else if (chunk.IsContainer()) self(self, chunk.payloadOffset, chunk.size, depth + 1);
        }
    };
    visit(visit, 0, geometry.size(), 0);
    // Register only once all allocations/validation have succeeded. The list
    // borrows these same stable records and these same retained run buffers.
    for (auto& definition : definitions_) {
        definition.bounds.runs = definition.runs.data();
        definition.bounds.runCount = definition.runs.size();
        bodyDamageLoadInfo(0x34030u, &definition.bounds);
    }
}

CarDamageData::~CarDamageData()
{
    for (auto& definition : definitions_) bodyDamageUnloadInfo(0x34030u, &definition.bounds);
}

const BodyDamageInfo* CarDamageData::Find(const std::string& name) const noexcept
{
    for (const auto& definition : definitions_) if (definition.name == name) return &definition.bounds;
    return nullptr;
}

} // namespace hp2

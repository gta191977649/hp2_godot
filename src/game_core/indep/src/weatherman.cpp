#include "WeatherMan.hpp"
#include "Track.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

struct WeatherRegion {
    WeatherRegion* next = nullptr;
    WeatherRegion* previous = nullptr;
    uint32_t groupKey = 0;
    const char* name = nullptr;
    uint32_t kind = 0;
    uint32_t fogColor = 0;
    float fogDistance = 0.0f;
    float position[3]{};
    float radius = 0.0f;
    float strength = 0.0f;
    float innerRadius = 0.0f;
};

// Final GP-4B50..-4B38 are initially all zero. These are the sole mutable
// controls, not flags copied into a camera or the sound subsystem.
WeatherManPolicy weatherManPolicy{};

namespace {

struct WeatherRegistry {
    std::array<WeatherRegion, 4> active;
    WeatherRegion inactive;
    std::unique_ptr<WeatherRegion[]> regions;
    size_t count = 0;
    const std::vector<uint8_t>* resource = nullptr;
    const GameTime* clock = nullptr;

    WeatherRegistry()
    {
        for (auto& head : active) head.next = head.previous = &head;
        inactive.next = inactive.previous = &inactive;
    }

    bool Owns(const WeatherRegion* region) const
    {
        for (size_t index = 0; index < count; ++index)
            if (&regions[index] == region) return true;
        return false;
    }
};

WeatherRegistry weather;

void appendRegion(WeatherRegion& head, WeatherRegion& region)
{
    region.next = &head;
    region.previous = head.previous;
    head.previous->next = &region;
    head.previous = &region;
}

void weatherAddRegion(WeatherRegion& region)
{
    if (region.kind < weather.active.size()) appendRegion(weather.active[region.kind], region);
}

void weatherRemoveRegion(WeatherRegion& region)
{
    region.previous->next = region.next;
    region.next->previous = region.previous;
    // The original resource is about to be released. Do not retain native
    // dangling links after unlinking the same node from its actual list.
    region.next = region.previous = nullptr;
}

float authoredFloat(const std::vector<uint8_t>& bytes, size_t offset)
{
    const uint32_t raw = hp2::ReadU32(bytes.data()+offset);
    float value;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

uint32_t packedColor(float red, float green, float blue, bool mask)
{
    uint32_t r = static_cast<uint32_t>(convertFloatToInt32(red));
    uint32_t g = static_cast<uint32_t>(convertFloatToInt32(green));
    uint32_t b = static_cast<uint32_t>(convertFloatToInt32(blue));
    if (mask) { r &= 255; g &= 255; b &= 255; }
    return (r << 16)|(g << 8)|b;
}

} // namespace

WeatherManResult weatherManBindClock(const GameTime* clock)
{
    if (!clock || (weather.clock && weather.clock != clock)) return WEATHER_MAN_INVALID_BINDING;
    weather.clock = clock;
    return WEATHER_MAN_OK;
}

extern "C" WeatherManResult weatherManUnbindClock(const GameTime* clock)
{
    // Native terminal detach. Track resources must release all their actual
    // nodes before the application's clock can cease to exist.
    if (!clock || weather.clock != clock || weather.resource || weather.regions || weather.count)
        return WEATHER_MAN_INVALID_BINDING;
    for (const auto& head : weather.active)
        if (head.next != &head || head.previous != &head) return WEATHER_MAN_INVALID_BINDING;
    if (weather.inactive.next != &weather.inactive || weather.inactive.previous != &weather.inactive)
        return WEATHER_MAN_INVALID_BINDING;
    weather.clock = nullptr;
    return WEATHER_MAN_OK;
}

WeatherManResult weatherSetRegionState(WeatherRegion* region, int32_t enabled)
{
    if (!weather.Owns(region) || !region->next || !region->previous)
        return WEATHER_MAN_INVALID_BINDING;
    weatherRemoveRegion(*region);
    if (enabled) appendRegion(weather.active[region->kind], *region);
    else appendRegion(weather.inactive, *region);
    return WEATHER_MAN_OK;
}

void weatherSetGroupState(uint32_t groupKey, int32_t enabled)
{
    if (enabled) {
        auto* region = weather.inactive.next;
        while (region != &weather.inactive) {
            auto* next = region->next;
            if (!groupKey || region->groupKey == groupKey) (void)weatherSetRegionState(region, enabled);
            region = next;
        }
    } else {
        for (auto& head : weather.active) {
            auto* region = head.next;
            while (region != &head) {
                auto* next = region->next;
                if (!groupKey || region->groupKey == groupKey) (void)weatherSetRegionState(region, enabled);
                region = next;
            }
        }
    }
}

WeatherManResult weatherManQuery(WeatherQuery* query, const float position[4])
{
    if (!query || !position) return WEATHER_MAN_INVALID_BINDING;
    // PAL 1A84F0..1A8DB8 evaluates region distances, overlap weights and
    // fog channels at scalar instruction boundaries. IEEE nearest rounding
    // changes real TRACKB overlap colors/strength before the final CLUT.
    // Use the same finite scalar owner as the original camera/physics path;
    // region order, three-soft-region limit and all authored values stay exact.
    if (weatherManPolicy.animatedOverride) {
        if (!weather.clock) return WEATHER_MAN_INVALID_BINDING;
        const float frames = static_cast<float>(weather.clock->worldTimeFrames);
        const float first = rotationSineRadians(bScalarMultiply(0.019999999552965164f,frames));
        const float second = rotationSineRadians(bScalarMultiply(0.0010000000474974513f,frames));
        const float third = rotationSineRadians(bScalarMultiply(0.009999999776482582f,frames));
        float distance = bScalarMultiply(first,200.0f);
        const float next = bScalarMultiply(second,150.0f);
        distance = bScalarAdd(distance,-1000.0f);
        distance = bScalarAdd(distance,next);
        distance = bScalarAdd(distance,bScalarMultiply(third,100.0f));
        query->fogStrength = 0.9577530026435852f;
        query->fogColor = 0x887062;
        query->fogDistance = distance;
        return WEATHER_MAN_OK;
    }
    if (weatherManPolicy.fixedOverride) {
        query->precipitation = query->parameter08 = query->parameter0C = 0.0f;
        query->fogStrength = weatherManPolicy.fixedFogStrength;
        query->fogDistance = weatherManPolicy.fixedFogDistance;
        query->fogColor = (weatherManPolicy.fixedRed << 16)|
            (weatherManPolicy.fixedGreen << 8)|weatherManPolicy.fixedBlue;
        return WEATHER_MAN_OK;
    }
    *query = {};
    for (size_t kind = 0; kind < weather.active.size(); ++kind) {
        std::array<const WeatherRegion*, 3> soft{};
        std::array<float, 3> weights{};
        size_t count = 0;
        const WeatherRegion* nearest = nullptr;
        float closest = 9999999.0f;
        const auto& head = weather.active[kind];
        for (auto* region = head.next; region != &head && count < soft.size(); region = region->next) {
            const float outer = bScalarMultiply(region->radius,region->radius);
            const float x = std::fabs(bScalarAdd(position[0],-region->position[0]));
            const float y = std::fabs(bScalarAdd(position[1],-region->position[1]));
            const float z = std::fabs(bScalarAdd(position[2],-region->position[2]));
            const float horizontal = bScalarAdd(bScalarMultiply(x,x),bScalarMultiply(y,y));
            const float squared = bScalarAdd(horizontal,bScalarMultiply(z,z));
            if (!(squared < outer)) continue;
            const float inner = bScalarMultiply(region->innerRadius,region->innerRadius);
            if (outer == inner) {
                if (squared < closest) { closest = squared; nearest = region; }
            } else {
                const float numerator = bScalarAdd(squared,-inner);
                const float denominator = bScalarAdd(outer,-inner);
                const float weight = bScalarAdd(1.0f,-bScalarDivide(numerator,denominator));
                if (0.0f < weight) { soft[count] = region; weights[count++] = weight; }
            }
        }
        float sum = bScalarAdd(weights[0],weights[1]);
        sum = bScalarAdd(sum,weights[2]);
        float scale = 1.0f, base = bScalarAdd(1.0f,-sum);
        if (1.0f < sum) { scale = bScalarDivide(1.0f,sum); base = 0.0f; }
        query->baseWeight = base;
        query->weightScale = scale;
        float amount = nearest ? bScalarMultiply(nearest->strength,base) : 0.0f;
        // The scalar types use scale here, not each individual soft weight.
        // Fog below has its own weighted path; preserve that real difference.
        for (const auto* region : soft) if (region) amount = bScalarAdd(amount,bScalarMultiply(region->strength,scale));
        if (kind == 0) query->precipitation = amount;
        else if (kind == 2) query->parameter08 = amount;
        else if (kind == 3) query->parameter0C = amount;
        else {
            std::array<const WeatherRegion*, 4> contributors = {nearest, soft[0], soft[1], soft[2]};
            float strength = 0.0f, red = 0.0f, green = 0.0f, blue = 0.0f, distance = 0.0f;
            for (size_t index = 0; index < contributors.size(); ++index) {
                const auto* region = contributors[index];
                if (!region) continue;
                const float weight = index ? bScalarMultiply(weights[index-1],scale) : base;
                const float b = bScalarMultiply(static_cast<float>(region->fogColor&255),weight);
                const float g = bScalarMultiply(static_cast<float>((region->fogColor >> 8)&255),weight);
                const float r = bScalarMultiply(static_cast<float>((region->fogColor >> 16)&255),weight);
                const float d = bScalarMultiply(region->fogDistance,weight);
                strength = bScalarAdd(strength,bScalarMultiply(region->strength,weight));
                blue = bScalarAdd(blue,b); green = bScalarAdd(green,g);
                red = bScalarAdd(red,r); distance = bScalarAdd(distance,d);
                query->contributions[index] = {region->name, packedColor(r, g, b, false), d, weight};
            }
            query->fogStrength = strength;
            query->fogColor = packedColor(red, green, blue, true);
            query->fogDistance = distance;
        }
    }
    return WEATHER_MAN_OK;
}

WeatherManResult weatherManLoadResources(const hp2::TrackScenarioData& track)
{
    if (weather.resource) return WEATHER_MAN_INVALID_BINDING;
    const auto& bytes = track.Authored();
    try {
        std::vector<size_t> records;
        const auto visit = [&](const auto& self, size_t offset, size_t extent) -> void {
            for (const auto& chunk : hp2::ReadChunks(bytes, offset, extent)) {
                if (chunk.id == 0x34250) {
                    const size_t start = (chunk.payloadOffset+15u)&~size_t(15u);
                    const size_t end = chunk.payloadOffset+chunk.size;
                    if (start > end || end-start < 16) throw std::invalid_argument("Weather header is truncated");
                    if (hp2::ReadU32(bytes.data()+start+8) != 2)
                        throw std::invalid_argument("Weather resource is not the final version2 domain");
                    const uint32_t count = hp2::ReadU32(bytes.data()+start+12);
                    if (count > (end-start-16)/128) throw std::invalid_argument("Weather regions exceed their retained chunk");
                    for (uint32_t index = 0; index < count; ++index) records.push_back(start+16+index*128);
                } else if (chunk.IsContainer()) self(self, chunk.payloadOffset, chunk.size);
            }
        };
        visit(visit, 0, bytes.size());
        auto regions = std::make_unique<WeatherRegion[]>(records.size());
        for (size_t index = 0; index < records.size(); ++index) {
            const size_t start = records[index];
            auto& region = regions[index];
            region.groupKey = hp2::ReadU32(bytes.data()+start+0x0C);
            region.name = reinterpret_cast<const char*>(bytes.data()+start+0x10);
            region.kind = hp2::ReadU32(bytes.data()+start+0x50);
            if (region.kind >= 4) throw std::invalid_argument("Weather region has no actual kind list");
            region.fogColor = hp2::ReadU32(bytes.data()+start+0x54);
            region.fogDistance = authoredFloat(bytes, start+0x58);
            for (size_t lane = 0; lane < 3; ++lane) region.position[lane] = authoredFloat(bytes, start+0x60+lane*4);
            region.radius = authoredFloat(bytes, start+0x6C);
            region.strength = authoredFloat(bytes, start+0x70);
            region.innerRadius = authoredFloat(bytes, start+0x74);
        }
        weather.regions = std::move(regions);
        weather.count = records.size();
        weather.resource = &bytes;
        for (size_t index = 0; index < weather.count; ++index) weatherAddRegion(weather.regions[index]);
        return WEATHER_MAN_OK;
    } catch (const std::bad_alloc&) { return WEATHER_MAN_ALLOCATION_FAILURE; }
    catch (const std::exception&) { return WEATHER_MAN_INVALID_RESOURCE; }
}

WeatherManResult weatherManUnloadResources(const hp2::TrackScenarioData& track)
{
    if (weather.resource != &track.Authored()) return WEATHER_MAN_INVALID_BINDING;
    for (size_t index = 0; index < weather.count; ++index) weatherRemoveRegion(weather.regions[index]);
    weather.regions.reset();
    weather.count = 0;
    weather.resource = nullptr;
    return WEATHER_MAN_OK;
}

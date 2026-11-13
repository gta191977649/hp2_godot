#ifndef NFSHP2_WEATHER_MAN_HPP
#define NFSHP2_WEATHER_MAN_HPP

#include <stdint.h>

struct GameTime;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WeatherRegion WeatherRegion;

/* Typed query output, not a packed console structure. Region names borrow
 * the retained34250 record's64-byte label and may require bounded display. */
typedef struct WeatherContribution {
    const char* regionName;
    uint32_t fogColor;
    float fogDistance;
    float weight;
} WeatherContribution;

typedef struct WeatherQuery {
    float precipitation;
    float fogStrength;
    float parameter08;
    float parameter0C;
    uint32_t fogColor;
    float fogDistance;
    float baseWeight;
    float weightScale;
    WeatherContribution contributions[4];
} WeatherQuery;

typedef struct WeatherManPolicy {
    int32_t fixedOverride;
    float fixedFogStrength;
    float fixedFogDistance;
    uint32_t fixedBlue;
    uint32_t fixedGreen;
    uint32_t fixedRed;
    int32_t animatedOverride;
} WeatherManPolicy;

extern WeatherManPolicy weatherManPolicy;

typedef enum WeatherManResult {
    WEATHER_MAN_OK,
    WEATHER_MAN_INVALID_BINDING,
    WEATHER_MAN_INVALID_RESOURCE,
    WEATHER_MAN_ALLOCATION_FAILURE
} WeatherManResult;

WeatherManResult weatherManQuery(WeatherQuery* query, const float position[4]);
WeatherManResult weatherSetRegionState(WeatherRegion* region, int32_t enabled);
void weatherSetGroupState(uint32_t groupKey, int32_t enabled);
WeatherManResult weatherManBindClock(const struct GameTime* clock);

#ifdef __cplusplus
}

namespace hp2 { class TrackScenarioData; }

/* One current track owns the retained bytes; the registry owns each decoded
 * node exactly once and merely links that node into active/inactive lists. */
WeatherManResult weatherManLoadResources(const hp2::TrackScenarioData& track);
WeatherManResult weatherManUnloadResources(const hp2::TrackScenarioData& track);
#endif
#endif

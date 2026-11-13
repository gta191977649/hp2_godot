#ifndef NFSHP2_INDEP_INC_HELISHEET_HPP
#define NFSHP2_INDEP_INC_HELISHEET_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Authored airborne-road corridor polygons are independent of the authoritative
 * wheel/contact terrain. Height queries retain the original triangulation. */
typedef struct AerialNavigationPolygon {
    float vertices[4][4];
    float minimumX, maximumX;
    int8_t vertexCount;
    uint8_t flags;
} AerialNavigationPolygon;

typedef struct AerialNavigation {
    AerialNavigationPolygon* polygons;
    int16_t count;
    int16_t recent[3];
    float minimumX, maximumX, maximumSpan;
    int16_t bins[256];
} AerialNavigation;

void aerialNavigationBind(AerialNavigation* navigation, AerialNavigationPolygon* polygons, int16_t count);
void aerialNavigationPolygonInitialize(AerialNavigationPolygon* polygon, const float vertices[4][4],
                                       int32_t count, uint8_t flags);
int32_t aerialNavigationContains(const AerialNavigationPolygon* polygon, const float point[2]);
float aerialNavigationHeight(const AerialNavigationPolygon* polygon, const float point[2]);
void aerialNavigationBuildLookup(AerialNavigation* navigation);
int32_t aerialNavigationQuery(AerialNavigation* navigation, const float point[2], float* height, uint32_t* flags);

#ifdef __cplusplus
}
#include <cstdint>
#include <vector>

namespace hp2 {
class AerialNavigationData final {
public:
    explicit AerialNavigationData(const std::vector<std::uint8_t>& track);
    AerialNavigationData(const AerialNavigationData&)=delete;
    AerialNavigationData& operator=(const AerialNavigationData&)=delete;
    AerialNavigation& Native() noexcept { return navigation_; }
private:
    std::vector<AerialNavigationPolygon> polygons_;
    AerialNavigation navigation_{};
};
}

#endif
#endif

#ifndef NFSHP2_TERRAIN_TYPE_HPP
#define NFSHP2_TERRAIN_TYPE_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TerrainMaterialEffect {
    const char* name;
    float start;
    float full;
} TerrainMaterialEffect;

/* A single typed projection of PAL TerrainTypeInfoTable,33x0x90.
 * The six existing physics fields retain their order. Names, skid metadata
 * and effect thresholds below belong to those same material records. */
typedef struct TerrainMaterial {
    float tireEffectFriction;
    float displacementFrequency;
    float displacementAmplitude;
    float staticGrip;
    float slidingGrip;
    float rollingResistance;
    const char* name;
    uint32_t key;
    int32_t type;
    uint32_t classification;
    const char* skidSampleName;
    float parameter2C;
    float parameter30;
    float parameter34;
    uint32_t color;
    TerrainMaterialEffect effects[7];
} TerrainMaterial;

const TerrainMaterial* terrainDefaultMaterials(size_t* count);
int32_t terrainTypeMatchName(const char* name);
int32_t terrainTypeMatchKey(uint32_t key);
void terrainTypeInitialize(void);

#ifdef __cplusplus
}
#endif
#endif

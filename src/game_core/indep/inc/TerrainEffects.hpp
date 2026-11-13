#ifndef NFSHP2_TERRAIN_EFFECTS_HPP
#define NFSHP2_TERRAIN_EFFECTS_HPP
#include "indep/inc/AcidFX.hpp"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct TerrainEffectEmitters TerrainEffectEmitters;

/* One world-owned catalogue of copied emitter instances. Every channel
 * borrows the same definitions and live CPU particle system. */
WorldServicesResult terrainEffectEmittersInitialize(TerrainEffectEmitters** owner,
    WorldParticleSystem* particles,const ChopperEffectGroup* groups,size_t groupCount);
void terrainEffectEmittersClose(TerrainEffectEmitters** owner);
WorldServicesResult terrainEffectEmittersTrigger(TerrainEffectEmitters* emitters,
    int32_t effect,int32_t material,float strength,const float position[4],const float velocity[4]);
size_t terrainEffectEmittersCount(const TerrainEffectEmitters* emitters);
const WorldParticleSystem* terrainEffectEmittersParticles(const TerrainEffectEmitters* emitters);
ChopperEffectInstance* terrainEffectEmittersGroup(TerrainEffectEmitters* emitters,
    int32_t effect,int32_t material,size_t* count);

#ifdef __cplusplus
}
#endif
#endif

#ifndef NFSHP2_COP_CHASE_HPP
#define NFSHP2_COP_CHASE_HPP

#include "indep/inc/Pursuit.hpp"

#ifdef __cplusplus
extern "C" {
#endif
// Final AICop/PlayerCop IsAIControlled vtable slot14: PAL230D60/230D98.
// The native driver kind selects this dispatch.
static inline int32_t pursuitCollisionEntryEligible(const PursuitCollisionEntry* entry)
{
    return entry->kind == PURSUIT_COLLISION_OWNED_CAR;
}
#ifdef __cplusplus
}
#endif
#endif

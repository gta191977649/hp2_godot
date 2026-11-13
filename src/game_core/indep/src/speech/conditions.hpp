#ifndef NFSHP2_SPEECH_CONDITIONS_HPP
#define NFSHP2_SPEECH_CONDITIONS_HPP

#include "actions.hpp"

inline uint32_t worldSpeechConditionAlways(const WorldSpeechContext&)
{
    return 1;
}
inline uint32_t worldSpeechConditionSupervisorActive(const WorldSpeechContext& context)
{
    return static_cast<uint32_t>(context.variant);
}
inline uint32_t worldSpeechConditionSupervisorNotActive(const WorldSpeechContext& context)
{
    /* Preserve XOR1 rather than substituting a logical negation. */
    return static_cast<uint32_t>(context.variant) ^ 1u;
}
inline uint32_t worldSpeechConditionLocationSet(const WorldSpeechContext& context)
{
    return context.location != 0;
}
inline uint32_t worldSpeechConditionTargetIdSet(const WorldSpeechIdentityMap& ids,
    const WorldSpeechContext& context)
{
    return WorldSpeechIdView{ids}.TargetToId(context) != 0;
}
inline uint32_t worldSpeechConditionRandom(const WorldSpeechContext&)
{
    return vectorDefaultRandomBounded(100) >= 50;
}

#endif

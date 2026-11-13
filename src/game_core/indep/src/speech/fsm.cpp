#include "indep/src/speech/fsm.hpp"

void worldSpeechFsmInitialize(WorldSpeechFsm* fsm)
{
    fsm->current = nullptr;
    fsm->initial = nullptr;
}

void worldSpeechFsmAddTransition(WorldSpeechFsm*, WorldSpeechState* from,
    WorldSpeechState* to, int32_t event, const WorldSpeechCondition* condition,
    const WorldSpeechAction* action)
{
    if (!from || !to || !condition || !action) return;
    worldSpeechStateAddTransition(from, to, event, condition, action);
}

WorldSpeechResult worldSpeechFsmHandleEvent(WorldSpeechFsm* fsm, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context)
{
    if (!fsm) return WORLD_SPEECH_INVALID_BINDING;
    if (!fsm->current) return WORLD_SPEECH_OK;
    WorldSpeechState* destination = fsm->current;
    const WorldSpeechResult result = worldSpeechStateHandleEvent(fsm->current,
        event, manager, context, &destination);
    fsm->current = destination;
    return result;
}

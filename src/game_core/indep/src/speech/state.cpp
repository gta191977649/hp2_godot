#include "indep/src/speech/fsm.hpp"

void worldSpeechStateInitialize(WorldSpeechState* state)
{
    state->transitions.clear();
}

void worldSpeechStateAddTransition(WorldSpeechState* state, WorldSpeechState* destination,
    int32_t event, const WorldSpeechCondition* condition, const WorldSpeechAction* action)
{
    WorldSpeechTransition transition{};
    worldSpeechTransitionInitialize(&transition, destination, event, condition, action);
    state->transitions.push_back(transition);
}

WorldSpeechResult worldSpeechStateFindTransition(WorldSpeechState* state, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context,
    const WorldSpeechTransition** result)
{
    if (!state || !result) return WORLD_SPEECH_INVALID_BINDING;
    *result = nullptr;
    for (const auto& transition : state->transitions) {
        int32_t accepted = 0;
        const WorldSpeechResult status = worldSpeechTransitionCanTraverse(&transition,
            event, manager ? manager->identities : nullptr, context, &accepted);
        if (status != WORLD_SPEECH_OK) return status;
        if (accepted) { *result = &transition; break; }
    }
    return WORLD_SPEECH_OK;
}

WorldSpeechResult worldSpeechStateHandleEvent(WorldSpeechState* state, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context, WorldSpeechState** destination)
{
    if (!destination) return WORLD_SPEECH_INVALID_BINDING;
    *destination = state;
    const WorldSpeechTransition* transition = nullptr;
    const WorldSpeechResult found = worldSpeechStateFindTransition(state, event,
        manager, context, &transition);
    if (found != WORLD_SPEECH_OK || !transition) return found;
    const WorldSpeechResult action = worldSpeechTransitionExecuteAction(transition, manager, context);
    /* The original obtains the destination after its action, regardless of
     * the message backend's ignored return. Native failure is still exposed. */
    *destination = transition->destination;
    return action;
}

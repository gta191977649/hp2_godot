#include "indep/src/speech/fsm.hpp"
#include "indep/src/speech/conditions.hpp"

void worldSpeechTransitionInitialize(WorldSpeechTransition* transition,
    WorldSpeechState* destination, int32_t event, const WorldSpeechCondition* condition,
    const WorldSpeechAction* action)
{
    transition->event = event;
    transition->destination = destination;
    transition->condition = condition;
    transition->action = action;
}

WorldSpeechResult worldSpeechTransitionCanTraverse(const WorldSpeechTransition* transition,
    int32_t event, const WorldSpeechIdentityMap* ids, const WorldSpeechContext* context,
    int32_t* result)
{
    if (!transition || !result) return WORLD_SPEECH_INVALID_BINDING;
    *result = 0;
    if (event != transition->event) return WORLD_SPEECH_OK;
    if (!transition->condition || !context) return WORLD_SPEECH_INVALID_BINDING;
    uint32_t value;
    switch (transition->condition->kind) {
    case WorldSpeechConditionKind::Always: value = worldSpeechConditionAlways(*context); break;
    case WorldSpeechConditionKind::SupervisorActive: value = worldSpeechConditionSupervisorActive(*context); break;
    case WorldSpeechConditionKind::SupervisorNotActive: value = worldSpeechConditionSupervisorNotActive(*context); break;
    case WorldSpeechConditionKind::Random: value = worldSpeechConditionRandom(*context); break;
    case WorldSpeechConditionKind::LocationSet: value = worldSpeechConditionLocationSet(*context); break;
    case WorldSpeechConditionKind::TargetIdSet:
        if (!ids) return WORLD_SPEECH_INVALID_BINDING;
        value = worldSpeechConditionTargetIdSet(*ids, *context); break;
    default: return WORLD_SPEECH_INVALID_BINDING;
    }
    *result = value != 0;
    return WORLD_SPEECH_OK;
}

WorldSpeechResult worldSpeechTransitionExecuteAction(const WorldSpeechTransition* transition,
    WorldSpeechManager* manager, const WorldSpeechContext* context)
{
    if (!transition || !transition->action) return WORLD_SPEECH_INVALID_BINDING;
    if (transition->action->style == WorldSpeechActionStyle::Helicopter)
        return worldSpeechHelicopterActionExecute(transition->action, manager, context);
    return worldSpeechActionExecute(transition->action, manager, context);
}

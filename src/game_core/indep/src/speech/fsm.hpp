#ifndef NFSHP2_SPEECH_FSM_HPP
#define NFSHP2_SPEECH_FSM_HPP

#include "indep/src/speech/mgr.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include <array>
#include <vector>

/* Native semantic ownership. Console vtables, fragmented-vector allocation
 * records and packed object offsets are replaced by typed state objects and
 * one ordered vector of transitions per state. The graph is PAL-authored. */
enum class WorldSpeechConditionKind { Always, SupervisorActive, SupervisorNotActive,
    Random, LocationSet, TargetIdSet };
enum class WorldSpeechActionStyle { Radio, Helicopter };
using WorldSpeechActionFunction = WorldSpeechResult (*)(WorldAudioMessages&,
    const WorldSpeechIdentityMap&, const WorldSpeechContext&);

struct WorldSpeechCondition {
    WorldSpeechConditionKind kind;
};
struct WorldSpeechAction {
    WorldSpeechActionFunction function;
    WorldSpeechActionStyle style;
};
struct WorldSpeechState;
struct WorldSpeechTransition {
    int32_t event;
    WorldSpeechState* destination;
    const WorldSpeechCondition* condition;
    const WorldSpeechAction* action;
};
struct WorldSpeechState {
    std::vector<WorldSpeechTransition> transitions;
};


void worldSpeechFsmInitialize(WorldSpeechFsm* fsm);
void worldSpeechFsmAddTransition(WorldSpeechFsm* fsm, WorldSpeechState* from,
    WorldSpeechState* to, int32_t event, const WorldSpeechCondition* condition,
    const WorldSpeechAction* action);
WorldSpeechResult worldSpeechFsmHandleEvent(WorldSpeechFsm* fsm, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context);
void worldSpeechStateInitialize(WorldSpeechState* state);
void worldSpeechStateAddTransition(WorldSpeechState* state, WorldSpeechState* destination,
    int32_t event, const WorldSpeechCondition* condition, const WorldSpeechAction* action);
WorldSpeechResult worldSpeechStateFindTransition(WorldSpeechState* state, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context,
    const WorldSpeechTransition** result);
WorldSpeechResult worldSpeechStateHandleEvent(WorldSpeechState* state, int32_t event,
    WorldSpeechManager* manager, const WorldSpeechContext* context, WorldSpeechState** destination);
void worldSpeechTransitionInitialize(WorldSpeechTransition* transition,
    WorldSpeechState* destination, int32_t event, const WorldSpeechCondition* condition,
    const WorldSpeechAction* action);
WorldSpeechResult worldSpeechTransitionCanTraverse(const WorldSpeechTransition* transition,
    int32_t event, const WorldSpeechIdentityMap* ids, const WorldSpeechContext* context,
    int32_t* result);
WorldSpeechResult worldSpeechTransitionExecuteAction(const WorldSpeechTransition* transition,
    WorldSpeechManager* manager, const WorldSpeechContext* context);
WorldSpeechResult worldSpeechActionExecute(const WorldSpeechAction* action,
    WorldSpeechManager* manager, const WorldSpeechContext* context);
WorldSpeechResult worldSpeechHelicopterActionExecute(const WorldSpeechAction* action,
    WorldSpeechManager* manager, const WorldSpeechContext* context);
void worldSpeechFsmBuild(WorldSpeechFsm* fsm);

#include "indep/src/speech/nfsFsm.hpp"

#endif

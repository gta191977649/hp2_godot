#include "indep/src/speech/mgr.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include "indep/src/speech/fsm.hpp"
#include "indep/src/speech/actions.hpp"
#include "indep/inc/SpeechNoise.hpp"
#include "indep/inc/CarSounds.hpp"

#include <cstring>
#include <memory>
#include <new>

/* Source GP-4024 is an independent developer command, not a speaking flag. */
int32_t worldSpeechDebugPhraseRequested = 0;

static WorldSpeechResult speechAudioResult(WorldAudioResult result)
{
    if (result == WORLD_AUDIO_OK) return WORLD_SPEECH_OK;
    if (result == WORLD_AUDIO_INVALID_BINDING || result == WORLD_AUDIO_STALE_REFERENCE)
        return WORLD_SPEECH_INVALID_BINDING;
    return WORLD_SPEECH_AUDIO_FAILURE;
}

static WorldSpeechResult maybeSayDebugPhrase(WorldSpeechManager* manager)
{
    if (!worldSpeechDebugPhraseRequested) return WORLD_SPEECH_OK;
    if (!manager->messages) return WORLD_SPEECH_INVALID_BINDING;
    const WorldSpeechResult result = worldSpeechEnqueueAction(*manager->messages, 168u, {0u});
    worldSpeechDebugPhraseRequested = 0;
    /* The source tail25D2C8 only notifies its PS2 debugger RPC channel with
     * command40/NULL/0. Retire that transport; retain the real request/reset. */
    return result;
}

void worldSpeechQueueInitialize(WorldSpeechQueue* queue)
{
    /* Explicit native storage initialization. PAL's context constructor is
     * empty; this helper is not claimed as its byte-identical constructor.
     * Raise producers subsequently retain and update their typed fields. */
    std::memset(queue, 0, sizeof(*queue));
}

WorldSpeechContext* worldSpeechNextContext(WorldSpeechManager* manager)
{
    if (!manager || manager->queue.contextWriteIndex >= WORLD_SPEECH_CONTEXT_LIMIT) {
        if (manager) manager->result = WORLD_SPEECH_INVALID_BINDING;
        return nullptr;
    }
    return &manager->queue.contexts[manager->queue.contextWriteIndex];
}

WorldSpeechResult worldSpeechPushEvent(WorldSpeechManager* manager, uint32_t event)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    if (manager->session) {
        const int32_t state = manager->session->state;
        if (state == 4 || state == 7 || state == 5 || state == 9)
            return manager->result = WORLD_SPEECH_SUPPRESSED;
    }
    WorldSpeechQueue& queue = manager->queue;
    if (queue.count == WORLD_SPEECH_EVENT_LIMIT)
        return manager->result = WORLD_SPEECH_DROPPED;
    if (queue.count > WORLD_SPEECH_EVENT_LIMIT || queue.readIndex >= WORLD_SPEECH_EVENT_LIMIT ||
        queue.writeIndex >= WORLD_SPEECH_EVENT_LIMIT || queue.contextWriteIndex >= WORLD_SPEECH_CONTEXT_LIMIT)
        return manager->result = WORLD_SPEECH_INVALID_BINDING;
    queue.events[queue.writeIndex] = {event, queue.contextWriteIndex};
    queue.writeIndex = (queue.writeIndex + 1u) % WORLD_SPEECH_EVENT_LIMIT;
    ++queue.count;
    queue.contextWriteIndex = (queue.contextWriteIndex + 1u) % WORLD_SPEECH_CONTEXT_LIMIT;
    return manager->result = WORLD_SPEECH_OK;
}

int32_t worldSpeechTakeEvent(WorldSpeechManager* manager, WorldSpeechEvent* event)
{
    if (!manager || !event) return 0;
    WorldSpeechQueue& queue = manager->queue;
    if (!queue.count) return 0;
    if (queue.count > WORLD_SPEECH_EVENT_LIMIT || queue.readIndex >= WORLD_SPEECH_EVENT_LIMIT ||
        queue.events[queue.readIndex].contextIndex >= WORLD_SPEECH_CONTEXT_LIMIT) {
        manager->result = WORLD_SPEECH_INVALID_BINDING;
        return 0;
    }
    *event = queue.events[queue.readIndex];
    queue.readIndex = (queue.readIndex + 1u) % WORLD_SPEECH_EVENT_LIMIT;
    --queue.count;
    manager->currentContext = event->contextIndex;
    return 1;
}

WorldSpeechResult worldSpeechInitialize(WorldSpeechManager* manager)
{
    if (!manager || manager->fsm) return WORLD_SPEECH_INVALID_BINDING;
    try {
        std::unique_ptr<WorldSpeechFsm> fsm(new WorldSpeechFsm);
        worldSpeechFsmBuild(fsm.get());
        worldSpeechQueueInitialize(&manager->queue);
        manager->speaking = 0;
        fsm->SetReadyState();
        /* PAL leaves unused context payloads uninitialized. Native semantic
         * storage is explicitly zeroed; no byte-parity claim covers them. */
        manager->currentContext = 0;
        manager->fsm = fsm.release();
        return manager->result = WORLD_SPEECH_OK;
    } catch (const std::bad_alloc&) {
        return manager->result = WORLD_SPEECH_ALLOCATION_FAILURE;
    }
}

void worldSpeechDispose(WorldSpeechManager* manager)
{
    if (!manager) return;
    delete manager->fsm;
    manager->fsm = nullptr;
}

WorldSpeechManager* worldSpeechGetInstance(void)
{
    struct ProcessOwner {
        WorldSpeechManager manager{};
        WorldSpeechIdentityMap identities{};
        ProcessOwner()
        {
            if (worldSpeechInitialize(&manager)!=WORLD_SPEECH_OK)
                throw std::bad_alloc();
            manager.identities=&identities;
            manager.noise=SpeechNoise::getInstance();
        }
        ~ProcessOwner() { worldSpeechDispose(&manager); }
    };
    // PAL334418 is one lazy static manager and its real FSM. C++ static
    // lifetime replaces the source guard/CRT atexit registration; failure
    // never publishes an uninitialized owner and a later call may retry.
    try {
        static ProcessOwner owner;
        return &owner.manager;
    } catch (const std::bad_alloc&) { return nullptr; }
}

// PAL20A288..20A290 is speech::mgr_t::AllocateVoices, a tail call to
// speechContext_t::ShuffleCopIds20F888. CopManager calls it before FlushQueue
// on each construction. These are authored speaker/address masks, not audio
// device channels; all actions borrow the manager's existing identity owner.
WorldSpeechResult worldSpeechAllocateVoices(WorldSpeechManager* manager)
{
    if(!manager || !manager->identities)return WORLD_SPEECH_INVALID_BINDING;
    worldSpeechShuffleCopIds(manager->identities);
    return manager->result=WORLD_SPEECH_OK;
}

WorldSpeechResult worldSpeechFade(WorldSpeechManager* manager,float duration)
{
    if (!manager)return WORLD_SPEECH_INVALID_BINDING;
    if (!manager->streamedAudio)return manager->result=WORLD_SPEECH_OK;
    auto* policy=carSoundCurrentPolicy();
    if (!policy)return manager->result=WORLD_SPEECH_INVALID_BINDING;
    auto* stream=manager->streamedAudio;
    const bool stops=policy->audioSystemEnabled && (stream->state==2 || stream->state==32);
    worldStreamAudioFade(stream,&policy->audioSystemEnabled,duration);
    if (stops && stream->state!=1)
        return manager->result=stream->result==WORLD_STREAM_AUDIO_INVALID_BINDING ?
            WORLD_SPEECH_INVALID_BINDING:WORLD_SPEECH_AUDIO_FAILURE;
    return manager->result=WORLD_SPEECH_OK;
}

WorldSpeechResult worldSpeechUpdate(WorldSpeechManager* manager)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    if (manager->session) {
        const int32_t state = manager->session->state;
        if (state == 4 || state == 7 || state == 5 || state == 9)
            return manager->result = WORLD_SPEECH_SUPPRESSED;
    }
    if (!manager->fsm || !manager->noise)
        return manager->result = WORLD_SPEECH_INVALID_BINDING;
    const WorldSpeechResult debug = maybeSayDebugPhrase(manager);
    if (debug != WORLD_SPEECH_OK) return manager->result = debug;
    const bool pcmBusy = manager->streamedAudio && manager->streamedAudio->state != 1 &&
        manager->streamedAudio->state != 8;
    if (pcmBusy) {
        if (!manager->noise->IsRadioNoise()) manager->noise->StartRadioNoise();
        manager->speaking = 1;
        return manager->result = WORLD_SPEECH_OK;
    }
    if (manager->noise->IsRadioNoise()) {
        manager->noise->StopRadioNoise();
        const WorldSpeechResult stop = speechAudioResult(manager->noise->StopHelicopterNoise());
        if (stop != WORLD_SPEECH_OK) return manager->result = stop;
    }
    if (!manager->fsm->isReadyState()) {
        if (manager->currentContext >= WORLD_SPEECH_CONTEXT_LIMIT)
            return manager->result = WORLD_SPEECH_INVALID_BINDING;
        const WorldSpeechResult result = worldSpeechFsmHandleEvent(manager->fsm, 1,
            manager, &manager->queue.contexts[manager->currentContext]);
        manager->speaking = 1;
        return manager->result = result;
    }
    if (!manager->queue.count) {
        manager->speaking = 0;
        return manager->result = WORLD_SPEECH_OK;
    }
    WorldSpeechEvent event{};
    if (!worldSpeechTakeEvent(manager, &event))
        return manager->result = WORLD_SPEECH_INVALID_BINDING;
    int32_t signedEvent;
    std::memcpy(&signedEvent, &event.type, sizeof(signedEvent));
    const WorldSpeechResult result = worldSpeechFsmHandleEvent(manager->fsm,
        signedEvent, manager, &manager->queue.contexts[event.contextIndex]);
    manager->speaking = 1;
    return manager->result = result;
}

// PAL20A2B0: the installed global SED owner is optional. The native manager
// borrows that same owner; muting does not clear the event/FSM state.
WorldSpeechResult worldSpeechMute(WorldSpeechManager* manager,int32_t muted)
{
    if (!manager)return WORLD_SPEECH_INVALID_BINDING;
    if (!manager->streamedAudio)return manager->result=WORLD_SPEECH_OK;
    const auto result=worldStreamAudioMute(manager->streamedAudio,muted);
    if (result==WORLD_STREAM_AUDIO_OK)return manager->result=WORLD_SPEECH_OK;
    return manager->result=result==WORLD_STREAM_AUDIO_INVALID_BINDING ?
        WORLD_SPEECH_INVALID_BINDING:WORLD_SPEECH_AUDIO_FAILURE;
}

WorldSpeechResult worldSpeechPausePlayback(WorldSpeechManager* manager,int32_t paused)
{
    if (!manager)return WORLD_SPEECH_INVALID_BINDING;
    if (!manager->streamedAudio)return manager->result=WORLD_SPEECH_OK;
    const auto result=worldStreamAudioPauseForMenu(manager->streamedAudio,paused);
    if (result==WORLD_STREAM_AUDIO_OK)return manager->result=WORLD_SPEECH_OK;
    return manager->result=result==WORLD_STREAM_AUDIO_INVALID_BINDING ?
        WORLD_SPEECH_INVALID_BINDING:WORLD_SPEECH_AUDIO_FAILURE;
}

WorldSpeechResult worldSpeechFlush(WorldSpeechManager* manager)
{
    if (!manager || !manager->fsm || !manager->noise)
        return WORLD_SPEECH_INVALID_BINDING;
    manager->fsm->SetReadyState();
    manager->queue.count = 0;
    manager->queue.writeIndex = manager->queue.readIndex;
    manager->noise->StopRadioNoise();
    const WorldSpeechResult stop = speechAudioResult(manager->noise->StopHelicopterNoise());
    if (stop != WORLD_SPEECH_OK) return manager->result = stop;
    if (manager->streamedAudio) {
        const bool initialized = manager->streamedAudio->initialized == 1;
        if (initialized && (!manager->messages || manager->streamedAudio->messages != manager->messages))
            return manager->result = WORLD_SPEECH_INVALID_BINDING;
        const int32_t cancelled = worldStreamAudioCancel(manager->streamedAudio);
        if (initialized && !cancelled)
            return manager->result = WORLD_SPEECH_AUDIO_FAILURE;
    }
    return manager->result = WORLD_SPEECH_OK;
}

#include "bware/inc/bMath.hpp"

// PAL 0x20ad40..0x20ad7c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseTacticDivert(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 29u);
}

// Final PAL20AC40..20AC9C writes fromId(+0), location(+8) and
// variant(+20) before event25. Beta205B10 omitted the location store.
WorldSpeechResult worldSpeechRaiseSuspectFound(WorldSpeechManager* manager,uint32_t fromId,int32_t variant,int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    context->variant = variant;
    context->location = location;
    return worldSpeechPushEvent(manager, 25u);
}

// PAL 0x20af50..0x20af9c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 41u);
}

// PAL 0x20b140..0x20b18c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRoadblockRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 49u);
}

// PAL 0x20b080..0x20b0dc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRoadBlockFailureAvoid(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    context->toId = toId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 47u);
}

// PAL 0x20aa50..0x20aa8c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCrashAmbulance(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 37u);
}

// PAL 0x20b340..0x20b39c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripSet(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->copLocation = copLocation;
    context->fromId = fromId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 55u);
}

// PAL 0x20b828..0x20b864; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseDispatchHelicopterNA(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 70u);
}

// PAL 0x20ab80..0x20abdc; only its own context fields are written.
WorldSpeechResult worldSpeechRaisePursuitUpdateSubjectLocation(WorldSpeechManager* manager, uint32_t fromId, int32_t location, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    context->location = location;
    return worldSpeechPushEvent(manager, 22u);
}

// PAL 0x20ad80..0x20adbc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseTacticCutoff(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 30u);
}

// PAL 0x20a710..0x20a77c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitIdRepeated(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t exactVehicle, int32_t vehicleColor)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleColor = vehicleColor;
    context->fromId = fromId;
    context->variant = variant;
    context->exactVehicle = exactVehicle;
    return worldSpeechPushEvent(manager, 9u);
}

// PAL 0x20a7f0..0x20a850; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitSpeed(WorldSpeechManager* manager, uint32_t fromId, float speed)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    const int32_t speedCategory = worldSpeechConvertSpeed(speed);
    context->vehicleSpeed = speedCategory;
    return worldSpeechPushEvent(manager, 13u);
}

// PAL 0x20acf0..0x20ad3c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseTacticHit(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 27u);
}

// PAL 0x20b1e0..0x20b23c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRoadBlockSet(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t copLocation)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->copLocation = copLocation;
    context->fromId = fromId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 50u);
}

// PAL 0x20b4f0..0x20b584; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuperInitiate(WorldSpeechManager* manager, uint32_t toId, int32_t location, int32_t exactVehicle, int32_t vehicleColor, float speed)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = toId;
    const int32_t speedCategory = worldSpeechConvertSpeed(speed);
    context->vehicleColor = vehicleColor;
    context->location = location;
    context->exactVehicle = exactVehicle;
    context->vehicleSpeed = speedCategory;
    return worldSpeechPushEvent(manager, 60u);
}

// PAL 0x20a480..0x20a4bc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitGeneral(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 5u);
}

// PAL 0x20b078..0x20b080; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorTraffic(WorldSpeechManager* manager)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    return worldSpeechPushEvent(manager, 45u);
}

// PAL 0x20a780..0x20a7f0; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitRacing(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    const int32_t randomValue = vectorDefaultRandomBounded(2);
    context->location = location;
    context->racingVariant = (randomValue == 0 ? 1 : 2);
    return worldSpeechPushEvent(manager, 11u);
}

// PAL 0x20b4b0..0x20b4ec; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuperRequest(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 59u);
}

// PAL 0x20b588..0x20b5d4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseAirborne(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 61u);
}

// PAL 0x20af08..0x20af4c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRadioArrest(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    context->arrestType = 1;
    return worldSpeechPushEvent(manager, 35u);
}

// PAL 0x20a540..0x20a58c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitReckless(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 12u);
}

// PAL 0x20b758..0x20b7a4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuspectCrashTraffic(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 67u);
}

// PAL 0x20aca0..0x20acec; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuspectLost(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 26u);
}

// PAL 0x20b5d8..0x20b638; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCurrentSpeed(WorldSpeechManager* manager, uint32_t fromId, float speed)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    const int32_t speedCategory = worldSpeechConvertSpeed(speed);
    context->vehicleSpeed = speedCategory;
    return worldSpeechPushEvent(manager, 62u);
}

// PAL 0x20a930..0x20a96c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseChatterGeneral(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 18u);
}

// PAL 0x20b3a0..0x20b3fc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripSetLeft(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->copLocation = copLocation;
    context->fromId = fromId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 56u);
}

// PAL 0x20ae00..0x20ae5c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRacerCaught(WorldSpeechManager* manager, uint32_t fromId, uint32_t otherCopId, int32_t arrestType)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->otherCopId = otherCopId;
    context->fromId = fromId;
    context->arrestType = arrestType;
    return worldSpeechPushEvent(manager, 32u);
}

// PAL 0x20b718..0x20b754; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuspectCrashTow(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 66u);
}

// PAL 0x20a4c0..0x20a4fc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitErratic(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 4u);
}

// PAL 0x20a8e0..0x20a92c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBackupRequestLocation(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 16u);
}

// PAL 0x20aae0..0x20ab3c; only its own context fields are written.
WorldSpeechResult worldSpeechRaisePursuitUpdateSpeed(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t vehicleSpeed)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleSpeed = vehicleSpeed;
    context->fromId = fromId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 21u);
}

// PAL 0x20b7a8..0x20b7e4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseDispatchBeOnLookout(WorldSpeechManager* manager, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    return worldSpeechPushEvent(manager, 68u);
}

// PAL 0x20a430..0x20a47c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitCopHitRun(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 3u);
}

// PAL 0x20b190..0x20b1dc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRequestAnotherRoadblock(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 52u);
}

// PAL 0x20b638..0x20b684; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseDirection(WorldSpeechManager* manager, uint32_t fromId, int32_t direction)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->direction = direction;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 63u);
}

// PAL 0x20aa90..0x20aadc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCrashTow(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 38u);
}

// PAL 0x20afe8..0x20b024; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorChatter(WorldSpeechManager* manager, int32_t rotorChatterVariant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->rotorChatterVariant = rotorChatterVariant;
    return worldSpeechPushEvent(manager, 44u);
}

// PAL 0x20a500..0x20a53c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitNoId(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 10u);
}

// PAL 0x20a6b0..0x20a70c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitAgain(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle, int32_t vehicleColor)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleColor = vehicleColor;
    context->fromId = fromId;
    context->exactVehicle = exactVehicle;
    return worldSpeechPushEvent(manager, 14u);
}

// PAL 0x20b240..0x20b28c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRoadBlockSetting(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 51u);
}

// PAL 0x20ae68..0x20aeb4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCopBackOff(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 34u);
}

// PAL 0x20afa8..0x20afe4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorLocation(WorldSpeechManager* manager, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    return worldSpeechPushEvent(manager, 43u);
}

// PAL 0x20b290..0x20b2ec; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripFailure(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 53u);
}

// PAL 0x20aeb8..0x20af04; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCarStopped(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->exactVehicle = exactVehicle;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 36u);
}

// PAL 0x20aa00..0x20aa4c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseFlatTire(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 40u);
}

// PAL 0x20b400..0x20b45c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripSetRight(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->copLocation = copLocation;
    context->fromId = fromId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 57u);
}

// PAL 0x20a5e0..0x20a63c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitIdGeneral(WorldSpeechManager* manager, uint32_t fromId, int32_t approximateVehicle, int32_t vehicleColor)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleColor = vehicleColor;
    context->fromId = fromId;
    context->approximateVehicle = approximateVehicle;
    return worldSpeechPushEvent(manager, 7u);
}

// PAL 0x20a640..0x20a6ac; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitIdSpecific(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t exactVehicle, int32_t vehicleColor)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleColor = vehicleColor;
    context->fromId = fromId;
    context->variant = variant;
    context->exactVehicle = exactVehicle;
    return worldSpeechPushEvent(manager, 8u);
}

// PAL 0x20a590..0x20a5dc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBeginPursuitHitRunTraffic(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 6u);
}

// PAL 0x20b7e8..0x20b824; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseDispatchRoadBlockNA(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 69u);
}

// PAL 0x20b6d8..0x20b714; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuspectCrashAmbulance(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 65u);
}

// PAL 0x20afa0..0x20afa8; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorConfirm(WorldSpeechManager* manager)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    return worldSpeechPushEvent(manager, 42u);
}

// PAL 0x20a9b0..0x20a9fc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCrash(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 39u);
}

// PAL 0x20b2f0..0x20b33c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 54u);
}

// PAL 0x20adc0..0x20adfc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseUpdateReckless(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 31u);
}

// PAL 0x20ab40..0x20ab7c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseCopPursuitAvoidance(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 20u);
}

// PAL 0x20ae60..0x20ae68; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuperBackOff(WorldSpeechManager* manager)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    return worldSpeechPushEvent(manager, 33u);
}

// PAL 0x20a850..0x20a88c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBackupRequest(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 15u);
}

// PAL 0x20b460..0x20b4ac; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSpikeStripTireBlow(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 58u);
}

// PAL 0x20a970..0x20a9ac; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseChatterTraffic(WorldSpeechManager* manager, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 19u);
}

// PAL 0x20b028..0x20b074; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRotorSpikeFailed(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 46u);
}

// PAL 0x20b688..0x20b6d4; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseSuspectCrash(WorldSpeechManager* manager, uint32_t fromId, int32_t variant)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->variant = variant;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 64u);
}

// PAL 0x20a3e0..0x20a42c; only its own context fields are written.
WorldSpeechResult worldSpeechRaisePursuitCopHit(WorldSpeechManager* manager, uint32_t fromId, int32_t location)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->location = location;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 2u);
}

// PAL 0x20a890..0x20a8dc; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseBackupConfirm(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->toId = toId;
    context->fromId = fromId;
    return worldSpeechPushEvent(manager, 17u);
}

// PAL 0x20abe0..0x20ac3c; only its own context fields are written.
WorldSpeechResult worldSpeechRaisePursuitUpdateVehicleIdSpecific(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle, int32_t vehicleColor)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->vehicleColor = vehicleColor;
    context->fromId = fromId;
    context->exactVehicle = exactVehicle;
    return worldSpeechPushEvent(manager, 24u);
}

// PAL 0x20b0e0..0x20b13c; only its own context fields are written.
WorldSpeechResult worldSpeechRaiseRoadBlockFailureThrough(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId)
{
    if (!manager) return WORLD_SPEECH_INVALID_BINDING;
    WorldSpeechContext* context = worldSpeechNextContext(manager);
    if (!context) return manager->result = WORLD_SPEECH_INVALID_BINDING;
    context->fromId = fromId;
    context->toId = toId;
    context->variant = variant;
    return worldSpeechPushEvent(manager, 48u);
}

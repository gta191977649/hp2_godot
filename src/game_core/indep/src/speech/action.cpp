#include "indep/src/speech/fsm.hpp"
#include "indep/inc/SpeechNoise.hpp"

static WorldSpeechResult noiseFailure(WorldAudioResult result)
{
    return result == WORLD_AUDIO_INVALID_BINDING || result == WORLD_AUDIO_STALE_REFERENCE ?
        WORLD_SPEECH_INVALID_BINDING : WORLD_SPEECH_AUDIO_FAILURE;
}

WorldSpeechResult worldSpeechActionExecute(const WorldSpeechAction* action,
    WorldSpeechManager* manager, const WorldSpeechContext* context)
{
    if (!action || !action->function || !manager || !manager->noise ||
        !manager->messages || !manager->identities || !context)
        return WORLD_SPEECH_INVALID_BINDING;
    const WorldAudioResult squelch = manager->noise->BeginSquelchNoise();
    if (squelch != WORLD_AUDIO_OK) return noiseFailure(squelch);
    return action->function(*manager->messages, *manager->identities, *context);
}

WorldSpeechResult worldSpeechHelicopterActionExecute(const WorldSpeechAction* action,
    WorldSpeechManager* manager, const WorldSpeechContext* context)
{
    if (!action || !action->function || !manager || !manager->noise ||
        !manager->messages || !manager->identities || !context)
        return WORLD_SPEECH_INVALID_BINDING;
    const WorldAudioResult squelch = manager->noise->BeginSquelchNoise();
    if (squelch != WORLD_AUDIO_OK) return noiseFailure(squelch);
    /* PAL's default helicopter voice is genuinely NULL. The real noise
     * owner preserves that branch; no guessed helicopter cue is allocated. */
    const WorldAudioResult helicopter = manager->noise->StartHelicopterNoise();
    if (helicopter != WORLD_AUDIO_OK) return noiseFailure(helicopter);
    return action->function(*manager->messages, *manager->identities, *context);
}

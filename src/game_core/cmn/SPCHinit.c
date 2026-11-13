#include "indep/src/speech/SEDWedge.hpp"

#include <stdlib.h>
#include <string.h>

enum { SPEECH_ENGINE_MAGIC=0x01789A34 };

static int32_t signedWord(uint32_t word)
{
    int32_t value;
    memcpy(&value,&word,sizeof(value));
    return value;
}

int32_t spchGetSampleDataRate(int32_t sampleRate,int32_t bits,int32_t encoding)
{
    const int32_t product=signedWord((uint32_t)sampleRate*(uint32_t)bits);
    int32_t rate=product/8;
    if (encoding==1) rate/=10;
    else if (encoding==2) rate=signedWord((uint32_t)rate*2u)/7;
    return rate;
}

void spchInitializeChannels(WorldAudioMessages* messages)
{
    if (!messages) abort();
    for (size_t index=0;index<WORLD_AUDIO_BANK_LIMIT;++index) {
        messages->channels[index].state=0;
        messages->channels[index].id=-1;
    }
}

static void initializeCallbacks(SpeechEngineState* engine)
{
    engine->sentenceOwner=NULL;
    engine->ruleOwner=NULL;
    engine->ruleSetterOwner=NULL;
}

static void initializeExternalVectors(SpeechEngineState* engine)
{
    /* The actual release SED abort/debug functions are console diagnostics.
     * The live external owner supplies its real TimerGetTick role later. */
    engine->externalOwner=NULL;
}

SpeechDatabaseResult spchInitializeRuleCallbacks(SpeechEngineState* engine,
    struct WorldStreamAudio* ruleOwner)
{
    if (!engine || engine->magic!=SPEECH_ENGINE_MAGIC)
        return SPEECH_DATABASE_INVALID_BINDING;
    /* SED's actual TestRule returns0 and SetRule returns without stores.
     * The owner tag retains those exact release roles, not a callback shim. */
    engine->ruleOwner=ruleOwner;
    engine->ruleSetterOwner=ruleOwner;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchEngineInitialize(SpeechEngineState* engine,
    WorldAudioMessages* messages,struct WorldStreamAudio* sentenceOwner,
    uint32_t seed,uint32_t bytesPerSecond)
{
    if (!engine || !messages || !sentenceOwner)
        return SPEECH_DATABASE_INVALID_BINDING;
    initializeCallbacks(engine);
    initializeExternalVectors(engine);
    engine->messages=messages;
    messages->engine=engine;
    engine->addingEvent=0;engine->playingEvent=0;
    engine->consumeCycleBits=1;engine->operationResult=SPEECH_DATABASE_OK;
    engine->sentenceOwner=sentenceOwner;
    engine->cyclePhase=seed;
    engine->bytesPerSecond=bytesPerSecond;
    engine->internalEventRule=1;
    spchRandomInitialize(&engine->random,seed);
    spchInitializeSentenceChoice(&engine->sentenceChoice);
    spchSetPreloadTicks(engine,0);
    for (size_t index=0;index<8;++index) engine->eventCounters[index]=0;
    spchInitializeEventDatabases(&engine->databases);
    spchInitializeChannels(messages);
    spchBankDropStorage(&engine->banks);
    engine->banks.engineMagic=&engine->magic;
    engine->banks.cyclePhase=&engine->cyclePhase;
    spchInitializeEventQueue(messages);
    engine->magic=SPEECH_ENGINE_MAGIC;
    return SPEECH_DATABASE_OK;
}

/* PAL290A08 and290A38 change only the independently installed custom
 * provider. Init/Deinit do not reset those two source cells. */
SpeechDatabaseResult spchInitializeReparameterizeCallback(SpeechEngineState* engine,
    struct WorldStreamAudio* owner)
{
    if (!engine || engine->magic!=SPEECH_ENGINE_MAGIC)
        return SPEECH_DATABASE_INVALID_BINDING;
    engine->reparameterizeOwner=owner;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchInitializeEventRuleCallback(SpeechEngineState* engine,
    struct WorldStreamAudio* owner)
{
    if (!engine || engine->magic!=SPEECH_ENGINE_MAGIC)
        return SPEECH_DATABASE_INVALID_BINDING;
    engine->eventRuleOwner=owner;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchEngineDeinitialize(SpeechEngineState* engine)
{
    if (!engine || engine->magic!=SPEECH_ENGINE_MAGIC)
        return SPEECH_DATABASE_INVALID_BINDING;
    /* PAL clears magic BEFORE callbacks/banks/databases/log teardown. The
     * retained resource owner releases its buffers only after this returns. */
    engine->magic=0;
    initializeCallbacks(engine);
    spchBankDropStorage(&engine->banks);
    spchInitializeEventDatabases(&engine->databases);
    spchEventLogSave(engine);
    spchEventLogDeinitialize(engine);
    return SPEECH_DATABASE_OK;
}

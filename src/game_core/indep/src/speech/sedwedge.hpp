#ifndef NFSHP2_SED_WEDGE_HPP
#define NFSHP2_SED_WEDGE_HPP

#include <stddef.h>
#include <stdint.h>
#include "bsound/include/bSample.h"

/* A checked view of one retained final-PAL resource. These are native spans,
 * not copied console addresses or a second resource owner. */
typedef struct SpeechBytes { const uint8_t* bytes; size_t size; } SpeechBytes;
struct SpeechEngineState;

#ifdef __cplusplus
extern "C" {
#endif

enum { WORLD_AUDIO_MESSAGE_LIMIT = 16, WORLD_AUDIO_BANK_LIMIT = 8, WORLD_AUDIO_MESSAGE_PARAMETERS = 19 };

typedef enum WorldAudioMessagesResult {
    WORLD_AUDIO_MESSAGES_OK,
    WORLD_AUDIO_MESSAGES_INVALID_REQUEST,
    WORLD_AUDIO_MESSAGES_CAPACITY
} WorldAudioMessagesResult;

typedef struct WorldAudioMessage {
    uint32_t category;
    uint32_t parameters[WORLD_AUDIO_MESSAGE_PARAMETERS];
    size_t parameterCount;
    uint64_t sequence;
    uint32_t epoch;
    int32_t bankIndex;
    int32_t active;
    int32_t dispatched;
    uint32_t tick;
    uint16_t order;
    int16_t follow;
    int16_t nextFollow;
    SpeechBytes definition;
} WorldAudioMessage;

typedef struct WorldAudioMessageChannel {
    int16_t id;
    uint32_t state;
    uint16_t databaseId;
} WorldAudioMessageChannel;

typedef struct WorldAudioMessageHandle {
    WorldAudioMessage* message;
    uint64_t sequence;
    uint32_t epoch;
} WorldAudioMessageHandle;

/* The SPCH event/sentence queue is distinct from the audio device-command
 * queue. Pending parameters, channel state and bank references live here.
 * Clear cancels pending requests and advances the native handle epoch. */
typedef struct WorldAudioMessages {
    WorldAudioMessage pending[WORLD_AUDIO_MESSAGE_LIMIT];
    int32_t bankReferences[WORLD_AUDIO_BANK_LIMIT];
    WorldAudioMessageChannel channels[WORLD_AUDIO_BANK_LIMIT];
    uint64_t nextSequence;
    uint32_t epoch;
    WorldAudioMessagesResult result;
    /* Native arrangement of the two eight-word arrays reset at
     * PAL291468/46C, plus291484/508/510. No console layout is implied. */
    int32_t eventSelection[2][WORLD_AUDIO_BANK_LIMIT];
    uint32_t selectionState;
    uint32_t selectionTick;
    uint16_t selectionFlags;
    struct SpeechEngineState* engine; /* borrowed SPCH owner; never a second queue */
    /* Native admission borrows the same live policies as WorldStreamAudio.
     * An intentionally disabled sound/speech system has no initialized SPCH
     * engine. That state suppresses new requests; it is not a device ACK. */
    const int32_t* audioEnabled;
    const int32_t* speechEnabled;
} WorldAudioMessages;

struct GameSession;
WorldAudioMessagesResult worldAudioPlayPursuitFinish(WorldAudioMessages* messages,
    const struct GameSession* session, int32_t classOneCount, int32_t variant,
    int32_t finishKind, int32_t completedCount);

void worldAudioMessagesInitialize(WorldAudioMessages* messages);
WorldAudioMessagesResult worldAudioMessagesEnqueue(WorldAudioMessages* messages, uint32_t category,
                                                    const uint32_t* parameters, size_t parameterCount);
void worldAudioMessagesClear(WorldAudioMessages* messages);
WorldAudioMessageHandle worldAudioMessagesNext(WorldAudioMessages* messages);
WorldAudioMessagesResult worldAudioMessageBindBank(WorldAudioMessages* messages, WorldAudioMessageHandle handle, int32_t bank);
void worldAudioMessageComplete(WorldAudioMessages* messages, WorldAudioMessageHandle handle);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct WorldStreamAudioResources;
struct SpeechEngineState;

typedef enum WorldStreamAudioResult {
    WORLD_STREAM_AUDIO_OK,
    WORLD_STREAM_AUDIO_INVALID_BINDING,
    WORLD_STREAM_AUDIO_CAPACITY,
    WORLD_STREAM_AUDIO_RESOURCE_FAILURE,
    WORLD_STREAM_AUDIO_BUSY
} WorldStreamAudioResult;

/* One bSound voice and its one ordered device queue. The old opaque
 * hardware/stop-ring representation was removed: it must not coexist with
 * the positional/JBC/movie instance pool. The private owner retains actual
 * BAN/EVT/DAT resources and ten bSegments across their real source uses. */
typedef struct WorldStreamAudio {
    int32_t initialized;
    int32_t state;
    WorldAudioVoiceRef voice;
    WorldAudioMessages* messages;
    WorldAudioRequestQueue* audio;
    WorldStreamAudioResult result;
    struct WorldStreamAudioResources* resources;
    const int32_t* audioEnabled;
    const int32_t* speechEnabled;
    int32_t muted;
    // PC sound device cursor state. PAL worldStreamAudioMute only sets gain;
    // the native Pause Menu must also suspend the XAudio2 playback cursor.
    // Not serialized into the PAL World/Replay snapshot.
    int32_t pausedForMenu;
    float timerMilliseconds;
} WorldStreamAudio;

int32_t worldStreamAudioStop(WorldStreamAudio* stream);
int32_t worldStreamAudioCancel(WorldStreamAudio* stream);
void worldStreamAudioFade(WorldStreamAudio* stream,const int32_t* audioEnabled,float duration);
int32_t worldStreamAudioRequestStop(WorldStreamAudio* stream);
WorldStreamAudioResult worldStreamAudioCleanup(WorldStreamAudio* stream);
WorldStreamAudioResult worldStreamAudioDestroy(WorldStreamAudio* stream);
WorldStreamAudioResult worldStreamAudioMute(WorldStreamAudio* stream,int32_t muted);
// Native PC-only cursor suspend. Never substitutes PAL's mute transition.
WorldStreamAudioResult worldStreamAudioPauseForMenu(WorldStreamAudio* stream,int32_t paused);
int32_t worldStreamAudioGetEventPending(WorldStreamAudio* stream,int32_t channel);
int32_t worldStreamAudioQueueSentenceEvent(WorldStreamAudio* stream,int32_t channel);
int32_t worldStreamAudioPlaySentence(WorldStreamAudio* stream,int32_t channel,float volume);
WorldStreamAudioResult worldStreamAudioUpdate(WorldStreamAudio* stream);
WorldStreamAudioResult worldStreamAudioAddSentencePart(WorldStreamAudio* stream,
    int32_t bankIndex,uint32_t offset,uint32_t bytes);
int32_t worldStreamAudioSentencePartRequest(WorldStreamAudio* stream,
    const uint32_t* parameters,size_t parameterCount);
WorldAudioResult worldStreamAudioSampleCallback(WorldStreamAudio* stream,
    uint32_t event,WorldAudioVoiceRef voice);
const struct SpeechEngineState* worldStreamAudioEngine(const WorldStreamAudio* stream);
size_t worldStreamAudioBankCount(const WorldStreamAudio* stream);
size_t worldStreamAudioSegmentCount(const WorldStreamAudio* stream);

/* Native ownership of the eight SPCH event-database views. The retained
 * resource owner keeps these byte ranges alive; registration never copies
 * Events.evt or creates a second pending-message/channel queue. This is
 * not the console's pointer layout. */
typedef struct SpeechEventDatabaseView {
    const uint8_t* bytes;
    size_t size;
    uint32_t channel;
} SpeechEventDatabaseView;

typedef struct SpeechDatabaseRegistry {
    SpeechEventDatabaseView databases[8];
} SpeechDatabaseRegistry;

typedef enum SpeechDatabaseResult {
    SPEECH_DATABASE_OK,
    SPEECH_DATABASE_INVALID_BINDING,
    SPEECH_DATABASE_INVALID_DATA,
    SPEECH_DATABASE_DUPLICATE,
    SPEECH_DATABASE_CAPACITY
} SpeechDatabaseResult;

SpeechDatabaseResult spchBindData(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size,uint32_t channel);
SpeechDatabaseResult spchAddEventDatabase(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size,uint32_t channel);
SpeechDatabaseResult spchResolveData(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size);

/* Bank views address the SAME mutable retained BAN resource. The source
 * keeps its queue cursor and cycle bits inside each bank record. */
typedef struct SpeechBankView {
    uint8_t* bytes;
    size_t size;
} SpeechBankView;

typedef struct SpeechBankRegistry {
    SpeechBankView* banks;
    size_t capacity;
    const uint32_t* engineMagic;
    const uint32_t* cyclePhase;
} SpeechBankRegistry;

SpeechDatabaseResult spchBankBindStorage(SpeechBankRegistry* registry,
    SpeechBankView* storage,size_t capacity,const uint32_t* engineMagic,
    const uint32_t* cyclePhase);
void spchBankDropStorage(SpeechBankRegistry* registry);
const SpeechBankView* spchBankGet(const SpeechBankRegistry* registry,int32_t index);
int32_t spchBankGetFree(const SpeechBankRegistry* registry);
int32_t spchBankFind(const SpeechBankRegistry* registry,uint16_t id);
int32_t spchBankFindSubBank(const SpeechBankRegistry* registry,uint16_t id,uint16_t subId);
int32_t spchBankTestSubBankBounds(const SpeechBankRegistry* registry,int32_t index,uint32_t subBank);
SpeechDatabaseResult spchBankFree(SpeechBankRegistry* registry,int32_t index);
SpeechDatabaseResult spchBankAddToQueue(SpeechBankView* bank,uint8_t sample);
SpeechDatabaseResult spchBankGetSampleTimeInQueue(const SpeechBankView* bank,uint8_t sample,int32_t* time);
SpeechDatabaseResult spchBankSetCycleBits(SpeechBankView* bank,uint32_t cyclePhase);
SpeechDatabaseResult spchAddBank(SpeechBankRegistry* registry,SpeechBankView bank,int32_t* index);

/* This is the speech library's independent six-word RNG and 32-entry
 * category history, not WorldAudioRuntime.randomState or bMath's RNG. */
typedef struct SpeechRandomState {
    uint32_t words[6];
    uint16_t history[32][2];
    int32_t writeIndex;
} SpeechRandomState;

void spchRandomSeed(SpeechRandomState* state,uint32_t seed);
void spchRandomInitialize(SpeechRandomState* state,uint32_t seed);
uint32_t spchRandomNext(SpeechRandomState* state);
int32_t spchRandomFindInQueue(const SpeechRandomState* state,uint16_t value,
    int32_t lookback,uint16_t category);
void spchRandomAddToQueue(SpeechRandomState* state,uint16_t value,uint16_t category);
uint32_t spchRandomPick(SpeechRandomState* state,int32_t bound,uint16_t category);

/* One logical sentence-choice owner. The source initializes48 four-halfword
 * rows and eight phrase records; unrelated source members are not invented
 * here. These are typed CPU values, not a packed SDK structure. */
typedef struct SpeechChoiceRow {
    int16_t first;
    int16_t second;
    uint16_t values[2];
} SpeechChoiceRow;

typedef struct SpeechPhraseChoice {
    /* Native resource-relative offsets replacing the two source pointers. */
    uint32_t values[2];
    uint8_t flags[3];
    uint8_t database;
    uint32_t parameters[20];
} SpeechPhraseChoice;

typedef struct SpeechSentenceChoice {
    int32_t index;
    uint32_t values[2];
    SpeechChoiceRow rows[48];
    SpeechPhraseChoice phrases[8];
} SpeechSentenceChoice;

/* The event log borrows the caller's buffer. Flush writes the released
 * Dstt/Dend records through the source's optional writer before resetting
 * the SAME buffer; no diagnostic side queue or log-success wrapper. */
typedef void (*SpeechEventLogWriter)(const uint8_t* bytes,uint32_t size);
typedef struct SpeechEventLog {
    uint8_t* bytes;
    uint32_t used;
    uint32_t capacity;
    SpeechEventLogWriter writer;
    int32_t enabled;
} SpeechEventLog;

typedef struct SpeechEngineState {
    uint32_t magic;
    WorldAudioMessages* messages;
    SpeechDatabaseRegistry databases;
    SpeechBankRegistry banks;
    SpeechRandomState random;
    SpeechSentenceChoice sentenceChoice;
    struct WorldStreamAudio* sentenceOwner;
    struct WorldStreamAudio* ruleOwner;
    struct WorldStreamAudio* ruleSetterOwner;
    struct WorldStreamAudio* reparameterizeOwner;
    struct WorldStreamAudio* eventRuleOwner;
    struct WorldStreamAudio* externalOwner;
    /* Original SPCH diagnostic context, distinct from the sentence provider.
     * The filename is borrowed for the duration of the failing SDK call. */
    const char* diagnosticFile;
    uint32_t diagnosticLine;
    uint32_t cyclePhase;
    uint32_t bytesPerSecond;
    uint32_t eventCounters[8];
    uint32_t preloadTicks;
    /* Source Init installs its own event rule2919D0 before any custom rule.
     * This records that typed provider role, not an arbitrary callback. */
    int32_t internalEventRule;
    SpeechEventLog eventLog;
    int32_t addingEvent;
    int32_t playingEvent;
    int32_t matchParameter;
    uint32_t matchValue;
    int32_t consumeCycleBits;
    SpeechDatabaseResult operationResult;
} SpeechEngineState;

typedef struct SpeechSample {
    uint32_t bytes;
    uint32_t offset;
    uint32_t parameterCount;
    uint8_t parameters[4];
} SpeechSample;

typedef struct SpeechPhrasePick {
    int16_t bank;
    int16_t subBank;
    uint8_t begin;
    uint8_t count;
    uint8_t choice;
} SpeechPhrasePick;

typedef struct SpeechSentencePick {
    SpeechPhrasePick phrases[12];
    uint8_t phraseCount;
    uint8_t sampleCount;
    uint8_t samples[100];
} SpeechSentencePick;

SpeechBytes spchBytesSlice(SpeechBytes bytes,size_t offset,size_t minimum);
uint8_t spchRead8(SpeechBytes bytes,size_t offset);
uint16_t spchRead16(SpeechBytes bytes,size_t offset);
uint32_t spchRead32(SpeechBytes bytes,size_t offset);
uint32_t spchTick(const SpeechEngineState* engine);
struct WorldStreamAudio* spchGetExtVecs(const SpeechEngineState* engine);
void spchDebugPrint(const SpeechEngineState* engine,const char* format,...);
void spchAbortMessage(SpeechEngineState* engine,const char* format,...);

/* Individually proven SPCH49 original file APIs. A view with bytes==NULL is
 * failure; callers do not turn absent EVT/BAN data into successful speech. */
SpeechBytes spchFindEvent(const SpeechEngineState* engine,uint32_t event);
const SpeechEventDatabaseView* spchFindEventDatabase(const SpeechEngineState* engine,uint32_t event);
SpeechBytes spchSearchEventDatabase(SpeechBytes database,uint16_t event);
int32_t spchFindEventChannel(const SpeechEngineState* engine,uint32_t event,uint32_t* channel);
int32_t spchGetDatabaseId(const SpeechEngineState* engine,uint32_t event,uint32_t* id);
uint32_t spchGetFilterLength(const SpeechEngineState* engine,uint32_t event);
void spchGetEventDatabaseInfo(SpeechBytes database,uint32_t* first,uint32_t* second);
int32_t spchAddEvent(SpeechEngineState* engine,const uint32_t parameters[20]);
int32_t spchAddEventV(SpeechEngineState* engine,uint32_t event,const uint32_t* parameters,size_t count);
int32_t spchFindEventSlot(SpeechEngineState* engine,uint32_t priority,int32_t channel,int32_t follow);
int32_t spchCheckLastEventIndex(const WorldAudioMessages* messages,int32_t channel);
int32_t spchEventInFollowGroup(uint16_t event,SpeechBytes group);
int32_t spchChooseEventSearch(SpeechEngineState* engine,SpeechBytes group,int32_t channel);
int32_t spchChooseEvent(SpeechEngineState* engine,int32_t channel);
void spchClearEvent(SpeechEngineState* engine,int32_t index);
void spchClearOldEvents(SpeechEngineState* engine,int32_t index);
void spchClearEventQueue(SpeechEngineState* engine);
int32_t spchCheckEventQueue(const SpeechEngineState* engine,int32_t channel);
int32_t spchChoose(SpeechEngineState* engine,int32_t channel);
int32_t spchPlay(SpeechEngineState* engine,int32_t channel);
int32_t spchPlayLastEvent(SpeechEngineState* engine,int32_t channel);
void spchSetFilter(SpeechEngineState* engine,uint32_t filter,int32_t channel);
void spchInitializeSample(SpeechSample* sample);
int32_t spchUnpackSample(const SpeechBankView* bank,int32_t index,SpeechSample* sample);
void spchSetMatchParameter(SpeechEngineState* engine,int32_t parameter,uint32_t value);
uint32_t spchGetMatchParameter(const SpeechEngineState* engine,int32_t parameter);
void spchClearMatchParameter(SpeechEngineState* engine);
int32_t spchMatchSample(const SpeechSample* sample,SpeechBytes phrase,const uint32_t parameters[20]);
int32_t spchGetPhraseBank(SpeechEngineState* engine,SpeechBytes phrase,const uint32_t parameters[20],SpeechPhrasePick* pick);
int32_t spchTestBit(SpeechBytes bits,int32_t index);
void spchClearCycleBit(SpeechBankView* bank,int32_t index);
int32_t spchCheckTemplateSample(const SpeechPhrasePick* pick,const SpeechBankView* bank,int32_t sample);
int32_t spchSampleExists(const SpeechPhrasePick* pick,const SpeechBankView* bank,int32_t sample);
int32_t spchChooseSamples(SpeechEngineState* engine,SpeechSentencePick* sentence,SpeechPhrasePick* pick,SpeechBytes phrase,const uint32_t parameters[20]);
int32_t spchConvertTime(const SpeechEngineState* engine,uint32_t bytes);
int32_t spchSentenceLength(SpeechEngineState* engine,SpeechBytes sentence,const SpeechSentencePick* pick);
uint32_t spchDecodeWeight(uint8_t weight);
int32_t spchOrderSentences(SpeechEngineState* engine,SpeechBytes event,uint8_t order[100]);
int32_t spchRepeatEvent(const SpeechEngineState* engine,SpeechBytes event,int32_t channel);
int32_t spchShortRuleStatus(SpeechBytes sentence,int32_t filter);
int32_t spchGetBankParameterId(const SpeechEngineState* engine,int32_t bank,int32_t parameter);
int32_t spchSentenceGetChoices(SpeechEngineState* engine,SpeechSentencePick* pick,SpeechBytes sentence,const uint32_t parameters[20],uint8_t rules,uint8_t unknownRules,int32_t filter);
void spchRandomizePhrasePicks(SpeechEngineState* engine,uint8_t* samples,int32_t count);
void spchRandomizeSentencePicks(SpeechEngineState* engine,SpeechBytes sentence,SpeechSentencePick* pick);
int32_t spchIterateChoice(SpeechBytes sentence,SpeechSentencePick* pick);
int32_t spchChooseShortSentence(SpeechEngineState* engine,SpeechBytes sentence,SpeechSentencePick* pick,int32_t limit);
int32_t spchPickPhraseSample(SpeechEngineState* engine,SpeechSentencePick* sentence,SpeechBytes phrase,SpeechPhrasePick* pick);
int32_t spchSentenceMakeChoice(SpeechEngineState* engine,SpeechSentencePick* pick,SpeechBytes sentence,int32_t filter,int32_t limit);
void spchInitializeSentencePick(SpeechSentencePick* pick);
int32_t spchSaveChosenSentence(SpeechEngineState* engine,const SpeechSentencePick* pick,int32_t channel,SpeechBytes event,SpeechBytes sentence,uint8_t ordinal,const uint32_t parameters[20]);
int32_t spchOneChosen(const SpeechEngineState* engine,int32_t channel);
int32_t spchPlayChosen(SpeechEngineState* engine,int32_t channel);
int32_t spchTestValidParameter(uint32_t descriptor,uint32_t value,uint32_t index);
int32_t spchChooseSentence(SpeechEngineState* engine,const uint32_t parameters[20]);
int32_t spchChooseSingleSentence(SpeechEngineState* engine,int32_t index);
int32_t spchSentenceUsesParameter(SpeechBytes sentence,uint32_t parameter);
int32_t spchGetRuleId(SpeechBytes event,uint32_t index);
void spchRuleSet(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,uint8_t sentenceIndex,const uint32_t parameters[20]);
uint8_t spchGetRuleSettings(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,const uint32_t parameters[20],uint8_t* unknown);
int32_t spchCheckSentenceRules(uint8_t rules,uint8_t unknown,SpeechBytes sentence);
void spchConstantRuleSet(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,SpeechBytes sentence,uint32_t firstRow);
int32_t spchMakeSampleRequests(SpeechEngineState* engine,SpeechBytes sentence,uint32_t eventId,uint32_t firstRow);
void spchEventLogAddEvent(SpeechEngineState* engine,SpeechBytes event,const uint32_t parameters[20]);
void spchEventLogTryEvent(SpeechEngineState* engine,SpeechBytes event,const uint32_t parameters[20],uint32_t rules);
void spchEventLogEventRejected(SpeechEngineState* engine,SpeechBytes event,uint32_t tick,uint32_t reason);
void spchEventLogEventChoice(SpeechEngineState* engine,SpeechBytes event,uint32_t index,uint32_t chosen);
void spchEventLogSentenceChoice(SpeechEngineState* engine,SpeechBytes event,SpeechBytes sentence,const uint32_t parameters[20]);
void spchEventLogRuleSettings(SpeechEngineState* engine,uint32_t rules);
void spchEventLogFilter(SpeechEngineState* engine,uint32_t filter);
void spchEventLogSampleRequest(SpeechEngineState* engine,const SpeechBankView* bank,const SpeechChoiceRow* row,const uint32_t parameters[7]);
void spchEventLogMessage(SpeechEngineState* engine,const char* message);

void spchInitializeChannels(WorldAudioMessages* messages);
void spchInitializeEventDatabases(SpeechDatabaseRegistry* registry);
void spchInitializeEventQueue(WorldAudioMessages* messages);
void spchInitializeSentenceChoice(SpeechSentenceChoice* choice);
void spchSetPreloadTicks(SpeechEngineState* engine,uint32_t ticks);
int32_t spchGetSampleDataRate(int32_t sampleRate,int32_t bits,int32_t encoding);
SpeechDatabaseResult spchEngineInitialize(SpeechEngineState* engine,
    WorldAudioMessages* messages,struct WorldStreamAudio* sentenceOwner,
    uint32_t seed,uint32_t bytesPerSecond);
SpeechDatabaseResult spchInitializeRuleCallbacks(SpeechEngineState* engine,
    struct WorldStreamAudio* ruleOwner);
SpeechDatabaseResult spchInitializeReparameterizeCallback(SpeechEngineState* engine,
    struct WorldStreamAudio* owner);
SpeechDatabaseResult spchInitializeEventRuleCallback(SpeechEngineState* engine,
    struct WorldStreamAudio* owner);
SpeechDatabaseResult spchEngineDeinitialize(SpeechEngineState* engine);
SpeechDatabaseResult spchEventLogInitialize(SpeechEngineState* engine,
    uint8_t* bytes,uint32_t capacity,SpeechEventLogWriter writer);
void spchEventLogDeinitialize(SpeechEngineState* engine);
void spchEventLogSave(SpeechEngineState* engine);
void spchEventLogSaveStart(SpeechEngineState* engine);
void spchEventLogClear(SpeechEngineState* engine);
void spchEventLogSaveEnd(SpeechEngineState* engine);
SpeechDatabaseResult spchEventLogSaveChunk(SpeechEngineState* engine,
    const uint8_t* record,uint32_t recordBytes);
int32_t worldStreamAudioTimerGetTick(const struct WorldStreamAudio* stream);

#ifdef __cplusplus
}

namespace hp2 { class GameResources; }
struct GameTime;
struct CarSoundGlobals;
struct SoundDriverState;

// Borrow actual application policy/time owners. No new launch flag, locale,
// sound-library singleton, device queue or copied Timer is manufactured.
WorldStreamAudioResult worldStreamAudioBind(WorldStreamAudio& stream,
    WorldAudioMessages& messages,bSound& sound,const hp2::GameResources& files,
    const GameTime& clock,const CarSoundGlobals& globals,const SoundDriverState& driver);
WorldStreamAudioResult worldStreamAudioInitialize(WorldStreamAudio& stream,
    const char* eventsFilename,const char* unusedHeaderFilename,
    const char* dataFilename,const char* banksFilename);
WorldStreamAudioResult soundLoadLocalizedSpeech(WorldStreamAudio& stream,int32_t language);
WorldStreamAudioResult soundUnloadLocalizedSpeech(WorldStreamAudio& stream);
#endif
#endif

#include "indep/src/speech/SEDWedge.hpp"

#include <stdlib.h>
#include <string.h>

void spchInitializeEventDatabases(SpeechDatabaseRegistry* registry)
{
    if (!registry) abort();
    for (size_t index=0;index<8;++index) {
        registry->databases[index].bytes=NULL;
        registry->databases[index].size=0;
        registry->databases[index].channel=0;
    }
}

void spchInitializeEventQueue(WorldAudioMessages* messages)
{
    if (!messages) abort();
    for (size_t index=0;index<WORLD_AUDIO_BANK_LIMIT;++index) {
        messages->bankReferences[index]=0;
        messages->eventSelection[0][index]=-1;
        messages->eventSelection[1][index]=-1;
    }
    messages->selectionState=0;
    for (size_t index=0;index<WORLD_AUDIO_MESSAGE_LIMIT;++index) {
        WorldAudioMessage* event=&messages->pending[index];
        event->category=0;
        event->tick=0;event->order=0;event->definition.bytes=NULL;event->definition.size=0;
        event->follow=0;event->nextFollow=-1;
        event->active=0;
        event->bankIndex=-1;
        event->dispatched=0;
        event->parameterCount=0;
        event->sequence=0;
        event->epoch=0;
        for (size_t parameter=0;parameter<WORLD_AUDIO_MESSAGE_PARAMETERS;++parameter)
            event->parameters[parameter]=0;
    }
    messages->selectionTick=0;
    messages->selectionFlags=0;
    /* Native handles additionally carry an epoch. An SDK queue reset must
     * invalidate outstanding handles even when the same slot is reused. */
    ++messages->epoch;
    messages->nextSequence=0;
    messages->result=WORLD_AUDIO_MESSAGES_OK;
}

/* Initialize the single Windows-owned speech message queue. */
void worldAudioMessagesInitialize(WorldAudioMessages* messages)
{
    memset(messages,0,sizeof(*messages));
    for (size_t index=0;index<WORLD_AUDIO_BANK_LIMIT;++index)
        messages->channels[index].id=-1;
    for (size_t index=0;index<WORLD_AUDIO_MESSAGE_LIMIT;++index)
        messages->pending[index].bankIndex=-1;
}



/* PAL291250..29144C. Event identifiers retain their database bytes; the
 * native implementation never treats a console resource pointer as a host one. */
SpeechBytes spchSearchEventDatabase(SpeechBytes database,uint16_t event)
{
    SpeechBytes none={NULL,0};
    if (database.size<24) return none;
    const uint32_t count=spchRead16(database,16);
    if (!spchBytesSlice(database,24,count*2u).bytes) return none;
    for (uint32_t index=0;index<count;++index) {
        SpeechBytes record=spchBytesSlice(database,(uint32_t)spchRead16(database,24u+index*2u)*4u,12);
        if (record.bytes && spchRead16(record,0)==event) return record;
    }
    return none;
}
const SpeechEventDatabaseView* spchFindEventDatabase(const SpeechEngineState* engine,uint32_t event)
{
    if (!engine) return NULL;
    for (size_t index=0;index<8;++index) {
        const SpeechEventDatabaseView* database=&engine->databases.databases[index];
        if (database->bytes && database->size>=10 && database->bytes[9]==((event>>16)&255u) &&
            database->bytes[8]==(event>>24)) return database;
    }
    return NULL;
}
SpeechBytes spchFindEvent(const SpeechEngineState* engine,uint32_t event)
{
    const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,event);
    const SpeechBytes bytes={database?database->bytes:NULL,database?database->size:0};
    return spchSearchEventDatabase(bytes,(uint16_t)event);
}
int32_t spchFindEventChannel(const SpeechEngineState* engine,uint32_t event,uint32_t* channel)
{
    const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,event);
    if (!database || !channel) return 0;
    *channel=database->channel;return 1;
}
int32_t spchGetDatabaseId(const SpeechEngineState* engine,uint32_t event,uint32_t* id)
{
    const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,event);
    if (!database || !id) return 0;
    *id=database->bytes[9];return 1;
}
uint32_t spchGetFilterLength(const SpeechEngineState* engine,uint32_t event)
{
    const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,event);
    if (!database || database->size<22) return 0;
    const SpeechBytes bytes={database->bytes,database->size};return spchRead16(bytes,20);
}

int32_t spchCheckLastEventIndex(const WorldAudioMessages* messages,int32_t channel)
{
    return messages && channel>=0 && channel<8 &&
        (uint32_t)messages->eventSelection[0][channel]<16 &&
        (uint32_t)messages->eventSelection[1][channel]<16;
}

void spchClearEvent(SpeechEngineState* engine,int32_t index)
{
    if (!engine || !engine->messages || (uint32_t)index>=16) return;
    WorldAudioMessages* queue=engine->messages;
    WorldAudioMessage* event=&queue->pending[index];
    if (event->follow || event->bankIndex<0 || event->bankIndex>=8) {
        engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return;
    }
    const int32_t channel=event->bankIndex;
    event->active=0;
    if (queue->eventSelection[1][channel]==index) queue->eventSelection[1][channel]=-1;
    if (queue->eventSelection[0][channel]==index) queue->eventSelection[0][channel]=-1;
    int32_t next=event->nextFollow;
    for (size_t step=0;next!=-1;++step) {
        if ((uint32_t)next>=16 || step>=16) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;break;}
        WorldAudioMessage* follow=&queue->pending[next];
        if (!follow->follow || !follow->active) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;break;}
        if (queue->eventSelection[0][channel]==next) queue->eventSelection[0][channel]=-1;
        follow->active=0;next=follow->nextFollow;
    }
    --queue->bankReferences[channel];
    if (queue->bankReferences[channel]<0) {
        queue->bankReferences[channel]=0;engine->operationResult=SPEECH_DATABASE_INVALID_DATA;
    }
}

int32_t spchFindEventSlot(SpeechEngineState* engine,uint32_t priority,int32_t channel,int32_t follow)
{
    if (!engine || !engine->messages || (uint32_t)channel>=8) return -1;
    WorldAudioMessages* queue=engine->messages;
    for (int32_t i=0;i<16;++i) if (!queue->pending[i].active) return i;
    const int32_t protectedIndex=follow?queue->eventSelection[1][channel]:-1;
    const uint32_t tick=spchTick(engine);
    for (int32_t i=0;i<16;++i) {
        WorldAudioMessage* event=&queue->pending[i];
        if (event->follow || !event->definition.bytes) continue;
        const uint32_t expires=spchRead16(event->definition,2);
        if (expires && expires<tick-event->tick && i!=protectedIndex) {
            spchClearEvent(engine,i);return i;
        }
    }
    for (int32_t i=0;i<16;++i) {
        WorldAudioMessage* event=&queue->pending[i];
        if (event->follow || !event->definition.bytes || priority<spchRead16(event->definition,4) ||
            event->bankIndex!=channel || i==protectedIndex) continue;
        event->active=0;--queue->bankReferences[channel];return i;
    }
    return -1;
}

int32_t spchAddEvent(SpeechEngineState* engine,const uint32_t parameters[20])
{
    if (!engine || !engine->messages || !parameters || engine->magic!=0x01789A34u) return 0;
    if (engine->addingEvent) {engine->addingEvent=0;engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;return 0;}
    engine->addingEvent=1;
    const SpeechBytes definition=spchFindEvent(engine,parameters[0]);
    uint32_t channel=0;
    if (!definition.bytes || !spchFindEventChannel(engine,parameters[0],&channel) || channel>=8) {
        engine->addingEvent=0;engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return 0;
    }
    spchEventLogAddEvent(engine,definition,parameters);
    WorldAudioMessages* queue=engine->messages;
    const int32_t follow=(spchRead8(definition,10)>>5)&1;
    int32_t previous=-1,head=-1;
    uint32_t priority=spchRead16(definition,4);
    if (follow) {
        if (!spchCheckLastEventIndex(queue,(int32_t)channel)) {
            engine->addingEvent=0;engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return 0;
        }
        head=queue->eventSelection[1][channel];previous=queue->eventSelection[0][channel];
        priority=spchRead16(queue->pending[head].definition,4);
    } else if ((int8_t)spchRead8(definition,9)<(int32_t)spchRandomPick(&engine->random,100,UINT16_MAX)) {
        engine->addingEvent=0;return 0;
    }
    const int32_t index=spchFindEventSlot(engine,priority,(int32_t)channel,follow);
    if (index<0) {
        if (follow) spchClearEvent(engine,head);
        engine->addingEvent=0;engine->operationResult=SPEECH_DATABASE_CAPACITY;return 0;
    }
    const uint32_t tick=spchTick(engine);
    if (tick==queue->selectionTick) ++queue->selectionFlags; else queue->selectionFlags=0;
    queue->selectionTick=tick;
    WorldAudioMessage* event=&queue->pending[index];
    event->definition=definition;event->bankIndex=(int32_t)channel;event->tick=tick;
    event->nextFollow=-1;event->order=queue->selectionFlags;event->follow=(int16_t)follow;
    if (follow) queue->pending[previous].nextFollow=(int16_t)index;
    event->category=parameters[0];memcpy(event->parameters,parameters+1,sizeof(event->parameters));
    event->parameterCount=19;event->sequence=queue->nextSequence++;event->epoch=queue->epoch;event->dispatched=0;
    if (!follow) ++queue->bankReferences[channel];
    event->active=1;queue->eventSelection[0][channel]=index;
    if (!follow) queue->eventSelection[1][channel]=index;
    engine->addingEvent=0;return 1;
}

int32_t spchAddEventV(SpeechEngineState* engine,uint32_t event,const uint32_t* parameters,size_t count)
{
    if (count>19 || (!parameters && count)) return 0;
    uint32_t values[20]={0};values[0]=event;
    if (count) memcpy(values+1,parameters,count*sizeof(*parameters));
    return spchAddEvent(engine,values);
}

int32_t spchEventInFollowGroup(uint16_t event,SpeechBytes group)
{
    if (!group.bytes || group.size<2) return 0;
    const uint32_t count=spchRead8(group,0);
    if (!spchBytesSlice(group,2,count*2u).bytes) return 0;
    for (uint32_t i=0;i<count;++i) if (spchRead16(group,2u+i*2u)==event) return 1;
    return 0;
}

int32_t spchChooseEventSearch(SpeechEngineState* engine,SpeechBytes group,int32_t channel)
{
    if (!engine || !engine->messages || (uint32_t)channel>=8) return -1;
    uint32_t youngest=UINT32_MAX,bestPriority=0,bestOrder=0;
    int32_t best=-1;
    const uint32_t tick=spchTick(engine)+engine->preloadTicks;
    for (int32_t i=0;i<16;++i) {
        WorldAudioMessage* event=&engine->messages->pending[i];
        if (!event->active || event->bankIndex!=channel || event->follow || !event->definition.bytes) continue;
        const SpeechBytes definition=event->definition;
        const int32_t inGroup=!group.bytes || spchEventInFollowGroup(spchRead16(definition,0),group);
        const uint32_t age=tick-event->tick,lifetime=spchRead16(definition,2);
        int32_t shortRejected=0;
        if (engine->eventCounters[channel]==1 && (spchRead8(definition,10)&1u)) {
            const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,event->category);
            if (database && database->size>=24) {
                const SpeechBytes bytes={database->bytes,database->size};
                shortRejected=spchRead16(definition,4)<spchRead16(bytes,22);
            }
        }
        if (lifetime && lifetime<age) {
            spchEventLogEventRejected(engine,definition,event->tick,0);spchClearEvent(engine,i);continue;
        }
        if (!inGroup) spchEventLogEventRejected(engine,definition,event->tick,4);
        if (shortRejected) spchEventLogEventRejected(engine,definition,event->tick,1);
        if (!inGroup || shortRejected) continue;
        const uint32_t priority=spchRead16(definition,4);
        if (bestPriority<priority || (bestPriority==priority &&
            (age<youngest || (age==youngest && bestOrder<event->order)))) {
            best=i;bestPriority=priority;youngest=age;bestOrder=event->order;
        }
    }
    return best;
}

int32_t spchChooseEvent(SpeechEngineState* engine,int32_t channel)
{
    if (!engine || !engine->messages || (uint32_t)channel>=8) return -1;
    const WorldAudioMessageChannel* last=&engine->messages->channels[channel];
    SpeechBytes group={NULL,0};
    if ((uint16_t)last->id!=UINT16_MAX) {
        const SpeechBytes previous=spchFindEvent(engine,(uint16_t)last->id|((uint32_t)last->databaseId<<16));
        if (previous.bytes && (spchRead8(previous,10)&16u)) {
            const uint32_t sentences=spchRead8(previous,6);
            const uint32_t rules=(uint32_t)(int32_t)(int8_t)spchRead8(previous,7);
            uint32_t offset=12u+((sentences*2u+3u)&~3u)+((rules*3u+3u)&~3u);
            if (spchRead8(previous,10)&8u) offset+=(sentences*2u+7u)&~3u;
            group=spchBytesSlice(previous,offset,2);
        }
    }
    int32_t choice=spchChooseEventSearch(engine,group,channel);
    if (group.bytes && choice<0) {const SpeechBytes none={NULL,0};choice=spchChooseEventSearch(engine,none,channel);}
    return choice;
}

void spchClearOldEvents(SpeechEngineState* engine,int32_t index)
{
    if (!engine || !engine->messages || (uint32_t)index>=16) return;
    WorldAudioMessages* queue=engine->messages;
    const WorldAudioMessage* chosen=&queue->pending[index];
    const uint32_t tick=chosen->tick;const uint16_t order=chosen->order;const int32_t channel=chosen->bankIndex;
    queue->selectionState=0;
    for (int32_t i=0;i<16;++i) {
        WorldAudioMessage* event=&queue->pending[i];
        if (i==index || !event->active || event->bankIndex!=channel || event->follow || !event->definition.bytes) continue;
        if (event->tick<tick || (event->tick==tick && event->order<order)) {
            if (!(spchRead8(event->definition,10)&4u)) {
                spchClearEvent(engine,i);spchEventLogEventRejected(engine,event->definition,event->tick,3);
            }
        } else if ((int8_t)spchRead8(event->definition,9)==100) queue->selectionState=1;
    }
}

void spchClearEventQueue(SpeechEngineState* engine)
{
    if (!engine || !engine->messages) return;
    WorldAudioMessages* queue=engine->messages;
    for (size_t i=0;i<16;++i) {
        WorldAudioMessage* event=&queue->pending[i];
        if (event->active) {
            event->active=0;
            if ((uint32_t)event->bankIndex<8) --queue->bankReferences[event->bankIndex];
        }
    }
    /* The source repairs its diagnostic underflow after follow entries,
     * then resets only the per-channel repeat state, not selection indices. */
    for (size_t i=0;i<8;++i) queue->bankReferences[i]=0;
    spchInitializeChannels(queue);++queue->epoch;
}
int32_t spchCheckEventQueue(const SpeechEngineState* engine,int32_t channel)
{
    return engine && engine->messages && (uint32_t)channel<8?engine->messages->bankReferences[channel]:0;
}

int32_t spchChoose(SpeechEngineState* engine,int32_t channel)
{
    if (!spchCheckEventQueue(engine,channel)) return 0;
    /* SED installs no custom EventRule. A tagged but unavailable provider is
     * an explicit unsupported binding, not an invented successful rule. */
    if (engine->eventRuleOwner) {engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;return 0;}
    const int32_t selected=spchChooseEvent(engine,channel);
    if (selected<0) return 0;
    spchClearOldEvents(engine,selected);spchInitializeSentenceChoice(&engine->sentenceChoice);
    int32_t current=selected,result=0;
    for (size_t step=0;step<16;++step) {
        result=spchChooseSingleSentence(engine,current);
        current=engine->messages->pending[current].nextFollow;
        if (current==-1 || !result) break;
        if ((uint32_t)current>=16 || step==15) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;result=0;break;}
    }
    spchClearEvent(engine,selected);return result;
}
int32_t spchPlay(SpeechEngineState* engine,int32_t channel)
{
    if (!engine) return 0;
    if (engine->playingEvent) {engine->playingEvent=0;engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;return 0;}
    engine->playingEvent=1;int32_t result=0;
    if (spchOneChosen(engine,channel) || spchChoose(engine,channel)) result=spchPlayChosen(engine,channel);
    engine->playingEvent=0;return result;
}
int32_t spchPlayLastEvent(SpeechEngineState* engine,int32_t channel)
{
    if (!engine || !spchCheckLastEventIndex(engine->messages,channel)) return 0;
    const int32_t selected=engine->messages->eventSelection[1][channel];
    spchInitializeSentenceChoice(&engine->sentenceChoice);
    const int32_t chosen=spchChooseSingleSentence(engine,selected);
    spchClearEvent(engine,selected);
    return chosen?spchPlayChosen(engine,channel):0;
}
void spchSetFilter(SpeechEngineState* engine,uint32_t filter,int32_t channel)
{
    if (!engine || (uint32_t)channel>=8) return;
    engine->eventCounters[channel]=filter;
}

/* Native compatibility API for existing callers. The speculative queue
 * implementation was removed; producers now enter the actual EVT-driven
 * AddEvent, with the same one SPCH queue and resource owner. */
WorldAudioMessagesResult worldAudioMessagesEnqueue(WorldAudioMessages* messages,uint32_t event,const uint32_t* parameters,size_t count)
{
    if (!messages) return WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    if (count>19 || (!parameters && count))
        return messages->result=WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    /* soundLoadLocalizedSpeech deliberately leaves SPCH uninitialized when
     * sound or speech is disabled. Use its bound policy at this native
     * admission boundary, before requiring an engine. No event, sentence or
     * device completion is manufactured. Enabled speech still requires the
     * actual initialized owner, including after a policy is changed back. */
    if ((messages->audioEnabled && !*messages->audioEnabled) ||
        (messages->speechEnabled && !*messages->speechEnabled))
        return messages->result=WORLD_AUDIO_MESSAGES_OK;
    if (!messages->engine || messages->engine->magic!=0x01789A34u)
        return messages->result=WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    SpeechEngineState* engine=messages->engine;engine->operationResult=SPEECH_DATABASE_OK;
    const int32_t added=spchAddEventV(engine,event,parameters,count);
    if (added || engine->operationResult==SPEECH_DATABASE_OK)
        return messages->result=WORLD_AUDIO_MESSAGES_OK; /* authored probability may reject */
    messages->result=engine->operationResult==SPEECH_DATABASE_CAPACITY?WORLD_AUDIO_MESSAGES_CAPACITY:WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
    return messages->result;
}
void worldAudioMessagesClear(WorldAudioMessages* messages)
{
    if (!messages) return;
    if (messages->engine) spchClearEventQueue(messages->engine);
    else {spchInitializeEventQueue(messages);spchInitializeChannels(messages);}
}
WorldAudioMessageHandle worldAudioMessagesNext(WorldAudioMessages* messages)
{
    WorldAudioMessageHandle result={NULL,0,0};
    if (!messages) return result;
    for (size_t i=0;i<16;++i) {
        WorldAudioMessage* candidate=&messages->pending[i];
        if (!candidate->active || candidate->dispatched) continue;
        if (!result.message || candidate->sequence<result.sequence) {
            result.message=candidate;result.sequence=candidate->sequence;result.epoch=candidate->epoch;
        }
    }
    if (result.message) result.message->dispatched=1;
    return result;
}
static int32_t messageIndex(WorldAudioMessages* messages,WorldAudioMessageHandle handle)
{
    if (!messages || !handle.message || handle.epoch!=messages->epoch) return -1;
    for (int32_t i=0;i<16;++i) if (&messages->pending[i]==handle.message) {
        WorldAudioMessage* message=&messages->pending[i];
        if (message->active && message->sequence==handle.sequence && message->epoch==handle.epoch) return i;
    }
    return -1;
}
void worldAudioMessageComplete(WorldAudioMessages* messages,WorldAudioMessageHandle handle)
{
    const int32_t index=messageIndex(messages,handle);
    if (index>=0 && messages->engine) spchClearEvent(messages->engine,index);
}
WorldAudioMessagesResult worldAudioMessageBindBank(WorldAudioMessages* messages,WorldAudioMessageHandle handle,int32_t bank)
{
    const int32_t index=messageIndex(messages,handle);
    /* The real database owns the channel assignment. The old helper must
     * not overwrite it or double-increment its reference count. */
    return index>=0 && messages->pending[index].bankIndex==bank?
        WORLD_AUDIO_MESSAGES_OK:WORLD_AUDIO_MESSAGES_INVALID_REQUEST;
}

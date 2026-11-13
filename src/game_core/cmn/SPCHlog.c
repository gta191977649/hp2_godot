#include "indep/src/speech/SEDWedge.hpp"

#include <string.h>

/* Individually matched final PAL294648..294978. Header fields are written
 * bytewise because the retained log is a released little-endian byte stream,
 * not an overlay of a console structure or a host-aligned uint32 array. */
static uint32_t readWord(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1]<<8 |
        (uint32_t)bytes[2]<<16 | (uint32_t)bytes[3]<<24;
}

static void writeWord(uint8_t* bytes,uint32_t value)
{
    bytes[0]=(uint8_t)value;bytes[1]=(uint8_t)(value>>8);
    bytes[2]=(uint8_t)(value>>16);bytes[3]=(uint8_t)(value>>24);
}



SpeechDatabaseResult spchEventLogInitialize(SpeechEngineState* engine,
    uint8_t* bytes,uint32_t capacity,SpeechEventLogWriter writer)
{
    if (!engine || engine->magic!=0x01789A34u)
        return SPEECH_DATABASE_INVALID_BINDING;
    /* Native bounds validation precedes the original writes. The released
     * lifecycle requires a24-byte start record and a12-byte closing record. */
    if ((bytes && capacity<36u) || (!bytes && capacity))
        return SPEECH_DATABASE_INVALID_DATA;
    engine->eventLog.bytes=bytes;
    engine->eventLog.capacity=capacity;
    engine->eventLog.writer=writer;
    engine->eventLog.enabled=1;
    engine->eventLog.used=0;
    spchEventLogClear(engine);
    return SPEECH_DATABASE_OK;
}

void spchEventLogDeinitialize(SpeechEngineState* engine)
{
    if (!engine)return;
    engine->eventLog.bytes=NULL;
    engine->eventLog.enabled=0;
}

void spchEventLogSave(SpeechEngineState* engine)
{
    if (!engine || !engine->eventLog.enabled)return;
    SpeechEventLog* log=&engine->eventLog;
    if (log->bytes)spchEventLogSaveEnd(engine);
    if (log->writer)log->writer(log->bytes,log->used);
    log->used=0;
    spchEventLogClear(engine);
}

void spchEventLogSaveStart(SpeechEngineState* engine)
{
    if (!engine || !engine->eventLog.enabled || !engine->eventLog.bytes)return;
    uint8_t record[24];
    writeWord(record,0x44737474u);
    writeWord(record+4,12u);
    writeWord(record+8,spchTick(engine));
    writeWord(record+12,0x00030800u);
    writeWord(record+16,engine->cyclePhase);
    writeWord(record+20,0u);
    (void)spchEventLogSaveChunk(engine,record,sizeof(record));
}

void spchEventLogClear(SpeechEngineState* engine)
{
    if (!engine || !engine->eventLog.enabled || !engine->eventLog.bytes)return;
    engine->eventLog.used=0;
    memset(engine->eventLog.bytes,0,engine->eventLog.capacity);
    spchEventLogSaveStart(engine);
}

void spchEventLogSaveEnd(SpeechEngineState* engine)
{
    if (!engine || !engine->eventLog.enabled)return;
    SpeechEventLog* log=&engine->eventLog;
    if (!log->bytes || log->used>log->capacity || log->capacity-log->used<12u)return;
    writeWord(log->bytes+log->used,0x44656E64u);
    writeWord(log->bytes+log->used+4,0u);
    writeWord(log->bytes+log->used+8,spchTick(engine));
    log->used+=12u;
}

SpeechDatabaseResult spchEventLogSaveChunk(SpeechEngineState* engine,
    const uint8_t* record,uint32_t recordBytes)
{
    if (!engine)return SPEECH_DATABASE_INVALID_BINDING;
    SpeechEventLog* log=&engine->eventLog;
    if (!log->enabled)return SPEECH_DATABASE_OK;
    if (!log->bytes || !record || recordBytes<12u)
        return SPEECH_DATABASE_INVALID_DATA;
    const uint32_t payload=readWord(record+4);
    if (payload>recordBytes-12u)return SPEECH_DATABASE_INVALID_DATA;
    const uint32_t bytes=payload+12u;
    /* The source's flush comparison is >=. Reject impossible records before
     * that recursive Save/Clear/Start chain can overwrite its buffer. */
    if (log->capacity<36u || bytes>log->capacity-36u) {
        if (!(log->used==0 && bytes==24u && log->capacity>=36u))
            return SPEECH_DATABASE_CAPACITY;
    }
    if (log->used && ((uint64_t)log->used+bytes+12u>=log->capacity))
        spchEventLogSave(engine);
    if (log->used>log->capacity || bytes>log->capacity-log->used)
        return SPEECH_DATABASE_CAPACITY;
    /* SPCHMEM_blockmove uses a forward byte loop, including overlap. */
    for (uint32_t index=0;index<bytes;++index)
        log->bytes[log->used+index]=record[index];
    log->used+=bytes;
    return SPEECH_DATABASE_OK;
}

/* PAL294980..294F14 release event records. Serialization uses the same
 * retained event log as lifecycle48; no additional diagnostic owner. */
static int32_t beginRecord(SpeechEngineState* engine,uint8_t* record,uint32_t tag,uint32_t payload)
{
    if (!engine || !engine->eventLog.enabled || !engine->eventLog.bytes) return 0;
    writeWord(record,tag);writeWord(record+4,payload);writeWord(record+8,spchTick(engine));return 1;
}
static void copyParameters(uint8_t* output,const uint32_t* values,size_t count)
{
    for (size_t i=0;i<count;++i) writeWord(output+i*4,values[i]);
}
void spchEventLogAddEvent(SpeechEngineState* engine,SpeechBytes event,const uint32_t parameters[20])
{
    uint8_t record[104];if (!beginRecord(engine,record,0x44676576u,92)) return;
    if (event.size<12) return;
    memcpy(record+12,event.bytes,12);copyParameters(record+24,parameters,20);
    (void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogTryEvent(SpeechEngineState* engine,SpeechBytes event,const uint32_t parameters[20],uint32_t rules)
{
    uint8_t record[108];if (!beginRecord(engine,record,0x44746576u,96)) return;
    if (event.size<12) return;
    memcpy(record+12,event.bytes,12);copyParameters(record+24,parameters,20);writeWord(record+104,rules);
    (void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogFilter(SpeechEngineState* engine,uint32_t filter)
{
    uint8_t record[16];if (!beginRecord(engine,record,0x44666c74u,4)) return;
    writeWord(record+12,filter);(void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogRuleSettings(SpeechEngineState* engine,uint32_t rules)
{
    uint8_t record[16];if (!beginRecord(engine,record,0x44726c73u,4)) return;
    writeWord(record+12,rules);(void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogSentenceChoice(SpeechEngineState* engine,SpeechBytes event,SpeechBytes sentence,const uint32_t parameters[20])
{
    uint8_t record[1024];
    if (!engine || !engine->eventLog.enabled || !engine->eventLog.bytes || event.size<12 || sentence.size<4) return;
    const uint32_t count=spchRead8(sentence,3)>>2,payload=96u+count*16u;
    if (payload>sizeof(record)-12u || !beginRecord(engine,record,0x4473656eu,payload)) return;
    memcpy(record+12,event.bytes,12);memcpy(record+24,sentence.bytes,4);
    for (uint32_t i=0;i<count;++i) {
        const SpeechBytes phrase=spchBytesSlice(sentence,(uint32_t)spchRead8(sentence,4u+i)*4u,16);
        if (!phrase.bytes) return;
        memcpy(record+28u+i*16u,phrase.bytes,16);
    }
    copyParameters(record+28u+count*16u,parameters,20);
    (void)spchEventLogSaveChunk(engine,record,payload+12u);
}
void spchEventLogEventChoice(SpeechEngineState* engine,SpeechBytes event,uint32_t index,uint32_t chosen)
{
    uint8_t record[36];if (!beginRecord(engine,record,0x44656368u,24) || event.size<12) return;
    writeWord(record+12,index);writeWord(record+16,chosen);writeWord(record+20,0);
    memcpy(record+24,event.bytes,12);(void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogEventRejected(SpeechEngineState* engine,SpeechBytes event,uint32_t tick,uint32_t reason)
{
    uint8_t record[32];if (!beginRecord(engine,record,0x4472656au,20) || event.size<12) return;
    writeWord(record+12,reason);writeWord(record+16,tick);memcpy(record+20,event.bytes,12);
    (void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogSampleRequest(SpeechEngineState* engine,const SpeechBankView* bank,const SpeechChoiceRow* row,const uint32_t parameters[7])
{
    uint8_t record[60];if (!beginRecord(engine,record,0x44737271u,48) || !bank || bank->size<12) return;
    memcpy(record+12,bank->bytes,12);
    writeWord(record+24,(uint16_t)row->first|((uint32_t)(uint16_t)row->second<<16));
    writeWord(record+28,row->values[0]|((uint32_t)row->values[1]<<16));
    copyParameters(record+32,parameters,7);(void)spchEventLogSaveChunk(engine,record,sizeof(record));
}
void spchEventLogMessage(SpeechEngineState* engine,const char* message)
{
    uint8_t record[268];if (!beginRecord(engine,record,0x446d7367u,256) || !message) return;
    /* PAL copies256 bytes even past a string terminator. Native diagnostics
     * retain the authored text and fixed record size without reading beyond
     * a host allocation; the padding is explicitly zero, not asserted PAL. */
    memset(record+12,0,256);
    size_t count=0;while (count<256 && message[count]) ++count;
    memcpy(record+12,message,count);(void)spchEventLogSaveChunk(engine,record,sizeof(record));
}

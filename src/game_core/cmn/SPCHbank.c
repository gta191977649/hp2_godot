#include "indep/src/speech/SEDWedge.hpp"

#include <limits.h>

enum { SPEECH_ENGINE_MAGIC = 0x01789A34 };

static uint16_t bankWord(const uint8_t* bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | (uint16_t)bytes[1]<<8);
}

/* Actual consumers290E90/290F58/291018/2910E8 all compute this same
 * variable-length tail. The source has no separate copy of queue/cycle
 * state; these offsets continue to address the retained bank's bytes. */
static SpeechDatabaseResult bankTail(const SpeechBankView* bank,
    size_t* cycleOffset,size_t* queueOffset)
{
    if (!bank || !bank->bytes) return SPEECH_DATABASE_INVALID_BINDING;
    if (bank->size<12) return SPEECH_DATABASE_INVALID_DATA;
    const uint8_t flags=bank->bytes[4];
    const size_t samples=bank->bytes[5];
    const size_t tail=12+samples*((flags&15u)+2u)+((flags&0x70u)>>3);
    if (tail>bank->size) return SPEECH_DATABASE_INVALID_DATA;
    const size_t cycleSize=(flags&0x80u) ? ((samples+7)/8)+1 : 0;
    if (cycleSize>bank->size-tail) return SPEECH_DATABASE_INVALID_DATA;
    *cycleOffset=tail;
    *queueOffset=tail+cycleSize;
    const size_t queued=bank->bytes[6];
    if (queued && (*queueOffset==bank->size ||
        queued>bank->size-*queueOffset-1))
        return SPEECH_DATABASE_INVALID_DATA;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchBankBindStorage(SpeechBankRegistry* registry,
    SpeechBankView* storage,size_t capacity,const uint32_t* engineMagic,
    const uint32_t* cyclePhase)
{
    if (!registry || !storage || !capacity || capacity>INT32_MAX ||
        !engineMagic || !cyclePhase)
        return SPEECH_DATABASE_INVALID_BINDING;
    registry->banks=storage;
    registry->capacity=capacity;
    registry->engineMagic=engineMagic;
    registry->cyclePhase=cyclePhase;
    for (size_t index=0;index<capacity;++index) {
        storage[index].bytes=NULL;
        storage[index].size=0;
    }
    return SPEECH_DATABASE_OK;
}

void spchBankDropStorage(SpeechBankRegistry* registry)
{
    if (!registry) return;
    registry->banks=NULL;
    registry->capacity=0;
}

const SpeechBankView* spchBankGet(const SpeechBankRegistry* registry,int32_t index)
{
    if (!registry || !registry->engineMagic ||
        *registry->engineMagic!=SPEECH_ENGINE_MAGIC || !registry->banks ||
        index<0 || (size_t)index>=registry->capacity)
        return NULL;
    const SpeechBankView* bank=&registry->banks[index];
    return bank->bytes ? bank : NULL;
}

int32_t spchBankGetFree(const SpeechBankRegistry* registry)
{
    if (!registry || !registry->banks) return -1;
    for (size_t index=0;index<registry->capacity;++index)
        if (!registry->banks[index].bytes) return (int32_t)index;
    return -1;
}

int32_t spchBankFind(const SpeechBankRegistry* registry,uint16_t id)
{
    if (!registry || !registry->banks) return -1;
    for (size_t index=0;index<registry->capacity;++index) {
        const SpeechBankView* bank=&registry->banks[index];
        if (bank->bytes && bank->size>=2 && bankWord(bank->bytes)==id)
            return (int32_t)index;
    }
    return -1;
}

int32_t spchBankFindSubBank(const SpeechBankRegistry* registry,uint16_t id,uint16_t subId)
{
    if (!registry || !registry->banks) return -1;
    for (size_t index=0;index<registry->capacity;++index) {
        const SpeechBankView* bank=&registry->banks[index];
        if (bank->bytes && bank->size>=4 && bankWord(bank->bytes)==id &&
            bankWord(bank->bytes+2)==subId)
            return (int32_t)index;
    }
    return -1;
}

int32_t spchBankTestSubBankBounds(const SpeechBankRegistry* registry,int32_t index,uint32_t subBank)
{
    if (!registry || !registry->banks || index<0 || (size_t)index>=registry->capacity)
        return 0;
    const SpeechBankView* bank=&registry->banks[index];
    if (!bank->bytes || bank->size<12) return 0;
    const uint16_t limit=bankWord(bank->bytes+10);
    return limit!=UINT16_MAX && subBank<limit;
}

SpeechDatabaseResult spchBankFree(SpeechBankRegistry* registry,int32_t index)
{
    if (!registry || !registry->banks || index<0 || (size_t)index>=registry->capacity)
        return SPEECH_DATABASE_INVALID_BINDING;
    registry->banks[index].bytes=NULL;
    registry->banks[index].size=0;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchBankAddToQueue(SpeechBankView* bank,uint8_t sample)
{
    size_t cycle,queue;
    const SpeechDatabaseResult result=bankTail(bank,&cycle,&queue);
    if (result!=SPEECH_DATABASE_OK) return result;
    const uint8_t count=bank->bytes[6];
    if (!count) return SPEECH_DATABASE_OK;
    uint8_t cursor=bank->bytes[queue];
    if (cursor>=count) return SPEECH_DATABASE_INVALID_DATA;
    bank->bytes[queue+1+cursor]=sample;
    cursor=(uint8_t)(cursor+1);
    if (cursor>=count) cursor=0;
    bank->bytes[queue]=cursor;
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchBankGetSampleTimeInQueue(const SpeechBankView* bank,uint8_t sample,int32_t* time)
{
    if (!time) return SPEECH_DATABASE_INVALID_BINDING;
    size_t cycle,queue;
    const SpeechDatabaseResult result=bankTail(bank,&cycle,&queue);
    if (result!=SPEECH_DATABASE_OK) return result;
    *time=0;
    const int32_t count=bank->bytes[6];
    if (!count) return SPEECH_DATABASE_OK;
    int32_t cursor=bank->bytes[queue];
    for (int32_t elapsed=0;elapsed<count;++elapsed) {
        --cursor;
        if (cursor<0) cursor=count-1;
        if (cursor>=count) return SPEECH_DATABASE_INVALID_DATA;
        if (bank->bytes[queue+1+(size_t)cursor]==sample) {
            *time=elapsed+1;
            break;
        }
    }
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchBankSetCycleBits(SpeechBankView* bank,uint32_t cyclePhase)
{
    size_t cycle,queue;
    const SpeechDatabaseResult result=bankTail(bank,&cycle,&queue);
    if (result!=SPEECH_DATABASE_OK) return result;
    if (!(bank->bytes[4]&0x80u)) return SPEECH_DATABASE_INVALID_BINDING;
    const uint32_t period=bank->bytes[cycle];
    if (!period) return SPEECH_DATABASE_INVALID_DATA;
    const uint32_t samples=bank->bytes[5];
    const uint32_t scaled=(cyclePhase%period)*samples;
    const uint32_t first=scaled/period;
    const uint32_t last=(scaled+samples)/period;
    for (uint32_t index=first;index<last;++index)
        bank->bytes[cycle+1+(index>>3)]|=(uint8_t)(1u<<(index&7u));
    return SPEECH_DATABASE_OK;
}

SpeechDatabaseResult spchAddBank(SpeechBankRegistry* registry,SpeechBankView bank,int32_t* index)
{
    if (!registry || !registry->banks || !registry->cyclePhase || !index)
        return SPEECH_DATABASE_INVALID_BINDING;
    *index=-1;
    size_t cycle,queue;
    const SpeechDatabaseResult result=bankTail(&bank,&cycle,&queue);
    if (result!=SPEECH_DATABASE_OK) return result;
    const uint8_t count=bank.bytes[6];
    if (count) {
        if (bank.bytes[queue]!=0) return SPEECH_DATABASE_INVALID_DATA;
        for (size_t item=0;item<count;++item)
            if (bank.bytes[queue+1+item]!=UINT8_MAX)
                return SPEECH_DATABASE_INVALID_DATA;
    }
    const int32_t available=spchBankGetFree(registry);
    if (available<0) return SPEECH_DATABASE_CAPACITY;
    if (bank.bytes[4]&0x80u) {
        const SpeechDatabaseResult cycleResult=spchBankSetCycleBits(&bank,*registry->cyclePhase);
        if (cycleResult!=SPEECH_DATABASE_OK) return cycleResult;
    }
    registry->banks[available]=bank;
    *index=available;
    return SPEECH_DATABASE_OK;
}

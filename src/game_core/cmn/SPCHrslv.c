#include "indep/src/speech/SEDWedge.hpp"

/* PAL290720 binds the actual release-format database to the first empty
 * slot. Header bytes8/9 are the same database identity consumed by
 * GetEventDatInfo292678; they are not a hash of a display filename. */
SpeechDatabaseResult spchBindData(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size,uint32_t channel)
{
    if (!registry || !bytes) return SPEECH_DATABASE_INVALID_BINDING;
    if (size<10 || bytes[0]!=3 || bytes[1]!=8)
        return SPEECH_DATABASE_INVALID_DATA;

    for (size_t index=0;index<8;++index) {
        const SpeechEventDatabaseView* existing=&registry->databases[index];
        if (existing->bytes && existing->bytes[9]==bytes[9] &&
            existing->bytes[8]==bytes[8])
            return SPEECH_DATABASE_DUPLICATE;
    }
    for (size_t index=0;index<8;++index) {
        SpeechEventDatabaseView* destination=&registry->databases[index];
        if (destination->bytes) continue;
        destination->bytes=bytes;
        destination->size=size;
        destination->channel=channel;
        return SPEECH_DATABASE_OK;
    }
    return SPEECH_DATABASE_CAPACITY;
}

SpeechDatabaseResult spchAddEventDatabase(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size,uint32_t channel)
{
    if (channel>=8) return SPEECH_DATABASE_INVALID_BINDING;
    return spchBindData(registry,bytes,size,channel);
}

SpeechDatabaseResult spchResolveData(SpeechDatabaseRegistry* registry,
    const uint8_t* bytes,size_t size)
{
    return spchAddEventDatabase(registry,bytes,size,0);
}

#include "indep/src/speech/SEDWedge.hpp"

#include <stdlib.h>

/* Checked native span access is an adaptation, not an additional ELF body.
 * Corrupt data is an explicit failure; it is never a zero-filled asset. */
SpeechBytes spchBytesSlice(SpeechBytes bytes,size_t offset,size_t minimum)
{
    SpeechBytes empty={NULL,0};
    if (!bytes.bytes || offset>bytes.size || minimum>bytes.size-offset) return empty;
    bytes.bytes+=offset;bytes.size-=offset;return bytes;
}
uint8_t spchRead8(SpeechBytes bytes,size_t offset)
{
    if (!bytes.bytes || offset>=bytes.size) abort();
    return bytes.bytes[offset];
}
uint16_t spchRead16(SpeechBytes bytes,size_t offset)
{
    SpeechBytes part=spchBytesSlice(bytes,offset,2);
    if (!part.bytes) abort();
    return (uint16_t)((uint16_t)part.bytes[0]|(uint16_t)((uint16_t)part.bytes[1]<<8));
}
uint32_t spchRead32(SpeechBytes bytes,size_t offset)
{
    SpeechBytes part=spchBytesSlice(bytes,offset,4);
    if (!part.bytes) abort();
    return (uint32_t)part.bytes[0]|((uint32_t)part.bytes[1]<<8)|
        ((uint32_t)part.bytes[2]<<16)|((uint32_t)part.bytes[3]<<24);
}

/* PAL2958C8 /2958F0; independent source/path proof in spch49_paths.json. */
void spchInitializeSample(SpeechSample* sample)
{
    if (!sample) abort();
    sample->bytes=0;sample->offset=UINT32_MAX;sample->parameterCount=0;
    for (size_t lane=0;lane<4;++lane) sample->parameters[lane]=255;
}
int32_t spchUnpackSample(const SpeechBankView* bank,int32_t index,SpeechSample* sample)
{
    spchInitializeSample(sample);
    if (!bank || !bank->bytes || bank->size<12 || index<0) return 0;
    const SpeechBytes bytes={bank->bytes,bank->size};
    const uint32_t count=spchRead8(bytes,5),parameters=spchRead8(bytes,4)&15u;
    if ((uint32_t)index>=count || parameters>4) return 0;
    const uint32_t stride=parameters+2;
    SpeechBytes row=spchBytesSlice(bytes,12u+(uint32_t)index*stride,stride);
    if (!row.bytes) return 0;
    sample->parameterCount=parameters;
    for (uint32_t lane=0;lane<parameters;++lane) sample->parameters[lane]=spchRead8(row,2u+lane);
    /* An alias uses its own match attributes and the target's offset/length. */
    if (spchRead8(row,0)&128u) {
        index=(int32_t)spchRead8(row,1);
        if ((uint32_t)index>=count) return 0;
        row=spchBytesSlice(bytes,12u+(uint32_t)index*stride,stride);
        if (!row.bytes || (spchRead8(row,0)&128u)) return 0;
    }
    const uint32_t unit=((uint32_t)spchRead8(bytes,7)+1u)<<8;
    sample->offset=(((uint32_t)spchRead8(row,0)<<8)|spchRead8(row,1))*unit;
    uint32_t end=(uint32_t)spchRead16(bytes,8)*unit;
    for (uint32_t next=(uint32_t)index+1;next<count;++next) {
        const SpeechBytes following=spchBytesSlice(bytes,12u+next*stride,stride);
        if (!following.bytes) return 0;
        if (!(spchRead8(following,0)&128u)) {
            end=(((uint32_t)spchRead8(following,0)<<8)|spchRead8(following,1))*unit;
            break;
        }
    }
    sample->bytes=end-sample->offset;return 1;
}

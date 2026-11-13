#include <exception>
#include "bsound/include/bSound.h"
#include "bsound/include/NativeEnvelope.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <ostream>

namespace hp2 {
namespace {
uint32_t word(const uint8_t* bytes,size_t size,size_t offset)
{
    if (!bytes || offset>size || size-offset<4)
        throw std::out_of_range("Truncated sound device metadata");
    return uint32_t{bytes[offset]} | uint32_t{bytes[offset+1]}<<8 |
        uint32_t{bytes[offset+2]}<<16 | uint32_t{bytes[offset+3]}<<24;
}
int32_t signedWord(uint32_t value) noexcept
{
    int32_t result;
    std::memcpy(&result,&value,sizeof(result));
    return result;
}
}

NativeEnvelopeParameters CaptureVoiceEnvelope(const WorldAudioRequest& operation)
{
    if (!operation.instance)throw std::invalid_argument("Envelope has no retained sample owner");
    // Audible stream/movie/SED-chain paths use this explicit PAL constant.
    // Silent transport preparation in STREAM is not another audible attack.
    if (operation.instance->sampleType)return {};
    if (!operation.cue || !operation.cue->metadata || operation.cue->metadataSize!=52)
        throw std::invalid_argument("Resident envelope has no exact MSH cue");
    uint32_t authored=word(operation.cue->metadata,operation.cue->metadataSize,0x2C);
    if (!authored)authored=0x1A00000Au;
    return {static_cast<uint8_t>(GetAttack(authored)),static_cast<uint8_t>(GetRelease(authored))};
}

// PAL26D9D8. This SDK header is distinct from bSound's52-byte cue table.
uint32_t SOUND_GetMSHSampleCount(const uint8_t* bytes,size_t size)
{
    return word(bytes,size,8);
}

// PAL26DB30. The source writes these three header outputs even when index
// is invalid. The diagnostic-only SDK printf calls do not change its result.
int32_t SOUND_GetMSHInfo(const uint8_t* bytes,size_t size,uint32_t index,
    std::array<uint32_t,7>& fields)
{
    fields[0]=word(bytes,size,0);
    fields[1]=word(bytes,size,4);
    fields[2]=word(bytes,size,8);
    if (index>=fields[2]) return -1;
    const uint64_t offset=12u+uint64_t{index}*16u;
    if (offset>size || size-static_cast<size_t>(offset)<16)
        throw std::out_of_range("MSH sample table exceeds its retained data");
    const size_t at=static_cast<size_t>(offset);
    fields[4]=word(bytes,size,at);
    fields[3]=word(bytes,size,at+4);
    fields[5]=word(bytes,size,at+8);
    fields[6]=word(bytes,size,at+12);
    return 0;
}

// PAL26DFD8. Keep the released output order and invalid-index partial write.
int32_t SOUND_GetMIHInfo(const uint8_t* bytes,size_t size,uint32_t index,
    std::array<uint32_t,7>& fields)
{
    fields[0]=word(bytes,size,0);
    fields[1]=word(bytes,size,4);
    fields[2]=word(bytes,size,8);
    fields[6]=word(bytes,size,12);
    fields[4]=word(bytes,size,16);
    fields[5]=word(bytes,size,20);
    if (index>=fields[2]) return -1;
    const uint64_t offset=24u+uint64_t{index}*4u;
    if (offset>size || size-static_cast<size_t>(offset)<4)
        throw std::out_of_range("MIH stream table exceeds its retained data");
    fields[3]=word(bytes,size,static_cast<size_t>(offset));
    return 0;
}

// PAL26FC28. This is only the original fixed-header accessor: it does not
// prove that movie01wb bytes are PCM and does not validate a RIFF container.
void SOUND_GetWAVHeaderInfo(const uint8_t* bytes,size_t size,bSoundWaveHeader& header)
{
    header.dataBytes=word(bytes,size,40);
    header.dataOffset=44;
    const uint16_t channels=static_cast<uint16_t>(word(bytes,size,20)>>16);
    header.channelCode=channels==1 ? 0u:channels==2 ? 1u:0x1234u;
}

// PAL26FB80. EE multiply keeps its low32 bits. Express wrap explicitly;
// signed C++ overflow and implementation-defined right shifts are avoided.
int32_t SOUND_ScaleVol(int32_t master,int32_t volume) noexcept
{
    const bool inverted=volume>=16384;
    if (inverted) volume=signedWord(32767u-static_cast<uint32_t>(volume));
    const auto product=signedWord(static_cast<uint32_t>(master)*static_cast<uint32_t>(volume));
    const int32_t scaled=product/4096;
    return inverted ? signedWord(32767u-static_cast<uint32_t>(scaled)):scaled;
}

// PAL26F9E0. These are borrowed device gain arrays, never another game
// mixer. The caller supplies the real applied group/channel state.
std::array<int32_t,2> SOUND_MixVol(int32_t group,uint32_t channel,
    const std::array<int16_t,4>& masterLeft,const std::array<int16_t,4>& masterRight,
    const std::array<int16_t,48>& left,const std::array<int16_t,48>& right)
{
    const int32_t high=group>=0 ? group/65536:-1-static_cast<int32_t>((~static_cast<uint32_t>(group))/65536u);
    const int32_t index=high-1;
    if (index<0 || index>=4 || channel>=left.size())
        throw std::out_of_range("Unbound sound mixer group or physical channel");
    return {{SOUND_ScaleVol(masterLeft[static_cast<size_t>(index)],left[channel]),
        SOUND_ScaleVol(masterRight[static_cast<size_t>(index)],right[channel])}};
}

PcmWave DecodePcmWave(const uint8_t* bytes,size_t size)
{
    if (size<12 || word(bytes,size,0)!=0x46464952u || word(bytes,size,8)!=0x45564157u)
        throw std::invalid_argument("Sound payload is not an explicit RIFF/WAVE PCM resource");
    const uint64_t end=uint64_t{word(bytes,size,4)}+8u;
    if (end>size || end<12) throw std::out_of_range("RIFF sound extent is invalid");
    size_t format=0,formatBytes=0,data=0,dataBytes=0;
    for (size_t at=12;at<end;) {
        if (end-at<8) throw std::out_of_range("Truncated sound chunk header");
        const uint32_t kind=word(bytes,size,at),count=word(bytes,size,at+4);
        const uint64_t next=uint64_t{at}+8u+count+(count&1u);
        if (next>end) throw std::out_of_range("Sound chunk exceeds RIFF extent");
        if (kind==0x20746D66u) {
            if (format) throw std::invalid_argument("Repeated sound format chunk");
            format=at+8;formatBytes=count;
        } else if (kind==0x61746164u) {
            if (data) throw std::invalid_argument("Repeated sound PCM data chunk");
            data=at+8;dataBytes=count;
        }
        at=static_cast<size_t>(next);
    }
    if (!format || formatBytes<16 || !data || !dataBytes)
        throw std::invalid_argument("Sound has no complete PCM format/data pair");
    const uint32_t formatChannels=word(bytes,size,format);
    const uint16_t tag=static_cast<uint16_t>(formatChannels);
    const uint16_t channels=static_cast<uint16_t>(formatChannels>>16);
    const uint32_t rate=word(bytes,size,format+4);
    const uint32_t alignBits=word(bytes,size,format+12);
    const uint16_t align=static_cast<uint16_t>(alignBits),bits=static_cast<uint16_t>(alignBits>>16);
    if (tag!=1 || (channels!=1 && channels!=2) || (bits!=8 && bits!=16) ||
        rate<1000 || rate>200000 || align!=channels*(bits/8u) ||
        word(bytes,size,format+8)!=rate*align || dataBytes%align)
        throw std::invalid_argument("Unsupported or inconsistent PCM sound format");
    PcmWave result;result.channels=channels;result.sampleRate=rate;
    result.samples.resize(dataBytes/(bits/8u));
    for (size_t sample=0;sample<result.samples.size();++sample) {
        if (bits==8) result.samples[sample]=static_cast<int16_t>((int32_t{bytes[data+sample]}-128)*256);
        else {
            const size_t at=data+sample*2;
            const uint16_t value=uint16_t{bytes[at]} | uint16_t{bytes[at+1]}<<8;
            std::memcpy(&result.samples[sample],&value,sizeof(value));
        }
    }
    return result;
}

PcmWave DecodePs2Adpcm(const uint8_t* bytes,size_t size,uint32_t sampleRate,
    uint16_t channels,size_t interleaveBytes,bool stopAtEnd,Ps2AdpcmHistory* retainedHistory)
{
    if (!bytes || !size || size%16 || (channels!=1 && channels!=2) ||
        !interleaveBytes || interleaveBytes%16 || size%(16u*channels) ||
        sampleRate<1000 || sampleRate>200000 || (stopAtEnd && channels!=1))
        throw std::invalid_argument("Invalid PS2 ADPCM extent/rate/channel layout");
    if (size/16>std::numeric_limits<size_t>::max()/28)
        throw std::out_of_range("PS2 ADPCM decoded extent overflows native memory");
    // SPU blocks hold predictor/shift, loop flags, then28 signed four-bit
    // residuals (low nibble first). This native PCM conversion is neither
    // the console DSP/SDK implementation nor an accepted original ELF body.
    constexpr int32_t coefficients[5][2]={{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
    // A movie's successive FIFO packets share each channel's predictor.
    // Publish history only after a complete validated packet; the same core
    // remains the single implementation for whole bank/VAG resources.
    auto history=retainedHistory ? retainedHistory->channels:Ps2AdpcmHistory{}.channels;
    PcmWave result;result.sampleRate=sampleRate;result.channels=channels;
    result.samples.reserve(size/16*28);
    size_t cursor=0;bool ended=false;
    while (cursor<size && !ended) {
        const size_t channelBytes=std::min(interleaveBytes,(size-cursor)/channels);
        if (!channelBytes || channelBytes%16)
            throw std::out_of_range("PS2 ADPCM final channel block is truncated");
        const size_t outputStart=result.samples.size();
        const size_t frames=channelBytes/16*28;
        result.samples.resize(outputStart+frames*channels);
        for (uint16_t channel=0;channel<channels;++channel) {
            auto& previous=history[channel];
            for (size_t block=0;block<channelBytes;block+=16) {
                const auto* data=bytes+cursor+channel*channelBytes+block;
                const uint8_t predictor=data[0]>>4,shift=data[0]&15u,flags=data[1];
                if (predictor>=5 || shift>12 || flags>7)
                    throw std::invalid_argument("Invalid PS2 ADPCM predictor/shift/flags at byte "+
                        std::to_string(cursor+channel*channelBytes+block));
                const size_t firstFrame=(outputStart/channels)+block/16*28;
                if (channels==1 && (flags&4u) && !(flags&1u))
                    result.loopStartFrame=static_cast<uint32_t>(firstFrame);
                for (size_t index=0;index<28;++index) {
                    const uint8_t encoded=data[2+index/2];
                    const int32_t nibble=static_cast<int32_t>((encoded>>(index%2*4))&15u);
                    const int32_t residual=(nibble<8 ? nibble:nibble-16)*(4096/(1<<shift));
                    // Explicit division preserves truncation toward zero;
                    // int64 arithmetic avoids signed-shift/overflow UB.
                    const int64_t prediction=(int64_t{previous[0]}*coefficients[predictor][0]+
                        int64_t{previous[1]}*coefficients[predictor][1])/64;
                    const int64_t sample=flags==7 ? 0:int64_t{residual}+prediction;
                    if (sample<std::numeric_limits<int32_t>::min() || sample>std::numeric_limits<int32_t>::max())
                        throw std::out_of_range("PS2 ADPCM predictor history overflow");
                    result.samples[outputStart+(block/16*28+index)*channels+channel]=
                        static_cast<int16_t>(std::clamp<int64_t>(sample,-32768,32767));
                    previous[1]=previous[0];previous[0]=static_cast<int32_t>(sample);
                }
                if (stopAtEnd && (flags&1u)) {
                    const size_t end=firstFrame+28;
                    if (flags&2u) result.loopEndFrame=static_cast<uint32_t>(end);
                    result.samples.resize(end);ended=true;break;
                }
            }
        }
        cursor+=channelBytes*channels;
    }
    if (stopAtEnd && !ended)
        throw std::invalid_argument("MSB cue has no ADPCM end marker before its next resource extent");
    if (retainedHistory)retainedHistory->channels=history;
    return result;
}

bool ReadMoviePcmChunk(const WorldAudioStreamSource& source,Ps2AdpcmHistory& history,PcmWave& pcm)
{
    if (!source.movie)return false;
    if (source.sampleRate!=0xAA66u)
        throw std::invalid_argument("Movie stream lost its final PAL sample-rate descriptor");
    // PAL261B10..261B24 sends TrackSize=(Initialize buffer0x10000)/2
    // to SOUND_SetStreamParent_Int(Track0); PAL261B80 selects child Track1.
    // The final _MOVIE route reads the callback directly, with no VAG header.
    // AVI01wb chunks may split either track; the original FIFO joins them.
    constexpr size_t trackBytes=0x8000;
    std::array<uint8_t,trackBytes*2> encoded;
    if (!worldAudioReadMovieStream(source,encoded.data(),static_cast<uint32_t>(encoded.size())))
        return false; // RingBuffer zeroes short reads without consuming FIFO.
    pcm=DecodePs2Adpcm(encoded.data(),encoded.size(),source.sampleRate,2,trackBytes,false,&history);
    return true;
}

PcmWave DecodeSoundStream(const uint8_t* bytes,size_t size,uint32_t sampleRate,uint16_t channels)
{
    if ((channels!=1 && channels!=2) || sampleRate<1000 || sampleRate>200000)
        throw std::invalid_argument("Stream has no valid retained channel/rate descriptor");
    if (size>=12 && word(bytes,size,0)==0x46464952u) return DecodePcmWave(bytes,size);
    if (size>=4 && word(bytes,size,0)==0x70474156u) {
        // Native VAGp container boundary. Speech uses version2 mono clips;
        // the final JBC uses version2 VAGx stereo tracks. Both declare their
        // big-endian audio extent/rate; aligned padding is never audio.
        constexpr size_t headerBytes=0x30;
        if (size<headerBytes) throw std::out_of_range("Truncated VAG header");
        const auto bigWord=[bytes](size_t at) {
            return uint32_t{bytes[at]}<<24 | uint32_t{bytes[at+1]}<<16 |
                uint32_t{bytes[at+2]}<<8 | uint32_t{bytes[at+3]};
        };
        if (bigWord(4)!=2u)
            throw std::invalid_argument("Unsupported VAG version descriptor");
        const uint32_t dataBytes=bigWord(12),rate=bigWord(16);
        if (rate!=sampleRate)
            throw std::invalid_argument("VAG rate differs from its retained stream descriptor");
        if (!dataBytes || dataBytes%(16u*channels) || dataBytes>size-headerBytes)
            throw std::out_of_range("VAG payload exceeds its bounded stream segment");
        if (channels==1)
            return DecodePs2Adpcm(bytes+headerBytes,dataBytes,rate,1,16,false);
        if (bigWord(0x24)!=0x56414778u || bigWord(0x2c)!=channels || size%16)
            throw std::invalid_argument("Stereo VAG has no final VAGx channel descriptor");

        // The two channel end markers identify the actual VAGx interleave.
        // All final JBC tracks have exactly one flag1 block per channel,
        // followed by the declared flag7 guard. The guard is silent but is
        // after the SPU end marker; physical alignment padding follows it.
        // No fixed/generic MIH interleave is substituted for these bytes.
        std::array<size_t,2> terminal{};
        size_t terminalCount=0;
        for (size_t at=headerBytes;at<size;at+=16) {
            if (bytes[at+1]!=1u) continue;
            if (terminalCount==terminal.size())
                throw std::invalid_argument("VAGx has more than one end marker per channel");
            terminal[terminalCount++]=at;
        }
        if (terminalCount!=channels)
            throw std::invalid_argument("VAGx has no paired channel end markers");
        const size_t interleave=terminal[1]-terminal[0];
        const size_t channelBytes=dataBytes/channels;
        if (!interleave || interleave%16 || channelBytes<32 ||
            interleave>std::numeric_limits<size_t>::max()/channels)
            throw std::out_of_range("Invalid VAGx channel interleave");
        const size_t groupBytes=interleave*channels;
        const uint64_t groups=(uint64_t{channelBytes}+interleave-1)/interleave;
        if (groups*groupBytes!=size-headerBytes)
            throw std::out_of_range("VAGx padded channel span does not match its track extent");
        const auto physical=[=](size_t logical,size_t channel) {
            return headerBytes+(logical/interleave)*groupBytes+channel*interleave+logical%interleave;
        };
        for (size_t channel=0;channel<channels;++channel) {
            if (physical(channelBytes-32,channel)!=terminal[channel] ||
                bytes[physical(channelBytes-16,channel)+1]!=7u)
                throw std::invalid_argument("VAGx end/guard markers disagree with its declared channel size");
        }

        // Preserve the full physical channel stride, including in the last
        // group. The last logical group is packed only after each channel's
        // original bytes have been selected; truncating the input to the
        // declared combined byte count would splice the two channels.
        std::vector<uint8_t> packed;
        packed.reserve(dataBytes);
        for (size_t logical=0;logical<channelBytes;) {
            const size_t take=std::min(interleave,channelBytes-logical);
            for (size_t channel=0;channel<channels;++channel) {
                const size_t at=physical(logical,channel);
                packed.insert(packed.end(),bytes+at,bytes+at+take);
            }
            logical+=take;
        }
        auto pcm=DecodePs2Adpcm(packed.data(),packed.size(),rate,channels,interleave,false);
        // Include the terminal flag1 block, just as mono stopAtEnd does.
        // The following flag7 guard contributes exactly28 zero frames per
        // channel and must not delay the native stream-completion event.
        const size_t terminalFrames=(channelBytes/16-1)*28;
        pcm.samples.resize(terminalFrames*channels);
        return pcm;
    }
    if (channels==1)
        return DecodePs2Adpcm(bytes,size,sampleRate,1,16,false);
    throw std::invalid_argument("Raw stereo PS2 stream has no proven retained interleave descriptor");
}

// Own the immutable file range and format when the Windows audio transport
// starts an asynchronous decode. The worker never borrows a live sample,
// jukebox player, request ring or XAudio2 device. FileRange keeps its physical
// file alive across a STOP, reset or cartridge change.
NativeStreamDecodeRange CaptureStreamDecodeRange(const WorldAudioRequest& operation)
{
    if (operation.kind!=WORLD_AUDIO_OPEN_STREAM && operation.kind!=WORLD_AUDIO_APPEND_CHAIN &&
        operation.kind!=WORLD_AUDIO_OPEN_CHAIN)
        throw std::invalid_argument("Sound range decode requires a retained stream command");
    const auto* source=operation.streamSource;
    const auto* sample=operation.instance;
    if (!source || source->movie || !source->file.Valid() || !sample ||
        sample->streamSource!=source || (sample->sampleType!=1 && sample->sampleType!=3) ||
        operation.streamOffset>source->file.Size() ||
        operation.streamSize>source->file.Size()-operation.streamOffset ||
        operation.streamSize>std::numeric_limits<size_t>::max())
        throw std::invalid_argument("Stream range exceeds its retained final file/owner");
    const uint16_t channels=sample->sampleType==3 ? 2:1;
    const bool jukebox=sample->completionKind==WORLD_AUDIO_COMPLETION_JUKEBOX;
    if (jukebox && (operation.kind!=WORLD_AUDIO_OPEN_STREAM || channels!=2 ||
        !sample->completionJukebox || operation.streamOffset<0x30u ||
        operation.streamSize<0x30u || source->file.Size()<32))
        throw std::invalid_argument("Jukebox stream lacks its retained header/owner");
    return {source->file,operation.streamOffset,operation.streamSize,source->sampleRate,channels,jukebox};
}

PcmWave DecodeStreamRangeSnapshot(const NativeStreamDecodeRange& source)
{
    uint64_t offset=source.offset;
    if (source.jukebox) {
        // The final JBC offset skips the VAG header while its extent
        // includes it. Validate the actual pinned cartridge, not a copy of
        // mutable game state, and preserve the original ADPCM decoder.
        constexpr size_t headerBytes=0x30;
        const auto cartridge=source.file.Read(0,32);
        if (word(cartridge.data(),cartridge.size(),0)!=16u ||
            word(cartridge.data(),cartridge.size(),4)!=2u ||
            word(cartridge.data(),cartridge.size(),8)!=source.sampleRate ||
            word(cartridge.data(),cartridge.size(),12)>16u ||
            word(cartridge.data(),cartridge.size(),16)>source.file.Size()-32)
            throw std::invalid_argument("Jukebox range does not retain a final JBC cartridge");
        offset-=headerBytes;
        const auto header=source.file.Read(offset,headerBytes);
        if (word(header.data(),header.size(),0)!=0x70474156u ||
            word(header.data(),header.size(),0x24)!=0x78474156u ||
            word(header.data(),header.size(),0x2c)!=0x02000000u)
            throw std::invalid_argument("Jukebox payload has no preceding final VAGx header");
    }
    const auto bytes=source.file.Read(offset,static_cast<size_t>(source.bytes));
    auto pcm=DecodeSoundStream(bytes.data(),bytes.size(),source.sampleRate,source.channels);
    if (pcm.channels!=source.channels || pcm.sampleRate!=source.sampleRate)
        throw std::invalid_argument("Decoded stream format disagrees with its retained owner");
    return pcm;
}

PcmWave DecodeSoundStreamRange(const WorldAudioRequest& operation)
{
    return DecodeStreamRangeSnapshot(CaptureStreamDecodeRange(operation));
}

PcmWave DecodeSoundBankCue(const WorldAudioBank& bank,size_t at)
{
    if (!bank.metadata || !bank.encodedSamples || bank.metadataSize<12 || at<12 ||
        (at-12)%52 || at>bank.metadataSize || bank.metadataSize-at<52)
        throw std::invalid_argument("Sound cue is outside its retained52-byte MSH table");
    const size_t count=word(bank.metadata,bank.metadataSize,8);
    if (count>(bank.metadataSize-12)/52 || (at-12)/52>=count)
        throw std::out_of_range("MSH cue index exceeds the authored table");
    const size_t offset=word(bank.metadata,bank.metadataSize,at+0x28);
    if (offset>=bank.encodedSampleSize)
        throw std::out_of_range("MSH cue offset exceeds retained MSB");
    size_t end=bank.encodedSampleSize;
    for (size_t index=0;index<count;++index) {
        if (bank.metadata[12+index*52+0x21]>>4)continue;
        const size_t next=word(bank.metadata,bank.metadataSize,12+index*52+0x28);
        if (next>offset && next<end) end=next;
    }
    const auto* encoded=bank.encodedSamples+offset;
    if (end-offset>=12 && word(encoded,end-offset,0)==0x46464952u)
        return DecodePcmWave(encoded,end-offset);
    const uint32_t rate=word(bank.metadata,bank.metadataSize,at+0x20)>>16;
    return DecodePs2Adpcm(encoded,end-offset,rate,1,16,true);
}

void NativeJukeboxStream::ReadBytes(uint64_t offset,void* destination,size_t count)
{
    file_.Read(offset,destination,count);
    bytesRead_+=count;
    maximumReadBytes_=std::max(maximumReadBytes_,count);
}

uint64_t NativeJukeboxStream::Physical(size_t logical,size_t channel) const noexcept
{
    return 0x30u+uint64_t{logical/interleave_}*interleave_*2u+
        uint64_t{channel}*interleave_+logical%interleave_;
}

NativeJukeboxStream::NativeJukeboxStream(NativeStreamDecodeRange source)
{
    if (!source.jukebox || source.channels!=2 || !source.file.Valid() ||
        source.file.Size()<32 || source.offset<0x30u || source.bytes<0x30u ||
        source.offset>source.file.Size() || source.bytes>source.file.Size()-source.offset ||
        source.bytes>std::numeric_limits<size_t>::max() || source.bytes%16u ||
        source.sampleRate<1000 || source.sampleRate>200000)
        throw std::invalid_argument("Jukebox cursor has no valid retained physical range");
    const auto cartridge=source.file.Read(0,32);
    bytesRead_=32;maximumReadBytes_=32;
    if (word(cartridge.data(),cartridge.size(),0)!=16u ||
        word(cartridge.data(),cartridge.size(),4)!=2u ||
        word(cartridge.data(),cartridge.size(),8)!=source.sampleRate ||
        word(cartridge.data(),cartridge.size(),12)>16u ||
        word(cartridge.data(),cartridge.size(),16)>source.file.Size()-32)
        throw std::invalid_argument("Jukebox cursor lost the original cartridge header");
    file_=source.file.Subrange(source.offset-0x30u,source.bytes);
    std::array<uint8_t,0x30> header{};
    ReadBytes(0,header.data(),header.size());
    const auto bigWord=[&](size_t at) {
        return uint32_t{header[at]}<<24 | uint32_t{header[at+1]}<<16 |
            uint32_t{header[at+2]}<<8 | uint32_t{header[at+3]};
    };
    rate_=source.sampleRate;
    const size_t dataBytes=bigWord(12);
    const size_t physicalBytes=static_cast<size_t>(file_.Size()-header.size());
    if (word(header.data(),header.size(),0)!=0x70474156u || bigWord(4)!=2u ||
        bigWord(16)!=rate_ || bigWord(0x24)!=0x56414778u || bigWord(0x2c)!=2u ||
        !dataBytes || dataBytes%32u || dataBytes>physicalBytes || dataBytes/2u<32u)
        throw std::invalid_argument("Jukebox cursor has an invalid VAGx descriptor");

    // Locate the last paired channel end markers with bounded physical
    // reads. The actual cartridge needs one 64 KiB tail read; a different
    // legal stride may need more reads but never a whole-track allocation.
    // Earlier unexpected flag1 blocks are rejected as their chunks arrive.
    std::array<size_t,2> terminal{};
    size_t terminalCount=0,cursor=physicalBytes;
    std::array<uint8_t,0x10000> tail{};
    while (cursor && terminalCount<terminal.size()) {
        const size_t count=std::min(cursor,tail.size());
        cursor-=count;ReadBytes(header.size()+cursor,tail.data(),count);
        for (size_t at=count;at;at-=16) {
            if (tail[at-16+1]!=1u)continue;
            if (terminalCount==terminal.size())
                throw std::invalid_argument("VAGx has extra channel end markers");
            terminal[terminalCount++]=header.size()+cursor+at-16;
        }
    }
    if (terminalCount!=terminal.size())
        throw std::invalid_argument("VAGx has no paired channel end markers");
    std::sort(terminal.begin(),terminal.end());
    interleave_=terminal[1]-terminal[0];
    const size_t declaredChannelBytes=dataBytes/2u;
    if (!interleave_ || interleave_%16u || interleave_>physicalBytes/2u)
        throw std::out_of_range("Invalid retained VAGx channel stride");
    const uint64_t groups=(uint64_t{declaredChannelBytes}+interleave_-1u)/interleave_;
    const uint64_t groupBytes=uint64_t{interleave_}*2u;
    if (groups>physicalBytes/groupBytes || groups*groupBytes!=physicalBytes)
        throw std::out_of_range("VAGx padded stride does not match the track extent");
    for (size_t channel=0;channel<2;++channel) {
        if (Physical(declaredChannelBytes-32u,channel)!=terminal[channel])
            throw std::invalid_argument("VAGx channel end disagrees with its declared size");
        std::array<uint8_t,16> guard{};
        ReadBytes(Physical(declaredChannelBytes-16u,channel),guard.data(),guard.size());
        if ((guard[0]>>4)>=5 || (guard[0]&15u)>12 || guard[1]!=7u)
            throw std::invalid_argument("VAGx final guard is not the authored silent block");
    }
    // Match the existing native conversion: play flag1, omit the following
    // flag7 guard and all physical alignment padding from completion time.
    channelBytes_=declaredChannelBytes-16u;
    totalFrames_=uint64_t{channelBytes_/16u}*28u;
}

NativeJukeboxStream::Chunk NativeJukeboxStream::Read()
{
    if (Finished())throw std::out_of_range("Jukebox cursor read past its declared audio end");
    const size_t count=std::min(FramesPerChunk/28u*16u,channelBytes_-logicalBytes_);
    std::vector<uint8_t> encoded(count*2u);
    for (size_t channel=0;channel<2;++channel) {
        for (size_t copied=0;copied<count;) {
            const size_t logical=logicalBytes_+copied;
            const size_t take=std::min(count-copied,interleave_-logical%interleave_);
            ReadBytes(Physical(logical,channel),encoded.data()+channel*count+copied,take);
            copied+=take;
        }
        for (size_t at=0;at<count;at+=16u) {
            const auto flags=encoded[channel*count+at+1];
            const bool terminal=logicalBytes_+at+16u==channelBytes_;
            if ((flags==1u)!=terminal)
                throw std::invalid_argument("VAGx contains an unexpected channel end marker");
        }
    }
    // Selecting each original channel span before decoding preserves the
    // physical stride of the final padded group and the predictor across
    // every native chunk boundary. There is one ADPCM arithmetic core.
    auto pcm=DecodePs2Adpcm(encoded.data(),encoded.size(),rate_,2,count,false,&history_);
    logicalBytes_+=count;decodedFrames_+=pcm.samples.size()/2u;
    return {std::move(pcm),Finished()};
}

NativeJukeboxStart PrepareJukeboxStream(const NativeStreamDecodeRange& source)
{
    NativeJukeboxStart result;
    result.stream=std::make_unique<NativeJukeboxStream>(source);
    result.first=result.stream->Read();
    return result;
}

NativeSoundBankCache::NativeSoundBankCache(const WorldAudioBank& bank):bank_(&bank)
{
    if (!bank.metadata || !bank.encodedSamples || bank.metadataSize<12)
        throw std::invalid_argument("Native bank cache has no retained MSH/MSB owner");
    const size_t count=word(bank.metadata,bank.metadataSize,8);
    if (count>(bank.metadataSize-12u)/52u)
        throw std::out_of_range("Native bank cache exceeds the authored MSH table");
    entries_.resize(count);
    std::vector<size_t> starts;
    starts.reserve(count);
    for (size_t index=0;index<count;++index) {
        const size_t at=12u+index*52u;
        auto& entry=entries_[index];
        if (GetChannelNum(bank.metadata[at+0x21]))continue;
        entry.resident=true;
        entry.offset=word(bank.metadata,bank.metadataSize,at+0x28);
        entry.rate=word(bank.metadata,bank.metadataSize,at+0x20)>>16;
        if (entry.offset>=bank.encodedSampleSize)
            throw std::out_of_range("MSH cue offset exceeds the retained MSB");
        starts.push_back(entry.offset);
    }
    std::sort(starts.begin(),starts.end());
    starts.erase(std::unique(starts.begin(),starts.end()),starts.end());
    // The original cue offset is immutable. Compute its next resident
    // extent once, instead of rescanning all N metadata records for each of
    // M master samples during every physical bank load.
    for (auto& entry:entries_)if (entry.resident) {
        const auto next=std::upper_bound(starts.begin(),starts.end(),entry.offset);
        const size_t end=next==starts.end() ? bank.encodedSampleSize:*next;
        entry.bytes=end-entry.offset;
    }
}

std::shared_ptr<const PcmWave> NativeSoundBankCache::Acquire(size_t metadataOffset)
{
    if (metadataOffset<12u || (metadataOffset-12u)%52u ||
        (metadataOffset-12u)/52u>=entries_.size())
        throw std::out_of_range("PCM request is outside the retained MSH table");
    auto& entry=entries_[(metadataOffset-12u)/52u];
    if (!entry.resident)throw std::invalid_argument("Stream descriptor is not a resident PCM cue");
    if (auto existing=entry.pcm.lock())return existing;
    const auto* encoded=bank_->encodedSamples+entry.offset;
    PcmWave pcm;
    try {
        pcm=entry.bytes>=12u && word(encoded,entry.bytes,0)==0x46464952u ?
            DecodePcmWave(encoded,entry.bytes):
            DecodePs2Adpcm(encoded,entry.bytes,entry.rate,1,16,true);
        if (pcm.channels!=1)
            throw std::invalid_argument("PCM channels disagree with the authored mono MSH cue");
    } catch (const std::exception& error) {
        const char* name=reinterpret_cast<const char*>(bank_->metadata+metadataOffset);
        const char* end=std::find(name,name+32,'\0');
        throw std::runtime_error(std::string("MSH cue '")+std::string(name,end)+"': "+error.what());
    }
    auto result=std::make_shared<const PcmWave>(std::move(pcm));
    entry.pcm=result;++decodeCount_;
    return result;
}

size_t NativeSoundBankCache::ResidentBytes() const
{
    size_t result=0;
    for (const auto& entry:entries_)if (const auto pcm=entry.pcm.lock())
        result+=pcm->samples.size()*sizeof(int16_t);
    return result;
}

} // namespace hp2

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <xapo.h>
#include "bsound/include/NativeAudioCom.hpp"
#include "bsound/include/NativeEnvelopeXapo.hpp"
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <thread>
#include <future>
#include <chrono>
#include "indep/src/speech/SEDWedge.hpp"

#ifdef __MINGW32__
extern "C" HRESULT WINAPI CreateAudioReverb(IUnknown** result);
#endif

namespace {
#ifdef __MINGW32__
// The selected MinGW import is xaudio2_8. Its public header currently omits
// this SDK declaration: reproduce only the documented2.8 parameter ABI.
// Microsoft documents the packed fields;2.9 alone adds SideDelay. MSVC uses
// its SDK's own version below. This is an OS API structure, not a PS2 layout.
#pragma pack(push,1)
struct NativeReverbParameters {
    float WetDryMix;
    UINT32 ReflectionsDelay;
    BYTE ReverbDelay,RearDelay,PositionLeft,PositionRight;
    BYTE PositionMatrixLeft,PositionMatrixRight,EarlyDiffusion,LateDiffusion;
    BYTE LowEQGain,LowEQCutoff,HighEQGain,HighEQCutoff;
    float RoomFilterFreq,RoomFilterMain,RoomFilterHF,ReflectionsGain;
    float ReverbGain,DecayTime,Density,RoomSize;
    BOOL DisableLateField;
};
#pragma pack(pop)
static_assert(sizeof(NativeReverbParameters)==56 &&
    offsetof(NativeReverbParameters,RoomFilterFreq)==20 &&
    offsetof(NativeReverbParameters,DisableLateField)==52,
    "The linked XAudio2.8 reverb parameter ABI must be exact");
#else
using NativeReverbParameters=XAUDIO2FX_REVERB_PARAMETERS;
#endif

NativeReverbParameters defaultNativeReverb() noexcept
{
    // Explicit Microsoft XAUDIO2FX_REVERB_DEFAULT values. This is the
    // documented Windows approximation for the sole game-emitted mode6;
    // it does not claim a sample-exact SPU2 reverb algorithm or preset.
    NativeReverbParameters parameters{};
    parameters.WetDryMix=100.0f;parameters.ReflectionsDelay=5;
    parameters.ReverbDelay=5;parameters.RearDelay=5;
#if !defined(__MINGW32__) && defined(XAUDIO2FX_REVERB_DEFAULT_7POINT1_SIDE_DELAY)
    parameters.SideDelay=XAUDIO2FX_REVERB_DEFAULT_7POINT1_SIDE_DELAY;
#elif !defined(__MINGW32__) && defined(XAUDIO2FX_REVERB_DEFAULT_SIDE_DELAY)
    parameters.SideDelay=XAUDIO2FX_REVERB_DEFAULT_SIDE_DELAY;
#endif
    parameters.PositionLeft=6;parameters.PositionRight=6;
    parameters.PositionMatrixLeft=27;parameters.PositionMatrixRight=27;
    parameters.EarlyDiffusion=8;parameters.LateDiffusion=8;
    parameters.LowEQGain=8;parameters.LowEQCutoff=4;
    parameters.HighEQGain=8;parameters.HighEQCutoff=4;
    parameters.RoomFilterFreq=5000.0f;parameters.RoomFilterMain=0.0f;
    parameters.RoomFilterHF=0.0f;parameters.ReflectionsGain=0.0f;
    parameters.ReverbGain=0.0f;parameters.DecayTime=1.0f;
    parameters.Density=100.0f;parameters.RoomSize=100.0f;
    parameters.DisableLateField=FALSE;
    return parameters;
}
}

// Native Windows endpoint. The owner applies retained audio requests to
// XAudio2 source voices and keeps PCM buffers alive until their consumers
// finish or physical voice destruction joins their final borrow.
struct bSound::NativeDevice {
    struct Voice {
        IXAudio2SourceVoice* device{};
        hp2::NativeGainXapo* envelope{};
        WorldAudioVoiceRef sample{};
        uint64_t sequence{};
        const WorldAudioBank* bank{};
        std::vector<hp2::PcmWave> streamBuffers;
        std::shared_ptr<const hp2::PcmWave> residentPcm;
        std::unique_ptr<hp2::NativeJukeboxStream> fileStream;
        std::future<hp2::NativeJukeboxStream::Chunk> fileRead;
        bool fileEnd{};
        const WorldAudioStreamSource* movieSource{};
        hp2::Ps2AdpcmHistory movieHistory{};
        uint32_t rate{};
        hp2::NativeVoiceMix mix{};
        int effectCore{-1};
        bool started{},paused{},muted{},opened{},releaseTail{},releaseAfterTail{};
    };
    struct EffectBus {
        IXAudio2SubmixVoice* device{};
        NativeReverbParameters parameters{defaultNativeReverb()};
        uint16_t volume{};
        int8_t pan{};
        bool enabled{true};
    };
    struct Failure { WorldAudioResult result;HRESULT system;const char* reason; };
    bSound& library;
    WorldAudioRequestQueue& queue;
    uint64_t generation;
    std::thread::id thread{std::this_thread::get_id()};
    IXAudio2* engine{};
    IXAudio2MasteringVoice* master{};
    hp2::NativeOutputClock outputClock;
    std::array<EffectBus,2> effects{};
    bool com{};
    uint8_t mode{2};
    std::array<Voice,48> voices{};
    std::map<const WorldAudioBank*,hp2::NativeSoundBankCache> banks;
    hp2::NativeAudioCommandPump commands;
    std::future<hp2::NativeJukeboxStart> jukeboxDecode;
    uint64_t jukeboxSequence{};

    explicit NativeDevice(bSound& owner):library(owner),queue(*owner.Driver()),generation(queue.generation) {}
    ~NativeDevice()
    {
        if (std::this_thread::get_id()!=thread)std::terminate();
        for (auto& voice:voices)destroy(voice);
        // Sources must release their actual send destinations first.
        for (auto& effect:effects)if(effect.device)effect.device->DestroyVoice();
        if (master)master->DestroyVoice();
        if (engine)engine->Release();
        if (com)CoUninitialize();
    }
    static void checked(HRESULT result,const char* reason)
    {
        if (FAILED(result))throw Failure{WORLD_AUDIO_BACKEND_FAILURE,result,reason};
    }
    static void required(WorldAudioResult result,const char* reason)
    {
        if (result!=WORLD_AUDIO_OK)throw Failure{result,E_FAIL,reason};
    }
    void effectMatrix(EffectBus& effect)
    {
        // PAL267270..2673F4 uses linear +/-129 double arithmetic for
        // effect-return pan, independently of the sample equal-power table.
        // Keep the authored pan/gain relation, then produce a native matrix
        // directly. No signed16 device depth word is packed or reinterpreted.
        const int pan=mode<2 ? 0:std::max(-127,static_cast<int>(effect.pan));
        const int left=pan>0 ? std::max(0,static_cast<int>(double(pan)*-129.0+effect.volume)):effect.volume;
        const int right=pan<0 ? std::max(0,static_cast<int>(double(pan)*129.0+effect.volume)):effect.volume;
        const std::array<float,4> coefficients{{
            effect.enabled ? static_cast<float>(left)/16383.0f:0.0f,0.0f,
            0.0f,effect.enabled ? static_cast<float>(right)/16383.0f:0.0f}};
        checked(effect.device->SetOutputMatrix(master,2,2,coefficients.data()),
            "XAudio2 effect-return matrix failed");
    }
    void effectParameters(EffectBus& effect)
    {
        checked(effect.device->SetEffectParameters(0,&effect.parameters,sizeof(effect.parameters)),
            "XAudio2 reverb parameters failed");
    }
    void initializeEffects()
    {
        for(auto& effect:effects) {
            IUnknown* apo=nullptr;
            checked(CreateAudioReverb(&apo),"Windows reverb APO creation failed");
            XAUDIO2_EFFECT_DESCRIPTOR descriptor{apo,TRUE,2};
            XAUDIO2_EFFECT_CHAIN chain{1,&descriptor};
            const auto result=engine->CreateSubmixVoice(&effect.device,2,48000,0,0,nullptr,&chain);
            // The created voice takes its own COM reference to the effect.
            apo->Release();
            checked(result,"XAudio2 reverb submix creation failed");
            effectParameters(effect);effectMatrix(effect);
        }
    }
    EffectBus& effectBus(const WorldAudioRequest& operation)
    {
        if(operation.effectCore<0 || operation.effectCore>=static_cast<int32_t>(effects.size()) ||
            !effects[static_cast<size_t>(operation.effectCore)].device)
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Effect command has no physical core submix"};
        return effects[static_cast<size_t>(operation.effectCore)];
    }
    void effectEnabled(EffectBus& effect,bool enabled)
    {
        if(enabled) {
            // Original STREAM EnableEffects clears this core's work area
            // before enabling. EnableEffect alone keeps an old APO's delay
            // history. Replace only the wet DSP instance on the same native
            // submix so a new preset/scene cannot inherit an obsolete tail.
            effect.enabled=false;effectMatrix(effect);
            IUnknown* fresh=nullptr;
            checked(CreateAudioReverb(&fresh),"Windows fresh reverb APO creation failed");
            XAUDIO2_EFFECT_DESCRIPTOR descriptor{fresh,TRUE,2};
            XAUDIO2_EFFECT_CHAIN chain{1,&descriptor};
            const auto replaced=effect.device->SetEffectChain(&chain);
            fresh->Release();
            checked(replaced,"XAudio2 reverb history replacement failed");
            effectParameters(effect);
        }
        else {
            // DisableEffect bypasses its APO. Zero the return first so a
            // bypass can never double the independent direct/dry source.
            effect.enabled=false;effectMatrix(effect);
            checked(effect.device->DisableEffect(0),"XAudio2 reverb disable failed");
        }
        effect.enabled=enabled;effectMatrix(effect);
    }
    void effectAttribute(const WorldAudioRequest& operation)
    {
        auto& effect=effectBus(operation);
        switch(operation.effectParameter) {
        case 1:
            if(operation.effectPreset!=6)
                throw Failure{WORLD_AUDIO_BACKEND_FAILURE,E_NOTIMPL,"Only the game's actual reverb mode6 has a Windows mapping"};
            effectEnabled(effect,false);
            effect.parameters=defaultNativeReverb();effectParameters(effect);return;
        case 2:
            // PAL265518 writes the actual16-bit patch volume.
            effect.volume=static_cast<uint16_t>(operation.effectPreset);effectMatrix(effect);return;
        case 3:
            // PAL26553C subtracts127 from the low byte, then stores a byte.
            {const auto bits=static_cast<uint8_t>(static_cast<uint8_t>(operation.effectPreset)-127u);
            std::memcpy(&effect.pan,&bits,sizeof(bits));}
            effectMatrix(effect);return;
        case 4:
            if(operation.effectPreset!=0)
                throw Failure{WORLD_AUDIO_BACKEND_FAILURE,E_NOTIMPL,"Nonzero legacy reverb delay has no verified Windows mapping"};
            // The game's fixed zero extension restores the documented
            // Windows preset timing; apply it to the real APO immediately.
            effect.parameters.ReflectionsDelay=5;effect.parameters.ReverbDelay=5;
            effectParameters(effect);return;
        case 5:
            if(operation.effectPreset!=0)
                throw Failure{WORLD_AUDIO_BACKEND_FAILURE,E_NOTIMPL,"Nonzero legacy reverb feedback has no verified Windows mapping"};
            // The supported zero extension uses the documented native
            // default decay, not a silent success or a cached-only value.
            effect.parameters.DecayTime=1.0f;effect.parameters.Density=100.0f;
            effectParameters(effect);return;
        default:throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Unknown retained effect attribute"};
        }
    }
    bool usesEffect(const WorldAudioRequest& operation) const
    {
        // Actual PlayChain262E14 turns effects off. Normal VoicePlay
        //2625B8..2625F4 gates them on the authored MSH+26 halfword.
        if(operation.instance && operation.instance->sampleType==1)return false;
        if(!operation.cue || !operation.cue->metadata)return false;
        if(operation.cue->metadataSize!=52)
            throw Failure{WORLD_AUDIO_RESOURCE_FAILURE,E_INVALIDARG,"Effect route has no actual52-byte MSH record"};
        return operation.cue->metadata[0x26] || operation.cue->metadata[0x27];
    }
    void destroy(Voice& voice)
    {
        // A worker borrows only this owned immutable file cursor. Join its
        // single bounded chunk before dropping the cursor, never a whole
        // track decode. It has no sample/device or request-ring pointers.
        if (voice.fileRead.valid())voice.fileRead.wait();
        // DestroyVoice waits until the processing thread no longer borrows
        // the source data. Only then may stream PCM and bank views expire.
        if (voice.device)voice.device->DestroyVoice();
        if (voice.envelope)voice.envelope->Release();
        voice=Voice{};
    }
    Voice* find(const WorldAudioRequest& operation)
    {
        for (auto& voice:voices)
            if (voice.device && voice.sample.instance==operation.instance &&
                voice.sample.generation==operation.instanceGeneration)return &voice;
        return nullptr;
    }
    Voice& requireVoice(const WorldAudioRequest& operation)
    {
        if (auto* voice=find(operation))return *voice;
        throw Failure{WORLD_AUDIO_STALE_REFERENCE,E_FAIL,"Command has no matching physical voice"};
    }
    void create(Voice& voice,const WorldAudioRequest& operation,uint32_t rate,uint16_t channels,bool assign=true)
    {
        if (!master || rate<1000 || rate>200000 || (channels!=1 && channels!=2))
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid physical PCM voice format"};
        destroy(voice);
        // A failed recreate still owes terminal cleanup of the same sample
        // association. Keep its identity before effect/creation can throw;
        // CloseWindowsDevice retires it only after the actual device joins.
        voice.sample={operation.instance,operation.instanceGeneration};
        WAVEFORMATEX format{};
        format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=channels;
        format.nSamplesPerSec=rate;format.wBitsPerSample=16;
        format.nBlockAlign=static_cast<WORD>(channels*2u);
        format.nAvgBytesPerSec=rate*format.nBlockAlign;
        // The48 native device slots retain two independent24-slot effect
        // buses. Each source is sent once to dry output and, only when its
        // real MSH enables it, once to its own core's100%-wet return.
        voice.effectCore=usesEffect(operation) ? static_cast<int>((&voice-voices.data())/24):-1;
        std::array<XAUDIO2_SEND_DESCRIPTOR,2> descriptors{{{0,master},{0,nullptr}}};
        if(voice.effectCore>=0)descriptors[1].pOutputVoice=effects[static_cast<size_t>(voice.effectCore)].device;
        XAUDIO2_VOICE_SENDS sends{voice.effectCore>=0 ? 2u:1u,descriptors.data()};
        voice.envelope=new hp2::NativeGainXapo(outputClock,hp2::CaptureVoiceEnvelope(operation));
        XAUDIO2_EFFECT_DESCRIPTOR envelopeDescriptor{voice.envelope,TRUE,channels};
        XAUDIO2_EFFECT_CHAIN envelopeChain{1,&envelopeDescriptor};
        checked(engine->CreateSourceVoice(&voice.device,&format,0,XAUDIO2_MAX_FREQ_RATIO,nullptr,&sends,&envelopeChain),
            "XAudio2 CreateSourceVoice failed");
        voice.sequence=operation.sequence;voice.bank=operation.bank;
        voice.rate=rate;
        voice.mix.Initialize(channels,operation.instance && !operation.instance->sampleType,0x3FFF,0,mode);
        if (assign)required(library.Voices().Assign(operation,static_cast<uint16_t>(&voice-voices.data())),
            "Physical voice assignment rejected the retained operation");
    }
    Voice& allocate(const WorldAudioRequest& operation,uint32_t rate,uint16_t channels)
    {
        const size_t id=operation.reservedVoiceId;
        if(id>=voices.size() || operation.reservedVoiceChannels!=channels ||
            channels>voices.size()-id || (channels!=1 && channels!=2))
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"PLAY/PRIME has no captured source channel reservation"};
        auto& selected=voices[id];
        for(size_t other=0;other<voices.size();++other) {
            auto& held=voices[other];if(!held.device)continue;
            const bool same=other==id && held.sample.instance==operation.instance &&
                held.sample.generation==operation.instanceGeneration;
            if(same)continue;
            if(other<id+channels && id<other+held.mix.channels) {
                // Original channel reuse overwrites a keyed-off tail. The
                // detached native tail retains PCM, never a retired sample.
                if (held.releaseTail && held.releaseAfterTail) { destroy(held);continue; }
                throw Failure{WORLD_AUDIO_STALE_REFERENCE,E_FAIL,"Captured source channel is still owned by another physical voice before its ordered STOP"};
            }
        }
        create(selected,operation,rate,channels);
        return selected;
    }

    void matrix(Voice& voice)
    {
        const auto coefficients=voice.mix.Coefficients();
        checked(voice.device->SetOutputMatrix(master,voice.mix.channels,2,coefficients.data()),
            "XAudio2 output matrix failed");
        if(voice.effectCore>=0)
            checked(voice.device->SetOutputMatrix(effects[static_cast<size_t>(voice.effectCore)].device,
                voice.mix.channels,2,coefficients.data()),"XAudio2 authored wet-send matrix failed");
    }
    void pitch(Voice& voice,uint32_t frequency)
    {
        const float ratio=static_cast<float>(frequency)/static_cast<float>(voice.rate);
        if (ratio<XAUDIO2_MIN_FREQ_RATIO || ratio>XAUDIO2_MAX_FREQ_RATIO)
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Authored sound pitch exceeds device range"};
        checked(voice.device->SetFrequencyRatio(ratio),"XAudio2 frequency ratio failed");
    }
    void submit(Voice& voice,const hp2::PcmWave& pcm,bool last,bool loop)
    {
        if (pcm.sampleRate!=voice.rate || pcm.channels!=voice.mix.channels || pcm.samples.empty() ||
            pcm.samples.size()>XAUDIO2_MAX_BUFFER_BYTES/sizeof(int16_t))
            throw Failure{WORLD_AUDIO_RESOURCE_FAILURE,E_INVALIDARG,"PCM buffer does not match its retained physical voice"};
        XAUDIO2_BUFFER buffer{};
        buffer.AudioBytes=static_cast<UINT32>(pcm.samples.size()*sizeof(int16_t));
        buffer.pAudioData=reinterpret_cast<const BYTE*>(pcm.samples.data());
        buffer.Flags=last ? XAUDIO2_END_OF_STREAM:0;
        buffer.LoopCount=loop ? XAUDIO2_LOOP_INFINITE:0;
        if (loop && pcm.loopEndFrame>pcm.loopStartFrame) {
            buffer.LoopBegin=pcm.loopStartFrame;
            buffer.LoopLength=pcm.loopEndFrame-pcm.loopStartFrame;
        }
        checked(voice.device->SubmitSourceBuffer(&buffer),"XAudio2 PCM submission failed");
    }
    static size_t cueOffset(const WorldAudioBank& bank,const WorldAudioCue& cue)
    {
        const uintptr_t begin=reinterpret_cast<uintptr_t>(bank.metadata);
        const uintptr_t address=reinterpret_cast<uintptr_t>(cue.metadata);
        if (!bank.metadata || !cue.metadata || cue.metadataSize!=52 || address<begin ||
            address-begin>bank.metadataSize || bank.metadataSize-(address-begin)<52)
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Cue does not borrow its retained bank metadata"};
        return static_cast<size_t>(address-begin);
    }
    void load(const WorldAudioRequest& operation)
    {
        const auto* bank=operation.bank;
        if (!bank || banks.count(bank))
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid physical bank load"};
        // Region readiness means the actual encoded MSB and its MSH are
        // loaded. Decode only cues requested by live voices, retaining the
        // same immutable bytes until ordered bank retirement.
        banks.emplace(bank,hp2::NativeSoundBankCache(*bank));
        required(library.CompleteRegion(operation,true),"Real bank completion was rejected");
    }
    bool liveFile(const Voice& voice) const noexcept
    {
        return voice.fileStream && (!voice.fileEnd || voice.fileRead.valid());
    }
    void refillFile(Voice& voice)
    {
        if (!voice.fileStream)return;
        XAUDIO2_VOICE_STATE state{};
        voice.device->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued>voice.streamBuffers.size())
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_FAIL,"File stream lost a retained PCM owner"};
        const size_t consumed=voice.streamBuffers.size()-state.BuffersQueued;
        voice.streamBuffers.erase(voice.streamBuffers.begin(),voice.streamBuffers.begin()+consumed);
        if (voice.fileRead.valid() &&
            voice.fileRead.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
            auto chunk=voice.fileRead.get();
            voice.streamBuffers.push_back(std::move(chunk.pcm));
            submit(voice,voice.streamBuffers.back(),chunk.last,false);
            voice.fileEnd=chunk.last;
        }
        // At most three PCM chunks, counting the one being decoded. Paused
        // playback keeps its current cursor and reaches this same bound.
        if (!voice.fileEnd && !voice.fileRead.valid() && voice.streamBuffers.size()<3) {
            auto* reader=voice.fileStream.get();
            voice.fileRead=std::async(std::launch::async,[reader] { return reader->Read(); });
        }
    }
    bool liveMovie(const Voice& voice) const noexcept
    {
        const auto* sample=voice.sample.instance;
        return sample && sample->occupied && !sample->retiring &&
            sample->generation==voice.sample.generation && voice.movieSource &&
            sample->streamSource==voice.movieSource && voice.movieSource->movie;
    }
    void refillMovie(Voice& voice)
    {
        if (!liveMovie(voice))return;
        XAUDIO2_VOICE_STATE state{};
        voice.device->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued>voice.streamBuffers.size())
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_FAIL,"Movie device queue lost its retained PCM owners"};
        // GetState has already observed these leading buffers as consumed.
        // Moving the remaining PcmWave vectors preserves their audio storage;
        // the XAudio2 thread can only consume more, never restore an old borrow.
        const auto consumed=voice.streamBuffers.size()-state.BuffersQueued;
        voice.streamBuffers.erase(voice.streamBuffers.begin(),voice.streamBuffers.begin()+consumed);
        // Three real channel-pair packets bound native PCM storage. This is
        // host queue depth, not another guessed audio-format descriptor.
        while (voice.streamBuffers.size()<3) {
            hp2::PcmWave pcm;
            if (!hp2::ReadMoviePcmChunk(*voice.movieSource,voice.movieHistory,pcm))break;
            voice.streamBuffers.push_back(std::move(pcm));
            submit(voice,voice.streamBuffers.back(),false,false);
        }
    }
    bool openMovie(const WorldAudioRequest& operation)
    {
        const auto* source=operation.streamSource;
        if (!source || !source->movie || !operation.instance || operation.instance->sampleType!=3 ||
            operation.instance->streamSource!=source || operation.streamOffset || operation.streamSize!=0x7fffffffu)
            throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Movie OPEN lost its exact callback source"};
        auto* voice=find(operation);
        if (!voice || voice->movieSource!=source) {
            voice=&allocate(operation,source->sampleRate,2);
            voice->movieSource=source;
        }
        refillMovie(*voice);
        if (voice->streamBuffers.empty())return false;
        voice->opened=true;voice->sequence=operation.sequence;
        return true;
    }
    void observe(const WorldAudioRequest& operation)
    {
        bVoiceStatusSnapshot snapshot{};
        for (uint16_t id=0;id<voices.size();++id) {
            auto& voice=voices[id];if (!voice.device)continue;
            if (voice.releaseTail) {
                XAUDIO2_VOICE_STATE tailState{};
                voice.device->GetState(&tailState,XAUDIO2_VOICE_NOSAMPLESPLAYED);
                // Original loopless EOF ends gain immediately. A drained
                // source cannot retain a tail while waiting for a future
                // source-APO Process that the device need not supply.
                if (!tailState.BuffersQueued || voice.envelope->Finished()) {
                    if (voice.releaseAfterTail) { destroy(voice);continue; }
                    checked(voice.device->Stop(),"XAudio2 completed release stop failed");
                    checked(voice.device->FlushSourceBuffers(),"XAudio2 completed release flush failed");
                    voice.releaseTail=false;
                }
                // STOP completion is already logical; an audible tail may
                // not resurrect the stopped sample or deliver a second EOF.
                snapshot[id]={voice.sample,voice.sequence,false,false,voice.mix.channels};
                continue;
            }
            refillMovie(voice);refillFile(voice);
            XAUDIO2_VOICE_STATE state{};
            voice.device->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
            const bool buffered=state.BuffersQueued!=0;
            if (!buffered)voice.residentPcm.reset();
            // A successful XAudio2 Start stays armed during callback starvation:
            // a movie has no END_OF_STREAM buffer until its owning Close/STOP.
            // This preserves that actual started operation, while readiness
            // still requires a real device buffer and is never made by a zero fill.
            const bool running=voice.started && (buffered || liveMovie(voice) || liveFile(voice));
            snapshot[id]={voice.sample,voice.sequence,running,
                voice.opened && buffered,voice.mix.channels};
        }
        required(library.Voices().CompleteUpdate(operation,snapshot),"Matched physical voice observation was rejected");
        // A ready callback may enqueue PLAY. Dispatch after completing this
        // observation, so its captured idle status cannot stop that later
        // PLAY before the command reaches the real device.
        for (uint16_t id=0;id<voices.size();++id) {
            auto& voice=voices[id];if (!voice.device)continue;
            const bool buffered=snapshot[id].prebuffered;
            auto* sample=voice.sample.instance;
            if (buffered && voice.opened && sample && sample->occupied && !sample->retiring &&
                sample->generation==voice.sample.generation && !sample->prebuffered &&
                sample->playSequence==voice.sequence) {
                const auto ready=sample->sampleType==1 ?
                    worldAudioSpeechEvent(&queue,voice.sample,voice.sequence,4):
                    worldAudioStreamEvent(&queue,voice.sample,voice.sequence,4);
                required(ready,"Real stream readiness callback failed");
            }
        }
    }
    void apply(const WorldAudioRequest& operation,hp2::NativeJukeboxStart* preparedStream=nullptr)
    {
        if (operation.kind==WORLD_AUDIO_INITIALIZE_DEVICE) {
            if (engine)throw Failure{WORLD_AUDIO_INVALID_BINDING,E_FAIL,"Device was initialized twice"};
            checked(hp2::InitializeNativeAudioCom(),"COM initialization failed");com=true;
            checked(XAudio2Create(&engine,0,XAUDIO2_DEFAULT_PROCESSOR),"XAudio2 engine creation failed");
            // The native endpoint uses two channels at 48 kHz; every
            // source retains its actual authored rate.
            auto* clockEffect=new hp2::NativeGainXapo(outputClock,{},true);
            XAUDIO2_EFFECT_DESCRIPTOR clockDescriptor{clockEffect,TRUE,2};
            XAUDIO2_EFFECT_CHAIN clockChain{1,&clockDescriptor};
            const auto masterResult=engine->CreateMasteringVoice(&master,2,48000,0,nullptr,&clockChain,AudioCategory_GameEffects);
            clockEffect->Release(); // The real mastering voice owns its COM reference.
            checked(masterResult,"Windows physical mastering endpoint creation failed");
            initializeEffects();
            return;
        }
        if (!master)throw Failure{WORLD_AUDIO_INVALID_BINDING,E_FAIL,"No Windows audio endpoint"};
        switch (operation.kind) {
        case WORLD_AUDIO_LOAD_REGION:load(operation);return;
        case WORLD_AUDIO_UNLOAD_BANK:
            for (auto& voice:voices)if (voice.device && voice.bank==operation.bank) {
                const auto sample=voice.sample;
                destroy(voice); // Join actual PCM borrows before erasing data.
                library.Voices().ReleaseSample(sample);
            }
            if (banks.erase(operation.bank)!=1)
                throw Failure{WORLD_AUDIO_STALE_REFERENCE,E_FAIL,"Unknown physical bank release"};
            return;
        case WORLD_AUDIO_SET_REGION_PATH:
            // GameResources already applies this path to the actual file
            // reads. There is no second device filesystem path owner.
            if (!std::memchr(operation.resourcePath,0,sizeof(operation.resourcePath)))
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Unterminated retained resource path"};
            return;
        case WORLD_AUDIO_SET_CHANNEL_VOLUME:
            // bSound owns the category and submits each affected sample's
            // resolved gain immediately after this CPU publication barrier.
            if (operation.channel<0 || operation.channel>=16 || !std::isfinite(operation.parameterValue))
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid mixer publication barrier"};
            return;
        case WORLD_AUDIO_DRIVER_UPDATE:observe(operation);return;
        case WORLD_AUDIO_PRIME: {
            auto* sample=operation.instance;
            if (!sample)throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Prime has no retained source"};
            uint32_t rate=0;uint16_t channels=1;
            if (sample->sampleType) {
                if (!sample->streamSource)
                    throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Stream prime has no retained format owner"};
                rate=sample->streamSource->sampleRate;
                channels=sample->sampleType==3 ? 2:1;
            } else {
                if (!operation.cue || !operation.cue->metadata || operation.cue->metadataSize!=52)
                    throw Failure{WORLD_AUDIO_RESOURCE_FAILURE,E_INVALIDARG,"Prime has no actual MSH record"};
                rate=hp2::word(operation.cue->metadata,52,0x20)>>16;
                channels=static_cast<uint16_t>(hp2::GetChannelNum(operation.cue->metadata[0x21])+1);
            }
            (void)allocate(operation,rate,channels);return;
        }
        case WORLD_AUDIO_PLAY: {
            required(worldAudioValidateVoice(&queue,{operation.instance,operation.instanceGeneration}),
                "Retained PLAY has no live sample owner");
            Voice* voice=find(operation);
            if (!operation.instance->sampleType) {
                const auto bank=banks.find(operation.bank);
                if (bank==banks.end() || !operation.cue)
                    throw Failure{WORLD_AUDIO_RESOURCE_FAILURE,E_FAIL,"Sample bank is not physically loaded"};
                auto pcm=bank->second.Acquire(cueOffset(*operation.bank,*operation.cue));
                voice=&allocate(operation,pcm->sampleRate,pcm->channels);
                voice->residentPcm=std::move(pcm);
                submit(*voice,*voice->residentPcm,true,hp2::GetLoop(operation.cue->metadata[0x21])!=0);
            } else if (!voice || !voice->opened) {
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_FAIL,"Stream PLAY has no actual prebuffered device source"};
            }
            voice->mix.Initialize(voice->mix.channels,!operation.instance->sampleType,
                operation.sampleVolume,operation.samplePan,mode);
            matrix(*voice);pitch(*voice,operation.samplePitch);
            voice->envelope->KeyOn();
            checked(voice->device->Start(),"XAudio2 source start failed");
            voice->started=true;voice->paused=false;voice->releaseTail=false;voice->releaseAfterTail=false;
            voice->sequence=operation.sequence;
            required(library.Voices().Assign(operation,static_cast<uint16_t>(voice-voices.data())),
                "PLAY physical operation identity was rejected");
            return;
        }
        case WORLD_AUDIO_OPEN_STREAM:
        case WORLD_AUDIO_APPEND_CHAIN:
        case WORLD_AUDIO_OPEN_CHAIN: {
            const auto* source=operation.streamSource;
            if (!source || source->movie)
                throw Failure{WORLD_AUDIO_RESOURCE_FAILURE,E_NOTIMPL,
                    "Movie OPEN must use its retained callback pump"};
            if (preparedStream) {
                if (operation.kind!=WORLD_AUDIO_OPEN_STREAM || !preparedStream->stream ||
                    operation.instance->completionKind!=WORLD_AUDIO_COMPLETION_JUKEBOX)
                    throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Prepared file stream lost its music owner"};
                auto& chunk=preparedStream->first;
                auto& voice=allocate(operation,chunk.pcm.sampleRate,chunk.pcm.channels);
                voice.fileStream=std::move(preparedStream->stream);voice.fileEnd=chunk.last;
                voice.streamBuffers.push_back(std::move(chunk.pcm));
                submit(voice,voice.streamBuffers.back(),chunk.last,false);
                voice.opened=true;voice.sequence=operation.sequence;
                return;
            }
            auto pcm=hp2::DecodeSoundStreamRange(operation);
            Voice* voice=find(operation);
            XAUDIO2_VOICE_STATE state{};if (voice)voice->device->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
            // A new chain replaces a stopped source's release tail. Appending
            // to that tail would let its later retirement flush the new PCM
            // before READY. The existing recreate path joins the old borrow.
            const bool append=operation.kind!=WORLD_AUDIO_OPEN_STREAM && voice &&
                !voice->releaseTail && state.BuffersQueued;
            if (!append && operation.kind==WORLD_AUDIO_APPEND_CHAIN) {
                if (!voice)throw Failure{WORLD_AUDIO_STALE_REFERENCE,E_FAIL,"Chain append lost its primed physical voice"};
                const auto sequence=voice->sequence;
                create(*voice,operation,pcm.sampleRate,pcm.channels,false);
                voice->sequence=sequence; // Same PRIME association until final OPEN.
            } else if (!append)voice=&allocate(operation,pcm.sampleRate,pcm.channels);
            if (voice->streamBuffers.size()>=XAUDIO2_MAX_QUEUED_BUFFERS)
                throw Failure{WORLD_AUDIO_CAPACITY,E_OUTOFMEMORY,"Chain exceeds actual device buffer capacity"};
            voice->streamBuffers.push_back(std::move(pcm));
            const bool last=operation.kind!=WORLD_AUDIO_APPEND_CHAIN;
            submit(*voice,voice->streamBuffers.back(),last,false);
            if (last) {
                voice->opened=true;voice->sequence=operation.sequence;
                required(library.Voices().Assign(operation,static_cast<uint16_t>(voice-voices.data())),
                    "OPEN physical operation identity was rejected");
            }
            return;
        }
        case WORLD_AUDIO_STOP: {
            auto* voice=find(operation);
            // A previous ordered STOP may have already destroyed exactly
            // this voice. The absence is valid only with no live association.
            if (!voice) {
                for (const auto& binding:library.Voices().Voices())
                    if (binding.GetSample().instance==operation.instance &&
                        binding.GetSample().generation==operation.instanceGeneration)
                        throw Failure{WORLD_AUDIO_STALE_REFERENCE,E_FAIL,"STOP lost a still-bound physical voice"};
                required(library.Voices().CompleteStop(operation),"Matched stopped voice identity was rejected");
                return;
            }
            XAUDIO2_VOICE_STATE state{};
            voice->device->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
            const bool tail=voice->started && !voice->paused && state.BuffersQueued;
            if (tail) {
                // STREAM command4 keys resident voices off without zeroing
                // gain. Stream command5 replaces release with rate0. Both
                // curves use output time; the original request ACK need not
                // keep a mutable sample alive through this owned PCM tail.
                voice->envelope->KeyOff(operation.instance->sampleType!=0);
                voice->releaseTail=true;voice->releaseAfterTail=operation.releaseDeviceVoice!=0;
            } else {
                checked(voice->device->Stop(),"XAudio2 source stop failed");
                checked(voice->device->FlushSourceBuffers(),"XAudio2 buffer flush failed");
            }
            voice->started=false;voice->paused=false;voice->opened=false;
            voice->movieSource=nullptr;voice->movieHistory={};
            if (voice->fileRead.valid()) {
                voice->fileRead.wait();
                // STOP owns retirement, so an unused final chunk (or its
                // decode error) cannot publish readiness to another track.
                voice->fileRead={};
            }
            voice->fileStream.reset();voice->fileEnd=false;
            if (operation.releaseDeviceVoice) {
                if (tail)voice->sample={};
                else destroy(*voice);
            }
            required(library.Voices().CompleteStop(operation),"Matched stopped voice identity was rejected");
            return;
        }
        case WORLD_AUDIO_SET_SAMPLE_VOLUME: {
            auto& voice=requireVoice(operation);voice.mix.SetVolume(operation.sampleVolume,mode);matrix(voice);return;
        }
        case WORLD_AUDIO_SET_PITCH_OFFSET:pitch(requireVoice(operation),operation.samplePitch);return;
        case WORLD_AUDIO_SET_CONTROL_CODE: {
            auto& voice=requireVoice(operation);voice.mix.SetAzimuth(operation.controlCode,mode);matrix(voice);return;
        }
        case WORLD_AUDIO_SET_PAUSED: {
            auto& voice=requireVoice(operation);const bool paused=operation.parameterValue!=0.0f;
            if (paused)checked(voice.device->Stop(),"XAudio2 pause failed");
            else if (voice.started)checked(voice.device->Start(),"XAudio2 resume failed");
            voice.paused=paused;return;
        }
        case WORLD_AUDIO_SET_MUTED: {
            auto& voice=requireVoice(operation);
            voice.mix.Mute(operation.muted!=0,operation.instance->savedMuteVolume,mode);
            matrix(voice);
            voice.muted=operation.muted!=0;return;
        }
        case WORLD_AUDIO_FADE_VOLUME:
            // The sample owner evaluates fade against native monotonic time
            // on observation; SET_SAMPLE_VOLUME applies the actual gain.
            if (!std::isfinite(operation.parameterValue) || operation.durationMilliseconds<=32)
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid CPU fade publication"};
            return;
        case WORLD_AUDIO_SET_CORE_EFFECT_VOLUME: {
            auto& effect=effectBus(operation);effect.volume=operation.effectCoreVolume;
            effectMatrix(effect);return;
        }
        case WORLD_AUDIO_SET_CORE_EFFECT_PAN: {
            auto& effect=effectBus(operation);effect.pan=operation.effectCorePan;
            effectMatrix(effect);return;
        }
        case WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE:effectAttribute(operation);return;
        case WORLD_AUDIO_SET_CORE_EFFECT_ENABLED:
            if(operation.effectPreset!=0 && operation.effectPreset!=1)
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid retained effect enable value"};
            effectEnabled(effectBus(operation),operation.effectPreset!=0);return;
        case WORLD_AUDIO_SET_AUDIO_MODE:
            if (operation.audioMode<0 || operation.audioMode>2)
                throw Failure{WORLD_AUDIO_INVALID_BINDING,E_INVALIDARG,"Invalid retained audio output mode"};
            mode=static_cast<uint8_t>(operation.audioMode+1);
            for(auto& effect:effects)effectMatrix(effect);
            for (auto& voice:voices)if (voice.device) {
                voice.mix.SetMode(mode);matrix(voice);
            }
            return;
        default:
            throw Failure{WORLD_AUDIO_BACKEND_FAILURE,E_NOTIMPL,
                "This retained SDK DSP/device operation has no implemented Windows mapping"};
        }
    }
    WorldAudioResult pump(size_t maximum)
    {
        if (std::this_thread::get_id()!=thread || queue.generation!=generation)
            return WORLD_AUDIO_STALE_REFERENCE;
        if (library.windowsFailure_.result!=WORLD_AUDIO_OK)return library.windowsFailure_.result;
        try {
            // Native buffer progress is independent of a logical game tick.
            // This never advances world/input clocks or invents readiness.
            for (auto& voice:voices)if (voice.device)refillFile(voice);
            const auto result=commands.Pump(queue,maximum,[&](const WorldAudioRequest& operation) {
                using Applied=hp2::NativeAudioApply;
                const bool ownsDecode=jukeboxDecode.valid() && jukeboxSequence==operation.sequence;
                // The work item owns a FileRange. Never destroy its future
                // on the game thread while reading/decoding is still active:
                // unrelated voices keep their own command/status progress.
                if (ownsDecode &&
                    jukeboxDecode.wait_for(std::chrono::seconds(0))!=std::future_status::ready)
                    return Applied::Blocked;
                if (operation.instance && operation.instance->retiring && operation.kind!=WORLD_AUDIO_STOP) {
                    // No physical OPEN was performed: retire through the
                    // original queued failure and then drain any decode result.
                    if (ownsDecode) {
                        try { (void)jukeboxDecode.get(); }
                        catch (const std::exception&) { /* retired decode has no live owner */ }
                        jukeboxSequence=0;
                    }
                    return Applied::Cancelled;
                }
                if (operation.kind==WORLD_AUDIO_OPEN_STREAM && operation.streamSource && operation.streamSource->movie) {
                    // Movie FIFO is a separate owner-thread callback.
                    if (!openMovie(operation))return Applied::Blocked;
                } else if (operation.kind==WORLD_AUDIO_OPEN_STREAM && operation.instance &&
                    operation.instance->completionKind==WORLD_AUDIO_COMPLETION_JUKEBOX) {
                    if (!jukeboxDecode.valid()) {
                        auto range=hp2::CaptureStreamDecodeRange(operation);
                        // The worker accesses only copied immutable data,
                        // never the live queue or device.
                        jukeboxDecode=std::async(std::launch::async,
                            [range=std::move(range)] { return hp2::PrepareJukeboxStream(range); });
                        jukeboxSequence=operation.sequence;
                        return Applied::Blocked;
                    }
                    if (!ownsDecode)return Applied::Blocked;
                    auto pcm=jukeboxDecode.get();
                    jukeboxSequence=0;
                    apply(operation,&pcm);
                } else apply(operation);
                return Applied::Applied;
            });
            required(result,"Completed physical request acknowledgement failed");
        } catch (const Failure& failure) {
            const auto& operation=commands.Current();
            library.windowsFailure_={failure.result,operation ? operation->kind:WORLD_AUDIO_INITIALIZE_DEVICE,
                operation ? operation->sequence:0,static_cast<int32_t>(failure.system),failure.reason};
            return failure.result;
        } catch (const std::bad_alloc&) {
            const auto& operation=commands.Current();
            library.windowsFailure_={WORLD_AUDIO_CAPACITY,operation ? operation->kind:WORLD_AUDIO_INITIALIZE_DEVICE,
                operation ? operation->sequence:0,static_cast<int32_t>(E_OUTOFMEMORY),"Native audio allocation failed"};
            return WORLD_AUDIO_CAPACITY;
        } catch (const std::exception& error) {
            const auto& operation=commands.Current();
            std::string context;
            if (operation && operation->kind==WORLD_AUDIO_LOAD_REGION)
                context=std::string("Bank '")+operation->regionName+"': ";
            else if (operation && operation->streamSource)
                context=std::string("Stream '")+operation->streamSource->path+"': ";
            else if (operation && operation->cue && operation->cue->name)
                context=std::string("Cue '")+operation->cue->name+"': ";
            library.windowsFailure_={WORLD_AUDIO_RESOURCE_FAILURE,operation ? operation->kind:WORLD_AUDIO_INITIALIZE_DEVICE,
                operation ? operation->sequence:0,static_cast<int32_t>(E_INVALIDARG),context+error.what()};
            return WORLD_AUDIO_RESOURCE_FAILURE;
        }
        return WORLD_AUDIO_OK;
    }
};

void bSound::DeviceDeleter::operator()(NativeDevice* device) const noexcept { delete device; }

WorldAudioResult bSound::OpenWindowsDevice()
{
    if (!queue_ || !status_ || windowsDevice_ || !queue_->count ||
        queue_->retainedCount!=queue_->count ||
        queue_->requests[queue_->readIndex].kind!=WORLD_AUDIO_INITIALIZE_DEVICE)
        return WORLD_AUDIO_INVALID_BINDING;
    windowsFailure_={};
    try {windowsDevice_.reset(new NativeDevice(*this));}
    catch (const std::bad_alloc&) {return WORLD_AUDIO_CAPACITY;}
    const auto result=windowsDevice_->pump(1);
    if(result!=WORLD_AUDIO_OK) {
        // Initialization failure still owns a real XAudio2 device/COM scope.
        // Join and reject its actual queued work before exposing the failure;
        // cancellation is never reported as a successful playback ACK.
        const auto failure=windowsFailure_;
        const auto closed=CloseWindowsDevice();
        windowsFailure_=failure;
        if(windowsDevice_)return closed;
    }
    return result;
}

WorldAudioResult bSound::PumpWindowsDevice(size_t maximumOperations)
{
    if (!windowsDevice_)return WORLD_AUDIO_INVALID_BINDING;
    return windowsDevice_->pump(maximumOperations);
}


WorldAudioResult bSound::CloseWindowsDevice()
{
    if (!windowsDevice_ || std::this_thread::get_id()!=windowsDevice_->thread)
        return WORLD_AUDIO_INVALID_BINDING;
    auto& device=*windowsDevice_;
    if (queue_->generation!=device.generation)return WORLD_AUDIO_STALE_REFERENCE;
    for (auto& voice:device.voices) {
        const auto sample=voice.sample;
        device.destroy(voice); // Real API join precedes release of every borrow.
        if (sample.instance && sample.instance->occupied && sample.instance->generation==sample.generation) {
            sample.instance->hasHandle=0;sample.instance->playing=0;
            voices_.ReleaseSample(sample);
        }
    }
    // Master/engine destruction also completes before any queue payload can
    // expire. The original failed operation remains recorded independently
    // of this terminal close result; returning that old error here masked
    // the startup exception and caused a second, fatal shutdown attempt.
    windowsDevice_.reset();
    return CancelDeviceRequestsAfterClose();
}
#endif

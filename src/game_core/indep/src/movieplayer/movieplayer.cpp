#include "MV3Codec.h"
#include "RingBuffer.h"
#include "bware/inc/bFile.hpp"
#include "bsound/include/bSample.h"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace hp2 {
namespace {
std::int32_t signedWord(std::uint32_t bits) noexcept {
    return bits<=0x7fffffffu ? static_cast<std::int32_t>(bits) :
        -1-static_cast<std::int32_t>(~bits);
}
std::uint32_t floatBits(float value) noexcept {
    std::uint32_t bits;
    std::memcpy(&bits,&value,sizeof(bits));
    return bits;
}
float bitsFloat(std::uint32_t bits) noexcept {
    float value;
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
float frameInterval(float fps) noexcept {
    // Native IEEE arithmetic: a zero playback rate has an infinite interval.
    // No console DIV.S saturation or FPU state is reproduced on the PC.
    return 1.0f/fps;
}
std::uint32_t unsignedFloat(float value) noexcept {
    // The released __fixunssfsi truncates in the unsigned word domain.
    // Authored movie clocks are finite/nonnegative; preserve explicit limits
    // rather than invoke C++ undefined float-to-integer overflow.
    if (!(value>=0.0f))return 0;
    if (value>=4294967296.0f)return 0xffffffffu;
    return static_cast<std::uint32_t>(value);
}
void audioResult(WorldAudioResult result) {
    if (result!=WORLD_AUDIO_OK)
        throw std::runtime_error("Movie audio queue rejected its real stream operation: "+
            std::to_string(static_cast<int>(result)));
}
}

struct MoviePlayer::Storage {
    struct StreamHeader {
        std::uint32_t type{},handler{},flags{},priorityAndLanguage{};
        std::uint16_t firstHalf{},secondHalf{};
        std::uint32_t scale{},rate{},start{},length{},suggestedBytes{},quality{},sampleSize{},last{};
    };
    const GameResources& resources;
    ResourceRegistry& registry;
    const GameTime& clock;
    const std::int32_t& soundEnabled;
    const std::int32_t& fivePass;
    WorldAudioRequestQueue* audio;
    FileRange file;
    RingBuffer ring;
    MV3Codec codec;
    std::shared_ptr<WorldAudioStreamSource> stream;
    WorldAudioVoiceRef sample{};
    std::string filename;
    float currentTime{},frameTime{},fps{},startTime{};
    std::uint32_t currentFrame{},junkSkip{};
    bool externalPlayback{}, externalFinished{};
    std::uint64_t movieRequestId{};
    std::int32_t lastReadFrame{};
    bool withAudio{},firstUpdate{},validFrame{};
    BitmapInfo input{},output{};
    StreamHeader video{},sound{};
    ICDecompressEx request{};
    // Y, UA, VA, US, VS: the actual loaded pack records. No second texture
    // catalogue or independent image allocation is introduced.
    std::array<TextureInfo*,5> textures{};
    std::array<std::shared_ptr<const TextureMetadataStorage>,5> textureOwners{};
    DCTFrameView planes{};
    std::exception_ptr shutdownFailure;
    bool closing{};

    Storage(const GameResources& files,FileIoRuntime& io,ResourceRegistry& models,
        const GameTime& time,const std::int32_t& enabled,const std::int32_t& pass,
        WorldAudioRequestQueue* queue)
        :resources(files),registry(models),clock(time),soundEnabled(enabled),
         fivePass(pass),audio(queue),ring(io) {}

    void DetachPlanes() noexcept {
        const std::array<std::uint8_t*,5> addresses{{planes.y,planes.u,planes.v,planes.u,planes.v}};
        // The lease keeps each destination allocated even after its pack
        // revokes publication; clear our own plane before releasing it.
        for (std::size_t i=0;i<textures.size();++i)
            if (textures[i] && textureOwners[i] && textures[i]->pixels.borrowed==addresses[i])
                textures[i]->pixels.borrowed=nullptr;
        textures.fill(nullptr);
        for (auto& owner:textureOwners)owner.reset();
        planes={};
    }
};

// PAL237698..2376F8. The ring and codec are the actual embedded owners.
MoviePlayer::MoviePlayer(const GameResources& resources,FileIoRuntime& io,
    ResourceRegistry& registry,const GameTime& clock,const std::int32_t& soundEnabled,
    const std::int32_t& enableFivePass,WorldAudioRequestQueue* audio)
    :storage_(new Storage(resources,io,registry,clock,soundEnabled,enableFivePass,audio)) {}

// PAL2376F8..23775C. Native C++ provides the deleting-destructor ABI.
MoviePlayer::~MoviePlayer() { (void)Shutdown(); }

// PAL237760..237AB4. These offsets are parsed from final RIFF bytes. In
// particular this is the source's52-byte stream projection, not a substituted
// SDK AVISTREAMHEADER or the beta object's member layout.
int MoviePlayer::ExtractVideoHeader(const void* header,std::size_t bytes) {
    auto& s=*storage_;
    s.withAudio=false;
    if (!header)return 0;
    const auto* data=static_cast<const std::uint8_t*>(header);
    const auto contains=[&](std::uint64_t offset,std::uint64_t length) {
        return offset<=bytes && length<=bytes-offset;
    };
    const auto word=[&](std::size_t at) {return GetU32(data+at);};
    if (!contains(0,32) || word(0)!=0x46464952u || word(8)!=0x20495641u)return 0;
    const std::uint64_t avih=word(28);
    const std::uint64_t listAt=avih+32;
    if (!contains(listAt,8) || word(static_cast<std::size_t>(listAt))!=0x5453494cu)return 0;
    const std::uint64_t listSize=word(static_cast<std::size_t>(listAt+4));
    const std::uint64_t nextList=listAt+listSize+8;
    const std::uint64_t streamAt=avih+48;
    if (!contains(streamAt,56) || word(static_cast<std::size_t>(streamAt+4))!=0x73646976u)return 0;
    const auto stream=[&](std::size_t at,Storage::StreamHeader& out) {
        out.type=word(at);out.handler=word(at+4);out.flags=word(at+8);
        out.priorityAndLanguage=word(at+12);
        out.firstHalf=GetU16(data+at+16);out.secondHalf=GetU16(data+at+18);
        out.scale=word(at+20);out.rate=word(at+24);out.start=word(at+28);
        out.length=word(at+32);out.suggestedBytes=word(at+36);out.quality=word(at+40);
        out.sampleSize=word(at+44);out.last=word(at+48);
    };
    stream(static_cast<std::size_t>(streamAt+4),s.video);
    const std::uint64_t strf=streamAt+word(static_cast<std::size_t>(streamAt));
    if (!contains(strf,64) || word(static_cast<std::size_t>(strf+4))!=0x66727473u)return 0;
    const auto at=static_cast<std::size_t>(strf+12);
    auto& b=s.input.bmiHeader;
    b.biSize=word(at);b.biWidth=signedWord(word(at+4));b.biHeight=signedWord(word(at+8));
    b.biPlanes=GetU16(data+at+12);b.biBitCount=GetU16(data+at+14);
    b.biCompression=word(at+16);b.biSizeImage=word(at+20);
    b.biXPelsPerMeter=signedWord(word(at+24));b.biYPelsPerMeter=signedWord(word(at+28));
    b.biClrUsed=word(at+32);b.biClrImportant=word(at+36);
    b.biRedMask=word(at+40);b.biGreenMask=word(at+44);b.biBlueMask=word(at+48);
    if (!contains(nextList,24) || word(static_cast<std::size_t>(nextList))!=0x5453494cu ||
        word(static_cast<std::size_t>(nextList+20))!=0x73647561u)return 1;
    if (!contains(nextList+20,52))return 0;
    s.withAudio=true;
    stream(static_cast<std::size_t>(nextList+20),s.sound);
    return 1;
}

// PAL237AD8..237FC8. The PS2-only bSound device flush/busy waits are replaced
// by ordered retirement of the real native sample and retained request data.
// They are not emulated timers or fake completion calls.
int MoviePlayer::InitFile(const char* filename) {
    auto& s=*storage_;
    if (!filename)return 0;
    const std::string requested(filename); // May alias this owner's saved name.
    Close();
    s.closing=false;
    s.lastReadFrame=s.clock.realTimeFrames;
    s.currentTime=0.0f;s.currentFrame=0;
    s.filename=requested;
    if(s.externalPlayback) {
        ++s.movieRequestId;
        s.externalFinished=false;
        return 1;
    }
    const auto physical=s.resources.TryOpenPhysical(requested);
    if (!physical || physical->expanded)return 0;
    s.file=physical->range;
    if (s.file.Size()<4096)return 0;
    alignas(32) std::array<std::uint8_t,4096> header{};
    s.file.Read(0,header.data(),header.size());
    if (!ExtractVideoHeader(header.data(),header.size()))return 0;
    s.fps=static_cast<float>(static_cast<double>(s.video.rate)/static_cast<double>(s.video.scale));
    s.frameTime=frameInterval(s.fps);
    s.junkSkip=2048; // The explicit final-PAL branch when no movi is found.
    for (std::uint32_t at=0;at<header.size();at+=4)
        if (GetU32(header.data()+at)==0x69766f6du) {s.junkSkip=at+4;break;}
    s.codec.GetDecompressFormat(&s.input,&s.output);
    auto& output=s.output.bmiHeader;
    output.biBitCount=32;output.biCompression=0;
    output.biHeight=signedWord(0u-static_cast<std::uint32_t>(output.biHeight));
    output.biSizeImage=static_cast<std::uint32_t>(output.biWidth)*
        (0u-static_cast<std::uint32_t>(output.biHeight))*4u;
    s.request={};s.request.lpbiSrc=&s.input.bmiHeader;s.request.lpbiDst=&s.output.bmiHeader;
    if (s.codec.QueryDecompressEx(&s.request,52)!=0)return 0;
    s.codec.BeginDecompressEx(&s.request,52);
    s.ring.Init(0x80000,s.withAudio);
    s.ring.StartupBuffer(s.file,s.junkSkip);
    s.ring.SetFrameLimit(s.video.length);
    s.firstUpdate=true;
    static constexpr const char* endings[]={"Y","UA","VA","US","VS"};
    for (std::size_t i=0;i<s.textures.size();++i) {
        char name[80]{};
        std::snprintf(name,sizeof(name),"MOVIE_%d_%d_%s",Width(),Height(),endings[i]);
        auto* texture=textureInfoForDescriptor(s.registry.ResolveTexture(hashString(name),false));
        if (texture) {
            auto owner=textureMetadataOwner(texture->descriptor);
            Require(textureMetadataPublished(owner),"Movie texture has no published metadata owner");
            s.textureOwners[i]=std::move(owner);
            s.textures[i]=texture;
        }
    }
    const auto palette=[&](std::size_t i)->std::uint32_t* {
        auto* texture=s.textures[i];
        if (!texture)return nullptr;
        if (!texture->palette.MutableData() || texture->palette.size<1024)
            throw std::runtime_error("Movie palette does not retain its authored256 entries");
        return reinterpret_cast<std::uint32_t*>(texture->palette.MutableData());
    };
    BuildYUV2ABGRPalettes(palette(0),palette(1),palette(3),palette(2),palette(4));
    // These palettes are generated on the PC, not loaded in the authored
    // console texture layout. Keep their native order with the owning view;
    // copies preserve the format and a physical reload constructs a new view.
    for (auto* texture:s.textures)
        if (texture)texture->palette.nativeLinearPalette=true;
    return 1;
}

// PAL237FC8..238070. Source planes retire after synchronous CPU capture;
// independent GPU allocations keep their own fence lifetime. Pending audio
// operations retain their source until actual retirement/acknowledgement.
void MoviePlayer::Close() {
    if (const auto failure=Shutdown())std::rethrow_exception(failure);
}

std::exception_ptr MoviePlayer::Shutdown() noexcept {
    auto& s=*storage_;
    const auto rememberFailure=[&]() noexcept {
        if (!s.shutdownFailure)s.shutdownFailure=std::current_exception();
    };
    s.closing=true;s.validFrame=false;s.externalFinished=false;
    // Retire these independent writable borrows even if audio submission
    // fails. A retained audio source cannot call back into a released ring.
    s.DetachPlanes();
    if (s.stream)s.stream->movie=nullptr;
    try {
        if (s.sample.instance) {
            audioResult(worldAudioRetireVoice(s.audio,s.sample));
            s.sample={};
        }
    } catch (...) {rememberFailure();}
    // Failure retains the exact voice/source for retry. Submitted/deferred
    // STOP already retains its stream metadata independently of this owner.
    if (!s.sample.instance)s.stream.reset();
    s.codec.EndDecompress();
    try {
        s.ring.ReleaseAll(s.file.Valid()?&s.file:nullptr);
        s.file={};
    } catch (...) {rememberFailure();}
    return s.shutdownFailure;
}

// PAL238070..238088; intentionally no invented range-clamp on the authored FPS.
void MoviePlayer::SetFPS(float fps) {
    storage_->fps=fps;storage_->frameTime=frameInterval(fps);
}

// PAL238088..2380A4. The actual audio FIFO supplies the callback bytes.
int MoviePlayer::GetAudio(void* output,std::uint32_t unused,std::uint32_t bytes) {
    auto& s=*storage_;
    return s.closing ? 0 : s.ring.GetAudio(output,signedWord(unused),signedWord(bytes));
}

// PAL2380A8..23851C supplies the timeline, streaming owner and decoder. The
// native scheduler may decode several due frames per presentation: MV3 delta
// pictures must still consume every preceding encoded frame in order.
int MoviePlayer::Update(float elapsed,void* output,std::size_t outputBytes) {
    static_cast<void>(elapsed); // Final code does not use this argument.
    auto& s=*storage_;
    if(s.externalPlayback)return !s.closing && !s.externalFinished;
    if (s.closing || !s.file.Valid())return 0;
    for (std::size_t i=0;i<s.textures.size();++i)
        if (s.textures[i] && !textureMetadataPublished(s.textureOwners[i])) {
            s.DetachPlanes();s.validFrame=false;
            throw std::runtime_error("Movie texture metadata retired while the movie was active");
        }
    if (s.firstUpdate)
        s.ring.SetAudioConsumerActive(s.withAudio && s.soundEnabled!=0);
    s.ring.ReadData(s.file,0x18000);
    const float displayRate=gameTimeGetVideoMode(&s.clock)==0 ? 50.0f : bitsFloat(0x426fc28fu);
    if (s.firstUpdate) {
        const bool playAudio=s.withAudio && s.soundEnabled!=0;
        if (playAudio) {
            if (!s.audio)throw std::logic_error("Movie audio is enabled without the live audio manager");
            // PRIME and OPEN are one owner-thread transaction. Reserve their
            // complete capacity before acquiring a voice; a rejected OPEN
            // must not leave a half-created sample during stack unwinding.
            if (worldAudioAvailableRequests(s.audio)<2 ||
                s.audio->nextSequence>std::numeric_limits<std::uint64_t>::max()-2)
                throw std::runtime_error("Movie audio needs two available transport requests");
            s.stream=std::make_shared<WorldAudioStreamSource>();
            s.stream->path=s.filename;s.stream->movie=this;
            s.stream->sampleRate=0xAA66u; // PAL2381B0/2381B8, before category/Open.
            const auto created=worldAudioCreateStreamVoice(s.audio,s.stream.get(),&s.sample);
            if (s.sample.instance)s.stream->retainedByVoice=s.stream;
            audioResult(created);
            audioResult(worldAudioSetVoiceCategory(s.audio,s.sample,0xffffffffu));
            audioResult(worldAudioOpenStream(s.audio,s.sample,s.stream.get(),0,0x7fffffffu));
        }
        s.startTime=static_cast<float>(graphicsVBlankCount())/displayRate;
        if (playAudio)s.startTime+=bitsFloat(0x3e23d70au);
        s.firstUpdate=false;
    }
    const auto ticks=signedWord(static_cast<std::uint32_t>(s.clock.realTimeFrames)-
        static_cast<std::uint32_t>(s.lastReadFrame));
    if (static_cast<float>(ticks)*bitsFloat(0x3c888889u)>20.0f) {
        // PAL2545E8 only copies the file's name to an unused80-byte local.
        // It is not another file read or a new disk watchdog transport.
        s.lastReadFrame=s.clock.realTimeFrames;
        return 0;
    }
    s.currentTime=std::max(static_cast<float>(graphicsVBlankCount())/displayRate-s.startTime,0.0f);
    const auto targetFrame=unsignedFloat(s.currentTime/s.frameTime);
    if (s.currentFrame>=targetFrame && s.validFrame)return 1;
    // A temporarily absent destination is prefetch, not permission to discard
    // an encoded reference picture. The normal frontend supplies its texture.
    if (!output)return 1;
    // Bound CPU catch-up after a stalled window/device without skipping a
    // dependent picture or blocking on physical I/O. Even15Hz presentation
    // can keep a25fps source current; longer stalls recover over later calls.
    constexpr std::uint32_t maximumCatchUpFrames=8;
    for (std::uint32_t decoded=0;decoded<maximumCatchUpFrames;++decoded) {
        auto* frame=s.ring.GetFrame();
        while (frame && !frame->videoData) {s.ring.ReleaseFrame();frame=s.ring.GetFrame();}
        if (!frame) {
            // A frame decoded in this call must be presented before returning
            // EOF. Pending native I/O and full audio FIFO are not completion.
            if (decoded)break;
            return s.ring.Drained() || s.currentFrame>=s.video.length ? 0:1;
        }
        s.planes=s.codec.Image().Current();
        s.input.bmiHeader.biSizeImage=frame->videoBytes;
        s.request.lpSrc=frame->videoData;s.request.lpDst=static_cast<std::uint8_t*>(output);
        s.codec.ConvertImageToRGBA=s.fivePass==0;
        s.codec.DecompressFrameEx(&s.request,s.ring.ReadableBytes(frame->videoData),outputBytes);
        ++s.currentFrame;s.lastReadFrame=s.clock.realTimeFrames;
        s.validFrame=true;s.ring.ReleaseFrame();
        if (s.currentFrame>=targetFrame)break;
        // Release before refill: the parser may reuse this exact record's
        // allocation. Audio backpressure and asynchronous read readiness keep
        // their existing owner-thread contracts.
        if (decoded+1<maximumCatchUpFrames)s.ring.ReadData(s.file,0x18000);
    }
    const std::array<std::uint8_t*,5> planes{{s.planes.y,s.planes.u,s.planes.v,s.planes.u,s.planes.v}};
    for (std::size_t i=0;i<s.textures.size();++i)
        if (s.textures[i])s.textures[i]->pixels.borrowed=planes[i];
    return 1;
}

// PAL238520..238944 supplies the exact integer coefficients/truncation.
// PC-generated color tables remain linear; the native texture consumer uses
// that explicit format instead of a console CLUT permutation roundtrip.
void MoviePlayer::BuildYUV2ABGRPalettes(std::uint32_t* y,std::uint32_t* ua,
    std::uint32_t* us,std::uint32_t* va,std::uint32_t* vs) {
    for (auto* table:{y,ua,us,va,vs})if (table)std::fill_n(table,256,0u);
    if (y)for (std::uint32_t i=0;i<256;++i)y[i]=0x80000000u|i|(i<<8)|(i<<16);
    const auto positive=[](int value) {return static_cast<std::uint32_t>(std::clamp(value,0,255));};
    int red=-45952,blue=-58112,greenV=23424,greenU=11264;
    for (std::size_t i=0;i<256;++i) {
        const int r=red/256,b=blue/256,gv=greenV/256,gu=greenU/256;
        if (va)va[i]|=0x80000000u|positive(r)|(positive(gv)<<8);
        if (vs)vs[i]|=0x80000000u|positive(-r)|(positive(-gv)<<8);
        if (ua)ua[i]|=0x80000000u|(positive(b)<<16)|(positive(gu)<<8);
        if (us)us[i]|=0x80000000u|(positive(-b)<<16)|(positive(-gu)<<8);
        red+=359;blue+=454;greenV-=183;greenU-=88;
    }
}


int MoviePlayer::Width() const noexcept {return storage_->output.bmiHeader.biWidth;}
int MoviePlayer::Height() const noexcept {
    return signedWord(0u-static_cast<std::uint32_t>(storage_->output.bmiHeader.biHeight));
}
bool MoviePlayer::ValidFrame() const noexcept {return storage_->validFrame;}
void MoviePlayer::UseExternalPlayback() noexcept {storage_->externalPlayback=true;}
const char* MoviePlayer::RequestedMovie() const noexcept {
    return storage_->externalPlayback && !storage_->closing && storage_->movieRequestId
        ? storage_->filename.c_str():nullptr;
}
std::uint64_t MoviePlayer::MovieRequestId() const noexcept {return storage_->movieRequestId;}
void MoviePlayer::CompleteExternalMovie(std::uint64_t request) noexcept {
    if(storage_->externalPlayback && !storage_->closing && request==storage_->movieRequestId)
        storage_->externalFinished=true;
}
const GameTime& MoviePlayer::Clock() const noexcept {return storage_->clock;}
std::array<std::uint32_t,16> MoviePlayer::InspectState() const noexcept {
    const auto& s=*storage_;
    return {{floatBits(s.currentTime),floatBits(s.frameTime),floatBits(s.fps),s.currentFrame,
        s.junkSkip,s.withAudio,s.firstUpdate,static_cast<std::uint32_t>(s.lastReadFrame),
        s.video.scale,s.video.rate,s.video.length,s.input.bmiHeader.biSizeImage,
        static_cast<std::uint32_t>(Width()),static_cast<std::uint32_t>(Height()),
        s.validFrame,floatBits(s.startTime)}};
}
std::array<std::uint32_t,39> MoviePlayer::InspectHeaders() const noexcept {
    const auto& s=*storage_;
    std::array<std::uint32_t,39> words{};
    std::memcpy(words.data(),&s.input.bmiHeader,52);
    const auto put=[&](std::size_t at,const Storage::StreamHeader& h) {
        const std::array<std::uint32_t,13> fields{{h.type,h.handler,h.flags,h.priorityAndLanguage,
            std::uint32_t{h.firstHalf}|(std::uint32_t{h.secondHalf}<<16),
            h.scale,h.rate,h.start,h.length,h.suggestedBytes,h.quality,h.sampleSize,h.last}};
        std::copy(fields.begin(),fields.end(),words.begin()+static_cast<std::ptrdiff_t>(at));
    };
    put(13,s.video);put(26,s.sound);
    return words;
}
} // namespace hp2

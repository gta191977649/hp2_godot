#include "game_core/movieplayer/movieplayer.hpp"
#include "game_core/not_implemented.hpp"
#include <algorithm>
#include <climits>
#include <cstring>
#include <limits>
#include <stdexcept>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}

namespace port::media {
namespace {
std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t p) {
    if (p > b.size() || b.size() - p < 4) throw std::runtime_error("AVI truncated word");
    return b[p] | std::uint32_t(b[p+1])<<8 | std::uint32_t(b[p+2])<<16 | std::uint32_t(b[p+3])<<24;
}
std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t p) {
    if (p > b.size() || b.size() - p < 2) throw std::runtime_error("AVI truncated halfword");
    return b[p] | std::uint16_t(b[p+1])<<8;
}
bool tag(const std::vector<std::uint8_t>& b, std::size_t p, const char* t) {
    return p <= b.size() && b.size()-p >= 4 && std::memcmp(b.data()+p,t,4)==0;
}
struct Reader {
    const std::vector<std::uint8_t>& b; AviMovie result;
    unsigned streams{}, video_stream{~0u}, audio_stream{~0u};
    void chunks(std::size_t begin, std::size_t end, bool movi=false, unsigned stream=~0u) {
        for (auto p=begin; p < end;) {
            if (end-p < 8) throw std::runtime_error("AVI truncated chunk header");
            const auto n=u32(b,p+4); const auto data=p+8;
            if (n > end-data) throw std::runtime_error("AVI chunk escapes container");
            if (tag(b,p,"LIST")) {
                if(n<4) throw std::runtime_error("AVI short LIST");
                const auto child_stream=tag(b,data,"strl") ? streams++ : stream;
                chunks(data+4,data+n,movi||tag(b,data,"movi"),child_stream);
            } else if (tag(b,p,"strh")) {
                if (n<48) throw std::runtime_error("AVI short stream header");
                if(tag(b,data,"vids")) {
                    if(video_stream != ~0u) throw std::runtime_error("AVI multiple video streams unsupported");
                    if(!tag(b,data+4,"mv30") && !tag(b,data+4,"MV30")) HP2_NOT_IMPLEMENTED("0x00238FB0","non-MV30 AVI codec");
                    video_stream=stream; result.scale=u32(b,data+20);result.rate=u32(b,data+24);
                } else if(tag(b,data,"auds")) audio_stream=stream;
            } else if(tag(b,p,"strf")) {
                if(stream==video_stream) {
                    if(n<40) throw std::runtime_error("AVI short BITMAPINFOHEADER");
                    result.width=u32(b,data+4); const auto h=static_cast<std::int32_t>(u32(b,data+8));
                    if (h==std::numeric_limits<std::int32_t>::min()) throw std::runtime_error("AVI invalid height");
                    result.height=static_cast<std::uint32_t>(h<0?-h:h);
                    if(u16(b,data+14)!=24 || !tag(b,data+16,"MV30")) HP2_NOT_IMPLEMENTED("0x00238FB0","unsupported MV30 bitmap format");
                } else if(stream==audio_stream) {
                    if(n<16) throw std::runtime_error("AVI short WAVEFORMAT");
                    if(u16(b,data)!=1) HP2_NOT_IMPLEMENTED("0x00237760","non-PCM movie audio");
                    result.channels=u16(b,data+2);result.sample_rate=u32(b,data+4);result.bits=u16(b,data+14);
                    if((result.bits!=8 && result.bits!=16) || !result.channels || !result.sample_rate) throw std::runtime_error("AVI invalid PCM format");
                }
            } else if(movi && b[p]>='0' && b[p]<='9' && b[p+1]>='0' && b[p+1]<='9') {
                const unsigned index=(b[p]-'0')*10+b[p+1]-'0';
                if(index==video_stream && b[p+2]=='d' && (b[p+3]=='c'||b[p+3]=='b'))
                    result.video_frames.emplace_back(b.begin()+data,b.begin()+data+n);
                else if(index==audio_stream && b[p+2]=='w' && b[p+3]=='b') result.audio_data.insert(result.audio_data.end(),b.begin()+data,b.begin()+data+n);
            }
            p=data+n+(n&1u);
            if(p>end) throw std::runtime_error("AVI missing chunk pad");
        }
    }
};
void avcheck(int code, const char* action) {
    if(code>=0) return;
    char msg[AV_ERROR_MAX_STRING_SIZE]{};av_strerror(code,msg,sizeof(msg));
    throw std::runtime_error(std::string(action)+": "+msg);
}
}
AviMovie read_avi(const std::vector<std::uint8_t>& bytes) {
    if(!tag(bytes,0,"RIFF")||!tag(bytes,8,"AVI ")) throw std::runtime_error("not RIFF AVI");
    const std::uint64_t end=std::uint64_t(u32(bytes,4))+8;
    if(end>bytes.size() || end<12) throw std::runtime_error("AVI invalid RIFF size");
    Reader r{bytes,{}};r.chunks(12,static_cast<std::size_t>(end));
    if(!r.result.width||!r.result.height||!r.result.scale||!r.result.rate||r.result.video_frames.empty()) throw std::runtime_error("AVI missing video metadata/frames");
    return std::move(r.result);
}
struct MovieDecoder::Impl {
    AVCodecContext* context{};AVFrame* frame{};AVPacket* packet{};SwsContext* sws{};
    unsigned width{},height{};
    ~Impl(){sws_freeContext(sws);av_packet_free(&packet);av_frame_free(&frame);avcodec_free_context(&context);}
};
MovieDecoder::MovieDecoder(std::uint32_t width,std::uint32_t height):impl_(std::make_unique<Impl>()) {
    if(!width||!height||width>4096||height>4096) throw std::runtime_error("invalid movie dimensions");
    const auto* codec=avcodec_find_decoder(AV_CODEC_ID_MV30);
    if(!codec) HP2_NOT_IMPLEMENTED("0x00239148","host FFmpeg MV30 decoder missing");
    impl_->context=avcodec_alloc_context3(codec);impl_->frame=av_frame_alloc();impl_->packet=av_packet_alloc();
    if(!impl_->context||!impl_->frame||!impl_->packet) throw std::bad_alloc();
    impl_->width=width;impl_->height=height;impl_->context->width=static_cast<int>(width);impl_->context->height=static_cast<int>(height);
    impl_->context->thread_count=1;avcheck(avcodec_open2(impl_->context,codec,nullptr),"MV30 open");
}
MovieDecoder::~MovieDecoder()=default;
void orient_rgba_for_ps2(std::vector<std::uint8_t>& rgba,std::uint32_t width,std::uint32_t height) {
    const auto stride=static_cast<std::size_t>(width)*4;
    if(!width||!height||stride>std::numeric_limits<std::size_t>::max()/height||rgba.size()!=stride*height)
        throw std::runtime_error("invalid RGBA orientation dimensions");
    for(std::uint32_t y=0;y<height/2;++y) {
        auto top=rgba.begin()+static_cast<std::size_t>(y)*stride;
        auto bottom=rgba.begin()+static_cast<std::size_t>(height-1-y)*stride;
        std::swap_ranges(top,top+stride,bottom);
    }
}
std::vector<std::uint8_t> MovieDecoder::decode_frame(const std::vector<std::uint8_t>& bytes) {
    if(bytes.empty()||bytes.size()>static_cast<std::size_t>(INT_MAX)) throw std::runtime_error("invalid MV30 packet length");
    auto& i=*impl_;av_packet_unref(i.packet);avcheck(av_new_packet(i.packet,static_cast<int>(bytes.size())),"MV30 packet allocation");
    std::memcpy(i.packet->data,bytes.data(),bytes.size());
    avcheck(avcodec_send_packet(i.context,i.packet),"MV30 send");avcheck(avcodec_receive_frame(i.context,i.frame),"MV30 decode");
    if(i.frame->width!=static_cast<int>(i.width)||i.frame->height!=static_cast<int>(i.height)) throw std::runtime_error("MV30 frame changed dimensions");
    i.sws=sws_getCachedContext(i.sws,i.frame->width,i.frame->height,static_cast<AVPixelFormat>(i.frame->format),i.frame->width,i.frame->height,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!i.sws) throw std::runtime_error("MV30 RGBA converter unavailable");
    std::vector<std::uint8_t> result(static_cast<std::size_t>(i.width)*i.height*4);std::uint8_t* dest[]={result.data(),nullptr,nullptr,nullptr};int strides[]={static_cast<int>(i.width*4),0,0,0};
    if(sws_scale(i.sws,i.frame->data,i.frame->linesize,0,i.frame->height,dest,strides)!=i.frame->height) throw std::runtime_error("MV30 incomplete RGBA conversion");
    // Original output header height is negated by 00237AD8, flags bit0 is set
    // by 00239148 and 002363A8 writes from last row using -width*4 stride.
    // Keep FFmpeg prediction buffers untouched; orient only returned host pixels.
    orient_rgba_for_ps2(result,i.width,i.height);
    return result;
}
}

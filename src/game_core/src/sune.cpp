#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace hp2 {
// Native lifetime of the original fifteen zero-initialized static SunZ
// tiles. A deferred GPU transfer may finish after its application closes;
// retaining this allocation does not retain a track or invent completion.
SunResources::SunResources():depthSamples_(std::make_shared<std::array<SunDepthSamples,15>>()) {}

bool SunResources::LoadChunk(const std::vector<std::uint8_t>& owner,const Chunk& chunk)
{
    if (chunk.id!=0x34202u)return false;
    const auto first=(chunk.payloadOffset+15u)&~std::size_t(15u);
    Require(chunk.endOffset<=owner.size() && first<=chunk.endOffset && chunk.endOffset-first>=16,
        "Sun34202 record is truncated");
    const auto* record=owner.data()+first;
    if (ReadU32(record)==2) {
        info_=record;infoSize_=chunk.endOffset-first;
        for (std::size_t axis=0;axis<3;++axis) {
            const auto word=ReadU32(record+4+axis*4);std::memcpy(&vector_[axis],&word,4);
            Require(std::isfinite(vector_[axis]),"Sun vector is not finite");
        }
    } else {info_=nullptr;infoSize_=0;vector_={};}

    // PAL124FB8..125160 initializes the persistent CPU flare state even
    // for an unsupported resource version. No GPU object is manufactured.
    mode_=2;flares_={};
    flares_[0]={0,1,1.0f,60.0f,{0,0},{255,200,100,128},0,0};
    flares_[1]={0,1,0.5f,320.0f,{0,0},{130,159,122,128},0,0};
    flares_[2]={2,1,0.7f,800.0f,{-9.0f,28.0f},{159,122,75,128},0,0.2f};
    flares_[3]={2,1,0.3f,800.0f,{28.0f,9.0f},{166,119,72,128},32768,0.2f};
    return true;
}

void SunResources::Unload() noexcept
{
    // PAL1251A4..1251BC clears the borrowed info and vector, retaining the
    // already initialized flare descriptors in their application owner.
    info_=nullptr;infoSize_=0;vector_={};
}

// PAL1251C0..125290, complete five retained texture lookups followed by
// the source byte29 stores. There is no synthetic descriptor or GPU handle.
void SunResources::LoadTrack(ResourceRegistry& resources)
{
    static constexpr std::uint32_t keys[5]={
        0x47beb4b6u,0xd4d26c59u,0x1744b82du,0xad3c5239u,0xd4d80a65u};
    Require(!trackTextures_[0],"Sun track textures are already borrowed");
    for(std::size_t i=0;i<trackTextures_.size();++i)
        trackTextures_[i]=resources.ResolveTexture(keys[i],true);
    for(auto* descriptor:trackTextures_) {
        auto* texture=textureInfoForDescriptor(descriptor);
        Require(texture && texture->pack,"Sun texture has no retained physical owner");
        texture->pack->SetDescriptorByte29(descriptor,7);
    }
}

// PAL125290..1252B0: texture storage belongs to its original pack.
void SunResources::UnloadTrack() noexcept {trackTextures_={};}
const EditorMarkerTexture* SunResources::TrackTexture(std::size_t index) const
{
    Require(index<trackTextures_.size(),"Sun texture index is outside its source bank");
    return trackTextures_[index];
}
}

namespace hp2 {
namespace {
float SunReal(const std::uint8_t* bytes)
{
    const auto bits=ReadU32(bytes);float result;std::memcpy(&result,&bits,4);return result;
}
SunFlareParameters SunFlare(const std::uint8_t* bytes)
{
    SunFlareParameters row;
    row.kind=static_cast<std::int32_t>(ReadU32(bytes));row.enabled=static_cast<std::int32_t>(ReadU32(bytes+4));
    row.scale=SunReal(bytes+8);row.size=SunReal(bytes+12);
    row.offset={{SunReal(bytes+16),SunReal(bytes+20)}};
    std::copy_n(bytes+24,4,row.color.begin());row.angle=ReadU16(bytes+28);row.parameter20=SunReal(bytes+32);
    return row;
}
struct SunScreenPosition {
    float x{},y{},margin{},widthWithMargin{},pixelScale{1.0f};
    bool visible{};
};
SunScreenPosition ProjectSun(const CarCinematicScreen& view,const float position[4],
    const std::array<SunFlareParameters,6>& flares,const std::array<float,2>& overridePosition)
{
    Require(view.view && view.target && view.matricesCalculated,"Sun projection lacks its actual calculated view");
    float clip[4],camera[4];eMulVector4(clip,view.matrices.worldToClip,position);
    eMulVector3(camera,view.matrices.worldToView,position);
    SunScreenPosition result;
    // The calculated PC projection and the authored CalculateH focal input
    // have different pixel units. Sun34202 sizes/offsets remain source pixels;
    // convert their geometry by the same focal ratio as world particles.
    Require(std::isfinite(view.pixelScale) && view.pixelScale>0.0f &&
        std::isfinite(view.sourcePixelScale) && view.sourcePixelScale>0.0f,
        "Sun projection has no calculated source/native focal scales");
    result.pixelScale=view.pixelScale/view.sourcePixelScale;
    Require(std::isfinite(result.pixelScale) && result.pixelScale>0.0f,
        "Sun source/native pixel ratio is not finite and positive");
    if(!std::isfinite(clip[3]) || clip[3]==0.0f)return result;
    const auto* projection=view.matrices.clipToScreen;
    const float z=clip[2]/clip[3],divisor=projection[11]*z+projection[15];
    if(!std::isfinite(divisor) || divisor==0.0f)return result;
    // Native viewport coordinates use the projected centre directly. The
    // old GS2048 origin and subsequent subtraction were transport only.
    result.x=(clip[0]/clip[3])*(projection[0]/divisor)+static_cast<float>(view.target->width/2);
    result.y=(clip[1]/clip[3])*(projection[5]/divisor)+static_cast<float>(view.target->height/2);
    if(overridePosition[0]!=0.0f || overridePosition[1]!=0.0f) {
        result.x=overridePosition[0];result.y=overridePosition[1];
    }
    for(const auto& row:flares)if(row.kind==0 && result.margin<row.size*result.pixelScale)
        result.margin=row.size*result.pixelScale;
    result.widthWithMargin=static_cast<float>(view.target->width)+result.margin;
    result.visible=!(camera[2]<0.0f || result.x < -result.margin || result.widthWithMargin<result.x ||
        result.y < -result.margin || static_cast<float>(view.target->height)+result.margin<result.y);
    return result;
}
FlailerPoly SunPolygon(const SunFlareParameters& row,const SunScreenPosition& center,
    float halfSize,float opacity,float z)
{
    // PAL125644..1257E8 /125D74..125EF8. The released integer angle and
    // byte alpha are retained, including negative intensity's bypass branch.
    const float fraction=(center.x+center.margin)/(center.widthWithMargin+center.margin);
    const auto angle=static_cast<std::uint16_t>(static_cast<std::uint32_t>(row.angle)+
        static_cast<std::uint32_t>(convertFloatToInt32((row.parameter20*fraction)*65536.0f)));
    halfSize*=center.pixelScale;
    const auto sine=rotationSineAngle(angle),cosine=vectorCosineAngle(angle);
    const auto c=halfSize*cosine,s=halfSize*sine,difference=c-s,sum=s+c;
    const auto x=center.x+row.offset[0]*center.pixelScale;
    const auto y=center.y+row.offset[1]*center.pixelScale;
    const float positions[4][2]={{x-sum,y-difference},{x+difference,y-sum},
        {x+sum,y+difference},{x-difference,y+sum}};
    const float coordinates[4][2]={{0,0},{1,0},{1,1},{0,1}};
    FlailerPoly polygon;
    // Constructor byte97 starts1; original ePoly::Render clears it for
    // byte96=1 (literal ALPHA packet), leaving these target-pixel coordinates.
    polygon.rasterState={{row.enabled ? 0x01000200u:0x01000100u,0x01010140u,0,0}};
    const auto alpha=static_cast<std::uint32_t>(convertFloatToInt32(opacity))&255u;
    const auto rgba=static_cast<std::uint32_t>(row.color[0])|
        (static_cast<std::uint32_t>(row.color[1])<<8)|
        (static_cast<std::uint32_t>(row.color[2])<<16)|(alpha<<24);
    for(std::size_t vertex=0;vertex<4;++vertex) {
        polygon.vertices[vertex].position={{positions[vertex][0],positions[vertex][1],z,0}};
        polygon.vertices[vertex].textureCoordinate={{coordinates[vertex][0],coordinates[vertex][1],1}};
        polygon.vertices[vertex].color=rgba;
    }
    return polygon;
}
}

std::array<float,2> SunResources::AdvanceGlare(const GameTime& clock)
{
    // PAL125918..125F6C source controls and envelope; the native clock
    // adaptation below preserves its duration across presentation rates.
    auto& state=glare_;
    if(state.reset) {state.timer=0;state.start=0;state.reset=0;}
    const auto timerActive=[](std::int32_t timer){return timer!=0 && timer!=INT32_MAX;};
    if(state.start && static_cast<float>(clock.realTimer)*0.0002500000118743628f>0.0f && !timerActive(state.timer)) {
        state.start=0;state.timer=clock.realTimer;state.phase=0;
    }
    if(!timerActive(state.timer))return {};
    static constexpr std::uint32_t durationTicks[]={200,800,2600};
    Require(state.phase>=0 && state.phase<4,"Sun glare phase is outside the original four-phase table");
    auto elapsedTicks=static_cast<std::uint32_t>(clock.realTimer)-
        static_cast<std::uint32_t>(state.timer);
    // PC presentation can cross several phases in one frame. Keep the
    // released 50/200/650ms envelope on the clock, retaining the remainder
    // instead of stretching it by resetting each phase to the draw time.
    while(state.phase<3 && elapsedTicks>=durationTicks[state.phase]) {
        const auto duration=durationTicks[state.phase];
        elapsedTicks-=duration;
        const auto next=static_cast<std::uint32_t>(state.timer)+duration;
        std::memcpy(&state.timer,&next,sizeof(next));
        ++state.phase;
    }
    float size=0.0f,fade=1.0f;
    if(state.phase==0)size=static_cast<float>(elapsedTicks)/static_cast<float>(durationTicks[0]);
    else if(state.phase==1)size=1.0f;
    else if(state.phase==2) {size=1.0f;fade=1.0f-static_cast<float>(elapsedTicks)/static_cast<float>(durationTicks[2]);}
    else {state.timer=0;size=0.0f;fade=0.0f;}
    return {size*1.899999976158142f,fade};
}

std::array<SunFlareParameters,6> SunResources::AuthoredFlares() const
{
    std::array<SunFlareParameters,6> authored{};
    if (!info_) return authored;
    Require(infoSize_>=0x1cu+6u*0x24u,"Sun34202 flare bank is truncated");
    for(std::size_t index=0;index<authored.size();++index) authored[index]=SunFlare(info_+0x1c+index*0x24);
    return authored;
}

void SunResources::StuffSunGlare(const CarCinematicScreen& view,EcstasyFrame& frame,const GameTime& clock)
{
    const auto [size,fade]=AdvanceGlare(clock);
    if (size==0.0f) return;
    const auto& state=glare_;
    Require(view.view,"Sun glare has no original camera");
    float point[4];
    for(std::size_t axis=0;axis<3;++axis)
        point[axis]=view.view->current.forward[axis]*state.distance+view.view->current.eye[axis];
    point[3]=1;
    const auto projected=ProjectSun(view,point,flares_,state.positionOverride);
    if(!projected.visible)return;
    const float maximum=view.target->format==0x31 ? 16777215.0f:65535.0f;
    for(const auto& row:flares_) {
        const auto* descriptor=TrackTexture(static_cast<std::size_t>(row.kind));
        if(!descriptor || size==0.0f)continue;
        const float opacity=size<0.0f ? (-size)*128.0f:fade*(size*128.0f);
        frame.RenderScreenPolygon(SunPolygon(row,projected,(row.size*0.5f)*size,opacity,maximum),
            textureInfoForDescriptor(descriptor),view);
    }
}

void SunResources::StuffSun(const CarCinematicScreen& view,EcstasyFrame& frame,
    const GameTime& clock,bool inGame)
{
    // Complete PAL1252B0..125914: glare first; ordinary authored sunlight
    // belongs to primary view1 only. Z samples persist across resource loads.
    StuffSunGlare(view,frame,clock);
    if(!info_ || view.id!=1)return;
    Require(view.target && view.target->id>=0 && view.target->id<15,
        "Sun depth owner is outside the original15-target array");
    const auto authored=AuthoredFlares();
    const float position[4]={vector_[0],vector_[1],vector_[2],1};
    const auto projected=ProjectSun(view,position,authored,glare_.positionOverride);
    if(!projected.visible)return;
    const auto targetIndex=static_cast<std::size_t>(view.target->id);
    auto& samples=DepthSamples(targetIndex);
    float visible=0.0f;
    for(auto y=samples.bounds[2];y<samples.bounds[3];++y)
        for(auto x=samples.bounds[0];x<samples.bounds[1];++x) {
            Require(x>=0 && x<32 && y>=0 && y<32,"Sun retained sample bounds leave their32-square tile");
            if((samples.values[static_cast<std::size_t>(y*32+x)]&0x00ffffffu)<2u)visible+=1.0f;
        }
    visible*=0.0009765625f;
    const float maximum=view.target->format==0x31 ? 16777215.0f:65535.0f;
    for(std::size_t index=0;index<authored.size();++index) {
        const auto& row=authored[index];const auto* descriptor=TrackTexture(static_cast<std::size_t>(row.kind));
        if(!descriptor || row.scale==0.0f)continue;
        const float opacity=row.scale<0.0f ? (-row.scale)*128.0f:visible*(row.scale*128.0f);
        frame.RenderScreenPolygon(SunPolygon(row,projected,row.size*0.5f,opacity,index ? maximum:1.0f),
            textureInfoForDescriptor(descriptor),view);
    }
    // PAL125808..1258B0 clamps the transfer rectangle while retaining its
    // original valid subrectangle. Pixels outside it never count as clear.
    // Sample the same angular footprint as the authored sprite geometry.
    // Increasing the PC resolution must not shrink this occlusion window
    // to32 native pixels; only its1024 retained output samples stay fixed.
    const auto step=projected.pixelScale;
    const auto gridWidth=std::floor(static_cast<float>(view.target->width)/step);
    const auto gridHeight=std::floor(static_cast<float>(view.target->height)/step);
    Require(std::isfinite(gridWidth) && std::isfinite(gridHeight) &&
        static_cast<double>(gridWidth)<=INT32_MAX && static_cast<double>(gridHeight)<=INT32_MAX,
        "Sun source sample grid exceeds its native coordinate range");
    const auto sourceWidth=static_cast<std::int32_t>(gridWidth);
    const auto sourceHeight=static_cast<std::int32_t>(gridHeight);
    Require(sourceWidth>=33 && sourceHeight>=33,"Sun target is smaller than its source transfer tile");
    auto x=convertFloatToInt32(projected.x/step-16.0f),y=convertFloatToInt32(projected.y/step-16.0f);
    std::int32_t left=0,right=32,top=0,bottom=32;
    if(x<0) {right=x+32;x=0;}
    if(sourceWidth-33<x) {left=(x+1)-(sourceWidth-32);x=sourceWidth-33;}
    if(y<0) {bottom=y+32;y=0;}
    if(sourceHeight-33<y) {top=(y+1)-(sourceHeight-32);y=sourceHeight-33;}
    samples.bounds={{left,right,top,bottom}};
    if(inGame)frame.TransferZBufferRect(RetainDepthSamples(targetIndex),view,x,y,32,32,step);
}
}

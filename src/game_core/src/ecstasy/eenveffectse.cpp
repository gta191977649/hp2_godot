#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace hp2 {
EcstasyFogSettings fogSettings{};
EcstasyFocusSettings focusSettings{};

void EcstasyFrame::AttachBlackAndWhite(const CarCinematicServices& views)
{
    Require(phase_==Phase::Building,"Cannot attach colour feedback to a sealed frame");
    const bool forced=nativeRenderPolicy.blackAndWhite!=0;
    // Complete PAL11A9F0..11B124: raw requests select the early no-op,
    // then forced mode bypasses all three individual active/request gates.
    if(!forced && !views.screens[1].blackAndWhite && !views.screens[2].blackAndWhite)return;
    AttachAlphaPolys();
    const auto group=++nextFeedbackGroup_;
    Require(group!=0,"Native colour-feedback group count overflowed");
    for(std::size_t index=1;index<3;++index) {
        const auto& view=views.screens[index];
        if(!forced && (!view.enabled || !view.target || !view.target->enabled || !view.blackAndWhite))continue;
        Require(view.target && view.target->id>=0 && view.target->id<7 &&
            view.target->width>=0 && view.target->height>=0,"Black-and-white view lost its original display rectangle");
        // Native single-player layouts give view2 a zero-area rectangle.
        // PAL emits a degenerate sprite there; it has no pixels to replace.
        if(!view.target->width || !view.target->height)continue;
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::BlackAndWhitePass;
        command.view=&view;command.target=view.target;command.zWriteEnabled=false;
        command.feedbackGroup=group;commands_.push_back(command);
    }
    // Keep image passes outside all later texture-priority/alpha sorting.
    pendingAlphaBegin_=commands_.size();
}

namespace {
// PAL119B70..119BD4 selects the same 256 depth samples used by the
// projection. The PC projection stores numeric depth units directly;
// constructing a floating-point value from a GS address is unnecessary.
float MapPaletteIndexToScreenZ(std::int32_t index,const EcstasyViewProjectionSettings& projection)
{
    return projection.depthRanges[1][0]+static_cast<float>(index)*256.0f;
}
float MapScreenZToViewZ(float nearClip,float farClip,float screenZ,const EcstasyViewProjectionSettings& projection)
{
    const float low=projection.depthRanges[1][0],high=projection.depthRanges[1][1];
    const float span=farClip-nearClip;
    const float product=farClip*nearClip;
    const float first=-high*nearClip;
    const float second=low*farClip;
    const float slope=(first+second)/span;
    const float offset=product*(high-low)/span;
    return offset/(screenZ-slope);
}
}

// PAL119C20..119F6C zero-focus gate, depth order and fade transitions.
// Native numeric depth and linear palette storage replace GS transport.
bool CreateFocusPalette(const CarCinematicScreen* view,const EcstasyViewProjectionSettings& projection,
    std::array<std::uint32_t,256>* output)
{
    if(!view || !output)return false;
    Require(view->view,"Focus palette lost its original camera");
    std::array<float,256> distances{};
    for(std::int32_t index=0;index<256;++index)
        distances[255-index]=MapScreenZToViewZ(view->nearClip,view->farClip,
            MapPaletteIndexToScreenZ(index,projection),projection);
    const float center=focusSettings.focalDistance!=0.0f ? focusSettings.focalDistance:
        view->view->current.minimumVisibility;
    const float range=focusSettings.depthOfField!=0.0f ? focusSettings.depthOfField:
        view->view->current.maximumVisibility;
    if(center==0.0f && range==0.0f) {output->fill(0);return false;}
    const float alphaMin=focusSettings.alphaMin,alphaMax=focusSettings.alphaMax;
    const float alphaSpan=alphaMax-alphaMin;
    const float half=range*0.5f,near= center-half,far=center+half;
    float previous=-std::numeric_limits<float>::max();
    std::int32_t first=0,last=255;
    for(std::int32_t index=0;index<256;++index) {
        const float current=distances[index];
        if(previous<near && near<=current)first=index;
        if(previous<far && far<=current)last=index;
        previous=current;
    }
    const auto color=[](float alpha) {
        return 0x00808080u|(static_cast<std::uint32_t>(convertFloatToInt32(alpha))<<24);
    };
    const auto inside=color(alphaMin),outside=color(alphaMax);
    std::array<std::uint32_t,256> linear{};
    for(std::int32_t index=0;index<256;++index)linear[255-index]=index<first || last<index ? outside:inside;
    const auto slots=focusSettings.transitionSlots;
    for(std::int32_t index=1;index<slots;++index) {
        const auto position=first-index;
        if(static_cast<std::uint32_t>(position)<256u)
            linear[255-position]=color(alphaMin+(static_cast<float>(index)/static_cast<float>(slots))*alphaSpan);
    }
    for(std::int32_t index=1;index<slots;++index) {
        const auto position=last+index;
        if(static_cast<std::uint32_t>(position)<256u)
            linear[255-position]=color(alphaMin+(static_cast<float>(index)/static_cast<float>(slots))*alphaSpan);
    }
    // PAL's diagnostic loop only evaluates the same pure conversion; it
    // has no print call or output. Its one observable side effect is reset.
    focusSettings.printPalette=0;
    *output=linear;return true;
}

// The authored fog curve is a power of the normalized depth. On native PC
// calculate this using the hardware FPU instead of the PS2 integer-bit
// approximation of IEEE754 words. Logical depth order and original weather
// constants stay unchanged; the generated palette is linear PC data.
void CreateFogPalette(const WeatherQuery& weather,std::int32_t mode,float power,
    std::array<std::uint32_t,256>& output)
{
    std::array<std::uint32_t,256> linear{};
    const auto rgb=weather.fogColor&0x00ffffffu;
    const float alphaScale=weather.fogStrength*128.0f;
    if(mode==2) {
        for(std::uint32_t index=0;index<256;++index)
            linear[index]=0x80000000u|(index<<16)|(index<<8)|index;
    } else if(mode==3) {
        const auto index=convertFloatToInt32(weather.fogDistance);
        Require(index>=0 && index<256,"Fog palette marker exceeds its original 256 entries");
        linear[static_cast<std::size_t>(index)]=rgb|
            (static_cast<std::uint32_t>(convertFloatToInt32(alphaScale))<<24);
    } else {
        for(std::size_t index=0;index<256;++index) {
            const float depth=static_cast<float>(index);
            float amount=depth<weather.fogDistance ? 0.0f:
                depth<255.0f ? (depth-weather.fogDistance)/(255.0f-weather.fogDistance):1.0f;
            std::uint32_t color=rgb;
            if(mode==1) {amount=1.0f-amount;color=0;}
            else {
                amount=std::pow(std::max(amount,0.009999999776482582f),power);
            }
            const auto alpha=static_cast<std::uint32_t>(convertFloatToInt32(amount*alphaScale));
            linear[255-index]=color|(alpha<<24);
        }
    }
    output=linear;
}
void EcstasyFrame::AttachFog(const CarCinematicServices& views)
{
    Require(phase_==Phase::Building,"Cannot attach fog to a sealed graphics frame");
    if(!fogSettings.enabled)return;
    // This is a source render-stage boundary. The generated fog packet is
    // never inserted into texture-priority or world alpha sorting.
    AttachAlphaPolys();
    static constexpr std::uint32_t blend[4]={0x01000100u,0x02000001u,0x01000100u,0x01000100u};
    for(std::size_t index=0;index<2;++index) {
        const auto& view=views.screens[index+1];
        if(!view.enabled || !view.target || !view.target->enabled)continue;
        Require(view.view && view.matricesCalculated && view.id==static_cast<std::int32_t>(index+1),
            "Fog view lost its original camera");
        const auto mode=fogSettings.modes[index];
        Require(mode>=0 && mode<4,"Fog mode exceeds the original four contexts");
        WeatherQuery weather{};
        Require(weatherManQuery(&weather,view.view->current.eye)==WEATHER_MAN_OK,
            "Fog query lost its live weather owner");
        auto& fog=fogPasses_[index];CreateFogPalette(weather,mode,fogSettings.power,fog.palette);
        fog.blend=blend[mode];fog.blendConstant=1.0f;
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::FogPass;
        command.view=&view;command.target=view.target;command.depthPalette=&fog;command.zWriteEnabled=false;
        commands_.push_back(command);
    }
    pendingAlphaBegin_=commands_.size();
}

void EcstasyFrame::AttachDepthOfField(const CarCinematicServices& views)
{
    Require(phase_==Phase::Building,"Cannot attach focus to a sealed graphics frame");
    if(!focusSettings.enabled)return;
    bool active=false;const CarCinematicScreen* firstView=nullptr;
    for(std::size_t index=0;index<2;++index) {
        const auto& view=views.screens[index+1];
        if(!view.enabled || !view.target || !view.target->enabled)continue;
        Require(view.view && view.matricesCalculated && view.id==static_cast<std::int32_t>(index+1),
            "Focus view lost its original camera");
        if(!firstView)firstView=&view;
        active=CreateFocusPalette(&view,views.projection,&focusPasses_[index].palette) || active;
    }
    // PAL119F70 builds both active-view palettes, then skips every image
    // operation only if neither palette requests focus. A sharp second
    // view still writes its zero mask when the other view requests blur.
    if(!active)return;
    focusSettings_=focusSettings;
    AttachAlphaPolys();
    const auto blend=[](const std::array<std::int32_t,5>& selectors) {
        std::uint32_t value=0;
        for(std::size_t i=0;i<4;++i) {
            Require(selectors[i]>=0 && selectors[i]<=2,"Focus blend selector exceeds original ALPHA domain");
            value|=static_cast<std::uint32_t>(selectors[i])<<(8*i);
        }
        return value;
    };
    for(std::size_t index=0;index<2;++index) {
        const auto& view=views.screens[index+1];
        if(!view.enabled || !view.target || !view.target->enabled)continue;
        auto& mask=focusPasses_[index];mask.blend=blend(focusSettings_.maskBlend);
        mask.blendConstant=static_cast<float>(focusSettings_.maskBlend[4])/128.0f;
        EcstasyDrawCommand command;command.kind=EcstasyDrawKind::FocusMaskPass;
        command.view=&view;command.target=view.target;command.depthPalette=&mask;command.zWriteEnabled=false;
        commands_.push_back(command);
    }
    EcstasyDrawCommand command;command.kind=EcstasyDrawKind::DepthOfFieldPass;
    command.view=firstView;command.target=firstView->target;command.focus=&focusSettings_;
    command.rasterState[0]=blend(focusSettings_.compositeBlend);
    command.zWriteEnabled=false;commands_.push_back(command);
    pendingAlphaBegin_=commands_.size();
}


}

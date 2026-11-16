#include "FEAnim.hpp"
#include "FMenus.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "Language.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>

namespace hp2 {

void FlailerHighlightAnimation::ForceStart()
{
    // Complete PAL248AD0. Timer belongs to the one live UI clock.
    startedTicks=static_cast<std::uint32_t>(ui_->clock->realTimer);
}

void FlailerHighlightAnimation::HandleEvents(ScreenMessage message,ScreenCallbackData& data)
{
    if (message==ScreenMessage::Tick)Animate();
    else if (message==ScreenMessage::Highlight)SetCurrentMenuItem(data.key);
}

namespace {
float BlurbObjectLeft(FlailerUiState& ui,std::uint32_t screen,std::uint32_t key)
{
    Require(ui.resources && ui.clock,"Animated blurb has no actual resources/clock");
    const auto* object=ui.resources->FindObject(screen,key);
    Require(object,"Animated blurb text object is absent from its authored screen");
    float x,y;ui.resources->GetTopLeft(*object,x,y,ui.language);
    return x;
}

float BlurbElapsed(std::int32_t now,std::int32_t then)
{
    const auto raw=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(then);
    std::int32_t elapsed;std::memcpy(&elapsed,&raw,sizeof(elapsed));
    return static_cast<float>(elapsed)*0.0002500000118743628f;
}
}

FlailerAnimatedBlurb::FlailerAnimatedBlurb(FlailerUiState& ui,std::uint32_t screen,
    std::uint32_t text,std::uint32_t firstAlpha,std::uint32_t secondAlpha)
    : ui_(ui),screenKey_(screen),textKey_(text),
      slide_(ui,screen,0,text,650.0f,BlurbObjectLeft(ui,screen,text),0.25f),
      firstAlpha_(ui,screen),secondAlpha_(ui,screen),destination_(slide_.position.desiredValue)
{
    // PAL2491B0 constructs two single-object alpha owners. The existing
    // native alpha owner has retained multi-object storage; use one entry.
    firstAlpha_.AddObject(firstAlpha);
    secondAlpha_.AddObject(secondAlpha);
}

FlailerAnimatedBlurb::~FlailerAnimatedBlurb()=default;

void FlailerAnimatedBlurb::Animate()
{
    if(ui_.externalPresentation) {
        if(!disabled_ && !textInstalled_) {
            ui_.resources->SetLanguageID(screenKey_,textKey_,0);
            ui_.resources->Printf(screenKey_,textKey_,"%s",text_.data());
            textInstalled_=true;
        }
        return;
    }
    // PAL249348 includes the final-only disabled gate, absent from beta.
    if (disabled_)return;
    const float fadeElapsed=BlurbElapsed(ui_.clock->realTimer,fadeStartedTicks_);
    const float changedElapsed=BlurbElapsed(ui_.clock->realTimer,textChangedTicks_);
    if (phase_==1) {
        const float duration=ui_.animationSettings.blurbFadeDuration;
        const float alpha=fadeElapsed<duration ? fadeElapsed/duration : 1.0f;
        firstAlpha_.SetAlphaScale(1.0f-alpha);
        secondAlpha_.SetAlphaScale(alpha);
        if (ui_.animationSettings.blurbTextDelay<changedElapsed) {
            textInstalled_=false;
            phase_=2;
        }
    }
    if (phase_!=2 && phase_!=0)return;
    firstAlpha_.SetAlphaScale(1.0f);
    secondAlpha_.SetAlphaScale(0.0f);
    if (!textInstalled_) {
        ui_.resources->SetLanguageID(screenKey_,textKey_,0);
        ui_.resources->Printf(screenKey_,textKey_,"%s",text_.data());
        slide_.SetupToAnimate(650.0f,destination_);
        textInstalled_=true;
    } else if (slide_.position.mode!=0) {
        slide_.Animate();
    } else {
        phase_=3;
    }
}

// Final-only PAL2492F0..249348, called by the final campaign bonus view.
void FlailerAnimatedBlurb::Hide()
{
    NewText("");firstAlpha_.SetAlphaScale(0.0f);secondAlpha_.SetAlphaScale(0.0f);disabled_=true;
}

void FlailerAnimatedBlurb::NewText(std::int32_t languageId)
{
    Require(ui_.language,"Animated blurb has no actual Language manager");
    NewText(ui_.language->GetString(languageId));
}

void FlailerAnimatedBlurb::NewText(const char* text)
{
    Require(text,"Animated blurb text is null");
    const auto length=std::strlen(text);
    Require(length<text_.size(),"Animated blurb exceeds its original 256-byte text extent");
    if(ui_.externalPresentation)textInstalled_=false;
    if (phase_==2 || phase_==3) {
        phase_=1;
        fadeStartedTicks_=ui_.clock->realTimer;
        textInstalled_=false;
    }
    textChangedTicks_=ui_.clock->realTimer;
    std::memcpy(text_.data(),text,length+1);
    disabled_=false;
}

FlailerSlideAnimation::FlailerSlideAnimation(FlailerUiState& ui,std::uint32_t screen)
    : ui_(ui),screen_(screen)
{
    Require(ui_.resources && ui_.clock,"Menu slide has no actual resources/real-time clock");
}

FlailerSlideAnimation::~FlailerSlideAnimation()
{
    // The original list releases from its head. Each group restores the
    // actual resource coordinates before its retained node is discarded.
    for (auto& group:groups_)group.reset();
}

void FlailerSlideAnimation::AddGroup(std::uint32_t group,std::uint32_t anchor,
    float initial,float desired,float duration)
{
    if(ui_.externalPresentation)return;
    if (maximumDuration_<duration)maximumDuration_=duration;
    groups_.push_back(std::make_unique<FlailerAnimationGroup>(ui_,screen_,group,anchor,initial,desired,duration));
}

bool FlailerSlideAnimation::HasControl() const noexcept
{
    return startedTicks_!=0 && startedTicks_!=std::numeric_limits<std::int32_t>::max();
}

void FlailerSlideAnimation::Start()
{
    startedTicks_=ui_.clock->realTimer;
}

bool FlailerSlideAnimation::IsFinished() const
{
    if (!HasControl())return false;
    bool finished=true;
    for (const auto& group:groups_)finished=(group->position.mode==0) && finished;
    return finished;
}

void FlailerSlideAnimation::Animate()
{
    if (IsFinished())return;
    for (auto& group:groups_)group->Animate();
}

float FlailerSlideAnimation::GetFractionCompleted() const
{
    if (!HasControl())return 0.0f;
    if (IsFinished())return 1.0f;
    const auto raw=static_cast<std::uint32_t>(ui_.clock->realTimer)-static_cast<std::uint32_t>(startedTicks_);
    std::int32_t ticks;std::memcpy(&ticks,&raw,sizeof(ticks));
    return finiteRatio(static_cast<float>(ticks)*0.0002500000118743628f,maximumDuration_);
}

namespace {
std::uint32_t MenuSlideKey(const char* format,std::int32_t index)
{
    char name[32];const auto count=std::snprintf(name,sizeof(name),format,index);
    Require(count>=0 && static_cast<std::size_t>(count)<sizeof(name),"Menu slide key exceeds source scratch extent");
    return hashString(name);
}
}

FlailerSlideInAnimation::FlailerSlideInAnimation(FlailerUiState& ui,std::uint32_t screen,float destination)
    : FlailerSlideAnimation(ui,screen)
{
    const auto& settings=ui.animationSettings;
    for (std::int32_t index=0;index<16;++index) {
        const auto group=MenuSlideKey("MENUITEM_%d_GROUP",index);
        const auto anchor=MenuSlideKey("MI_%d_MASK",index);
        if (ui.resources->FindObjectByGroup(screen,group,0)) {
            const float item=static_cast<float>(index);
            AddGroup(group,anchor,item*settings.itemSlideSpacing+640.0f,destination,
                settings.itemSlideDuration+item*settings.itemSlideDelay);
        }
    }
    Start();
}

FlailerSlideOutAnimation::FlailerSlideOutAnimation(FlailerUiState& ui,std::uint32_t screen)
    : FlailerSlideAnimation(ui,screen)
{
    const auto& settings=ui.animationSettings;
    for (std::int32_t index=0;index<16;++index) {
        const auto group=MenuSlideKey("MENUITEM_%d_GROUP",index);
        const auto anchor=MenuSlideKey("MI_%d_MASK",index);
        if (ui.resources->FindObjectByGroup(screen,group,0)) {
            float x=0.0f,y=0.0f;
            if (const auto* object=ui.resources->FindObject(screen,anchor))
                ui.resources->GetTopLeft(*object,x,y,ui.language);
            const float item=static_cast<float>(index);
            AddGroup(group,anchor,x,settings.itemSlideExitPosition,
                settings.itemSlideDuration+item*settings.itemSlideDelay);
        }
    }
}

FlailerExitScreenAnimation::FlailerExitScreenAnimation(FlailerUiState& ui,FlailerScreenList& screens,
    std::uint32_t screen,const char* destination,bool push)
    : FlailerSlideOutAnimation(ui,screen),screens_(screens),destination_(destination?destination:""),
      hasDestination_(destination!=nullptr),push_(push)
{
    Start();
}

void FlailerExitScreenAnimation::HandleMessages(ScreenMessage message,ScreenCallbackData& data)
{
    data.sound=0;
    if (message==ScreenMessage::LeaveScreen) {data.value=0;return;}
    if (message==ScreenMessage::Previous || message==ScreenMessage::Next) {data.value=1;return;}
    if (message==ScreenMessage::Tick) {
        Animate();
        if (IsFinished())screens_.ChangeToScreen(hasDestination_?destination_.c_str():nullptr,push_);
    }
}

FlailerAlphaAnimation::FlailerAlphaAnimation(FlailerUiState& state,std::uint32_t screen)
    : ui_(&state),screenKey_(screen)
{
}

// PAL248360..24842C visits every actual object in the supplied group.
// Duplicate object keys inside a group are valid; retain each object itself.
FlailerAlphaAnimation::FlailerAlphaAnimation(FlailerUiState& state,std::uint32_t screen,std::uint32_t group)
    : FlailerAlphaAnimation(state,screen)
{
    Require(ui_->resources,"Alpha group has no actual Flailer resource owner");
    for (auto* object=ui_->resources->FindObjectByGroup(screenKey_,group,0);object;
         object=ui_->resources->FindObjectByGroup(screenKey_,group,0,object)) {
        Require(object->kind==1 || object->kind==2,"Alpha group requires actual text or bitmap objects");
        SavedAlpha saved{object,{}};
        for (std::size_t corner=0;corner<4;++corner)
            saved.alpha[corner]=static_cast<float>(object->kind==2 ?
                static_cast<FlailerBitmapChunk*>(object)->GetAlpha(static_cast<std::int32_t>(corner)) :
                static_cast<FlailerTextChunk*>(object)->GetAlpha());
        objects_.push_back(saved);
    }
}

void FlailerAlphaAnimation::AddObject(std::uint32_t key)
{
    auto* object=ui_->resources->FindObject(screenKey_,key);
    if (!object)return;
    Require(object->kind==1 || object->kind==2,"Alpha animation requires an actual text or bitmap object");
    SavedAlpha saved{object,{}};
    // Source248028 snapshots all four bitmap corners; text repeats its
    // single alpha four times. Storage is native lifetime, not a PS2 node.
    for (std::size_t corner=0;corner<4;++corner)
        saved.alpha[corner]=static_cast<float>(object->kind==2 ?
            static_cast<FlailerBitmapChunk*>(object)->GetAlpha(static_cast<std::int32_t>(corner)) :
            static_cast<FlailerTextChunk*>(object)->GetAlpha());
    objects_.push_back(saved);
}

void FlailerAlphaAnimation::SetAlphaScale(float scale)
{
    for (const auto& saved:objects_) {
        if (saved.object->kind==2) {
            for (std::size_t corner=0;corner<4;++corner)
                static_cast<FlailerBitmapChunk*>(saved.object)->SetAlpha(
                    convertFloatToInt32(saved.alpha[corner]*scale),static_cast<std::int32_t>(corner));
        } else saved.object->SetAlpha(convertFloatToInt32(saved.alpha[0]*scale));
    }
}

FlailerAlphaAnimation::~FlailerAlphaAnimation()
{
    // Source24AA30 removes each node in insertion order;2482A8 restores
    // its captured alpha before releasing it. No final scale is assumed.
    for (const auto& saved:objects_) {
        if (saved.object->kind==2) {
            for (std::size_t corner=0;corner<4;++corner)
                static_cast<FlailerBitmapChunk*>(saved.object)->SetAlpha(
                    convertFloatToInt32(saved.alpha[corner]),static_cast<std::int32_t>(corner));
        } else saved.object->SetAlpha(convertFloatToInt32(saved.alpha[0]));
    }
}

FlailerAnimationGroup::FlailerAnimationGroup(FlailerUiState& state,std::uint32_t screen,
    std::uint32_t group,std::uint32_t anchor,float initial,float desired,float duration)
    : ui(&state),screenKey(screen),groupKey(group),anchorKey(anchor)
{
    Require(ui->resources,"Animation group has no actual Flailer resource owner");
    tCubic1DInitialize(&position);
    position.continuous=1;
    auto* object=ui->resources->FindObject(screenKey,anchorKey);
    Require(object,"Animation group anchor is absent from the actual screen");
    float x,y;ui->resources->GetTopLeft(*object,x,y,ui->language);
    savedX=convertFloatToInt32(x);savedY=convertFloatToInt32(y);
    position.duration=duration;
    SetupToAnimate(initial,desired);
}

FlailerAnimationGroup::~FlailerAnimationGroup()
{
    if (groupKey)ui->resources->MoveGroupTo(screenKey,groupKey,anchorKey,
        static_cast<float>(savedX),static_cast<float>(savedY));
    else ui->resources->SetTopLeftPosition(screenKey,anchorKey,savedX,savedY);
    // The original's final base deleting-destructor transport is replaced
    // by native object lifetime. The real resource restoration above remains.
}

void FlailerAnimationGroup::SetupToAnimate(float initial,float desired)
{
    if(ui->externalPresentation) {
        position.value=position.desiredValue=desired;
        position.mode=0;
        return;
    }
    position.desiredValue=initial;position.continuous=0;
    if (initial!=position.value)position.mode=2;
    position.mode=0;
    position.value=position.desiredValue;
    position.derivative=position.desiredDerivative;
    position.desiredValue=desired;
    if (desired!=position.value)position.mode=2;
    const auto x=convertFloatToInt32(position.value);
    if (groupKey)ui->resources->MoveGroupTo(screenKey,groupKey,anchorKey,
        static_cast<float>(x),static_cast<float>(savedY));
    else ui->resources->SetTopLeftPosition(screenKey,anchorKey,x,savedY);
}

void FlailerAnimationGroup::Animate()
{
    if(ui->externalPresentation)return;
    Require(ui->clock,"Animation group has no live real-time clock");
    tCubic1DUpdate(&position,ui->clock->realTimeElapsed,0.0f,0.0f);
    const auto x=convertFloatToInt32(position.value);
    if (groupKey)ui->resources->MoveGroupTo(screenKey,groupKey,anchorKey,
        static_cast<float>(x),static_cast<float>(savedY));
    else ui->resources->SetTopLeftPosition(screenKey,anchorKey,x,savedY);
}

void FlailerResources::SetBitmapCenter(FlailerBitmapChunk& bitmap,float x,float y) const
{
    bitmap.SetTopLeftX(x-ScaledBitmapWidth(bitmap)*0.5f);
    bitmap.SetTopLeftY(y-ScaledBitmapHeight(bitmap)*0.5f);
}

FlailerHighlightAnimation::FlailerHighlightAnimation(FlailerUiState& ui,std::uint32_t screen,
    std::uint32_t echo,std::int32_t tint)
    : screenKey(screen),startedTicks(static_cast<std::uint32_t>(ui.clock->realTimer)),echoKey(echo),
      color(tint),startScale(ui.animationSettings.highlightStartScale),
      peakScale(ui.animationSettings.highlightPeakScale),restartDuration(ui.animationSettings.highlightRestart),ui_(&ui)
{
}

void FlailerHighlightAnimation::SetCurrentMenuItem(std::uint32_t key)
{
    targetKey=key;startedTicks=static_cast<std::uint32_t>(ui_->clock->realTimer);
    float scale=1.0f;
    if (auto* object=ui_->resources->FindObject(screenKey,key)) scale=object->scaleX;
    startScale=scale*ui_->animationSettings.highlightStartScale;
    scale=1.0f;
    if (auto* object=ui_->resources->FindObject(screenKey,key)) scale=object->scaleX;
    peakScale=scale*ui_->animationSettings.highlightPeakScale;
}

void FlailerHighlightAnimation::Animate()
{
    if(ui_->externalPresentation) {
        const auto& settings=ui_->animationSettings;
        const auto* target=static_cast<const FlailerTextChunk*>(ui_->resources->FindObject(screenKey,targetKey,1));
        auto* echo=static_cast<FlailerTextChunk*>(ui_->resources->FindObject(screenKey,echoKey,1));
        if(!target || !echo || target==echo || target->visible!=1 || startScale<=0.0f ||
            settings.highlightStartAlpha<=0.0f || settings.highlightDuration<=0.0f)return;
        echo->CopyAttributesFrom(*target);
        if(!target->languageId)echo->SetString(target->text);
        ui_->resources->ScaleTextAboutCenter(*echo,startScale,ui_->language);
        echo->SetAlpha(convertFloatToInt32(settings.highlightStartAlpha));
        if(color!=-1)echo->SetColor(static_cast<std::uint32_t>(color));
        echo->visible=0;
        // Publish the start pose directly. It is excluded from the normal
        // screen list and never enters the Ecstasy queues.
        echo->DeleteFlailerStrip();
        echo->Render(*ui_->resources,ui_->language,nullptr,nullptr,1.0f);
        if(echo->strip) {
            auto& highlights=ui_->render->highlights;
            highlights.push_back({echo->strip,screenKey,echoKey,targetKey,startedTicks,peakScale/startScale,
                settings.highlightDuration,restartDuration,settings.highlightEndAlpha/settings.highlightStartAlpha});
            try {echo->strip->pool->Retain(*echo->strip);}
            catch(...) {highlights.pop_back();throw;}
        }
        return;
    }
    const auto now=static_cast<std::uint32_t>(ui_->clock->realTimer);
    const auto raw=now-startedTicks;
    std::int32_t ticks;std::memcpy(&ticks,&raw,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    const auto& settings=ui_->animationSettings;
    if (elapsed<settings.highlightDuration) {
        const float angle=(elapsed*1.5707499980926514f)/settings.highlightDuration;
        const float scale=startScale+(peakScale-startScale)*rotationSineRadians(angle);
        const float alpha=settings.highlightStartAlpha+
            (settings.highlightEndAlpha-settings.highlightStartAlpha)*rotationSineRadians(angle);
        auto* target=static_cast<FlailerTextChunk*>(ui_->resources->FindObject(screenKey,targetKey,1));
        auto* echo=static_cast<FlailerTextChunk*>(ui_->resources->FindObject(screenKey,echoKey,1));
        if (target && echo) {
            echo->CopyAttributesFrom(*target);
            // Authored labels resolve their language ID at render time.
            // Literal labels need their complete string, including the
            // terminator, beyond the copied inline attribute prefix.
            if (target->languageId==0) echo->SetString(target->text);
            ui_->resources->ScaleTextAboutCenter(*echo,scale,ui_->language);
            echo->SetAlpha(convertFloatToInt32(alpha));
            if (color!=-1) echo->SetColor(static_cast<std::uint32_t>(color));
        } else if (echo) echo->visible=0;
    }
    if (restartDuration<elapsed) startedTicks=now;
}

float FlailerAnimationRandomRange(float minimum,float maximum)
{
    const float value=static_cast<float>(vectorDefaultRandomBounded(10000));
    return minimum+(value*0.00009999999747378752f)*(maximum-minimum);
}

FlailerGlobalBackgroundAnimation::FlailerGlobalBackgroundAnimation(FlailerUiState& ui):ui_(&ui)
{
    for (auto& axis:position.axes) {
        tCubic1DInitialize(&axis);axis.duration=ui.animationSettings.backgroundDuration;
        axis.threshold=1.0f;axis.continuous=0;
    }
    position.axes[0].value=position.axes[0].desiredValue=320.0f;
    position.axes[1].value=position.axes[1].desiredValue=240.0f;
    position.axes[0].mode=position.axes[1].mode=0;
    DefaultTextureList();
}

void FlailerGlobalBackgroundAnimation::AddTextureHash(std::uint32_t key)
{
    RemoveTextureHash(key);
    for (auto& entry:textures) if (!entry) { entry=key;break; }
}

void FlailerGlobalBackgroundAnimation::RemoveTextureHash(std::uint32_t key)
{
    for (auto& entry:textures) if (entry==key) entry=0;
}

std::uint32_t FlailerGlobalBackgroundAnimation::GetNumTextures() const
{
    // PAL returns one past the highest occupied slot, including holes.
    std::uint32_t result=0;
    for (std::size_t index=0;index<textures.size();++index)
        if (textures[index]) result=static_cast<std::uint32_t>(index)+1u;
    return result;
}

std::uint32_t FlailerGlobalBackgroundAnimation::GetNthTexture(std::uint32_t index) const
{
    for (auto key:textures) {
        if (!key) continue;
        if (!index) return key;
        --index;
    }
    return 0;
}

void FlailerGlobalBackgroundAnimation::DefaultTextureList()
{
    textures.fill(0);
    AddTextureHash(hashString("GRID_BLUEPRINT1"));
    AddTextureHash(hashString("GRID_BLUEPRINT2"));
}

void FlailerGlobalBackgroundAnimation::Reset()
{
    if (fading && master) {
        const float first=FlailerAnimationRandomRange(160.0f,165.0f);
        const float second=FlailerAnimationRandomRange(180.0f,300.0f);
        const float destination=FlailerAnimationRandomRange(300.0f,400.0f);
        fading->SetAlpha(0,-1);master->SetAlpha(0,-1);
        ui_->resources->SetBitmapCenter(*fading,first,second);
        auto& x=position.axes[0];auto& y=position.axes[1];
        x.desiredValue=first;if (x.value!=first) x.mode=2;
        y.desiredValue=second;if (y.value!=second) y.mode=2;
        x.mode=y.mode=0;x.value=x.desiredValue;x.derivative=x.desiredDerivative;
        y.value=y.desiredValue;y.derivative=y.desiredDerivative;
        x.desiredValue=destination;if (x.value!=destination) x.mode=2;
        y.desiredValue=second;if (y.value!=second) y.mode=2;
        x.duration=y.duration=ui_->animationSettings.backgroundDuration;
        const auto count=GetNumTextures();
        Require(count,"Background animation has no authored texture choices");
        fading->ChangeTexture(GetNthTexture(vectorDefaultRandomBounded(count)));
    }
    startedTicks=static_cast<std::uint32_t>(ui_->clock->realTimer);
}

void FlailerGlobalBackgroundAnimation::ChangeScreenHash(std::uint32_t key)
{
    screenKey=key;
    if (key) {
        grid=static_cast<FlailerBitmapChunk*>(ui_->resources->FindObject(key,hashString("GRID"),2));
        master=static_cast<FlailerBitmapChunk*>(ui_->resources->FindObject(key,hashString("MASTER_BP_OBJECT"),2));
        fading=static_cast<FlailerBitmapChunk*>(ui_->resources->FindObject(key,hashString("FADING_BP_OBJECT"),2));
        Reset();screenChangedTicks=static_cast<std::uint32_t>(ui_->clock->realTimer);
    } else { fading=nullptr;grid=nullptr;master=nullptr; }
}

void FlailerGlobalBackgroundAnimation::Tick()
{
    if(ui_->externalPresentation)return;
    const auto& settings=ui_->animationSettings;
    position.axes[0].duration=position.axes[1].duration=settings.backgroundDuration;
    phase+=ui_->clock->realTimeElapsed;
    if (phase>=6.2829999923706055f) phase-=6.2829999923706055f;
    if (!master || !fading) return;
    const auto raw=static_cast<std::uint32_t>(ui_->clock->realTimer)-startedTicks;
    std::int32_t ticks;std::memcpy(&ticks,&raw,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    if (elapsed<settings.backgroundFadeDuration)
        fading->SetAlpha(convertFloatToInt32((elapsed/settings.backgroundFadeDuration)*128.0f),-1);
    if (elapsed<settings.backgroundDuration) {
        tCubic2DUpdate(&position,ui_->clock->realTimeElapsed);
        float point[2];tCubic2DGetVal(&position,point);
        ui_->resources->SetBitmapCenter(*fading,point[0],point[1]);
    }
    const float ending=settings.backgroundDuration-1.0f;
    if (ending<elapsed && elapsed<settings.backgroundDuration)
        fading->SetAlpha(convertFloatToInt32((1.0f-(elapsed-ending))*128.0f),-1);
    if (settings.backgroundDuration<elapsed) Reset();
}

} // namespace hp2

namespace hp2 {
float flailerSineWavePeriod=6.2829999923706055f;
FlailerSineWave::FlailerSineWave(float low,float high,float speed)
    : minimum(low),maximum(high),rate(speed) {}
void FlailerSineWave::Step(float elapsed)
{
    phase+=rate*elapsed;
    // PAL24A5D8 performs one subtraction; a large elapsed value does not
    // use fmod and an exactly equal phase is retained.
    if (flailerSineWavePeriod<phase)phase-=flailerSineWavePeriod;
}
float FlailerSineWave::GetValFloat() const
{
    return minimum+(maximum-minimum)*((rotationSineRadians(phase)+1.0f)*0.5f);
}
std::int32_t FlailerSineWave::GetValInt() const
{
    // Final24A67C CVT.W.S uses the released scalar conversion policy.
    return convertFloatToInt32(GetValFloat());
}
FlailerLeftRightArrowScroll::FlailerLeftRightArrowScroll(FlailerUiState& ui,
    std::uint32_t screen,std::uint32_t leftKey,std::uint32_t rightKey)
    : ui_(ui),screen_(screen),pulse_(0.0f,128.0f,10.0f)
{
    Require(ui_.resources && ui_.clock,"Arrow animation has no actual UI resource/clock owner");
    left_=static_cast<FlailerBitmapChunk*>(ui_.resources->FindObject(screen,leftKey,2));
    right_=static_cast<FlailerBitmapChunk*>(ui_.resources->FindObject(screen,rightKey,2));
    SetCurrentItem(0);
}
void FlailerLeftRightArrowScroll::Animate()
{
    pulse_.Step(ui_.clock->realTimeElapsed);
    if (current_) {
        if (ui_.externalPresentation)
            ui_.render->pointerArrowRows.push_back({screen_,current_});
        std::int32_t left=0,top=0,right=0,bottom=0;
        Require(ui_.resources->GetBoundingBox(screen_,current_,left,top,right,bottom,ui_.language),
            "Arrow target has no actual authored bounding box");
        const float row=static_cast<float>(top+(bottom-top)/2);
        if (left_) {
            left_->SetAlpha(pulse_.GetValInt());
            left_->SetTopLeftY(row-ui_.resources->ScaledBitmapHeight(*left_)*0.5f);
        }
        if (right_) {
            right_->SetAlpha(pulse_.GetValInt());
            right_->SetTopLeftY(row-ui_.resources->ScaledBitmapHeight(*right_)*0.5f);
        }
    }
    UpdateVisibility();
}
void FlailerLeftRightArrowScroll::UpdateVisibility()
{
    if (left_)left_->visible=current_?1:0;
    if (right_)right_->visible=current_?1:0;
}
void FlailerLeftRightArrowScroll::SetCurrentItem(std::uint32_t key)
{
    current_=key;UpdateVisibility();
}
void FlailerLeftRightArrowScroll::HandleEvent(ScreenMessage message,ScreenCallbackData&)
{
    if (message==ScreenMessage::Tick)Animate();
}
}

namespace hp2 {
struct FlailerScrollingMenuList::Storage {
    struct Element {
        FlailerObjectChunk& object;
        float originalY;
        tCubic1D position{};
        Element(FlailerUiState& ui,FlailerObjectChunk& source):object(source) {
            float x;ui.resources->GetTopLeft(source,x,originalY,ui.language);
            position.duration=0.5f;position.threshold=1.0f;
            position.value=position.desiredValue=originalY;
        }
        ~Element(){object.SetTopLeftY(originalY);}
        void Apply(float delta) {
            position.desiredValue+=delta;
            if(position.value!=position.desiredValue)position.mode=2;
        }
        void Tick(float elapsed) {
            tCubic1DUpdate(&position,elapsed,0.0f,0.0f);object.SetTopLeftY(position.value);
        }
    };
    FlailerUiState& ui;
    std::uint32_t screen,up,down,clip,first{},lastMaskRow{};
    float top{},bottom{};
    std::int32_t mask{-1},count{};
    Element* selected{};
    float maskRowY{};
    FlailerSineWave pulse{0.0f,128.0f,10.0f};
    std::vector<std::unique_ptr<Element>> elements;
    Storage(FlailerUiState& state,std::uint32_t s,std::uint32_t u,std::uint32_t d,std::uint32_t c)
        :ui(state),screen(s),up(u),down(d),clip(c) {}
    float Y(std::uint32_t key) const {
        float x=0,y=0;if(auto* object=ui.resources->FindObject(screen,key))
            ui.resources->GetTopLeft(*object,x,y,ui.language);
        return y;
    }
    Element* Find(std::uint32_t key) const {
        for(const auto& entry:elements)if(entry->object.key==key)return entry.get();
        return nullptr;
    }
    void SyncSelectedMask(std::uint32_t key) {
        selected=Find(key);
        Require(selected,"Scrolling menu highlight has no authored row");
        float closest=std::numeric_limits<float>::max();
        for(std::int32_t slot=0;slot<std::min(count,7);++slot) {
            const auto* row=Find(slot==6 && lastMaskRow?lastMaskRow:first+static_cast<std::uint32_t>(slot));
            if(!row)continue;
            const float distance=std::abs(selected->position.desiredValue-row->originalY);
            if(distance>=closest)continue;
            closest=distance;mask=slot;maskRowY=row->originalY;
        }
        Require(closest<std::numeric_limits<float>::max(),"Scrolling menu has no authored mask row");
    }
    void Alpha() {
        for(const auto key:{up,down})if(auto* object=ui.resources->FindObject(screen,key))
            object->SetAlpha(pulse.GetValInt());
    }
    bool Show(bool upwards) const {
        if(count<8)return false;
        for(const auto& entry:elements)
            if(upwards ? entry->position.desiredValue<top : bottom<entry->position.desiredValue)return true;
        return false;
    }
};
FlailerScrollingMenuList::FlailerScrollingMenuList(FlailerUiState& ui,std::uint32_t screen,
    std::uint32_t first,std::uint32_t last,std::uint32_t up,std::uint32_t down,std::uint32_t clip)
    :storage_(std::make_unique<Storage>(ui,screen,up,down,clip))
{
    Require(ui.resources && ui.clock,"Scrolling menu has no actual resource/time owners");
    SetVisibleArea(first,last);storage_->Alpha();
}
FlailerScrollingMenuList::~FlailerScrollingMenuList()
{
    // Original destructor249ED0 releases elements in list order.
    for(auto& entry:storage_->elements)entry.reset();
}
void FlailerScrollingMenuList::SetVisibleArea(std::uint32_t first,std::uint32_t last)
{
    auto& s=*storage_;s.first=first;s.top=s.Y(first);s.bottom=s.Y(last);
}
void FlailerScrollingMenuList::AddObject(std::uint32_t key)
{
    auto& s=*storage_;auto* object=s.ui.resources->FindObject(s.screen,key);
    if(!object)return; // PAL249F90 permits an absent optional object.
    object->SetClipToPoly(s.clip);
    s.elements.push_back(std::make_unique<Storage::Element>(s.ui,*object));
}
void FlailerScrollingMenuList::AddObjects(const std::uint32_t* keys,std::size_t count)
{
    Require(keys || !count,"Scrolling menu object list is absent");
    for(std::size_t index=0;index<count;++index)AddObject(keys[index]);
}
void FlailerScrollingMenuList::SetItemCount(std::int32_t count)
{
    Require(count>=0,"Scrolling menu item count is negative");storage_->count=count;
}
void FlailerScrollingMenuList::SetLastMaskRow(std::uint32_t key) {storage_->lastMaskRow=key;}
void FlailerScrollingMenuList::SetDesiredTopLeftY(std::uint32_t key,float desired)
{
    auto& s=*storage_;
    if(!s.ui.resources->FindObject(s.screen,key))return;
    auto* selected=s.Find(key);Require(selected,"Scrolling target has no original animation element");
    const float delta=desired-selected->position.desiredValue;
    for(auto& entry:s.elements)entry->Apply(delta);
}
void FlailerScrollingMenuList::SetCurrentObject(std::uint32_t key)
{
    auto& s=*storage_;
    const auto* selected=s.Find(key);
    if(!selected)return;
    const float y=s.ui.externalPresentation ? selected->position.desiredValue:s.Y(key);
    if(y<s.top)SetDesiredTopLeftY(key,s.top);
    else if(s.bottom<y)SetDesiredTopLeftY(key,s.bottom);
}
std::uint32_t FlailerScrollingMenuList::GetCurrentMenuMask() const
{
    // PAL24A888 hashes the seven actual strings MI_1_MASK..MI_7_MASK into
    // final31CFA0. These are the exact resulting source hash values.
    constexpr std::uint32_t masks[]={0x6e34e350u,0x708a0bf1u,0x72df3492u,
        0x75345d33u,0x778985d4u,0x79deae75u,0x7c33d716u};
    Require(storage_->mask>=0 && storage_->mask<7,"Scrolling menu mask has no highlighted item");
    return masks[storage_->mask];
}
float FlailerScrollingMenuList::CurrentMenuOffsetY() const
{
    const auto& s=*storage_;
    return s.selected ? s.selected->position.value-s.maskRowY:0.0f;
}
void FlailerScrollingMenuList::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    auto& s=*storage_;
    if(message==ScreenMessage::Highlight) {
        if(!s.ui.externalPresentation && s.mask==-1) {
            if(data.key>=0x52e039f3u && data.key<=0x52e039fbu)
                s.mask=std::min(static_cast<std::int32_t>(data.key-0x52e039f3u),6);
            else if(data.key==0xaee77883u || data.key==0xaee77884u)s.mask=6;
        }
        SetCurrentObject(data.key);
        // Godot can focus any visible row directly. Resolve the fixed mask
        // from the committed row's scroll target, including mid-scroll input.
        if(s.ui.externalPresentation)s.SyncSelectedMask(data.key);
        s.ui.resources->SetObjectVisibility(s.screen,s.up,s.Show(true)?1:0);
        s.ui.resources->SetObjectVisibility(s.screen,s.down,s.Show(false)?1:0);
    } else if(message==ScreenMessage::Tick) {
        for(auto& entry:s.elements)entry->Tick(s.ui.clock->realTimeElapsed);
        s.pulse.Step(s.ui.clock->realTimeElapsed);s.Alpha();
    } else if(!s.ui.externalPresentation) {
        if(message==ScreenMessage::Next)s.mask=std::min(s.mask+1,std::min(s.count,7)-1);
        else if(message==ScreenMessage::Previous)s.mask=std::max(s.mask-1,0);
    }
}
}

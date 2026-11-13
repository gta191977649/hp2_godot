#include "indep/inc/HudAnim.hpp"
#include "FlailerDB.hpp"
#include "bware/inc/bMath.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace hp2 {
namespace {

void hudObjectVisibility(FlailerObjectChunk* object, bool visible)
{
    object->visible = static_cast<std::uint8_t>(visible);
    if (!visible && object->strip) object->strip->pool->Discard(*object);
}

void hudObjectPose(FlailerObjectChunk& object, const std::array<float,2>& position,
    const std::array<float,2>& scale)
{
    object.topLeftX = static_cast<float>(convertFloatToInt32(position[0]));
    object.topLeftY = static_cast<float>(convertFloatToInt32(position[1]));
    object.scaleX = scale[0];
    object.scaleY = scale[1];
}

std::int32_t hudAlphaAdd(std::int32_t initial, float change)
{
    const std::uint32_t bits = static_cast<std::uint32_t>(initial) +
        static_cast<std::uint32_t>(convertFloatToInt32(change));
    std::int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

float hudElapsedSeconds(std::int32_t now,std::int32_t started)
{
    const std::uint32_t bits=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(started);
    std::int32_t elapsed;
    std::memcpy(&elapsed,&bits,sizeof(elapsed));
    return std::max(0.0f,static_cast<float>(elapsed)*0.0002500000118743628f);
}

void hudCheckDuration(float seconds)
{
    if (!std::isfinite(seconds) || seconds<0.0f)
        throw std::invalid_argument("HUD animation duration must be finite and nonnegative");
}

float hudAnimationFraction(float elapsed,float duration)
{
    return duration==0.0f ? 1.0f:std::clamp(elapsed/duration,0.0f,1.0f);
}

}

HudAnimator::HudAnimator()
{
    blinkStart_ = {128.0f,64.0f,0.0f,0.0f};
}

void HudAnimator::SetVisibility(bool visible)
{
    if (text_) hudObjectVisibility(text_, visible);
    if (bitmap_) hudObjectVisibility(bitmap_, visible);
}

void HudAnimator::StartAnimation(const HudTextAnimation& animation, const GameTime& clock)
{
    for (const float duration:animation.durationSeconds)hudCheckDuration(duration);
    kind_ = 3;
    phase_ = 0;
    ownsCaption_ = animation.caption != nullptr;
    if (animation.caption) {
        const std::size_t size = std::strlen(animation.caption);
        if (size >= caption_.size()) throw std::invalid_argument("HUD caption exceeds its owned buffer");
        std::memcpy(caption_.data(), animation.caption, size + 1);
    } else {
        caption_[0] = 0;
    }
    position_ = animation.position;
    positionIn_ = animation.positionIn;
    scale_ = animation.scale;
    scaleIn_ = animation.scaleIn;
    alpha_ = animation.alpha;
    alphaIn_ = animation.alphaIn;
    text_ = animation.text;
    bitmap_ = nullptr;
    color_ = animation.color;
    positionOut_ = animation.positionOut;
    heldScale_ = animation.heldScale;
    scaleOut_ = animation.scaleOut;
    heldAlpha_ = animation.heldAlpha;
    alphaOut_ = animation.alphaOut;
    colorChange_ = animation.colorChange;
    durationSeconds_ = animation.durationSeconds;
    useRealTimer_ = animation.useRealTimer;
    startedTicks_ = useRealTimer_ ? clock.realTimer : clock.worldTimer;
    phaseStartedSeconds_ = 0.0f;
    active_ = true;
    if (text_) {
        hudObjectPose(*text_, position_, scale_);
        text_->SetAlpha(alpha_);
        hudObjectVisibility(text_, true);
    }
}

// PAL23B498: unlike the bitmap overload, text restarts an active slot.
void HudAnimator::StartAnimation(FlailerTextChunk& text,const HudMoveAnimation& animation,const GameTime& clock)
{
    hudCheckDuration(animation.durationSeconds);
    position_=animation.position;positionIn_=animation.positionChange;
    scale_=animation.scale;scaleIn_=animation.scaleChange;
    alpha_=animation.alpha;alphaIn_=animation.alphaChange;
    durationSeconds_[0]=animation.durationSeconds;
    text_=&text;bitmap_=nullptr;kind_=1;active_=true;
    blinkStart_={128.0f,64.0f,0.0f,0.0f};blinkChange_={0.0f,64.0f,128.0f,0.0f};
    startedTicks_=clock.worldTimer;phaseStartedSeconds_=0.0f;
    hudObjectPose(text,position_,scale_);text.SetAlpha(alpha_);hudObjectVisibility(&text,true);
}

// PAL23B5E0: setting kind precedes the active check. Repeated HUD calls
// therefore do not reset an in-progress wrong-way/bitmap interpolation.
void HudAnimator::StartAnimation(FlailerBitmapChunk& bitmap,const HudMoveAnimation& animation,
    const GameTime& clock,bool keepVisible)
{
    kind_=1;
    if (active_) return;
    hudCheckDuration(animation.durationSeconds);
    position_=animation.position;positionIn_=animation.positionChange;
    scale_=animation.scale;scaleIn_=animation.scaleChange;
    alpha_=animation.alpha;alphaIn_=animation.alphaChange;
    durationSeconds_[0]=animation.durationSeconds;
    bitmap_=&bitmap;text_=nullptr;keepVisible_=keepVisible;active_=true;
    startedTicks_=clock.worldTimer;phaseStartedSeconds_=0.0f;
    hudObjectPose(bitmap,position_,scale_);bitmap.SetAlpha(alpha_);hudObjectVisibility(&bitmap,true);
}

// PAL23B700. The source stores both RGBA vectors but blinking consumes
// their alpha component only; RGB is not silently recoloured.
void HudAnimator::StartBlink(FlailerBitmapChunk& bitmap,const std::array<float,4>& color,
    const std::array<float,4>& colorChange,float period,float periodChange,const GameTime& clock)
{
    bitmap_=&bitmap;text_=nullptr;kind_=2;blinkStart_=color;blinkChange_=colorChange;
    blinkPeriod_=period;blinkPeriodChange_=periodChange;active_=true;
    durationSeconds_[0]=0.0f;
    startedTicks_=clock.worldTimer;phaseStartedSeconds_=0.0f;
    bitmap.SetAlpha(convertFloatToInt32(blinkStart_[3]));hudObjectVisibility(&bitmap,true);
}

// Complete PAL23B788. Its bool result remains false even when the active
// bit clears; callers inspect Active rather than inventing a completion event.
bool HudAnimator::DoAnimation(std::int32_t nowTicks)
{
    if (!active_) return false;
    const float fraction=hudAnimationFraction(hudElapsedSeconds(nowTicks,startedTicks_),durationSeconds_[0]);
    if (kind_!=1) return false;
    std::array<float,2> position{},scale{};
    for (std::size_t axis=0;axis<2;++axis) {
        position[axis]=positionIn_[axis]*fraction+position_[axis];
        scale[axis]=scaleIn_[axis]*fraction+scale_[axis];
    }
    const auto alpha=hudAlphaAdd(alpha_,alphaIn_*fraction);
    if (text_) {
        hudObjectPose(*text_,position,scale);text_->SetAlpha(alpha);
        std::array<float,4> color{};
        for (std::size_t axis=0;axis<4;++axis) color[axis]=blinkChange_[axis]*fraction+blinkStart_[axis];
        text_->SetColor(static_cast<uint32_t>(convertFloatToInt32(color[0]))|
            (static_cast<uint32_t>(convertFloatToInt32(color[1]))<<8)|
            (static_cast<uint32_t>(convertFloatToInt32(color[2]))<<16));
        if (fraction>=1.0f) {hudObjectVisibility(text_,false);active_=false;}
    } else if (bitmap_) {
        hudObjectPose(*bitmap_,position,scale);bitmap_->SetAlpha(alpha);
        if (fraction>=1.0f) {
            bitmap_->SetAlpha(hudAlphaAdd(alpha_,alphaIn_));active_=false;
            if (!keepVisible_) hudObjectVisibility(bitmap_,false);
        }
    }
    return false;
}

// PAL23BA48: strength0/1 directly selects the endpoint alpha, independent
// of kind/active. The intermediate branch uses the original positive modulus.
bool HudAnimator::DoAnimation(std::int32_t nowTicks,float blinkStrength)
{
    if (!bitmap_) return false;
    if (blinkStrength==0.0f) bitmap_->SetAlpha(convertFloatToInt32(blinkStart_[3]));
    else if (blinkStrength==1.0f)
        bitmap_->SetAlpha(convertFloatToInt32(blinkStart_[3]+blinkChange_[3]));
    else {
        const float elapsed=hudElapsedSeconds(nowTicks,startedTicks_);
        const float phase=vectorWrapFloat(elapsed,blinkPeriod_+blinkStrength*blinkPeriodChange_);
        if (kind_==2) bitmap_->SetAlpha(convertFloatToInt32(blinkStart_[3]+phase*blinkChange_[3]));
    }
    return false;
}

bool HudAnimator::DoAnimation(std::int32_t nowTicks, const char* dynamicCaption)
{
    if (!active_) return false;
    if (kind_ != 3) return false;
    if (static_cast<std::uint32_t>(phase_) >= 3u) phase_ = 0;
    // PAL advances at most one phase per presentation call. Native playback
    // consumes all elapsed phases, including a long frame, so score credit
    // and the same authored envelope depend on time rather than render FPS.
    // Keep an exact phase endpoint visible on its boundary, as at source FPS.
    const float elapsed=hudElapsedSeconds(nowTicks,startedTicks_);
    while (phase_<2 && (durationSeconds_[phase_]==0.0f ||
           elapsed-phaseStartedSeconds_>durationSeconds_[phase_])) {
        phaseStartedSeconds_+=durationSeconds_[phase_];
        ++phase_;
    }
    const float fraction=hudAnimationFraction(elapsed-phaseStartedSeconds_,durationSeconds_[phase_]);

    std::array<float,2> position{}, scale{};
    std::array<float,4> color{};
    std::int32_t alpha;
    if (phase_ == 0) {
        for (std::size_t axis = 0; axis < 2; ++axis) {
            position[axis] = positionIn_[axis] * fraction + position_[axis];
            scale[axis] = scaleIn_[axis] * fraction + scale_[axis];
        }
        alpha = hudAlphaAdd(alpha_, alphaIn_ * fraction);
        for (std::size_t axis = 0; axis < 4; ++axis)
            color[axis] = colorChange_[axis] * fraction + color_[axis];
    } else if (phase_ == 1) {
        for (std::size_t axis = 0; axis < 2; ++axis)
            position[axis] = positionIn_[axis] + position_[axis];
        scale = heldScale_;
        alpha = heldAlpha_;
        for (std::size_t axis = 0; axis < 4; ++axis)
            color[axis] = colorChange_[axis] + color_[axis];
    } else {
        for (std::size_t axis = 0; axis < 2; ++axis) {
            const float exitOffset = positionOut_[axis] * fraction;
            const float enteredPosition = exitOffset + positionIn_[axis];
            position[axis] = enteredPosition + position_[axis];
            scale[axis] = scaleOut_[axis] * fraction + heldScale_[axis];
        }
        alpha = hudAlphaAdd(heldAlpha_, alphaOut_ * fraction);
        for (std::size_t axis = 0; axis < 4; ++axis)
            color[axis] = colorChange_[axis] + color_[axis];
    }
    if (text_) {
        hudObjectPose(*text_, position, scale);
        text_->SetAlpha(alpha);
        text_->SetString(ownsCaption_ ? caption_.data() : dynamicCaption);
        const std::uint32_t red = static_cast<std::uint32_t>(convertFloatToInt32(color[0]));
        const std::uint32_t green = static_cast<std::uint32_t>(convertFloatToInt32(color[1]));
        const std::uint32_t blue = static_cast<std::uint32_t>(convertFloatToInt32(color[2]));
        text_->SetColor(red | green << 8 | blue << 16);
    }
    if (fraction >= 1.0f) {
        if (phase_ == 0 || phase_ == 1) {
            phaseStartedSeconds_ += durationSeconds_[phase_];
            ++phase_;
        } else {
            // The logical score timer completes even while its HUD widget
            // is absent; a hidden screen must not defer points to the next
            // event or to a frame in which the widget becomes available.
            if (text_)hudObjectVisibility(text_, false);
            active_ = false;
            return true;
        }
    }
    return false;
}

void HudAnimator::FinishAnimation()
{
    if (kind_ == 1) {
        if (text_) {
            hudObjectVisibility(text_, false);
            active_ = false;
        } else if (bitmap_) {
            std::array<float,2> position{}, scale{};
            for (std::size_t axis = 0; axis < 2; ++axis) {
                position[axis] = positionIn_[axis] + position_[axis];
                scale[axis] = scaleIn_[axis] + scale_[axis];
            }
            hudObjectPose(*bitmap_, position, scale);
            bitmap_->SetAlpha(hudAlphaAdd(alpha_, alphaIn_));
            active_ = false;
            if (!keepVisible_) hudObjectVisibility(bitmap_, false);
        }
    } else if (kind_ == 2) {
        if (bitmap_) bitmap_->SetAlpha(convertFloatToInt32(blinkStart_[3]));
    } else if (kind_ == 3 && text_) {
        hudObjectVisibility(text_, false);
        active_ = false;
    }
}

bool HudAnimator::Active() const noexcept
{
    return active_;
}

std::int32_t HudAnimator::Phase() const noexcept
{
    return phase_;
}

}

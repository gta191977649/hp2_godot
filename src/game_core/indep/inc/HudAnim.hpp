#ifndef NFSHP2_HUD_ANIM_HPP
#define NFSHP2_HUD_ANIM_HPP

#include "FlailerChunks.hpp"
#include "indep/inc/Timer.hpp"
#include <array>
#include <cstdint>

namespace hp2 {

struct HudTextAnimation {
    FlailerTextChunk* text;
    std::array<float,2> position;
    std::array<float,2> positionIn;
    std::array<float,2> positionOut;
    std::array<float,2> scale;
    std::array<float,2> scaleIn;
    std::int32_t alpha;
    float alphaIn;
    std::array<float,4> color;
    std::array<float,2> heldScale;
    std::array<float,2> scaleOut;
    std::int32_t heldAlpha;
    float alphaOut;
    std::array<float,4> colorChange;
    std::array<float,3> durationSeconds;
    const char* caption;
    bool useRealTimer;
};

struct HudMoveAnimation {
    std::array<float,2> position{};
    std::array<float,2> positionChange{};
    std::array<float,2> scale{};
    std::array<float,2> scaleChange{};
    std::int32_t alpha{};
    float alphaChange{};
    float durationSeconds{};
};

/* A CPU animator borrows the same live Flailer object and strip pool. It
 * owns its interpolation state and copied caption, never another UI object. */
class HudAnimator final {
public:
    HudAnimator();
    void SetVisibility(bool visible);
    void StartAnimation(const HudTextAnimation& animation, const GameTime& clock);
    void StartAnimation(FlailerTextChunk& text,const HudMoveAnimation& animation,const GameTime& clock);
    void StartAnimation(FlailerBitmapChunk& bitmap,const HudMoveAnimation& animation,
        const GameTime& clock,bool keepVisible);
    void StartBlink(FlailerBitmapChunk& bitmap,const std::array<float,4>& color,
        const std::array<float,4>& colorChange,float period,float periodChange,const GameTime& clock);
    bool DoAnimation(std::int32_t nowTicks);
    bool DoAnimation(std::int32_t nowTicks,float blinkStrength);
    bool DoAnimation(std::int32_t nowTicks, const char* dynamicCaption);
    void FinishAnimation();
    bool Active() const noexcept;
    std::int32_t Phase() const noexcept;
private:
    std::array<char,64> caption_{};
    bool ownsCaption_{};
    FlailerTextChunk* text_{};
    FlailerBitmapChunk* bitmap_{};
    std::array<float,2> position_{};
    std::array<float,2> positionIn_{};
    std::array<float,2> scale_{};
    std::array<float,2> scaleIn_{};
    std::int32_t alpha_{};
    float alphaIn_{};
    std::array<float,2> positionOut_{};
    std::array<float,2> heldScale_{};
    std::array<float,2> scaleOut_{};
    std::int32_t heldAlpha_{};
    float alphaOut_{};
    std::array<float,4> blinkStart_{};
    std::array<float,4> blinkChange_{};
    std::array<float,4> color_{};
    std::array<float,4> colorChange_{};
    std::array<float,3> durationSeconds_{};
    // Native clock origin stays in ticks. Subtracting two large float
    // timestamps loses sub-frame precision and fails at the timer wrap.
    std::int32_t startedTicks_{};
    float phaseStartedSeconds_{};
    std::int32_t phase_{};
    bool active_{};
    bool keepVisible_{};
    bool useRealTimer_{};
    std::int32_t kind_{};
    float blinkPeriod_{};
    float blinkPeriodChange_{};
};

}

#endif

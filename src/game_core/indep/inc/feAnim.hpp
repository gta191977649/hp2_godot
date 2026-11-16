#pragma once

#include "FlailerChunks.hpp"
#include "indep/inc/Point.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <vector>
#include <memory>
#include <string>

namespace hp2 {
struct FlailerUiState;
struct FlailerScreenList;
enum class ScreenMessage : std::int32_t;
struct ScreenCallbackData;

struct FlailerAnimationSettings {
    float highlightDuration{0.5f};
    float highlightStartScale{1.0f};
    float highlightPeakScale{2.0f};
    float highlightStartAlpha{128.0f};
    float highlightEndAlpha{};
    float highlightRestart{4.0f};
    std::int32_t highlightColor{128}; // Same final3319E8 tint used by all screens.
    float backgroundDuration{50.0f};
    float backgroundFadeDuration{1.0f};
    // Final source globals used by the same UI animation owners.
    float itemSlideDuration{0.05000000074505806f};
    float itemSlideDelay{0.05000000074505806f};
    float itemSlideSpacing{10.0f};
    float itemSlideExitPosition{650.0f};
    float screenSlideStartPosition{650.0f};
    float blurbFadeDuration{0.6000000238418579f}; // PAL331E48.
    float blurbTextDelay{0.5f}; // PAL331E4C.
};

class FlailerAlphaAnimation final {
public:
    FlailerAlphaAnimation(FlailerUiState& ui,std::uint32_t screen);
    FlailerAlphaAnimation(FlailerUiState& ui,std::uint32_t screen,std::uint32_t group);
    ~FlailerAlphaAnimation();
    FlailerAlphaAnimation(const FlailerAlphaAnimation&)=delete;
    FlailerAlphaAnimation& operator=(const FlailerAlphaAnimation&)=delete;
    void AddObject(std::uint32_t key);
    void SetAlphaScale(float scale);
private:
    struct SavedAlpha {FlailerObjectChunk* object;std::array<float,4> alpha;};
    FlailerUiState* ui_{};
    std::uint32_t screenKey_{};
    std::vector<SavedAlpha> objects_;
};

class FlailerHighlightAnimation final {
public:
    FlailerHighlightAnimation(FlailerUiState& ui,std::uint32_t screen,std::uint32_t echo,std::int32_t color);
    void SetCurrentMenuItem(std::uint32_t key);
    void Animate();
    void ForceStart();
    void HandleEvents(ScreenMessage message,ScreenCallbackData& data);
    std::uint32_t screenKey{};
    std::uint32_t targetKey{};
    std::uint32_t startedTicks{};
    std::uint32_t echoKey{};
    std::int32_t color{};
    float startScale{};
    float peakScale{};
    float restartDuration{};
private:
    FlailerUiState* ui_{};
};

class FlailerGlobalBackgroundAnimation final {
public:
    explicit FlailerGlobalBackgroundAnimation(FlailerUiState& ui);
    void ChangeScreenHash(std::uint32_t key);
    void Reset();
    void Tick();
    void AddTextureHash(std::uint32_t key);
    void RemoveTextureHash(std::uint32_t key);
    std::uint32_t GetNumTextures() const;
    std::uint32_t GetNthTexture(std::uint32_t index) const;
    void DefaultTextureList();
    std::uint32_t screenKey{};
    FlailerBitmapChunk* grid{};
    FlailerBitmapChunk* master{};
    FlailerBitmapChunk* fading{};
    tCubic2D position{};
    std::uint32_t startedTicks{};
    std::uint32_t screenChangedTicks{};
    float phase{};
    std::array<std::uint32_t,8> textures{};
private:
    FlailerUiState* ui_{};
};

class FlailerAnimationGroup final {
public:
    FlailerAnimationGroup(FlailerUiState& ui,std::uint32_t screen,std::uint32_t group,
        std::uint32_t anchor,float initial,float desired,float duration,bool animateExternally=false);
    ~FlailerAnimationGroup();
    FlailerAnimationGroup(const FlailerAnimationGroup&)=delete;
    FlailerAnimationGroup& operator=(const FlailerAnimationGroup&)=delete;
    void SetupToAnimate(float initial,float desired);
    void Animate();
    FlailerUiState* ui{};
    std::uint32_t screenKey{};
    std::uint32_t groupKey{};
    std::uint32_t anchorKey{};
    std::int32_t savedX{};
    std::int32_t savedY{};
    tCubic1D position{};
private:
    bool animateExternally_{};
};

// Original tAnimatedBlurb. Its slide and alpha members borrow the same
// authored Flailer objects used by the screen renderer.
class FlailerAnimatedBlurb final {
public:
    FlailerAnimatedBlurb(FlailerUiState& ui,std::uint32_t screen,
        std::uint32_t text,std::uint32_t firstAlpha,std::uint32_t secondAlpha);
    ~FlailerAnimatedBlurb();
    FlailerAnimatedBlurb(const FlailerAnimatedBlurb&)=delete;
    FlailerAnimatedBlurb& operator=(const FlailerAnimatedBlurb&)=delete;
    void Animate();
    void NewText(std::int32_t languageId);
    void NewText(const char* text);
    void Hide();
private:
    FlailerUiState& ui_;
    std::uint32_t screenKey_;
    std::uint32_t textKey_;
    std::int32_t fadeStartedTicks_{};
    bool textInstalled_{};
    FlailerAnimationGroup slide_;
    FlailerAlphaAnimation firstAlpha_;
    FlailerAlphaAnimation secondAlpha_;
    std::array<char,256> text_{};
    float destination_;
    std::int32_t textChangedTicks_{};
    std::int32_t phase_{};
    bool disabled_{};
};

float FlailerAnimationRandomRange(float minimum,float maximum);

class FlailerSlideAnimation {
public:
    FlailerSlideAnimation(FlailerUiState& ui,std::uint32_t screen);
    ~FlailerSlideAnimation();
    FlailerSlideAnimation(const FlailerSlideAnimation&)=delete;
    FlailerSlideAnimation& operator=(const FlailerSlideAnimation&)=delete;
    void AddGroup(std::uint32_t group,std::uint32_t anchor,float initial,float desired,float duration);
    void Animate();
    bool HasControl() const noexcept;
    void Start();
    bool IsFinished() const;
    float GetFractionCompleted() const;
    float MaximumDuration() const noexcept {return maximumDuration_;}
private:
    FlailerUiState& ui_;
    std::uint32_t screen_;
    std::vector<std::unique_ptr<FlailerAnimationGroup>> groups_;
    std::int32_t startedTicks_{};
    float maximumDuration_{};
};

class FlailerSlideInAnimation final : public FlailerSlideAnimation {
public:
    FlailerSlideInAnimation(FlailerUiState& ui,std::uint32_t screen,float destination);
};

class FlailerSlideOutAnimation : public FlailerSlideAnimation {
public:
    FlailerSlideOutAnimation(FlailerUiState& ui,std::uint32_t screen);
};

class FlailerExitScreenAnimation final : public FlailerSlideOutAnimation {
public:
    FlailerExitScreenAnimation(FlailerUiState& ui,FlailerScreenList& screens,
        std::uint32_t screen,const char* destination,bool push);
    void HandleMessages(ScreenMessage message,ScreenCallbackData& data);
private:
    FlailerScreenList& screens_;
    std::string destination_;
    bool hasDestination_;
    bool push_;
};
} // namespace hp2

namespace hp2 {
// The actual sine-wave phase period is a mutable source scalar (PAL32F18C).
extern float flailerSineWavePeriod;
class FlailerSineWave final {
public:
    FlailerSineWave(float minimum,float maximum,float rate);
    void Step(float elapsed);
    float GetValFloat() const;
    std::int32_t GetValInt() const;
    float minimum{},maximum{},rate{},phase{};
};
class FlailerLeftRightArrowScroll final {
public:
    FlailerLeftRightArrowScroll(FlailerUiState& ui,std::uint32_t screen,
        std::uint32_t leftKey,std::uint32_t rightKey);
    void Animate();
    void UpdateVisibility();
    void SetCurrentItem(std::uint32_t key);
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
private:
    FlailerUiState& ui_;
    std::uint32_t screen_;
    FlailerBitmapChunk* left_{};
    FlailerBitmapChunk* right_{};
    std::uint32_t current_{};
    FlailerSineWave pulse_;
};
}

namespace hp2 {
// Port-side viewport for lists with a row for each item. Navigation commits
// immediately while the clipped artwork moves on the frontend clock.
class FlailerListViewport final {
public:
    FlailerListViewport(FlailerResources& resources,FlailerScreenResource& screen,
        const FlailerBitmapChunk& model,float left,float top,float width,float height,
        float pitch,std::size_t visibleRows);
    ~FlailerListViewport();
    FlailerListViewport(const FlailerListViewport&)=delete;
    FlailerListViewport& operator=(const FlailerListViewport&)=delete;
    void Reset(std::size_t top=0);
    bool Reveal(std::size_t row);
    bool Tick(float elapsed);
    bool Moving() const {return scroll_.mode!=0;}
    std::size_t Top() const {return top_;}
    float Displacement(std::size_t row) const {return static_cast<float>(row)*pitch_-scroll_.value;}
    void Clip(FlailerObjectChunk& object) const {object.SetClipToPoly(clip_.key);}
    FlailerTextChunk& CloneText(const FlailerTextChunk& model,std::uint32_t key);
private:
    FlailerScreenResource& screen_;
    FlailerBitmapChunk clip_;
    std::deque<FlailerTextChunk> texts_;
    tCubic1D scroll_{};
    float pitch_;
    std::size_t visibleRows_,top_{};
};

// Original tScrollingMenuList and its owned tMenuAnimElement list. Resources
// are borrowed; deleting the list restores their original vertical positions.
class FlailerScrollingMenuList final {
public:
    FlailerScrollingMenuList(FlailerUiState& ui,std::uint32_t screen,
        std::uint32_t first,std::uint32_t last,std::uint32_t up,
        std::uint32_t down,std::uint32_t clip);
    ~FlailerScrollingMenuList();
    FlailerScrollingMenuList(const FlailerScrollingMenuList&)=delete;
    FlailerScrollingMenuList& operator=(const FlailerScrollingMenuList&)=delete;
    void SetVisibleArea(std::uint32_t first,std::uint32_t last);
    void AddObject(std::uint32_t key);
    void AddObjects(const std::uint32_t* keys,std::size_t count);
    void SetItemCount(std::int32_t count);
    void SetLastMaskRow(std::uint32_t key);
    void SetDesiredTopLeftY(std::uint32_t key,float desired);
    void SetCurrentObject(std::uint32_t key);
    std::uint32_t GetCurrentMenuMask() const;
    // Port-side displacement of the selected label from its authored mask slot.
    float CurrentMenuOffsetY() const;
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}

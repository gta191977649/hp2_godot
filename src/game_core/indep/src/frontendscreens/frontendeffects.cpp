#include "FMenus.hpp"
#include "Timer.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

namespace hp2 {
namespace {
// Actual released3319C4/3319C8. The first overload shares this process
// phase; the second borrows the individual screen's float phase.
float cursorAnimationRate=600.0f;
float cursorAnimationPhase=0.0f;
}

// Complete PAL215EE8, beta210B20, original FrontendEffects.cpp definition.
void AnimateCursor(FlailerUiState& ui,FlailerBitmapChunk* first,
    FlailerBitmapChunk* second,float* phase)
{
    Require(ui.clock && first && second && phase,
        "Cursor animation lacks its original clock, bitmaps or phase owner");
    *phase+=cursorAnimationRate*ui.clock->realTimeElapsed;
    if(*phase>360.0f)*phase-=360.0f;
    const float sine=rotationSineRadians(*phase*0.01745329424738884f);
    const auto alpha=convertFloatToInt32((sine+1.0f)*64.0f);
    first->SetAlpha(alpha);second->SetAlpha(alpha);
}

// Complete PAL215ED0: the integer is a reset flag, never a phase pointer.
void AnimateCursor(FlailerUiState& ui,FlailerBitmapChunk* first,
    FlailerBitmapChunk* second,std::int32_t reset)
{
    if(reset)cursorAnimationPhase=0.0f;
    AnimateCursor(ui,first,second,&cursorAnimationPhase);
}
} // namespace hp2

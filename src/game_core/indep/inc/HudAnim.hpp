#pragma once
// E-0112-H02. Debug ../indep/inc/HudAnim.hpp; explicit logical-memory view,
// extent0x120. Native arrays carry values, never the original PS2 call ABI.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/flailer_resources.hpp"
namespace hp2::debug {
class HudAnimator:public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_00234E38(); // Original constructor; writes only original initialized fields.
 void Sub_00234EA8(int flags); // Original destructor.
 void SetVisibility(bool); //00234EC8.
 void StartAnimation(FlailerTextChunk,const std::array<float,2>& Pos1,
  const std::array<float,2>& Vec1,const std::array<float,2>& Vec2,
  const std::array<float,2>& ScaleStrt1,const std::array<float,2>& ScaleVec1,
  int AlphaStrt1,float AlphaMag1,const std::array<float,4>& Clr1,
  const std::array<float,2>& ScaleStrt2,const std::array<float,2>& ScaleVec2,
  int AlphaStrt2,float AlphaMag2,const std::array<float,4>& Clr2,
  float Time1,float TimeHold,float Time2,std::uint32_t strString,
  std::uint32_t bMusic); //00234F20; Q-copy colour lane4/fullword bool explicit.
 void StartAnimation(FlailerTextChunk,const std::array<float,2>& Pos,
  const std::array<float,2>& Vec,const std::array<float,2>& ScaleStrt,
  const std::array<float,2>& ScaleVec,int AlphaStrt,float AlphaMag,float TotalTime,
  std::uint32_t t); //002351D8; source ignores supplied Timer, uses WorldTimer.
 void StartAnimation(FlailerBitmapChunk,const std::array<float,2>& Pos,
  const std::array<float,2>& Vec,const std::array<float,2>& ScaleStrt,
  const std::array<float,2>& ScaleVec,int AlphaStrt,float AlphaMag,float TotalTime,
  std::uint32_t t,std::uint32_t bVisibility); //00235320; explicit source fullword.
 void StartBlink(FlailerBitmapChunk,const std::array<float,4>& ColorStrt,
  const std::array<float,4>& ColorMag,float RateStrt,float RateMagnitude); //00235440.
 bool DoAnimation(std::uint32_t t); //002354C8; WorldTimer, returns false even at completion.
 bool DoAnimation(std::uint32_t t,float Intensity); //00235788; WorldTimer.
 bool DoAnimation(const std::uint32_t* t,std::uint32_t string); //00235898; pointed Timer value, source string address.
 void FinishAnimation(); //00235CA0.
};
}

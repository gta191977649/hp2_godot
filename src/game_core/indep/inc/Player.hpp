#pragma once
// Exact original ../indep/inc/Player.hpp referenced by authorized ELF inline
// records. ObjectView construction is port-side; original00173AB0 is separate
// and is not implemented by this view constructor. E-0099-W50/W51.
#include "game_core/port/driving/object_view.hpp"
namespace port::race {struct PhysicsConstructionServices;struct AiDriveTargetServices;struct PlayerHudGameplayState;struct PlayerHudPresentationServices;}
namespace hp2::debug {
class FlailerTextChunk;class FlailerBitmapChunk;
class Player:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 // E-0110-F09: explicit PC logical animation adapter; strict original HUD entry guards.
 void UpdateHud(::port::race::PlayerHudGameplayState&,bool minimal_presentation=false); //0017D3B0.
 void UpdateScoring(::port::race::PlayerHudGameplayState&); //0017CEE0.
 void StartScore(int,::port::race::PlayerHudGameplayState&); //0017CA70; sound/title/render disabled before entry.
 void StartScore(int,int,::port::race::PlayerHudGameplayState&); //0017CC18; caller formats original score text.
 void AddToTotalScore(std::uint32_t); //0017CA10; original data phase, presentation disabled.
 int ConvertScoreString(std::uint32_t text); //Debug0017C8E0,E-0110-F02.
 int GetFinishBonus(int finish_position); //Debug0017C990,E-0110-F02.
 void Sub_00173AB0(int port,int player_number); //E-0099-W55, explicit original construction, extent0xAA0.
 void Sub_00174448(int flags); //Original destruction; active unrecovered dependencies remain explicit.
 void DoSnapshot(std::uint32_t snapshot); //Debug00174608, E-0099-W74.
 void InitForRace(std::uint32_t car); //0017FF80.
 void CleanupAfterRace(); //0017FFD0, E-0099-W62.
 void SetFinishedRacing(); //00174EA0, E-0099-W70.
 void AutoPilotOff(); //001770D0, original empty.
 void ChangeCameraMover(int type,bool save); //001752F0, source type0/1; E-0112-C01.
 int CycleDriveCameraPOVIndex(int index); //00176EE0; E-0112-C01.
 void SetTrackMapMode(int); //001774B0; E-0112-H03.
 void CycleHud(int); //001773C8; E-0112-H03, original preference table cycle.
 int GetGaugeMode()const; //00177DB0; E-0112-C01.
 void RacingJoyHandler(int port,int event,std::uint8_t param); //00177198; E-0112-C01, camera events93/94 only.
 void RemoveCameraMovers(); //00175C38.
 void SetDrivingStyle(int driver_mode); //001804A0, E-0099-W81; exact byte store.
 float GetSpeedRatio()const; //Debug0017F848, E-0099-W92.
 bool CatchupAllowed(float minimum)const; //Debug0017F858, E-0099-W92.
 void DetermineInputMode(); //00175E10, E-0099-W77.
 void SetInputMode(int mode); //00175EB8, NONE transition component.
 void ResetFlailerMovement(); //0017A3F0.
 void SetHudFeatures(std::uint32_t);
 void SetHudFeatures(std::uint32_t,const ::port::race::PlayerHudPresentationServices&);
 void RenderHud(const ::port::race::PlayerHudPresentationServices&); //0017D738; E-0112-H03.
 void UpdateHudPresentation(::port::race::PlayerHudGameplayState&); //0017D3B0; full native source path.
 void HudDoAnimations(); //0017C6F0; E-0112-H03.
 void DoScoreTitleAlpha(); //0017BE98; E-0112-H03.
 void UpdatePursuitMeter(int); //0017AA50; E-0112-H03.
 void UpdateRadarDetector(const ::port::race::PlayerHudPresentationServices&); //0017A540.
 void DrawTracker(std::uint32_t,int); //0017A948. //00177E68; zero-feature transition restored first.
 void ClearHudAnimations(); //0017AE98; E-0112-H02, including pending score completion.
 void HudStartAnimation(int,FlailerTextChunk,const std::array<float,2>&,
  const std::array<float,2>&,const std::array<float,2>&,const std::array<float,2>&,
  int,int,float,std::uint32_t); //0017B688; E-0112-H02.
 void HudStartAnimation(int,FlailerBitmapChunk,const std::array<float,2>&,
  const std::array<float,2>&,const std::array<float,2>&,const std::array<float,2>&,
  int,int,float,std::uint32_t,std::uint32_t); //0017B800; explicit fullword visibility.
 void HudStartBlink(int,FlailerBitmapChunk,const std::array<float,4>&,
  const std::array<float,4>&,float,float); //0017B980.
 void HudStartGrowAnimation(FlailerTextChunk,FlailerTextChunk,std::uint32_t,bool); //0017B1E8.
 void HudStartSpeedAnimation(FlailerTextChunk,FlailerTextChunk,std::uint32_t,bool); //0017B400.
 void HudStartWrongWay(); //0017AF58.
 void CleanUpJumpCam(); //001752B8.
 void PredetermineNisNumbers(); //00180038.
 int FindNextNisIndex(std::uint32_t,std::uint32_t,int); //00174900,E-0100-PA01.
 int PlayNextNis(std::uint32_t,std::uint32_t,std::uint32_t,int,const ::port::race::PhysicsConstructionServices* physics=nullptr); //00174B60; native args plus explicit port dependency.
 void LoadNextNis(std::uint32_t,std::uint32_t,int,bool disable_asset_load=false); //00174A40,E-0100-PA01.
 void CreateRandomNisArray(int type); //00180080.
 bool IsCameraPOVInCar()const; //Debug00177028, E-0099-W50.
 int IsNisPlaying()const; //00174D78, E-0099-W63; actual manager Playing word.
 void ClearZone(bool); //00176B70.
 void ClearZoneSound(); //00176C80.
 void RestoreCameraMover(); //00175198.
 void ServiceNis(); //00174D88, W69; ongoing/inactive source branches.
 int DetectWrongWay(int currently_wrong_way); //0017F6B8.
 bool IsAllowedToReset(); //001804F0.
 void ChooseToDisplayReset(); //001805C0.
 void UpdateGameState(const ::port::race::AiDriveTargetServices* =nullptr); //0017F280; positive freeze/NIS tails explicit; F04 actual finished AI localization.
 void TriggerVisibleDrivingLine(int difficulty,float lifetime); //Debug0017F1D0, E-0099-W64; positive line construction remains explicit.
};
void UpdatePlayersGameState(::port::driving::Memory&,const ::port::race::AiDriveTargetServices* =nullptr); //00180440,E-0099-W69/E-0110-F04.
}

namespace hp2::debug {void DetermineAllPlayersInputModes(::port::driving::Memory&); /*00180280,E-0099-W77*/}

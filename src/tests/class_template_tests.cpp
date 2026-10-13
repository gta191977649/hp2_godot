// TASK-0028 generated exception contract tests. No original-behaviour equivalence claim.
#include "game_core/AIActionAccidentRecovery.hpp"
#include "game_core/AIActionAvoidCars.hpp"
#include "game_core/AIActionAvoidRoadBlock.hpp"
#include "game_core/AIActionBlock.hpp"
#include "game_core/AIActionDraft.hpp"
#include "game_core/AIActionGetUnstuck.hpp"
#include "game_core/AIActionIJustGotHit.hpp"
#include "game_core/AIActionJustDrive.hpp"
#include "game_core/AIActionNudgeCar.hpp"
#include "game_core/AIActionPass.hpp"
#include "game_core/AIActionPullOver.hpp"
#include "game_core/AICarController.hpp"
#include "game_core/AICarSound.hpp"
#include "game_core/AIDriver.hpp"
#include "game_core/AIEngineSound.hpp"
#include "game_core/AcidEffectSystem.hpp"
#include "game_core/AnimationGroup.hpp"
#include "game_core/Autosaver.hpp"
#include "game_core/BeACopChallengeSetup.hpp"
#include "game_core/BeACopResults1P.hpp"
#include "game_core/BeACopResults2P.hpp"
#include "game_core/BeTheCopSetup.hpp"
#include "game_core/BestLapTimes.hpp"
#include "game_core/BestTotalTimes.hpp"
#include "game_core/BootMemcardLoader.hpp"
#include "game_core/BumperCameraMover.hpp"
#include "game_core/CDBFileSys.hpp"
#include "game_core/Car.hpp"
#include "game_core/CarCollisionBody.hpp"
#include "game_core/CarDamage.hpp"
#include "game_core/CarList.hpp"
#include "game_core/CarPartCuller.hpp"
#include "game_core/CarRenderInfo.hpp"
#include "game_core/ChangeableMenuItem.hpp"
#include "game_core/Chopper.hpp"
#include "game_core/ChopperSound.hpp"
#include "game_core/CollisionBodyNumberAllocator.hpp"
#include "game_core/CollisionBodyTable.hpp"
#include "game_core/CollisionBodyWitnessBitField.hpp"
#include "game_core/CopManager.hpp"
#include "game_core/CopsOffModeSelect.hpp"
#include "game_core/CopsOnModeSelect.hpp"
#include "game_core/CountdownCameraCoordinator.hpp"
#include "game_core/CreditsLine.hpp"
#include "game_core/CreditsScroller.hpp"
#include "game_core/DebugPauseMenu.hpp"
#include "game_core/DebugWorldCameraMover.hpp"
#include "game_core/DeliveryMover.hpp"
#include "game_core/DemoCameraCoordinator.hpp"
#include "game_core/DemoDiscManager.hpp"
#include "game_core/DriveCameraMover.hpp"
#include "game_core/DrivingChoice.hpp"
#include "game_core/DuplicatedSolid.hpp"
#include "game_core/DuplicatedStripEntryData.hpp"
#include "game_core/DuplicatedStripEntryTable.hpp"
#include "game_core/DynamicCollisionBody.hpp"
#include "game_core/EndGameMenu.hpp"
#include "game_core/EndGameMenuItem.hpp"
#include "game_core/EndGameScreens.hpp"
#include "game_core/EventRewardScreen.hpp"
#include "game_core/FernShakerInfoTable.hpp"
#include "game_core/FlailerRenderManager.hpp"
#include "game_core/FlailerScreenList.hpp"
#include "game_core/FreeRunSetup.hpp"
#include "game_core/FuturePoints.hpp"
#include "game_core/GlobalBackgroundAnimation.hpp"
#include "game_core/GrandSceneryCullInfo.hpp"
#include "game_core/HeliSheet.hpp"
#include "game_core/HelpAlphaAnimation.hpp"
#include "game_core/HornSound3d.hpp"
#include "game_core/HudTexturePack.hpp"
#include "game_core/JoystickCalibrationScreen.hpp"
#include "game_core/LanguageBuffer.hpp"
#include "game_core/LapKnockoutSetup.hpp"
#include "game_core/LapTimesDisplay.hpp"
#include "game_core/LoadedTextureData.hpp"
#include "game_core/MaxCameraMover.hpp"
#include "game_core/MellowMover.hpp"
#include "game_core/MumboJumboScreen.hpp"
#include "game_core/MunitionEffectsHandler.hpp"
#include "game_core/NameEntry.hpp"
#include "game_core/NearbyCarList.hpp"
#include "game_core/NewMainMenu.hpp"
#include "game_core/NisCameraCoordinator.hpp"
#include "game_core/NisManager.hpp"
#include "game_core/NullController.hpp"
#include "game_core/NullDriver.hpp"
#include "game_core/NumberAllocator.hpp"
#include "game_core/OrbitCarCameraMover.hpp"
#include "game_core/PauseAudioOpts.hpp"
#include "game_core/PauseDisplayOpts.hpp"
#include "game_core/PauseMenu.hpp"
#include "game_core/PauseOptions.hpp"
#include "game_core/PauseRaceObjective.hpp"
#include "game_core/PauseViewControls.hpp"
#include "game_core/PhysicsCar.hpp"
#include "game_core/PhysicsSystem.hpp"
#include "game_core/PlayerEngineSound.hpp"
#include "game_core/PlayerTireSound.hpp"
#include "game_core/PreRace2PBeACop.hpp"
#include "game_core/PreRaceBlurb.hpp"
#include "game_core/PreRaceStats.hpp"
#include "game_core/PreviewPause.hpp"
#include "game_core/PreviewPausePoint.hpp"
#include "game_core/PsychoDriver.hpp"
#include "game_core/Race.hpp"
#include "game_core/RaceCoordinator.hpp"
#include "game_core/RaceResultsScreen.hpp"
#include "game_core/RealController.hpp"
#include "game_core/RealDriver.hpp"
#include "game_core/RearViewMirrorCameraMover.hpp"
#include "game_core/ReplayLogBuffer.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/ReplayScreen.hpp"
#include "game_core/ReplaySnapshot.hpp"
#include "game_core/ResourceFile.hpp"
#include "game_core/RewardTrackModel.hpp"
#include "game_core/SelectTrackScreen.hpp"
#include "game_core/SelectableTrack.hpp"
#include "game_core/SelectionSet.hpp"
#include "game_core/SingleRaceSetup.hpp"
#include "game_core/SirenSound.hpp"
#include "game_core/SlotPool.hpp"
#include "game_core/SlotPoolManager.hpp"
#include "game_core/Smokeable.hpp"
#include "game_core/StillCameraMover.hpp"
#include "game_core/TerrainEffectEmitters.hpp"
#include "game_core/TextureTable.hpp"
#include "game_core/TimeAroundTrackTable.hpp"
#include "game_core/TimeOfDayTrack.hpp"
#include "game_core/TourneyPoints.hpp"
#include "game_core/TourneyStandingsScreen.hpp"
#include "game_core/TrackCarCameraMover.hpp"
#include "game_core/TrackFlyCameraMover.hpp"
#include "game_core/TrackKnockoutSetup.hpp"
#include "game_core/TrackMap.hpp"
#include "game_core/TrackMarkerFile.hpp"
#include "game_core/TrackPathCoordinate.hpp"
#include "game_core/TrackPolygonCollisionBody.hpp"
#include "game_core/TrackRoute.hpp"
#include "game_core/TrackRouteEditor.hpp"
#include "game_core/TrackRouteEditorCameraMover.hpp"
#include "game_core/TrafficCar.hpp"
#include "game_core/TrafficDriver.hpp"
#include "game_core/TrafficTeleporter.hpp"
#include "game_core/World.hpp"
#include "game_core/eAnimation.hpp"
#include "game_core/eFrameMallocBuffer0.hpp"
#include "game_core/eFrameMallocBuffer1.hpp"
#include "game_core/feAnimAlphaElement.hpp"
#include "game_core/indep/src/AverageInRealController.hpp"
#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/indep/src/CubicCameraMover.hpp"
#include "game_core/indep/src/PlayerCarSound.hpp"
#include "game_core/indep/src/QuickSplineInRealController.hpp"
#include "game_core/indep/src/VisibleDrivingLine.hpp"
#include "game_core/indep/src/ZoneFreezeCameraMover.hpp"
#include "game_core/indep/src/ZonePreviewCameraMover.hpp"
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/frontendscreens/joystickconfiguration.hpp"
#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/kAITireSound.hpp"
#include "game_core/mExitScreenAnimation.hpp"
#include "game_core/m_ptr_SOUND_Memory_EE.hpp"
#include "game_core/pCarTexturePack.hpp"
#include "game_core/pDynamicTexturePack.hpp"
#include "game_core/pInitialSpotlightAlphas.hpp"
#include "game_core/pInitialSpotlightColours.hpp"
#include "game_core/pTrackSelectTexturePack.hpp"
#include "game_core/port/police/cop_control.hpp"
#include "game_core/src/bFile.hpp"
#include "game_core/tCubic1D.hpp"
#include "game_core/tCustomizeHUDScreen.hpp"
#include "game_core/tExitScreenAnimation.hpp"
#include "game_core/tHelpMenu.hpp"
#include "game_core/tLineSegment.hpp"
#include "game_core/tMainMenuTrackDesc.hpp"
#include "game_core/tMenuAnimElement.hpp"
#include "game_core/tPointsThingy.hpp"
#include "game_core/tPopupMenu.hpp"
#include "game_core/tRandomlyPickableTrackInfo.hpp"
#include "game_core/tStatsColumn.hpp"
#include "game_core/tTotalTimeRecord.hpp"
#include "game_core/port/not_implemented.hpp"
#include <iostream>
#include <string>

namespace {
template<class Body> bool expect_gap(const char* address, const char* member, Body&& body) {
    try { body(); }
    catch (const hp2::NotImplemented& error) {
        const std::string message = error.what();
        if (message.find(address) != std::string::npos && message.find(member) != std::string::npos) return true;
        std::cerr << "Wrong exception provenance: " << message << '\n';
        return false;
    }
    catch (...) { std::cerr << "Wrong exception type: " << member << '\n'; return false; }
    std::cerr << "Missing NotImplemented: " << member << '\n';
    return false;
}
} // namespace
int main() {
    int failures = 0;
    failures += !expect_gap("0x0018D480", "AICarSound::Sub_0018D480", [] { hp2::recovered::AICarSound instance; instance.Sub_0018D480(); });
    failures += !expect_gap("0x0018D550", "AICarSound::Sub_0018D550", [] { hp2::recovered::AICarSound instance; instance.Sub_0018D550(); });
    failures += !expect_gap("0x0018D830", "AICarSound::Sub_0018D830", [] { hp2::recovered::AICarSound instance; instance.Sub_0018D830(); });
    failures += !expect_gap("0x0018D8D8", "AICarSound::Sub_0018D8D8", [] { hp2::recovered::AICarSound instance; instance.Sub_0018D8D8(); });
    failures += !expect_gap("0x0018E6E8", "AICarSound::Sub_0018E6E8", [] { hp2::recovered::AICarSound instance; instance.Sub_0018E6E8(); });
    failures += !expect_gap("0x0018E7E8", "AICarSound::Sub_0018E7E8", [] { hp2::recovered::AICarSound instance; instance.Sub_0018E7E8(); });
    failures += !expect_gap("0x0018E800", "AICarSound::Sub_0018E800", [] { hp2::recovered::AICarSound instance; instance.Sub_0018E800(); });
    failures += !expect_gap("0x0018F160", "AICarSound::Sub_0018F160", [] { hp2::recovered::AICarSound instance; instance.Sub_0018F160(); });
    failures += !expect_gap("0x0018F188", "AICarSound::Sub_0018F188", [] { hp2::recovered::AICarSound instance; instance.Sub_0018F188(); });
    failures += !expect_gap("0x0018F208", "AICarSound::Sub_0018F208", [] { hp2::recovered::AICarSound instance; instance.Sub_0018F208(); });
    failures += !expect_gap("0x0018F218", "AICarSound::Sub_0018F218", [] { hp2::recovered::AICarSound instance; instance.Sub_0018F218(); });
    failures += !expect_gap("0x0018F2B0", "AICarSound::Sub_0018F2B0", [] { hp2::recovered::AICarSound instance; instance.Sub_0018F2B0(); });
    failures += !expect_gap("0x00208B28", "AICarSound::Sub_00208B28", [] { hp2::recovered::AICarSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "AICarSound::Sub_00208C78", [] { hp2::recovered::AICarSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x0012FFC8", "AIDriver::Sub_0012FFC8", [] { hp2::recovered::AIDriver instance; instance.Sub_0012FFC8(); });
    failures += !expect_gap("0x00130090", "AIDriver::Sub_00130090", [] { hp2::recovered::AIDriver instance; instance.Sub_00130090(); });
    failures += !expect_gap("0x00130108", "AIDriver::Sub_00130108", [] { hp2::recovered::AIDriver instance; instance.Sub_00130108(); });
    failures += !expect_gap("0x00130188", "AIDriver::Sub_00130188", [] { hp2::recovered::AIDriver instance; instance.Sub_00130188(); });
    failures += !expect_gap("0x00130238", "AIDriver::Sub_00130238", [] { hp2::recovered::AIDriver instance; instance.Sub_00130238(); });
    failures += !expect_gap("0x00130890", "AIDriver::Sub_00130890", [] { hp2::recovered::AIDriver instance; instance.Sub_00130890(); });
    failures += !expect_gap("0x00190670", "AIDriver::Sub_00190670", [] { hp2::recovered::AIDriver instance; instance.Sub_00190670(); });
    failures += !expect_gap("0x00190708", "AIDriver::Sub_00190708", [] { hp2::recovered::AIDriver instance; instance.Sub_00190708(); });
    failures += !expect_gap("0x00190760", "AIDriver::Sub_00190760", [] { hp2::recovered::AIDriver instance; instance.Sub_00190760(); });
    failures += !expect_gap("0x001907B8", "AIDriver::Sub_001907B8", [] { hp2::recovered::AIDriver instance; instance.Sub_001907B8(); });
    failures += !expect_gap("0x00190850", "AIDriver::Sub_00190850", [] { hp2::recovered::AIDriver instance; instance.Sub_00190850(); });
    failures += !expect_gap("0x00191138", "AIDriver::Sub_00191138", [] { hp2::recovered::AIDriver instance; instance.Sub_00191138(); });
    failures += !expect_gap("0x00192668", "AIDriver::Sub_00192668", [] { hp2::recovered::AIDriver instance; instance.Sub_00192668(); });
    failures += !expect_gap("0x00192F10", "AIDriver::Sub_00192F10", [] { hp2::recovered::AIDriver instance; instance.Sub_00192F10(); });
    failures += !expect_gap("0x00193650", "AIDriver::Sub_00193650", [] { hp2::recovered::AIDriver instance; instance.Sub_00193650(); });
    failures += !expect_gap("0x00193758", "AIDriver::Sub_00193758", [] { hp2::recovered::AIDriver instance; instance.Sub_00193758(); });
    failures += !expect_gap("0x00193780", "AIDriver::Sub_00193780", [] { hp2::recovered::AIDriver instance; instance.Sub_00193780(); });
    failures += !expect_gap("0x00208B28", "AIEngineSound::Sub_00208B28", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "AIEngineSound::Sub_00208C78", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x002117C8", "AIEngineSound::Sub_002117C8", [] { hp2::recovered::AIEngineSound instance; instance.Sub_002117C8(); });
    failures += !expect_gap("0x002118A0", "AIEngineSound::Sub_002118A0", [] { hp2::recovered::AIEngineSound instance; instance.Sub_002118A0(); });
    failures += !expect_gap("0x00211A80", "AIEngineSound::Sub_00211A80", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00211A80(); });
    failures += !expect_gap("0x00211D58", "AIEngineSound::Sub_00211D58", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00211D58(); });
    failures += !expect_gap("0x00211D60", "AIEngineSound::Sub_00211D60", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00211D60(); });
    failures += !expect_gap("0x00212418", "AIEngineSound::Sub_00212418", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00212418(); });
    failures += !expect_gap("0x001EF3C8", "AudioOptionsScreen::Sub_001EF3C8", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EF3C8(); });
    failures += !expect_gap("0x001EFA00", "AudioOptionsScreen::Sub_001EFA00", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFA00(); });
    failures += !expect_gap("0x001EFB78", "AudioOptionsScreen::Sub_001EFB78", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFB78(); });
    failures += !expect_gap("0x001EFB80", "AudioOptionsScreen::Sub_001EFB80", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFB80(); });
    failures += !expect_gap("0x001DD0F0", "BeACopResults1P::Sub_001DD0F0", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001E0898", "BeACopResults1P::Sub_001E0898", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E0898(); });
    failures += !expect_gap("0x001E2358", "BeACopResults1P::Sub_001E2358", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2358(); });
    failures += !expect_gap("0x001E2580", "BeACopResults1P::Sub_001E2580", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "BeACopResults1P::Sub_001E2588", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BeACopResults1P::Sub_001E2590", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001DD0F0", "BeACopResults2P::Sub_001DD0F0", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001E0A10", "BeACopResults2P::Sub_001E0A10", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E0A10(); });
    failures += !expect_gap("0x001E0BC8", "BeACopResults2P::Sub_001E0BC8", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E0BC8(); });
    failures += !expect_gap("0x001E2340", "BeACopResults2P::Sub_001E2340", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2340(); });
    failures += !expect_gap("0x001E2588", "BeACopResults2P::Sub_001E2588", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BeACopResults2P::Sub_001E2590", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x00204098", "BeltMover::Sub_00204098", [] { hp2::recovered::BeltMover instance; instance.Sub_00204098(); });
    failures += !expect_gap("0x002043A0", "BeltMover::Sub_002043A0", [] { hp2::recovered::BeltMover instance; instance.Sub_002043A0(); });
    failures += !expect_gap("0x00204468", "BeltMover::Sub_00204468", [] { hp2::recovered::BeltMover instance; instance.Sub_00204468(); });
    failures += !expect_gap("0x00204D78", "BeltMover::Sub_00204D78", [] { hp2::recovered::BeltMover instance; instance.Sub_00204D78(); });
    failures += !expect_gap("0x001DD0F0", "BestLapTimes::Sub_001DD0F0", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001DDC10", "BestLapTimes::Sub_001DDC10", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001DDC10(); });
    failures += !expect_gap("0x001E2388", "BestLapTimes::Sub_001E2388", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2388(); });
    failures += !expect_gap("0x001E2580", "BestLapTimes::Sub_001E2580", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "BestLapTimes::Sub_001E2588", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BestLapTimes::Sub_001E2590", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001DD0F0", "BestTotalTimes::Sub_001DD0F0", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001DDFD0", "BestTotalTimes::Sub_001DDFD0", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001DDFD0(); });
    failures += !expect_gap("0x001E2370", "BestTotalTimes::Sub_001E2370", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2370(); });
    failures += !expect_gap("0x001E2580", "BestTotalTimes::Sub_001E2580", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "BestTotalTimes::Sub_001E2588", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BestTotalTimes::Sub_001E2590", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001577D0", "BumperCameraMover::Sub_001577D0", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "BumperCameraMover::Sub_001577D8", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "BumperCameraMover::Sub_00157850", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "BumperCameraMover::Sub_00157920", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00158800", "BumperCameraMover::Sub_00158800", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_00158800(); });
    failures += !expect_gap("0x00158910", "BumperCameraMover::Sub_00158910", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_00158910(); });
    failures += !expect_gap("0x001589A8", "BumperCameraMover::Sub_001589A8", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_001589A8(); });
    failures += !expect_gap("0x00158A68", "BumperCameraMover::Sub_00158A68", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_00158A68(); });
    failures += !expect_gap("0x0015FB10", "BumperCameraMover::Sub_0015FB10", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FB10(); });
    failures += !expect_gap("0x0015FD10", "BumperCameraMover::Sub_0015FD10", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "BumperCameraMover::Sub_0015FD18", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "BumperCameraMover::Sub_0015FD20", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "BumperCameraMover::Sub_0015FD28", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "BumperCameraMover::Sub_0015FD30", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "BumperCameraMover::Sub_0015FD38", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "BumperCameraMover::Sub_0015FD40", [] { hp2::recovered::BumperCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x0012C938", "CDBFileSys::Sub_0012C938", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012C938(); });
    failures += !expect_gap("0x0012CCA8", "CDBFileSys::Sub_0012CCA8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012CCA8(); });
    failures += !expect_gap("0x0012CD00", "CDBFileSys::Sub_0012CD00", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012CD00(); });
    failures += !expect_gap("0x0012CD78", "CDBFileSys::Sub_0012CD78", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012CD78(); });
    failures += !expect_gap("0x0012D1A0", "CDBFileSys::Sub_0012D1A0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D1A0(); });
    failures += !expect_gap("0x0012D1E0", "CDBFileSys::Sub_0012D1E0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D1E0(); });
    failures += !expect_gap("0x0012D220", "CDBFileSys::Sub_0012D220", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D220(); });
    failures += !expect_gap("0x0012D2A8", "CDBFileSys::Sub_0012D2A8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D2A8(); });
    failures += !expect_gap("0x0012D2E8", "CDBFileSys::Sub_0012D2E8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D2E8(); });
    failures += !expect_gap("0x0012D3D8", "CDBFileSys::Sub_0012D3D8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D3D8(); });
    failures += !expect_gap("0x0012D518", "CDBFileSys::Sub_0012D518", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D518(); });
    failures += !expect_gap("0x0012D560", "CDBFileSys::Sub_0012D560", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D560(); });
    failures += !expect_gap("0x0012D5F8", "CDBFileSys::Sub_0012D5F8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D5F8(); });
    failures += !expect_gap("0x0012D690", "CDBFileSys::Sub_0012D690", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012D690(); });
    failures += !expect_gap("0x0012DDC0", "CDBFileSys::Sub_0012DDC0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012DDC0(); });
    failures += !expect_gap("0x0012DEE0", "CDBFileSys::Sub_0012DEE0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012DEE0(); });
    failures += !expect_gap("0x0012E0F0", "CDBFileSys::Sub_0012E0F0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E0F0(); });
    failures += !expect_gap("0x0012E190", "CDBFileSys::Sub_0012E190", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E190(); });
    failures += !expect_gap("0x0012E1A8", "CDBFileSys::Sub_0012E1A8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1A8(); });
    failures += !expect_gap("0x0012E1B8", "CDBFileSys::Sub_0012E1B8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1B8(); });
    failures += !expect_gap("0x0012E1C0", "CDBFileSys::Sub_0012E1C0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1C0(); });
    failures += !expect_gap("0x0012E1C8", "CDBFileSys::Sub_0012E1C8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1C8(); });
    failures += !expect_gap("0x0012E1F0", "CDBFileSys::Sub_0012E1F0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1F0(); });
    failures += !expect_gap("0x0012E1F8", "CDBFileSys::Sub_0012E1F8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E1F8(); });
    failures += !expect_gap("0x0012E200", "CDBFileSys::Sub_0012E200", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E200(); });
    failures += !expect_gap("0x0012E208", "CDBFileSys::Sub_0012E208", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E208(); });
    failures += !expect_gap("0x0012E210", "CDBFileSys::Sub_0012E210", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E210(); });
    failures += !expect_gap("0x0012E218", "CDBFileSys::Sub_0012E218", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E218(); });
    failures += !expect_gap("0x0012E220", "CDBFileSys::Sub_0012E220", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E220(); });
    failures += !expect_gap("0x0012E228", "CDBFileSys::Sub_0012E228", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E228(); });
    failures += !expect_gap("0x0012E230", "CDBFileSys::Sub_0012E230", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E230(); });
    failures += !expect_gap("0x0012E238", "CDBFileSys::Sub_0012E238", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E238(); });
    failures += !expect_gap("0x0012E240", "CDBFileSys::Sub_0012E240", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E240(); });
    failures += !expect_gap("0x0012E248", "CDBFileSys::Sub_0012E248", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E248(); });
    failures += !expect_gap("0x0012E250", "CDBFileSys::Sub_0012E250", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E250(); });
    failures += !expect_gap("0x0012E258", "CDBFileSys::Sub_0012E258", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E258(); });
    failures += !expect_gap("0x0012E260", "CDBFileSys::Sub_0012E260", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E260(); });
    failures += !expect_gap("0x0012E268", "CDBFileSys::Sub_0012E268", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E268(); });
    failures += !expect_gap("0x0012E270", "CDBFileSys::Sub_0012E270", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E270(); });
    failures += !expect_gap("0x0012E2B8", "CDBFileSys::Sub_0012E2B8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E2B8(); });
    failures += !expect_gap("0x0012E2C0", "CDBFileSys::Sub_0012E2C0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E2C0(); });
    failures += !expect_gap("0x0012E2C8", "CDBFileSys::Sub_0012E2C8", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E2C8(); });
    failures += !expect_gap("0x0012E2D0", "CDBFileSys::Sub_0012E2D0", [] { hp2::recovered::CDBFileSys instance; instance.Sub_0012E2D0(); });
    failures += !expect_gap("0x00208B28", "ChopperSound::Sub_00208B28", [] { hp2::recovered::ChopperSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "ChopperSound::Sub_00208C78", [] { hp2::recovered::ChopperSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x00214D68", "ChopperSound::Sub_00214D68", [] { hp2::recovered::ChopperSound instance; instance.Sub_00214D68(); });
    failures += !expect_gap("0x00214DF0", "ChopperSound::Sub_00214DF0", [] { hp2::recovered::ChopperSound instance; instance.Sub_00214DF0(); });
    failures += !expect_gap("0x00214F30", "ChopperSound::Sub_00214F30", [] { hp2::recovered::ChopperSound instance; instance.Sub_00214F30(); });
    failures += !expect_gap("0x00215000", "ChopperSound::Sub_00215000", [] { hp2::recovered::ChopperSound instance; instance.Sub_00215000(); });
    failures += !expect_gap("0x00215028", "ChopperSound::Sub_00215028", [] { hp2::recovered::ChopperSound instance; instance.Sub_00215028(); });
    failures += !expect_gap("0x00215050", "ChopperSound::Sub_00215050", [] { hp2::recovered::ChopperSound instance; instance.Sub_00215050(); });
    failures += !expect_gap("0x00130958", "CopDriver::Sub_00130958", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00130958(); });
    failures += !expect_gap("0x00136BB8", "CopDriver::Sub_00136BB8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BB8(); });
    failures += !expect_gap("0x00136BC0", "CopDriver::Sub_00136BC0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BC0(); });
    failures += !expect_gap("0x00136BD0", "CopDriver::Sub_00136BD0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BD0(); });
    failures += !expect_gap("0x00136BD8", "CopDriver::Sub_00136BD8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BD8(); });
    failures += !expect_gap("0x00136BE0", "CopDriver::Sub_00136BE0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BE0(); });
    failures += !expect_gap("0x00136BE8", "CopDriver::Sub_00136BE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BE8(); });
    failures += !expect_gap("0x00136C00", "CopDriver::Sub_00136C00", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C00(); });
    failures += !expect_gap("0x00136C08", "CopDriver::Sub_00136C08", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C08(); });
    failures += !expect_gap("0x00136C10", "CopDriver::Sub_00136C10", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C10(); });
    failures += !expect_gap("0x00136C20", "CopDriver::Sub_00136C20", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C20(); });
    failures += !expect_gap("0x00136C30", "CopDriver::Sub_00136C30", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C30(); });
    failures += !expect_gap("0x00136C38", "CopDriver::Sub_00136C38", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C38(); });
    failures += !expect_gap("0x00136C40", "CopDriver::Sub_00136C40", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C40(); });
    failures += !expect_gap("0x00136C50", "CopDriver::Sub_00136C50", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C50(); });
    failures += !expect_gap("0x00136C58", "CopDriver::Sub_00136C58", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C58(); });
    failures += !expect_gap("0x00136C60", "CopDriver::Sub_00136C60", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C60(); });
    failures += !expect_gap("0x00136C68", "CopDriver::Sub_00136C68", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C68(); });
    failures += !expect_gap("0x00136C70", "CopDriver::Sub_00136C70", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C70(); });
    failures += !expect_gap("0x00136CE8", "CopDriver::Sub_00136CE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136CE8(); });
    failures += !expect_gap("0x00136CF0", "CopDriver::Sub_00136CF0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136CF0(); });
    failures += !expect_gap("0x00136D00", "CopDriver::Sub_00136D00", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136D00(); });
    failures += !expect_gap("0x001A1CE8", "CopDriver::Sub_001A1CE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A1CE8(); });
    failures += !expect_gap("0x001A1E30", "CopDriver::Sub_001A1E30", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A1E30(); });
    failures += !expect_gap("0x001A2110", "CopDriver::Sub_001A2110", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A2110(); });
    failures += !expect_gap("0x001A2B58", "CopDriver::Sub_001A2B58", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A2B58(); });
    failures += !expect_gap("0x001C0100", "CopDriver::Sub_001C0100", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001C0100(); });
    failures += !expect_gap("0x001C0140", "CopDriver::Sub_001C0140", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001C0140(); });
    failures += !expect_gap("0x001C0160", "CopDriver::Sub_001C0160", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001C0160(); });
    failures += !expect_gap("0x001C0198", "CopDriver::Sub_001C0198", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001C0198(); });
    failures += !expect_gap("0x0023A178", "CopDriver::Sub_0023A178", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A178(); });
    failures += !expect_gap("0x0023A198", "CopDriver::Sub_0023A198", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A198(); });
    failures += !expect_gap("0x0023A1C8", "CopDriver::Sub_0023A1C8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A1C8(); });
    failures += !expect_gap("0x0023A2F8", "CopDriver::Sub_0023A2F8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A2F8(); });
    failures += !expect_gap("0x0023A300", "CopDriver::Sub_0023A300", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A300(); });
    failures += !expect_gap("0x0023A3D0", "CopDriver::Sub_0023A3D0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_0023A3D0(); });
    failures += !expect_gap("0x002A0550", "CopDriver::Sub_002A0550", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_002A0550(); });
    failures += !expect_gap("0x001A0DD8", "CopMover::Sub_001A0DD8", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0DD8(); });
    failures += !expect_gap("0x001A0E88", "CopMover::Sub_001A0E88", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0E88(); });
    failures += !expect_gap("0x001A0F20", "CopMover::Sub_001A0F20", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0F20(); });
    failures += !expect_gap("0x001A1030", "CopMover::Sub_001A1030", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A1030(); });
    failures += !expect_gap("0x0019B130", "CountdownCameraCoordinator::Sub_0019B130", [] { hp2::recovered::CountdownCameraCoordinator instance; instance.Sub_0019B130(); });
    failures += !expect_gap("0x0019B188", "CountdownCameraCoordinator::Sub_0019B188", [] { hp2::recovered::CountdownCameraCoordinator instance; instance.Sub_0019B188(); });
    failures += !expect_gap("0x0019B1A0", "CountdownCameraCoordinator::Sub_0019B1A0", [] { hp2::recovered::CountdownCameraCoordinator instance; instance.Sub_0019B1A0(); });
    failures += !expect_gap("0x001577D0", "CubicCameraMover::Sub_001577D0", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "CubicCameraMover::Sub_001577D8", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "CubicCameraMover::Sub_00157850", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "CubicCameraMover::Sub_00157920", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015E7B0", "CubicCameraMover::Sub_0015E7B0", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015E7B0(); });
    failures += !expect_gap("0x0015EA30", "CubicCameraMover::Sub_0015EA30", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015EA30(); });
    failures += !expect_gap("0x0015ECE0", "CubicCameraMover::Sub_0015ECE0", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015ECE0(); });
    failures += !expect_gap("0x0015FA78", "CubicCameraMover::Sub_0015FA78", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FA78(); });
    failures += !expect_gap("0x0015FCB0", "CubicCameraMover::Sub_0015FCB0", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FD10", "CubicCameraMover::Sub_0015FD10", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "CubicCameraMover::Sub_0015FD18", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "CubicCameraMover::Sub_0015FD20", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "CubicCameraMover::Sub_0015FD28", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "CubicCameraMover::Sub_0015FD30", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "CubicCameraMover::Sub_0015FD38", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "CubicCameraMover::Sub_0015FD40", [] { hp2::recovered::CubicCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x001577D0", "DebugWorldCameraMover::Sub_001577D0", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "DebugWorldCameraMover::Sub_001577D8", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "DebugWorldCameraMover::Sub_00157850", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "DebugWorldCameraMover::Sub_00157920", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015B268", "DebugWorldCameraMover::Sub_0015B268", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015B268(); });
    failures += !expect_gap("0x0015B318", "DebugWorldCameraMover::Sub_0015B318", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015B318(); });
    failures += !expect_gap("0x0015B360", "DebugWorldCameraMover::Sub_0015B360", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015B360(); });
    failures += !expect_gap("0x0015B4D0", "DebugWorldCameraMover::Sub_0015B4D0", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015B4D0(); });
    failures += !expect_gap("0x0015B878", "DebugWorldCameraMover::Sub_0015B878", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015B878(); });
    failures += !expect_gap("0x0015FCB0", "DebugWorldCameraMover::Sub_0015FCB0", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FCF8", "DebugWorldCameraMover::Sub_0015FCF8", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FCF8(); });
    failures += !expect_gap("0x0015FD10", "DebugWorldCameraMover::Sub_0015FD10", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "DebugWorldCameraMover::Sub_0015FD18", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "DebugWorldCameraMover::Sub_0015FD20", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "DebugWorldCameraMover::Sub_0015FD28", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "DebugWorldCameraMover::Sub_0015FD30", [] { hp2::recovered::DebugWorldCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x00202F60", "DeliveryMover::Sub_00202F60", [] { hp2::recovered::DeliveryMover instance; instance.Sub_00202F60(); });
    failures += !expect_gap("0x00202F78", "DeliveryMover::Sub_00202F78", [] { hp2::recovered::DeliveryMover instance; instance.Sub_00202F78(); });
    failures += !expect_gap("0x002030C0", "DeliveryMover::Sub_002030C0", [] { hp2::recovered::DeliveryMover instance; instance.Sub_002030C0(); });
    failures += !expect_gap("0x002033D8", "DeliveryMover::Sub_002033D8", [] { hp2::recovered::DeliveryMover instance; instance.Sub_002033D8(); });
    failures += !expect_gap("0x0019A440", "DemoCameraCoordinator::Sub_0019A440", [] { hp2::recovered::DemoCameraCoordinator instance; instance.Sub_0019A440(); });
    failures += !expect_gap("0x0019A4A0", "DemoCameraCoordinator::Sub_0019A4A0", [] { hp2::recovered::DemoCameraCoordinator instance; instance.Sub_0019A4A0(); });
    failures += !expect_gap("0x0019A4B8", "DemoCameraCoordinator::Sub_0019A4B8", [] { hp2::recovered::DemoCameraCoordinator instance; instance.Sub_0019A4B8(); });
    failures += !expect_gap("0x0012E6C8", "DemoDiscManager::Init", [] { hp2::recovered::DemoDiscManager instance; instance.Init(); });
    failures += !expect_gap("0x0012EB00", "DemoDiscManager::SetEndReason", [] { hp2::recovered::DemoDiscManager instance; instance.SetEndReason(); });
    failures += !expect_gap("0x001577D0", "DriveCameraMover::Sub_001577D0", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "DriveCameraMover::Sub_001577D8", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "DriveCameraMover::Sub_00157850", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x0015C138", "DriveCameraMover::Sub_0015C138", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C138(); });
    failures += !expect_gap("0x0015C258", "DriveCameraMover::Sub_0015C258", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C258(); });
    failures += !expect_gap("0x0015C400", "DriveCameraMover::Sub_0015C400", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C400(); });
    failures += !expect_gap("0x0015C470", "DriveCameraMover::Sub_0015C470", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C470(); });
    failures += !expect_gap("0x0015C528", "DriveCameraMover::Sub_0015C528", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C528(); });
    failures += !expect_gap("0x0015C760", "DriveCameraMover::Sub_0015C760", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C760(); });
    failures += !expect_gap("0x0015C7A0", "DriveCameraMover::Sub_0015C7A0", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C7A0(); });
    failures += !expect_gap("0x0015C838", "DriveCameraMover::Sub_0015C838", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C838(); });
    failures += !expect_gap("0x0015C880", "DriveCameraMover::Sub_0015C880", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C880(); });
    failures += !expect_gap("0x0015FBF0", "DriveCameraMover::Sub_0015FBF0", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015FBF0(); });
    failures += !expect_gap("0x0015FC20", "DriveCameraMover::Sub_0015FC20", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015FC20(); });
    failures += !expect_gap("0x0015FD38", "DriveCameraMover::Sub_0015FD38", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "DriveCameraMover::Sub_0015FD40", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x001E12B8", "EndGameMenu::Sub_001E12B8", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E12B8(); });
    failures += !expect_gap("0x001E19E0", "EndGameMenu::Sub_001E19E0", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E19E0(); });
    failures += !expect_gap("0x001E2270", "EndGameMenu::Sub_001E2270", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2270(); });
    failures += !expect_gap("0x001E2580", "EndGameMenu::Sub_001E2580", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "EndGameMenu::Sub_001E2588", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "EndGameMenu::Sub_001E2590", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001DD0F0", "EventRewardScreen::Sub_001DD0F0", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001DEF38", "EventRewardScreen::Sub_001DEF38", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001DEF38(); });
    failures += !expect_gap("0x001E24F8", "EventRewardScreen::Sub_001E24F8", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E24F8(); });
    failures += !expect_gap("0x001E2580", "EventRewardScreen::Sub_001E2580", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "EventRewardScreen::Sub_001E2588", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "EventRewardScreen::Sub_001E2590", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001F4E68", "JoystickPreconfig::Sub_001F4E68", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_001F4E68(); });
    failures += !expect_gap("0x002400C0", "JoystickPreconfig::Sub_002400C0", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_002400C0(); });
    failures += !expect_gap("0x00240230", "JoystickPreconfig::Sub_00240230", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_00240230(); });
    failures += !expect_gap("0x002402C0", "JoystickPreconfig::Sub_002402C0", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_002402C0(); });
    failures += !expect_gap("0x001F02D8", "JukeBoxMusicScreen::Sub_001F02D8", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F02D8(); });
    failures += !expect_gap("0x001F0460", "JukeBoxMusicScreen::Sub_001F0460", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F0460(); });
    failures += !expect_gap("0x001F0D10", "JukeBoxMusicScreen::Sub_001F0D10", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F0D10(); });
    failures += !expect_gap("0x001F4E68", "JukeBoxMusicScreen::Sub_001F4E68", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F4E68(); });
    failures += !expect_gap("0x001577D0", "MaxCameraMover::Sub_001577D0", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "MaxCameraMover::Sub_001577D8", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "MaxCameraMover::Sub_00157850", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "MaxCameraMover::Sub_00157920", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015A340", "MaxCameraMover::Sub_0015A340", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015A340(); });
    failures += !expect_gap("0x0015A3F8", "MaxCameraMover::Sub_0015A3F8", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015A3F8(); });
    failures += !expect_gap("0x0015A410", "MaxCameraMover::Sub_0015A410", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015A410(); });
    failures += !expect_gap("0x0015FAF0", "MaxCameraMover::Sub_0015FAF0", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FAF0(); });
    failures += !expect_gap("0x0015FCB0", "MaxCameraMover::Sub_0015FCB0", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FD10", "MaxCameraMover::Sub_0015FD10", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "MaxCameraMover::Sub_0015FD18", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "MaxCameraMover::Sub_0015FD20", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "MaxCameraMover::Sub_0015FD28", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "MaxCameraMover::Sub_0015FD30", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "MaxCameraMover::Sub_0015FD38", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "MaxCameraMover::Sub_0015FD40", [] { hp2::recovered::MaxCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x0019B4B0", "NisCameraCoordinator::Sub_0019B4B0", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B4B0(); });
    failures += !expect_gap("0x0019B5F0", "NisCameraCoordinator::Sub_0019B5F0", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B5F0(); });
    failures += !expect_gap("0x0019B658", "NisCameraCoordinator::Sub_0019B658", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B658(); });
    failures += !expect_gap("0x001C0090", "NullController::Sub_001C0090", [] { hp2::recovered::NullController instance; instance.Sub_001C0090(); });
    failures += !expect_gap("0x001C00A8", "NullController::Sub_001C00A8", [] { hp2::recovered::NullController instance; instance.Sub_001C00A8(); });
    failures += !expect_gap("0x001C00D0", "NullController::Sub_001C00D0", [] { hp2::recovered::NullController instance; instance.Sub_001C00D0(); });
    failures += !expect_gap("0x001C0100", "NullController::Sub_001C0100", [] { hp2::recovered::NullController instance; instance.Sub_001C0100(); });
    failures += !expect_gap("0x001C0140", "NullController::Sub_001C0140", [] { hp2::recovered::NullController instance; instance.Sub_001C0140(); });
    failures += !expect_gap("0x001C0160", "NullController::Sub_001C0160", [] { hp2::recovered::NullController instance; instance.Sub_001C0160(); });
    failures += !expect_gap("0x001C0198", "NullController::Sub_001C0198", [] { hp2::recovered::NullController instance; instance.Sub_001C0198(); });
    failures += !expect_gap("0x002A0550", "NullController::Sub_002A0550", [] { hp2::recovered::NullController instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x001577D0", "OrbitCarCameraMover::Sub_001577D0", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "OrbitCarCameraMover::Sub_001577D8", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "OrbitCarCameraMover::Sub_00157850", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "OrbitCarCameraMover::Sub_00157920", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00157E48", "OrbitCarCameraMover::Sub_00157E48", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00157E48(); });
    failures += !expect_gap("0x00158080", "OrbitCarCameraMover::Sub_00158080", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00158080(); });
    failures += !expect_gap("0x001580E0", "OrbitCarCameraMover::Sub_001580E0", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_001580E0(); });
    failures += !expect_gap("0x00158100", "OrbitCarCameraMover::Sub_00158100", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00158100(); });
    failures += !expect_gap("0x001581F8", "OrbitCarCameraMover::Sub_001581F8", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_001581F8(); });
    failures += !expect_gap("0x00158298", "OrbitCarCameraMover::Sub_00158298", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_00158298(); });
    failures += !expect_gap("0x0015FB18", "OrbitCarCameraMover::Sub_0015FB18", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FB18(); });
    failures += !expect_gap("0x0015FD10", "OrbitCarCameraMover::Sub_0015FD10", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "OrbitCarCameraMover::Sub_0015FD18", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "OrbitCarCameraMover::Sub_0015FD20", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "OrbitCarCameraMover::Sub_0015FD28", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "OrbitCarCameraMover::Sub_0015FD30", [] { hp2::recovered::OrbitCarCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x001309F0", "PhysicsCar::Sub_001309F0", [] { hp2::recovered::PhysicsCar instance; instance.Sub_001309F0(); });
    failures += !expect_gap("0x00136B78", "PhysicsCar::Sub_00136B78", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00136B78(); });
    failures += !expect_gap("0x00137100", "PhysicsCar::Sub_00137100", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00137100(); });
    failures += !expect_gap("0x00137450", "PhysicsCar::Sub_00137450", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00137450(); });
    failures += !expect_gap("0x00137708", "PhysicsCar::Sub_00137708", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00137708(); });
    failures += !expect_gap("0x00137788", "PhysicsCar::Sub_00137788", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00137788(); });
    failures += !expect_gap("0x00137D90", "PhysicsCar::Sub_00137D90", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00137D90(); });
    failures += !expect_gap("0x00138E08", "PhysicsCar::Sub_00138E08", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00138E08(); });
    failures += !expect_gap("0x00139678", "PhysicsCar::Sub_00139678", [] { hp2::recovered::PhysicsCar instance; instance.Sub_00139678(); });
    failures += !expect_gap("0x0019C808", "PhysicsCar::Sub_0019C808", [] { hp2::recovered::PhysicsCar instance; instance.Sub_0019C808(); });
    failures += !expect_gap("0x0019C8F8", "PhysicsCar::Sub_0019C8F8", [] { hp2::recovered::PhysicsCar instance; instance.Sub_0019C8F8(); });
    failures += !expect_gap("0x001A5D10", "PhysicsCar::Sub_001A5D10", [] { hp2::recovered::PhysicsCar instance; instance.Sub_001A5D10(); });
    failures += !expect_gap("0x001A5D60", "PhysicsCar::Sub_001A5D60", [] { hp2::recovered::PhysicsCar instance; instance.Sub_001A5D60(); });
    failures += !expect_gap("0x001A66F0", "PhysicsCar::Sub_001A66F0", [] { hp2::recovered::PhysicsCar instance; instance.Sub_001A66F0(); });
    failures += !expect_gap("0x002A0550", "PhysicsCar::Sub_002A0550", [] { hp2::recovered::PhysicsCar instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x0018D830", "PlayerCarSound::Sub_0018D830", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018D830(); });
    failures += !expect_gap("0x0018D8D8", "PlayerCarSound::Sub_0018D8D8", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018D8D8(); });
    failures += !expect_gap("0x0018D9A0", "PlayerCarSound::Sub_0018D9A0", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018D9A0(); });
    failures += !expect_gap("0x0018DB78", "PlayerCarSound::Sub_0018DB78", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018DB78(); });
    failures += !expect_gap("0x0018DBF0", "PlayerCarSound::Sub_0018DBF0", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018DBF0(); });
    failures += !expect_gap("0x0018DD10", "PlayerCarSound::Sub_0018DD10", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018DD10(); });
    failures += !expect_gap("0x0018DDA0", "PlayerCarSound::Sub_0018DDA0", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018DDA0(); });
    failures += !expect_gap("0x0018F160", "PlayerCarSound::Sub_0018F160", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018F160(); });
    failures += !expect_gap("0x0018F188", "PlayerCarSound::Sub_0018F188", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018F188(); });
    failures += !expect_gap("0x0018F208", "PlayerCarSound::Sub_0018F208", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018F208(); });
    failures += !expect_gap("0x0018F218", "PlayerCarSound::Sub_0018F218", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018F218(); });
    failures += !expect_gap("0x0018F2B0", "PlayerCarSound::Sub_0018F2B0", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018F2B0(); });
    failures += !expect_gap("0x00208B28", "PlayerCarSound::Sub_00208B28", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "PlayerCarSound::Sub_00208C78", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x00208B28", "PlayerEngineSound::Sub_00208B28", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "PlayerEngineSound::Sub_00208C78", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x0020FC70", "PlayerEngineSound::Sub_0020FC70", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_0020FC70(); });
    failures += !expect_gap("0x0020FE70", "PlayerEngineSound::Sub_0020FE70", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_0020FE70(); });
    failures += !expect_gap("0x00210400", "PlayerEngineSound::Sub_00210400", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00210400(); });
    failures += !expect_gap("0x00210408", "PlayerEngineSound::Sub_00210408", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00210408(); });
    failures += !expect_gap("0x00210410", "PlayerEngineSound::Sub_00210410", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00210410(); });
    failures += !expect_gap("0x00211390", "PlayerEngineSound::Sub_00211390", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_00211390(); });
    failures += !expect_gap("0x00208B28", "PlayerTireSound::Sub_00208B28", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "PlayerTireSound::Sub_00208C78", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x00212950", "PlayerTireSound::Sub_00212950", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212950(); });
    failures += !expect_gap("0x00212CF0", "PlayerTireSound::Sub_00212CF0", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212CF0(); });
    failures += !expect_gap("0x00212E88", "PlayerTireSound::Sub_00212E88", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212E88(); });
    failures += !expect_gap("0x00212E90", "PlayerTireSound::Sub_00212E90", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212E90(); });
    failures += !expect_gap("0x002134D8", "PlayerTireSound::Sub_002134D8", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_002134D8(); });
    failures += !expect_gap("0x002135A0", "PlayerTireSound::Sub_002135A0", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_002135A0(); });
    failures += !expect_gap("0x002149F8", "PlayerTireSound::Sub_002149F8", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_002149F8(); });
    failures += !expect_gap("0x00214A00", "PlayerTireSound::Sub_00214A00", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00214A00(); });
    failures += !expect_gap("0x00130958", "PsychoDriver::Sub_00130958", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00130958(); });
    failures += !expect_gap("0x00136C00", "PsychoDriver::Sub_00136C00", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C00(); });
    failures += !expect_gap("0x00136C08", "PsychoDriver::Sub_00136C08", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C08(); });
    failures += !expect_gap("0x00136C10", "PsychoDriver::Sub_00136C10", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C10(); });
    failures += !expect_gap("0x00136C20", "PsychoDriver::Sub_00136C20", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C20(); });
    failures += !expect_gap("0x00136C30", "PsychoDriver::Sub_00136C30", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C30(); });
    failures += !expect_gap("0x00136C38", "PsychoDriver::Sub_00136C38", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C38(); });
    failures += !expect_gap("0x00136C40", "PsychoDriver::Sub_00136C40", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C40(); });
    failures += !expect_gap("0x00136C50", "PsychoDriver::Sub_00136C50", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C50(); });
    failures += !expect_gap("0x00136C58", "PsychoDriver::Sub_00136C58", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C58(); });
    failures += !expect_gap("0x00136C60", "PsychoDriver::Sub_00136C60", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C60(); });
    failures += !expect_gap("0x00136C68", "PsychoDriver::Sub_00136C68", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C68(); });
    failures += !expect_gap("0x00136C70", "PsychoDriver::Sub_00136C70", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C70(); });
    failures += !expect_gap("0x00136CE8", "PsychoDriver::Sub_00136CE8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136CE8(); });
    failures += !expect_gap("0x00136CF0", "PsychoDriver::Sub_00136CF0", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136CF0(); });
    failures += !expect_gap("0x00136D00", "PsychoDriver::Sub_00136D00", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136D00(); });
    failures += !expect_gap("0x00139760", "PsychoDriver::Sub_00139760", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139760(); });
    failures += !expect_gap("0x001398E8", "PsychoDriver::Sub_001398E8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_001398E8(); });
    failures += !expect_gap("0x00139968", "PsychoDriver::Sub_00139968", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139968(); });
    failures += !expect_gap("0x00139980", "PsychoDriver::Sub_00139980", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139980(); });
    failures += !expect_gap("0x00139B28", "PsychoDriver::Sub_00139B28", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139B28(); });
    failures += !expect_gap("0x00139B68", "PsychoDriver::Sub_00139B68", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139B68(); });
    failures += !expect_gap("0x00139C48", "PsychoDriver::Sub_00139C48", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139C48(); });
    failures += !expect_gap("0x0013A3E0", "PsychoDriver::Sub_0013A3E0", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013A3E0(); });
    failures += !expect_gap("0x0013A638", "PsychoDriver::Sub_0013A638", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013A638(); });
    failures += !expect_gap("0x0013A658", "PsychoDriver::Sub_0013A658", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013A658(); });
    failures += !expect_gap("0x0013A678", "PsychoDriver::Sub_0013A678", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013A678(); });
    failures += !expect_gap("0x0013AC80", "PsychoDriver::Sub_0013AC80", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013AC80(); });
    failures += !expect_gap("0x0013ACD8", "PsychoDriver::Sub_0013ACD8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013ACD8(); });
    failures += !expect_gap("0x0013ACE8", "PsychoDriver::Sub_0013ACE8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013ACE8(); });
    failures += !expect_gap("0x0013ACF0", "PsychoDriver::Sub_0013ACF0", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013ACF0(); });
    failures += !expect_gap("0x0013AD00", "PsychoDriver::Sub_0013AD00", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013AD00(); });
    failures += !expect_gap("0x0013AD10", "PsychoDriver::Sub_0013AD10", [] { hp2::recovered::PsychoDriver instance; instance.Sub_0013AD10(); });
    failures += !expect_gap("0x002A0550", "PsychoDriver::Sub_002A0550", [] { hp2::recovered::PsychoDriver instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x001DD0F0", "RaceResultsScreen::Sub_001DD0F0", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001DFFE8", "RaceResultsScreen::Sub_001DFFE8", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001DFFE8(); });
    failures += !expect_gap("0x001E0838", "RaceResultsScreen::Sub_001E0838", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001E0838(); });
    failures += !expect_gap("0x001E0868", "RaceResultsScreen::Sub_001E0868", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001E0868(); });
    failures += !expect_gap("0x001E23B8", "RaceResultsScreen::Sub_001E23B8", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001E23B8(); });
    failures += !expect_gap("0x001E2580", "RaceResultsScreen::Sub_001E2580", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001C0100", "RealController::Sub_001C0100", [] { hp2::recovered::RealController instance; instance.Sub_001C0100(); });
    failures += !expect_gap("0x001C0140", "RealController::Sub_001C0140", [] { hp2::recovered::RealController instance; instance.Sub_001C0140(); });
    failures += !expect_gap("0x001C0160", "RealController::Sub_001C0160", [] { hp2::recovered::RealController instance; instance.Sub_001C0160(); });
    failures += !expect_gap("0x001C0198", "RealController::Sub_001C0198", [] { hp2::recovered::RealController instance; instance.Sub_001C0198(); });
    failures += !expect_gap("0x0023A178", "RealController::Sub_0023A178", [] { hp2::recovered::RealController instance; instance.Sub_0023A178(); });
    failures += !expect_gap("0x0023A198", "RealController::Sub_0023A198", [] { hp2::recovered::RealController instance; instance.Sub_0023A198(); });
    failures += !expect_gap("0x0023A1C8", "RealController::Sub_0023A1C8", [] { hp2::recovered::RealController instance; instance.Sub_0023A1C8(); });
    failures += !expect_gap("0x0023A2F8", "RealController::Sub_0023A2F8", [] { hp2::recovered::RealController instance; instance.Sub_0023A2F8(); });
    failures += !expect_gap("0x0023A300", "RealController::Sub_0023A300", [] { hp2::recovered::RealController instance; instance.Sub_0023A300(); });
    failures += !expect_gap("0x0023A3D0", "RealController::Sub_0023A3D0", [] { hp2::recovered::RealController instance; instance.Sub_0023A3D0(); });
    failures += !expect_gap("0x0023A3D8", "RealController::Sub_0023A3D8", [] { hp2::recovered::RealController instance; instance.Sub_0023A3D8(); });
    failures += !expect_gap("0x0023A4D8", "RealController::Sub_0023A4D8", [] { hp2::recovered::RealController instance; instance.Sub_0023A4D8(); });
    failures += !expect_gap("0x0023A560", "RealController::Sub_0023A560", [] { hp2::recovered::RealController instance; instance.Sub_0023A560(); });
    failures += !expect_gap("0x0023AA50", "RealController::Sub_0023AA50", [] { hp2::recovered::RealController instance; instance.Sub_0023AA50(); });
    failures += !expect_gap("0x00130958", "RealDriver::Sub_00130958", [] { hp2::recovered::RealDriver instance; instance.Sub_00130958(); });
    failures += !expect_gap("0x00136C00", "RealDriver::Sub_00136C00", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C00(); });
    failures += !expect_gap("0x00136C08", "RealDriver::Sub_00136C08", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C08(); });
    failures += !expect_gap("0x00136C10", "RealDriver::Sub_00136C10", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C10(); });
    failures += !expect_gap("0x00136C20", "RealDriver::Sub_00136C20", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C20(); });
    failures += !expect_gap("0x00136C30", "RealDriver::Sub_00136C30", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C30(); });
    failures += !expect_gap("0x00136C38", "RealDriver::Sub_00136C38", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C38(); });
    failures += !expect_gap("0x00136C40", "RealDriver::Sub_00136C40", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C40(); });
    failures += !expect_gap("0x00136C50", "RealDriver::Sub_00136C50", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C50(); });
    failures += !expect_gap("0x00136C58", "RealDriver::Sub_00136C58", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C58(); });
    failures += !expect_gap("0x00136C60", "RealDriver::Sub_00136C60", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C60(); });
    failures += !expect_gap("0x00136C68", "RealDriver::Sub_00136C68", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C68(); });
    failures += !expect_gap("0x00136C70", "RealDriver::Sub_00136C70", [] { hp2::recovered::RealDriver instance; instance.Sub_00136C70(); });
    failures += !expect_gap("0x00136CE8", "RealDriver::Sub_00136CE8", [] { hp2::recovered::RealDriver instance; instance.Sub_00136CE8(); });
    failures += !expect_gap("0x00136CF0", "RealDriver::Sub_00136CF0", [] { hp2::recovered::RealDriver instance; instance.Sub_00136CF0(); });
    failures += !expect_gap("0x00136D00", "RealDriver::Sub_00136D00", [] { hp2::recovered::RealDriver instance; instance.Sub_00136D00(); });
    failures += !expect_gap("0x0013AD90", "RealDriver::Sub_0013AD90", [] { hp2::recovered::RealDriver instance; instance.Sub_0013AD90(); });
    failures += !expect_gap("0x0013AE58", "RealDriver::Sub_0013AE58", [] { hp2::recovered::RealDriver instance; instance.Sub_0013AE58(); });
    failures += !expect_gap("0x0013AEB8", "RealDriver::Sub_0013AEB8", [] { hp2::recovered::RealDriver instance; instance.Sub_0013AEB8(); });
    failures += !expect_gap("0x0013B080", "RealDriver::Sub_0013B080", [] { hp2::recovered::RealDriver instance; instance.Sub_0013B080(); });
    failures += !expect_gap("0x0013B250", "RealDriver::Sub_0013B250", [] { hp2::recovered::RealDriver instance; instance.Sub_0013B250(); });
    failures += !expect_gap("0x0013B3D0", "RealDriver::Sub_0013B3D0", [] { hp2::recovered::RealDriver instance; instance.Sub_0013B3D0(); });
    failures += !expect_gap("0x0013B3F8", "RealDriver::Sub_0013B3F8", [] { hp2::recovered::RealDriver instance; instance.Sub_0013B3F8(); });
    failures += !expect_gap("0x0013C420", "RealDriver::Sub_0013C420", [] { hp2::recovered::RealDriver instance; instance.Sub_0013C420(); });
    failures += !expect_gap("0x0013C460", "RealDriver::Sub_0013C460", [] { hp2::recovered::RealDriver instance; instance.Sub_0013C460(); });
    failures += !expect_gap("0x0013CAE0", "RealDriver::Sub_0013CAE0", [] { hp2::recovered::RealDriver instance; instance.Sub_0013CAE0(); });
    failures += !expect_gap("0x0013CAF0", "RealDriver::Sub_0013CAF0", [] { hp2::recovered::RealDriver instance; instance.Sub_0013CAF0(); });
    failures += !expect_gap("0x002A0550", "RealDriver::Sub_002A0550", [] { hp2::recovered::RealDriver instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x001577D0", "RearViewMirrorCameraMover::Sub_001577D0", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "RearViewMirrorCameraMover::Sub_001577D8", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "RearViewMirrorCameraMover::Sub_00157850", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "RearViewMirrorCameraMover::Sub_00157920", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00158F78", "RearViewMirrorCameraMover::Sub_00158F78", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_00158F78(); });
    failures += !expect_gap("0x00158FE0", "RearViewMirrorCameraMover::Sub_00158FE0", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_00158FE0(); });
    failures += !expect_gap("0x00158FF8", "RearViewMirrorCameraMover::Sub_00158FF8", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_00158FF8(); });
    failures += !expect_gap("0x0015FB08", "RearViewMirrorCameraMover::Sub_0015FB08", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FB08(); });
    failures += !expect_gap("0x0015FCB0", "RearViewMirrorCameraMover::Sub_0015FCB0", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FD10", "RearViewMirrorCameraMover::Sub_0015FD10", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "RearViewMirrorCameraMover::Sub_0015FD18", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "RearViewMirrorCameraMover::Sub_0015FD20", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "RearViewMirrorCameraMover::Sub_0015FD28", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "RearViewMirrorCameraMover::Sub_0015FD30", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "RearViewMirrorCameraMover::Sub_0015FD38", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "RearViewMirrorCameraMover::Sub_0015FD40", [] { hp2::recovered::RearViewMirrorCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x0018F388", "SirenSound::Sub_0018F388", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F388(); });
    failures += !expect_gap("0x0018F4D0", "SirenSound::Sub_0018F4D0", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F4D0(); });
    failures += !expect_gap("0x0018F508", "SirenSound::Sub_0018F508", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F508(); });
    failures += !expect_gap("0x0018F528", "SirenSound::Sub_0018F528", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F528(); });
    failures += !expect_gap("0x0018F548", "SirenSound::Sub_0018F548", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F548(); });
    failures += !expect_gap("0x0018F710", "SirenSound::Sub_0018F710", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F710(); });
    failures += !expect_gap("0x00208B28", "SirenSound::Sub_00208B28", [] { hp2::recovered::SirenSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "SirenSound::Sub_00208C78", [] { hp2::recovered::SirenSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x00257F28", "SlotPool::CleanupExpandedSlotPools", [] { hp2::recovered::SlotPool instance; instance.CleanupExpandedSlotPools(); });
    failures += !expect_gap("0x002584B0", "SlotPoolManager::NewSlotPool", [] { hp2::recovered::SlotPoolManager instance; instance.NewSlotPool(); });
    failures += !expect_gap("0x00258548", "SlotPoolManager::DeleteSlotPool", [] { hp2::recovered::SlotPoolManager instance; instance.DeleteSlotPool(); });
    failures += !expect_gap("0x001577D0", "StillCameraMover::Sub_001577D0", [] { hp2::recovered::StillCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "StillCameraMover::Sub_001577D8", [] { hp2::recovered::StillCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "StillCameraMover::Sub_00157850", [] { hp2::recovered::StillCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "StillCameraMover::Sub_00157920", [] { hp2::recovered::StillCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015E6A0", "StillCameraMover::Sub_0015E6A0", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015E6A0(); });
    failures += !expect_gap("0x0015E718", "StillCameraMover::Sub_0015E718", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015E718(); });
    failures += !expect_gap("0x0015E730", "StillCameraMover::Sub_0015E730", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015E730(); });
    failures += !expect_gap("0x0015FCB0", "StillCameraMover::Sub_0015FCB0", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FCF8", "StillCameraMover::Sub_0015FCF8", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FCF8(); });
    failures += !expect_gap("0x0015FD10", "StillCameraMover::Sub_0015FD10", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "StillCameraMover::Sub_0015FD18", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "StillCameraMover::Sub_0015FD20", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "StillCameraMover::Sub_0015FD28", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "StillCameraMover::Sub_0015FD30", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "StillCameraMover::Sub_0015FD38", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "StillCameraMover::Sub_0015FD40", [] { hp2::recovered::StillCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x0016F190", "TitleScreen::Sub_0016F190", [] { hp2::frontend::TitleScreen instance; instance.Sub_0016F190(); });
    failures += !expect_gap("0x001DD0F0", "TourneyStandingsScreen::Sub_001DD0F0", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001DD0F0(); });
    failures += !expect_gap("0x001DD5D0", "TourneyStandingsScreen::Sub_001DD5D0", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001DD5D0(); });
    failures += !expect_gap("0x001E23A0", "TourneyStandingsScreen::Sub_001E23A0", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E23A0(); });
    failures += !expect_gap("0x001E2580", "TourneyStandingsScreen::Sub_001E2580", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E2580(); });
    failures += !expect_gap("0x001E2588", "TourneyStandingsScreen::Sub_001E2588", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "TourneyStandingsScreen::Sub_001E2590", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001577D0", "TrackCarCameraMover::Sub_001577D0", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "TrackCarCameraMover::Sub_001577D8", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "TrackCarCameraMover::Sub_00157850", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "TrackCarCameraMover::Sub_00157920", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00159130", "TrackCarCameraMover::Sub_00159130", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159130(); });
    failures += !expect_gap("0x00159490", "TrackCarCameraMover::Sub_00159490", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159490(); });
    failures += !expect_gap("0x00159520", "TrackCarCameraMover::Sub_00159520", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159520(); });
    failures += !expect_gap("0x00159588", "TrackCarCameraMover::Sub_00159588", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159588(); });
    failures += !expect_gap("0x0015FB00", "TrackCarCameraMover::Sub_0015FB00", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FB00(); });
    failures += !expect_gap("0x0015FD10", "TrackCarCameraMover::Sub_0015FD10", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "TrackCarCameraMover::Sub_0015FD18", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "TrackCarCameraMover::Sub_0015FD20", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "TrackCarCameraMover::Sub_0015FD28", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "TrackCarCameraMover::Sub_0015FD30", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "TrackCarCameraMover::Sub_0015FD38", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "TrackCarCameraMover::Sub_0015FD40", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x001577D0", "TrackFlyCameraMover::Sub_001577D0", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "TrackFlyCameraMover::Sub_001577D8", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "TrackFlyCameraMover::Sub_00157850", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "TrackFlyCameraMover::Sub_00157920", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00159D08", "TrackFlyCameraMover::Sub_00159D08", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00159D08(); });
    failures += !expect_gap("0x00159DE8", "TrackFlyCameraMover::Sub_00159DE8", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00159DE8(); });
    failures += !expect_gap("0x00159E78", "TrackFlyCameraMover::Sub_00159E78", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00159E78(); });
    failures += !expect_gap("0x00159E98", "TrackFlyCameraMover::Sub_00159E98", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_00159E98(); });
    failures += !expect_gap("0x0015FAF8", "TrackFlyCameraMover::Sub_0015FAF8", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FAF8(); });
    failures += !expect_gap("0x0015FD10", "TrackFlyCameraMover::Sub_0015FD10", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "TrackFlyCameraMover::Sub_0015FD18", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "TrackFlyCameraMover::Sub_0015FD20", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "TrackFlyCameraMover::Sub_0015FD28", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "TrackFlyCameraMover::Sub_0015FD30", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "TrackFlyCameraMover::Sub_0015FD38", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "TrackFlyCameraMover::Sub_0015FD40", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x001577D0", "TrackRouteEditorCameraMover::Sub_001577D0", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "TrackRouteEditorCameraMover::Sub_001577D8", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "TrackRouteEditorCameraMover::Sub_00157850", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "TrackRouteEditorCameraMover::Sub_00157920", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015C010", "TrackRouteEditorCameraMover::Sub_0015C010", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015C010(); });
    failures += !expect_gap("0x0015C058", "TrackRouteEditorCameraMover::Sub_0015C058", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015C058(); });
    failures += !expect_gap("0x0015C0A0", "TrackRouteEditorCameraMover::Sub_0015C0A0", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015C0A0(); });
    failures += !expect_gap("0x0015FCB0", "TrackRouteEditorCameraMover::Sub_0015FCB0", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FCF8", "TrackRouteEditorCameraMover::Sub_0015FCF8", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FCF8(); });
    failures += !expect_gap("0x0015FD10", "TrackRouteEditorCameraMover::Sub_0015FD10", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "TrackRouteEditorCameraMover::Sub_0015FD18", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "TrackRouteEditorCameraMover::Sub_0015FD20", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "TrackRouteEditorCameraMover::Sub_0015FD28", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "TrackRouteEditorCameraMover::Sub_0015FD30", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "TrackRouteEditorCameraMover::Sub_0015FD38", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "TrackRouteEditorCameraMover::Sub_0015FD40", [] { hp2::recovered::TrackRouteEditorCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x00167A08", "TrafficCar::Sub_00167A08", [] { hp2::recovered::TrafficCar instance; instance.Sub_00167A08(); });
    failures += !expect_gap("0x00167A60", "TrafficCar::Sub_00167A60", [] { hp2::recovered::TrafficCar instance; instance.Sub_00167A60(); });
    failures += !expect_gap("0x00167AF8", "TrafficCar::Sub_00167AF8", [] { hp2::recovered::TrafficCar instance; instance.Sub_00167AF8(); });
    failures += !expect_gap("0x00168088", "TrafficCar::Sub_00168088", [] { hp2::recovered::TrafficCar instance; instance.Sub_00168088(); });
    failures += !expect_gap("0x0018C380", "TrafficDriver::Sub_0018C380", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018C380(); });
    failures += !expect_gap("0x0018C418", "TrafficDriver::Sub_0018C418", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018C418(); });
    failures += !expect_gap("0x0018C478", "TrafficDriver::Sub_0018C478", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018C478(); });
    failures += !expect_gap("0x0018C4D8", "TrafficDriver::Sub_0018C4D8", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018C4D8(); });
    failures += !expect_gap("0x0018CAE8", "TrafficDriver::Sub_0018CAE8", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018CAE8(); });
    failures += !expect_gap("0x0018CB10", "TrafficDriver::Sub_0018CB10", [] { hp2::recovered::TrafficDriver instance; instance.Sub_0018CB10(); });
    failures += !expect_gap("0x00190670", "TrafficDriver::Sub_00190670", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00190670(); });
    failures += !expect_gap("0x00190708", "TrafficDriver::Sub_00190708", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00190708(); });
    failures += !expect_gap("0x00190760", "TrafficDriver::Sub_00190760", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00190760(); });
    failures += !expect_gap("0x001907B8", "TrafficDriver::Sub_001907B8", [] { hp2::recovered::TrafficDriver instance; instance.Sub_001907B8(); });
    failures += !expect_gap("0x00191138", "TrafficDriver::Sub_00191138", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00191138(); });
    failures += !expect_gap("0x00192668", "TrafficDriver::Sub_00192668", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00192668(); });
    failures += !expect_gap("0x00192780", "TrafficDriver::Sub_00192780", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00192780(); });
    failures += !expect_gap("0x00192F10", "TrafficDriver::Sub_00192F10", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00192F10(); });
    failures += !expect_gap("0x00193650", "TrafficDriver::Sub_00193650", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00193650(); });
    failures += !expect_gap("0x00193758", "TrafficDriver::Sub_00193758", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00193758(); });
    failures += !expect_gap("0x00193780", "TrafficDriver::Sub_00193780", [] { hp2::recovered::TrafficDriver instance; instance.Sub_00193780(); });
    failures += !expect_gap("0x001577D0", "ZoneFreezeCameraMover::Sub_001577D0", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "ZoneFreezeCameraMover::Sub_001577D8", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "ZoneFreezeCameraMover::Sub_00157850", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "ZoneFreezeCameraMover::Sub_00157920", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015FCB0", "ZoneFreezeCameraMover::Sub_0015FCB0", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FCF8", "ZoneFreezeCameraMover::Sub_0015FCF8", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FCF8(); });
    failures += !expect_gap("0x0015FD10", "ZoneFreezeCameraMover::Sub_0015FD10", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "ZoneFreezeCameraMover::Sub_0015FD18", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "ZoneFreezeCameraMover::Sub_0015FD20", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "ZoneFreezeCameraMover::Sub_0015FD28", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "ZoneFreezeCameraMover::Sub_0015FD30", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "ZoneFreezeCameraMover::Sub_0015FD38", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "ZoneFreezeCameraMover::Sub_0015FD40", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x00160298", "ZoneFreezeCameraMover::Sub_00160298", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_00160298(); });
    failures += !expect_gap("0x00161000", "ZoneFreezeCameraMover::Sub_00161000", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_00161000(); });
    failures += !expect_gap("0x001610A8", "ZoneFreezeCameraMover::Sub_001610A8", [] { hp2::recovered::ZoneFreezeCameraMover instance; instance.Sub_001610A8(); });
    failures += !expect_gap("0x00157920", "ZonePreviewCameraMover::Sub_00157920", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x0015FCB0", "ZonePreviewCameraMover::Sub_0015FCB0", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FCB0(); });
    failures += !expect_gap("0x0015FCF8", "ZonePreviewCameraMover::Sub_0015FCF8", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FCF8(); });
    failures += !expect_gap("0x0015FD10", "ZonePreviewCameraMover::Sub_0015FD10", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "ZonePreviewCameraMover::Sub_0015FD18", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "ZonePreviewCameraMover::Sub_0015FD20", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "ZonePreviewCameraMover::Sub_0015FD28", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "ZonePreviewCameraMover::Sub_0015FD30", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "ZonePreviewCameraMover::Sub_0015FD38", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "ZonePreviewCameraMover::Sub_0015FD40", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x00161F98", "ZonePreviewCameraMover::Sub_00161F98", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_00161F98(); });
    failures += !expect_gap("0x00163620", "ZonePreviewCameraMover::Sub_00163620", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_00163620(); });
    failures += !expect_gap("0x00163790", "ZonePreviewCameraMover::Sub_00163790", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_00163790(); });
    failures += !expect_gap("0x00163978", "ZonePreviewCameraMover::Sub_00163978", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_00163978(); });
    failures += !expect_gap("0x001639A0", "ZonePreviewCameraMover::Sub_001639A0", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_001639A0(); });
    failures += !expect_gap("0x001639D0", "ZonePreviewCameraMover::Sub_001639D0", [] { hp2::recovered::ZonePreviewCameraMover instance; instance.Sub_001639D0(); });
    failures += !expect_gap("0x00208B28", "kAITireSound::Sub_00208B28", [] { hp2::recovered::kAITireSound instance; instance.Sub_00208B28(); });
    failures += !expect_gap("0x00208C78", "kAITireSound::Sub_00208C78", [] { hp2::recovered::kAITireSound instance; instance.Sub_00208C78(); });
    failures += !expect_gap("0x002124F0", "kAITireSound::Sub_002124F0", [] { hp2::recovered::kAITireSound instance; instance.Sub_002124F0(); });
    failures += !expect_gap("0x002125A0", "kAITireSound::Sub_002125A0", [] { hp2::recovered::kAITireSound instance; instance.Sub_002125A0(); });
    failures += !expect_gap("0x00214A68", "kAITireSound::Sub_00214A68", [] { hp2::recovered::kAITireSound instance; instance.Sub_00214A68(); });
    failures += !expect_gap("0x00214A70", "kAITireSound::Sub_00214A70", [] { hp2::recovered::kAITireSound instance; instance.Sub_00214A70(); });
    failures += !expect_gap("0x00214A78", "kAITireSound::Sub_00214A78", [] { hp2::recovered::kAITireSound instance; instance.Sub_00214A78(); });
    failures += !expect_gap("0x002A0550", "kAITireSound::Sub_002A0550", [] { hp2::recovered::kAITireSound instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x001F3F20", "tCustomizeHUDScreen::Sub_001F3F20", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F3F20(); });
    failures += !expect_gap("0x001F4178", "tCustomizeHUDScreen::Sub_001F4178", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F4178(); });
    failures += !expect_gap("0x001F46D8", "tCustomizeHUDScreen::Sub_001F46D8", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F46D8(); });
    failures += !expect_gap("0x001F4E68", "tCustomizeHUDScreen::Sub_001F4E68", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F4E68(); });
    failures += !expect_gap("0x001F33B8", "tDisplayOptionsScreen::Sub_001F33B8", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F33B8(); });
    failures += !expect_gap("0x001F3588", "tDisplayOptionsScreen::Sub_001F3588", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F3588(); });
    failures += !expect_gap("0x001F35E0", "tDisplayOptionsScreen::Sub_001F35E0", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F35E0(); });
    failures += !expect_gap("0x001F3A28", "tDisplayOptionsScreen::Sub_001F3A28", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F3A28(); });
    failures += !expect_gap("0x001F6FA0", "tEventLeaf::Sub_001F6FA0", [] { hp2::frontend::tEventLeaf instance; instance.Sub_001F6FA0(); });
    failures += !expect_gap("0x001EEBB8", "tOptionsScreen::Sub_001EEBB8", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EEBB8(); });
    failures += !expect_gap("0x001EEC50", "tOptionsScreen::Sub_001EEC50", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EEC50(); });
    failures += !expect_gap("0x001EECA8", "tOptionsScreen::Sub_001EECA8", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EECA8(); });
    failures += !expect_gap("0x001F4E68", "tOptionsScreen::Sub_001F4E68", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001F4E68(); });
    std::cout << "CLASS_TEMPLATE_GAPS_CHECKED: 603\n";
    return failures ? 1 : 0;
}

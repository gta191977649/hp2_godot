// TASK-0028 generated exception contract tests. No original-behaviour equivalence claim.
#include "game_core/AICarSound.hpp"
#include "game_core/AIDriver.hpp"
#include "game_core/AIEngineSound.hpp"
#include "game_core/AnimationGroup.hpp"
#include "game_core/Autosaver.hpp"
#include "game_core/BaseAICarSound.hpp"
#include "game_core/BasePlayerCarSound.hpp"
#include "game_core/BaseTireSound.hpp"
#include "game_core/BeACopChallengeSetup.hpp"
#include "game_core/BeACopResults1P.hpp"
#include "game_core/BeACopResults2P.hpp"
#include "game_core/BeTheCopSetup.hpp"
#include "game_core/BestLapTimes.hpp"
#include "game_core/BestTotalTimes.hpp"
#include "game_core/BootMemcardLoader.hpp"
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
#include "game_core/DemoCameraCoordinator.hpp"
#include "game_core/DuplicatedSolid.hpp"
#include "game_core/DuplicatedStripEntryData.hpp"
#include "game_core/DuplicatedStripEntryTable.hpp"
#include "game_core/EndGameMenu.hpp"
#include "game_core/EndGameMenuItem.hpp"
#include "game_core/EndGameMenusBase.hpp"
#include "game_core/EventRewardScreen.hpp"
#include "game_core/FernShakerInfoTable.hpp"
#include "game_core/FlailerScreenList.hpp"
#include "game_core/FreeRunSetup.hpp"
#include "game_core/GlobalBackgroundAnimation.hpp"
#include "game_core/HeliSheet.hpp"
#include "game_core/HelpAlphaAnimation.hpp"
#include "game_core/HornSound3d.hpp"
#include "game_core/HudTexturePack.hpp"
#include "game_core/JoystickCalibrationScreen.hpp"
#include "game_core/LanguageBuffer.hpp"
#include "game_core/LapKnockoutSetup.hpp"
#include "game_core/LapTimesDisplay.hpp"
#include "game_core/MumboJumboScreen.hpp"
#include "game_core/MunitionEffectsHandler.hpp"
#include "game_core/NameEntry.hpp"
#include "game_core/NewMainMenu.hpp"
#include "game_core/NullController.hpp"
#include "game_core/NumberAllocator.hpp"
#include "game_core/PauseAudioOpts.hpp"
#include "game_core/PauseDisplayOpts.hpp"
#include "game_core/PauseMenu.hpp"
#include "game_core/PauseOptions.hpp"
#include "game_core/PauseRaceObjective.hpp"
#include "game_core/PauseViewControls.hpp"
#include "game_core/PlayerEngineSound.hpp"
#include "game_core/PlayerTireSound.hpp"
#include "game_core/PreRace2PBeACop.hpp"
#include "game_core/PreRaceBlurb.hpp"
#include "game_core/PreRaceStats.hpp"
#include "game_core/Race.hpp"
#include "game_core/RaceResultsScreen.hpp"
#include "game_core/RealController.hpp"
#include "game_core/ReplayLogBuffer.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/ResourceFile.hpp"
#include "game_core/RewardTrackModel.hpp"
#include "game_core/SelectTrackScreen.hpp"
#include "game_core/SelectableTrack.hpp"
#include "game_core/SelectionSet.hpp"
#include "game_core/SingleRaceSetup.hpp"
#include "game_core/SirenSound.hpp"
#include "game_core/SirenSound3d.hpp"
#include "game_core/SlotPoolManager.hpp"
#include "game_core/TerrainEffectEmitters.hpp"
#include "game_core/TextureTable.hpp"
#include "game_core/TimeOfDayTrack.hpp"
#include "game_core/TourneyPoints.hpp"
#include "game_core/TourneyStandingsScreen.hpp"
#include "game_core/TrackKnockoutSetup.hpp"
#include "game_core/TrackMap.hpp"
#include "game_core/TrackMarkerFile.hpp"
#include "game_core/TrackRouteEditor.hpp"
#include "game_core/TrackRouteEditorCameraMover.hpp"
#include "game_core/TrafficTeleporter.hpp"
#include "game_core/eAnimation.hpp"
#include "game_core/eFrameMallocBuffer0.hpp"
#include "game_core/eFrameMallocBuffer1.hpp"
#include "game_core/feAnimAlphaElement.hpp"
#include "game_core/inc/bFile.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/inc/dbFiles.hpp"
#include "game_core/include/bJukebox.h"
#include "game_core/include/bSample.h"
#include "game_core/include/bVoice.h"
#include "game_core/indep/inc/AICarController.hpp"
#include "game_core/indep/inc/AIPlan.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/indep/inc/CameraCoordinator.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CarController.hpp"
#include "game_core/indep/inc/CarDamage.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/indep/inc/CarMover.hpp"
#include "game_core/indep/inc/CarSounds.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/ComponentPhysics.hpp"
#include "game_core/indep/inc/DeliveryMover.hpp"
#include "game_core/indep/inc/DemoDisc.hpp"
#include "game_core/indep/inc/DrivingChoice.hpp"
#include "game_core/indep/inc/EndGameScreens.hpp"
#include "game_core/indep/inc/FlailerDB.hpp"
#include "game_core/indep/inc/MellowMover.hpp"
#include "game_core/indep/inc/NIS.hpp"
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/indep/inc/PsychoDriver.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/inc/RaceCoordinator.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/ReplayScreen.hpp"
#include "game_core/indep/inc/Scenery.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include "game_core/indep/inc/Sound3d.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/inc/Texture.hpp"
#include "game_core/indep/inc/TrackPath.hpp"
#include "game_core/indep/inc/TrackRoute.hpp"
#include "game_core/indep/inc/TrackedController.hpp"
#include "game_core/indep/inc/TrafficCar.hpp"
#include "game_core/indep/inc/TrafficDriver.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/dbFilesBase.hpp"
#include "game_core/indep/inc/feAnim.hpp"
#include "game_core/indep/src/AverageInRealController.hpp"
#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/indep/src/PlayerCarSound.hpp"
#include "game_core/indep/src/QuickSplineInRealController.hpp"
#include "game_core/indep/src/Speech/actions.hpp"
#include "game_core/indep/src/VisibleDrivingLine.hpp"
#include "game_core/indep/src/ZoneFreezeCameraMover.hpp"
#include "game_core/indep/src/ZonePreviewCameraMover.hpp"
#include "game_core/indep/src/aerodynamics.hpp"
#include "game_core/indep/src/aicarcontroller.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/bodydamage.hpp"
#include "game_core/indep/src/brepeat.hpp"
#include "game_core/indep/src/cameracoordinator.hpp"
#include "game_core/indep/src/cameramover.hpp"
#include "game_core/indep/src/car.hpp"
#include "game_core/indep/src/carcollisiondetect.hpp"
#include "game_core/indep/src/carinfo.hpp"
#include "game_core/indep/src/carloader.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/src/collision.hpp"
#include "game_core/indep/src/collisionbody.hpp"
#include "game_core/indep/src/cop.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/src/demomodereplay.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/drivinglocation.hpp"
#include "game_core/indep/src/eanimationsolid.hpp"
#include "game_core/indep/src/engine.hpp"
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/indep/src/flailerdb.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/fred.hpp"
#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"
#include "game_core/indep/src/frontendscreens/feanim.hpp"
#include "game_core/indep/src/frontendscreens/joystickconfiguration.hpp"
#include "game_core/indep/src/frontendscreens/memcardscreens.hpp"
#include "game_core/indep/src/frontendscreens/nameentry.hpp"
#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/indep/src/frontendscreens/racesetup.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/indep/src/frontendscreens/selecttrack.hpp"
#include "game_core/indep/src/frontendscreens/startracescreens.hpp"
#include "game_core/indep/src/helisheet.hpp"
#include "game_core/indep/src/hudanim.hpp"
#include "game_core/indep/src/joystick.hpp"
#include "game_core/indep/src/language.hpp"
#include "game_core/indep/src/mellowmover.hpp"
#include "game_core/indep/src/movieplayer/cyclicbuffer.hpp"
#include "game_core/indep/src/movieplayer/dctblock.hpp"
#include "game_core/indep/src/movieplayer/dcttable.hpp"
#include "game_core/indep/src/movieplayer/huffman2.hpp"
#include "game_core/indep/src/movieplayer/mv3codec.hpp"
#include "game_core/indep/src/movieplayer/ringbuffer.hpp"
#include "game_core/indep/src/munitions.hpp"
#include "game_core/indep/src/point.hpp"
#include "game_core/indep/src/pursuit.hpp"
#include "game_core/indep/src/quickspline.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/indep/src/roadblock.hpp"
#include "game_core/indep/src/rumble.hpp"
#include "game_core/indep/src/scenery.hpp"
#include "game_core/indep/src/skids.hpp"
#include "game_core/indep/src/sound3dsample.hpp"
#include "game_core/indep/src/soundscape.hpp"
#include "game_core/indep/src/spacefiller.hpp"
#include "game_core/indep/src/speech/action.hpp"
#include "game_core/indep/src/speech/fsm.hpp"
#include "game_core/indep/src/speech/nfsfsm.hpp"
#include "game_core/indep/src/speech/sedwedge.hpp"
#include "game_core/indep/src/spikestrip.hpp"
#include "game_core/indep/src/spline.hpp"
#include "game_core/indep/src/suspension.hpp"
#include "game_core/indep/src/table.hpp"
#include "game_core/indep/src/texture.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/src/trackgraph.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/src/trackrouteeditor.hpp"
#include "game_core/indep/src/volumetree.hpp"
#include "game_core/indep/src/voronoiclip.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/kAITireSound.hpp"
#include "game_core/mExitScreenAnimation.hpp"
#include "game_core/m_ptr_SOUND_Memory_EE.hpp"
#include "game_core/pCarTexturePack.hpp"
#include "game_core/pDynamicTexturePack.hpp"
#include "game_core/pInitialSpotlightAlphas.hpp"
#include "game_core/pInitialSpotlightColours.hpp"
#include "game_core/pTrackSelectTexturePack.hpp"
#include "game_core/port/police/cop_control.hpp"
#include "game_core/src/acidfxe.hpp"
#include "game_core/src/bEffect.hpp"
#include "game_core/src/bFile.hpp"
#include "game_core/src/bList.hpp"
#include "game_core/src/bMemory.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/src/bSegment.hpp"
#include "game_core/src/bSound.hpp"
#include "game_core/src/dmadump.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/src/ecstasy/estripdatae.hpp"
#include "game_core/src/logitechwheel.hpp"
#include "game_core/src/memorycard.hpp"
#include "game_core/src/texturee.hpp"
#include "game_core/tCubic1D.hpp"
#include "game_core/tCustomizeHUDScreen.hpp"
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
    failures += !expect_gap("0x001C9018", "AIActionGetUnstuck::GetGoalTrackRoutePoint", [] { hp2::recovered::AIActionGetUnstuck instance; instance.GetGoalTrackRoutePoint(); });
    failures += !expect_gap("0x001C8660", "AIActionPullOver::GetDistanceToRightEdgeOfRoute", [] { hp2::recovered::AIActionPullOver instance; instance.GetDistanceToRightEdgeOfRoute(); });
    failures += !expect_gap("0x00191D10", "AICarController::ClearStickyDecisions", [] { hp2::recovered::AICarController instance; instance.ClearStickyDecisions(); });
    failures += !expect_gap("0x00191D90", "AICarController::GetStickyIndex", [] { hp2::recovered::AICarController instance; instance.GetStickyIndex(); });
    failures += !expect_gap("0x00193758", "AICarController::ForceBoost", [] { hp2::recovered::AICarController instance; instance.ForceBoost(); });
    failures += !expect_gap("0x00193780", "AICarController::ReleaseBoost", [] { hp2::recovered::AICarController instance; instance.ReleaseBoost(); });
    failures += !expect_gap("0x0018E6E8", "AICarSound::Sub_0018E6E8", [] { hp2::recovered::AICarSound instance; instance.Sub_0018E6E8(); });
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
    failures += !expect_gap("0x002117C8", "AIEngineSound::Sub_002117C8", [] { hp2::recovered::AIEngineSound instance; instance.Sub_002117C8(); });
    failures += !expect_gap("0x002118A0", "AIEngineSound::Update", [] { hp2::recovered::AIEngineSound instance; instance.Update(); });
    failures += !expect_gap("0x00211A80", "AIEngineSound::Mute", [] { hp2::recovered::AIEngineSound instance; instance.Mute(); });
    failures += !expect_gap("0x00211D58", "AIEngineSound::DeactivateSound", [] { hp2::recovered::AIEngineSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x00211D60", "AIEngineSound::ActivateSound", [] { hp2::recovered::AIEngineSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x00212418", "AIEngineSound::Sub_00212418", [] { hp2::recovered::AIEngineSound instance; instance.Sub_00212418(); });
    failures += !expect_gap("0x00127418", "AcidEffectSystem::Sub_00127418", [] { hp2::recovered::AcidEffectSystem instance; instance.Sub_00127418(); });
    failures += !expect_gap("0x00127688", "AcidEffectSystem::RemoveAutoEmitter", [] { hp2::recovered::AcidEffectSystem instance; instance.RemoveAutoEmitter(); });
    failures += !expect_gap("0x00248568", "AnimationGroup::Sub_00248568", [] { hp2::recovered::AnimationGroup instance; instance.Sub_00248568(); });
    failures += !expect_gap("0x002486E8", "AnimationGroup::SetupToAnimate", [] { hp2::recovered::AnimationGroup instance; instance.SetupToAnimate(); });
    failures += !expect_gap("0x00253A40", "AsyncChunkPri::Sub_00253A40", [] { hp2::recovered::AsyncChunkPri instance; instance.Sub_00253A40(); });
    failures += !expect_gap("0x002538B8", "AsyncEntry::Sub_002538B8", [] { hp2::recovered::AsyncEntry instance; instance.Sub_002538B8(); });
    failures += !expect_gap("0x001EF3C8", "AudioOptionsScreen::Sub_001EF3C8", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EF3C8(); });
    failures += !expect_gap("0x001EFA00", "AudioOptionsScreen::Sub_001EFA00", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFA00(); });
    failures += !expect_gap("0x001EFA68", "AudioOptionsScreen::FindSlider", [] { hp2::frontend::AudioOptionsScreen instance; instance.FindSlider(); });
    failures += !expect_gap("0x001EFB78", "AudioOptionsScreen::Sub_001EFB78", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFB78(); });
    failures += !expect_gap("0x001EFB80", "AudioOptionsScreen::Sub_001EFB80", [] { hp2::frontend::AudioOptionsScreen instance; instance.Sub_001EFB80(); });
    failures += !expect_gap("0x0018FCB8", "Average::Record", [] { hp2::recovered::Average instance; instance.Record(); });
    failures += !expect_gap("0x0018FD50", "Average::Reset", [] { hp2::recovered::Average instance; instance.Reset(); });
    failures += !expect_gap("0x0018FDA0", "Average::Flush", [] { hp2::recovered::Average instance; instance.Flush(); });
    failures += !expect_gap("0x0018E7E8", "BaseAICarSound::Sub_0018E7E8", [] { hp2::recovered::BaseAICarSound instance; instance.Sub_0018E7E8(); });
    failures += !expect_gap("0x0018E800", "BaseAICarSound::Update", [] { hp2::recovered::BaseAICarSound instance; instance.Update(); });
    failures += !expect_gap("0x0018D258", "BaseCarSound::Sub_0018D258", [] { hp2::recovered::BaseCarSound instance; instance.Sub_0018D258(); });
    failures += !expect_gap("0x0018D340", "BaseCarSound::Update", [] { hp2::recovered::BaseCarSound instance; instance.Update(); });
    failures += !expect_gap("0x0018D480", "BaseCarSound::GetHornSample", [] { hp2::recovered::BaseCarSound instance; instance.GetHornSample(); });
    failures += !expect_gap("0x0018D550", "BaseCarSound::Mute", [] { hp2::recovered::BaseCarSound instance; instance.Mute(); });
    failures += !expect_gap("0x0018D750", "BaseCarSound::InSoundSphere", [] { hp2::recovered::BaseCarSound instance; instance.InSoundSphere(); });
    failures += !expect_gap("0x0018D830", "BaseCarSound::DeactivateSound", [] { hp2::recovered::BaseCarSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x0018D8D8", "BaseCarSound::ActivateSound", [] { hp2::recovered::BaseCarSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x0018DB78", "BasePlayerCarSound::Sub_0018DB78", [] { hp2::recovered::BasePlayerCarSound instance; instance.Sub_0018DB78(); });
    failures += !expect_gap("0x0018DBF0", "BasePlayerCarSound::Update", [] { hp2::recovered::BasePlayerCarSound instance; instance.Update(); });
    failures += !expect_gap("0x0018DD10", "BasePlayerCarSound::Mute", [] { hp2::recovered::BasePlayerCarSound instance; instance.Mute(); });
    failures += !expect_gap("0x0018DDA0", "BasePlayerCarSound::GetHornSample", [] { hp2::recovered::BasePlayerCarSound instance; instance.GetHornSample(); });
    failures += !expect_gap("0x002125A0", "BaseTireSound::Mute", [] { hp2::recovered::BaseTireSound instance; instance.Mute(); });
    failures += !expect_gap("0x00214A68", "BaseTireSound::ScaleRoadNoise", [] { hp2::recovered::BaseTireSound instance; instance.ScaleRoadNoise(); });
    failures += !expect_gap("0x00214A70", "BaseTireSound::TriggerTireBlowOut", [] { hp2::recovered::BaseTireSound instance; instance.TriggerTireBlowOut(); });
    failures += !expect_gap("0x00214A78", "BaseTireSound::Sub_00214A78", [] { hp2::recovered::BaseTireSound instance; instance.Sub_00214A78(); });
    failures += !expect_gap("0x001E0898", "BeACopResults1P::Sub_001E0898", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E0898(); });
    failures += !expect_gap("0x001E2358", "BeACopResults1P::Sub_001E2358", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2358(); });
    failures += !expect_gap("0x001E2588", "BeACopResults1P::Sub_001E2588", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BeACopResults1P::Sub_001E2590", [] { hp2::recovered::BeACopResults1P instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001E0A10", "BeACopResults2P::Sub_001E0A10", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E0A10(); });
    failures += !expect_gap("0x001E0BC8", "BeACopResults2P::Sub_001E0BC8", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E0BC8(); });
    failures += !expect_gap("0x001E2340", "BeACopResults2P::Sub_001E2340", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2340(); });
    failures += !expect_gap("0x001E2588", "BeACopResults2P::Sub_001E2588", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BeACopResults2P::Sub_001E2590", [] { hp2::recovered::BeACopResults2P instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x00204098", "BeltMover::Sub_00204098", [] { hp2::recovered::BeltMover instance; instance.Sub_00204098(); });
    failures += !expect_gap("0x002043A0", "BeltMover::Sub_002043A0", [] { hp2::recovered::BeltMover instance; instance.Sub_002043A0(); });
    failures += !expect_gap("0x00204468", "BeltMover::DoSnapshot", [] { hp2::recovered::BeltMover instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x00204D78", "BeltMover::Move", [] { hp2::recovered::BeltMover instance; instance.Move(); });
    failures += !expect_gap("0x001DDC10", "BestLapTimes::Sub_001DDC10", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001DDC10(); });
    failures += !expect_gap("0x001E2388", "BestLapTimes::Sub_001E2388", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2388(); });
    failures += !expect_gap("0x001E2588", "BestLapTimes::Sub_001E2588", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BestLapTimes::Sub_001E2590", [] { hp2::recovered::BestLapTimes instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001DDFD0", "BestTotalTimes::Sub_001DDFD0", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001DDFD0(); });
    failures += !expect_gap("0x001E2370", "BestTotalTimes::Sub_001E2370", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2370(); });
    failures += !expect_gap("0x001E2588", "BestTotalTimes::Sub_001E2588", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "BestTotalTimes::Sub_001E2590", [] { hp2::recovered::BestTotalTimes instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001B4D48", "BodyDamage::SetDamage", [] { hp2::recovered::BodyDamage instance; instance.SetDamage(); });
    failures += !expect_gap("0x001B4E10", "BodyDamage::GetSectorNumber", [] { hp2::recovered::BodyDamage instance; instance.GetSectorNumber(); });
    failures += !expect_gap("0x001B4F10", "BodyDamage::GetDamageAmount", [] { hp2::recovered::BodyDamage instance; instance.GetDamageAmount(); });
    failures += !expect_gap("0x001B50C8", "BodyDamage::GetVisibleTotalDamage", [] { hp2::recovered::BodyDamage instance; instance.GetVisibleTotalDamage(); });
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
    failures += !expect_gap("0x0019B6B0", "CameraDataItems::Sub_0019B6B0", [] { hp2::recovered::CameraDataItems instance; instance.Sub_0019B6B0(); });
    failures += !expect_gap("0x0019BAB8", "CameraDataItems::Sub_0019BAB8", [] { hp2::recovered::CameraDataItems instance; instance.Sub_0019BAB8(); });
    failures += !expect_gap("0x001D7FA0", "CarCollisionBody::Sub_001D7FA0", [] { hp2::recovered::CarCollisionBody instance; instance.Sub_001D7FA0(); });
    failures += !expect_gap("0x001D8058", "CarCollisionBody::DoSnapshot", [] { hp2::recovered::CarCollisionBody instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001D8138", "CarCollisionBody::SetupPhysicsPointers", [] { hp2::recovered::CarCollisionBody instance; instance.SetupPhysicsPointers(); });
    failures += !expect_gap("0x001D8198", "CarCollisionBody::SetToPhysicsMode", [] { hp2::recovered::CarCollisionBody instance; instance.SetToPhysicsMode(); });
    failures += !expect_gap("0x001D81F0", "CarCollisionBody::Move", [] { hp2::recovered::CarCollisionBody instance; instance.Move(); });
    failures += !expect_gap("0x001D8270", "CarCollisionBody::AreCollisionsEnabled", [] { hp2::recovered::CarCollisionBody instance; instance.AreCollisionsEnabled(); });
    failures += !expect_gap("0x001D8298", "CarCollisionBody::GetMassInverse", [] { hp2::recovered::CarCollisionBody instance; instance.GetMassInverse(); });
    failures += !expect_gap("0x001D82D0", "CarCollisionBody::GetInertiaTensorInverse", [] { hp2::recovered::CarCollisionBody instance; instance.GetInertiaTensorInverse(); });
    failures += !expect_gap("0x001D8318", "CarCollisionBody::ApplyImpulse", [] { hp2::recovered::CarCollisionBody instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x001D8E58", "CarCollisionBody::GetFriction", [] { hp2::recovered::CarCollisionBody instance; instance.GetFriction(); });
    failures += !expect_gap("0x001D8E68", "CarCollisionBody::GetBounciness", [] { hp2::recovered::CarCollisionBody instance; instance.GetBounciness(); });
    failures += !expect_gap("0x001D8E78", "CarCollisionBody::GetTerrainType", [] { hp2::recovered::CarCollisionBody instance; instance.GetTerrainType(); });
    failures += !expect_gap("0x001D8E80", "CarCollisionBody::IsBlinking", [] { hp2::recovered::CarCollisionBody instance; instance.IsBlinking(); });
    failures += !expect_gap("0x001D8ED8", "CarCollisionBody::IsMoveable", [] { hp2::recovered::CarCollisionBody instance; instance.IsMoveable(); });
    failures += !expect_gap("0x001D8EE0", "CarCollisionBody::ConsiderImpulse", [] { hp2::recovered::CarCollisionBody instance; instance.ConsiderImpulse(); });
    failures += !expect_gap("0x001D8EF0", "CarCollisionBody::GetRotation", [] { hp2::recovered::CarCollisionBody instance; instance.GetRotation(); });
    failures += !expect_gap("0x001D8F10", "CarCollisionBody::GetVelocity", [] { hp2::recovered::CarCollisionBody instance; instance.GetVelocity(); });
    failures += !expect_gap("0x001D8F30", "CarCollisionBody::GetPosition", [] { hp2::recovered::CarCollisionBody instance; instance.GetPosition(); });
    failures += !expect_gap("0x001E6010", "CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe", [] { hp2::recovered::CarCollisionPredict instance; instance.IsThisCarStoppedDirectlyInFrontOfMe(); });
    failures += !expect_gap("0x001C00A8", "CarController::Sub_001C00A8", [] { hp2::recovered::CarController instance; instance.Sub_001C00A8(); });
    failures += !expect_gap("0x001C0100", "CarController::SetToPhysicsMode", [] { hp2::recovered::CarController instance; instance.SetToPhysicsMode(); });
    failures += !expect_gap("0x001C0140", "CarController::ForceBoost", [] { hp2::recovered::CarController instance; instance.ForceBoost(); });
    failures += !expect_gap("0x001C0160", "CarController::ReleaseBoost", [] { hp2::recovered::CarController instance; instance.ReleaseBoost(); });
    failures += !expect_gap("0x001C0198", "CarController::DebugDisplay", [] { hp2::recovered::CarController instance; instance.DebugDisplay(); });
    failures += !expect_gap("0x001B0518", "CarLoader::FindLoadedSkin", [] { hp2::recovered::CarLoader instance; instance.FindLoadedSkin(); });
    failures += !expect_gap("0x001309F0", "CarMover::DoSnapshot", [] { hp2::recovered::CarMover instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x00136B78", "CarMover::Sub_00136B78", [] { hp2::recovered::CarMover instance; instance.Sub_00136B78(); });
    failures += !expect_gap("0x0011E860", "CarRenderInfo::Sub_0011E860", [] { hp2::recovered::CarRenderInfo instance; instance.Sub_0011E860(); });
    failures += !expect_gap("0x00120EF8", "CarRenderInfo::Sub_00120EF8", [] { hp2::recovered::CarRenderInfo instance; instance.Sub_00120EF8(); });
    failures += !expect_gap("0x00195C40", "CarSelector::SetupCarOverrideModel", [] { hp2::recovered::CarSelector instance; instance.SetupCarOverrideModel(); });
    failures += !expect_gap("0x001884B8", "CarTypeInfo::GetNumAvailableSkins", [] { hp2::recovered::CarTypeInfo instance; instance.GetNumAvailableSkins(); });
    failures += !expect_gap("0x001E8560", "Chopper::GetMaxDeceleration", [] { hp2::recovered::Chopper instance; instance.GetMaxDeceleration(); });
    failures += !expect_gap("0x001E9AE8", "Chopper::SetRotateTarget", [] { hp2::recovered::Chopper instance; instance.SetRotateTarget(); });
    failures += !expect_gap("0x001E9E40", "Chopper::CalcChopperRotation", [] { hp2::recovered::Chopper instance; instance.CalcChopperRotation(); });
    failures += !expect_gap("0x001E9E80", "Chopper::SubtractFromYaw", [] { hp2::recovered::Chopper instance; instance.SubtractFromYaw(); });
    failures += !expect_gap("0x001EA578", "Chopper::RenderChopper", [] { hp2::recovered::Chopper instance; instance.RenderChopper(); });
    failures += !expect_gap("0x00214D68", "ChopperSound::Sub_00214D68", [] { hp2::recovered::ChopperSound instance; instance.Sub_00214D68(); });
    failures += !expect_gap("0x00214DF0", "ChopperSound::Sub_00214DF0", [] { hp2::recovered::ChopperSound instance; instance.Sub_00214DF0(); });
    failures += !expect_gap("0x00214F30", "ChopperSound::Update", [] { hp2::recovered::ChopperSound instance; instance.Update(); });
    failures += !expect_gap("0x00215000", "ChopperSound::Mute", [] { hp2::recovered::ChopperSound instance; instance.Mute(); });
    failures += !expect_gap("0x00215028", "ChopperSound::DeactivateSound", [] { hp2::recovered::ChopperSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x00215050", "ChopperSound::ActivateSound", [] { hp2::recovered::ChopperSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x0021EB20", "Collision::IsTwoCarCollision", [] { hp2::recovered::Collision instance; instance.IsTwoCarCollision(); });
    failures += !expect_gap("0x0021EB48", "Collision::GetCollidingCars", [] { hp2::recovered::Collision instance; instance.GetCollidingCars(); });
    failures += !expect_gap("0x0021EBD8", "Collision::IsSmokeableInvolved", [] { hp2::recovered::Collision instance; instance.IsSmokeableInvolved(); });
    failures += !expect_gap("0x001D69B0", "CollisionBody::Sub_001D69B0", [] { hp2::recovered::CollisionBody instance; instance.Sub_001D69B0(); });
    failures += !expect_gap("0x001D6A98", "CollisionBody::DoSnapshot", [] { hp2::recovered::CollisionBody instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001D91D0", "CollisionBody::IsBlinking", [] { hp2::recovered::CollisionBody instance; instance.IsBlinking(); });
    failures += !expect_gap("0x00130958", "CopDriver::Sub_00130958", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00130958(); });
    failures += !expect_gap("0x00136BB8", "CopDriver::Sub_00136BB8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BB8(); });
    failures += !expect_gap("0x00136BC0", "CopDriver::Sub_00136BC0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BC0(); });
    failures += !expect_gap("0x00136BD0", "CopDriver::Sub_00136BD0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BD0(); });
    failures += !expect_gap("0x00136BD8", "CopDriver::Sub_00136BD8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BD8(); });
    failures += !expect_gap("0x00136BE0", "CopDriver::Sub_00136BE0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BE0(); });
    failures += !expect_gap("0x00136BE8", "CopDriver::Sub_00136BE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136BE8(); });
    failures += !expect_gap("0x00136C58", "CopDriver::Sub_00136C58", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C58(); });
    failures += !expect_gap("0x00136C60", "CopDriver::Sub_00136C60", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136C60(); });
    failures += !expect_gap("0x00136CE8", "CopDriver::Sub_00136CE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136CE8(); });
    failures += !expect_gap("0x00136CF0", "CopDriver::Sub_00136CF0", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_00136CF0(); });
    failures += !expect_gap("0x001A1CE8", "CopDriver::Sub_001A1CE8", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A1CE8(); });
    failures += !expect_gap("0x001A1E30", "CopDriver::Sub_001A1E30", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_001A1E30(); });
    failures += !expect_gap("0x001A2110", "CopDriver::Think", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Think(); });
    failures += !expect_gap("0x001A2B58", "CopDriver::DoSnapshot", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.DoSnapshot(); });
    failures += !expect_gap("0x002A0550", "CopDriver::Sub_002A0550", [] { hp2::recovered::CopDriverStorage storage{}; hp2::recovered::CopDriver instance(storage); instance.Sub_002A0550(); });
    failures += !expect_gap("0x00231C08", "CopFillerSpeech::GetWeight", [] { hp2::recovered::CopFillerSpeech instance; instance.GetWeight(); });
    failures += !expect_gap("0x00231B68", "CopLocationFillerClass::Speak", [] { hp2::recovered::CopLocationFillerClass instance; instance.Speak(); });
    failures += !expect_gap("0x00231BA0", "CopLocationFillerClass::IsAppropriate", [] { hp2::recovered::CopLocationFillerClass instance; instance.IsAppropriate(); });
    failures += !expect_gap("0x00231C00", "CopLocationFillerClass::GetBank", [] { hp2::recovered::CopLocationFillerClass instance; instance.GetBank(); });
    failures += !expect_gap("0x001A0DD8", "CopMover::Sub_001A0DD8", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0DD8(); });
    failures += !expect_gap("0x001A0E88", "CopMover::Sub_001A0E88", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0E88(); });
    failures += !expect_gap("0x001A0F20", "CopMover::Sub_001A0F20", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A0F20(); });
    failures += !expect_gap("0x001A1030", "CopMover::Sub_001A1030", [] { hp2::recovered::CopMoverStorage storage{}; hp2::recovered::CopMover instance(storage); instance.Sub_001A1030(); });
    failures += !expect_gap("0x002319D8", "CopSpeedFillerClass::Speak", [] { hp2::recovered::CopSpeedFillerClass instance; instance.Speak(); });
    failures += !expect_gap("0x00231A20", "CopSpeedFillerClass::IsAppropriate", [] { hp2::recovered::CopSpeedFillerClass instance; instance.IsAppropriate(); });
    failures += !expect_gap("0x00231AA0", "CopSpeedFillerClass::GetBank", [] { hp2::recovered::CopSpeedFillerClass instance; instance.GetBank(); });
    failures += !expect_gap("0x00231AA8", "CopTacticMiscFillerClass::Speak", [] { hp2::recovered::CopTacticMiscFillerClass instance; instance.Speak(); });
    failures += !expect_gap("0x00231B08", "CopTacticMiscFillerClass::IsAppropriate", [] { hp2::recovered::CopTacticMiscFillerClass instance; instance.IsAppropriate(); });
    failures += !expect_gap("0x00231B60", "CopTacticMiscFillerClass::GetBank", [] { hp2::recovered::CopTacticMiscFillerClass instance; instance.GetBank(); });
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
    failures += !expect_gap("0x00232268", "CyclicBuffer::DataAvail", [] { hp2::recovered::CyclicBuffer instance; instance.DataAvail(); });
    failures += !expect_gap("0x00232498", "DCTImage::IDCTRowReference", [] { hp2::recovered::DCTImage instance; instance.IDCTRowReference(); });
    failures += !expect_gap("0x00234A68", "DCTImage::IDCTZero", [] { hp2::recovered::DCTImage instance; instance.IDCTZero(); });
    failures += !expect_gap("0x00236D90", "DCTTable::ScaleTable", [] { hp2::recovered::DCTTable instance; instance.ScaleTable(); });
    failures += !expect_gap("0x00236DF8", "DCTTable::ConvertQualityToScale", [] { hp2::recovered::DCTTable instance; instance.ConvertQualityToScale(); });
    failures += !expect_gap("0x00101948", "DMAC::DumpGifTag", [] { hp2::recovered::DMAC instance; instance.DumpGifTag(); });
    failures += !expect_gap("0x00101BB8", "DMAC::dmaTransferAddress", [] { hp2::recovered::DMAC instance; instance.dmaTransferAddress(); });
    failures += !expect_gap("0x00102028", "DMAC::FastSync", [] { hp2::recovered::DMAC instance; instance.FastSync(); });
    failures += !expect_gap("0x001B93E0", "DataBeTheCopSetup::InitWithDefaults", [] { hp2::recovered::DataBeTheCopSetup instance; instance.InitWithDefaults(); });
    failures += !expect_gap("0x001F5D00", "DataEventSelectScreen::GetSelectedEvent", [] { hp2::recovered::DataEventSelectScreen instance; instance.GetSelectedEvent(); });
    failures += !expect_gap("0x001F5D28", "DataEventSelectScreen::SetSelectedEvent", [] { hp2::recovered::DataEventSelectScreen instance; instance.SetSelectedEvent(); });
    failures += !expect_gap("0x001B8F38", "DataLapKnockoutSetup::InitWithDefaults", [] { hp2::recovered::DataLapKnockoutSetup instance; instance.InitWithDefaults(); });
    failures += !expect_gap("0x001B9218", "DataRaceKnockoutSetup::InitWithDefaults", [] { hp2::recovered::DataRaceKnockoutSetup instance; instance.InitWithDefaults(); });
    failures += !expect_gap("0x001B8AF0", "DataSingleRaceSetup::InitWithDefaults", [] { hp2::recovered::DataSingleRaceSetup instance; instance.InitWithDefaults(); });
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
    failures += !expect_gap("0x0013EE08", "DemoModeReplay::Sub_0013EE08", [] { hp2::recovered::DemoModeReplay instance; instance.Sub_0013EE08(); });
    failures += !expect_gap("0x0013EE58", "DemoModeReplay::Loader", [] { hp2::recovered::DemoModeReplay instance; instance.Loader(); });
    failures += !expect_gap("0x001577D0", "DriveCameraMover::Sub_001577D0", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "DriveCameraMover::Sub_001577D8", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "DriveCameraMover::Sub_00157850", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x0015C138", "DriveCameraMover::Sub_0015C138", [] { hp2::recovered::DriveCameraMover instance; instance.Sub_0015C138(); });
    failures += !expect_gap("0x0015C1D0", "DriveCameraMover::GetCorrectionPitch", [] { hp2::recovered::DriveCameraMover instance; instance.GetCorrectionPitch(); });
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
    failures += !expect_gap("0x0021BDC8", "DriveTarget::GetLastIndexToCheckForDrivability", [] { hp2::recovered::DriveTarget instance; instance.GetLastIndexToCheckForDrivability(); });
    failures += !expect_gap("0x0021DAC8", "DriveTarget::RecalculateLength", [] { hp2::recovered::DriveTarget instance; instance.RecalculateLength(); });
    failures += !expect_gap("0x00241CF8", "DrivingChoice::SetChoice", [] { hp2::recovered::DrivingChoice instance; instance.SetChoice(); });
    failures += !expect_gap("0x00243988", "DrivingChoice::ReduceCurrentL", [] { hp2::recovered::DrivingChoice instance; instance.ReduceCurrentL(); });
    failures += !expect_gap("0x00193E68", "DrivingInformation::Clear", [] { hp2::recovered::DrivingInformation instance; instance.Clear(); });
    failures += !expect_gap("0x00239C90", "DrivingLocation::DoesThisRoutePointMatchThisRoute", [] { hp2::recovered::DrivingLocation instance; instance.DoesThisRoutePointMatchThisRoute(); });
    failures += !expect_gap("0x001BEF20", "DroppedSpikes::Sub_001BEF20", [] { hp2::recovered::DroppedSpikes instance; instance.Sub_001BEF20(); });
    failures += !expect_gap("0x001BEF90", "DroppedSpikes::DoSnapshot", [] { hp2::recovered::DroppedSpikes instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001BF078", "DroppedSpikes::MaybeExplodeWarhead", [] { hp2::recovered::DroppedSpikes instance; instance.MaybeExplodeWarhead(); });
    failures += !expect_gap("0x001BF0D8", "DroppedSpikes::Move", [] { hp2::recovered::DroppedSpikes instance; instance.Move(); });
    failures += !expect_gap("0x001BF160", "DroppedSpikes::ConsiderImpulse", [] { hp2::recovered::DroppedSpikes instance; instance.ConsiderImpulse(); });
    failures += !expect_gap("0x001BF178", "DroppedSpikes::ApplyImpulse", [] { hp2::recovered::DroppedSpikes instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x001BFD90", "DroppedSpikes::GetSmokeType", [] { hp2::recovered::DroppedSpikes instance; instance.GetSmokeType(); });
    failures += !expect_gap("0x001D7028", "DynamicCollisionBody::Sub_001D7028", [] { hp2::recovered::DynamicCollisionBody instance; instance.Sub_001D7028(); });
    failures += !expect_gap("0x001D7050", "DynamicCollisionBody::DoSnapshot", [] { hp2::recovered::DynamicCollisionBody instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001D7408", "DynamicCollisionBody::Interpolate", [] { hp2::recovered::DynamicCollisionBody instance; instance.Interpolate(); });
    failures += !expect_gap("0x001D7660", "DynamicCollisionBody::Backup", [] { hp2::recovered::DynamicCollisionBody instance; instance.Backup(); });
    failures += !expect_gap("0x001D7710", "DynamicCollisionBody::RecomputeAuxilliaries", [] { hp2::recovered::DynamicCollisionBody instance; instance.RecomputeAuxilliaries(); });
    failures += !expect_gap("0x001D7740", "DynamicCollisionBody::GetCurrentVelocities", [] { hp2::recovered::DynamicCollisionBody instance; instance.GetCurrentVelocities(); });
    failures += !expect_gap("0x001D7760", "DynamicCollisionBody::GetTestImpulseVelocities", [] { hp2::recovered::DynamicCollisionBody instance; instance.GetTestImpulseVelocities(); });
    failures += !expect_gap("0x001D7E08", "DynamicCollisionBody::ChangeInEnergy", [] { hp2::recovered::DynamicCollisionBody instance; instance.ChangeInEnergy(); });
    failures += !expect_gap("0x001D8F98", "DynamicCollisionBody::ApplyImpulse", [] { hp2::recovered::DynamicCollisionBody instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x001D8FA0", "DynamicCollisionBody::GetInertiaTensorInverse", [] { hp2::recovered::DynamicCollisionBody instance; instance.GetInertiaTensorInverse(); });
    failures += !expect_gap("0x001D8FB0", "DynamicCollisionBody::GetMassInverse", [] { hp2::recovered::DynamicCollisionBody instance; instance.GetMassInverse(); });
    failures += !expect_gap("0x001D8FC0", "DynamicCollisionBody::SetToPhysicsMode", [] { hp2::recovered::DynamicCollisionBody instance; instance.SetToPhysicsMode(); });
    failures += !expect_gap("0x001D8FC8", "DynamicCollisionBody::GetMaxBackupTime", [] { hp2::recovered::DynamicCollisionBody instance; instance.GetMaxBackupTime(); });
    failures += !expect_gap("0x001D8FD0", "DynamicCollisionBody::SetPosition", [] { hp2::recovered::DynamicCollisionBody instance; instance.SetPosition(); });
    failures += !expect_gap("0x001E12B8", "EndGameMenu::Sub_001E12B8", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E12B8(); });
    failures += !expect_gap("0x001E1918", "EndGameMenu::FindEndGameMenuItem", [] { hp2::recovered::EndGameMenu instance; instance.FindEndGameMenuItem(); });
    failures += !expect_gap("0x001E19E0", "EndGameMenu::Sub_001E19E0", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E19E0(); });
    failures += !expect_gap("0x001E2270", "EndGameMenu::Sub_001E2270", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2270(); });
    failures += !expect_gap("0x001E2588", "EndGameMenu::Sub_001E2588", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "EndGameMenu::Sub_001E2590", [] { hp2::recovered::EndGameMenu instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001DD0F0", "EndGameMenusBase::HandleEvent", [] { hp2::recovered::EndGameMenusBase instance; instance.HandleEvent(); });
    failures += !expect_gap("0x001E2580", "EndGameMenusBase::Render", [] { hp2::recovered::EndGameMenusBase instance; instance.Render(); });
    failures += !expect_gap("0x001DCE18", "EndGameScreens::GetNextScreen", [] { hp2::recovered::EndGameScreens instance; instance.GetNextScreen(); });
    failures += !expect_gap("0x001DCF00", "EndGameScreens::MoveBackScreen", [] { hp2::recovered::EndGameScreens instance; instance.MoveBackScreen(); });
    failures += !expect_gap("0x00200878", "EventInfo::GetCar", [] { hp2::recovered::EventInfo instance; instance.GetCar(); });
    failures += !expect_gap("0x002008B8", "EventInfo::GetGoal", [] { hp2::recovered::EventInfo instance; instance.GetGoal(); });
    failures += !expect_gap("0x002008F8", "EventInfo::GetTrack", [] { hp2::recovered::EventInfo instance; instance.GetTrack(); });
    failures += !expect_gap("0x00200938", "EventInfo::GetReward", [] { hp2::recovered::EventInfo instance; instance.GetReward(); });
    failures += !expect_gap("0x00200978", "EventInfo::GetOpponent", [] { hp2::recovered::EventInfo instance; instance.GetOpponent(); });
    failures += !expect_gap("0x00200D48", "EventInfo::GetRankState", [] { hp2::recovered::EventInfo instance; instance.GetRankState(); });
    failures += !expect_gap("0x00200D70", "EventInfo::GetTournamentState", [] { hp2::recovered::EventInfo instance; instance.GetTournamentState(); });
    failures += !expect_gap("0x00200D90", "EventInfo::GetKoState", [] { hp2::recovered::EventInfo instance; instance.GetKoState(); });
    failures += !expect_gap("0x00200DB0", "EventInfo::GetTimeState", [] { hp2::recovered::EventInfo instance; instance.GetTimeState(); });
    failures += !expect_gap("0x00201350", "EventInfo::GetRaceCount", [] { hp2::recovered::EventInfo instance; instance.GetRaceCount(); });
    failures += !expect_gap("0x002001A8", "EventKoState::GetRaceIndex", [] { hp2::recovered::EventKoState instance; instance.GetRaceIndex(); });
    failures += !expect_gap("0x001FC580", "EventLeaf::SetScrollNotch", [] { hp2::recovered::EventLeaf instance; instance.SetScrollNotch(); });
    failures += !expect_gap("0x001FCC88", "EventLeaf::FillVerts", [] { hp2::recovered::EventLeaf instance; instance.FillVerts(); });
    failures += !expect_gap("0x001FD050", "EventLeaf::GetNeighbour", [] { hp2::recovered::EventLeaf instance; instance.GetNeighbour(); });
    failures += !expect_gap("0x001FD0B0", "EventLeaf::GetReward", [] { hp2::recovered::EventLeaf instance; instance.GetReward(); });
    failures += !expect_gap("0x00200018", "EventRankState::GetStatus", [] { hp2::recovered::EventRankState instance; instance.GetStatus(); });
    failures += !expect_gap("0x001DEF38", "EventRewardScreen::Sub_001DEF38", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001DEF38(); });
    failures += !expect_gap("0x001E24F8", "EventRewardScreen::Sub_001E24F8", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E24F8(); });
    failures += !expect_gap("0x001E2588", "EventRewardScreen::Sub_001E2588", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "EventRewardScreen::Sub_001E2590", [] { hp2::recovered::EventRewardScreen instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x00200068", "EventTimeState::GetStatus", [] { hp2::recovered::EventTimeState instance; instance.GetStatus(); });
    failures += !expect_gap("0x00200638", "EventTournamentState::Reset", [] { hp2::recovered::EventTournamentState instance; instance.Reset(); });
    failures += !expect_gap("0x00200710", "EventTournamentState::RacerRank", [] { hp2::recovered::EventTournamentState instance; instance.RacerRank(); });
    failures += !expect_gap("0x001FDFE8", "EventTree::GetEventNumber", [] { hp2::recovered::EventTree instance; instance.GetEventNumber(); });
    failures += !expect_gap("0x001FFF60", "EventTree::SetEvents", [] { hp2::recovered::EventTree instance; instance.SetEvents(); });
    failures += !expect_gap("0x001C3718", "FancyJoyEventHandler::IsButtonPressedRepeat", [] { hp2::recovered::FancyJoyEventHandler instance; instance.IsButtonPressedRepeat(); });
    failures += !expect_gap("0x001411D8", "FlailerBitmapChunk::SetAlpha", [] { hp2::recovered::FlailerBitmapChunk instance; instance.SetAlpha(); });
    failures += !expect_gap("0x00141258", "FlailerBitmapChunk::SetRotation", [] { hp2::recovered::FlailerBitmapChunk instance; instance.SetRotation(); });
    failures += !expect_gap("0x00141288", "FlailerBitmapChunk::SetColor", [] { hp2::recovered::FlailerBitmapChunk instance; instance.SetColor(); });
    failures += !expect_gap("0x0016A130", "FlailerMenu::GetCurrentItemNumber", [] { hp2::recovered::FlailerMenu instance; instance.GetCurrentItemNumber(); });
    failures += !expect_gap("0x0016A5B0", "FlailerMenu::FindItem", [] { hp2::recovered::FlailerMenu instance; instance.FindItem(); });
    failures += !expect_gap("0x001407D0", "FlailerObjectChunk::SetScaleX", [] { hp2::recovered::FlailerObjectChunk instance; instance.SetScaleX(); });
    failures += !expect_gap("0x001407F8", "FlailerObjectChunk::SetScaleY", [] { hp2::recovered::FlailerObjectChunk instance; instance.SetScaleY(); });
    failures += !expect_gap("0x00140A20", "FlailerObjectChunk::SetTopLeftX", [] { hp2::recovered::FlailerObjectChunk instance; instance.SetTopLeftX(); });
    failures += !expect_gap("0x00140A48", "FlailerObjectChunk::SetTopLeftY", [] { hp2::recovered::FlailerObjectChunk instance; instance.SetTopLeftY(); });
    failures += !expect_gap("0x00145AF0", "FlailerRenderManager::Sub_00145AF0", [] { hp2::recovered::FlailerRenderManager instance; instance.Sub_00145AF0(); });
    failures += !expect_gap("0x00145CF8", "FlailerRenderManager::IsScreenVisible", [] { hp2::recovered::FlailerRenderManager instance; instance.IsScreenVisible(); });
    failures += !expect_gap("0x0016C888", "FlailerScreenList::FindScreen", [] { hp2::recovered::FlailerScreenList instance; instance.FindScreen(); });
    failures += !expect_gap("0x0016CA08", "FlailerScreenList::IsAnyPopupActive", [] { hp2::recovered::FlailerScreenList instance; instance.IsAnyPopupActive(); });
    failures += !expect_gap("0x0016B340", "FlailerScreenNode::FindFlailerScreen", [] { hp2::recovered::FlailerScreenNode instance; instance.FindFlailerScreen(); });
    failures += !expect_gap("0x0016B378", "FlailerScreenNode::SetFilterPort", [] { hp2::recovered::FlailerScreenNode instance; instance.SetFilterPort(); });
    failures += !expect_gap("0x0016B3B0", "FlailerScreenNode::IsPortFiltered", [] { hp2::recovered::FlailerScreenNode instance; instance.IsPortFiltered(); });
    failures += !expect_gap("0x00140CA0", "FlailerTextChunk::SetColor", [] { hp2::recovered::FlailerTextChunk instance; instance.SetColor(); });
    failures += !expect_gap("0x00140ED0", "FlailerTextChunk::SetAlpha", [] { hp2::recovered::FlailerTextChunk instance; instance.SetAlpha(); });
    failures += !expect_gap("0x00140098", "Font::GetKerning", [] { hp2::recovered::Font instance; instance.GetKerning(); });
    failures += !expect_gap("0x001401D8", "Font::GetWidestCharacter", [] { hp2::recovered::Font instance; instance.GetWidestCharacter(); });
    failures += !expect_gap("0x001841A8", "FrontendDatabase::UseTheRaceEvent", [] { hp2::recovered::FrontendDatabase instance; instance.UseTheRaceEvent(); });
    failures += !expect_gap("0x001855F0", "FrontendDatabase::IsChallengeGame", [] { hp2::recovered::FrontendDatabase instance; instance.IsChallengeGame(); });
    failures += !expect_gap("0x00187928", "FrontendPlayerInfo::FlushAvailableCars", [] { hp2::recovered::FrontendPlayerInfo instance; instance.FlushAvailableCars(); });
    failures += !expect_gap("0x00244768", "FuturePoints::AddPoint", [] { hp2::recovered::FuturePoints instance; instance.AddPoint(); });
    failures += !expect_gap("0x002452B0", "FuturePoints::Dump", [] { hp2::recovered::FuturePoints instance; instance.Dump(); });
    failures += !expect_gap("0x0015E5E8", "GarageCameraMover::PrevTrack", [] { hp2::recovered::GarageCameraMover instance; instance.PrevTrack(); });
    failures += !expect_gap("0x0015E610", "GarageCameraMover::NextTrack", [] { hp2::recovered::GarageCameraMover instance; instance.NextTrack(); });
    failures += !expect_gap("0x001EB6A8", "HeliPoly::IsPointInPoly", [] { hp2::recovered::HeliPoly instance; instance.IsPointInPoly(); });
    failures += !expect_gap("0x001EB840", "HeliPoly::GetZValue", [] { hp2::recovered::HeliPoly instance; instance.GetZValue(); });
    failures += !expect_gap("0x0018F160", "HornSound3d::ActivateSound", [] { hp2::recovered::HornSound3d instance; instance.ActivateSound(); });
    failures += !expect_gap("0x0018F188", "HornSound3d::DeactivateSound", [] { hp2::recovered::HornSound3d instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x0018F208", "HornSound3d::Mute", [] { hp2::recovered::HornSound3d instance; instance.Mute(); });
    failures += !expect_gap("0x0018F218", "HornSound3d::Update", [] { hp2::recovered::HornSound3d instance; instance.Update(); });
    failures += !expect_gap("0x0018F2B0", "HornSound3d::Sub_0018F2B0", [] { hp2::recovered::HornSound3d instance; instance.Sub_0018F2B0(); });
    failures += !expect_gap("0x0023B0F8", "HudAnimator::Sub_0023B0F8", [] { hp2::recovered::HudAnimator instance; instance.Sub_0023B0F8(); });
    failures += !expect_gap("0x00236EE8", "Huffman::NumBits", [] { hp2::recovered::Huffman instance; instance.NumBits(); });
    failures += !expect_gap("0x00236F18", "Huffman::STORE", [] { hp2::recovered::Huffman instance; instance.STORE(); });
    failures += !expect_gap("0x00237228", "Huffman::GetBits", [] { hp2::recovered::Huffman instance; instance.GetBits(); });
    failures += !expect_gap("0x00237660", "Huffman::GETBITS", [] { hp2::recovered::Huffman instance; instance.GETBITS(); });
    failures += !expect_gap("0x00153C20", "Joystick::FindEventNode", [] { hp2::recovered::Joystick instance; instance.FindEventNode(); });
    failures += !expect_gap("0x0023F300", "JoystickCalibrationScreen::ResetJoystickCalibration", [] { hp2::recovered::JoystickCalibrationScreen instance; instance.ResetJoystickCalibration(); });
    failures += !expect_gap("0x002400C0", "JoystickPreconfig::Sub_002400C0", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_002400C0(); });
    failures += !expect_gap("0x00240230", "JoystickPreconfig::Sub_00240230", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_00240230(); });
    failures += !expect_gap("0x002402C0", "JoystickPreconfig::Sub_002402C0", [] { hp2::frontend::JoystickPreconfig instance; instance.Sub_002402C0(); });
    failures += !expect_gap("0x001F02D8", "JukeBoxMusicScreen::Sub_001F02D8", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F02D8(); });
    failures += !expect_gap("0x001F0460", "JukeBoxMusicScreen::Sub_001F0460", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F0460(); });
    failures += !expect_gap("0x001F0D10", "JukeBoxMusicScreen::Sub_001F0D10", [] { hp2::frontend::JukeBoxMusicScreen instance; instance.Sub_001F0D10(); });
    failures += !expect_gap("0x0021E2E8", "LanguageManager::GetString", [] { hp2::recovered::LanguageManager instance; instance.GetString(); });
    failures += !expect_gap("0x0021E3D0", "LanguageManager::Loader", [] { hp2::recovered::LanguageManager instance; instance.Loader(); });
    failures += !expect_gap("0x0021E428", "LanguageManager::Unloader", [] { hp2::recovered::LanguageManager instance; instance.Unloader(); });
    failures += !expect_gap("0x001F1A78", "LanguageOptionsScreenBase::Sub_001F1A78", [] { hp2::recovered::LanguageOptionsScreenBase instance; instance.Sub_001F1A78(); });
    failures += !expect_gap("0x00190050", "Linear::Init", [] { hp2::recovered::Linear instance; instance.Init(); });
    failures += !expect_gap("0x001B02B8", "LoadedSkin::Sub_001B02B8", [] { hp2::recovered::LoadedSkin instance; instance.Sub_001B02B8(); });
    failures += !expect_gap("0x00238E68", "MV3Codec::Sub_00238E68", [] { hp2::recovered::MV3Codec instance; instance.Sub_00238E68(); });
    failures += !expect_gap("0x00238FB0", "MV3Codec::QueryDecompressEx", [] { hp2::recovered::MV3Codec instance; instance.QueryDecompressEx(); });
    failures += !expect_gap("0x00239050", "MV3Codec::BeginDecompress", [] { hp2::recovered::MV3Codec instance; instance.BeginDecompress(); });
    failures += !expect_gap("0x002390B8", "MV3Codec::BeginDecompressEx", [] { hp2::recovered::MV3Codec instance; instance.BeginDecompressEx(); });
    failures += !expect_gap("0x002390E8", "MV3Codec::DecompressFrame", [] { hp2::recovered::MV3Codec instance; instance.DecompressFrame(); });
    failures += !expect_gap("0x00239118", "MV3Codec::DecompressFrameEx", [] { hp2::recovered::MV3Codec instance; instance.DecompressFrameEx(); });
    failures += !expect_gap("0x00239200", "MV3Codec::EndDecompress", [] { hp2::recovered::MV3Codec instance; instance.EndDecompress(); });
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
    failures += !expect_gap("0x0021A638", "Mellovator::Sub_0021A638", [] { hp2::recovered::Mellovator instance; instance.Sub_0021A638(); });
    failures += !expect_gap("0x0021A680", "Mellovator::GetDerivative", [] { hp2::recovered::Mellovator instance; instance.GetDerivative(); });
    failures += !expect_gap("0x0021A8C8", "Mellovator::ComputeAuxiliaries", [] { hp2::recovered::Mellovator instance; instance.ComputeAuxiliaries(); });
    failures += !expect_gap("0x0021AED8", "Mellovator::ResolveForces", [] { hp2::recovered::Mellovator instance; instance.ResolveForces(); });
    failures += !expect_gap("0x0021B7F0", "Mellovator::Sub_0021B7F0", [] { hp2::recovered::Mellovator instance; instance.Sub_0021B7F0(); });
    failures += !expect_gap("0x0021B880", "Mellovator::Sub_0021B880", [] { hp2::recovered::Mellovator instance; instance.Sub_0021B880(); });
    failures += !expect_gap("0x0021B8A8", "Mellovator::Sub_0021B8A8", [] { hp2::recovered::Mellovator instance; instance.Sub_0021B8A8(); });
    failures += !expect_gap("0x0021B8D0", "Mellovator::Sub_0021B8D0", [] { hp2::recovered::Mellovator instance; instance.Sub_0021B8D0(); });
    failures += !expect_gap("0x0021B8D8", "Mellovator::GetSize", [] { hp2::recovered::Mellovator instance; instance.GetSize(); });
    failures += !expect_gap("0x002197F0", "MellowMover::Sub_002197F0", [] { hp2::recovered::MellowMover instance; instance.Sub_002197F0(); });
    failures += !expect_gap("0x002198C8", "MellowMover::DoSnapshot", [] { hp2::recovered::MellowMover instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x00219D38", "MellowMover::Move", [] { hp2::recovered::MellowMover instance; instance.Move(); });
    failures += !expect_gap("0x00219ED8", "MellowMover::ComputeAuxiliaries", [] { hp2::recovered::MellowMover instance; instance.ComputeAuxiliaries(); });
    failures += !expect_gap("0x00219F50", "MellowMover::ResolveForces", [] { hp2::recovered::MellowMover instance; instance.ResolveForces(); });
    failures += !expect_gap("0x00246BD8", "MemcardLoader::Sub_00246BD8", [] { hp2::recovered::MemcardLoader instance; instance.Sub_00246BD8(); });
    failures += !expect_gap("0x002572D8", "MemoryPool::GetAmountFree", [] { hp2::recovered::MemoryPool instance; instance.GetAmountFree(); });
    failures += !expect_gap("0x00257310", "MemoryPool::GetLargestFreeBlock", [] { hp2::recovered::MemoryPool instance; instance.GetLargestFreeBlock(); });
    failures += !expect_gap("0x001BEAD0", "Munition::Backup", [] { hp2::recovered::Munition instance; instance.Backup(); });
    failures += !expect_gap("0x00193D90", "NearbyCarList::GetCarsIndexInArray", [] { hp2::recovered::NearbyCarList instance; instance.GetCarsIndexInArray(); });
    failures += !expect_gap("0x001CD408", "NearestRoutePointInfo::IsValid", [] { hp2::recovered::NearestRoutePointInfo instance; instance.IsValid(); });
    failures += !expect_gap("0x0019B4B0", "NisCameraCoordinator::Sub_0019B4B0", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B4B0(); });
    failures += !expect_gap("0x0019B5F0", "NisCameraCoordinator::Sub_0019B5F0", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B5F0(); });
    failures += !expect_gap("0x0019B658", "NisCameraCoordinator::Sub_0019B658", [] { hp2::recovered::NisCameraCoordinator instance; instance.Sub_0019B658(); });
    failures += !expect_gap("0x0019B660", "NisCameraCoordinator::NumCameraKeys", [] { hp2::recovered::NisCameraCoordinator instance; instance.NumCameraKeys(); });
    failures += !expect_gap("0x0019B688", "NisCameraCoordinator::GetCameraKeys", [] { hp2::recovered::NisCameraCoordinator instance; instance.GetCameraKeys(); });
    failures += !expect_gap("0x001C0090", "NullController::Sub_001C0090", [] { hp2::recovered::NullController instance; instance.Sub_001C0090(); });
    failures += !expect_gap("0x001C00D0", "NullController::Sub_001C00D0", [] { hp2::recovered::NullController instance; instance.Sub_001C00D0(); });
    failures += !expect_gap("0x002A0550", "NullController::Sub_002A0550", [] { hp2::recovered::NullController instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x001309F8", "NullMover::Move", [] { hp2::recovered::NullMover instance; instance.Move(); });
    failures += !expect_gap("0x00136AF8", "NullMover::Sub_00136AF8", [] { hp2::recovered::NullMover instance; instance.Sub_00136AF8(); });
    failures += !expect_gap("0x001D5BF8", "NumberAllocator::AllocateNumber", [] { hp2::recovered::NumberAllocator instance; instance.AllocateNumber(); });
    failures += !expect_gap("0x001D5CB8", "NumberAllocator::FreeNumber", [] { hp2::recovered::NumberAllocator instance; instance.FreeNumber(); });
    failures += !expect_gap("0x001F4DC8", "OptionsScreenBase::Sub_001F4DC8", [] { hp2::recovered::OptionsScreenBase instance; instance.Sub_001F4DC8(); });
    failures += !expect_gap("0x001F4E68", "OptionsScreenBase::HandleHighlight", [] { hp2::recovered::OptionsScreenBase instance; instance.HandleHighlight(); });
    failures += !expect_gap("0x001F5B80", "OptionsScreenBase::HandleEvent", [] { hp2::recovered::OptionsScreenBase instance; instance.HandleEvent(); });
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
    failures += !expect_gap("0x001715C0", "PauseAudioOpts::FindSlider", [] { hp2::recovered::PauseAudioOpts instance; instance.FindSlider(); });
    failures += !expect_gap("0x00139678", "PhysicsVehicle::Sub_00139678", [] { hp2::recovered::PhysicsVehicle instance; instance.Sub_00139678(); });
    failures += !expect_gap("0x0018D9A0", "PlayerCarSound::Sub_0018D9A0", [] { hp2::recovered::PlayerCarSound instance; instance.Sub_0018D9A0(); });
    failures += !expect_gap("0x0020FC70", "PlayerEngineSound::Sub_0020FC70", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_0020FC70(); });
    failures += !expect_gap("0x0020FE70", "PlayerEngineSound::Sub_0020FE70", [] { hp2::recovered::PlayerEngineSound instance; instance.Sub_0020FE70(); });
    failures += !expect_gap("0x00210400", "PlayerEngineSound::DeactivateSound", [] { hp2::recovered::PlayerEngineSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x00210408", "PlayerEngineSound::ActivateSound", [] { hp2::recovered::PlayerEngineSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x00210410", "PlayerEngineSound::Mute", [] { hp2::recovered::PlayerEngineSound instance; instance.Mute(); });
    failures += !expect_gap("0x002105D0", "PlayerEngineSound::ResolveTables", [] { hp2::recovered::PlayerEngineSound instance; instance.ResolveTables(); });
    failures += !expect_gap("0x00211390", "PlayerEngineSound::Update", [] { hp2::recovered::PlayerEngineSound instance; instance.Update(); });
    failures += !expect_gap("0x00212950", "PlayerTireSound::Sub_00212950", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212950(); });
    failures += !expect_gap("0x00212CF0", "PlayerTireSound::Sub_00212CF0", [] { hp2::recovered::PlayerTireSound instance; instance.Sub_00212CF0(); });
    failures += !expect_gap("0x00212E88", "PlayerTireSound::DeactivateSound", [] { hp2::recovered::PlayerTireSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x00212E90", "PlayerTireSound::ActivateSound", [] { hp2::recovered::PlayerTireSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x002134D8", "PlayerTireSound::Mute", [] { hp2::recovered::PlayerTireSound instance; instance.Mute(); });
    failures += !expect_gap("0x002135A0", "PlayerTireSound::Update", [] { hp2::recovered::PlayerTireSound instance; instance.Update(); });
    failures += !expect_gap("0x002149F8", "PlayerTireSound::ScaleRoadNoise", [] { hp2::recovered::PlayerTireSound instance; instance.ScaleRoadNoise(); });
    failures += !expect_gap("0x00214A00", "PlayerTireSound::TriggerTireBlowOut", [] { hp2::recovered::PlayerTireSound instance; instance.TriggerTireBlowOut(); });
    failures += !expect_gap("0x001E2D00", "PreRaceStats::GetStatsForRacePosition", [] { hp2::recovered::PreRaceStats instance; instance.GetStatsForRacePosition(); });
    failures += !expect_gap("0x001657A0", "PreviewPause::GetPoint", [] { hp2::recovered::PreviewPause instance; instance.GetPoint(); });
    failures += !expect_gap("0x00130958", "PsychoDriver::Sub_00130958", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00130958(); });
    failures += !expect_gap("0x00136C58", "PsychoDriver::Sub_00136C58", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C58(); });
    failures += !expect_gap("0x00136C60", "PsychoDriver::Sub_00136C60", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136C60(); });
    failures += !expect_gap("0x00136CE8", "PsychoDriver::Sub_00136CE8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136CE8(); });
    failures += !expect_gap("0x00136CF0", "PsychoDriver::Sub_00136CF0", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00136CF0(); });
    failures += !expect_gap("0x00139760", "PsychoDriver::Sub_00139760", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139760(); });
    failures += !expect_gap("0x001398E8", "PsychoDriver::Sub_001398E8", [] { hp2::recovered::PsychoDriver instance; instance.Sub_001398E8(); });
    failures += !expect_gap("0x00139968", "PsychoDriver::Sub_00139968", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139968(); });
    failures += !expect_gap("0x00139980", "PsychoDriver::Sub_00139980", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139980(); });
    failures += !expect_gap("0x00139B28", "PsychoDriver::Sub_00139B28", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139B28(); });
    failures += !expect_gap("0x00139B68", "PsychoDriver::Sub_00139B68", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139B68(); });
    failures += !expect_gap("0x00139C48", "PsychoDriver::Sub_00139C48", [] { hp2::recovered::PsychoDriver instance; instance.Sub_00139C48(); });
    failures += !expect_gap("0x0013A3E0", "PsychoDriver::SetTarget", [] { hp2::recovered::PsychoDriver instance; instance.SetTarget(); });
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
    failures += !expect_gap("0x00153A00", "QueuedJoystickEvents::AddEvent", [] { hp2::recovered::QueuedJoystickEvents instance; instance.AddEvent(); });
    failures += !expect_gap("0x00153A50", "QueuedJoystickEvents::RemoveEvent", [] { hp2::recovered::QueuedJoystickEvents instance; instance.RemoveEvent(); });
    failures += !expect_gap("0x0023D368", "QuickSpline::FixupEndpoints", [] { hp2::recovered::QuickSpline instance; instance.FixupEndpoints(); });
    failures += !expect_gap("0x0023D600", "QuickSpline::GenerateExtrapolatedControlPoints", [] { hp2::recovered::QuickSpline instance; instance.GenerateExtrapolatedControlPoints(); });
    failures += !expect_gap("0x0023D868", "QuickSpline::FindClosestControlPoint", [] { hp2::recovered::QuickSpline instance; instance.FindClosestControlPoint(); });
    failures += !expect_gap("0x00148218", "Race::SortRacingCarsByLeadership", [] { hp2::recovered::Race instance; instance.SortRacingCarsByLeadership(); });
    failures += !expect_gap("0x001491C0", "Race::GetFinisher", [] { hp2::recovered::Race instance; instance.GetFinisher(); });
    failures += !expect_gap("0x00201558", "RaceEvent::RacerIndexFromDriverNumber", [] { hp2::recovered::RaceEvent instance; instance.RacerIndexFromDriverNumber(); });
    failures += !expect_gap("0x00201578", "RaceEvent::DriverNumberFromRacerIndex", [] { hp2::recovered::RaceEvent instance; instance.DriverNumberFromRacerIndex(); });
    failures += !expect_gap("0x00201858", "RaceEvent::GetRankState", [] { hp2::recovered::RaceEvent instance; instance.GetRankState(); });
    failures += !expect_gap("0x00201880", "RaceEvent::GetTournamentState", [] { hp2::recovered::RaceEvent instance; instance.GetTournamentState(); });
    failures += !expect_gap("0x002018A0", "RaceEvent::GetKoState", [] { hp2::recovered::RaceEvent instance; instance.GetKoState(); });
    failures += !expect_gap("0x002018C0", "RaceEvent::GetTimeState", [] { hp2::recovered::RaceEvent instance; instance.GetTimeState(); });
    failures += !expect_gap("0x00202088", "RaceEvent::GetRaceCount", [] { hp2::recovered::RaceEvent instance; instance.GetRaceCount(); });
    failures += !expect_gap("0x001EDC50", "RaceParameters::ClearEliminatedDrivers", [] { hp2::recovered::RaceParameters instance; instance.ClearEliminatedDrivers(); });
    failures += !expect_gap("0x001EDCD0", "RaceParameters::GetNumEliminated", [] { hp2::recovered::RaceParameters instance; instance.GetNumEliminated(); });
    failures += !expect_gap("0x001EDDB8", "RaceParameters::GetLocationNumber", [] { hp2::recovered::RaceParameters instance; instance.GetLocationNumber(); });
    failures += !expect_gap("0x001EE390", "RaceParameters::IsFreeRun", [] { hp2::recovered::RaceParameters instance; instance.IsFreeRun(); });
    failures += !expect_gap("0x001DFFE8", "RaceResultsScreen::Sub_001DFFE8", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001DFFE8(); });
    failures += !expect_gap("0x001E0838", "RaceResultsScreen::Scroll", [] { hp2::recovered::RaceResultsScreen instance; instance.Scroll(); });
    failures += !expect_gap("0x001E0868", "RaceResultsScreen::Tick", [] { hp2::recovered::RaceResultsScreen instance; instance.Tick(); });
    failures += !expect_gap("0x001E23B8", "RaceResultsScreen::Sub_001E23B8", [] { hp2::recovered::RaceResultsScreen instance; instance.Sub_001E23B8(); });
    failures += !expect_gap("0x001B7AB0", "RaceSetupBaseClass::FindChangeable", [] { hp2::recovered::RaceSetupBaseClass instance; instance.FindChangeable(); });
    failures += !expect_gap("0x001B7AE8", "RaceSetupBaseClass::FindChangeableByLanguageID", [] { hp2::recovered::RaceSetupBaseClass instance; instance.FindChangeableByLanguageID(); });
    failures += !expect_gap("0x001493B8", "RacingCar::Sub_001493B8", [] { hp2::recovered::RacingCar instance; instance.Sub_001493B8(); });
    failures += !expect_gap("0x00129880", "RainSplash::Recalc", [] { hp2::recovered::RainSplash instance; instance.Recalc(); });
    failures += !expect_gap("0x0023A3D8", "RealController::Sub_0023A3D8", [] { hp2::recovered::RealController instance; instance.Sub_0023A3D8(); });
    failures += !expect_gap("0x0023A4D8", "RealController::Sub_0023A4D8", [] { hp2::recovered::RealController instance; instance.Sub_0023A4D8(); });
    failures += !expect_gap("0x0023A560", "RealController::DoSnapshot", [] { hp2::recovered::RealController instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x0023AA50", "RealController::Think", [] { hp2::recovered::RealController instance; instance.Think(); });
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
    failures += !expect_gap("0x001EC510", "ReplayScreen::HasCameraMover", [] { hp2::recovered::ReplayScreen instance; instance.HasCameraMover(); });
    failures += !expect_gap("0x001ED410", "ReplayScreen::GetTimelinePositionX", [] { hp2::recovered::ReplayScreen instance; instance.GetTimelinePositionX(); });
    failures += !expect_gap("0x0013CD68", "ReplaySnapshot::GetChecksum", [] { hp2::recovered::ReplaySnapshot instance; instance.GetChecksum(); });
    failures += !expect_gap("0x0013CE78", "ReplaySnapshot::ReadData", [] { hp2::recovered::ReplaySnapshot instance; instance.ReadData(); });
    failures += !expect_gap("0x001995E0", "ResourceFile::GetSize", [] { hp2::recovered::ResourceFile instance; instance.GetSize(); });
    failures += !expect_gap("0x002393A0", "RingBuffer::GetLastFrame", [] { hp2::recovered::RingBuffer instance; instance.GetLastFrame(); });
    failures += !expect_gap("0x002395A0", "RingBuffer::ContinuousAvail", [] { hp2::recovered::RingBuffer instance; instance.ContinuousAvail(); });
    failures += !expect_gap("0x00239A58", "RingBuffer::GetFrame", [] { hp2::recovered::RingBuffer instance; instance.GetFrame(); });
    failures += !expect_gap("0x00239B48", "RingBuffer::ReleaseFrame", [] { hp2::recovered::RingBuffer instance; instance.ReleaseFrame(); });
    failures += !expect_gap("0x001BF3D8", "Rocket::Sub_001BF3D8", [] { hp2::recovered::Rocket instance; instance.Sub_001BF3D8(); });
    failures += !expect_gap("0x001BF4D0", "Rocket::MaybeExplodeWarhead", [] { hp2::recovered::Rocket instance; instance.MaybeExplodeWarhead(); });
    failures += !expect_gap("0x001BF598", "Rocket::Move", [] { hp2::recovered::Rocket instance; instance.Move(); });
    failures += !expect_gap("0x001BF8F8", "Rocket::ConsiderImpulse", [] { hp2::recovered::Rocket instance; instance.ConsiderImpulse(); });
    failures += !expect_gap("0x001BF908", "Rocket::ApplyImpulse", [] { hp2::recovered::Rocket instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x001BF910", "Rocket::DoSnapshot", [] { hp2::recovered::Rocket instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001BFD10", "Rocket::GetSmokeType", [] { hp2::recovered::Rocket instance; instance.GetSmokeType(); });
    failures += !expect_gap("0x0016B438", "ScreenStack::Pop", [] { hp2::recovered::ScreenStack instance; instance.Pop(); });
    failures += !expect_gap("0x00198878", "SelectCarScreen::AreAllSelectorsReady", [] { hp2::recovered::SelectCarScreen instance; instance.AreAllSelectorsReady(); });
    failures += !expect_gap("0x0019E048", "SelectableTrack::GetTimeOfDay", [] { hp2::recovered::SelectableTrack instance; instance.GetTimeOfDay(); });
    failures += !expect_gap("0x0019E0A8", "SelectableTrack::ScrollTimeOfDay", [] { hp2::recovered::SelectableTrack instance; instance.ScrollTimeOfDay(); });
    failures += !expect_gap("0x0019E7B0", "SelectableTrackManager::GetNextTrack", [] { hp2::recovered::SelectableTrackManager instance; instance.GetNextTrack(); });
    failures += !expect_gap("0x0019E7D0", "SelectableTrackManager::GetPrevTrack", [] { hp2::recovered::SelectableTrackManager instance; instance.GetPrevTrack(); });
    failures += !expect_gap("0x0018F388", "SirenSound::Sub_0018F388", [] { hp2::recovered::SirenSound instance; instance.Sub_0018F388(); });
    failures += !expect_gap("0x0018F4D0", "SirenSound3d::ActivateSound", [] { hp2::recovered::SirenSound3d instance; instance.ActivateSound(); });
    failures += !expect_gap("0x0018F508", "SirenSound3d::DeactivateSound", [] { hp2::recovered::SirenSound3d instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x0018F528", "SirenSound3d::Mute", [] { hp2::recovered::SirenSound3d instance; instance.Mute(); });
    failures += !expect_gap("0x0018F548", "SirenSound3d::Update", [] { hp2::recovered::SirenSound3d instance; instance.Update(); });
    failures += !expect_gap("0x0018F710", "SirenSound3d::Sub_0018F710", [] { hp2::recovered::SirenSound3d instance; instance.Sub_0018F710(); });
    failures += !expect_gap("0x00257DF8", "SlotPool::FlushSlotPool", [] { hp2::recovered::SlotPool instance; instance.FlushSlotPool(); });
    failures += !expect_gap("0x00257F28", "SlotPool::CleanupExpandedSlotPools", [] { hp2::recovered::SlotPool instance; instance.CleanupExpandedSlotPools(); });
    failures += !expect_gap("0x00258200", "SlotPool::FastMalloc", [] { hp2::recovered::SlotPool instance; instance.FastMalloc(); });
    failures += !expect_gap("0x002584B0", "SlotPoolManager::NewSlotPool", [] { hp2::recovered::SlotPoolManager instance; instance.NewSlotPool(); });
    failures += !expect_gap("0x00258548", "SlotPoolManager::DeleteSlotPool", [] { hp2::recovered::SlotPoolManager instance; instance.DeleteSlotPool(); });
    failures += !expect_gap("0x001BEC28", "SmokeBomb::Sub_001BEC28", [] { hp2::recovered::SmokeBomb instance; instance.Sub_001BEC28(); });
    failures += !expect_gap("0x001BED20", "SmokeBomb::MaybeExplodeWarhead", [] { hp2::recovered::SmokeBomb instance; instance.MaybeExplodeWarhead(); });
    failures += !expect_gap("0x001BED78", "SmokeBomb::Move", [] { hp2::recovered::SmokeBomb instance; instance.Move(); });
    failures += !expect_gap("0x001BFE10", "SmokeBomb::GetSmokeType", [] { hp2::recovered::SmokeBomb instance; instance.GetSmokeType(); });
    failures += !expect_gap("0x00220690", "Smokeable::Sub_00220690", [] { hp2::recovered::Smokeable instance; instance.Sub_00220690(); });
    failures += !expect_gap("0x002208A0", "Smokeable::DoSnapshot", [] { hp2::recovered::Smokeable instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x00220CE8", "Smokeable::Move", [] { hp2::recovered::Smokeable instance; instance.Move(); });
    failures += !expect_gap("0x002212D0", "Smokeable::MaybeExplodeWarhead", [] { hp2::recovered::Smokeable instance; instance.MaybeExplodeWarhead(); });
    failures += !expect_gap("0x00221340", "Smokeable::ConsiderImpulse", [] { hp2::recovered::Smokeable instance; instance.ConsiderImpulse(); });
    failures += !expect_gap("0x00221F00", "Smokeable::GetFriction", [] { hp2::recovered::Smokeable instance; instance.GetFriction(); });
    failures += !expect_gap("0x00221F10", "Smokeable::GetBounciness", [] { hp2::recovered::Smokeable instance; instance.GetBounciness(); });
    failures += !expect_gap("0x00221F20", "Smokeable::GetTerrainType", [] { hp2::recovered::Smokeable instance; instance.GetTerrainType(); });
    failures += !expect_gap("0x00221F30", "Smokeable::IsMoveable", [] { hp2::recovered::Smokeable instance; instance.IsMoveable(); });
    failures += !expect_gap("0x00221F58", "Smokeable::GetInertiaTensorInverse", [] { hp2::recovered::Smokeable instance; instance.GetInertiaTensorInverse(); });
    failures += !expect_gap("0x00221F88", "Smokeable::GetMassInverse", [] { hp2::recovered::Smokeable instance; instance.GetMassInverse(); });
    failures += !expect_gap("0x00221FB8", "Smokeable::AreCollisionsEnabled", [] { hp2::recovered::Smokeable instance; instance.AreCollisionsEnabled(); });
    failures += !expect_gap("0x00221FD8", "Smokeable::IsBlinking", [] { hp2::recovered::Smokeable instance; instance.IsBlinking(); });
    failures += !expect_gap("0x00221FF0", "Smokeable::ApplyImpulse", [] { hp2::recovered::Smokeable instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x00222010", "Smokeable::GetRotation", [] { hp2::recovered::Smokeable instance; instance.GetRotation(); });
    failures += !expect_gap("0x00222020", "Smokeable::GetVelocity", [] { hp2::recovered::Smokeable instance; instance.GetVelocity(); });
    failures += !expect_gap("0x00222030", "Smokeable::GetPosition", [] { hp2::recovered::Smokeable instance; instance.GetPosition(); });
    failures += !expect_gap("0x00222050", "Smokeable::GetSmokeType", [] { hp2::recovered::Smokeable instance; instance.GetSmokeType(); });
    failures += !expect_gap("0x00208B28", "Sound3d::Activate", [] { hp2::recovered::Sound3d instance; instance.Activate(); });
    failures += !expect_gap("0x00208C78", "Sound3d::UpdateCamRelativeDistAndVelocity", [] { hp2::recovered::Sound3d instance; instance.UpdateCamRelativeDistAndVelocity(); });
    failures += !expect_gap("0x00208FE0", "Sound3d::AzimuthToPan", [] { hp2::recovered::Sound3d instance; instance.AzimuthToPan(); });
    failures += !expect_gap("0x002091A0", "Sound3d::Sub_002091A0", [] { hp2::recovered::Sound3d instance; instance.Sub_002091A0(); });
    failures += !expect_gap("0x002095D0", "Sound3dSample::Sub_002095D0", [] { hp2::recovered::Sound3dSample instance; instance.Sub_002095D0(); });
    failures += !expect_gap("0x002096A0", "Sound3dSample::Sub_002096A0", [] { hp2::recovered::Sound3dSample instance; instance.Sub_002096A0(); });
    failures += !expect_gap("0x002096F8", "Sound3dSample::Sub_002096F8", [] { hp2::recovered::Sound3dSample instance; instance.Sub_002096F8(); });
    failures += !expect_gap("0x00209DC0", "Sound3dSample::ActivateSound", [] { hp2::recovered::Sound3dSample instance; instance.ActivateSound(); });
    failures += !expect_gap("0x00209E18", "Sound3dSample::DeactivateSound", [] { hp2::recovered::Sound3dSample instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x00209E98", "Sound3dSample::Mute", [] { hp2::recovered::Sound3dSample instance; instance.Mute(); });
    failures += !expect_gap("0x001CDDE0", "SoundArea::Initialize", [] { hp2::recovered::SoundArea instance; instance.Initialize(); });
    failures += !expect_gap("0x00189238", "Spline::Sub_00189238", [] { hp2::recovered::Spline instance; instance.Sub_00189238(); });
    failures += !expect_gap("0x0018A030", "Spline::LinearEstimateLength", [] { hp2::recovered::Spline instance; instance.LinearEstimateLength(); });
    failures += !expect_gap("0x001D9048", "StaticCollisionBody::ChangeInEnergy", [] { hp2::recovered::StaticCollisionBody instance; instance.ChangeInEnergy(); });
    failures += !expect_gap("0x001D9058", "StaticCollisionBody::IsMoveable", [] { hp2::recovered::StaticCollisionBody instance; instance.IsMoveable(); });
    failures += !expect_gap("0x001D9060", "StaticCollisionBody::AreCollisionsEnabled", [] { hp2::recovered::StaticCollisionBody instance; instance.AreCollisionsEnabled(); });
    failures += !expect_gap("0x001D9070", "StaticCollisionBody::ApplyImpulse", [] { hp2::recovered::StaticCollisionBody instance; instance.ApplyImpulse(); });
    failures += !expect_gap("0x001D9078", "StaticCollisionBody::ConsiderImpulse", [] { hp2::recovered::StaticCollisionBody instance; instance.ConsiderImpulse(); });
    failures += !expect_gap("0x001D9088", "StaticCollisionBody::SetToPhysicsMode", [] { hp2::recovered::StaticCollisionBody instance; instance.SetToPhysicsMode(); });
    failures += !expect_gap("0x001D9090", "StaticCollisionBody::GetFriction", [] { hp2::recovered::StaticCollisionBody instance; instance.GetFriction(); });
    failures += !expect_gap("0x001D90A0", "StaticCollisionBody::GetBounciness", [] { hp2::recovered::StaticCollisionBody instance; instance.GetBounciness(); });
    failures += !expect_gap("0x001D90B0", "StaticCollisionBody::GetInertiaTensorInverse", [] { hp2::recovered::StaticCollisionBody instance; instance.GetInertiaTensorInverse(); });
    failures += !expect_gap("0x001D90C0", "StaticCollisionBody::GetMassInverse", [] { hp2::recovered::StaticCollisionBody instance; instance.GetMassInverse(); });
    failures += !expect_gap("0x001D90D0", "StaticCollisionBody::GetTestImpulseVelocities", [] { hp2::recovered::StaticCollisionBody instance; instance.GetTestImpulseVelocities(); });
    failures += !expect_gap("0x001D90F0", "StaticCollisionBody::GetCurrentVelocities", [] { hp2::recovered::StaticCollisionBody instance; instance.GetCurrentVelocities(); });
    failures += !expect_gap("0x001D9110", "StaticCollisionBody::RecomputeAuxilliaries", [] { hp2::recovered::StaticCollisionBody instance; instance.RecomputeAuxilliaries(); });
    failures += !expect_gap("0x001D9118", "StaticCollisionBody::Interpolate", [] { hp2::recovered::StaticCollisionBody instance; instance.Interpolate(); });
    failures += !expect_gap("0x001D9158", "StaticCollisionBody::Backup", [] { hp2::recovered::StaticCollisionBody instance; instance.Backup(); });
    failures += !expect_gap("0x001D9160", "StaticCollisionBody::Move", [] { hp2::recovered::StaticCollisionBody instance; instance.Move(); });
    failures += !expect_gap("0x001D9168", "StaticCollisionBody::GetMaxBackupTime", [] { hp2::recovered::StaticCollisionBody instance; instance.GetMaxBackupTime(); });
    failures += !expect_gap("0x001D9178", "StaticCollisionBody::SetPosition", [] { hp2::recovered::StaticCollisionBody instance; instance.SetPosition(); });
    failures += !expect_gap("0x001D9180", "StaticCollisionBody::GetRotation", [] { hp2::recovered::StaticCollisionBody instance; instance.GetRotation(); });
    failures += !expect_gap("0x001D9190", "StaticCollisionBody::GetVelocity", [] { hp2::recovered::StaticCollisionBody instance; instance.GetVelocity(); });
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
    failures += !expect_gap("0x00100A50", "TextureInfo::GetScroll", [] { hp2::recovered::TextureInfo instance; instance.GetScroll(); });
    failures += !expect_gap("0x00100AF8", "TextureInfo::GetAlphaChannel", [] { hp2::recovered::TextureInfo instance; instance.GetAlphaChannel(); });
    failures += !expect_gap("0x00100B98", "TextureInfo::GetColourChannel", [] { hp2::recovered::TextureInfo instance; instance.GetColourChannel(); });
    failures += !expect_gap("0x00100BF0", "TextureInfo::Print", [] { hp2::recovered::TextureInfo instance; instance.Print(); });
    failures += !expect_gap("0x001AC3F0", "TexturePack::GetLoadedTexture", [] { hp2::recovered::TexturePack instance; instance.GetLoadedTexture(); });
    failures += !expect_gap("0x001AC468", "TexturePack::GetTexture", [] { hp2::recovered::TexturePack instance; instance.GetTexture(); });
    failures += !expect_gap("0x001AC4B0", "TexturePack::GetNthTexture", [] { hp2::recovered::TexturePack instance; instance.GetNthTexture(); });
    failures += !expect_gap("0x001AC780", "TexturePack::FindLoadedTextureData", [] { hp2::recovered::TexturePack instance; instance.FindLoadedTextureData(); });
    failures += !expect_gap("0x001AC7D0", "TexturePack::FindNotYetLoadedNeighbourTextureData", [] { hp2::recovered::TexturePack instance; instance.FindNotYetLoadedNeighbourTextureData(); });
    failures += !expect_gap("0x001AD008", "TexturePack::GetLoadedSize", [] { hp2::recovered::TexturePack instance; instance.GetLoadedSize(); });
    failures += !expect_gap("0x00146AE8", "TimeAroundTrackTable::GetTimeTableEntry", [] { hp2::recovered::TimeAroundTrackTable instance; instance.GetTimeTableEntry(); });
    failures += !expect_gap("0x001A3B28", "Timer::GetHoursMinsSeconds", [] { hp2::recovered::Timer instance; instance.GetHoursMinsSeconds(); });
    failures += !expect_gap("0x001A3CF0", "Timer::CountDown", [] { hp2::recovered::Timer instance; instance.CountDown(); });
    failures += !expect_gap("0x0016F190", "TitleScreen::Sub_0016F190", [] { hp2::frontend::TitleScreen instance; instance.Sub_0016F190(); });
    failures += !expect_gap("0x00152070", "TopologyCoordinate::GetElevation", [] { hp2::recovered::TopologyCoordinate instance; instance.GetElevation(); });
    failures += !expect_gap("0x00152B10", "TopologyTree::Sub_00152B10", [] { hp2::recovered::TopologyTree instance; instance.Sub_00152B10(); });
    failures += !expect_gap("0x00152C10", "TopologyTree::Loader", [] { hp2::recovered::TopologyTree instance; instance.Loader(); });
    failures += !expect_gap("0x00152D30", "TopologyTree::Unloader", [] { hp2::recovered::TopologyTree instance; instance.Unloader(); });
    failures += !expect_gap("0x00152DB0", "TopologyTree::GetSquareCorner", [] { hp2::recovered::TopologyTree instance; instance.GetSquareCorner(); });
    failures += !expect_gap("0x00152E08", "TopologyTree::GetSquare", [] { hp2::recovered::TopologyTree instance; instance.GetSquare(); });
    failures += !expect_gap("0x00152F38", "TopologyTree::GetIntersectingPolygons", [] { hp2::recovered::TopologyTree instance; instance.GetIntersectingPolygons(); });
    failures += !expect_gap("0x001DD5D0", "TourneyStandingsScreen::Sub_001DD5D0", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001DD5D0(); });
    failures += !expect_gap("0x001E23A0", "TourneyStandingsScreen::Sub_001E23A0", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E23A0(); });
    failures += !expect_gap("0x001E2588", "TourneyStandingsScreen::Sub_001E2588", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E2588(); });
    failures += !expect_gap("0x001E2590", "TourneyStandingsScreen::Sub_001E2590", [] { hp2::recovered::TourneyStandingsScreen instance; instance.Sub_001E2590(); });
    failures += !expect_gap("0x001577D0", "TrackCarCameraMover::Sub_001577D0", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_001577D0(); });
    failures += !expect_gap("0x001577D8", "TrackCarCameraMover::Sub_001577D8", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_001577D8(); });
    failures += !expect_gap("0x00157850", "TrackCarCameraMover::Sub_00157850", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00157850(); });
    failures += !expect_gap("0x00157920", "TrackCarCameraMover::Sub_00157920", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00157920(); });
    failures += !expect_gap("0x00159130", "TrackCarCameraMover::Sub_00159130", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159130(); });
    failures += !expect_gap("0x00159490", "TrackCarCameraMover::Sub_00159490", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159490(); });
    failures += !expect_gap("0x00159520", "TrackCarCameraMover::Sub_00159520", [] { hp2::recovered::TrackCarCameraMover instance; instance.Sub_00159520(); });
    failures += !expect_gap("0x00159540", "TrackCarCameraMover::CalcSpringForce", [] { hp2::recovered::TrackCarCameraMover instance; instance.CalcSpringForce(); });
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
    failures += !expect_gap("0x0015A2A0", "TrackFlyCameraMover::IsFinished", [] { hp2::recovered::TrackFlyCameraMover instance; instance.IsFinished(); });
    failures += !expect_gap("0x0015FAF8", "TrackFlyCameraMover::Sub_0015FAF8", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FAF8(); });
    failures += !expect_gap("0x0015FD10", "TrackFlyCameraMover::Sub_0015FD10", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD10(); });
    failures += !expect_gap("0x0015FD18", "TrackFlyCameraMover::Sub_0015FD18", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD18(); });
    failures += !expect_gap("0x0015FD20", "TrackFlyCameraMover::Sub_0015FD20", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD20(); });
    failures += !expect_gap("0x0015FD28", "TrackFlyCameraMover::Sub_0015FD28", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD28(); });
    failures += !expect_gap("0x0015FD30", "TrackFlyCameraMover::Sub_0015FD30", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD30(); });
    failures += !expect_gap("0x0015FD38", "TrackFlyCameraMover::Sub_0015FD38", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD38(); });
    failures += !expect_gap("0x0015FD40", "TrackFlyCameraMover::Sub_0015FD40", [] { hp2::recovered::TrackFlyCameraMover instance; instance.Sub_0015FD40(); });
    failures += !expect_gap("0x001BCA08", "TrackMap::EmitPoint", [] { hp2::recovered::TrackMap instance; instance.EmitPoint(); });
    failures += !expect_gap("0x001BDE60", "TrackMarkerFile::GetTargetedMarker", [] { hp2::recovered::TrackMarkerFile instance; instance.GetTargetedMarker(); });
    failures += !expect_gap("0x002086B8", "TrackPathCoordinate::Sub_002086B8", [] { hp2::recovered::TrackPathCoordinate instance; instance.Sub_002086B8(); });
    failures += !expect_gap("0x00207C68", "TrackPathManager::FindClosestPoint", [] { hp2::recovered::TrackPathManager instance; instance.FindClosestPoint(); });
    failures += !expect_gap("0x00207ED0", "TrackPathManager::FindClosestAdjacentPointOnSamePath", [] { hp2::recovered::TrackPathManager instance; instance.FindClosestAdjacentPointOnSamePath(); });
    failures += !expect_gap("0x00208598", "TrackPathManager::CompareDistances", [] { hp2::recovered::TrackPathManager instance; instance.CompareDistances(); });
    failures += !expect_gap("0x002085E8", "TrackPathManager::ComparePercentDistances", [] { hp2::recovered::TrackPathManager instance; instance.ComparePercentDistances(); });
    failures += !expect_gap("0x001517D8", "TrackPolygon::PointInShaft", [] { hp2::recovered::TrackPolygon instance; instance.PointInShaft(); });
    failures += !expect_gap("0x001D8880", "TrackPolygonCollisionBody::Sub_001D8880", [] { hp2::recovered::TrackPolygonCollisionBody instance; instance.Sub_001D8880(); });
    failures += !expect_gap("0x001D8918", "TrackPolygonCollisionBody::DoSnapshot", [] { hp2::recovered::TrackPolygonCollisionBody instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x001D8B18", "TrackPolygonCollisionBody::GetPosition", [] { hp2::recovered::TrackPolygonCollisionBody instance; instance.GetPosition(); });
    failures += !expect_gap("0x001D8B28", "TrackPolygonCollisionBody::GetTerrainType", [] { hp2::recovered::TrackPolygonCollisionBody instance; instance.GetTerrainType(); });
    failures += !expect_gap("0x0019EF00", "TrackRecordDisplayer::Sub_0019EF00", [] { hp2::recovered::TrackRecordDisplayer instance; instance.Sub_0019EF00(); });
    failures += !expect_gap("0x001C1910", "TrackRoute::GetRelativePoint", [] { hp2::recovered::TrackRoute instance; instance.GetRelativePoint(); });
    failures += !expect_gap("0x001C1D30", "TrackRoute::FindClosestPointAhead", [] { hp2::recovered::TrackRoute instance; instance.FindClosestPointAhead(); });
    failures += !expect_gap("0x001C2780", "TrackRoute::DistanceIsWithinRoute", [] { hp2::recovered::TrackRoute instance; instance.DistanceIsWithinRoute(); });
    failures += !expect_gap("0x001C4188", "TrackRouteEditor::GetCameraPosition", [] { hp2::recovered::TrackRouteEditor instance; instance.GetCameraPosition(); });
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
    failures += !expect_gap("0x001C28A0", "TrackRouteManager::AddRouteToRightList", [] { hp2::recovered::TrackRouteManager instance; instance.AddRouteToRightList(); });
    failures += !expect_gap("0x001C1340", "TrackRoutePoint::Sub_001C1340", [] { hp2::recovered::TrackRoutePoint instance; instance.Sub_001C1340(); });
    failures += !expect_gap("0x001C1408", "TrackRoutePoint::GetPosition", [] { hp2::recovered::TrackRoutePoint instance; instance.GetPosition(); });
    failures += !expect_gap("0x001C1490", "TrackRoutePoint::SetWidth", [] { hp2::recovered::TrackRoutePoint instance; instance.SetWidth(); });
    failures += !expect_gap("0x002455B0", "TrackRoutePoint::GetAmountOutOfBounds", [] { hp2::recovered::TrackRoutePoint instance; instance.GetAmountOutOfBounds(); });
    failures += !expect_gap("0x0023A070", "TrackedController::Sub_0023A070", [] { hp2::recovered::TrackedController instance; instance.Sub_0023A070(); });
    failures += !expect_gap("0x0023A088", "TrackedController::Think", [] { hp2::recovered::TrackedController instance; instance.Think(); });
    failures += !expect_gap("0x0023A0F0", "TrackedController::DoSnapshot", [] { hp2::recovered::TrackedController instance; instance.DoSnapshot(); });
    failures += !expect_gap("0x0023A178", "TrackedController::SetCurrentRoute", [] { hp2::recovered::TrackedController instance; instance.SetCurrentRoute(); });
    failures += !expect_gap("0x0023A198", "TrackedController::GetCurrentRoute", [] { hp2::recovered::TrackedController instance; instance.GetCurrentRoute(); });
    failures += !expect_gap("0x0023A1C8", "TrackedController::FindWhereIAm", [] { hp2::recovered::TrackedController instance; instance.FindWhereIAm(); });
    failures += !expect_gap("0x0023A2F8", "TrackedController::JustReset", [] { hp2::recovered::TrackedController instance; instance.JustReset(); });
    failures += !expect_gap("0x0023A300", "TrackedController::GetAmountOutOfBounds", [] { hp2::recovered::TrackedController instance; instance.GetAmountOutOfBounds(); });
    failures += !expect_gap("0x0023A3D0", "TrackedController::GetCurrentRoutePoint", [] { hp2::recovered::TrackedController instance; instance.GetCurrentRoutePoint(); });
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
    failures += !expect_gap("0x00187080", "TransientStats::GetDriver", [] { hp2::recovered::TransientStats instance; instance.GetDriver(); });
    failures += !expect_gap("0x001F1528", "UserNameOptionsScreen::DisplayGender", [] { hp2::recovered::UserNameOptionsScreen instance; instance.DisplayGender(); });
    failures += !expect_gap("0x001D2A48", "VCEdge::SetVoronoiPlanes", [] { hp2::recovered::VCEdge instance; instance.SetVoronoiPlanes(); });
    failures += !expect_gap("0x001D30A8", "VCFace::Move", [] { hp2::recovered::VCFace instance; instance.Move(); });
    failures += !expect_gap("0x001D3100", "VCPolyhedron::Sub_001D3100", [] { hp2::recovered::VCPolyhedron instance; instance.Sub_001D3100(); });
    failures += !expect_gap("0x001D3200", "VCPolyhedron::Init", [] { hp2::recovered::VCPolyhedron instance; instance.Init(); });
    failures += !expect_gap("0x001D3398", "VCPolyhedron::NewVoronoiPlanes", [] { hp2::recovered::VCPolyhedron instance; instance.NewVoronoiPlanes(); });
    failures += !expect_gap("0x001D3430", "VCPolyhedron::NewVertex", [] { hp2::recovered::VCPolyhedron instance; instance.NewVertex(); });
    failures += !expect_gap("0x001D3498", "VCPolyhedron::NewEdge", [] { hp2::recovered::VCPolyhedron instance; instance.NewEdge(); });
    failures += !expect_gap("0x001D3540", "VCPolyhedron::NewFace", [] { hp2::recovered::VCPolyhedron instance; instance.NewFace(); });
    failures += !expect_gap("0x001D3BA8", "VCPolyhedron::GetFeatureCode", [] { hp2::recovered::VCPolyhedron instance; instance.GetFeatureCode(); });
    failures += !expect_gap("0x001D3C60", "VCPolyhedron::GetFeature", [] { hp2::recovered::VCPolyhedron instance; instance.GetFeature(); });
    failures += !expect_gap("0x001D0058", "VCTransform::TransformVector", [] { hp2::recovered::VCTransform instance; instance.TransformVector(); });
    failures += !expect_gap("0x001D00B0", "VCTransform::TransformPoint", [] { hp2::recovered::VCTransform instance; instance.TransformPoint(); });
    failures += !expect_gap("0x001D00F8", "VCTransformedFeature::TransformVertex", [] { hp2::recovered::VCTransformedFeature instance; instance.TransformVertex(); });
    failures += !expect_gap("0x001D0168", "VCTransformedFeature::TransformEdge", [] { hp2::recovered::VCTransformedFeature instance; instance.TransformEdge(); });
    failures += !expect_gap("0x001D2800", "VCVertex::SetVoronoiPlanes", [] { hp2::recovered::VCVertex instance; instance.SetVoronoiPlanes(); });
    failures += !expect_gap("0x001D2950", "VCVertex::GetNextEdgeInFace", [] { hp2::recovered::VCVertex instance; instance.GetNextEdgeInFace(); });
    failures += !expect_gap("0x001D29E8", "VCVertex::Move", [] { hp2::recovered::VCVertex instance; instance.Move(); });
    failures += !expect_gap("0x001D0228", "VCVoronoiClip::Sub_001D0228", [] { hp2::recovered::VCVoronoiClip instance; instance.Sub_001D0228(); });
    failures += !expect_gap("0x001D0250", "VCVoronoiClip::ClipPlanes", [] { hp2::recovered::VCVoronoiClip instance; instance.ClipPlanes(); });
    failures += !expect_gap("0x001D0890", "VCWitness::Sub_001D0890", [] { hp2::recovered::VCWitness instance; instance.Sub_001D0890(); });
    failures += !expect_gap("0x001010A0", "VifController::Reset", [] { hp2::recovered::VifController instance; instance.Reset(); });
    failures += !expect_gap("0x001942D0", "VisibleCar::SetCurrentSkinNumber", [] { hp2::recovered::VisibleCar instance; instance.SetCurrentSkinNumber(); });
    failures += !expect_gap("0x00194350", "VisibleCar::NextSkin", [] { hp2::recovered::VisibleCar instance; instance.NextSkin(); });
    failures += !expect_gap("0x00194380", "VisibleCar::PrevSkin", [] { hp2::recovered::VisibleCar instance; instance.PrevSkin(); });
    failures += !expect_gap("0x0014F340", "VisibleZone::FindIndexTo", [] { hp2::recovered::VisibleZone instance; instance.FindIndexTo(); });
    failures += !expect_gap("0x0012B3B8", "WheelDevice::Sub_0012B3B8", [] { hp2::recovered::WheelDevice instance; instance.Sub_0012B3B8(); });
    failures += !expect_gap("0x0012BC68", "WheelDevice::RecalculateDeviceNumber", [] { hp2::recovered::WheelDevice instance; instance.RecalculateDeviceNumber(); });
    failures += !expect_gap("0x0014D338", "World::BuildCarTable", [] { hp2::recovered::World instance; instance.BuildCarTable(); });
    failures += !expect_gap("0x0014D508", "World::GetCarByNumber", [] { hp2::recovered::World instance; instance.GetCarByNumber(); });
    failures += !expect_gap("0x0014D638", "World::ResetTimeScale", [] { hp2::recovered::World instance; instance.ResetTimeScale(); });
    failures += !expect_gap("0x0014E3E8", "World::Sub_0014E3E8", [] { hp2::recovered::World instance; instance.Sub_0014E3E8(); });
    failures += !expect_gap("0x0014E440", "World::Sub_0014E440", [] { hp2::recovered::World instance; instance.Sub_0014E440(); });
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
    failures += !expect_gap("0x0020BE48", "actionHelicopter_t::execute", [] { hp2::recovered::actionHelicopter_t instance; instance.execute(); });
    failures += !expect_gap("0x002705C0", "bEffect::SetParameter", [] { hp2::recovered::bEffect instance; instance.SetParameter(); });
    failures += !expect_gap("0x002503F8", "bList::GetNode", [] { hp2::recovered::bList instance; instance.GetNode(); });
    failures += !expect_gap("0x00250428", "bList::TraversebList", [] { hp2::recovered::bList instance; instance.TraversebList(); });
    failures += !expect_gap("0x00250458", "bList::AddTail", [] { hp2::recovered::bList instance; instance.AddTail(); });
    failures += !expect_gap("0x001EB050", "bRepeat::WriteToLocation", [] { hp2::recovered::bRepeat instance; instance.WriteToLocation(); });
    failures += !expect_gap("0x001EB0C0", "bRepeat::ReadFromLocation", [] { hp2::recovered::bRepeat instance; instance.ReadFromLocation(); });
    failures += !expect_gap("0x001EB498", "bRepeat::PrepareToRemoveData", [] { hp2::recovered::bRepeat instance; instance.PrepareToRemoveData(); });
    failures += !expect_gap("0x00273280", "bSample::Initialize", [] { hp2::recovered::bSample instance; instance.Initialize(); });
    failures += !expect_gap("0x002735A8", "bSample::GetName", [] { hp2::recovered::bSample instance; instance.GetName(); });
    failures += !expect_gap("0x002735E0", "bSample::GetBankName", [] { hp2::recovered::bSample instance; instance.GetBankName(); });
    failures += !expect_gap("0x00273618", "bSample::SetCategory", [] { hp2::recovered::bSample instance; instance.SetCategory(); });
    failures += !expect_gap("0x00275B70", "bSample::GetType", [] { hp2::recovered::bSample instance; instance.GetType(); });
    failures += !expect_gap("0x00275BA8", "bSample::IsVoiceLockedPlaying", [] { hp2::recovered::bSample instance; instance.IsVoiceLockedPlaying(); });
    failures += !expect_gap("0x00275BE0", "bSample::IsVoiceLocked", [] { hp2::recovered::bSample instance; instance.IsVoiceLocked(); });
    failures += !expect_gap("0x00275C18", "bSample::ReleaseVoice", [] { hp2::recovered::bSample instance; instance.ReleaseVoice(); });
    failures += !expect_gap("0x00275D40", "bSample::GetFlags", [] { hp2::recovered::bSample instance; instance.GetFlags(); });
    failures += !expect_gap("0x00275D70", "bSample::GetPitch", [] { hp2::recovered::bSample instance; instance.GetPitch(); });
    failures += !expect_gap("0x00275DA8", "bSample::GetPan", [] { hp2::recovered::bSample instance; instance.GetPan(); });
    failures += !expect_gap("0x00275DE0", "bSample::GetSampleVolume", [] { hp2::recovered::bSample instance; instance.GetSampleVolume(); });
    failures += !expect_gap("0x00275E50", "bSample::GetCallback", [] { hp2::recovered::bSample instance; instance.GetCallback(); });
    failures += !expect_gap("0x00275E88", "bSample::SetCallback", [] { hp2::recovered::bSample instance; instance.SetCallback(); });
    failures += !expect_gap("0x00275EB8", "bSample::GetCategory", [] { hp2::recovered::bSample instance; instance.GetCategory(); });
    failures += !expect_gap("0x00275EF0", "bSample::IsPaused", [] { hp2::recovered::bSample instance; instance.IsPaused(); });
    failures += !expect_gap("0x00280340", "bSampleStream::SetSampleRate", [] { hp2::recovered::bSampleStream instance; instance.SetSampleRate(); });
    failures += !expect_gap("0x00276500", "bSegment::GetOffset", [] { hp2::recovered::bSegment instance; instance.GetOffset(); });
    failures += !expect_gap("0x00276538", "bSegment::Sub_00276538", [] { hp2::recovered::bSegment instance; instance.Sub_00276538(); });
    failures += !expect_gap("0x00276570", "bSegment::Sub_00276570", [] { hp2::recovered::bSegment instance; instance.Sub_00276570(); });
    failures += !expect_gap("0x002765B0", "bSegment::GetDataSize", [] { hp2::recovered::bSegment instance; instance.GetDataSize(); });
    failures += !expect_gap("0x002765E8", "bSegment::GetHeaderSize", [] { hp2::recovered::bSegment instance; instance.GetHeaderSize(); });
    failures += !expect_gap("0x00277DC0", "bSound::GetCategoryVolume", [] { hp2::recovered::bSound instance; instance.GetCategoryVolume(); });
    failures += !expect_gap("0x00277EA0", "bSound::GetStatus", [] { hp2::recovered::bSound instance; instance.GetStatus(); });
    failures += !expect_gap("0x00278320", "bSound::EnablePrintf", [] { hp2::recovered::bSound instance; instance.EnablePrintf(); });
    failures += !expect_gap("0x00278368", "bSound::DisablePrintf", [] { hp2::recovered::bSound instance; instance.DisablePrintf(); });
    failures += !expect_gap("0x0027A6D8", "bSound::getPendingRegion", [] { hp2::recovered::bSound instance; instance.getPendingRegion(); });
    failures += !expect_gap("0x0027A720", "bSound::getPendingRegionsCount", [] { hp2::recovered::bSound instance; instance.getPendingRegionsCount(); });
    failures += !expect_gap("0x00280298", "bSound::GetPath", [] { hp2::recovered::bSound instance; instance.GetPath(); });
    failures += !expect_gap("0x0027C5C0", "bVoice::Release", [] { hp2::recovered::bVoice instance; instance.Release(); });
    failures += !expect_gap("0x0027C5E8", "bVoice::Set", [] { hp2::recovered::bVoice instance; instance.Set(); });
    failures += !expect_gap("0x0027C6F8", "bVoice::Sub_0027C6F8", [] { hp2::recovered::bVoice instance; instance.Sub_0027C6F8(); });
    failures += !expect_gap("0x0012C938", "cDBFileSys::Service", [] { hp2::recovered::cDBFileSys instance; instance.Service(); });
    failures += !expect_gap("0x0012CCA8", "cDBFileSys::SetState", [] { hp2::recovered::cDBFileSys instance; instance.SetState(); });
    failures += !expect_gap("0x0012CD00", "cDBFileSys::Format", [] { hp2::recovered::cDBFileSys instance; instance.Format(); });
    failures += !expect_gap("0x0012CD78", "cDBFileSys::Unformat", [] { hp2::recovered::cDBFileSys instance; instance.Unformat(); });
    failures += !expect_gap("0x0012D1A0", "cDBFileSys::Sub_0012D1A0", [] { hp2::recovered::cDBFileSys instance; instance.Sub_0012D1A0(); });
    failures += !expect_gap("0x0012D1E0", "cDBFileSys::Sub_0012D1E0", [] { hp2::recovered::cDBFileSys instance; instance.Sub_0012D1E0(); });
    failures += !expect_gap("0x0012D220", "cDBFileSys::CheckFreeSpace", [] { hp2::recovered::cDBFileSys instance; instance.CheckFreeSpace(); });
    failures += !expect_gap("0x0012D2A8", "cDBFileSys::GetFreeSpaceOnDevice", [] { hp2::recovered::cDBFileSys instance; instance.GetFreeSpaceOnDevice(); });
    failures += !expect_gap("0x0012D2E8", "cDBFileSys::GetFileListing", [] { hp2::recovered::cDBFileSys instance; instance.GetFileListing(); });
    failures += !expect_gap("0x0012D3D8", "cDBFileSys::CheckDevice", [] { hp2::recovered::cDBFileSys instance; instance.CheckDevice(); });
    failures += !expect_gap("0x0012D518", "cDBFileSys::IsDevicePresent", [] { hp2::recovered::cDBFileSys instance; instance.IsDevicePresent(); });
    failures += !expect_gap("0x0012D560", "cDBFileSys::GetCurrentDevice", [] { hp2::recovered::cDBFileSys instance; instance.GetCurrentDevice(); });
    failures += !expect_gap("0x0012D5F8", "cDBFileSys::GetNextDevice", [] { hp2::recovered::cDBFileSys instance; instance.GetNextDevice(); });
    failures += !expect_gap("0x0012D690", "cDBFileSys::GetPrevDevice", [] { hp2::recovered::cDBFileSys instance; instance.GetPrevDevice(); });
    failures += !expect_gap("0x0012DDC0", "cDBFileSys::SetLastError", [] { hp2::recovered::cDBFileSys instance; instance.SetLastError(); });
    failures += !expect_gap("0x0012DEE0", "cDBFileSys::Sub_0012DEE0", [] { hp2::recovered::cDBFileSys instance; instance.Sub_0012DEE0(); });
    failures += !expect_gap("0x0012E0F0", "cDBFileSys::Sub_0012E0F0", [] { hp2::recovered::cDBFileSys instance; instance.Sub_0012E0F0(); });
    failures += !expect_gap("0x0012E190", "cDBFileSys::GetFileName", [] { hp2::recovered::cDBFileSys instance; instance.GetFileName(); });
    failures += !expect_gap("0x0012E1A8", "cDBFileSys::GetFileCount", [] { hp2::recovered::cDBFileSys instance; instance.GetFileCount(); });
    failures += !expect_gap("0x0012E1B8", "cDBFilesBase::SetLastError", [] { hp2::recovered::cDBFilesBase instance; instance.SetLastError(); });
    failures += !expect_gap("0x0012E1C0", "cDBFilesBase::GetState", [] { hp2::recovered::cDBFilesBase instance; instance.GetState(); });
    failures += !expect_gap("0x0012E1C8", "cDBFilesBase::SetState", [] { hp2::recovered::cDBFilesBase instance; instance.SetState(); });
    failures += !expect_gap("0x0012E1F0", "cDBFilesBase::GetFreeSpaceOnDevice", [] { hp2::recovered::cDBFilesBase instance; instance.GetFreeSpaceOnDevice(); });
    failures += !expect_gap("0x0012E1F8", "cDBFilesBase::GetMaxDeviceSupported", [] { hp2::recovered::cDBFilesBase instance; instance.GetMaxDeviceSupported(); });
    failures += !expect_gap("0x0012E200", "cDBFilesBase::GetPrevDevice", [] { hp2::recovered::cDBFilesBase instance; instance.GetPrevDevice(); });
    failures += !expect_gap("0x0012E208", "cDBFilesBase::GetNextDevice", [] { hp2::recovered::cDBFilesBase instance; instance.GetNextDevice(); });
    failures += !expect_gap("0x0012E210", "cDBFilesBase::GetCurrentDevice", [] { hp2::recovered::cDBFilesBase instance; instance.GetCurrentDevice(); });
    failures += !expect_gap("0x0012E218", "cDBFilesBase::IsDevicePresent", [] { hp2::recovered::cDBFilesBase instance; instance.IsDevicePresent(); });
    failures += !expect_gap("0x0012E220", "cDBFilesBase::CheckDevice", [] { hp2::recovered::cDBFilesBase instance; instance.CheckDevice(); });
    failures += !expect_gap("0x0012E228", "cDBFilesBase::GetFileName", [] { hp2::recovered::cDBFilesBase instance; instance.GetFileName(); });
    failures += !expect_gap("0x0012E230", "cDBFilesBase::Sub_0012E230", [] { hp2::recovered::cDBFilesBase instance; instance.Sub_0012E230(); });
    failures += !expect_gap("0x0012E238", "cDBFilesBase::GetFileCount", [] { hp2::recovered::cDBFilesBase instance; instance.GetFileCount(); });
    failures += !expect_gap("0x0012E240", "cDBFilesBase::GetFileListing", [] { hp2::recovered::cDBFilesBase instance; instance.GetFileListing(); });
    failures += !expect_gap("0x0012E248", "cDBFilesBase::GetDeviceName", [] { hp2::recovered::cDBFilesBase instance; instance.GetDeviceName(); });
    failures += !expect_gap("0x0012E250", "cDBFilesBase::CheckFreeSpace", [] { hp2::recovered::cDBFilesBase instance; instance.CheckFreeSpace(); });
    failures += !expect_gap("0x0012E258", "cDBFilesBase::CalculateFreeSpaceRequired", [] { hp2::recovered::cDBFilesBase instance; instance.CalculateFreeSpaceRequired(); });
    failures += !expect_gap("0x0012E260", "cDBFilesBase::Sub_0012E260", [] { hp2::recovered::cDBFilesBase instance; instance.Sub_0012E260(); });
    failures += !expect_gap("0x0012E268", "cDBFilesBase::SetFileIndex", [] { hp2::recovered::cDBFilesBase instance; instance.SetFileIndex(); });
    failures += !expect_gap("0x0012E270", "cDBFilesBase::SetFileData", [] { hp2::recovered::cDBFilesBase instance; instance.SetFileData(); });
    failures += !expect_gap("0x0012E2B8", "cDBFilesBase::Unformat", [] { hp2::recovered::cDBFilesBase instance; instance.Unformat(); });
    failures += !expect_gap("0x0012E2C0", "cDBFilesBase::Format", [] { hp2::recovered::cDBFilesBase instance; instance.Format(); });
    failures += !expect_gap("0x0012E2C8", "cDBFilesBase::Service", [] { hp2::recovered::cDBFilesBase instance; instance.Service(); });
    failures += !expect_gap("0x0012E2D0", "cDBFilesBase::Sub_0012E2D0", [] { hp2::recovered::cDBFilesBase instance; instance.Sub_0012E2D0(); });
    failures += !expect_gap("0x002800E0", "cJukeboxPlayer::IsDonePlaying", [] { hp2::recovered::cJukeboxPlayer instance; instance.IsDonePlaying(); });
    failures += !expect_gap("0x00280130", "cJukeboxPlayer::IsDoneFading", [] { hp2::recovered::cJukeboxPlayer instance; instance.IsDoneFading(); });
    failures += !expect_gap("0x00280260", "cJukeboxPlayer::GetSampleRate", [] { hp2::recovered::cJukeboxPlayer instance; instance.GetSampleRate(); });
    failures += !expect_gap("0x0012AE28", "cMC::MCCloseResult", [] { hp2::recovered::cMC instance; instance.MCCloseResult(); });
    failures += !expect_gap("0x0012B0F0", "cMC::MCMakeDirResult", [] { hp2::recovered::cMC instance; instance.MCMakeDirResult(); });
    failures += !expect_gap("0x001EE1F8", "cPlayerSettings::ToggleGauges", [] { hp2::recovered::cPlayerSettings instance; instance.ToggleGauges(); });
    failures += !expect_gap("0x001EE230", "cPlayerSettings::ToggleLapInfo", [] { hp2::recovered::cPlayerSettings instance; instance.ToggleLapInfo(); });
    failures += !expect_gap("0x001EE258", "cPlayerSettings::CycleSplitTime", [] { hp2::recovered::cPlayerSettings instance; instance.CycleSplitTime(); });
    failures += !expect_gap("0x001EE2B0", "cPlayerSettings::ToggleRacePosition", [] { hp2::recovered::cPlayerSettings instance; instance.ToggleRacePosition(); });
    failures += !expect_gap("0x001EE2D8", "cPlayerSettings::ToggleLookback", [] { hp2::recovered::cPlayerSettings instance; instance.ToggleLookback(); });
    failures += !expect_gap("0x001EE300", "cPlayerSettings::CycleAlternateCamera", [] { hp2::recovered::cPlayerSettings instance; instance.CycleAlternateCamera(); });
    failures += !expect_gap("0x001DB3D0", "cPoint::PointVelocity", [] { hp2::recovered::cPoint instance; instance.PointVelocity(); });
    failures += !expect_gap("0x001DB810", "cPoint::SetPosX", [] { hp2::recovered::cPoint instance; instance.SetPosX(); });
    failures += !expect_gap("0x001DB870", "cPoint::SetPosZ", [] { hp2::recovered::cPoint instance; instance.SetPosZ(); });
    failures += !expect_gap("0x001DB940", "cPoint::SetPosDesiredX", [] { hp2::recovered::cPoint instance; instance.SetPosDesiredX(); });
    failures += !expect_gap("0x001DB9A0", "cPoint::SetPosDesiredZ", [] { hp2::recovered::cPoint instance; instance.SetPosDesiredZ(); });
    failures += !expect_gap("0x001DBAB8", "cPoint::SetdPosZ", [] { hp2::recovered::cPoint instance; instance.SetdPosZ(); });
    failures += !expect_gap("0x001DBC28", "cPoint::SetRotX", [] { hp2::recovered::cPoint instance; instance.SetRotX(); });
    failures += !expect_gap("0x001DBC50", "cPoint::SetRotY", [] { hp2::recovered::cPoint instance; instance.SetRotY(); });
    failures += !expect_gap("0x001DBC78", "cPoint::SetRotZ", [] { hp2::recovered::cPoint instance; instance.SetRotZ(); });
    failures += !expect_gap("0x001DBD38", "cPoint::SetRotDesiredX", [] { hp2::recovered::cPoint instance; instance.SetRotDesiredX(); });
    failures += !expect_gap("0x001DBD60", "cPoint::SetRotDesiredY", [] { hp2::recovered::cPoint instance; instance.SetRotDesiredY(); });
    failures += !expect_gap("0x001DBD88", "cPoint::SetRotDesiredZ", [] { hp2::recovered::cPoint instance; instance.SetRotDesiredZ(); });
    failures += !expect_gap("0x001DBE48", "cPoint::SetdRotX", [] { hp2::recovered::cPoint instance; instance.SetdRotX(); });
    failures += !expect_gap("0x001DBE70", "cPoint::SetdRotY", [] { hp2::recovered::cPoint instance; instance.SetdRotY(); });
    failures += !expect_gap("0x001DBE98", "cPoint::SetdRotZ", [] { hp2::recovered::cPoint instance; instance.SetdRotZ(); });
    failures += !expect_gap("0x001DBF70", "cPoint::SetScale", [] { hp2::recovered::cPoint instance; instance.SetScale(); });
    failures += !expect_gap("0x001DBF98", "cPoint::SetdScale", [] { hp2::recovered::cPoint instance; instance.SetdScale(); });
    failures += !expect_gap("0x001DBFC0", "cPoint::SetYScale", [] { hp2::recovered::cPoint instance; instance.SetYScale(); });
    failures += !expect_gap("0x001DBFE8", "cPoint::SetdYScale", [] { hp2::recovered::cPoint instance; instance.SetdYScale(); });
    failures += !expect_gap("0x001DC010", "cPoint::SetScaleDesired", [] { hp2::recovered::cPoint instance; instance.SetScaleDesired(); });
    failures += !expect_gap("0x001DC040", "cPoint::SetYScaleDesired", [] { hp2::recovered::cPoint instance; instance.SetYScaleDesired(); });
    failures += !expect_gap("0x001DC070", "cPoint::SetDuration", [] { hp2::recovered::cPoint instance; instance.SetDuration(); });
    failures += !expect_gap("0x002059D0", "cSEDWedge::S_DebugPrint", [] { hp2::recovered::cSEDWedge instance; instance.S_DebugPrint(); });
    failures += !expect_gap("0x001A78E0", "eAnimationSolid::GetTranslation", [] { hp2::recovered::eAnimationSolid instance; instance.GetTranslation(); });
    failures += !expect_gap("0x001047E0", "eDmaChain::Init", [] { hp2::recovered::eDmaChain instance; instance.Init(); });
    failures += !expect_gap("0x0010C5B0", "eModel::UnInit", [] { hp2::recovered::eModel instance; instance.UnInit(); });
    failures += !expect_gap("0x0010C878", "eModel::ApplyReplacementTextureTable", [] { hp2::recovered::eModel instance; instance.ApplyReplacementTextureTable(); });
    failures += !expect_gap("0x0010CB00", "eModel::IsLit", [] { hp2::recovered::eModel instance; instance.IsLit(); });
    failures += !expect_gap("0x001A74C8", "eRigidAnimation::GetMatrixAtFrame", [] { hp2::recovered::eRigidAnimation instance; instance.GetMatrixAtFrame(); });
    failures += !expect_gap("0x001A7518", "eRigidAnimation::GetMatrixAtTime", [] { hp2::recovered::eRigidAnimation instance; instance.GetMatrixAtTime(); });
    failures += !expect_gap("0x00103FF0", "eSolid::FixStripEntryTable", [] { hp2::recovered::eSolid instance; instance.FixStripEntryTable(); });
    failures += !expect_gap("0x00104070", "eSolid::SetEnvironmentMapping", [] { hp2::recovered::eSolid instance; instance.SetEnvironmentMapping(); });
    failures += !expect_gap("0x0010BD38", "eSolid::GetBoundingBox", [] { hp2::recovered::eSolid instance; instance.GetBoundingBox(); });
    failures += !expect_gap("0x0010C158", "eSolid::FixStripSTs", [] { hp2::recovered::eSolid instance; instance.FixStripSTs(); });
    failures += !expect_gap("0x0010C4B0", "eSolid::ReplaceLightMaterial", [] { hp2::recovered::eSolid instance; instance.ReplaceLightMaterial(); });
    failures += !expect_gap("0x00114BB0", "eSolid::Sub_00114BB0", [] { hp2::recovered::eSolid instance; instance.Sub_00114BB0(); });
    failures += !expect_gap("0x0011C248", "eStripData::GetNumVerts", [] { hp2::recovered::eStripData instance; instance.GetNumVerts(); });
    failures += !expect_gap("0x0011C268", "eStripData::GetST0", [] { hp2::recovered::eStripData instance; instance.GetST0(); });
    failures += !expect_gap("0x0011C2B8", "eStripData::GetXYZ", [] { hp2::recovered::eStripData instance; instance.GetXYZ(); });
    failures += !expect_gap("0x0011BF98", "eStripDataE::GetElementXYZ", [] { hp2::recovered::eStripDataE instance; instance.GetElementXYZ(); });
    failures += !expect_gap("0x0011C090", "eStripDataE::GetElementST0", [] { hp2::recovered::eStripDataE instance; instance.GetElementST0(); });
    failures += !expect_gap("0x0011C180", "eStripDataE::GetElementRGBA", [] { hp2::recovered::eStripDataE instance; instance.GetElementRGBA(); });
    failures += !expect_gap("0x00109260", "eTextureDownloader::Init", [] { hp2::recovered::eTextureDownloader instance; instance.Init(); });
    failures += !expect_gap("0x00110C60", "eTransferRectQueue::Sub_00110C60", [] { hp2::recovered::eTransferRectQueue instance; instance.Sub_00110C60(); });
    failures += !expect_gap("0x00110CD8", "eTransferRectQueue::GetTransferRect", [] { hp2::recovered::eTransferRectQueue instance; instance.GetTransferRect(); });
    failures += !expect_gap("0x001075B8", "eView::Sub_001075B8", [] { hp2::recovered::eView instance; instance.Sub_001075B8(); });
    failures += !expect_gap("0x00107600", "eView::Sub_00107600", [] { hp2::recovered::eView instance; instance.Sub_00107600(); });
    failures += !expect_gap("0x001076F8", "eView::Sub_001076F8", [] { hp2::recovered::eView instance; instance.Sub_001076F8(); });
    failures += !expect_gap("0x0020BFB0", "fsm::handleEvent", [] { hp2::recovered::fsm instance; instance.handleEvent(); });
    failures += !expect_gap("0x002124F0", "kAITireSound::Sub_002124F0", [] { hp2::recovered::kAITireSound instance; instance.Sub_002124F0(); });
    failures += !expect_gap("0x00214890", "kAITireSound::ActivateSound", [] { hp2::recovered::kAITireSound instance; instance.ActivateSound(); });
    failures += !expect_gap("0x00214898", "kAITireSound::DeactivateSound", [] { hp2::recovered::kAITireSound instance; instance.DeactivateSound(); });
    failures += !expect_gap("0x002148E0", "kAITireSound::Mute", [] { hp2::recovered::kAITireSound instance; instance.Mute(); });
    failures += !expect_gap("0x00214928", "kAITireSound::Update", [] { hp2::recovered::kAITireSound instance; instance.Update(); });
    failures += !expect_gap("0x002149A0", "kAITireSound::Sub_002149A0", [] { hp2::recovered::kAITireSound instance; instance.Sub_002149A0(); });
    failures += !expect_gap("0x002A0550", "kAITireSound::Sub_002A0550", [] { hp2::recovered::kAITireSound instance; instance.Sub_002A0550(); });
    failures += !expect_gap("0x0020D8C0", "nfsFsm::Sub_0020D8C0", [] { hp2::recovered::nfsFsm instance; instance.Sub_0020D8C0(); });
    failures += !expect_gap("0x0020E438", "rotorChatterAction_t::executeAction", [] { hp2::recovered::rotorChatterAction_t instance; instance.executeAction(); });
    failures += !expect_gap("0x0020E3E8", "rotorLocationAction_t::executeAction", [] { hp2::recovered::rotorLocationAction_t instance; instance.executeAction(); });
    failures += !expect_gap("0x0020E3C8", "rotorSpikeFailedAction_t::executeAction", [] { hp2::recovered::rotorSpikeFailedAction_t instance; instance.executeAction(); });
    failures += !expect_gap("0x0020E410", "rotorTrafficAction_t::executeAction", [] { hp2::recovered::rotorTrafficAction_t instance; instance.executeAction(); });
    failures += !expect_gap("0x0022ED68", "tAICop::CanPatrollerSeeThisCar", [] { hp2::recovered::tAICop instance; instance.CanPatrollerSeeThisCar(); });
    failures += !expect_gap("0x00230BF8", "tAICop::IsNearPursuitTarget", [] { hp2::recovered::tAICop instance; instance.IsNearPursuitTarget(); });
    failures += !expect_gap("0x002491B0", "tAnimatedBlurb::Sub_002491B0", [] { hp2::recovered::tAnimatedBlurb instance; instance.Sub_002491B0(); });
    failures += !expect_gap("0x001F8C10", "tCarLogoAnimation::Sub_001F8C10", [] { hp2::recovered::tCarLogoAnimation instance; instance.Sub_001F8C10(); });
    failures += !expect_gap("0x002244A0", "tCopManager::GetNextPursuitID", [] { hp2::recovered::tCopManager instance; instance.GetNextPursuitID(); });
    failures += !expect_gap("0x00224F70", "tCopManager::GetCopByCar", [] { hp2::recovered::tCopManager instance; instance.GetCopByCar(); });
    failures += !expect_gap("0x002254E8", "tCopManager::GetCurrentCopSpeaker", [] { hp2::recovered::tCopManager instance; instance.GetCurrentCopSpeaker(); });
    failures += !expect_gap("0x002255B8", "tCopManager::ReserveCopSpeaker", [] { hp2::recovered::tCopManager instance; instance.ReserveCopSpeaker(); });
    failures += !expect_gap("0x00225610", "tCopManager::ReleaseCopSpeaker", [] { hp2::recovered::tCopManager instance; instance.ReleaseCopSpeaker(); });
    failures += !expect_gap("0x00225C40", "tCopManager::GetArrestingOfficer", [] { hp2::recovered::tCopManager instance; instance.GetArrestingOfficer(); });
    failures += !expect_gap("0x00226238", "tCopManager::GetActiveRoadBlock", [] { hp2::recovered::tCopManager instance; instance.GetActiveRoadBlock(); });
    failures += !expect_gap("0x00226448", "tCopManager::GetRoadBlockFromLoadedDataIndex", [] { hp2::recovered::tCopManager instance; instance.GetRoadBlockFromLoadedDataIndex(); });
    failures += !expect_gap("0x00226968", "tCopManager::GetNumActiveRoadBlocks", [] { hp2::recovered::tCopManager instance; instance.GetNumActiveRoadBlocks(); });
    failures += !expect_gap("0x00226BB0", "tCopManager::GetSpeedTrapFromLoadedDataIndex", [] { hp2::recovered::tCopManager instance; instance.GetSpeedTrapFromLoadedDataIndex(); });
    failures += !expect_gap("0x00227438", "tCopManager::IsBeingPursued", [] { hp2::recovered::tCopManager instance; instance.IsBeingPursued(); });
    failures += !expect_gap("0x002289F8", "tCopManager::ShouldDisplayBustedText", [] { hp2::recovered::tCopManager instance; instance.ShouldDisplayBustedText(); });
    failures += !expect_gap("0x001DA780", "tCubic1D::MakeCoeffs", [] { hp2::recovered::tCubic1D instance; instance.MakeCoeffs(); });
    failures += !expect_gap("0x001DA7D0", "tCubic1D::GetVal", [] { hp2::recovered::tCubic1D instance; instance.GetVal(); });
    failures += !expect_gap("0x001DA800", "tCubic1D::GetdVal", [] { hp2::recovered::tCubic1D instance; instance.GetdVal(); });
    failures += !expect_gap("0x001DA830", "tCubic1D::GetddVal", [] { hp2::recovered::tCubic1D instance; instance.GetddVal(); });
    failures += !expect_gap("0x001DAA00", "tCubic2D::SetVal", [] { hp2::recovered::tCubic2D instance; instance.SetVal(); });
    failures += !expect_gap("0x001DAA50", "tCubic2D::SetdVal", [] { hp2::recovered::tCubic2D instance; instance.SetdVal(); });
    failures += !expect_gap("0x001DAAA0", "tCubic2D::SetValDesired", [] { hp2::recovered::tCubic2D instance; instance.SetValDesired(); });
    failures += !expect_gap("0x001DABF8", "tCubic3D::SetVal", [] { hp2::recovered::tCubic3D instance; instance.SetVal(); });
    failures += !expect_gap("0x001DAC68", "tCubic3D::SetdVal", [] { hp2::recovered::tCubic3D instance; instance.SetdVal(); });
    failures += !expect_gap("0x001DACD8", "tCubic3D::SetValDesired", [] { hp2::recovered::tCubic3D instance; instance.SetValDesired(); });
    failures += !expect_gap("0x001F3F20", "tCustomizeHUDScreen::Sub_001F3F20", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F3F20(); });
    failures += !expect_gap("0x001F4178", "tCustomizeHUDScreen::Sub_001F4178", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.Sub_001F4178(); });
    failures += !expect_gap("0x001F46D8", "tCustomizeHUDScreen::HandleEvent", [] { hp2::recovered::tCustomizeHUDScreen instance; instance.HandleEvent(); });
    failures += !expect_gap("0x001F33B8", "tDisplayOptionsScreen::Sub_001F33B8", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F33B8(); });
    failures += !expect_gap("0x001F3588", "tDisplayOptionsScreen::Sub_001F3588", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F3588(); });
    failures += !expect_gap("0x001F35E0", "tDisplayOptionsScreen::Sub_001F35E0", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F35E0(); });
    failures += !expect_gap("0x001F3A28", "tDisplayOptionsScreen::Sub_001F3A28", [] { hp2::frontend::tDisplayOptionsScreen instance; instance.Sub_001F3A28(); });
    failures += !expect_gap("0x00155948", "tEnvelope::GetIndex", [] { hp2::recovered::tEnvelope instance; instance.GetIndex(); });
    failures += !expect_gap("0x001559A8", "tEnvelope::OutOfRange", [] { hp2::recovered::tEnvelope instance; instance.OutOfRange(); });
    failures += !expect_gap("0x001559F0", "tEnvelope::GetSlope", [] { hp2::recovered::tEnvelope instance; instance.GetSlope(); });
    failures += !expect_gap("0x001F7FE8", "tEventGroup::IsMember", [] { hp2::frontend::tEventGroup instance; instance.IsMember(); });
    failures += !expect_gap("0x001F6FA0", "tEventLeaf::Sub_001F6FA0", [] { hp2::frontend::tEventLeaf instance; instance.Sub_001F6FA0(); });
    failures += !expect_gap("0x001F7FA8", "tEventLeaf::CalculateGeneration", [] { hp2::frontend::tEventLeaf instance; instance.CalculateGeneration(); });
    failures += !expect_gap("0x001FA840", "tEventTreeView::FindLeafByEventNumber", [] { hp2::frontend::tEventTreeView instance; instance.FindLeafByEventNumber(); });
    failures += !expect_gap("0x002490D8", "tExitScreenAnimation::Sub_002490D8", [] { hp2::recovered::tExitScreenAnimation instance; instance.Sub_002490D8(); });
    failures += !expect_gap("0x002495F0", "tGlobalBackgroundAnimation::Sub_002495F0", [] { hp2::recovered::tGlobalBackgroundAnimation instance; instance.Sub_002495F0(); });
    failures += !expect_gap("0x00249790", "tGlobalBackgroundAnimation::RemoveTextureHash", [] { hp2::recovered::tGlobalBackgroundAnimation instance; instance.RemoveTextureHash(); });
    failures += !expect_gap("0x002497B8", "tGlobalBackgroundAnimation::GetNumTextures", [] { hp2::recovered::tGlobalBackgroundAnimation instance; instance.GetNumTextures(); });
    failures += !expect_gap("0x002497F0", "tGlobalBackgroundAnimation::GetNthTexture", [] { hp2::recovered::tGlobalBackgroundAnimation instance; instance.GetNthTexture(); });
    failures += !expect_gap("0x001EE120", "tGlobalDisplaySettings::CycleLeaderboardMode", [] { hp2::recovered::tGlobalDisplaySettings instance; instance.CycleLeaderboardMode(); });
    failures += !expect_gap("0x001E4170", "tHelpMenu::IsActive", [] { hp2::recovered::tHelpMenu instance; instance.IsActive(); });
    failures += !expect_gap("0x0024D048", "tKeyboard::PlayerRight", [] { hp2::recovered::tKeyboard instance; instance.PlayerRight(); });
    failures += !expect_gap("0x0024A690", "tLeftRightArrowScroll::Sub_0024A690", [] { hp2::recovered::tLeftRightArrowScroll instance; instance.Sub_0024A690(); });
    failures += !expect_gap("0x00249C78", "tMenuAnimElement::Sub_00249C78", [] { hp2::recovered::tMenuAnimElement instance; instance.Sub_00249C78(); });
    failures += !expect_gap("0x00249D70", "tMenuAnimElement::ApplyDeltaY", [] { hp2::recovered::tMenuAnimElement instance; instance.ApplyDeltaY(); });
    failures += !expect_gap("0x0022D080", "tNearFollowTarget::GetVelocity", [] { hp2::recovered::tNearFollowTarget instance; instance.GetVelocity(); });
    failures += !expect_gap("0x001EEBB8", "tOptionsScreen::Sub_001EEBB8", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EEBB8(); });
    failures += !expect_gap("0x001EEC50", "tOptionsScreen::Sub_001EEC50", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EEC50(); });
    failures += !expect_gap("0x001EECA8", "tOptionsScreen::Sub_001EECA8", [] { hp2::frontend::tOptionsScreen instance; instance.Sub_001EECA8(); });
    failures += !expect_gap("0x001F8AD8", "tParallaxer::Sub_001F8AD8", [] { hp2::recovered::tParallaxer instance; instance.Sub_001F8AD8(); });
    failures += !expect_gap("0x0016B5A8", "tPopupMenu::GetCurrentMenuItem", [] { hp2::recovered::tPopupMenu instance; instance.GetCurrentMenuItem(); });
    failures += !expect_gap("0x00229ED0", "tPursuit::SelectPrimaryCop", [] { hp2::recovered::tPursuit instance; instance.SelectPrimaryCop(); });
    failures += !expect_gap("0x0022BB28", "tPursuit::IsCopNearChasedCar", [] { hp2::recovered::tPursuit instance; instance.IsCopNearChasedCar(); });
    failures += !expect_gap("0x0022BC08", "tPursuit::IsRacingWithMe", [] { hp2::recovered::tPursuit instance; instance.IsRacingWithMe(); });
    failures += !expect_gap("0x0022C920", "tPursuit::HasThisCop", [] { hp2::recovered::tPursuit instance; instance.HasThisCop(); });
    failures += !expect_gap("0x00249DE0", "tScrollingMenuList::Sub_00249DE0", [] { hp2::recovered::tScrollingMenuList instance; instance.Sub_00249DE0(); });
    failures += !expect_gap("0x0024A050", "tScrollingMenuList::FindMenuElement", [] { hp2::recovered::tScrollingMenuList instance; instance.FindMenuElement(); });
    failures += !expect_gap("0x0024A248", "tScrollingMenuList::Highlight", [] { hp2::recovered::tScrollingMenuList instance; instance.Highlight(); });
    failures += !expect_gap("0x0024A3E0", "tScrollingMenuList::ShowUpArrow", [] { hp2::recovered::tScrollingMenuList instance; instance.ShowUpArrow(); });
    failures += !expect_gap("0x0024A430", "tScrollingMenuList::ShowDownArrow", [] { hp2::recovered::tScrollingMenuList instance; instance.ShowDownArrow(); });
    failures += !expect_gap("0x00248BE8", "tSlideAnimation::Sub_00248BE8", [] { hp2::recovered::tSlideAnimation instance; instance.Sub_00248BE8(); });
    failures += !expect_gap("0x00241960", "tSpikeStrip::GetRenderOrigin", [] { hp2::recovered::tSpikeStrip instance; instance.GetRenderOrigin(); });
    failures += !expect_gap("0x001E2848", "tStartRaceScreenManager::AddScreen", [] { hp2::recovered::tStartRaceScreenManager instance; instance.AddScreen(); });
    failures += !expect_gap("0x0024E4D0", "tTrackGraph::GetVertexNumber", [] { hp2::recovered::tTrackGraph instance; instance.GetVertexNumber(); });
    failures += !expect_gap("0x001F90F0", "tTrackLogoAnimation::Sub_001F90F0", [] { hp2::recovered::tTrackLogoAnimation instance; instance.Sub_001F90F0(); });
    failures += !expect_gap("0x001F91D0", "tTrackLogoAnimation::Show", [] { hp2::recovered::tTrackLogoAnimation instance; instance.Show(); });
    failures += !expect_gap("0x001F65C8", "tTreeReward::HandleMessages", [] { hp2::recovered::tTreeReward instance; instance.HandleMessages(); });
    failures += !expect_gap("0x001F72B0", "tUnlockMap::AddGoal", [] { hp2::recovered::tUnlockMap instance; instance.AddGoal(); });
    failures += !expect_gap("0x002500B0", "tWordQueue::Store", [] { hp2::recovered::tWordQueue instance; instance.Store(); });
    failures += !expect_gap("0x00250100", "tWordQueue::Remove", [] { hp2::recovered::tWordQueue instance; instance.Remove(); });
    failures += !expect_gap("0x001B9CD0", "vAABB::Contains", [] { hp2::recovered::vAABB instance; instance.Contains(); });
    failures += !expect_gap("0x001B9D40", "vAABB::Encloses", [] { hp2::recovered::vAABB instance; instance.Encloses(); });
    std::cout << "CLASS_TEMPLATE_GAPS_CHECKED: 1064\n";
    return failures ? 1 : 0;
}

#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/TrackMap.hpp"
#include "indep/inc/Player.hpp"
#include "CarSounds.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bware/inc/bMath.hpp"
#include "bsound/include/bSample.h"
#include "indep/inc/Pursuit.hpp"
#include "FlailerChunks.hpp"
#include "FlailerDB.hpp"
#include "CarInfo.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/HudAnim.hpp"
#include "Language.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Ecstasy.hpp"
#include <limits.h>
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Scenery.hpp"

#include <string.h>
#include <cstring>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>

int32_t carGlareResetRequested = 0;
int32_t displayPlayerDebugInfo = 0;
float playerSecondLapColor[4] = {0.0f,79.0f,128.0f,128.0f};

CarNisOverlayTuning carNisOverlayTuning = {5.0f, 0.0f, 128.0f, 0.25f};

CarNisOverlay::CarNisOverlay()
    : resources(nullptr), manager(nullptr), screenKey(0), object(nullptr),
      savedScaleX(1.0f), savedScaleY(1.0f), savedTopLeftY(0.0f), mode(0),
      lastDateFrames(0), scale{}, alpha{}, secondText(nullptr),
      startTopLeftX(0.0f), startTopLeftY(0.0f), startScaleX(1.0f),
      startScaleY(1.0f), clock(nullptr), language(nullptr), tuning(nullptr)
{
    tCubic1DInitialize(&scale);
    tCubic1DInitialize(&alpha);
}

/* PAL frees its two owned curve allocations. Inline native ownership releases
 * the same two curves with this object, without a second allocation owner. */
CarNisOverlay::~CarNisOverlay() = default;

// PAL182A48..182BB4; state4/7/5/9 suppression is the same live session
// gate used by the final score/audio path. No copied application state.
CarNisResult carNisOverlaySetVisible(CarNisOverlay* overlay,int32_t visible,
    const RaceParameters* race,const int32_t* sessionState)
{
    if (!overlay) return CAR_NIS_INVALID_BINDING;
    if (!overlay->screenKey) return CAR_NIS_OK;
    if (!race || !sessionState || !overlay->resources) return CAR_NIS_INVALID_BINDING;
    const int32_t state=*sessionState;
    if (state==4 || state==7 || state==5 || state==9) return CAR_NIS_OK;
    auto& resources=*overlay->resources;
    if (!visible) {
        resources.SetGroupVisibility(overlay->screenKey,0x97F9252Au,0);
        return CAR_NIS_OK;
    }
    if (overlay->mode==1 || overlay->mode==2)
        resources.SetGroupVisibility(overlay->screenKey,0x97F9252Au,1);
    else if (overlay->mode==3) {
        resources.SetGroupVisibility(overlay->screenKey,0x97F9252Au,0);
        resources.SetObjectVisibility(overlay->screenKey,0x76494C5Bu,1);
    } else return CAR_NIS_OK;
    resources.SetObjectVisibility(overlay->screenKey,0x233DFBE1u,race->specialPursuit && race->mode==1);
    return CAR_NIS_OK;
}

CarNisResult hp2::BindNisOverlay(CarNisOverlay& overlay,
    FlailerResources& resources, FlailerRenderManager& manager,
    LanguageManager& language, const GameTime& clock,
    const CarNisOverlayTuning& tuning, uint32_t screenKey)
{
    if (screenKey && !resources.FindScreen(screenKey)) return CAR_NIS_INVALID_RECORD;
    overlay.resources = &resources;
    overlay.manager = &manager;
    overlay.language = &language;
    overlay.clock = &clock;
    overlay.tuning = &tuning;
    overlay.screenKey = screenKey;

    /* PAL1829A8..182A48 snapshots these separate Finish-restoration fields.
     * Missing objects preserve the source GetScale defaults1/1 and position0. */
    overlay.savedScaleX = 1.0f;
    overlay.savedScaleY = 1.0f;
    overlay.savedTopLeftY = 0.0f;
    if (auto* text = resources.FindObject(screenKey, 0x76494C5Bu)) {
        overlay.savedScaleX = text->scaleX;
        overlay.savedScaleY = text->scaleY;
        float left = 0.0f;
        resources.GetTopLeft(*text, left, overlay.savedTopLeftY, &language);
    }
    return CAR_NIS_OK;
}

CarNisResult carNisOverlayUpdateDateAndTime(CarNisOverlay* overlay)
{
    if (!overlay || !overlay->resources || !overlay->clock || !overlay->language)
        return CAR_NIS_INVALID_BINDING;

    /* Windows supplies the actual RTC and configured local timezone. The PAL
     * sceCdReadClock/status failure branch also leaves the prior text intact. */
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (now == static_cast<std::time_t>(-1))return CAR_NIS_OK;
#ifdef _WIN32
    if (localtime_s(&local,&now)!=0)return CAR_NIS_OK;
#else
    if (!localtime_r(&now,&local))return CAR_NIS_OK;
#endif

    const int32_t month = local.tm_mon + 1;
    const int32_t monthId = month >= 2 && month <= 12 ? 0xE86B + month : 0xE86C;
    const char* translatedMonth = overlay->language->GetString(monthId);
    if (!translatedMonth) return CAR_NIS_RESOURCE_FAILURE;
    const int32_t year = (local.tm_year + 1900) % 100;
    if (overlay->language->currentLanguage != 0) {
        overlay->resources->Printf(overlay->screenKey, 0x001450DDu,
            "%d %s 20%02d", local.tm_mday, translatedMonth, year);
    } else {
        overlay->resources->Printf(overlay->screenKey, 0x001450DDu,
            "%s %d 20%02d", translatedMonth, local.tm_mday, year);
    }
    overlay->resources->Printf(overlay->screenKey, 0x001D380Eu,
        "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    overlay->lastDateFrames = overlay->clock->realTimeFrames;
    return CAR_NIS_OK;
}

CarNisResult carNisOverlayStart(CarNisOverlay* overlay)
{
    if (!overlay) return CAR_NIS_INVALID_BINDING;
    if (!overlay->screenKey) return CAR_NIS_OK;
    if (!overlay->resources || !overlay->manager || !overlay->clock ||
        !overlay->language || !overlay->tuning)
        return CAR_NIS_INVALID_BINDING;
    const auto& tuning = *overlay->tuning;
    if (!(tuning.durationSeconds > 0.0f) || !std::isfinite(tuning.durationSeconds) ||
        !std::isfinite(tuning.startScale) || !std::isfinite(tuning.startAlpha) ||
        !std::isfinite(tuning.endAlpha))
        return CAR_NIS_INVALID_BINDING;

    overlay->lastDateFrames = 0;
    const auto date = carNisOverlayUpdateDateAndTime(overlay);
    if (date != CAR_NIS_OK) return date;
    overlay->manager->SetScreenVisibility(*overlay->resources, overlay->screenKey, 0);
    overlay->object = overlay->resources->FindObject(overlay->screenKey, 0x76494C5Bu, 1);
    overlay->secondText = static_cast<hp2::FlailerTextChunk*>(
        overlay->resources->FindObject(overlay->screenKey, 0x233DFBE1u, 1));
    overlay->startTopLeftX = 0.0f;
    overlay->startTopLeftY = 0.0f;
    if (overlay->object) {
        overlay->resources->GetTopLeft(*overlay->object, overlay->startTopLeftX,
            overlay->startTopLeftY, overlay->language);
        overlay->startScaleX = overlay->object->scaleX;
        overlay->startScaleY = overlay->object->scaleY;
    }

    tCubic1D& scale = overlay->scale;
    scale.continuous = 0;
    scale.duration = tuning.durationSeconds;
    if (scale.value != tuning.startScale) scale.mode = 2;
    scale.desiredValue = tuning.startScale;
    scale.value = scale.desiredValue;
    scale.derivative = scale.desiredDerivative;
    scale.mode = 0;
    if (scale.value != overlay->startScaleX) scale.mode = 2;
    scale.desiredValue = overlay->startScaleX;

    tCubic1D& alpha = overlay->alpha;
    alpha.continuous = 0;
    alpha.duration = tuning.durationSeconds;
    if (alpha.value != tuning.startAlpha) alpha.mode = 2;
    alpha.desiredValue = tuning.startAlpha;
    alpha.value = alpha.desiredValue;
    alpha.derivative = alpha.desiredDerivative;
    alpha.mode = 0;
    if (alpha.value != tuning.endAlpha) alpha.mode = 2;
    alpha.desiredValue = tuning.endAlpha;
    return CAR_NIS_OK;
}

CarNisResult carNisOverlayUpdate(CarNisOverlay* overlay, int32_t blink)
{
    if (!overlay || !overlay->resources || !overlay->clock || !overlay->language)
        return CAR_NIS_INVALID_BINDING;
    if (overlay->object) {
        if (overlay->object->kind != 1 || !overlay->secondText)
            return CAR_NIS_INVALID_RECORD;
        auto& text = *static_cast<hp2::FlailerTextChunk*>(overlay->object);
        text.SetTopLeftY(overlay->startTopLeftY);
        text.SetScaleY(overlay->startScaleY);
        overlay->resources->ScaleTextAboutCenter(text, overlay->scale.value, overlay->language);
        text.SetAlpha(convertFloatToInt32(overlay->alpha.value));
        overlay->secondText->SetAlpha(convertFloatToInt32(overlay->alpha.value));
    }
    tCubic1DUpdate(&overlay->scale, overlay->clock->worldTimeElapsed, 0.0f, 0.0f);
    tCubic1DUpdate(&overlay->alpha, overlay->clock->worldTimeElapsed, 0.0f, 0.0f);
    const uint32_t elapsedBits = static_cast<uint32_t>(overlay->clock->realTimeFrames) -
        static_cast<uint32_t>(overlay->lastDateFrames);
    int32_t elapsedFrames;
    memcpy(&elapsedFrames, &elapsedBits, sizeof(elapsedFrames));
    if (elapsedFrames >= 31) {
        const auto date = carNisOverlayUpdateDateAndTime(overlay);
        if (date != CAR_NIS_OK) return date;
    }
    // PAL1826B8 reads330C84, the accumulated World-frame clock. The
    // per-frame elapsed quantum is not a blink phase at steady frame rates.
    const int32_t firstVisible = blink && !(overlay->clock->worldTimeFrames & 8) ? 1 : 0;
    overlay->resources->SetObjectVisibility(overlay->screenKey, 0x61CB41DFu, firstVisible);
    overlay->resources->SetObjectVisibility(overlay->screenKey, 0x9ABA325Eu, blink ? 1 : 0);
    return CAR_NIS_OK;
}

int32_t carFramePlayerBindSettings(CarFramePlayerState* player,const PlayerSettings* settings)
{
    if (!player || !settings) return 0;
    player->settings = *settings;
    return 1;
}


int32_t carFramePlayerInitializeNisSelection(CarFramePlayerState* player)
{
    if (!player) return 0;
    if (player->nisSelection) return 1;
    try {
        player->nisSelection = new CarPlayerNisSelection;
        return 1;
    } catch (const std::bad_alloc&) {
        player->result = 1;
        return 0;
    }
}

void carFramePlayerDisposeNisSelection(CarFramePlayerState* player)
{
    if (!player) return;
    delete player->nisSelection;
    player->nisSelection = nullptr;
}

int32_t carFramePlayerCreateRandomNisArray(CarFramePlayerState* player, int32_t kind)
{
    if (!player || !player->replay || !player->nisSelection || (kind != 1 && kind != 2)) {
        if (player) player->result = 1;
        return 0;
    }
    const auto* resources = hp2::NisResourceCatalogue(*player->replay);
    if (!resources || resources->CountKind(kind) > 32) {
        player->result = 1;
        return 0;
    }
    const int32_t count = static_cast<int32_t>(resources->CountKind(kind));
    int32_t remaining[32]{};
    for (int32_t ordinal = 0; ordinal < count; ++ordinal) {
        const auto* info = resources->InfoByKind(static_cast<size_t>(ordinal), kind);
        if (!info) info = resources->InfoByKind(0, 1);
        if (!info) {
            player->result = 1;
            return 0;
        }
        remaining[ordinal] = info->authoredIndex;
    }
    const int32_t table = kind == 1 ? 0 : 1;
    for (int32_t selected = 0; selected < count; ++selected) {
        uint32_t ordinal = vectorDefaultRandomBounded(static_cast<uint32_t>(count - selected));
        int32_t position = -1;
        for (int32_t candidate = 0; candidate < count; ++candidate) {
            if (remaining[candidate] == -1) continue;
            if (ordinal == 0) {
                position = candidate;
                break;
            }
            --ordinal;
        }
        if (position < 0) {
            player->result = 1;
            return 0;
        }
        const int32_t index = remaining[position];
        player->nisSelection->infoIndices[table][selected] = index;
        remaining[position] = -1;
        const auto* info = index < 0 ? nullptr : resources->Info(static_cast<size_t>(index));
        if (!info || info->takeCount <= 0) {
            player->result = 1;
            return 0;
        }
        player->nisSelection->takeIndices[table][selected] = static_cast<int32_t>(
            vectorDefaultRandomBounded(static_cast<uint32_t>(info->takeCount)));
    }
    return 1;
}

int32_t carFramePlayerPredetermineNis(CarFramePlayerState* player)
{
    if (!carFramePlayerInitializeNisSelection(player)) return 0;
    if (!carFramePlayerCreateRandomNisArray(player, 1)) return 0;
    player->nisSelection->cursors[0] = -1;
    if (!carFramePlayerCreateRandomNisArray(player, 2)) return 0;
    player->nisSelection->cursors[1] = -1;
    return 1;
}

static uint32_t playerNisAppearanceKey(const CarMovement* movement)
{
    const CarDriverVisual* visual = movement->bindings.owner->visual;
    const CarDriverCharacter* character = visual ? visual->bindings.character : nullptr;
    return character ? hashString(character->appearanceName) : 0;
}

int32_t carFramePlayerFindNextNisIndex(CarFramePlayerState* player,
    const CarMovement* first, const CarMovement* second, int32_t kind)
{
    if (!player || !player->replay || !player->nisSelection || !player->registry ||
        !first || !second || !first->bindings.owner || !second->bindings.owner ||
        (kind != 1 && kind != 2)) {
        if (player) player->result = 1;
        return -1;
    }
    const auto* resources = hp2::NisResourceCatalogue(*player->replay);
    if (!resources || resources->CountKind(kind) == 0 || resources->CountKind(kind) > 32) {
        player->result = 1;
        return -1;
    }
    const uint32_t firstKey = playerNisAppearanceKey(first);
    const uint32_t secondKey = playerNisAppearanceKey(second);
    const int32_t table = kind == 2 ? 1 : 0;
    int32_t cursor = player->nisSelection->cursors[table];
    const auto* other = player == player->registry->players[0] ?
        player->registry->players[1] : player->registry->players[0];
    int32_t excluded = -1;
    if (other && other->replay) excluded = carLiveReplayCurrentInfoIndex(other->replay);
    const int32_t count = static_cast<int32_t>(resources->CountKind(kind));
    if (cursor < -1 || cursor >= count) {
        player->result = 1;
        return -1;
    }
    for (int32_t attempt = 0; ; ++attempt) {
        ++cursor;
        if (cursor >= count) cursor = 0;
        // PAL176184..176194 advances/wraps the candidate before testing the
        //33-check limit. The exhausted return is the next, untested slot.
        if (attempt == 33) break;
        if (cursor < 0) {
            player->result = 1;
            return -1;
        }
        const int32_t index = player->nisSelection->infoIndices[table][cursor];
        if (index < 0 || !resources->Info(static_cast<size_t>(index))) {
            player->result = 1;
            return -1;
        }
        if (resources->IsSuitable(static_cast<size_t>(index), firstKey, secondKey, excluded))
            break;
    }
    // Missing appearances preserve the original bounded fallback.
    return cursor;
}

int32_t carFramePlayerLoadNextNis(CarFramePlayerState* player,
    const CarMovement* first, const CarMovement* second, int32_t pulloverCount)
{
    if (!player || !player->registry || !player->registry->services ||
        !player->registry->services->race || !player->registry->services->race->services ||
        !player->registry->services->race->services->settings || !player->nisSelection) {
        if (player) player->result = 1;
        return 0;
    }
    const RaceParameters* parameters = player->registry->services->race->services->settings;
    int32_t kind = 1;
    if (!raceParametersIsFreeRun(parameters) && pulloverCount >= parameters->pulloverLimit)
        kind = 2;
    const int32_t cursor = carFramePlayerFindNextNisIndex(player, first, second, kind);
    if (cursor < 0) return 0;
    const int32_t table = kind == 1 ? 0 : 1;
    player->nisSelection->cursors[table] = cursor;
    const int32_t index = player->nisSelection->infoIndices[table][cursor];
    if (carLiveReplayLoad(player->replay, index) != CAR_NIS_OK) {
        player->result = 1;
        return 0;
    }
    return 1;
}

int32_t carFramePlayerPlayNextNis(CarFramePlayerState* player,
    CarMovement* first, CarMovement* second, CarPlayerNisCompletion completion,
    int32_t completionIndex)
{
    if (!player || !player->registry || !player->registry->services || !player->replay ||
        !player->nisSelection || !first || !second || !first->bindings.owner ||
        !first->bindings.ownerMotion || !player->nisOverlay ||
        (completion != CAR_PLAYER_NIS_COMPLETION_NONE &&
         completion != CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET)) {
        if (player) player->result = 1;
        return 0;
    }
    CarFramePlayerServices* services = player->registry->services;
    if (!services->pursuit || !services->session || !services->race ||
        !services->race->services || !services->race->services->settings) {
        player->result = 1;
        return 0;
    }
    player->nisCompletion = completion;
    player->nisCompletionIndex = completionIndex;
    int32_t tickets;
    if (pursuitManagerGetPulloverCount(services->pursuit, first->bindings.owner,
        &tickets) != WORLD_SERVICES_OK) {
        player->result = 1;
        return 0;
    }
    carFramePlayerEndState(player, 1);
    if (player->result) return 0;
    if (second->bindings.player) {
        if (!services->radialBlur || !services->radialBlur->cameras) {
            player->result = 1;
            return 0;
        }
        screenRadialBlurStop(services->radialBlur, second->bindings.player->seat);
    }
    carMotionClearWindow(first->bindings.ownerMotion);
    carGlareResetRequested = 1;
    auto& selection = *player->nisSelection;
    if (selection.cursors[1] < 0) selection.cursors[1] = 0; // Source Player+E8.
    if (selection.cursors[0] < 0) selection.cursors[0] = 0; // Source Player+EC.

    const auto* other = player == player->registry->playersByNumber[0] ?
        player->registry->playersByNumber[1] : player->registry->playersByNumber[0];
    int32_t excludedTake = -1;
    if (other && other->replay && other->replay->active) {
        if (!other->replay->cameraContext) {
            player->result = 1;
            return 0;
        }
        excludedTake = other->replay->cameraContext->takeIndex;
    }
    const RaceParameters* parameters = services->race->services->settings;
    const bool ticket = raceParametersIsFreeRun(parameters) || tickets < parameters->pulloverLimit;
    const int32_t table = ticket ? 0 : 1;
    // PAL176388/1763E4/1763F4 deliberately reads EC for both branches.
    // Preserve this shared cursor rather than substitute kind2's E8.
    const int32_t cursor = selection.cursors[0];
    if (cursor < 0 || cursor >= 32) {
        player->result = 1;
        return 0;
    }
    const int32_t index = selection.infoIndices[table][cursor];
    int32_t take = selection.takeIndices[table][cursor];
    const auto* resources = hp2::NisResourceCatalogue(*player->replay);
    const CarNisInfo* info = resources && index >= 0 ? resources->Info(static_cast<size_t>(index)) : nullptr;
    if (!info || info->takeCount <= 0 || take < 0 || take >= info->takeCount) {
        player->result = 1;
        return 0;
    }
    if (take == excludedTake) take = (take + 1) % info->takeCount;
    if (carLiveReplayStart(player->replay, index, take, first, second) != CAR_NIS_OK) {
        player->result = 1;
        return 0;
    }
    const CarNisResult overlay = ticket ? carNisOverlayStartTicket(player->nisOverlay) :
        carNisOverlayStartArrest(player->nisOverlay);
    if (overlay != CAR_NIS_OK) {
        player->result = 1;
        return 0;
    }
    const int32_t state = services->session->state;
    if (state != 4 && state != 7 && state != 5 && state != 9) {
        if (!player->cameras || carFramePlayerChangeCamera(player, 0, 0) != CAR_CINEMATIC_OK) {
            player->result = 1;
            return 0;
        }
        CarViewMoverResult result;
        CarViewMover* camera = carLiveReplayCreateView(player->replay,
            player->cameras->viewNumber, &result);
        if (!camera || result != CAR_VIEW_MOVER_OK) {
            player->result = 1;
            return 0;
        }
        player->cameras->replay = camera;
    }
    return 1;
}

struct CarPlayerVisibleDrivingLine {
    CarFramePlayerState* player = nullptr;
    float duration = 0.0f;
    int32_t startedTicks = 0;
    Spline spline{};
    PreviewComet* comet = nullptr;
};

CarPlayerVisibleDrivingLine* carPlayerVisibleDrivingLineCreate(
    CarFramePlayerState* player,int32_t variant,float duration)
{
    if (!player || !player->registry || !player->registry->clock ||
        !player->registry->services || !player->registry->services->previewComet ||
        !player->car || !player->car->bindings.state || !player->car->bindings.info ||
        !player->car->bindings.owner || !player->car->bindings.recoveryPoints ||
        !player->car->bindings.environment || !player->car->bindings.environment->contact.terrain ||
        !player->car->bindings.environment->contact.terrain->topology || !std::isfinite(duration))
        return nullptr;
    auto line=std::make_unique<CarPlayerVisibleDrivingLine>();
    if (!splineInitialize(&line->spline,SPLINE_EXTRAPOLATE,100,0))return nullptr;
    line->comet=previewCometInitialize(&line->spline,previewCometParameters(variant),
        player->registry->services->previewComet);
    if (!line->comet) {
        splineDestroy(&line->spline,0);
        return nullptr;
    }
    line->duration=std::min(20.0f,std::max(duration,1.0f));
    line->player=player;
    line->startedTicks=player->registry->clock->worldTimer;

    auto& movement=*player->car;
    const auto* driver=static_cast<const CarDriver*>(movement.bindings.owner->controller);
    if (!driver) {
        previewCometDispose(line->comet);
        splineDestroy(&line->spline,0);
        return nullptr;
    }
    const auto* group=carDriverCurrentGroup(driver,1);
    const auto* pose=carDriverCurrentPose(driver,1);
    const float length=movement.bindings.state->speed*line->duration+100.0f;
    CarRoadPreview preview{};
    const int32_t prepared=carRoadPreviewInitialize(&preview,movement.bindings.recoveryPoints,
        group,pose,movement.bindings.state->position,movement.routeLocation.direction,
        40.0f,length,movement.bindings.info->routeVehicleClass,0);
    int32_t built=0;
    if (prepared && preview.steps && preview.count>=4) {
        TerrainSurface terrain;
        terrainSurfaceInitialize(&terrain,movement.bindings.environment->contact.terrain->topology);
        built=carCameraBuildTerrainCurves(&preview,&terrain,&line->spline,nullptr,nullptr,nullptr);
        terrainSurfaceDispose(&terrain);
    }
    carRoadPreviewDispose(&preview);
    if (!built || line->spline.count<2) {
        // Source rendering a missing/degenerate PreviewRoute has no valid
        // spline domain. Native callers receive an explicit creation failure.
        previewCometDispose(line->comet);
        splineDestroy(&line->spline,0);
        return nullptr;
    }
    return line.release();
}

int32_t carFramePlayerTriggerVisibleDrivingLine(CarFramePlayerState* player,uint32_t variant,float duration)
{
    if (!player || !player->registry || !player->registry->enableVisibleDrivingLine)return 0;
    if (!*player->registry->enableVisibleDrivingLine)return 1;
    if (!player->registry->services || !player->registry->services->session)return 0;
    if (player->registry->services->session->state!=3 || player->visibleDrivingLine)return 1;
    if (duration==0.0f)duration=5.0f;
    int32_t signedVariant;
    memcpy(&signedVariant,&variant,sizeof(signedVariant));
    try {
        player->visibleDrivingLine=carPlayerVisibleDrivingLineCreate(player,signedVariant,duration);
    } catch (const std::bad_alloc&) {
        return 0;
    }
    return player->visibleDrivingLine ? 1:0;
}

void carFramePlayerDisposeVisibleDrivingLine(CarFramePlayerState* player)
{
    if (!player || !player->visibleDrivingLine)return;
    auto* line=player->visibleDrivingLine;
    previewCometDispose(line->comet);
    splineDestroy(&line->spline,0);
    delete line;
    player->visibleDrivingLine=nullptr;
}

const Spline* carFramePlayerVisibleDrivingLineSpline(const CarFramePlayerState* player)
{
    return player && player->visibleDrivingLine ? &player->visibleDrivingLine->spline:nullptr;
}

static float playerVisibleDrivingLineParameter(const CarFramePlayerState* player)
{
    const auto& line=*player->visibleDrivingLine;
    const uint32_t ticks=static_cast<uint32_t>(player->registry->clock->worldTimer)-
        static_cast<uint32_t>(line.startedTicks);
    int32_t elapsed;
    memcpy(&elapsed,&ticks,sizeof(elapsed));
    const float seconds=static_cast<float>(elapsed)*0.0002500000118743628f;
    const float normalized=finiteRatio(seconds,line.duration);
    const float scaled=(normalized+0.09999999403953552f)*0.9090908765792847f;
    return std::min(1.0f,std::max(0.0f,scaled));
}

int32_t carFramePlayerRenderVisibleDrivingLine(CarFramePlayerState* player,PreviewCometMesh* mesh)
{
    if (!player || !player->registry || !player->registry->clock || !mesh)return 0;
    if (!player->visibleDrivingLine) {mesh->count=0;mesh->texture=nullptr;return 1;}
    return previewCometRender(player->visibleDrivingLine->comet,
        playerVisibleDrivingLineParameter(player),mesh)==WORLD_SERVICES_OK;
}

// PAL180AE0..180B78 RenderPlayers: the same retained players and visible
// lines consumed by the frame producer, with the original session/view gate.
int32_t carFramePlayerRenderPlayers(CarFramePlayerRegistry* registry,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame)
{
    if (!registry || !screen || !frame || registry->count<0 || registry->count>2)return 0;
    for (int32_t index=0;index<registry->count;++index) {
        auto* player=registry->players[index];
        if (!player)return 0;
        // PAL180AFC exits for the empty startup list;180B1C skips absent
        // visible lines before dereferencing the in-game session owner.
        if (!player->visibleDrivingLine)continue;
        if (!registry->services || !registry->services->session)return 0;
        if (registry->services->session->state!=3)continue;
        if (!player->cameras)return 0;
        if (player->cameras->viewNumber!=screen->id)continue;
        if (!registry->clock)return 0;
        if (previewCometRenderFrame(player->visibleDrivingLine->comet,
            playerVisibleDrivingLineParameter(player),screen,frame)!=WORLD_SERVICES_OK)return 0;
    }
    return 1;
}

int32_t carFramePlayerServiceVisibleDrivingLine(CarFramePlayerState* player)
{
    // Exact source frame subgraph180EC8..180F38, kept as a host dependency
    // of that still-pending whole Player camera/input frame. No extra body.
    if (!player || !player->registry || !player->registry->services ||
        !player->registry->services->session || !player->registry->clock)return 0;
    if (player->registry->services->session->state!=3 || !player->visibleDrivingLine)return 1;
    const uint32_t ticks=static_cast<uint32_t>(player->registry->clock->worldTimer)-
        static_cast<uint32_t>(player->visibleDrivingLine->startedTicks);
    int32_t elapsed;
    memcpy(&elapsed,&ticks,sizeof(elapsed));
    const float seconds=static_cast<float>(elapsed)*0.0002500000118743628f;
    if (player->visibleDrivingLine->duration<=seconds)
        carFramePlayerDisposeVisibleDrivingLine(player);
    return 1;
}

static int32_t playerFrameSignedTicks(uint32_t bits)
{
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

// Original Player::IsNisPlaying, PAL176520..176530. The actual NisManager
// playback remains the sole owner of this flag.
int32_t carFramePlayerReplayActive(const CarFramePlayerState* player)
{
    return player->replay->active;
}

// Original Player::IsAllowedToReset, PAL181E98..181F68. The final gate
// reads pCurrentRace+10, not a session clock or an independent timer borrow.
// Race::Update148AD8..148AF0 writes that same elapsed race timer.
int32_t carFramePlayerCrashEligible(const CarFramePlayerState* player)
{
    const int32_t mode=player->movement.steeringFeedbackBusy;
    // Final330798 contains {0,0,0,1,1,1,1,1}; source explicitly permits6/7.
    if (mode>=3 && mode<=5)return 0;
    if (carFramePlayerReplayActive(player))return 0;
    if (60.0f<player->car->bindings.state->speed*2.236999988555908f)return 0;
    if (!*player->registry->pursuitEnabled)return 1;
    const auto& race=*player->registry->services->race;
    return !(static_cast<float>(race.elapsedTicks)*0.0002500000118743628f<5.0f);
}

// Original Player::ChooseToDisplayReset, PAL181F68..182208. The source
// collision sample, world timer and pursuit group all remain borrowed from
// the live Car/World graph; the reset gate above reads the actual Race.
void carFramePlayerUpdateCrashState(CarFramePlayerState* player)
{
    const auto valid=[](int32_t ticks){return ticks!=0 && ticks!=INT_MAX;};
    const auto elapsed=[](int32_t now,int32_t then){
        return playerFrameSignedTicks(static_cast<uint32_t>(now)-static_cast<uint32_t>(then));
    };
    VehicleState* state=player->car->bindings.state;
    const int32_t now=player->registry->clock->worldTimer;
    const int32_t collision=player->car->bindings.ownerMotion->collision->collisionTicks;
    const float speed=state->speed*2.236999988555908f;
    const float collisionAge=static_cast<float>(elapsed(now,collision))*0.0002500000118743628f;
    if (60.0f<speed && collisionAge<3.0f &&
        50.0f<std::fabs(vehicleStateVelocityHeadingDifference(state)*57.2957763671875f))
        player->crashTicks=now;
    const float sampleAge=static_cast<float>(elapsed(now,player->speedSampleTicks))*0.0002500000118743628f;
    if (0.5f<sampleAge) {
        const float acceleration=finiteRatio(speed-player->previousSpeedMph,sampleAge);
        if (60.0f<player->previousSpeedMph && collisionAge<3.0f && acceleration< -70.0f)
            player->crashTicks=now;
        player->speedSampleTicks=now;
        player->previousSpeedMph=speed;
    }
    if (speed<10.0f && state->orientation[10]<0.0f)player->crashTicks=now;
    if (elapsed(now,player->crashTicks)>20000)player->crashTicks=0;
    if (valid(player->crashTicks) && carFramePlayerCrashEligible(player)) {
        const auto* manager=player->car->bindings.environment->recovery.pursuit;
        PursuitCollisionGroup* group=nullptr;
        if (manager && carRecoveryFind(manager,player->car->bindings.owner,&group)!=WORLD_SERVICES_OK) {
            player->result=1;
            return;
        }
        if (!manager || !group) {
            if (!valid(player->recoveryProtectionTicks))player->crashTicks=0;
            return;
        }
    }
    if (valid(player->recoveryProtectionTicks) &&
        elapsed(now,player->recoveryProtectionTicks)>12000)player->recoveryProtectionTicks=0;
}

// Original Player::PlayerSelectedReset, PAL182208..182288. Native camera
// Reset dispatch and the ordered UI command retain the existing concrete
// camera/Player owners; no replacement camera state is introduced.
void carFramePlayerOnRecovered(CarFramePlayerState* player)
{
    player->recoveryProtectionTicks=0;
    player->crashTicks=0;
    carFramePlayerResetCurrentCamera(player);
    auto* event=carFramePresentationAppend(player->presentation,
        CAR_FRAME_UI_CLEAR_TRANSIENT,player->car,8);
    if (!event)player->result=1;
}

static int playerFrameHasHistorySampleState(int32_t state)
{
    return state == 3 || state == 4 || state == 7 || state == 5 || state == 9;
}

static int playerFrameValidBinding(const CarFramePlayerState* player)
{
    return player && player->car && player->replay && player->registry &&
        player->registry->clock && player->registry->services &&
        player->registry->services->session && player->car->bindings.owner &&
        player->car->bindings.state && player->car->bindings.info &&
        player->car->bindings.ownerMotion &&
        player->car->bindings.ownerMotion->collision &&
        player->car->bindings.environment;
}

void carFramePlayerAdvance(CarFramePlayerState* player)
{
    if (!playerFrameValidBinding(player)) {
        if (player) player->result = 1;
        return;
    }
    player->result = 0;
    carFramePlayerServiceReplay(player);
    if (player->result) return;

    player->wrongWay = carFramePlayerWrongWay(player, player->wrongWay) != 0;
    carFramePlayerUpdateCrashState(player);

    /* PAL180C60..180C88 reads completion/camera fields, but both temporary
     * values are overwritten before any branch/store/call consumes them.
     * They do not become another native completion or camera-mode flag. */
    const GameTime* clock = player->registry->clock;
    const int32_t sessionState = player->registry->services->session->state;
    if (playerFrameHasHistorySampleState(sessionState)) {
        /* This is SetTime136DD0's rounding, not GameTimerAdvance's truncated
         * elapsed conversion and not the3600Hz replay quantizer. */
        const float scaledElapsed = clock->worldTimeElapsed * 4000.0f;
        const int32_t elapsedTicks = convertFloatToInt32(scaledElapsed + 0.5f);
        player->speedRatioCountdownTicks = playerFrameSignedTicks(
            (uint32_t)player->speedRatioCountdownTicks - (uint32_t)elapsedTicks);
        const float remainingSeconds =
            (float)player->speedRatioCountdownTicks * 0.0002500000118743628f;
        if (remainingSeconds < 0.0f) {
            CarMovement* movement = player->car;
            const CarDriver* driver = (const CarDriver*)movement->bindings.owner->controller;
            if (!driver) {
                player->result = 1;
                return;
            }
            const CarRecoveryGroup* group = carDriverCurrentGroup(driver, 1);
            if (group) {
                const int32_t vehicleClass = movement->bindings.info->routeVehicleClass;
                if (vehicleClass < 0 || vehicleClass > 10 || !player->speedHistory.capacity ||
                    !player->speedHistory.samples) {
                    player->result = 1;
                    return;
                }
                const int32_t lane = vehicleClass * 2;
                const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
                float ratio = 1.0f;
                if (pose && (group->laneFlags[lane] & 3u)) {
                    const float authoredSpeed = (float)pose->laneSpeeds[lane];
                    ratio = finiteRatio(movement->bindings.state->forwardSpeed, authoredSpeed);
                }
                driverHistoryPush(&player->speedHistory, ratio);
                player->maximumSpeedRatio = maximumFinite(
                    player->maximumSpeedRatio, player->speedHistory.value);
            }
            player->speedRatioCountdownTicks = 4000;
        }
    }
    if (0.0f < player->cinematicCountdown) {
        const float remaining = player->cinematicCountdown - clock->worldTimeElapsed;
        player->cinematicCountdown = remaining;
        if (remaining <= 0.0f) {
            player->cinematicCountdown = 0.0f;
            if (carFramePlayerStartState(player, 1, 0, 1, 1)) {
                const CarFramePlayerServices* services = player->registry->services;
                if (worldAudioPlayBustRoadBlock(services->audio, services->audioManager,
                        player->car->bindings.state->position) != WORLD_AUDIO_OK)
                    player->result = 1;
            }
        }
    }
}

void carFramePlayerAdvanceAll(CarFramePlayerRegistry* registry)
{
    if (!registry || registry->count < 0 || registry->count > 2) return;
    for (int32_t index = 0; index < registry->count; ++index) {
        CarFramePlayerState* player = registry->players[index];
        if (!player || player->registry != registry) {
            if (player) player->result = 1;
            return;
        }
        carFramePlayerAdvance(player);
    }
}

void carNisOverlayFinish(CarNisOverlay* overlay)
{
    if (!overlay || !overlay->screenKey) return;
    overlay->manager->SetScreenVisibility(*overlay->resources, overlay->screenKey, 1);
    if (overlay->object) {
        overlay->object->SetScaleX(overlay->savedScaleX);
        overlay->object->SetScaleY(overlay->savedScaleY);
        overlay->object->SetTopLeftY(overlay->savedTopLeftY);
    }
}

void carFramePlayerServiceReplay(CarFramePlayerState* player)
{
    if (!player || !player->replay) {
        if (player) player->result = 1;
        return;
    }
    CarFrameLiveReplay* replay = player->replay;
    if (!replay->active) return;
    if (!player->registry || !player->registry->clock || !player->registry->services ||
        !player->registry->services->session) {
        player->result = 1;
        return;
    }
    /* PAL1B13E8 is exactly JR RA / MOVE V0,ZERO. Its loop-only camera
     * creation arm is statically unreachable, not a neutral callback. */
    if (player->registry->clock->worldTimeFrames < replay->expiresWorldFrame) return;
    CarFramePlayerServices* services = player->registry->services;
    const int32_t state = services->session->state;
    if (state != 4 && state != 7 && state != 5 && state != 9) {
        if (carFramePlayerChangeCamera(player, 1, 0) != CAR_CINEMATIC_OK) {
            player->result = 1;
            return;
        }
    }
    if (carLiveReplayStop(replay) != CAR_NIS_OK) {
        player->result = 1;
        return;
    }
    switch (player->nisCompletion) {
    case CAR_PLAYER_NIS_COMPLETION_NONE:
        break;
    case CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET:
        if (!services->pursuit || pursuitManagerResetParticipant(services->pursuit,
                player->nisCompletionIndex) != WORLD_SERVICES_OK) {
            player->result = 1;
            return;
        }
        player->nisCompletion = CAR_PLAYER_NIS_COMPLETION_NONE;
        player->nisCompletionIndex = 0;
        break;
    default:
        player->result = 1;
        return;
    }
    if (carLiveReplayReleaseRecordBuffers(replay) != CAR_NIS_OK) {
        player->result = 1;
        return;
    }
    if (!player->nisOverlay || (player->nisOverlay->screenKey &&
        (!player->nisOverlay->manager || !player->nisOverlay->resources))) {
        player->result = 1;
        return;
    }
    carNisOverlayFinish(player->nisOverlay);
}

WorldAudioResult worldAudioPlayScore(WorldAudioRequestQueue* queue,
    const GameSession* session, int32_t score)
{
    if (!queue || !queue->spatialEnabled) return WORLD_AUDIO_INVALID_BINDING;
    if (!*queue->spatialEnabled) return WORLD_AUDIO_OK;
    if (!session) return WORLD_AUDIO_INVALID_BINDING;
    const int32_t state = session->state;
    if (state == 4 || state == 7 || state == 5 || state == 9) return WORLD_AUDIO_OK;
    if (score != 0 && score != 16 && score != 6 && score != 27) return WORLD_AUDIO_OK;
    return worldAudioPlayInGameIfIdle(queue, "ScorePoints", 1.0f, 0.0f, 0.0f);
}

struct CarPlayerScoreState {
    hp2::LanguageManager* language{};
    hp2::FlailerTextChunk* captionText{};
    hp2::FlailerTextChunk* amountText{};
    hp2::FlailerTextChunk* totalText{};
    std::unique_ptr<hp2::HudAnimator> amountAnimation;
    std::unique_ptr<hp2::HudAnimator> captionAnimation;
    std::array<char,32> caption{};
    std::array<char,32> amount{};
    std::array<char,32> runningAmount{};
    int32_t total{};
    int32_t lastStartedTicks{INT32_MAX};
};

namespace {

struct PlayerScoreDefinition {
    uint32_t languageId;
    const char* amount;
};

/* Final PAL compiled Scoring descriptors2FBBF0..2FC028. The unused English
 * diagnostic labels are not another localization database. */
const std::array<PlayerScoreDefinition,30> playerScoreDefinitions{{
    {0x000495D7u,"+2000"}, {0x000495D8u,"-1000"},
    {0x000495D9u,"-1000"}, {0x000495DAu,"+2000"},
    {0x000495DBu,"+1500"}, {0x000495DCu,"+1000"},
    {0x000495DDu,"+1000"}, {0x000495DEu,"+500"},
    {0x000495DFu,"+500"}, {0x000495E0u,"+500"},
    {0x000495E1u,"+0"}, {0x000495E2u,"+250"},
    {0x000495E3u,"+250"}, {0x000495E4u,"+250"},
    {0x000495E5u,"+250"}, {0x000495F6u,"+2000"},
    {0x000495E6u,"+5000"}, {0x000495E7u,"+2000"},
    {0x000495E8u,"+1000"}, {0x000495E9u,"+750"},
    {0x000495EAu,"+500"}, {0x000495EBu,"+250"},
    {0x000495ECu,"+100"}, {0x000495EEu,"+10000"},
    {0x000495EFu,"+5000"}, {0x000495F0u,"+2500"},
    {0x000495F1u,"+2000"}, {0x000495F2u,"+5000"},
    {0x000495F3u,"+500"}, {0x000495F4u,"-500"}
}};

void playerScoreCopy(std::array<char,32>& destination, const char* source)
{
    const std::size_t size = std::strlen(source);
    if (size >= destination.size())
        throw std::invalid_argument("Player score caption exceeds its owned buffer");
    std::memcpy(destination.data(), source, size + 1);
}

std::array<float,2> playerScoreVector(const float value[2])
{
    return {value[0],value[1]};
}

std::array<float,4> playerScoreColor(const float value[4])
{
    return {value[0],value[1],value[2],value[3]};
}

}

int32_t carFramePlayerScoreStringValue(const char* text)
{
    int32_t sign = 1;
    if (text[0] == '-') {
        sign = -1;
        ++text;
    } else if (text[0] == '+') {
        ++text;
    }
    const uint32_t magnitude = static_cast<uint32_t>(stringToLong(text));
    const uint32_t bits = static_cast<uint32_t>(sign) * magnitude;
    int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

void carFramePlayerAddToScore(CarFramePlayerState* player, const char* text)
{
    if (!player || !player->scoreState || !text) {
        if (player) player->result = 1;
        return;
    }
    CarPlayerScoreState* state = player->scoreState;
    const uint32_t sum = static_cast<uint32_t>(state->total) +
        static_cast<uint32_t>(carFramePlayerScoreStringValue(text));
    std::memcpy(&state->total, &sum, sizeof(sum));
    if (state->total < 0) state->total = 0;
    char total[32];
    std::snprintf(total, sizeof(total), "%d", state->total);
    if (state->totalText) state->totalText->SetString(total);
}

void carFramePlayerStartScoreAnimation(CarFramePlayerState* player,
    const CarPlayerScoreAnimation* animation)
{
    if (!player || !player->scoreState || !animation || !player->registry ||
        !player->registry->clock) {
        if (player) player->result = 1;
        return;
    }
    try {
        CarPlayerScoreState* state = player->scoreState;
        auto position = playerScoreVector(animation->position);
        auto positionIn = playerScoreVector(animation->positionIn);
        auto positionOut = playerScoreVector(animation->positionOut);
        if (player->registry->count >= 2 &&
            (player->movement.seat == 0 || player->movement.seat == 1)) {
            position[1] *= 0.5f;
            positionIn[1] *= 0.5f;
            positionOut[1] *= 0.5f;
            position[1] += player->movement.seat == 0 ? 40.0f : 280.0f;
        }
        if (!state->amountAnimation)
            state->amountAnimation = std::make_unique<hp2::HudAnimator>();
        if (state->amountAnimation->Active())
            carFramePlayerAddToScore(player, state->runningAmount.data());
        hp2::HudTextAnimation request{};
        request.text = state->amountText;
        request.position = position;
        request.positionIn = positionIn;
        request.positionOut = positionOut;
        request.scale = playerScoreVector(animation->scale);
        request.scaleIn = playerScoreVector(animation->scaleIn);
        request.alpha = 0;
        request.alphaIn = 128.0f;
        request.color = playerScoreColor(animation->color);
        request.heldScale = request.scale;
        request.scaleOut = request.scaleIn;
        request.heldAlpha = 64;
        request.alphaOut = -64.0f;
        request.colorChange = playerScoreColor(animation->colorChange);
        request.durationSeconds = {0.5f,0.5f,1.0f};
        request.caption = nullptr;
        request.useRealTimer = false;
        state->amountAnimation->StartAnimation(request, *player->registry->clock);
        if (!state->captionAnimation)
            state->captionAnimation = std::make_unique<hp2::HudAnimator>();
        playerScoreCopy(state->runningAmount, state->amount.data());
        position[1] -= 20.0f;
        request.text = state->captionText;
        request.position = position;
        state->captionAnimation->StartAnimation(request, *player->registry->clock);
        state->lastStartedTicks = player->registry->clock->worldTimer;
    } catch (...) {
        player->result = 1;
    }
}

void carFramePlayerStartScore(CarFramePlayerState* player, int32_t score)
{
    if (!player || !player->registry || !player->registry->services) {
        if (player) player->result = 1;
        return;
    }
    const CarFramePlayerServices* services = player->registry->services;
    if (worldAudioPlayScore(services->audio, services->session, score) != WORLD_AUDIO_OK) {
        player->result = 1;
        return;
    }
    if (!player->scoreState || !player->scoreState->language || score < 0 ||
        static_cast<size_t>(score) >= playerScoreDefinitions.size()) {
        player->result = 1;
        return;
    }
    try {
        CarPlayerScoreState* state = player->scoreState;
        const PlayerScoreDefinition& definition = playerScoreDefinitions[score];
        playerScoreCopy(state->amount, definition.amount);
        playerScoreCopy(state->caption, state->language->GetString(
            static_cast<int32_t>(definition.languageId)));
        const CarPlayerScoreAnimation animation{{530.0f,236.0f},{0,0},{90.0f,-100.0f},
            {0.699999988079071f,0.699999988079071f},{0,0},{128.0f,64.0f,0,0},{0,64.0f,128.0f,0}};
        carFramePlayerStartScoreAnimation(player, &animation);
    } catch (...) {
        player->result = 1;
    }
}

int32_t carFramePlayerReadTotalScore(const CarFramePlayerState* player, int32_t* total)
{
    if (!player || !player->scoreState || !total) return 0;
    *total = player->scoreState->total;
    return 1;
}

void carFramePlayerDisposeScore(CarFramePlayerState* player)
{
    if (!player || !player->scoreState) return;
    if (player->scoreState->amountAnimation)
        player->scoreState->amountAnimation->FinishAnimation();
    if (player->scoreState->captionAnimation)
        player->scoreState->captionAnimation->FinishAnimation();
    delete player->scoreState;
    player->scoreState = nullptr;
}

namespace hp2 {

bool BindPlayerScore(CarFramePlayerState& player, const PlayerScoreBindings& bindings)
{
    if (!bindings.resources || !bindings.language) return false;
    try {
        if (!player.scoreState) player.scoreState = new CarPlayerScoreState;
        CarPlayerScoreState* state = player.scoreState;
        state->language = bindings.language;
        state->captionText = nullptr;
        state->amountText = nullptr;
        state->totalText = nullptr;
        if (bindings.enabled) {
            auto text = [&](uint32_t key) {
                return static_cast<FlailerTextChunk*>(
                    bindings.resources->FindObject(bindings.screenKey,key,1));
            };
            state->captionText = text(0xA3FF1482u);
            state->amountText = text(0xA3E99D3Cu);
            state->totalText = text(0x5D85AFBEu);
        }
        return true;
    } catch (...) {
        return false;
    }
}

}



namespace {
enum class PlayerInputOperation : std::uint8_t {
    Ignore, CycleView, LookBack, StateOne, StateTwo, StateThree,
    RoadBlock, Chopper, CycleHud, SessionPause
};
// Semantic labels of the final writable39-entry compiler table2B3C80.
// This is one native table, not PS2 code pointers or a callback registry.
std::array<PlayerInputOperation,39> playerInputOperations{{
    PlayerInputOperation::CycleView, PlayerInputOperation::LookBack,
    PlayerInputOperation::StateOne, PlayerInputOperation::StateTwo,
    PlayerInputOperation::StateThree, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::Ignore,
    PlayerInputOperation::Ignore, PlayerInputOperation::RoadBlock,
    PlayerInputOperation::Chopper, PlayerInputOperation::CycleHud,
    PlayerInputOperation::SessionPause
}};
// The source copies both mutable mode tables to its stack before lookup.
std::array<int32_t,4> singlePlayerHudModes{{0,1,3,2}};
std::array<int32_t,3> twoPlayerHudModes{{0,3,2}};
}



static void raceAction(CarFramePlayerState* player,int32_t action,int32_t argument)
{
    const auto* registry=player->registry;
    auto* services=registry?registry->services:nullptr;
    if (!services || !services->session ||
        !raceActionsEnqueue(&services->session->actions,action,argument))player->result=1;
}

void carFramePlayerCycleInputProfile(CarFramePlayerState* player, int32_t delta)
{
    const auto singlePlayer=singlePlayerHudModes;
    const auto twoPlayers=twoPlayerHudModes;
    const int32_t* profiles = player->registry->count == 2 ? twoPlayers.data() : singlePlayer.data();
    const int32_t count = player->registry->count == 2 ? 3 : 4;
    int32_t index = 0;
    for (int32_t candidate = 0; candidate < count; ++candidate)
        if (profiles[candidate] == player->settings.inputProfile) index = candidate;
    index = (int32_t)((uint32_t)index + (uint32_t)delta);
    if (index < 0) index = count - 1;
    if (index >= count) index = 0;
    player->settings.inputProfile = (int8_t)profiles[index];
}

void carFramePlayerMenuInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed)
{
    (void)pressed;
    if (event == 0x83) {
        if (!player->drivingLocked) raceAction(player, 5, device);
    } else if (event == 0x84) {
        if (!player->registry || !player->registry->services || !player->registry->services->session) { player->result=1;return; }
        if (player->registry->services->session->state != 1 && !*player->registry->pursuitEnabled)
            raceAction(player, 4, 0);
    }
}

void carFramePlayerRaceInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed)
{
    (void)pressed;
    if (event != 0x83) return;
    if (!player->registry || !player->registry->services || !player->registry->services->session) { player->result=1;return; }
    const int32_t kind = player->registry->services->session->state;
    if (kind == 4 || kind == 7 || kind == 5 || kind == 9) return;
    if (carFramePlayerAllDrivingLocked(player->registry)) raceAction(player, 5, device);
}

void carFramePlayerRouteInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed)
{
    if (event != 0x7B && event != 0x7C) return;
    if (!*player->registry->pursuitEnabled) {
        carFramePlayerDispatchInput(player, device, event == 0x7B ? 0x60 : 0x5F, pressed);
        return;
    }
    if (!player->registry->services || !player->registry->services->pursuit) {
        player->result = 1;
        return;
    }
    PursuitCollisionEntry* selected = pursuitManagerSelectedEntry(player->registry->services->pursuit);
    if (!selected) { player->result = 1; return; }
    if (selected->movement == player->car)
        carFramePlayerDispatchInput(player, device, event == 0x7B ? 0x80 : 0x81, pressed);
}

void carFramePlayerDispatchInput(CarFramePlayerState* player,int32_t device,
    int32_t event,uint8_t pressed)
{
    if (!player)return;
    const auto index=static_cast<std::uint32_t>(event)-0x5du;
    if (index>=playerInputOperations.size())return;
    const auto operation=playerInputOperations[index];
    auto* registry=player->registry;
    auto* services=registry?registry->services:nullptr;
    switch (operation) {
    case PlayerInputOperation::Ignore:return;
    case PlayerInputOperation::SessionPause:
        raceAction(player,5,device);
        return;
    case PlayerInputOperation::CycleView: {
        if (!registry || !registry->clock) { player->result=1;return; }
        const auto difference=static_cast<std::uint32_t>(registry->clock->realTimer)-
            static_cast<std::uint32_t>(player->inputActionRealTimer);
        int32_t ticks;std::memcpy(&ticks,&difference,sizeof(ticks));
        const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
        if (!(1.0f<elapsed) || player->inputMode!=2)return;
        if (!player->cameras || !player->cameras->current.base ||
            player->cameras->current.base->kind!=1)return;
        player->settings.cameraView=static_cast<int8_t>(
            carFramePlayerSelectCameraView(player,player->settings.cameraView+1));
        return;
    }
    case PlayerInputOperation::CycleHud:
        if (player->settings.hudHotkey) {
            if (!registry) { player->result=1;return; }
            carFramePlayerCycleInputProfile(player,1);
        }
        return;
    case PlayerInputOperation::StateOne:
    case PlayerInputOperation::StateTwo:
    case PlayerInputOperation::StateThree: {
        const int32_t state=operation==PlayerInputOperation::StateOne?1:
            operation==PlayerInputOperation::StateTwo?2:3;
        carFramePlayerStartState(player,state,1,1,1);
        return;
    }
    case PlayerInputOperation::RoadBlock:
    case PlayerInputOperation::Chopper: {
        if (!registry || !registry->clock || !services || !services->pursuit) {
            player->result=1;return;
        }
        const auto result=operation==PlayerInputOperation::RoadBlock?
            pursuitManagerRequestRoadBlock(services->pursuit):
            pursuitManagerRequestChopper(services->pursuit);
        if (result!=WORLD_SERVICES_OK)player->result=1;
        // The original stores this even when a valid request is refused.
        player->inputActionRealTimer=registry->clock->realTimer;
        return;
    }
    case PlayerInputOperation::LookBack: {
        const int32_t requested=pressed!=0;
        player->lookBack=requested;
        if (!worldInputPauseGate) {
            if (!registry || !registry->clock) { player->result=1;return; }
            // PAL178AC8 writes the first world-time state cooldown+68;
            // it is distinct from the real-time input debounce cell+98.
            player->stateStartedTicks[0]=registry->clock->worldTimer;
        }
        if (player->cameras) {
            auto* camera=&player->cameras->current;
            if (!camera->base || camera->base->kind!=1)
                camera=&player->cameras->suspended[0];
            if (camera->base && camera->base->kind==1) {
                if (camera->family!=CAR_PLAYER_CAMERA_CHASE || !camera->owner.chase) {
                    player->result=1;return;
                }
                carChaseCameraSetFacingTarget(camera->owner.chase,requested);
            }
        }
        if (!player->car || !player->car->bindings.owner) { player->result=1;return; }
        auto* visual=player->car->bindings.owner->visual;
        if (visual) {
            if (pressed)carDriverVisualBeginLookBack(visual);
            else carDriverVisualEndLookBack(visual);
        }
        return;
    }
    default:player->result=1;return;
    }
}


int32_t carFramePlayerBindSoundPolicy(CarFramePlayerRegistry* registry)
{
    if (!registry) return 0;
    registry->soundPolicy = carSoundCurrentPolicy();
    return 1;
}

int32_t carFramePlayerSoundEnabled(const CarFramePlayerRegistry* registry)
{
    if (!registry || registry->soundPolicy != carSoundCurrentPolicy()) std::abort();
    return registry->soundPolicy->audioSystemEnabled;
}

void carFramePlayerUpdateCinematicAudio(CarFramePlayerState* player, uint16_t controlCode,
    float strength, float volume, float pitchOffset)
{
    if (!player || !player->registry) return;
    if (!carFramePlayerSoundEnabled(player->registry) || !player->cinematicVoice) return;
    CarFramePlayerServices* services = player->registry->services;
    const WorldAudioVoiceRef voice = {player->cinematicVoice, player->cinematicVoiceGeneration};
    if (!services || !services->audio ||
        worldAudioSetVoiceParameters(services->audio, voice,
            controlCode, pitchOffset, volume) != WORLD_AUDIO_OK) {
        player->result = 1;
        return;
    }
    player->transitionAudioStrength = strength;
}

WorldAudioResult playerCinematicVoiceStart(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t enabled, int32_t playAudio,
    WorldAudioVoiceRef* voice, const char* cueName, float gain, float pitchScale)
{
    const WorldAudioCue* cue;
    WorldAudioVoiceRef replacement;
    WorldAudioResult result;
    size_t needed;
    if (!enabled || !playAudio) return WORLD_AUDIO_OK;
    if (!manager || !manager->channelVolumes || !manager->channelVolumeCount)
        return WORLD_AUDIO_INVALID_BINDING;
    if (!(manager->channelVolumes[0] > 0.0f)) return WORLD_AUDIO_OK;
    if (!queue || !voice || !cueName) return WORLD_AUDIO_INVALID_BINDING;
    if (voice->instance) {
        result = worldAudioValidateVoice(queue, *voice);
        if (result != WORLD_AUDIO_OK) return result;
    }
    cue = worldAudioFindInGame(queue, cueName);
    if (!cue) return WORLD_AUDIO_INVALID_BINDING;
    needed = voice->instance ? 2 : 1;
    if (worldAudioAvailableRequests(queue) < needed ||
        queue->nextSequence > UINT64_MAX - needed) return WORLD_AUDIO_CAPACITY;
    /* Reserve the replacement before retiring the old voice. A capacity
     * failure cannot leave the Player pointing at a retired instance. */
    result = worldAudioAcquireVoice(queue, queue->inGameBank, cue, -1, 0, &replacement);
    if (result != WORLD_AUDIO_OK) return result;
    // PAL1782F0..1782FC: the private cinematic sample bypasses the
    // ordinary categories that this same effect deliberately muffles.
    result = worldAudioSetVoiceCategory(queue, replacement, UINT32_MAX);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice->instance) {
        result = worldAudioRetireVoice(queue, *voice);
        if (result != WORLD_AUDIO_OK) return result;
    }
    result = worldAudioPlayVoice(queue, replacement, gain, 0.0f, pitchScale - 1.0f);
    if (result != WORLD_AUDIO_OK) return result;
    *voice = replacement;
    return WORLD_AUDIO_OK;
}

WorldAudioResult playerCinematicVoiceStop(WorldAudioRequestQueue* queue,
    int32_t enabled, WorldAudioVoiceRef* voice)
{
    WorldAudioResult result;
    if (!enabled) return WORLD_AUDIO_OK;
    if (!voice) return WORLD_AUDIO_INVALID_BINDING;
    if (!voice->instance) return WORLD_AUDIO_OK;
    result = worldAudioRetireVoice(queue, *voice);
    if (result != WORLD_AUDIO_OK) return result;
    voice->instance = NULL;
    voice->generation = 0;
    return WORLD_AUDIO_OK;
}

WorldAudioResult playerCinematicVoiceRejectPreview(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t enabled)
{
    const WorldAudioCue* cue;
    if (!enabled) return WORLD_AUDIO_OK;
    if (!manager || !manager->frontendBank) return WORLD_AUDIO_INVALID_BINDING;
    cue = worldAudioFindInBank(manager->frontendBank, "MenuWrong");
    if (!cue) return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioPlayCue(queue, manager->frontendBank, cue, 1.0f, 0.0f, 0.0f);
}

void carFramePlayerStopStateAudio(CarFramePlayerState* player)
{
    if (!carFramePlayerSoundEnabled(player->registry) || !player->cinematicVoice) return;
    CarFramePlayerServices* services = player->registry->services;
    WorldAudioVoiceRef voice = {player->cinematicVoice, player->cinematicVoiceGeneration};
    if (!services || playerCinematicVoiceStop(services->audio,
        carFramePlayerSoundEnabled(player->registry), &voice) != WORLD_AUDIO_OK) {
        player->result = 1;
        return;
    }
    player->cinematicVoice = voice.instance;
    player->cinematicVoiceGeneration = voice.generation;
}

void carFramePlayerSetCameraSpring(CarFramePlayerState* player,int32_t enabled)
{
    // PAL181E50 stores the live Player settings even without a camera.
    player->settings.cameraSpring=enabled;
    if (player->cameras && player->cameras->current.base &&
        player->cameras->current.base->kind==1)
        player->cameras->current.owner.chase->context.dynamicResponse=enabled;
}
void carFramePlayerSetLookbackSpeed(CarFramePlayerState* player,float speed)
{
    // PAL181E78 only changes the current kind1 DriveCameraMover owner.
    if (player->cameras && player->cameras->current.base &&
        player->cameras->current.base->kind==1)
        player->cameras->current.owner.chase->facingRate=speed;
}

// Full PAL181E48; the signed byte belongs to the existing Player settings.
void carFramePlayerSetDrivingStyle(CarFramePlayerState* player,int32_t style)
{
    player->settings.initialVehicleMode=static_cast<int8_t>(style);
}

#include "indep/inc/TrackedController.hpp"
#include "indep/inc/RealDriver.hpp"
#include <limits>

// Constructor-owned state for Player1751D8. Object references always point
// into the shared Flailer package; there is no second renderer or Car state.
struct CarPlayerOwnedState {
    hp2::PlayerConstructionBindings bindings;
    std::unique_ptr<hp2::NisManager> nis;
    std::unique_ptr<hp2::TrackMap> trackMap;
    int32_t trackMapChangedTicks{};
    CarNisOverlay overlay;
    std::array<std::unique_ptr<hp2::HudAnimator>,20> animations;
    std::unique_ptr<std::byte[]> timingWorkspace;
    std::unique_ptr<float[]> routeWorkspace;
    CarFramePlayerState* otherPlayer{};
    int32_t viewNumber{};
    int32_t overlayViewNumber{};
    uint32_t hudScreen{},nisScreen{},hudFeatures{},blockedFeatures{};
    hp2::FlailerBitmapChunk* letterbox[4]{};
    float letterboxTop{},letterboxBottom{},letterboxTopDelta{},letterboxBottomDelta{};
    hp2::FlailerTextChunk* lapText[5]{};
    hp2::FlailerTextChunk* splitText[3]{};
    hp2::FlailerTextChunk* splitLabels[3]{};
    hp2::FlailerTextChunk* splitSecondPlayer[2]{};
    hp2::FlailerTextChunk* wrongWayText[2]{};
    hp2::FlailerBitmapChunk* positionIcons[4]{};
    hp2::FlailerBitmapChunk* tracker[7]{};
    hp2::FlailerBitmapChunk* wantedIcons[12]{};
    hp2::FlailerBitmapChunk* damageLights[5]{};
    hp2::FlailerBitmapChunk* pursuitCrosses[4]{};
    hp2::FlailerBitmapChunk* chopperIcons[3]{};
    hp2::FlailerBitmapChunk* copCarIcons[3]{};
    hp2::FlailerTextChunk* pursuitTime{};
    hp2::FlailerTextChunk* liveStatus[4]{};
    hp2::FlailerBitmapChunk* liveSwatch[4]{};
    hp2::FlailerTextChunk* livePosition[4]{};
    hp2::FlailerBitmapChunk* liveBackground[4]{};
    hp2::FlailerBitmapChunk* liveTail{};
    hp2::FlailerBitmapChunk* pursuitBackground{};
    hp2::FlailerBitmapChunk* pursuitIcons[4]{};
    hp2::FlailerBitmapChunk* pursuitLabels[3]{};
    std::array<std::array<float,2>,4> pursuitPositions{};
    int32_t pursuitState{};
    hp2::FlailerTextChunk* eventText[4]{};
    hp2::FlailerTextChunk* secondEventText[4]{};
    hp2::FlailerTextChunk* speedText{};
    hp2::FlailerTextChunk* speedSecondText{};
    hp2::FlailerTextChunk* gearText{};
    hp2::FlailerBitmapChunk* gearIcon{};
    hp2::FlailerBitmapChunk* speedNeedle{};
    hp2::FlailerBitmapChunk* tachometerNeedle{};
    hp2::FlailerBitmapChunk* tachometerBitmap{};
    std::array<hp2::FlailerTachometerPoint,9> tachometerPoints{};
    WorldAudioVoiceRef radarBeep{};
    int32_t flailerMoved{};
    float flailerOriginalTop{};
    int32_t countdownTimer{},lastHudSample{-1},timeout{INT32_MAX};
    int32_t showHud{1},showMap{1};
    float hudScale{0.8999999761581421f};
    // Actual independent PAL HUD cells. Timers are in the source4000Hz
    // domain except trackMapChangedTicks, which is realTimeFrames.
    int32_t lastPosition{},lastSpeed{},speedThreshold{},speedAward{},speedArmed{1};
    float speedAwardFraction{0.8999999761581421f};
    int32_t positionChange{};
    float positionChangedSeconds{},airborneStartedSeconds{};
    int32_t radarTicks{},initialPositionsScored{},lapHadCollision{};
    int32_t collisionChain{},bonusPoints{},lapPenalty{};
    std::array<int32_t,8> passedPositions{};
    bool shutdownDetached{};
    std::exception_ptr shutdownFailure;
};

namespace {
struct PlayerShutdownErrors {
    std::exception_ptr first;
    void Remember(std::exception_ptr error) noexcept {if(!first)first=error;}
    template<class Operation> void Run(Operation&& operation) noexcept {
        try {operation();}catch(...) {Remember(std::current_exception());}
    }
};
void ShutdownPlayerCameras(CarFramePlayerState* player,PlayerShutdownErrors& errors) noexcept;

void PlayerVisible(hp2::FlailerObjectChunk* object,bool visible)
{
    if (!object)return;
    // PAL140728 updates the borrowed object itself. Different authored
    // objects can share a key, including the last two P2 race-over rows.
    // Looking the key up again would update the first row twice and leave
    // the actual target visible with a stale retained strip.
    object->visible=static_cast<std::uint8_t>(visible);
    if (!visible && object->strip)object->strip->pool->Discard(*object);
}

float PlayerObjectTop(CarPlayerOwnedState& own,hp2::FlailerObjectChunk* object)
{
    float x=0.0f,y=0.0f;
    if (object) own.bindings.flailer->GetTopLeft(*object,x,y,own.bindings.language);
    return y;
}
hp2::HudAnimator* PlayerAnimator(CarFramePlayerState& p,int32_t index)
{
    // The source slots10/11 already live in the single score owner.
    if (index==10) return p.scoreState?p.scoreState->amountAnimation.get():nullptr;
    if (index==11) return p.scoreState?p.scoreState->captionAnimation.get():nullptr;
    return p.ownedState?p.ownedState->animations[index].get():nullptr;
}
void PlayerHudGroup(CarPlayerOwnedState& own,uint32_t key,int32_t visible)
{
    auto& ui=*own.bindings.flailer;
    for (auto* object=ui.FindObjectByGroup(own.hudScreen,key,0);object;
        object=ui.FindObjectByGroup(own.hudScreen,key,0,object))
        PlayerVisible(object,visible!=0);
}

bool PlayerTimerActive(int32_t ticks) {return ticks!=0 && ticks!=INT32_MAX;}
const RaceCarEntry* PlayerRaceEntry(const CarFramePlayerState& player)
{
    if (!player.car || !player.car->bindings.owner || !player.registry ||
        !player.registry->services || !player.registry->services->race) return nullptr;
    const auto& race=*player.registry->services->race;
    for (std::size_t i=0;i<race.entryCount;++i)
        if (race.entries[i] && race.entries[i]->car==player.car->bindings.owner)return race.entries[i];
    return nullptr;
}
const CarChaseCamera* PlayerDriveCamera(const CarFramePlayerState& p)
{
    return p.cameras && p.cameras->current.family==CAR_PLAYER_CAMERA_CHASE ?
        p.cameras->current.owner.chase:nullptr;
}
}

// PAL1795F8 calls GetPOV (vtable+3C), then OutsidePOV (vtable+44).
// The latter also recognizes an authored exterior view, independently of
// requested look-back direction. Base cameras without a POV return zero.
uint32_t carFramePlayerGetGaugeMode(const CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState,"GetGaugeMode has no Player");
    const auto* camera=PlayerDriveCamera(*player);
    if (!camera)return 0;
    const auto* view=carCameraViewAt(camera->catalogue,camera->base.services->presentationLayout,
        player->settings.cameraView);
    if (!view)return 0;
    // Playable traffic has no authored dashboard/gauge textures or dial
    // ranges. Use the existing digital speed/gear presentation of this same
    // live Player; never invent a traffic dial or substitute another model.
    if(player->car && player->car->bindings.info &&
       player->car->bindings.info->bodyCategory==2)return 0x10000000u;
    if (carChaseCameraUsesForwardSide(camera))return 0x40;
    const auto mode=player->ownedState->bindings.parameters->mode;
    if (mode==2)return 0x40;
    if (view->kind==0 && mode==1)return 0x20;
    return view->kind==1?0x10000000u:0u;
}

// PAL182318 -> final2289F8. The queried pending/arrest flags belong to the
// eight real Pursuit participant records, not to a copied Player bool.
int32_t carFramePlayerShouldShowBustedText(const CarFramePlayerState* player)
{
    if (!player || !player->registry || !player->registry->services)return 0;
    const auto* pursuit=player->registry->services->pursuit;
    if (!pursuit)return 0;
    return pursuitManagerShouldDisplayBustedText(pursuit);
}

// Complete final PAL178DA0. This derives features from actual state every
// application frame; no caller-supplied mask selects a simplified HUD.
uint32_t carFramePlayerDetermineHudFeatures(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->registry,
        "DetermineHudFeatures has no Player owner");
    auto& own=*player->ownedState;
    if (!player->car || !own.hudScreen)return 0;
    uint32_t mask=0x40000u;
    if (player->movement.steeringFeedbackBusy)return mask;
    auto& registry=*player->registry;
    hp2::Require(registry.services && registry.services->session && registry.services->race &&
        own.bindings.parameters && own.bindings.parameters->runtimeSettings,
        "DetermineHudFeatures has no live race/session settings");
    const auto& parameters=*own.bindings.parameters;
    const auto& display=parameters.runtimeSettings->display;
    const auto& session=*registry.services->session;
    const auto& race=*registry.services->race;
    const auto state=session.state;
    const bool special=parameters.specialPursuit!=0;
    const bool pursuit=parameters.policeDriverCount!=0;
    const bool freeRun=raceParametersIsFreeRun(&parameters)!=0;
    const bool onlyCar=parameters.mode+parameters.opponentCount==1;
    const auto profile=player->settings.inputProfile;
    if (state==8 || state==9 || state==4 || state==7)return 0;
    bool nis=carFramePlayerReplayActive(player)!=0;
    if (!nis && registry.count==2 && player->movement.seat==1 && special && registry.playersByNumber[0])
        nis=carFramePlayerReplayActive(registry.playersByNumber[0])!=0;
    if (nis) {
        if (pursuit && state!=6 && registry.count==1)mask|=0x08000000u;
        if (onlyCar || freeRun || profile==1 || profile==3 || special)mask&=~0x08000200u;
        if (state!=6)mask|=0x200000u;
        if (registry.count==1 || special)mask|=0x02000000u;
        if (registry.count==2 && player->movement.seat==0 && !special && display.splitViewTrackMapMode!=6)mask|=1u;
        return mask;
    }
    if (carFramePlayerShouldShowBustedText(player)) {
        mask&=~0x08000200u;
        if (state!=6)mask|=0x200000u;
        return mask;
    }
    if (race.state==1 || race.state==2)return 0x02000000u;
    if (race.state==3 && registry.count==1)return mask|0x02000000u;
    if (player->drivingLocked && state==3) {
        mask|=4u;
        if (registry.count==2 && player->movement.seat==0 && registry.playersByNumber[1] &&
            !registry.playersByNumber[1]->drivingLocked && display.splitViewTrackMapMode!=6)mask|=1u;
        return mask;
    }
    if (state==6) {
        if (session.stateBeforePause==1 || session.stateBeforePause==2)mask|=0x02000000u;
        if (own.trackMapChangedTicks) {
            if (registry.clock->realTimeFrames-own.trackMapChangedTicks<60)mask|=1u;
            else own.trackMapChangedTicks=0;
        }
        return mask;
    }
    if (player->movement.steeringFeedbackBusy==1 || player->movement.steeringFeedbackBusy==2)
        return mask|0x2000u;
    switch(profile) {
    case 2:
        if (pursuit && !special && !player->drivingLocked)mask|=0x04000000u;
        break;
    case 1:
        if (pursuit && !special)mask|=0x04000000u;
        if (registry.count==1)mask|=1u;
        break;
    case 3:
        if (pursuit && !special)mask|=0x04000000u;
        mask|=carFramePlayerGetGaugeMode(player);
        break;
    case 0: {
        mask|=0x100000u;
        const auto* drive=PlayerDriveCamera(*player);
        // PAL1791B4/B8 selects LookingBack (+4C), not OutsidePOV (+44).
        // The other actual camera families inherit the verified zero getter.
        if (drive && carChaseCameraHasRearFacing(drive))mask&=~0x100000u;
        const auto mapMode=special?display.pursuitTrackMapMode:
            (registry.count==2?display.splitViewTrackMapMode:display.singleViewTrackMapMode);
        if (mapMode!=6)mask|=1u;
        if (special && player->movement.seat==0) {
            mask&=~0x400000u;
            if (drive && drive->facingTarget==1.0f)mask|=0x400000u;
        }
        const auto settings=player->settings.hudFlags;
        if (settings&0x100u)mask|=0x508u;
        if (settings&4u)mask|=0x80u;
        if (settings&0x10u)mask|=0x200u;
        if ((settings&0x200u) && registry.count==1 && !special &&
            !PlayerTimerActive(parameters.timeLimitTicks) && !freeRun && display.leaderboardMode)mask|=0x800000u;
        if (display.authoredOption0 && !player->lookBack)mask|=0x1000u;
        if ((parameters.raceKind==2 && !parameters.pointToPoint) || onlyCar || freeRun)mask&=~0x200u;
        if (pursuit && !special) {
            mask|=(settings&0x100u)?0x8000u:0x04000000u;
            if (!player->lookBack)mask|=0x80000u;
        }
        if (special) {
            mask|=0x01000000u;
            if (player->movement.seat==0)mask&=~0x788u;
            if (player->movement.seat==1)mask&=~0x380u;
        }
        mask|=0x20000u;
        if (freeRun || (special && registry.count==2) || !player->settings.scoring)mask&=~0x20000u;
        if (settings&0x40u)mask|=carFramePlayerGetGaugeMode(player);
        break;
    }
    default:break;
    }
    if (special)mask&=~0x04008200u;
    if (registry.count==2) {
        if (mask&0x04000000u)mask=(mask&~0x04000000u)|0x8000u;
        mask&=~0x100000u;
        if (player->movement.seat==1)mask&=~1u;
    }
    if (player->wrongWay)mask|=0x4000u;
    if (PlayerTimerActive(player->recoveryProtectionTicks))mask|=0x10000u;
    return mask;
}

// PAL17EF60. The source Race's finish checkpoint selects the best-time
// array slot; it is not implicitly the start/finish slot zero.
int32_t carFramePlayerCurrentBestLapRecord(const CarFramePlayerState* player,int32_t previous)
{
    hp2::Require(player && player->ownedState,"Best lap query has no Player");
    const auto& parameters=*player->ownedState->bindings.parameters;
    int32_t best=parameters.pointToPoint?player->settings.bestTotalTimeTicks:player->settings.bestLapTimeTicks;
    if (parameters.raceKind==2 || parameters.pointToPoint)return best;
    const auto* entry=PlayerRaceEntry(*player);
    if (!entry)return best;
    const auto checkpoint=player->registry->services->race->finishCheckpoint;
    hp2::Require(checkpoint>=0 && checkpoint<5,"Best lap checkpoint is invalid");
    const int32_t current=previous?entry->previousBestSegmentTicks[checkpoint]:entry->bestSegmentTicks[checkpoint];
    if (PlayerTimerActive(current) && (!PlayerTimerActive(best) || current<best))best=current;
    return best;
}

float carFramePlayerDisplaySpeed(const CarFramePlayerState* player,float metresPerSecond)
{
    hp2::Require(player && player->ownedState && player->ownedState->bindings.parameters->runtimeSettings,
        "DisplaySpeed has no live unit setting");
    const auto units=player->ownedState->bindings.parameters->runtimeSettings->display.languageDependentOption;
    return metresPerSecond*(units==1?3.5999999046325684f:2.236999988555908f);
}

// PAL17E2D0 dispatches the actual final compiled score descriptors23..25.
int32_t carFramePlayerCalcBonusPoints(const CarFramePlayerState* player,int32_t finishPosition)
{
    hp2::Require(player,"Bonus points has no Player");
    if (finishPosition<1 || finishPosition>3)return 0;
    return carFramePlayerScoreStringValue(playerScoreDefinitions[22+finishPosition].amount);
}

void carFramePlayerEndPause(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState,"EndPause has no Player");
    player->joystickType=joystickGetType(player->ownedState->bindings.input,player->movement.controllerPort);
}

void carFramePlayerDetermineAllInputMaps(CarFramePlayerRegistry* registry)
{
    hp2::Require(registry && registry->count>=0 && registry->count<=2,"Player input registry is invalid");
    for (int32_t i=0;i<registry->count;++i)carFramePlayerReconcileInput(registry->players[i]);
}

// Final180E60 and181D88. Input disconnection requests the real coordinator
// pause action. Camera selection retains the same concrete Player owners.
static void UpdatePlayerHud(CarFramePlayerState* player, bool renderHud);

static void UpdatePlayerNonGameState(CarFramePlayerState* player, bool renderHud)
{
    hp2::Require(player && player->ownedState && player->registry && player->registry->services &&
        player->registry->services->session && player->registry->services->race,
        "Player non-game frame has no live session");
    auto& own=*player->ownedState;
    auto& registry=*player->registry;
    auto& session=*registry.services->session;
    const auto type=joystickGetType(own.bindings.input,player->movement.controllerPort);
    if (session.state==3) {
        if (!joystickIsPluggedIn(own.bindings.input,player->movement.controllerPort) || type!=player->joystickType)
            raceCoordinatorRequestPause(&session,player->movement.controllerPort);
    } else player->joystickType=type;
    if (session.state==3 && player->visibleDrivingLine) {
        auto& line=*player->visibleDrivingLine;
        const auto elapsed=playerFrameSignedTicks(static_cast<uint32_t>(registry.clock->worldTimer)-
            static_cast<uint32_t>(line.startedTicks));
        if (line.duration<=static_cast<float>(elapsed)*0.0002500000118743628f)
            carFramePlayerDisposeVisibleDrivingLine(player);
    }
    if (session.state==3) {
        hp2::Require(player->cameras,"Player non-game frame has no camera owner");
        auto& camera=*player->cameras;
        const bool anyCamera=camera.current.base || camera.scripted || camera.overlay ||
            camera.countdown || camera.replay;
        if (!anyCamera && !carFramePlayerReplayActive(player)) {
            const auto* other=registry.playersByNumber[player==registry.playersByNumber[0]?1:0];
            CarCinematicResult result=CAR_CINEMATIC_OK;
            if (registry.services->race->state==3 && !own.bindings.parameters->specialPursuit)
                result=carFramePlayerSetCountdownCamera(player);
            else if (!own.bindings.parameters->specialPursuit || !other || !carFramePlayerReplayActive(other))
                result=carFramePlayerChangeCamera(player,1,0);
            hp2::Require(result==CAR_CINEMATIC_OK,"Player frame camera construction failed");
        }
        if (camera.countdown && !carViewMoverCurrentBase(camera.countdown))
            hp2::Require(carFramePlayerChangeCamera(player,1,0)==CAR_CINEMATIC_OK,
                "Player countdown camera transition failed");
    }
    UpdatePlayerHud(player,renderHud);
}

void carFramePlayerUpdateNonGameState(CarFramePlayerState* player)
{
    UpdatePlayerNonGameState(player,true);
}

void carFramePlayerUpdateNonGameStateAll(CarFramePlayerRegistry* registry)
{
    hp2::Require(registry && registry->count>=0 && registry->count<=2,"Player frame registry is invalid");
    for (int32_t i=0;i<registry->count;++i)carFramePlayerUpdateNonGameState(registry->players[i]);
}

void hp2::AdvancePlayersWithoutFlailerHud(CarFramePlayerRegistry* registry)
{
    Require(registry && registry->count>=0 && registry->count<=2,"Player frame registry is invalid");
    for(int32_t i=0;i<registry->count;++i)UpdatePlayerNonGameState(registry->players[i],false);
}

namespace {
void PlayerDisplayDebugInfo(CarFramePlayerState& player)
{
    // Complete PAL181230..181498 formats the same four authored objects.
    auto& own=*player.ownedState;auto& resources=*own.bindings.flailer;
    for(const uint32_t key:{0xFA10332Fu,0x2DF92633u,0xD8F8BD54u,0xF8C20C4Au})
        resources.SetObjectVisibility(own.hudScreen,key,displayPlayerDebugInfo!=0);
    if(!displayPlayerDebugInfo)return;
    // PC rates can exceed the authored 10/11-character buffers. Only these
    // two decoded text buffers grow; source objects, layout and formats stay
    // unchanged. 32 characters cover the full signed int32 rate and suffix.
    for(const uint32_t key:{0xFA10332Fu,0x2DF92633u})
        if(auto* object=resources.FindObject(own.hudScreen,key,1)) {
            auto& text=static_cast<hp2::FlailerTextChunk&>(*object);
            if(text.textCapacity<32u) {
                text.ownedText.resize(33u,0);text.text=text.ownedText.data();
                text.textCapacity=32u;
            }
        }
    hp2::Require(player.car && player.car->bindings.state,"Debug HUD has no actual Car state");
    const auto& state=*player.car->bindings.state;
    resources.Printf(own.hudScreen,0xFA10332Fu,"C: %dfps",convertFloatToInt32(applicationPreviousCpuFrameRate));
    if(applicationGpuTimingAvailable)
        resources.Printf(own.hudScreen,0x2DF92633u,"G: %dfps",convertFloatToInt32(applicationPreviousGpuFrameRate));
    else resources.Printf(own.hudScreen,0x2DF92633u,"G: n/a");
    resources.Printf(own.hudScreen,0xD8F8BD54u,"%.0f %.0f %.0f s%d",
        static_cast<double>(state.position[0]),static_cast<double>(state.position[1]),
        static_cast<double>(state.position[2]),sceneryCurrentZoneNumber());
    resources.Printf(own.hudScreen,0xF8C20C4Au,"%f km",static_cast<double>(state.speedDistance*0.0010000000474974513f));
}

// Final17CFB8/17D130: borrowed objects, one animator for each source slot.
void PlayerMoveText(CarFramePlayerState& player,int32_t slot,hp2::FlailerTextChunk& text,
    const std::array<float,2>& from,const std::array<float,2>& to,
    const std::array<float,2>& scale,const std::array<float,2>& endScale,
    int32_t alpha,int32_t endAlpha,float seconds)
{
    hp2::Require(slot>=0 && slot<20 && slot!=10 && slot!=11,"HUD text animation slot is invalid");
    auto& animation=player.ownedState->animations[slot];
    if (!animation)animation=std::make_unique<hp2::HudAnimator>();
    const hp2::HudMoveAnimation request{from,{to[0]-from[0],to[1]-from[1]},
        scale,{endScale[0]-scale[0],endScale[1]-scale[1]},alpha,
        static_cast<float>(endAlpha-alpha),seconds};
    animation->StartAnimation(text,request,*player.registry->clock);
}

void PlayerMoveBitmap(CarFramePlayerState& player,int32_t slot,hp2::FlailerBitmapChunk& bitmap,
    const std::array<float,2>& from,const std::array<float,2>& to,
    const std::array<float,2>& scale,const std::array<float,2>& endScale,
    int32_t alpha,int32_t endAlpha,float seconds,bool keepVisible)
{
    hp2::Require(slot>=0 && slot<20 && slot!=10 && slot!=11,"HUD bitmap animation slot is invalid");
    auto& animation=player.ownedState->animations[slot];
    if (!animation)animation=std::make_unique<hp2::HudAnimator>();
    const hp2::HudMoveAnimation request{from,{to[0]-from[0],to[1]-from[1]},
        scale,{endScale[0]-scale[0],endScale[1]-scale[1]},alpha,
        static_cast<float>(endAlpha-alpha),seconds};
    animation->StartAnimation(bitmap,request,*player.registry->clock,keepVisible);
}

// Final17CB18/17CD30, complete two-direction branches. The selected text
// already contains the value formatted by RenderHud; the source fourth
// argument is the clock, not a second caption owner.
void PlayerGrowText(CarFramePlayerState& player,hp2::FlailerTextChunk* text,
    const hp2::FlailerTextChunk* reference,bool improved)
{
    if (!text || !reference)return;
    const std::array<float,2> position{reference->topLeftX,reference->topLeftY};
    const std::array<float,2> scale{reference->scaleX,reference->scaleY};
    if (improved)PlayerMoveText(player,0,*text,position,{position[0],position[1]-30.0f},
        scale,{2.0f,2.0f},reference->GetAlpha(),0,0.5f);
    else PlayerMoveText(player,0,*text,position,{position[0],position[1]+100.0f},
        scale,{0.5f,0.5f},0,reference->GetAlpha(),1.0f);
}

void PlayerSpeedText(CarFramePlayerState& player,hp2::FlailerTextChunk* text,
    const hp2::FlailerTextChunk* reference,bool improved)
{
    if (!player.ownedState->speedText || !text || !reference)return;
    auto& own=*player.ownedState;
    const std::array<float,2> position{reference->topLeftX,reference->topLeftY};
    const std::array<float,2> scale{reference->scaleX,reference->scaleY};
    if (improved) {
        const float width=own.bindings.flailer->ScaledTextWidth(*reference,own.bindings.language);
        const float height=own.bindings.flailer->ScaledTextHeight(*reference);
        PlayerMoveText(player,1,*text,position,{position[0]-width*0.5f,position[1]-height*0.5f},
            scale,{2.0f,2.0f},reference->GetAlpha(),0,1.0f);
    } else PlayerMoveText(player,1,*text,position,{position[0],position[1]+100.0f},
        scale,{0.5f,0.5f},reference->GetAlpha(),0,1.0f);
}

// Final17D7C8 owns the five-second score-title visibility envelope.
void PlayerScoreTitleAlpha(CarFramePlayerState& player)
{
    auto& own=*player.ownedState;
    if (player.scoreState->lastStartedTicks==INT32_MAX || !own.eventText[2] || !own.eventText[3])return;
    const auto elapsed=playerFrameSignedTicks(static_cast<uint32_t>(player.registry->clock->worldTimer)-
        static_cast<uint32_t>(player.scoreState->lastStartedTicks));
    const float seconds=static_cast<float>(elapsed)*0.0002500000118743628f;
    if (!(seconds>0.0f))return;
    if (seconds>5.0f) {
        PlayerVisible(own.eventText[2],false);PlayerVisible(own.eventText[3],false);
        player.scoreState->lastStartedTicks=INT32_MAX;
        return;
    }
    const int32_t alpha=seconds<=0.5f?convertFloatToInt32(seconds*2.0f*128.0f):
        (seconds<=4.5f?128:convertFloatToInt32((1.0f-(seconds-4.5f)*2.0f)*128.0f));
    for (auto* text:{own.eventText[2],own.eventText[3]}) {
        PlayerVisible(text,true);text->SetAlpha(alpha);
    }
}

// Final17E558 consumes the custom amount already stored by its caller.
void PlayerStartVariableScore(CarFramePlayerState& player,int32_t score,int32_t amount)
{
    auto& state=*player.scoreState;
    hp2::Require(score>=0 && static_cast<size_t>(score)<playerScoreDefinitions.size(),"Variable score index is invalid");
    std::snprintf(state.amount.data(),state.amount.size(),"+%d",amount);
    const auto audio=worldAudioPlayScore(player.registry->services->audio,player.registry->services->session,score);
    if (audio!=WORLD_AUDIO_OK)player.result=1;
    playerScoreCopy(state.caption,state.language->GetString(static_cast<int32_t>(playerScoreDefinitions[score].languageId)));
    const CarPlayerScoreAnimation animation{{530.0f,236.0f},{0,0},{90.0f,-100.0f},
        {0.699999988079071f,0.699999988079071f},{0,0},{128.0f,64.0f,0,0},{0,64.0f,128.0f,0}};
    carFramePlayerStartScoreAnimation(&player,&animation);
}
}

// Final17E820: actual lap, collision, landing, speed and overtake scoring.
// The collision source dispatch is GetMassInverse, confirmed independently
// from every concrete beta vtable+9C and the released callsite protocol.
void carFramePlayerUpdateScoring(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->car && player->car->bindings.state &&
        player->car->bindings.ownerMotion,"Player scoring has no actual car");
    auto& own=*player->ownedState;auto& movement=*player->car;
    const auto& parameters=*own.bindings.parameters;
    const auto& clock=*player->registry->clock;
    const auto* entry=PlayerRaceEntry(*player);
    if (carMotionWindowActive(movement.bindings.ownerMotion,clock.worldTimer)) {
        own.airborneStartedSeconds=0.0f;carGlareResetRequested=1;return;
    }
    if (entry && !entry->completionReason && entry->advancedLap) {
        const bool timed=parameters.raceKind==2 && PlayerTimerActive(parameters.timeLimitTicks) && !parameters.specialPursuit;
        if (!timed && !parameters.pointToPoint) {
            if (!own.lapHadCollision) {carFramePlayerStartScore(player,3);++own.bonusPoints;}
            else if (entry->position==1)carFramePlayerStartScore(player,entry->previousPosition==1?4:5);
            else if (!own.lapPenalty && entry->previousPosition>0)carFramePlayerStartScore(player,7);
            own.lapPenalty=0;own.lapHadCollision=0;
        } else {if (!own.lapPenalty)carFramePlayerStartScore(player,7);own.lapPenalty=0;}
    }
    if (own.speedAward)carFramePlayerStartScore(player,6);
    hp2::Require(movement.collisionSystem && movement.collisionParticipant && movement.collisionParticipant->shape,
        "Player scoring has no canonical collision participant");
    const auto* impact=collisionShapeContactAt(movement.collisionSystem,movement.collisionParticipant->shape,0);
    if (impact) {
        Car* first=nullptr;Car* second=nullptr;collisionImpactGetCars(impact,&first,&second);
        int32_t side=-1;
        if (first==movement.bindings.owner)side=0;
        if (second==movement.bindings.owner)side=1;
        if (side<0)own.collisionChain=0;
        else {
            const auto* other=side==0?impact->pair->second:impact->pair->first;
            const float inverseMass=collisionParticipantInverseMass(movement.collisionSystem,other);
            const auto* up=movement.bindings.state->orientation+8;
            const float vertical=(impact->direction[0]*up[0]+impact->direction[1]*up[1])+impact->direction[2]*up[2];
            if (inverseMass<=500.0f && own.airborneStartedSeconds==0.0f &&
                vertical<0.800000011920929f && -0.800000011920929f<vertical && vertical!=0.0f &&
                8.33329963684082f<impact->tangentialSpeed) {
                own.lapHadCollision=1;own.lapPenalty=1;player->undamagedRace=0;
                own.collisionChain=collisionImpactInvolvesSmokeable(impact)?1:0;
            }
        }
    }
    const float nowSeconds=static_cast<float>(clock.worldTimer)*0.0002500000118743628f;
    if (!movement.bindings.state->activeContactCount) {
        if (own.airborneStartedSeconds==0.0f)own.airborneStartedSeconds=nowSeconds;
    } else {
        if (own.airborneStartedSeconds!=0.0f) {
            int32_t points=convertFloatToInt32((nowSeconds-own.airborneStartedSeconds)*250.0f);
            points=(points/50)*50;
            if (points>=100 && points<200)PlayerStartVariableScore(*player,11,points);
            if (points>=200 && points<350)PlayerStartVariableScore(*player,12,points);
            if (points>=350)PlayerStartVariableScore(*player,13,points);
        }
        own.airborneStartedSeconds=0.0f;
    }
    if (own.positionChange>0) {
        hp2::Require(own.lastPosition>0 && own.lastPosition<=8,"Player scoring position is invalid");
        auto& scored=own.passedPositions[static_cast<size_t>(own.lastPosition-1)];
        if (!scored && !parameters.specialPursuit) {
            carFramePlayerStartScore(player,own.lastPosition+15);scored=1;
        }
    }
    own.positionChange=0;
}

// Final17ECF0. The port-side state-only path retains scoring and animation.
static void UpdatePlayerHud(CarFramePlayerState* player, bool renderHud)
{
    hp2::Require(player && player->ownedState && player->registry && player->registry->services,
        "UpdateHud has no actual Player");
    auto& own=*player->ownedState;
    PlayerDisplayDebugInfo(*player);
    const uint32_t requested=carFramePlayerDetermineHudFeatures(player);
    if (requested!=own.hudFeatures)
        hp2::Require(carFramePlayerSetHudFeatures(player,requested)!=0,"Original HUD feature dependency is not available");
    carFramePlayerHudDoAnimations(player);PlayerScoreTitleAlpha(*player);
    const auto* race=player->registry->services->race;
    const auto* session=player->registry->services->session;
    hp2::Require(session,"UpdateHud has no live session");
    const bool paused=session->state==4 || session->state==7 || session->state==5 ||
        hp2::IsGameFlowPaused(session);
    if (race && race->state==4 && session->state!=6 && !paused) {
        if (!own.initialPositionsScored) {
            if (const auto* entry=PlayerRaceEntry(*player)) {
                const int32_t count=static_cast<int32_t>(race->entryCount);
                hp2::Require(count>=1 && count<=8 && entry->position>=1 && entry->position<=count,
                    "Initial Player scoring has an invalid race position");
                if (player->registry->count==2 && !raceParametersGetNumEliminated(own.bindings.parameters)) {
                    const int32_t last=count-1;
                    int32_t effective=entry->position-1;
                    if (effective==last)effective=entry->position-2;
                    for (int32_t i=0;i<last-effective;++i)own.passedPositions[static_cast<size_t>(last-i-1)]=1;
                } else {
                    int32_t total=0;
                    for (int32_t i=0;i<count-entry->position;++i) {
                        total+=carFramePlayerScoreStringValue(playerScoreDefinitions[static_cast<size_t>(count+14-i)].amount);
                        own.passedPositions[static_cast<size_t>(count-2-i)]=1;
                    }
                    if (total)PlayerStartVariableScore(*player,15,total);
                }
            }
            own.initialPositionsScored=1;
        }
        carFramePlayerUpdateScoring(player);
    }
    if (renderHud && own.hudFeatures)carFramePlayerRenderHud(player);
}

void carFramePlayerUpdateHud(CarFramePlayerState* player)
{
    UpdatePlayerHud(player,true);
}

namespace {
// Released writable Player.cpp global330720 (beta LiveUpdateSplitTimeRate).
// Zero freezes a displayed split after its first0.2 seconds; nonzero is an
// explicit WorldLoopCounter interval, not a wall-clock approximation.
int32_t playerLiveUpdateSplitTimeRate=0;

float PlayerGaugeAngle(float first,float last,float fraction)
{
    const float arc=first<last?last-first:(360.0f-first)+last;
    float angle=first+fraction*arc;
    if (angle>360.0f)angle-=360.0f;
    if (angle<0.0f)angle=360.0f-angle;
    return angle;
}

void PlayerFormatTime(CarPlayerOwnedState& own,hp2::FlailerTextChunk* text,int32_t ticks,int32_t flags=0)
{
    if (!text)return;
    char buffer[GAME_TIMER_TEXT_CAPACITY];
    gameTimerPrintToString(&ticks,buffer,flags,&own.bindings.language->currentLanguage);
    text->SetString(buffer);
}

// Final178B70, all ten entries of the final2FBAF8 table, including automatic
// and the exact source X result for an unknown published gear.
char PlayerGearCharacter(int32_t gear)
{
    static constexpr int8_t gears[]={-1,0,1,2,3,4,5,6,7,8};
    static constexpr char letters[]={'A','R','N','1','2','3','4','5','6','7'};
    for (size_t i=0;i<10;++i)if (gears[i]==gear)return letters[i];
    return 'X';
}

void PlayerPursuitMeter(CarFramePlayerState& player,int32_t level)
{
    auto& own=*player.ownedState;
    if (!own.pursuitBackground)return;
    hp2::Require(level>=0 && level<=4,"Pursuit HUD level is out of range");
    int32_t pullovers=0;
    hp2::Require(pursuitManagerGetPulloverCount(player.registry->services->pursuit,
        player.car->bindings.owner,&pullovers)==WORLD_SERVICES_OK,"Pursuit HUD has no racer row");
    pullovers=std::min(3,pullovers);
    if (raceParametersIsFreeRun(own.bindings.parameters))pullovers=0;
    for (int32_t i=0;i<3;++i)PlayerVisible(own.pursuitLabels[i],i<pullovers);
    if (level==own.pursuitState)return;
    if (level>own.pursuitState) {
        for (int32_t i=own.pursuitState;i<level;++i)if (own.pursuitIcons[i]) {
            const auto& to=own.pursuitPositions[static_cast<size_t>(i)];
            PlayerMoveBitmap(player,2+i,*own.pursuitIcons[i],{to[0]-300.0f,to[1]},to,
                {4.0f,4.0f},{1.0f,1.0f},0,128,1.5f,true);
        }
    } else {
        static constexpr float yChanges[4]={-50.0f,50.0f,-50.0f,50.0f};
        const int32_t lowest=std::max(0,level-1);
        for (int32_t i=own.pursuitState-1;i>=lowest;--i)if (own.pursuitIcons[i]) {
            const auto& from=own.pursuitPositions[static_cast<size_t>(i)];
            PlayerMoveBitmap(player,2+i,*own.pursuitIcons[i],from,{from[0],from[1]+yChanges[i]},
                {1.0f,1.0f},{1.0f,1.0f},128,0,1.0f,true);
        }
    }
    own.pursuitState=level;
}

void PlayerRadarDetector(CarFramePlayerState& player)
{
    auto& own=*player.ownedState;
    int32_t activePursuit=0;
    const float intensity=pursuitManagerRadarIntensity(player.registry->services->pursuit,
        player.car->bindings.owner,&activePursuit);
    const int32_t level=std::min(10,convertFloatToInt32(intensity*11.0f));
    static constexpr uint32_t palette[4][12]={
        {0xFF008800,0xFF008800,0xFF008800,0xFF008800,0xFF008800,0xFF004488,0xFF004488,0xFF004488,0xFF000088,0xFF000088,0xFF000088,0xFF000088},
        {0x80004000,0x80004000,0x80004000,0x80004000,0x80004000,0x80002240,0x80002240,0x80002240,0x80000040,0x80000040,0x80000040,0x80000040},
        {0xFF008888,0xFF008888,0xFF008888,0xFF008888,0xFF008888,0xFF004488,0xFF004488,0xFF004488,0xFF000088,0xFF000088,0xFF000088,0xFF000088},
        {0x80004040,0x80004040,0x80004040,0x80004040,0x80004040,0x80002240,0x80002240,0x80002240,0x80000040,0x80000040,0x80000040,0x80000040}};
    const int32_t bank=activePursuit?2:0;
    for (int32_t i=0;i<12;++i)if (own.wantedIcons[i])own.wantedIcons[i]->SetColor(palette[bank+(i<level?0:1)][i]);
    for (int32_t i=0;i<2;++i) {
        auto* bitmap=own.wantedIcons[10+i];
        if (level>=9+i && bitmap && own.wantedIcons[8+i]) {
            const auto* reference=own.wantedIcons[8+i];
            const std::array<float,2> from{reference->topLeftX,reference->topLeftY};
            PlayerMoveBitmap(player,16+i,*bitmap,from,{from[0]-10.0f,from[1]-10.0f},
                {1.0f,1.0f},{2.0f,2.0f},128,0,0.20000000298023224f,false);
        } else PlayerVisible(bitmap,false);
    }
    auto* audio=player.registry->services->audio;
    hp2::Require(audio && audio->spatialEnabled,"Radar has no physical audio owner");
    if (*audio->spatialEnabled && level>0) {
        own.radarTicks=playerFrameSignedTicks(static_cast<uint32_t>(own.radarTicks)+
            static_cast<uint32_t>(convertFloatToInt32(player.registry->clock->worldTimeElapsed*4000.0f)));
        const float interval=static_cast<float>(level)*-0.038333334028720856f+0.5f;
        if (interval<=static_cast<float>(own.radarTicks)*0.0002500000118743628f) {
            own.radarTicks=0;
            hp2::Require(worldAudioPlayVoice(audio,own.radarBeep,1.0f,0.0f,0.0f)==WORLD_AUDIO_OK,
                "Radar physical voice playback failed");
        }
    } else own.radarTicks=0;
}
}

// Released17F040. Flailer objects are borrowed from the original authored
// screen; every value below comes from the live Car/Race/Player owners.
void carFramePlayerRenderHud(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->car && player->registry &&
        player->registry->services && player->registry->services->race,"RenderHud has no live race");
    auto& own=*player->ownedState;auto& registry=*player->registry;
    const auto& parameters=*own.bindings.parameters;const auto& settings=*parameters.runtimeSettings;
    const auto& clock=*registry.clock;const auto& race=*registry.services->race;
    auto& ui=*own.bindings.flailer;
    const uint32_t mask=own.hudFeatures;
    CarMovement* display=player->car;
    if (player->cameras && player->cameras->context && player->cameras->context->chaseSettings) {
        const auto& camera=*player->cameras->context->chaseSettings;
        if (!camera.useAttachedCar)if (auto* selected=worldGetCarByDriverNumber(registry.world,camera.selectedDriverNumber))
            display=static_cast<CarMovement*>(selected->movement);
    }
    hp2::Require(display && display->bindings.state,"RenderHud display CarState is absent");
    const auto& state=*display->bindings.state;
    const RaceCarEntry* entry=nullptr;
    for (size_t i=0;i<race.entryCount;++i)if (race.entries[i]->car==display->bindings.owner)entry=race.entries[i];
    auto write=[&](hp2::FlailerTextChunk* text,const char* format,auto... args) {
        if (text) {char buffer[128];std::snprintf(buffer,sizeof(buffer),format,args...);text->SetString(buffer);}
    };
    if (mask&0x10000060u) {
        hp2::Require(own.bindings.cars && player->car->bindings.info,"HUD gauges have no original CarInfo");
        const auto& car=own.bindings.cars->Car(static_cast<size_t>(player->car->bindings.info->modelCode));
        const auto& gauges=car.gauges;
        const size_t units=settings.display.languageDependentOption==1?1u:0u;
        const float speed=carFramePlayerDisplaySpeed(player,state.forwardSpeed);
        const int32_t speedInt=convertFloatToInt32(speed);
        own.speedAward=0;
        if(car.bodyCategory!=2) {
            const auto& speedRange=gauges.speedometerRange[units];
            const auto& angles=gauges.speedometerAngles[units];
            hp2::Require(speedRange[1]!=0.0f && speedRange[1]!=speedRange[0],"Authored gauge speed range is degenerate");
            if (!own.speedThreshold)own.speedThreshold=convertFloatToInt32(own.speedAwardFraction*speedRange[1]);
            const float ratio=speed/speedRange[1];
            if (ratio<0.699999988079071f)own.speedArmed=1;
            if (own.speedArmed && !parameters.specialPursuit && speedInt<own.lastSpeed &&
                own.speedThreshold<own.lastSpeed && own.speedAwardFraction<ratio) {
                own.speedAwardFraction=std::min(1.0f,own.speedAwardFraction+0.10000000149011612f);
                own.speedArmed=0;own.speedAward=1;
            }
            if (own.speedNeedle)own.speedNeedle->SetRotation(PlayerGaugeAngle(angles[0],angles[1],
                std::clamp((speed-speedRange[0])/(speedRange[1]-speedRange[0]),0.0f,1.0f)));
        }
        if (own.speedText) {
            if (!(mask&0x10000000u))for (int32_t threshold=100;threshold<=400;threshold+=50)
                if (own.lastSpeed<threshold && static_cast<float>(threshold)<=speed) {
                    write(own.speedSecondText,"%d",convertFloatToInt32(std::fabs(speed)));
                    PlayerSpeedText(*player,own.speedSecondText,own.speedText,true);break;
                }
            write(own.speedText,"%d",convertFloatToInt32(std::fabs(speed)));
        }
        own.lastSpeed=speedInt;
        const float rpm=carEngineSoundGlobals.overrideRpm==100.0f?state.engineRpm:carEngineSoundGlobals.overrideRpm;
        hp2::Require(car.engineInfo.maximumRpm!=0.0f,"Authored engine maximum RPM is zero");
        const float fraction=std::clamp(rpm/car.engineInfo.maximumRpm,0.0f,1.0f);
        const float first=(mask&0x20u)?gauges.tachometerAngles[0]:45.0f;
        const float last=(mask&0x20u)?gauges.tachometerAngles[1]:215.0f;
        if (own.gearIcon) {
            PlayerVisible(own.gearIcon,player->settings.gearInputMode==1);
            if (player->settings.gearInputMode==1)own.gearIcon->SetColor(
                car.engineInfo.redlineRpm<=rpm && state.gear>0?0x80008000u:0x80002000u);
        }
        if (own.tachometerNeedle) {
            own.tachometerNeedle->SetRotation(PlayerGaugeAngle(first,last,fraction));
            if (own.tachometerBitmap)own.tachometerBitmap->SetRotation(first+
                static_cast<float>((convertFloatToInt32(car.engineInfo.redlineRpm)/500)*500)/car.engineInfo.maximumRpm*(last-first));
        }
        if (own.gearText) {
            const char text[2]={PlayerGearCharacter(state.gear),0};own.gearText->SetString(text);
            own.gearText->SetColor(state.clutchEngaged?0x00808080u:0x00404040u);
        }
    } else own.lastSpeed=999;
    hp2::Require(race.previousCheckpoint>=0 && race.previousCheckpoint<5 &&
        race.finishCheckpoint>=0 && race.finishCheckpoint<5,"HUD race checkpoints are invalid");
    if ((mask&0x408u) && entry && own.splitSecondPlayer[0]) {
        const auto* other=own.otherPlayer?PlayerRaceEntry(*own.otherPlayer):nullptr;
        hp2::Require(other && own.splitSecondPlayer[1],"Split HUD has no other Player entry");
        int32_t ticks=0;
        hp2::Require(raceFrameCalculateSplitTime(&race,&ticks,other,entry)==WORLD_SERVICES_OK,"Split HUD calculation failed");
        char text[GAME_TIMER_TEXT_CAPACITY];gameTimerPrintToString(&ticks,text,2,&own.bindings.language->currentLanguage);
        own.splitSecondPlayer[1]->SetString(text);own.splitSecondPlayer[1]->SetFormatOptions(64);
        own.splitSecondPlayer[0]->SetLanguageID(text[0]=='+'?0x18E73u:0x18E74u);
    }
    if ((mask&8u) && entry && own.splitText[1]) {
        const auto started=entry->checkpointTicks[race.previousCheckpoint];
        int32_t ticks=PlayerTimerActive(started)?clock.worldTimer-started:0;
        if (PlayerTimerActive(ticks) && PlayerTimerActive(parameters.timeLimitTicks)) {
            const auto prior=entry->segmentTicks[race.finishCheckpoint];
            if (static_cast<float>(ticks)*0.0002500000118743628f<3.0f && PlayerTimerActive(prior))ticks=parameters.timeLimitTicks-prior;
            else ticks=parameters.timeLimitTicks-ticks;
            if (ticks<0) {ticks=-ticks;own.splitText[1]->SetColor(0x400535BEu);}
            else if (static_cast<float>(ticks)*0.0002500000118743628f<10.0f) {
                hp2::Require(soundPlayTimerCountdown(registry.services->audio,&ticks)==WORLD_AUDIO_OK,
                    "HUD timer warning audio request failed");
                own.splitText[1]->SetColor(0x4004D5E2u);
            } else own.splitText[1]->SetColor(0x402AAE01u);
        }
        PlayerFormatTime(own,own.splitText[1],ticks);
    }
    if ((mask&0x100u) && own.splitText[2]) {
        const int32_t best=carFramePlayerCurrentBestLapRecord(player,0);
        if (PlayerTimerActive(best))PlayerFormatTime(own,own.splitText[2],best);
        else own.splitText[2]->SetString("--:--.--");
    }
    if ((mask&0x80u) && entry && own.wrongWayText[0] && own.wrongWayText[1]) {
        bool showTime=false,showSplit=false;
        const int32_t checkpoint=(entry->currentCheckpoint+5)%5;
        const int32_t stamp=entry->checkpointTicks[checkpoint],time=entry->segmentTicks[checkpoint];
        const float seconds=static_cast<float>(clock.worldTimer-stamp)*0.0002500000118743628f;
        if (PlayerTimerActive(stamp) && seconds<5.0f && PlayerTimerActive(time)) {
            showTime=true;PlayerFormatTime(own,own.wrongWayText[0],time);own.wrongWayText[0]->fontKey=hashString("FONT_A");
            bool comparable=false;int32_t split=0;
            switch (player->settings.splitTimeMode) {
            case 0: {
                const auto* reference=race.entryCount?race.entries[0]:nullptr;
                if (reference==entry)reference=race.entryCount>=2?race.entries[1]:nullptr;
                if (reference) {comparable=true;
                    hp2::Require(raceFrameCalculateSplitTime(&race,&split,reference,entry)==WORLD_SERVICES_OK,"Checkpoint split calculation failed");}
                break;
            }
            case 1:if (checkpoint==0) {const auto best=carFramePlayerCurrentBestLapRecord(player,1);
                if (PlayerTimerActive(best)) {split=time-best;comparable=true;}}break;
            case 2:if (PlayerTimerActive(entry->previousBestSegmentTicks[checkpoint])) {
                split=time-entry->previousBestSegmentTicks[checkpoint];comparable=true;}break;
            case 3:if (PlayerTimerActive(entry->previousSegmentTicks[checkpoint])) {
                split=time-entry->previousSegmentTicks[checkpoint];comparable=true;}break;
            default:break;
            }
            if (comparable) {
                showSplit=true;
                const bool update=(!own.wrongWayText[1]->visible || (playerLiveUpdateSplitTimeRate &&
                    clock.worldLoopCounter%playerLiveUpdateSplitTimeRate==0)) &&
                    (playerLiveUpdateSplitTimeRate || seconds<0.20000000298023224f);
                if (update)PlayerFormatTime(own,own.wrongWayText[1],split,2);
            }
        }
        PlayerVisible(own.wrongWayText[0],showTime);PlayerVisible(own.wrongWayText[1],showSplit);
    }
    if ((mask&0x800000u) && entry) {
        int32_t first=2;
        if (settings.display.leaderboardMode==2)first=std::max(2,std::min(static_cast<int32_t>(race.entryCount)-2,entry->position-1));
        for (int32_t row=0;row<4;++row) {
            const int32_t position=row?first+row-1:1;
            const auto* current=position>0 && static_cast<size_t>(position)<=race.entryCount?race.entries[position-1]:nullptr;
            if (!current || !own.liveStatus[row] || !own.liveSwatch[row] || !own.livePosition[row] || !own.liveBackground[row])continue;
            if (current==entry && own.liveTail) {
                own.liveTail->SetTopLeftY(PlayerObjectTop(own,own.liveBackground[row]));PlayerVisible(own.liveBackground[row],false);
            } else PlayerVisible(own.liveBackground[row],true);
            write(own.livePosition[row],"%c",static_cast<char>('0'+position));
            const auto* selection=current->car->driverInfo;
            hp2::Require(selection && own.bindings.cars,"Leaderboard has no original CarInfo selection");
            const auto* skin=own.bindings.cars->FindSkin(selection->appearance.modelCode,selection->appearance.skinNumber,true);
            hp2::Require(skin,"Leaderboard authored skin is absent");own.liveSwatch[row]->SetColor(skin->authoredWord30);
            auto* text=own.liveStatus[row];const bool started=PlayerTimerActive(current->checkpointTicks[race.previousCheckpoint]);
            if (clock.worldTimeFrames%500<100 || !started) {
                if ((clock.worldTimeFrames/500)&1)text->SetString(own.bindings.cars->Car(static_cast<size_t>(selection->appearance.modelCode)).GetShortName(*own.bindings.language));
                else text->SetString(current==entry?player->settings.playerName:hp2::RaceDriverName(*selection));
                text->ClearFormatOptions(64);
            } else if (current->completionReason) {
                text->SetLanguageID(static_cast<uint32_t>(raceFinishReasonLanguageId(current->completionReason)));text->ClearFormatOptions(64);
            } else if (current==entry) {text->SetString(player->settings.playerName);text->ClearFormatOptions(64);}
            else if (registry.services->pursuit && current->car->tableIndex<8 &&
                registry.services->pursuit->participantReset[current->car->tableIndex].resetPending) {
                text->SetLanguageID(0x49446u);text->ClearFormatOptions(64);
            } else {
                const int32_t laps=convertFloatToInt32(current->progress-entry->progress);
                if (!laps) {
                    int32_t split=0;hp2::Require(raceFrameCalculateSplitTime(&race,&split,entry,current)==WORLD_SERVICES_OK,"Leaderboard split failed");
                    PlayerFormatTime(own,text,split,2);text->SetFormatOptions(64);
                } else {write(text,"%+d %s",laps,own.bindings.language->GetString(std::abs(laps)==1?0x494AA:0x494AE));text->ClearFormatOptions(64);}
            }
        }
    }
    if ((mask&0x400u) && entry && own.splitText[0]) {
        PlayerVisible(own.splitText[0],true);
        if (parameters.pointToPoint) {
            const float beginning=raceFrameCheckpointFraction(&race,race.checkpointPositions[race.previousCheckpoint]);
            const float progress=std::clamp((entry->progress-beginning)/(1.0f-beginning),0.0f,1.0f);
            // PAL180150 widens the float for the variadic text formatter;
            // it does not round to an integer. Literal2B3F60 keeps one decimal.
            write(own.splitText[0],"%5.1f %%",static_cast<double>(progress*100.0f));
        } else write(own.splitText[0],"%d/%d",entry->lap+1,race.targetLapCount);
    }
    if (entry) {
        if (own.lastPosition!=entry->position) {
            own.positionChangedSeconds=static_cast<float>(clock.worldTimer)*0.0002500000118743628f;
            own.positionChange=entry->position<own.lastPosition?1:-1;
        }
        own.lastPosition=entry->position;
        // PAL1801EC uses Player+3D4 (BIGRACE_POSITION). The optional
        // compact caption at+3D0 is absent in the single-player HUD.
        if ((mask&0x200u) && own.lapText[1]) {
            if (own.positionChange) {
                own.positionChangedSeconds=static_cast<float>(clock.worldTimer)*0.0002500000118743628f;
                write(own.lapText[2],"%d",own.positionChange==1?entry->position:own.lastPosition);
                PlayerGrowText(*player,own.lapText[2],own.lapText[1],entry->position<own.lastPosition);
            }
            for (auto index:{1,3,4})PlayerVisible(own.lapText[index],true);
            write(own.lapText[1],"%d",entry->position);
            write(own.lapText[4],"/%d",static_cast<int32_t>(race.entryCount)-(parameters.raceKind==3?race.finishedCount:0));
            if (own.lapText[3])own.lapText[3]->SetLanguageID(player->settings.authoredTimeDisplayOption==1 && entry->position==1?
                0xEEB5u:0xEEACu+static_cast<uint32_t>(entry->position));
        }
    }
    if (mask&2u)for (int32_t i=0;i<4;++i)if (own.positionIcons[i]) {
        const float slip=state.wheelSlipRatio[i],threshold=0.8999999761581421f;
        const bool above=slip>=threshold;
        const float fraction=above?1.0f-(slip-threshold)/(1.0f-threshold):slip/threshold;
        const uint32_t intensity=static_cast<uint32_t>(convertFloatToInt32(fraction*255.0f));
        own.positionIcons[i]->SetColor(above?0x80000080u+(intensity<<8):0x80008000u+intensity);
    }
    if (mask&0x0C008000u) {
        hp2::Require(registry.services->pursuit,"Pursuit HUD has no manager");
        int32_t pursuitLevel=0;
        hp2::Require(carRecoveryStatus(registry.services->pursuit,display->bindings.owner,
            &pursuitLevel)==WORLD_SERVICES_OK,"Pursuit HUD state query failed");
        PlayerPursuitMeter(*player,pursuitLevel);
    }
    if (mask&0x80000u)PlayerRadarDetector(*player);
    if (mask&0x01000000u) {
        const auto* pursuit=registry.services->pursuit;
        hp2::Require(pursuit && pursuit->collision,"Police HUD has no pursuit owner");
        write(own.pursuitTime,pursuit->remainingSeconds>10.0f?"%d":"%02d",static_cast<int32_t>(std::ceil(pursuit->remainingSeconds)));
        int32_t chopperAvailable=0;
        hp2::Require(pursuitManagerChopperAvailable(pursuit,&chopperAvailable)==WORLD_SERVICES_OK,"Police HUD chopper query failed");
        const int32_t greenChoppers=pursuit->chopperRequestsRemaining;
        for (int32_t i=0;i<3;++i)if (own.chopperIcons[i])own.chopperIcons[i]->SetColor(
            i<greenChoppers?0xFF005000u:(!chopperAvailable && i==greenChoppers?0xFF500000u:0xFF000050u));
        const auto* selected=pursuitManagerSelectedEntry(pursuit);
        // PAL1805A8/+1805E0 read CopManager+0 (remaining roadblock
        // requests), and PAL180620 reads the embedded machine state+618.
        // The cop pool count+4 and reserved roadblock pointer+14 differ.
        const bool waiting=selected && selected->group && selected->group->owner->roadBlock.state!=0;
        const int32_t greenCars=pursuit->roadBlocksRemaining;
        for (int32_t i=0;i<3;++i)if (own.copCarIcons[i])own.copCarIcons[i]->SetColor(
            i<greenCars?0xFF005000u:(waiting && i==greenCars?0xFF500000u:0xFF000050u));
        float damage=0.0f;
        if (selected && selected->group && selected->group->target && selected->group->target->bindings.owner)
            damage=pursuit->collision->damageLevels[selected->group->target->bindings.owner->tableIndex];
        const int32_t damageLights=damage==0.0f?0:std::min(5,convertFloatToInt32(damage*5.0f)+1);
        for (int32_t i=0;i<5;++i)if (own.damageLights[i])own.damageLights[i]->SetColor(i<damageLights?0xFF000070u:0xFF000020u);
        int32_t total=0;
        if (registry.count==1)for (int32_t i=registry.world->firstGroupEnd;i<registry.world->secondGroupEnd;++i) {
            int32_t count=0;if (auto* car=registry.world->carTable[i]) {
                hp2::Require(pursuitManagerGetPulloverCount(pursuit,car,&count)==WORLD_SERVICES_OK,"Police HUD pullover query failed");total+=count;}
        } else if (registry.players[1] && registry.players[1]->car)
            hp2::Require(pursuitManagerGetPulloverCount(pursuit,registry.players[1]->car->bindings.owner,&total)==WORLD_SERVICES_OK,"Police HUD second racer query failed");
        const int32_t icons=registry.count==1?4:3;
        for (int32_t i=0;i<icons;++i)PlayerVisible(own.pursuitCrosses[i],i<total);
    }
    if ((mask&0x200000u) && (carFramePlayerReplayActive(player) || carFramePlayerShouldShowBustedText(player)))
        hp2::Require(carNisOverlayUpdate(player->nisOverlay,carFramePlayerReplayActive(player)!=0)==CAR_NIS_OK,"NIS HUD update failed");
    for (auto* tracker:own.tracker)PlayerVisible(tracker,false);
    // PAL180900..180A98: source order, same World table, seven authored
    // bitmaps. Car::DamageAbove publishes the bearing; actionEnabled is
    // the existing motion owner, not an inferred visibility flag.
    if ((mask&0x100000u) && race.state==4) {
        const auto& world=*registry.world;
        hp2::Require(world.firstGroupEnd>=0 && world.firstGroupEnd<=world.secondGroupEnd &&
            world.secondGroupEnd<=world.thirdGroupEnd && world.thirdGroupEnd<=WORLD_MAX_CARS,
            "HUD tracker World groups are invalid");
        int32_t slot=0;
        const auto group=[&](int32_t begin,int32_t end,uint32_t color) {
            for (int32_t index=begin;index<end && slot<7;++index,++slot) {
                const auto* car=world.carTable[index];
                const auto* movement=car?static_cast<const CarMovement*>(car->movement):nullptr;
                hp2::Require(movement && movement->bindings.ownerMotion,"HUD tracker has no actual Car motion owner");
                if (movement->bindings.ownerMotion->actionEnabled) {
                    carFramePlayerDrawTracker(player,car,slot);
                    hp2::Require(own.tracker[slot],"Authored HUD tracker bitmap is absent");
                    own.tracker[slot]->SetColor(color);
                }
            }
        };
        group(world.firstGroupEnd,world.secondGroupEnd,0xFF0035ACu);
        group(world.secondGroupEnd,world.thirdGroupEnd,0xFF800000u);
    }
    (void)ui;
}

void carFramePlayerDrawTracker(CarFramePlayerState* player,const Car* car,int32_t slot)
{
    // Full PAL17C278..17C380. This consumes the actual source Car cell;
    // it neither calculates a replacement projection nor draws a marker
    // for a car outside the original50-unit/bearing gates.
    hp2::Require(player && player->ownedState && player->car && player->car->bindings.owner && car &&
        slot>=0 && slot<7,"DrawTracker lacks its actual Player/Car owners");
    const auto* ownCar=player->car->bindings.owner;
    const float delta[3]={ownCar->position[0]-car->position[0],ownCar->position[1]-car->position[1],
        ownCar->position[2]-car->position[2]};
    float distance=std::sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2]);
    if (!(distance<=50.0f))return;
    hp2::Require(std::isfinite(car->trackerBearing),"DrawTracker's Car bearing was not produced by DamageAbove");
    if (car->trackerBearing<-.5f || car->trackerBearing>.5f)return;
    auto& own=*player->ownedState;auto* bitmap=own.tracker[slot];
    hp2::Require(bitmap,"DrawTracker's final authored bitmap is absent");
    bitmap->topLeftX=static_cast<float>(convertFloatToInt32((car->trackerBearing+.5f)*400.0f)+120);
    distance*=0.019999999552965164f;
    if (distance>1.0f)distance=1.0f;
    const float remaining=1.0f-distance;
    bitmap->scaleY=remaining+remaining;bitmap->scaleX=remaining+remaining;
    PlayerVisible(bitmap,true);
}

// Final178C90 retains one actual display-settings cell and the existing map.
void carFramePlayerSetTrackMapMode(CarFramePlayerState* player,int32_t mode)
{
    hp2::Require(player && player->ownedState && player->registry && player->registry->clock,
        "TrackMap mode has no owning Player");
    auto& own=*player->ownedState;const auto& race=*own.bindings.parameters;
    hp2::Require(race.runtimeSettings,"TrackMap mode has no actual display settings");
    auto& display=race.runtimeSettings->display;int8_t* stored=nullptr;
    const int32_t* allowed=nullptr;int32_t count=0;
    if (race.specialPursuit) {stored=&display.pursuitTrackMapMode;allowed=pursuitTrackMapModes;count=2;}
    else if (player->registry->count==2) {
        stored=&display.splitViewTrackMapMode;allowed=splitPlayerTrackMapModes;count=2;
        if (player->movement.seat>0)mode=6;
    } else {stored=&display.singleViewTrackMapMode;allowed=singlePlayerTrackMapModes;count=6;}
    if (std::find(allowed,allowed+count,mode)==allowed+count)mode=allowed[0];
    if (*stored!=mode) {*stored=static_cast<int8_t>(mode);own.trackMapChangedTicks=player->registry->clock->realTimeFrames;}
    if (own.trackMap)own.trackMap->SetMode(mode);
}
// PAL181C08, the original Player wrapper has no independent map state.
void carFramePlayerRenderTrackMap(CarFramePlayerState* player)
{
    if (player && player->ownedState && player->ownedState->trackMap)player->ownedState->trackMap->Render();
}

// PAL17C888, including its real HudStartAnimation17D130 and bitmap
// HudAnimator23B5E0 dependency. The source slot15 keeps its own active state.
void carFramePlayerHudStartWrongWay(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->registry && player->registry->clock,
        "Wrong-way HUD animation has no actual Player owner");
    auto& own=*player->ownedState;
    std::array<float,2> first{258.0f,119.0f},last{-105.0f,-80.0f};
    if (raceParametersIsFreeRun(own.bindings.parameters)) last={-240.0f,-100.0f};
    if ((own.hudFeatures&0x800200u)==0x800000u) last={-95.0f,-80.0f};
    last[0]+=first[0];last[1]+=first[1];
    std::array<float,2> scale{1.0f,1.0f},lastScale{0.30000001192092896f,0.30000001192092896f};
    if (player->registry->count>=2) {
        scale={0.5f,0.5f};lastScale={0.25f,0.25f};
        if (player->movement.seat==0) {first={258.0f,125.0f};last={128.0f,125.0f};}
        if (player->movement.seat==1) {first={258.0f,350.0f};last={128.0f,350.0f};}
    }
    auto* bitmap=static_cast<hp2::FlailerBitmapChunk*>(
        own.bindings.flailer->FindObject(own.hudScreen,0x89AF1A1Eu,2));
    hp2::Require(bitmap,"Wrong-way HUD bitmap is absent from its final authored screen");
    const hp2::HudMoveAnimation animation{first,{last[0]-first[0],last[1]-first[1]},
        scale,{lastScale[0]-scale[0],lastScale[1]-scale[1]},64,0.0f,1.0f};
    PlayerAnimator(*player,15)->StartAnimation(*bitmap,animation,*player->registry->clock,true);
}

// PAL17E038..17E220. The original renderer advances these exact slots;
// the score credit is committed when amount animation10 finishes once.
void carFramePlayerHudDoAnimations(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->registry && player->registry->clock,
        "HUD animation frame has no actual Player clock");
    const int32_t ticks=player->registry->clock->worldTimer;
    const auto simple=[&](int32_t index) {
        auto* animation=PlayerAnimator(*player,index);
        if (animation && animation->Active()) animation->DoAnimation(ticks);
    };
    const uint32_t mask=player->ownedState->hudFeatures;
    if (mask&0x200u) simple(0);
    if (mask&0x60u) simple(1);
    for (int32_t index=2;index<9;++index) simple(index);
    if (auto* animation=PlayerAnimator(*player,10);animation && animation->Active()) {
        if (animation->DoAnimation(ticks,player->scoreState->runningAmount.data()))
            carFramePlayerAddToScore(player,player->scoreState->runningAmount.data());
    }
    if (auto* animation=PlayerAnimator(*player,11);animation && animation->Active())
        animation->DoAnimation(ticks,player->scoreState->caption.data());
    if (auto* animation=PlayerAnimator(*player,12);animation && animation->Active())
        animation->DoAnimation(ticks,"SCORE");
    if (auto* animation=PlayerAnimator(*player,13);animation && animation->Active())
        animation->DoAnimation(ticks,"");
    simple(15);simple(16);simple(17);
}

CarFramePlayerState* hp2::CreatePlayer(int32_t port,int32_t seat,const PlayerConstructionBindings& b)
{
    if (!b.registry || !b.parameters || !b.parameters->runtimeSettings || !b.input ||
        !b.flailer || !b.renderManager || !b.language || !b.clock ||
        port<0 || port>1 || seat<0 || seat>1 || b.registry->count<0 || b.registry->count>=2 ||
        b.registry->playersByNumber[seat] ||
        (b.registry->clock && b.registry->clock!=b.clock))
        throw std::invalid_argument("Player constructor has no actual pre-race owners");
    auto player=std::make_unique<CarFramePlayerState>();
    auto own=std::make_unique<CarPlayerOwnedState>();
    own->bindings=b;
    player->registry=b.registry;
    const auto* display=&b.parameters->runtimeSettings->display;
    hp2::Require(!b.registry->displaySettings || b.registry->displaySettings==display,
        "Players must borrow the same live display settings");
    b.registry->displaySettings=display;
    player->movement.controllerPort=port;
    player->movement.seat=seat;
    player->joystickType=joystickGetType(b.input,port);
    player->undamagedRace=1; // Final175AD4, cleared by qualifying real collisions.
    player->settings=b.parameters->runtimeSettings->players[seat];
    own->viewNumber=b.parameters->mode==2?seat+1:1;
    own->overlayViewNumber=b.parameters->mode==2?0:3;
    if (!b.parameters->fastSpeedTrapSpawns && !b.parameters->demoReplay) {
        if (b.parameters->mode==2) {
            own->hudScreen=seat==0?0xA8EB7AF0u:0x6283BE92u;
            own->nisScreen=seat==0?0xD4D25EE7u:0xC17316C9u;
        } else {own->hudScreen=0xF1F1F8E5u;own->nisScreen=0x3EA89B7Cu;}
    }
    if (b.parameters->specialPursuit) own->nisScreen=0x3EA89B7Cu;
    if (BindNisOverlay(own->overlay,*b.flailer,*b.renderManager,*b.language,*b.clock,
        carNisOverlayTuning,own->nisScreen)!=CAR_NIS_OK)
        throw std::runtime_error("Player NIS overlay construction failed");
    char name[32];std::snprintf(name,sizeof(name),"Player%d",1-seat);
    own->nis=std::make_unique<NisManager>(name,seat,b.nis);
    player->replay=&own->nis->Playback();
    player->nisOverlay=&own->overlay;
    if (!carFramePlayerInitializeNisSelection(player.get()))
        throw std::runtime_error("Player NIS selection allocation failed");
    if (!driverHistoryInitialize(&player->speedHistory,10)) {
        carFramePlayerDisposeNisSelection(player.get());
        throw std::bad_alloc();
    }
    // Original independent allocations+674(164 bytes) and+6E0(32 vectors).
    // They remain owned storage; no address-based interpreter consumes them.
    try {
        for (int32_t i=0;i<20;++i)
            if (i!=10 && i!=11) own->animations[i]=std::make_unique<HudAnimator>();
        own->timingWorkspace=std::unique_ptr<std::byte[]>(new std::byte[164]);
        own->routeWorkspace=std::unique_ptr<float[]>(new float[32*4]);
        if (b.registry->services && b.registry->services->audio)
            worldAudioAcquireInGameSample(b.registry->services->audio,"RadarBeep",&own->radarBeep);
        // PAL175A28..175AC8 deliberately rounds through float seconds, even
        // though its two scales nearly cancel. Preserve those operations.
        own->countdownTimer=convertFloatToInt32(
            (static_cast<float>(b.clock->worldTimer)*0.0002500000118743628f)*4000.0f+0.5f);
        own->letterbox[0]=static_cast<FlailerBitmapChunk*>(b.flailer->FindObject(0x49C15518u,0x464BA8EAu,2));
        own->letterbox[1]=static_cast<FlailerBitmapChunk*>(b.flailer->FindObject(0x49C15518u,0xD911BEACu,2));
        own->letterbox[2]=static_cast<FlailerBitmapChunk*>(b.flailer->FindObject(0x49C15518u,0xF7F1CFB3u,2));
        own->letterbox[3]=static_cast<FlailerBitmapChunk*>(b.flailer->FindObject(0x49C15518u,0xF88DD0B1u,2));
        own->letterboxBottom=PlayerObjectTop(*own,own->letterbox[0]);
        own->letterboxTop=PlayerObjectTop(*own,own->letterbox[1]);
        own->letterboxTopDelta=PlayerObjectTop(*own,own->letterbox[2])-own->letterboxTop;
        own->letterboxBottomDelta=PlayerObjectTop(*own,own->letterbox[3])-own->letterboxBottom;
        if (!BindPlayerScore(*player,{b.flailer,b.language,own->hudScreen,false}))
            throw std::bad_alloc();
    } catch (...) {
        carFramePlayerDisposeScore(player.get());
        driverHistoryDispose(&player->speedHistory);
        carFramePlayerDisposeNisSelection(player.get());
        throw;
    }
    b.registry->clock=b.clock;
    b.registry->players[b.registry->count++]=player.get();
    b.registry->playersByNumber[seat]=player.get();
    if (b.registry->count==2) {
        own->otherPlayer=b.registry->players[0];
        if (own->otherPlayer->ownedState) own->otherPlayer->ownedState->otherPlayer=player.get();
    }
    player->ownedState=own.release();
    return player.release();
}

bool hp2::BindPlayerForRace(CarFramePlayerState& player,const CarFramePlayerCameraContext& cameras)
{
    if (!player.ownedState || player.cameras || !player.registry || !player.registry->world ||
        !player.registry->services || !player.registry->services->race ||
        cameras.input!=player.registry->services->cameras || cameras.race!=player.ownedState->bindings.parameters)
        return false;
    return carFramePlayerCamerasInitialize(&player,&cameras,player.ownedState->viewNumber,
        player.ownedState->overlayViewNumber,player.settings.cameraSpring)==CAR_CINEMATIC_OK;
}

int32_t carFramePlayerAttachToCar(CarFramePlayerState* player,Car* car)
{
    if (!player || !car || player->car || !player->registry || !player->registry->world ||
        !player->replay || !player->nisSelection) return 0;
    auto* movement=car->Movement();auto* frame=car->Frame();auto* driver=car->Driver();
    if (!movement || !frame || !driver || !movement->bindings.state ||
        movement->bindings.owner!=car || frame->environment.player || movement->bindings.player)
        return 0;
    if (driver->kind==CAR_DRIVER_REAL && !driver->owner.real) return 0;
    frame->environment.player=player;
    movement->bindings.player=&player->movement;
    player->car=movement;player->driver=driver;player->presentation=frame->environment.presentation;
    movement->bindings.state->pursuitMode=player->settings.initialVehicleMode;
    if (driver->kind==CAR_DRIVER_REAL) {
        driver->owner.real->input.player=player;
        realDriverSetGearInputMode(&driver->owner.real->input,player->settings.gearInputMode);
    }
    if (!carFramePlayerPredetermineNis(player)) {
        player->result=CAR_PLAYER_LIFECYCLE_NIS_FAILURE;return 0;
    }
    return 1;
}

int32_t carFramePlayerDetachFromCar(CarFramePlayerState* player)
{
    if (!player || !player->car || !player->registry || !player->registry->services ||
        !player->registry->services->radialBlur || !player->replay || !player->nisOverlay)
        return 0;
    auto* movement=player->car;
    if (!movement->frame || movement->frame->environment.player!=player || movement->bindings.player!=&player->movement)
        return 0;
    carFramePlayerReconcileInput(player);
    screenRadialBlurStop(player->registry->services->radialBlur,player->movement.seat);
    // PAL18198C calls ClearCameraMovers177430 before StopAnimating/Stop and
    // before detaching the Car. A NIS view unregisters masks on its live
    // cars and presentation registry; deferring it to DestroyPlayer leaves
    // those borrowers pointing into the already retired World.
    if (carFramePlayerClearCameras(player)!=CAR_CINEMATIC_OK) {
        player->result=CAR_PLAYER_LIFECYCLE_CAMERA_FAILURE;return 0;
    }
    carFramePlayerClearPreview(player);
    if (player->replay->active) {
        if (carLiveReplayStop(player->replay)!=CAR_NIS_OK) {
            player->result=CAR_PLAYER_LIFECYCLE_NIS_FAILURE;return 0;
        }
        carNisOverlayFinish(player->nisOverlay);
    }
    carFramePlayerSetInputMode(player,0);
    movement->frame->environment.player=nullptr;
    movement->bindings.player=nullptr;
    if (player->driver && player->driver->kind==CAR_DRIVER_REAL && player->driver->owner.real)
        player->driver->owner.real->input.player=nullptr;
    player->car=nullptr;player->driver=nullptr;player->presentation=nullptr;
    return 1;
}

// Complete PAL17C7C8: keep source slots12..14 and18 active, flush the
// existing score slot10 exactly once, and finish every other animation.
void carFramePlayerClearHudAnimations(CarFramePlayerState* player)
{
    if (!player) return;
    for (int32_t index=0;index<20;++index) {
        if ((index>=12 && index<=14) || index==18) continue;
        auto* animation=PlayerAnimator(*player,index);
        if (!animation) continue;
        if (index==10 && animation->Active())
            carFramePlayerAddToScore(player,player->scoreState->runningAmount.data());
        animation->FinishAnimation();
    }
}

void carFramePlayerClearRecoveryAirborneTime(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState,"Recovery score reset has no actual Player owner");
    // PAL182270 clears Player+5E4. The same float is read/written by
    // PAL17E864/17EB44/17EB80/17EC74 for airborne scoring. The legacy
    // presentation code8 is not a request to clear HUD animation slot8.
    player->ownedState->airborneStartedSeconds=0.0f;
}

int32_t carFramePlayerHideHUD(CarFramePlayerRegistry* registry)
{
    if (!registry || registry->count<0 || registry->count>2) return 0;
    for (int32_t i=0;i<registry->count;++i) {
        if (!registry->players[i]) return 0;
        carFramePlayerClearHudAnimations(registry->players[i]);
    }
    return 1;
}
int32_t carFramePlayerShowHUD(CarFramePlayerRegistry* registry)
{
    if (!registry || registry->count<0 || registry->count>2 || !registry->services || !registry->services->input)
        return 0;
    for (int32_t i=0;i<registry->count;++i) {
        auto* player=registry->players[i];if (!player) return 0;
        player->joystickType=joystickGetType(registry->services->input,player->movement.controllerPort);
    }
    return 1;
}

void carFramePlayerResetFlailerMovement(CarFramePlayerState* player)
{
    if (!player || !player->ownedState || !player->registry) return;
    auto& own=*player->ownedState;
    if (own.flailerOriginalTop==0.0f || player->registry->count!=1) return;
    own.flailerMoved=PlayerObjectTop(own,own.wantedIcons[0])==own.flailerOriginalTop?0:1;
    auto& ui=*own.bindings.flailer;
    for (int32_t i=0;i<12;++i) {
        char name[32];std::snprintf(name,sizeof(name),"RADAR_LIGHT%d",i+1);
        own.wantedIcons[i]=static_cast<hp2::FlailerBitmapChunk*>(ui.FindObject(own.hudScreen,hashString(name),2));
    }
    for (auto* object=ui.FindObjectByGroup(own.hudScreen,0x062743F5u,0);object;
        object=ui.FindObjectByGroup(own.hudScreen,0x062743F5u,0,object))
        if (own.flailerMoved) object->SetTopLeftY(PlayerObjectTop(own,object)+70.0f);
}

uint32_t carFramePlayerHudFeatures(const CarFramePlayerState* player)
{ return player && player->ownedState?player->ownedState->hudFeatures:0; }
uint32_t carFramePlayerHudBlockedFeatures(const CarFramePlayerState* player)
{ return player && player->ownedState?player->ownedState->blockedFeatures:UINT32_MAX; }

bool hp2::DestroyPlayer(CarFramePlayerState* player)
{
    if (!player || !player->ownedState || !player->registry || player->car) return false;
    auto* registry=player->registry;
    if (registry->count<1 || registry->count>2 || registry->players[registry->count-1]!=player ||
        registry->playersByNumber[player->movement.seat]!=player) return false;
    carFramePlayerResetFlailerMovement(player);
    if (!carFramePlayerSetHudFeatures(player,0)) return false;
    // PAL175C3C..175C58: Player owns and destroys its private bSample at
    // +8C even when shutdown interrupts an active cinematic effect.
    if (player->cinematicVoice) {
        if (!registry->services || !registry->services->audio) return false;
        WorldAudioVoiceRef voice{player->cinematicVoice,player->cinematicVoiceGeneration};
        if (playerCinematicVoiceStop(registry->services->audio,1,&voice)!=WORLD_AUDIO_OK)
            return false;
        player->cinematicVoice=voice.instance;
        player->cinematicVoiceGeneration=voice.generation;
    }
    carFramePlayerClearPreview(player);
    carFramePlayerCamerasDispose(player);
    carFramePlayerDisposeVisibleDrivingLine(player);
    if (player->ownedState->nis->Close()!=CAR_NIS_OK) return false;
    driverHistoryDispose(&player->speedHistory);
    carFramePlayerDisposeScore(player);
    carFramePlayerDisposeNisSelection(player);
    registry->players[--registry->count]=nullptr;
    registry->playersByNumber[player->movement.seat]=nullptr;
    carGlareResetRequested=1;
    for (uint32_t key:{0x7307C47Fu,0x39C9EEADu}) {
        auto* bitmap=static_cast<FlailerBitmapChunk*>(player->ownedState->bindings.flailer->FindObject(
            player->ownedState->hudScreen,key,2));
        if (bitmap && bitmap->tachometerPoints==player->ownedState->tachometerPoints.data())
            bitmap->tachometerPoints=nullptr;
    }
    delete player->ownedState;
    delete player;
    return true;
}

std::exception_ptr hp2::DetachPlayerForShutdown(CarFramePlayerState* player) noexcept
{
    if(!player || !player->ownedState)return {};
    auto& own=*player->ownedState;
    if(own.shutdownDetached)return own.shutdownFailure;
    own.shutdownDetached=true;
    PlayerShutdownErrors errors;
    auto* registry=player->registry;
    auto* services=registry?registry->services:nullptr;
    if(own.hudFeatures)errors.Run([&] {
        Require(carFramePlayerSetHudFeatures(player,0)!=0,"Terminal Player HUD retirement failed");
    });
    // No new input mode is entered during terminal retirement. Remove the
    // actual typed borrowers without relying on a complete Car/pursuit graph.
    if(auto* input=services?services->input:own.bindings.input) {
        const auto belongs=[player](const GameInputTarget& target) {
            return target.kind>=GAME_INPUT_PLAYER && target.kind<=GAME_INPUT_PLAYER_CAMERA &&
                target.owner.player==player;
        };
        for(size_t index=0;index<input->count;) {
            if(!belongs(input->registrations[index].target)){++index;continue;}
            if(input->dispatching && input->dispatchDevice==input->registrations[index].device &&
                input->dispatchEvent==input->registrations[index].event)input->dispatchCancelled=1;
            for(size_t next=index+1;next<input->count;++next)input->registrations[next-1]=input->registrations[next];
            --input->count;
        }
        for(auto& event:input->queuedEvents)if(event.active && belongs(event.target))event.active=0;
    }
    player->inputMode=0;
    if(services && services->radialBlur)
        errors.Run([&]{screenRadialBlurStop(services->radialBlur,player->movement.seat);});
    if(registry && registry->world && registry->world->timing.callbackContext==player) {
        registry->world->timing.callback=nullptr;registry->world->timing.callbackContext=nullptr;
    }
    // NIS still borrows both live Cars here. Its shutdown owns replay-view
    // disposal, so the Player alias must not be freed a second time.
    if(own.nis)errors.Remember(own.nis->Shutdown());
    if(player->cameras)player->cameras->replay=nullptr;
    ShutdownPlayerCameras(player,errors);
    errors.Run([&]{carFramePlayerClearPreview(player);});
    if(player->nisOverlay)errors.Run([&]{carNisOverlayFinish(player->nisOverlay);});
    if(auto* movement=player->car) {
        if(movement->frame && movement->frame->environment.player==player)
            movement->frame->environment.player=nullptr;
        if(movement->bindings.player==&player->movement)movement->bindings.player=nullptr;
    }
    if(player->driver && player->driver->kind==CAR_DRIVER_REAL && player->driver->owner.real &&
        player->driver->owner.real->input.player==player)
        player->driver->owner.real->input.player=nullptr;
    player->car=nullptr;player->driver=nullptr;player->presentation=nullptr;
    own.hudFeatures=0;own.shutdownFailure=errors.first;
    return errors.first;
}

std::exception_ptr hp2::DestroyPlayerForShutdown(CarFramePlayerState* player) noexcept
{
    if(!player)return {};
    PlayerShutdownErrors errors;
    errors.Remember(DetachPlayerForShutdown(player));
    auto* registry=player->registry;
    auto* own=player->ownedState;
    if(own) {
        errors.Run([&]{carFramePlayerResetFlailerMovement(player);});
        if(player->cinematicVoice) {
            errors.Run([&] {
                Require(registry && registry->services && registry->services->audio,
                    "Terminal Player voice lost its audio owner");
                WorldAudioVoiceRef voice{player->cinematicVoice,player->cinematicVoiceGeneration};
                Require(playerCinematicVoiceStop(registry->services->audio,1,&voice)==WORLD_AUDIO_OK,
                    "Terminal Player voice retirement failed");
            });
            // The live audio bank/queue owns a failed transport retirement;
            // no completion borrows Player storage. Root cancels that queue.
            player->cinematicVoice=nullptr;player->cinematicVoiceGeneration=0;
        }
        errors.Run([&]{carFramePlayerDisposeVisibleDrivingLine(player);});
        errors.Run([&]{driverHistoryDispose(&player->speedHistory);});
        errors.Run([&]{carFramePlayerDisposeScore(player);});
        errors.Run([&]{carFramePlayerDisposeNisSelection(player);});
        if(own->otherPlayer && own->otherPlayer->ownedState &&
            own->otherPlayer->ownedState->otherPlayer==player)
            own->otherPlayer->ownedState->otherPlayer=nullptr;
        errors.Run([&] {
            if(!own->bindings.flailer)return;
            for(uint32_t key:{0x7307C47Fu,0x39C9EEADu}) {
                auto* bitmap=static_cast<FlailerBitmapChunk*>(own->bindings.flailer->FindObject(
                    own->hudScreen,key,2));
                if(bitmap && bitmap->tachometerPoints==own->tachometerPoints.data())bitmap->tachometerPoints=nullptr;
            }
        });
    }
    if(registry) {
        for(int32_t index=0;index<registry->count;) {
            if(registry->players[index]!=player){++index;continue;}
            for(int32_t next=index+1;next<registry->count;++next)registry->players[next-1]=registry->players[next];
            registry->players[--registry->count]=nullptr;
        }
        for(auto& registered:registry->playersByNumber)if(registered==player)registered=nullptr;
    }
    player->ownedState=nullptr;
    delete own;delete player;carGlareResetRequested=1;
    return errors.first;
}

// FillInRaceOverMessages, final181558..1818C4, with the two real formatting
// callees181498/181500. Text comes from the current final-PAL Language owner;
// the race/profile/speech borrowers are the same owners used by Results.
void carFramePlayerFillRaceOverMessages(CarFramePlayerState* player)
{
    hp2::Require(player && player->ownedState && player->registry &&
        player->registry->services && player->registry->services->race,
        "Race-over HUD has no live Player/Race");
    auto& own=*player->ownedState;
    const auto& race=*player->registry->services->race;
    const auto* entry=PlayerRaceEntry(*player);
    hp2::Require(entry && own.bindings.parameters && own.bindings.language,
        "Race-over HUD has no participant/Language");
    const auto& parameters=*own.bindings.parameters;
    auto& language=*own.bindings.language;
    const int32_t position=entry->finishPosition,reason=entry->completionReason;
    std::string message;
    bool selected=false;
    auto plain=[&](uint32_t key) {
        message=hp2::FormatLocalizedText(language.GetString(static_cast<int32_t>(key)),{},512);
        selected=true;
    };
    auto finish=[&](uint32_t key) {
        static constexpr uint32_t ordinals[]={0x4E5E9,0xEE51,0x4E5EB,0x4E5EC,
            0x4E5ED,0x4E5EE,0x4E5EF,0x4E5F0,0x4E5F1};
        hp2::Require(position>=0 && position<9,"Race-over finish position is invalid");
        message=hp2::FormatLocalizedText(language.GetString(static_cast<int32_t>(key)),
            {language.GetString(static_cast<int32_t>(ordinals[position]))},512);
        selected=true;
    };
    auto time=[&](int32_t ticks,uint32_t key) {
        char text[GAME_TIMER_TEXT_CAPACITY];
        gameTimerPrintRounded(&ticks,text,0.009999999776482582f,0,&language.currentLanguage);
        message=hp2::FormatLocalizedText(language.GetString(static_cast<int32_t>(key)),{text},512);
        selected=true;
    };
    auto knockout=[&](int32_t won) {
        hp2::Require(worldAudioKnockoutRaceOverSpeech(player,won)==WORLD_AUDIO_MESSAGES_OK,
            "Race-over knockout speech queue failed");
    };
    switch (parameters.raceKind) {
    case 2: {
        const int32_t best=raceCarEntryBestLapTicks(entry,&race);
        if (reason==2) {
            if (PlayerTimerActive(best))time(best,0xE684);else plain(0xE685);
        } else if (reason==1)time(best,0xE687);
        break;
    }
    case 3:
        if (reason==2)plain(0xE683);
        else if (reason==3) {finish(0xE68B);knockout(0);}
        else finish(0xE682);
        break;
    case 4:
        if (reason==2)plain(0xE683);
        else if (reason==3 || position==static_cast<int32_t>(race.entryCount)) {
            finish(0xE68A);knockout(0);
        } else {finish(race.entryCount<3?0xE682u:0xE689u);knockout(1);}
        break;
    case 1:case 5:
        if (parameters.specialPursuit) {
            if (parameters.mode==1)plain(reason==4?0xE68Cu:0xE68Eu);
            else if (parameters.mode==2) {
                const int32_t seat=player->movement.seat;
                hp2::Require(seat==0 || seat==1,"Pursuit finish Player index is invalid");
                if (reason==4)plain(seat==0?0xE690u:0xE692u);
                else if (reason==5) {
                    const auto* other=player->registry->playersByNumber[seat^1];
                    const auto* otherEntry=other?PlayerRaceEntry(*other):nullptr;
                    hp2::Require(otherEntry,"Pursuit finish has no other live Player entry");
                    plain(otherEntry->completionReason==5?0xE68Fu:(seat==0?0xE691u:0xE693u));
                } else if (seat==1 && reason==1)plain(0xE692);
            }
        } else {
            hp2::Require(race.services && race.services->profile,
                "Race-over HUD has no canonical FRED/profile");
            const int32_t useEvent=raceProfileUsesResults(race.services->profile);
            hp2::Require(useEvent>=0,"Race-over FRED classification is unavailable");
            hp2::Require(!useEvent || race.services->currentEvent,
                "Race-over event classification has no actual FRED event");
            if (useEvent && race.services->currentEvent->kind==5) {
                if (reason==2)plain(0xE686);
                else if (reason==1)time(entry->finishTicks,0xE688);
            } else finish(reason==2?0xE683u:0xE682u);
        }
        break;
    default:break;
    }
    // Released invalid combinations leave its stack buffer uninitialized.
    // The native owner rejects those inputs instead of publishing arbitrary
    // memory or inventing a successful finish message.
    hp2::Require(selected,"Race-over reason/race combination has no authored message");
    own.bindings.flailer->RenderTextIntoGroup(own.hudScreen,0xC707E093u,message.c_str());
}

// PAL1796B0..17BD20, closed native feature branches. The unsupported mask is
// checked before any source-visible state change; a rejected request never
// substitutes a smaller HUD or reports the complete source function as ported.
int32_t carFramePlayerSetHudFeatures(CarFramePlayerState* player,uint32_t requested)
{
    if (!player || !player->ownedState || !player->registry) return 0;
    if((requested&0x60u) && player->car && player->car->bindings.info &&
       player->car->bindings.info->bodyCategory==2)
        requested=(requested&~0x60u)|0x10000000u;
    const auto& unlocks=hp2::easterEggs;
    if (!unlocks.records || unlocks.recordCount<=4) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    auto& own=*player->ownedState;
    auto& ui=*own.bindings.flailer;
    auto& parameters=*own.bindings.parameters;
    constexpr uint32_t supported=0x10000000u|0x0C000000u|0x00C00000u|
        0x03000000u|0x00300000u|0x00080000u|0x00040000u|0x00020000u|0x00010000u|
        0x0000E000u|0x00001000u|0x00000508u|0x00000200u|0x000000E0u|0x00000007u;
    const uint32_t current=own.hudFeatures&~0x60u;
    const uint32_t changed=current^requested;
    own.blockedFeatures=(changed|requested)&~supported;
    if (own.blockedFeatures) {player->result=CAR_PLAYER_LIFECYCLE_HUD_NOT_PORTED;return 0;}
    if ((changed&requested&4u) && (!player->car || !player->registry->services ||
        !player->registry->services->race || !PlayerRaceEntry(*player))) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    // PAL GP-5700 is Race (ctor147638), not World. Its +18 count is the
    // canonical participant count, independently of traffic/police cars.
    if ((changed&0x800200u) && (!player->registry->services || !player->registry->services->race)) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    if ((changed&0x1000u) && !player->cameras) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    if ((changed&0x200000u) && own.nisScreen && !player->registry->sessionState) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    if ((changed&0x2000u) && (!player->cameras || !player->cameras->context ||
        !player->cameras->context->services)) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    if ((changed&requested&1u) && (!player->car || !own.bindings.track ||
        !player->cameras || !player->cameras->context || !player->cameras->context->services)) {
        player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
    }
    const hp2::CarDefinition* gaugeCar=nullptr;
    if (changed&0x60u) {
        if (!player->car || !player->car->bindings.info || !own.bindings.cars || !parameters.runtimeSettings) {
            player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
        }
        const int32_t model=player->car->bindings.info->modelCode;
        if (model<0 || static_cast<std::size_t>(model)>=own.bindings.cars->Cars().size()) {
            player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;
        }
        gaugeCar=&own.bindings.cars->Car(static_cast<std::size_t>(model));
    }
    auto object=[&](uint32_t key,int32_t kind){return ui.FindObject(own.hudScreen,key,kind);};
    auto text=[&](uint32_t key){return static_cast<hp2::FlailerTextChunk*>(object(key,1));};
    auto bitmap=[&](uint32_t key){return static_cast<hp2::FlailerBitmapChunk*>(object(key,2));};
    auto visibility=[&](uint32_t key,int32_t value){ui.SetObjectVisibility(own.hudScreen,key,value);};
    const auto showObject=PlayerVisible;
    auto namedKey=[](const char* format,int32_t index){
        char name[64];std::snprintf(name,sizeof(name),format,index);return hashString(name);
    };
    own.hudFeatures=current;
    carFramePlayerClearHudAnimations(player);
    if (current==0 && requested!=0)
        own.bindings.renderManager->SetScreenVisibility(ui,own.hudScreen,0);
    else if (current!=0 && requested==0)
        own.bindings.renderManager->SetScreenVisibility(ui,own.hudScreen,1);
    if ((changed&0x1000u) && carFramePlayerSetAttachedCamera(player,requested&0x1000u)!=CAR_CINEMATIC_OK) {
        player->result=CAR_PLAYER_LIFECYCLE_CAMERA_FAILURE;return 0;
    }
    constexpr uint32_t gaugeKeys[]={0x8D28ACCEu,0x01223ABDu,0xF446D30Eu,0x35710A5Du,
        0x7307C47Fu,0x7307C47Fu,0x31F74338u,0xF81DFFA6u,0xD9098A4Bu,0xF25E727Au,
        0x7AA7A2A1u,0xCF9BF6F3u,0xC637891Cu,0x9309E75Cu,0xD312F0CBu,0x39C9EEADu,
        0xD2F7FA46u,0x7BE1E0F4u,0xC8842AF9u,0x7BB54948u};
    if (changed&0x20u) {
        constexpr uint32_t hidden[]={0xC637891Cu,0x9309E75Cu,0xD312F0CBu,0x39C9EEADu,
            0xD2F7FA46u,0x7BE1E0F4u,0xC8842AF9u,0x7BB54948u,0xCF9BF6F3u};
        constexpr uint32_t shown[]={0x8D28ACCEu,0x01223ABDu,0xF446D30Eu,0x35710A5Du,
            0x31F74338u,0xF81DFFA6u,0xD9098A4Bu,0x7AA7A2A1u};
        for (uint32_t key:hidden) visibility(key,0);
        for (uint32_t key:shown) visibility(key,(requested>>5)&1);
        const auto textures=hp2::ChooseLoadableTextures(*gaugeCar,
            parameters.runtimeSettings->display.languageDependentOption);
        auto changeTexture=[&](uint32_t key,uint32_t textureKey) {
            if (auto* value=bitmap(key)) value->ChangeTexture(textureKey);
        };
        auto topLeft=[&](uint32_t key) {
            std::array<int32_t,2> point{};
            if (auto* value=object(key,0)) {
                float x,y;ui.GetTopLeft(*value,x,y,own.bindings.language);
                point={convertFloatToInt32(x),convertFloatToInt32(y)};
            }
            return point;
        };
        auto center=[&](uint32_t key,const std::array<int32_t,2>& base,
            const std::array<int16_t,2>& offset) {
            if (auto* value=bitmap(key)) ui.SetBitmapCenter(*value,
                static_cast<float>(base[0]+offset[0]),static_cast<float>(base[1]+offset[1]));
        };
        auto position=[&](uint32_t key,const std::array<int32_t,2>& base,
            const std::array<int16_t,2>& offset,int32_t dx=0,int32_t dy=0) {
            ui.SetTopLeftPosition(own.hudScreen,key,base[0]+offset[0]+dx,base[1]+offset[1]+dy);
        };
        const auto& gauges=gaugeCar->gauges;
        changeTexture(0xF446D30Eu,textures.speedometer);
        const auto speedOrigin=topLeft(0xF446D30Eu);
        center(0x8D28ACCEu,speedOrigin,gauges.speedometerCenter);
        position(0xD9098A4Bu,speedOrigin,gauges.speedTextPosition);
        visibility(0xD9098A4Bu,(requested>>5)&1);
        changeTexture(0xD9098A4Bu,textures.units);
        if (gauges.combined) {
            const auto origin=topLeft(0xF446D30Eu);
            position(0xD9098A4Bu,origin,gauges.unitsPosition,0,-30);
            visibility(0x01223ABDu,0);visibility(0x7307C47Fu,0);
            center(0x35710A5Du,origin,gauges.tachometerCenter);
            position(0x31F74338u,origin,gauges.speedTextPosition,0,-20);
            position(0x7AA7A2A1u,origin,gauges.speedTextPosition,2,-20);
            position(0xF81DFFA6u,origin,gauges.speedTextPosition,22,-20);
        } else {
            position(0xD9098A4Bu,speedOrigin,gauges.unitsPosition);
            changeTexture(0x01223ABDu,textures.tachometer);
            center(0x35710A5Du,topLeft(0x01223ABDu),gauges.tachometerCenter);
            position(0x31F74338u,speedOrigin,gauges.speedTextPosition,42,1);
            position(0xF81DFFA6u,speedOrigin,gauges.speedTextPosition,18,23);
            position(0x7AA7A2A1u,speedOrigin,gauges.speedTextPosition,16,53);
        }
        own.speedNeedle=bitmap(0x8D28ACCEu);own.tachometerNeedle=bitmap(0x35710A5Du);
        own.tachometerBitmap=bitmap(0x7307C47Fu);
        own.speedText=text(0x31F74338u);own.speedSecondText=text(0x8CD4FDA9u);
        own.gearText=text(0xF81DFFA6u);own.gearIcon=bitmap(0x7AA7A2A1u);
        if (own.tachometerBitmap) hp2::InitializeFlailerTachometer(ui,
            *own.tachometerBitmap,own.tachometerPoints);
        for (auto* value:{own.speedText,own.speedSecondText,own.gearText})
            if (value) value->SetFormatOptions(64);
    } else if (changed&0x40u) {
        const int32_t units=parameters.runtimeSettings->display.languageDependentOption;
        const auto textures=hp2::ChooseLoadableTextures(*gaugeCar,units);
        const auto texture=player->registry->count>=2 ?
            hashString(units==1?"3RDPERSONGAUGE_KMH":"3RDPERSONGAUGE_MPH") : textures.generic;
        if (auto* value=bitmap(0x9309E75Cu)) value->ChangeTexture(texture);
        for (std::size_t index=0;index<11;++index) visibility(gaugeKeys[index],0);
        constexpr uint32_t shown[]={0x9309E75Cu,0xD312F0CBu,0x39C9EEADu,
            0xD2F7FA46u,0x7BE1E0F4u,0xCF9BF6F3u};
        for (uint32_t key:shown) visibility(key,(requested>>6)&1);
        own.speedNeedle=bitmap(0xC637891Cu);own.tachometerNeedle=bitmap(0xD312F0CBu);
        own.tachometerBitmap=bitmap(0x39C9EEADu);
        own.speedText=text(0xD2F7FA46u);own.speedSecondText=text(0x70DFB037u);
        own.gearText=text(0x7BE1E0F4u);own.gearIcon=bitmap(0xCF9BF6F3u);
        if (own.tachometerBitmap) hp2::InitializeFlailerTachometer(ui,
            *own.tachometerBitmap,own.tachometerPoints);
        if (own.speedText) own.speedText->SetFormatOptions(64);
        if (own.speedSecondText) own.speedSecondText->SetFormatOptions(64);
    } else {
        // With neither analogue style selected PAL hides both sets, including
        // its repeated7307C47F call. The borrowed fan remains until teardown.
        for (uint32_t key:gaugeKeys) visibility(key,0);
    }
    if (changed&0x10000000u) {
        const int32_t visible=(requested>>28)&1;
        if (visible) {showObject(own.speedText,0);showObject(own.speedSecondText,0);}
        PlayerHudGroup(own,0xCD086C26u,visible);
        if (visible) {
            own.speedText=text(0x31F74339u);own.gearText=text(0xF81DFFA7u);own.gearIcon=bitmap(0xCF9BF6F4u);
            if (own.speedText) own.speedText->formatOptions|=64;
        }
    }
    if (changed&0x200u) {
        constexpr uint32_t keys[]={0x3405C44Eu,0x004E27A0u,0x4C769C11u,0x9A492668u,0x84BB420Fu};
        for (int32_t i=0;i<5;++i) {
            if (i!=2) visibility(keys[i],(requested>>9)&1);
            own.lapText[i]=text(keys[i]);
        }
        if (player->registry->count==2 && player->movement.seat==1) {
            const uint32_t rgb=static_cast<uint8_t>(convertFloatToInt32(playerSecondLapColor[0]))|
                (static_cast<uint32_t>(static_cast<uint8_t>(convertFloatToInt32(playerSecondLapColor[1])))<<8)|
                (static_cast<uint32_t>(static_cast<uint8_t>(convertFloatToInt32(playerSecondLapColor[2])))<<16);
            ui.SetTextColor(own.hudScreen,0x004E27A0u,rgb);
            ui.SetTextColor(own.hudScreen,0x9A492668u,rgb);
            if (own.lapText[1]) own.lapText[1]->SetAlpha(convertFloatToInt32(playerSecondLapColor[3]));
            if (own.lapText[3]) own.lapText[3]->SetAlpha(convertFloatToInt32(playerSecondLapColor[3]));
        }
    }
    if (changed&0x408u) {
        const int32_t visible=(requested&0x408u)!=0;
        PlayerHudGroup(own,0xB7A3BC7Au,0);
        own.splitText[1]=text(0xFC1B498Au);own.splitText[0]=text(0x6486623Bu);
        own.splitText[2]=text(0xA8247197u);
        if (!parameters.specialPursuit && player->registry->count==2 && visible) {
            own.splitSecondPlayer[0]=text(0xEB406FECu);own.splitSecondPlayer[1]=text(0xF7292837u);
            visibility(0xEB406FECu,1);visibility(0xF7292837u,1);
        } else {own.splitSecondPlayer[0]=nullptr;own.splitSecondPlayer[1]=nullptr;}
        for (auto* t:own.splitText) if (t) t->SetFormatOptions(64);
        if (own.splitSecondPlayer[1]) own.splitSecondPlayer[1]->SetFormatOptions(64);
        if (visible) {
            constexpr uint32_t keys[]={0xC2EA8DE7u,0x40E81BE1u,0x40E81BE2u,0x40E81BE3u,
                0x6486623Bu,0xFC1B498Au,0xA8247197u,0x0390D3DFu,0x0390D3E0u,0x0390D3E1u};
            for (uint32_t key:keys) visibility(key,1);
        }
        for (int32_t i=0;i<3;++i) own.splitLabels[i]=text(0x0390D3DFu+static_cast<uint32_t>(i));
        const bool finiteTime=parameters.timeLimitTicks!=0 && parameters.timeLimitTicks!=INT32_MAX;
        const bool timeLabel=finiteTime && !parameters.specialPursuit &&
            (parameters.raceKind==2 || parameters.pointToPoint);
        // PAL17A5FC calls the actual IsFreeRun predicate, not the separate
        // pursuit flag. Its label branch also hides the unused split value.
        const bool freeRun=raceParametersIsFreeRun(&parameters)!=0;
        const uint32_t labels[]={freeRun?0x494ACu:(parameters.pointToPoint?0x494ABu:0x494AAu),
            freeRun?0x494ADu:0x494A9u,
            !freeRun && timeLabel?0x494DAu:0x494A8u};
        for (int32_t i=0;i<3;++i) if (own.splitLabels[i]) own.splitLabels[i]->SetLanguageID(labels[i]);
        if (freeRun) {visibility(0x6486623Bu,0);own.splitText[0]=nullptr;}
    }
    if (changed&0x80u) {
        visibility(0x89C3AC59u,0);visibility(0xD1E5C713u,0);
        own.wrongWayText[0]=text(0x89C3AC59u);own.wrongWayText[1]=text(0xD1E5C713u);
    }
    if (changed&0x800200u) {
        int32_t visible=(requested>>23)&1;
        for (int32_t i=0;i<4;++i) {
            own.liveStatus[i]=text(namedKey("LIVE_SPLIT_STATUS%d",i+1));
            if (own.liveStatus[i]) own.liveStatus[i]->SetFormatOptions(64);
            own.liveSwatch[i]=bitmap(namedKey("LIVE_SPLIT_SWATCH%d",i+1));
            own.livePosition[i]=text(namedKey("LIVE_SPLIT_POS%d",i+1));
            own.liveBackground[i]=bitmap(namedKey("LIVE_SPLIT_BG%d",i+1));
        }
        own.liveTail=bitmap(0xA3E3B399u);
        auto* anchor=object((requested&0x200u)?0x363392AEu:0x363392AFu,0);
        float dx=0.0f,dy=0.0f;
        if (anchor && own.liveSwatch[0]) {
            float ax,ay,sx,sy;ui.GetTopLeft(*anchor,ax,ay,own.bindings.language);
            ui.GetTopLeft(*own.liveSwatch[0],sx,sy,own.bindings.language);dx=ax-sx;dy=ay-sy;
        }
        for (int32_t i=0;i<4;++i) {
            if (static_cast<std::size_t>(i)>=player->registry->services->race->entryCount) visible=0;
            const auto group=namedKey("LIVE_SPLIT_GROUP_%d",i+1);
            for (auto* o=ui.FindObjectByGroup(own.hudScreen,group,0);o;
                o=ui.FindObjectByGroup(own.hudScreen,group,0,o)) {
                showObject(o,visible);
                float x,y;ui.GetTopLeft(*o,x,y,own.bindings.language);o->SetTopLeftY(y+dy);
                ui.GetTopLeft(*o,x,y,own.bindings.language);o->SetTopLeftX(x+dx);
            }
        }
        showObject(own.liveTail,visible);
    }
    if (changed&2u) {
        constexpr uint32_t keys[]={0x9FAE4AF3u,0x13BE6877u,0x12420A8Au,0xDEBDE88Bu,0xB7FFBCD8u};
        for (uint32_t key:keys) visibility(key,(requested>>1)&1);
        for (int32_t i=0;i<4;++i) own.positionIcons[i]=bitmap(keys[i+1]);
    }
    // Three authored pursuit layouts select the same eight source pointers.
    // They do not create a second pursuit state, controller or texture pack.
    struct PursuitLayout {uint32_t bit,group,background,icons,labels;const char* dummy;};
    constexpr PursuitLayout layouts[]={
        {0x8000u,0x0B603727u,0x843FD334u,0xC7509D62u,0xFC401542u,"PURSUIT_DUMMY_%d"},
        {0x04000000u,0xED9FD777u,0x03B11464u,0x9BAB19D2u,0x9FF605B2u,"PURSUIT_DUMMY_HI_%d"},
        {0x08000000u,0xA19AE030u,0x79D3BABDu,0x117ABA2Bu,0x9F23220Bu,"PURSUIT_DUMMY_NIS_%d"}};
    for (const auto& layout:layouts) if (changed&layout.bit) {
        const int32_t visible=(requested&layout.bit)!=0;
        PlayerHudGroup(own,layout.group,visible);
        if (visible) {
            own.pursuitBackground=bitmap(layout.background);
            for (int32_t i=0;i<4;++i) own.pursuitIcons[i]=bitmap(layout.icons+static_cast<uint32_t>(i));
            for (int32_t i=0;i<3;++i) own.pursuitLabels[i]=bitmap(layout.labels+static_cast<uint32_t>(i));
            for (int32_t i=0;i<4;++i) {
                if (own.pursuitIcons[i]) own.pursuitIcons[i]->SetAlpha(0,-1);
                showObject(own.pursuitIcons[i],0);
                auto* dummy=bitmap(namedKey(layout.dummy,i+1));
                if (dummy) own.pursuitPositions[i]={dummy->topLeftX,dummy->topLeftY};
            }
            own.pursuitState=0;
        }
    }
    if (changed&0x20000u) {
        const int32_t visible=(requested>>17)&1;
        PlayerHudGroup(own,0x5C9C03A7u,visible);
        if (visible) {
            constexpr uint32_t keys[]={0xA3FF1482u,0xA3E99D3Cu,0x5D8266DCu,0x5D85AFBEu};
            for (int32_t i=0;i<4;++i) {own.eventText[i]=text(keys[i]);showObject(own.eventText[i],0);}
            char score[32];std::snprintf(score,sizeof(score),"%d",player->scoreState->total);
            if (own.eventText[3]) own.eventText[3]->SetString(score);
        } else for (auto*& t:own.eventText) t=nullptr;
        // PAL17B35C..17B43C publishes/clears the same Player+478/+47C/+484
        // pointers consumed by StartScoreAnimation17D370 and AddToScore17E350.
        // Construction precedes this HUD feature and intentionally binds no
        // widgets. Publish these actual authored objects to that score owner
        // here, so its animation completes and credits the displayed total.
        player->scoreState->captionText=own.eventText[0];
        player->scoreState->amountText=own.eventText[1];
        player->scoreState->totalText=own.eventText[3];
    }
    if (changed&0x40000u) {
        const int32_t visible=(requested>>18)&1;
        PlayerHudGroup(own,0x6EDAC70Cu,visible);
        if (visible) {
            constexpr uint32_t keys[]={0x3C374056u,0x6FC12A41u,0x6F2BB5BFu,0x1F87A436u};
            for (int32_t i=0;i<4;++i) {own.secondEventText[i]=text(keys[i]);showObject(own.secondEventText[i],0);}
        } else for (auto*& t:own.secondEventText) t=nullptr;
    }
    if (changed&0x10000u) PlayerHudGroup(own,0xF74C9D43u,(requested>>16)&1);
    if (changed&1u) {
        if (requested&1u) {
            own.trackMap=std::make_unique<hp2::TrackMap>(own.bindings.track,player->car,player,ui,
                *own.bindings.renderManager,*player->cameras->context->services,own.overlayViewNumber,
                parameters);
            const auto& display=parameters.runtimeSettings->display;
            int32_t mode=parameters.specialPursuit?display.pursuitTrackMapMode:
                (player->registry->count==1?display.singleViewTrackMapMode:display.splitViewTrackMapMode);
            // PAL17AC14 bypasses this coercion for pursuit;17AC2C reads
            // the HUD/input profile byte written by CycleInputProfile.
            if (!parameters.specialPursuit && player->settings.inputProfile==1 && mode==6)mode=2;
            carFramePlayerSetTrackMapMode(player,mode);
        } else own.trackMap.reset();
    }
    if (changed&4u) {
        const int32_t visible=(requested>>2)&1;
        PlayerHudGroup(own,0xC707E093u,visible);
        PlayerHudGroup(own,0x97F9252Au,visible);
        visibility(0x76494C5Bu,0);visibility(0x233DFBE1u,0);
        if (visible) {
            carFramePlayerFillRaceOverMessages(player);
            hp2::Require(worldAudioRaceFinishedSpeech(player)==WORLD_AUDIO_MESSAGES_OK,
                "Race-over finished speech queue failed");
        }
    }
    if (changed&0x4000u) {
        const int32_t visible=(requested>>14)&1;
        for (auto* value=ui.FindObjectByGroup(own.hudScreen,0xBBBB857Cu,0);value;
            value=ui.FindObjectByGroup(own.hudScreen,0xBBBB857Cu,0,value)) {
            showObject(value,visible);
            if (visible) carFramePlayerHudStartWrongWay(player);
        }
    }
    if (changed&0x2000u) {
        auto* view=ecstasyViewAt(player->cameras->context->services,own.overlayViewNumber);
        if (!view) {player->result=CAR_PLAYER_LIFECYCLE_INVALID_OWNER;return 0;}
        view->enabled=static_cast<uint8_t>((requested>>13)&1);
    }
    if (changed&0x02000000u) {
        const int32_t hidden=static_cast<int32_t>(((requested>>25)^1u)&1u);
        const int32_t count=player->registry->count;
        const auto screen=[&](uint32_t key,int32_t state){own.bindings.renderManager->SetScreenVisibility(ui,key,state);};
        if (count==1) screen(0x49C15518u,hidden);
        else if (count==2) {
            const auto* race=player->registry->services?player->registry->services->race:nullptr;
            // PAL17B734 explicitly accepts the absent pre-race Race owner.
            if (parameters.specialPursuit || (race && (race->state==1 || race->state==2)))
                screen(0x49C15518u,hidden);
            else {
                screen(0x49C15518u,1);
                screen(player->movement.seat==0?0xACCBFCB8u:0xACCBFCB9u,hidden);
            }
        }
    }
    if (changed&0x80000u) {
        const int32_t visible=(requested>>19)&1;
        bool first=true;
        const bool single=player->registry->count==1;
        bool keepLow=false;
        if (single) {own.flailerMoved=1;keepLow=(changed&0x1000u)!=0;}
        for (int32_t i=0;i<12;++i) own.wantedIcons[i]=bitmap(namedKey("RADAR_LIGHT%d",i+1));
        for (auto* o=ui.FindObjectByGroup(own.hudScreen,0x062743F5u,0);o;
            o=ui.FindObjectByGroup(own.hudScreen,0x062743F5u,0,o)) {
            showObject(o,visible);
            if (!single) continue;
            if (own.flailerOriginalTop==0.0f && own.wantedIcons[0])
                own.flailerOriginalTop=PlayerObjectTop(own,own.wantedIcons[0]);
            if (first) {
                if (PlayerObjectTop(own,own.wantedIcons[0])==own.flailerOriginalTop) own.flailerMoved=0;
                first=false;
            }
            if (own.flailerMoved) {
                if (keepLow) o->SetTopLeftY(PlayerObjectTop(own,o)+70.0f);
            } else if (!keepLow) o->SetTopLeftY(PlayerObjectTop(own,o)-70.0f);
        }
    }
    if (changed&0x100000u) {
        PlayerHudGroup(own,0xC258F297u,(requested>>20)&1);
        for (int32_t i=0;i<7;++i) own.tracker[i]=bitmap(namedKey("TRACKER_%d",i));
    }
    if (changed&0x01000000u) {
        PlayerHudGroup(own,0x068E2C75u,(requested>>24)&1);
        for (int32_t i=0;i<5;++i) own.damageLights[i]=bitmap(namedKey("DAMAGE_LIGHT%d",i+1));
        for (int32_t i=0;i<4;++i) own.pursuitCrosses[i]=bitmap(namedKey("BETHECOP_X_%d",i+1));
        for (int32_t i=0;i<3;++i) {
            const auto chopper=namedKey("HELICOPTERICON%d",i+1),cop=namedKey("COPCAR_ICON%d",i+1);
            if (player->movement.seat==0) {own.chopperIcons[i]=bitmap(chopper);own.copCarIcons[i]=bitmap(cop);}
            else {visibility(chopper,0);visibility(cop,0);}
        }
        own.pursuitTime=text(hashString("BETHECOP_TIME"));
        if (own.pursuitTime) own.pursuitTime->SetFormatOptions(64);
    }
    // PAL17BC7C..17BCB8 reads gEasterEggs.records+0x110:
    // record4.unlocked (4*0x38+0x30), not a HUD/profile enable flag.
    ui.SetGroupVisibility(own.hudScreen,0x06D1F9FBu,unlocks.records[4].unlocked?1:0);
    if ((changed&0x200000u) && carNisOverlaySetVisible(player->nisOverlay,
        requested&0x200000u,&parameters,player->registry->sessionState)!=CAR_NIS_OK) {
        player->result=CAR_PLAYER_LIFECYCLE_NIS_FAILURE;return 0;
    }
    own.hudFeatures=requested;
    player->result=CAR_PLAYER_LIFECYCLE_OK;
    return 1;
}

#include "indep/src/speech/SEDWedge.hpp"
#include <cmath>

void carFramePlayerFinish(CarFramePlayerState* player)
{
    if(player->drivingLocked)return;
    // PAL17666C reads worldTimeFrames at330C84; the finish delay uses
    // the same 60Hz world frames and stops while the World is paused.
    player->drivingLocked=player->registry->clock->worldTimeFrames;
    const CarFramePlayerServices* services=player->registry->services;
    if(!services||!services->race){player->result=1;return;}
    const RaceFrame* race=services->race;
    const RaceCarEntry* entry=player->car->bindings.owner->raceEntry;
    if(!entry){player->result=1;return;}
    if(entry->furthestLap==race->targetLapCount){
        if(!*player->registry->pursuitEnabled){
            if(!services->replay){player->result=1;return;}
            int32_t nearby=0;
            const float* position=entry->car->position;
            for(size_t index=0;index<race->entryCount;++index){
                const float* other=race->entries[index]->car->position;
                const float x=position[0]-other[0];
                const float y=position[1]-other[1];
                const float z=position[2]-other[2];
                const float xx=x*x;
                const float yy=y*y;
                const float zz=z*z;
                if(sqrtf((xx+yy)+zz)<30.0f)++nearby;
            }
            const float others=(float)nearby-1.0f;
            const float crowded=others*7.5f;
            replayManagerAddHighlight(services->replay,crowded+10.0f);
        }
        if(carFramePlayerAllDrivingLocked(player->registry)&&services->streamedAudio){
            worldStreamAudioCancel(services->streamedAudio);
            if(services->streamedAudio->result!=WORLD_STREAM_AUDIO_OK)player->result=1;
        }
        if(!services->audio||worldAudioPlayInGame(services->audio,"FinishLine",1.0f,0.0f,0.0f)!=WORLD_AUDIO_OK)
            player->result=1;
    }
    player->finishCounter=360;
}

// PAL177C80 / beta PlayerClearZone. The World timer borrows this Player;
// its snapshot translates that reference through the actual player number.
void carFramePlayerCinematicTimeout(void* context)
{
    carFramePlayerEndState(static_cast<CarFramePlayerState*>(context),1);
}

// PAL175DB0..175F70. Transfer order and widths are the original Player
// stream, including the nested Average and NisManager sections. Neither
// HUD state nor a second copy of the controlled Car is serialized here.
void carFramePlayerDoSnapshot(CarFramePlayerState* player,ReplaySnapshot* snapshot)
{
    if (!snapshot)return;
    if (!player || !player->registry || !player->registry->world ||
        !player->nisSelection || !player->replay || !player->speedHistory.samples) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        return;
    }
    replaySnapshotBeginSection(snapshot,player,"Player");
    replaySnapshotMoveInteger(snapshot,&player->drivingLocked,4);
    replaySnapshotMoveInteger(snapshot,&player->finishCounter,2);
    replaySnapshotMoveInteger(snapshot,&player->movement.steeringFeedbackBusy,1);
    replaySnapshotMoveData(snapshot,player->cinematicStateFlags,8,8,0);
    replaySnapshotMoveData(snapshot,player->stateStartedTicks,32,-1,0);
    replaySnapshotMoveInteger(snapshot,&player->snapshotRetainedFlag,1);
    driverHistorySnapshot(&player->speedHistory,snapshot);
    replaySnapshotMoveInteger(snapshot,&player->speedRatioCountdownTicks,4);
    replaySnapshotMoveFloat(snapshot,&player->maximumSpeedRatio);
    replaySnapshotMoveInteger(snapshot,&player->crashTicks,4);
    replaySnapshotMoveInteger(snapshot,&player->recoveryProtectionTicks,4);
    replaySnapshotMoveFloat(snapshot,&player->previousSpeedMph);
    replaySnapshotMoveInteger(snapshot,&player->speedSampleTicks,4);
    replaySnapshotMoveData(snapshot,&player->snapshotRetainedWord,4,-1,0);
    replaySnapshotMoveInteger(snapshot,&player->nisSelection->cursors[1],4);
    replaySnapshotMoveInteger(snapshot,&player->nisSelection->cursors[0],4);

    // The original serialized a32-bit function pointer. Its sole actual
    // producer2271F0..227204 supplies ResetParticipant callback225868.
    // Encode that source identity, never a truncated64-bit host address.
    uint32_t completion=0;
    if (player->nisCompletion==CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET)
        completion=0x00225868u;
    else if (player->nisCompletion!=CAR_PLAYER_NIS_COMPLETION_NONE) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        replaySnapshotEndSection(snapshot);
        return;
    }
    replaySnapshotMoveData(snapshot,&completion,4,-1,0);
    if (snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        if (completion==0)player->nisCompletion=CAR_PLAYER_NIS_COMPLETION_NONE;
        else if (completion==0x00225868u)
            player->nisCompletion=CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET;
        else snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
    }
    replaySnapshotMoveInteger(snapshot,&player->nisCompletionIndex,4);
    if (snapshot->result==REPLAY_SNAPSHOT_OK &&
        carLiveReplayDoSnapshot(player->replay,snapshot,player->registry->world)!=CAR_NIS_OK)
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
    replaySnapshotMoveData(snapshot,&player->wrongWay,1,4,0);
    replaySnapshotMoveData(snapshot,&player->lookBack,1,4,0);
    replaySnapshotMoveData(snapshot,&player->transitionAudio,1,4,0);
    if (snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        carFramePlayerSetInputMode(player,0);
        if (player->result)snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
    }
    replaySnapshotMoveFloat(snapshot,&player->cinematicCountdown);
    replaySnapshotEndSection(snapshot);
}

// RequestZone PAL177C88..178354 and ClearZone PAL178358..17846C.
// Complete original Player.cpp paths; the former disconnected adapter is removed.
static float secondsSince(int32_t now, int32_t then)
{
    const int32_t ticks = (int32_t)((uint32_t)now - (uint32_t)then);
    return (float)ticks * 0.0002500000118743628f;
}

static int32_t suppressesCinematicCamera(const CarFramePlayerState* player)
{
    const int32_t state = player->registry->services->session->state;
    return state == 4 || state == 7 || state == 5 || state == 9;
}

static int32_t changeCamera(CarFramePlayerState* player, int32_t kind)
{
    if (carFramePlayerChangeCamera(player, kind, 1) == CAR_CINEMATIC_OK) return 1;
    player->result = 1;
    return 0;
}

static void playPreviewRejected(CarFramePlayerState* player)
{
    const CarFramePlayerServices* services = player->registry->services;
    if (playerCinematicVoiceRejectPreview(services->audio, services->audioManager,
        carFramePlayerSoundEnabled(player->registry)) != WORLD_AUDIO_OK) player->result = 1;
}

int32_t carFramePlayerStartState(CarFramePlayerState* player, int32_t state,
    int32_t forced, int32_t playAudio, int32_t transitionAudio)
{
    if (!player || !player->registry || !player->replay) {
        if (player) player->result = 1;
        return 0;
    }
    if (carFramePlayerReplayActive(player)) return 0;
    CarFramePlayerRegistry* registry = player->registry;
    CarFramePlayerServices* services = registry->services;
    if (!services || !services->session || !registry->clock || !registry->world) {
        player->result = 1;
        return 0;
    }
    if (services->session->state == 8) return 0;
    /* Out-of-range requests are rejected before native array indexing. */
    if (state < 0 || state >= 8) return 0;
    const int32_t now = registry->clock->worldTimer;
    const int32_t previousTicks = player->stateStartedTicks[forced ? 0 : state];
    if (previousTicks && previousTicks != INT_MAX &&
        secondsSince(now, previousTicks) < (forced ? 0.10000000149011612f : 5.0f)) return 0;
    if (!services->race || services->race->state != 4 || registry->count == 2) return 0;
    if (state >= 1 && state <= 7) {
        if (!registry->displaySettings) { player->result = 1; return 0; }
        // PAL177DD4..177E10 reads the actual RaceSettings display bytes
        // 31AB8A/31AB8B. Pause options and FRED update this same owner.
        if (state >= 6 && !registry->displaySettings->authoredOption6) return 0;
        if (state <= 5 && !registry->displaySettings->authoredOption7) return 0;
    }
    if (player->movement.steeringFeedbackBusy == 1 || player->movement.steeringFeedbackBusy == 2)
        carFramePlayerEndState(player, 0);
    if (registry->count < 0 || registry->count > 2) {
        player->result = 1;
        return 0;
    }
    for (int32_t index = 0; index < registry->count; ++index) {
        const CarFramePlayerState* other = registry->players[index];
        if (!other) { player->result = 1; return 0; }
        const int32_t active = other->movement.steeringFeedbackBusy;
        if (active >= 3 && active <= 7) return 0;
    }
    player->inputActionRealTimer = registry->clock->realTimer;
    if (!forced) player->stateStartedTicks[state] = now;
    if (!carFramePlayerClearTransientPresentation(player)) return 0;
    float gain = 1.0f;
    float pitchScale = 1.0f;
    const char* cue = "Wind";
    switch (state) {
    case 1:
        if (suppressesCinematicCamera(player)) return 0;
        if (!changeCamera(player, 13)) return 0;
        worldSetPaused(&registry->world->timing,
            carFramePlayerCurrentCameraDuration(player) - registry->clock->worldTimeElapsed,
            carFramePlayerCinematicTimeout, player);
        break;
    case 2:
        if (suppressesCinematicCamera(player)) return 0;
        // PAL177F98 rejects this request before route/camera allocation.
        // Use the same sound-enabled feedback as a failed route preview.
        if (player->wrongWay) {
            playPreviewRejected(player);
            return 0;
        }
        if (!carFramePlayerPrepareCrashCamera(player)) {
            playPreviewRejected(player);
            return 0;
        }
        if (!changeCamera(player, 14)) return 0;
        if (carFramePlayerValidateCurrentCrashCamera(player)) {
            playPreviewRejected(player);
            if (carFramePlayerPopCamera(player) != CAR_CINEMATIC_OK) player->result = 1;
            return 0;
        }
        gain = 0.0f;
        cue = "PreviewZone";
        worldSetPaused(&registry->world->timing,
            carFramePlayerCurrentCameraDuration(player) - registry->clock->worldTimeElapsed,
            carFramePlayerCinematicTimeout, player);
        break;
    case 3:
        worldSetTimeScale(&registry->world->timing, 0.3330000042915344f, 8.0f,
            carFramePlayerCinematicTimeout, player);
        cue = "SlomoZone";
        break;
    case 4:
        worldSetTimeScale(&registry->world->timing, 0.3330000042915344f, 2.0f,
            carFramePlayerCinematicTimeout, player);
        cue = "SlomoZone";
        break;
    case 5:
        worldSetTimeScale(&registry->world->timing, 2.0f, 4.0f, carFramePlayerCinematicTimeout, player);
        break;
    case 6:
        worldSetTimeScale(&registry->world->timing, 0.4000000059604645f, 8.0f,
            carFramePlayerCinematicTimeout, player);
        pitchScale = 0.8999999761581421f;
        cue = "SlomoZone";
        if (!suppressesCinematicCamera(player) && !changeCamera(player, 15)) return 0;
        carFramePlayerClearPreview(player);
        break;
    case 7:
        worldSetTimeScale(&registry->world->timing, 0.4000000059604645f, 8.0f,
            carFramePlayerCinematicTimeout, player);
        pitchScale = 0.8999999761581421f;
        cue = "SlomoZone";
        if (!suppressesCinematicCamera(player) && !changeCamera(player, 16)) return 0;
        break;
    default:
        break;
    }
    if (!services->radialBlur) { player->result = 1; return 0; }
    screenRadialBlurStart(services->radialBlur, player->movement.seat);
    player->movement.steeringFeedbackBusy = state;
    player->transitionAudio = transitionAudio;
    player->transitionAudioStrength = transitionAudio ? 0.0f : 1.0f;
    WorldAudioVoiceRef voice = {player->cinematicVoice, player->cinematicVoiceGeneration};
    if (playerCinematicVoiceStart(services->audio, services->audioManager,
        carFramePlayerSoundEnabled(registry), playAudio, &voice, cue, gain, pitchScale) != WORLD_AUDIO_OK)
        player->result = 1;
    else {
        player->cinematicVoice = voice.instance;
        player->cinematicVoiceGeneration = voice.generation;
    }
    return 1;
}

void carFramePlayerEndState(CarFramePlayerState* player, int32_t stopAudio)
{
    if (!player) return;
    const int32_t state = player->movement.steeringFeedbackBusy;
    if (!state) return;
    CarFramePlayerRegistry* registry = player->registry;
    if (!registry || !registry->services || !registry->services->session ||
        !registry->services->radialBlur || !registry->clock || !registry->world) {
        player->result = 1;
        return;
    }
    screenRadialBlurStop(registry->services->radialBlur, player->movement.seat);
    if ((state >= 1 && state <= 2) || (state >= 6 && state <= 7)) {
        if (state <= 2) {
            // PAL178394 reads the canonical GP-5664 cell (33028C),
            // shared with the World input-pause gate; it is not a binding.
            if (!worldInputPauseGate) player->stateStartedTicks[0] = registry->clock->worldTimer;
        }
        if (!suppressesCinematicCamera(player) &&
            carFramePlayerPopCamera(player) != CAR_CINEMATIC_OK) player->result = 1;
    }
    worldResetTimeScale(&registry->world->timing);
    player->previousCinematicState = state;
    player->movement.steeringFeedbackBusy = 0;
    player->transitionAudio = 0;
    player->transitionAudioStrength = 0.0f;
    if (stopAudio) carFramePlayerStopStateAudio(player);
    carFramePlayerReconcileInput(player);
}

/* Player camera and input. */
void carFramePlayerAirborneSettingsInitialize(CarFramePlayerAirborneSettings* settings)
{
    settings->lensStart = 20.0f;
    settings->lensStep = 0.25f;
}

void carFramePlayerAirbornePathInitialize(CarFramePlayerAirbornePath* path)
{
    memset(path, 0, sizeof(*path));
    path->items.projection.lensKeys = (const CarCameraPoint*)path->lens;
    path->items.eyeKeys = (const CarCameraPoint*)path->eye;
    path->items.rollKeys = (const CarCameraPoint*)path->roll;
}

static float playerAirborneRouteFraction(const CarMovement* movement, const RaceParameters* race)
{
    const float percent = movement->routeLocation.progress * 100.0f;
    const float directed = race->reverse ? 100.0f - percent : percent;
    return directed * 0.009999999776482582f;
}

static void playerAirborneClearOffset(CarFramePlayerAirbornePath* path)
{
    memset(&path->offset, 0, sizeof(path->offset));
    path->offset.count = 2;
}

static float playerAirborneForwardProjection(const float point[4], const VehicleState* state)
{
    const float x = (point[0] - state->position[0]) * state->velocity[0];
    const float y = (point[1] - state->position[1]) * state->velocity[1];
    const float z = (point[2] - state->position[2]) * state->velocity[2];
    return (x + y) + z;
}

CarCinematicResult carFramePlayerAirbornePathEnter(CarFramePlayerAirbornePath* path,
    const CarMovement* movement, const CarRoadPreview* preview, TerrainTopology* terrain,
    const RaceParameters* race, const CarFramePlayerAirborneSettings* settings)
{
    if (!path || !movement || !movement->bindings.state || !movement->bindings.state->position ||
        !movement->bindings.route ||
        !preview || !preview->steps || preview->count < 3 || !terrain || !race || !settings)
        return CAR_CINEMATIC_INVALID_BINDING;

    const int32_t last = preview->count - 1;
    const int32_t endIndex = last < 5 ? last : preview->count - 3;
    const CarRecoveryPose* endPose = preview->steps[endIndex].pose;
    const CarRecoveryPose* startPose = preview->steps[2].pose;
    if (!endPose || !startPose) return CAR_CINEMATIC_NO_ROAD_PATH;

    float routePoint[4];
    driverRoadPointPosition(endPose, routePoint, 0.0f, 0.0f);
    TrackRouteLocation location;
    trackRouteLocationAtPoint(movement->bindings.route, &location, routePoint);

    carFramePlayerAirbornePathInitialize(path);
    CarCameraPathItems* items = &path->items;
    items->playback.routeStart = items->projection.routeStart = playerAirborneRouteFraction(movement, race);
    const float end = race->reverse ? 1.0f - location.progress : location.progress;
    items->playback.routeEnd = items->projection.routeEnd = end;
    items->playback.eyeUsesRouteFraction = 1;
    items->nearClip = 0.5f;
    items->farClip = 10000.0f;
    items->eyeCount = 2;
    items->legacy.start[0] = 0.20000000298023224f;
    for (int32_t index = 0; index < 32; ++index)
        path->lens[index][0] = settings->lensStart;

    const VehicleState* state = movement->bindings.state;
    float right[4], left[4];
    driverRoadPointPosition(endPose, right, 0.0f, endPose->halfWidths[1]);
    driverRoadPointPosition(endPose, left, 0.0f, -endPose->halfWidths[0]);
    if (playerAirborneForwardProjection(left, state) < playerAirborneForwardProjection(right, state)) {
        memcpy(path->eye[0], left, sizeof(left));
        driverRoadPointPosition(startPose, path->eye[2], 0.0f, startPose->halfWidths[1]);
        items->legacy.start[1] = 0.4000000059604645f;
    } else {
        memcpy(path->eye[0], right, sizeof(right));
        driverRoadPointPosition(startPose, path->eye[2], 0.0f, -startPose->halfWidths[0]);
        items->legacy.start[1] = -0.4000000059604645f;
    }
    path->eye[0][2] += 1.0f;
    path->eye[2][2] += 1.0f;

    TerrainSurface surface;
    terrainSurfaceInitialize(&surface, terrain);
    const float elevation = terrainSurfaceElevation(&surface, path->eye[0], NULL, NULL, NULL);
    if (path->eye[0][2] < elevation) path->eye[0][2] = elevation + 1.0f;
    terrainSurfaceDispose(&surface);
    memcpy(path->eye[1], path->eye[0], sizeof(path->eye[0]));
    playerAirborneClearOffset(path);
    path->prepared = 1;
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerAirbornePathExit(CarFramePlayerAirbornePath* path,
    const CarMovement* movement, TerrainTopology* terrain, const RaceParameters* race,
    const CarFramePlayerAirborneSettings* settings)
{
    if (!path || !movement || !movement->bindings.state || !movement->bindings.state->position ||
        !terrain || !race || !settings)
        return CAR_CINEMATIC_INVALID_BINDING;
    if (!path->prepared) return CAR_CINEMATIC_NO_ROAD_PATH;

    CarCameraPathItems* items = &path->items;
    items->playback.routeStart = items->projection.routeStart = playerAirborneRouteFraction(movement, race);
    const float carHeight = movement->bindings.state->position[2];
    if (path->eye[2][2] < carHeight) path->eye[2][2] = carHeight;

    TerrainSurface surface;
    terrainSurfaceInitialize(&surface, terrain);
    const float elevation = terrainSurfaceElevation(&surface, path->eye[2], NULL, NULL, NULL);
    if (path->eye[2][2] < elevation) path->eye[2][2] = elevation + 1.0f;
    terrainSurfaceDispose(&surface);
    memcpy(path->eye[0], path->eye[2], sizeof(path->eye[0]));
    memcpy(path->eye[1], path->eye[2], sizeof(path->eye[1]));
    items->legacy.start[1] = -items->legacy.start[1];
    float parameter = 0.0f;
    for (int32_t index = 0; index < 32; ++index) {
        const float offset = parameter * settings->lensStep;
        path->lens[index][0] = settings->lensStart - offset;
        path->lens[index][1] = path->lens[index][2] = path->lens[index][3] = 0.0f;
        parameter += 1.0f;
    }
    playerAirborneClearOffset(path);
    return CAR_CINEMATIC_OK;
}

static CarCinematicResult playerCameraFailure(CarFramePlayerState* player,CarCinematicResult status)
{
    if(player&&status!=CAR_CINEMATIC_OK)player->result=1;
    return status;
}

static CarCinematicResult playerCameraDisposeCamera(CarPlayerOwnedCamera* camera)
{
    switch(camera->family){
    case CAR_PLAYER_CAMERA_NONE:return CAR_CINEMATIC_OK;
    case CAR_PLAYER_CAMERA_CHASE:carChaseCameraDispose(camera->owner.chase);break;
    case CAR_PLAYER_CAMERA_FIXED:carFixedCameraDispose(camera->owner.fixed);break;
    case CAR_PLAYER_CAMERA_DRIVE:carDriveCameraDispose(camera->owner.drive);break;
    case CAR_PLAYER_CAMERA_ORBIT:carOrbitCameraDispose(camera->owner.orbit);break;
    case CAR_PLAYER_CAMERA_ZONE:carZoneFreezeCameraDispose(camera->owner.zone);break;
    case CAR_PLAYER_CAMERA_CRASH:carCrashCameraDispose(camera->owner.crash);break;
    case CAR_PLAYER_CAMERA_AIRBORNE:carAirborneCameraDispose(camera->owner.airborne);break;
    case CAR_PLAYER_CAMERA_VELOCITY:carVelocityCameraDispose(camera->owner.velocity);break;
    default:return CAR_CINEMATIC_INVALID_BINDING;
    }
    memset(camera,0,sizeof(*camera));
    return CAR_CINEMATIC_OK;
}

static void playerCameraActivateCamera(CarPlayerOwnedCamera* camera)
{
    if(!camera->base)return;
    if(camera->family==CAR_PLAYER_CAMERA_CRASH)carCrashCameraActivate(camera->owner.crash);
    else carCinematicBaseActivate(camera->base);
}

static void playerCameraDeactivateCamera(CarPlayerOwnedCamera* camera)
{
    if(!camera->base)return;
    if(camera->family==CAR_PLAYER_CAMERA_CRASH)carCrashCameraDeactivate(camera->owner.crash);
    else carCinematicBaseDeactivate(camera->base);
}

static CarCameraReference playerCameraCameraReference(const CarPlayerOwnedCamera* camera)
{
    CarCameraReference reference;
    if(camera->family==CAR_PLAYER_CAMERA_CHASE){
        reference.kind=CAR_CAMERA_REFERENCE_CHASE;
        reference.owner.chase=camera->owner.chase;
    }else{
        reference.kind=CAR_CAMERA_REFERENCE_BASE;
        reference.owner.base=camera->base;
    }
    return reference;
}

static CarMovement* playerCameraAttachedMovement(const CarFramePlayerState* player)
{
    if(player->car)return player->car;
    const CarFramePlayerCameraContext* context=player->cameras->context;
    return context->vehicleCount&&context->chaseVehicles?context->chaseVehicles[0].movement:NULL;
}

CarCinematicResult carFramePlayerCamerasInitialize(CarFramePlayerState* player,
    const CarFramePlayerCameraContext* context,int32_t viewNumber,
    int32_t overlayViewNumber,int32_t dynamicResponse)
{
    if(!player||player->cameras||!context||!context->services||!context->services->clock||
       viewNumber<0||viewNumber>3||!context->services->screens[viewNumber].view||
       overlayViewNumber<0||overlayViewNumber>3||
       !context->services->screens[overlayViewNumber].view)
        return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras=static_cast<CarFramePlayerCameras*>(calloc(1,sizeof(*cameras)));
    if(!cameras)return playerCameraFailure(player,CAR_CINEMATIC_ALLOCATION_FAILURE);
    cameras->context=context;
    cameras->viewNumber=viewNumber;
    cameras->overlayViewNumber=overlayViewNumber;
    cameras->dynamicResponse=dynamicResponse;
    carFramePlayerAirbornePathInitialize(&cameras->airbornePath);
    player->cameras=cameras;
    return CAR_CINEMATIC_OK;
}

static CarCinematicResult playerCameraClearViewMovers(CarFramePlayerState* player)
{
    CarFramePlayerCameras* cameras=player->cameras;
    if(cameras->scripted){carViewMoverDispose(cameras->scripted);cameras->scripted=NULL;}
    if(cameras->countdown){carViewMoverDispose(cameras->countdown);cameras->countdown=NULL;}
    if(cameras->replay){
        if(!player->replay||carViewMoverReplayRemove(player->replay,cameras->replay)!=CAR_VIEW_MOVER_OK)
            return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
        cameras->replay=NULL;
    }
    return CAR_CINEMATIC_OK;
}

int32_t carFramePlayerSelectCameraView(CarFramePlayerState* player,int32_t requested)
{
    if(!player||!player->cameras){playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);return -1;}
    int32_t index=requested%4;
    CarFramePlayerCameras* cameras=player->cameras;
    if(cameras->current.family!=CAR_PLAYER_CAMERA_CHASE)return index;
    const CarFramePlayerCameraContext* context=cameras->context;
    const CarChaseCamera* chase=cameras->current.owner.chase;
    const int32_t* allowAll=context->input?context->input->allowAllViewKinds:NULL;
    if(index<0||!allowAll||(!*allowAll&&!context->viewCategory)){
        playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);return -1;
    }
    int32_t inspected=0;
    for(;inspected<4;++inspected,index=(index+1)%4){
        const CarCameraView* view=carCameraViewAt(chase->catalogue,
            context->services->presentationLayout,index);
        if(view && view->kind==1 && hp2::applicationNativeGameplay.cockpitView) {
            const auto* car=chase->movement?chase->movement->bindings.owner:nullptr;
            const auto* dashboard=carRenderModel(car,21);
            // The PC switch never opens an empty cockpit, including when a
            // chase camera is attached to a different AI/replay vehicle.
            if(!dashboard || !dashboard->solid)continue;
        }
        if(view&&(*allowAll||*context->viewCategory!=0||view->kind==0||view->kind==2))break;
    }
    if(inspected==4)index=0;
    carChaseCameraSetViewIndex(cameras->current.owner.chase,index);
    return index;
}

static CarCinematicResult playerCameraCreateCamera(CarFramePlayerState* player,int32_t requestedKind)
{
    CarFramePlayerCameras* cameras=player->cameras;
    const CarFramePlayerCameraContext* context=cameras->context;
    CarPlayerOwnedCamera* camera=&cameras->current;
    CarMovement* movement=playerCameraAttachedMovement(player);
    CarCinematicResult status=CAR_CINEMATIC_INVALID_BINDING;
    if(requestedKind==0)return CAR_CINEMATIC_OK;
    if(!movement||!movement->bindings.state)return playerCameraFailure(player,status);
    switch(requestedKind){
    case 1:{
        CarChaseCameraContext binding{};
        binding.services=context->services;
        binding.terrain=context->terrain;
        binding.player=player;
        binding.movement=movement;
        binding.vehicles=context->chaseVehicles;
        binding.vehicleCount=context->vehicleCount;
        binding.defaultCatalogue=context->defaultCatalogue;
        binding.simulation=context->simulation;
        binding.shake=context->shake;
        binding.settings=context->chaseSettings;
        binding.viewNumber=cameras->viewNumber;
        binding.viewIndex=player->settings.cameraView;
        binding.dynamicResponse=cameras->dynamicResponse;
        binding.facingRate=300.8999938964844f;
        binding.extendedVisibility=1;
        camera->owner.chase=carChaseCameraCreate(&binding,&status);
        if(!camera->owner.chase)break;
        camera->family=CAR_PLAYER_CAMERA_CHASE;
        camera->base=carChaseCameraBase(camera->owner.chase);
        carChaseCameraSetFacingTarget(camera->owner.chase,player->lookBack!=0);
        const int32_t selected=carFramePlayerSelectCameraView(player,player->settings.cameraView);
        if(selected<0)return CAR_CINEMATIC_INVALID_BINDING;
        player->settings.cameraView=(int8_t)selected;
        break;
    }
    case 2:{
        CarFixedCameraContext binding={context->services,context->input,context->terrain,
            context->fixedCommands,cameras->viewNumber,1};
        float eye[4];
        memcpy(eye,movement->bindings.state->position,sizeof(eye));
        eye[2]=eye[2]+4.0f;
        const float target[4]={0,0,1,0};
        camera->owner.fixed=carFixedCameraCreate(&binding,eye,target,&status);
        if(camera->owner.fixed){camera->family=CAR_PLAYER_CAMERA_FIXED;camera->base=carFixedCameraBase(camera->owner.fixed);}
        break;
    }
    case 3:
        camera->owner.drive=carDriveCameraCreate(context->services,cameras->viewNumber,context->editor,&status);
        if(camera->owner.drive){camera->family=CAR_PLAYER_CAMERA_DRIVE;camera->base=carDriveCameraBase(camera->owner.drive);}
        break;
    case 13:{
        CarZoneFreezeCameraContext binding={context->services,context->terrain,player,movement,
            playerCameraCameraReference(&cameras->suspended[0]),cameras->viewNumber};
        camera->owner.zone=carZoneFreezeCameraCreate(&binding,&status);
        if(camera->owner.zone){camera->family=CAR_PLAYER_CAMERA_ZONE;camera->base=&camera->owner.zone->base;}
        break;
    }
    case 14:{
        CarCrashCameraContext binding={context->services,context->simulation,context->terrain,player,cameras->viewNumber};
        camera->owner.crash=carCrashCameraCreate(&binding,&cameras->crashPreview,&status);
        if(camera->owner.crash){camera->family=CAR_PLAYER_CAMERA_CRASH;camera->base=&camera->owner.crash->base;}
        carRoadPreviewDispose(&cameras->crashPreview);
        break;
    }
    case 15:
    case 16:{
        if(requestedKind==15)
            status=carFramePlayerAirbornePathEnter(&cameras->airbornePath,movement,&player->airbornePreview,
                context->terrain,context->race,context->airbornePathSettings);
        else
            status=carFramePlayerAirbornePathExit(&cameras->airbornePath,movement,
                context->terrain,context->race,context->airbornePathSettings);
        if(status!=CAR_CINEMATIC_OK)return playerCameraFailure(player,status);
        CarAirborneCameraContext binding={context->services,player,movement,context->race,
            &cameras->airbornePath.items,&cameras->airbornePath.offset,context->airborneSettings,
            cameras->viewNumber,requestedKind==16};
        camera->owner.airborne=carAirborneCameraCreate(&binding,&status);
        if(camera->owner.airborne){camera->family=CAR_PLAYER_CAMERA_AIRBORNE;camera->base=carAirborneCameraBase(camera->owner.airborne);}
        break;
    }
    case 17:{
        CarVelocityCameraContext binding={context->services,movement,context->velocitySettings,player,cameras->viewNumber};
        camera->owner.velocity=carVelocityCameraCreate(&binding,&status);
        if(camera->owner.velocity){camera->family=CAR_PLAYER_CAMERA_VELOCITY;camera->base=carVelocityCameraBase(camera->owner.velocity);}
        break;
    }
    default:{
        CarOrbitCameraContext binding={context->services,context->input,context->movements,
            context->vehicleCount,context->simulation,context->orbitSettings,cameras->viewNumber,1};
        camera->owner.orbit=carOrbitCameraCreateForCar(&binding,movement->bindings.owner,&status);
        if(camera->owner.orbit){camera->family=CAR_PLAYER_CAMERA_ORBIT;camera->base=carOrbitCameraBase(camera->owner.orbit);}
        break;
    }
    }
    if(camera->base)camera->base->player=player;
    return playerCameraFailure(player,status);
}

CarCinematicResult carFramePlayerChangeCamera(CarFramePlayerState* player,
    int32_t requestedKind,int32_t remember)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras=player->cameras;
    CarCinematicResult status=playerCameraClearViewMovers(player);
    if(status!=CAR_CINEMATIC_OK)return status;
    if(cameras->current.base&&cameras->current.base->kind==requestedKind)return CAR_CINEMATIC_OK;
    if(cameras->current.base){
        if(remember){
            /* Real Player transitions have at most two suspended cameras.
             * Reject a host misuse rather than lose an owned allocation. */
            if(cameras->suspended[1].base)return playerCameraFailure(player,CAR_CINEMATIC_BACKEND_CAPACITY);
            cameras->suspended[1]=cameras->suspended[0];
            cameras->suspended[0]=cameras->current;
            playerCameraDeactivateCamera(&cameras->suspended[0]);
            memset(&cameras->current,0,sizeof(cameras->current));
        }else{
            status=playerCameraDisposeCamera(&cameras->current);
            if(status!=CAR_CINEMATIC_OK)return playerCameraFailure(player,status);
        }
    }
    return playerCameraCreateCamera(player,requestedKind);
}

CarCinematicResult carFramePlayerPopCamera(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras=player->cameras;
    if(!cameras->suspended[0].base)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarCinematicResult status=playerCameraDisposeCamera(&cameras->current);
    if(status!=CAR_CINEMATIC_OK)return playerCameraFailure(player,status);
    cameras->current=cameras->suspended[0];
    cameras->suspended[0]=cameras->suspended[1];
    memset(&cameras->suspended[1],0,sizeof(cameras->suspended[1]));
    if(cameras->current.family==CAR_PLAYER_CAMERA_CHASE)
        carChaseCameraSetFacingTarget(cameras->current.owner.chase,player->lookBack!=0);
    playerCameraActivateCamera(&cameras->current);
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerClearCameras(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarCinematicResult status=carFramePlayerChangeCamera(player,0,0);
    if(status!=CAR_CINEMATIC_OK)return status;
    for(int index=0;index<2;++index){
        status=playerCameraDisposeCamera(&player->cameras->suspended[index]);
        if(status!=CAR_CINEMATIC_OK)return playerCameraFailure(player,status);
    }
    return CAR_CINEMATIC_OK;
}

void carFramePlayerCamerasDispose(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return;
    if(carFramePlayerClearCameras(player)!=CAR_CINEMATIC_OK)return;
    CarFramePlayerCameras* cameras=player->cameras;
    carAttachedCameraDispose(cameras->overlay);
    carRoadPreviewDispose(&cameras->crashPreview);
    free(cameras);
    player->cameras=NULL;
}

namespace {
void ShutdownPlayerCameras(CarFramePlayerState* player,PlayerShutdownErrors& errors) noexcept
{
    auto* cameras=player?player->cameras:nullptr;
    if(!cameras)return;
    errors.Run([&]{carViewMoverDispose(cameras->scripted);cameras->scripted=nullptr;});
    errors.Run([&]{carViewMoverDispose(cameras->countdown);cameras->countdown=nullptr;});
    const auto close=[&](CarPlayerOwnedCamera& camera) {
        errors.Run([&]{hp2::Require(playerCameraDisposeCamera(&camera)==CAR_CINEMATIC_OK,
            "Terminal Player concrete camera retirement failed");});
    };
    close(cameras->current);
    for(auto& suspended:cameras->suspended)close(suspended);
    errors.Run([&]{carAttachedCameraDispose(cameras->overlay);cameras->overlay=nullptr;});
    errors.Run([&]{carRoadPreviewDispose(&cameras->crashPreview);});
    free(cameras);player->cameras=nullptr;
}
}

CarCinematicResult carFramePlayerActivateCameras(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras=player->cameras;
    playerCameraActivateCamera(&cameras->current);
    if(cameras->overlay)carCinematicBaseActivate(carAttachedCameraBase(cameras->overlay));
    if(cameras->scripted)carViewMoverActivate(cameras->scripted);
    if(cameras->countdown)carViewMoverActivate(cameras->countdown);
    if(cameras->replay&&carViewMoverReplayActivate(player->replay,cameras->replay)!=CAR_VIEW_MOVER_OK)
        return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    // PAL177544 ->183208 ->161720: the same retained VisibleDrivingLine.
    if(player->visibleDrivingLine){
        const WorldServicesResult result=previewCometEnableEmitters(player->visibleDrivingLine->comet);
        if(result!=WORLD_SERVICES_OK)return playerCameraFailure(player,
            result==WORLD_SERVICES_CAPACITY?CAR_CINEMATIC_BACKEND_CAPACITY:CAR_CINEMATIC_INVALID_BINDING);
    }
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerDeactivateCameras(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras=player->cameras;
    playerCameraDeactivateCamera(&cameras->current);
    if(cameras->overlay)carCinematicBaseDeactivate(carAttachedCameraBase(cameras->overlay));
    if(cameras->scripted)carViewMoverDeactivate(cameras->scripted);
    if(cameras->countdown)carViewMoverDeactivate(cameras->countdown);
    if(cameras->replay&&carViewMoverReplayDeactivate(player->replay,cameras->replay)!=CAR_VIEW_MOVER_OK)
        return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    // PAL1775F4 ->183200 ->1617A0: stop emitters without destroying the line.
    if(player->visibleDrivingLine)previewCometDisableEmitters(player->visibleDrivingLine->comet);
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerAdvanceCamera(CarFramePlayerState* player)
{
    if(!player||!player->cameras)return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    CarPlayerOwnedCamera* camera=&player->cameras->current;
    CarCinematicResult status;
    switch(camera->family){
    case CAR_PLAYER_CAMERA_NONE:return CAR_CINEMATIC_OK;
    case CAR_PLAYER_CAMERA_CHASE:status=carChaseCameraAdvance(camera->owner.chase);break;
    case CAR_PLAYER_CAMERA_FIXED:status=carFixedCameraAdvance(camera->owner.fixed);break;
    case CAR_PLAYER_CAMERA_DRIVE:status=carDriveCameraAdvance(camera->owner.drive);break;
    case CAR_PLAYER_CAMERA_ORBIT:status=carOrbitCameraAdvance(camera->owner.orbit);break;
    case CAR_PLAYER_CAMERA_ZONE:status=carZoneFreezeCameraAdvance(camera->owner.zone);break;
    case CAR_PLAYER_CAMERA_CRASH:status=carCrashCameraAdvance(camera->owner.crash);break;
    case CAR_PLAYER_CAMERA_AIRBORNE:status=carAirborneCameraAdvance(camera->owner.airborne);break;
    case CAR_PLAYER_CAMERA_VELOCITY:status=carVelocityCameraAdvance(camera->owner.velocity);break;
    default:return playerCameraFailure(player,CAR_CINEMATIC_INVALID_BINDING);
    }
    return playerCameraFailure(player,status);
}

int32_t carFramePlayerCameraKind(const CarFramePlayerState* player)
{
    return player&&player->cameras&&player->cameras->current.base?player->cameras->current.base->kind:0;
}

CarCinematicBase* carFramePlayerCurrentCamera(CarFramePlayerState* player)
{
    return player&&player->cameras?player->cameras->current.base:NULL;
}

CarCinematicView* carFramePlayerCurrentCameraView(CarFramePlayerState* player)
{
    CarCinematicBase* camera=carFramePlayerCurrentCamera(player);
    return camera?camera->view:NULL;
}

CarViewMover* carFramePlayerCarViewMover(const CarFramePlayerState* player)
{
    return player && player->cameras ? player->cameras->scripted : NULL;
}

float carFramePlayerCurrentCameraDuration(const CarFramePlayerState* player)
{
    if(!player||!player->cameras)return 0.0f;
    const CarPlayerOwnedCamera* camera=&player->cameras->current;
    if(camera->family==CAR_PLAYER_CAMERA_ZONE)return carZoneFreezeCameraDuration(camera->owner.zone);
    if(camera->family==CAR_PLAYER_CAMERA_CRASH)return carCrashCameraDuration(camera->owner.crash);
    return 0.0f;
}

int32_t carFramePlayerPrepareCrashCamera(CarFramePlayerState* player)
{
    if (!player || !player->cameras || !player->car) {
        playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
        return 0;
    }
    CarRoadPreview* preview = &player->cameras->crashPreview;
    /* StartState rejects an outstanding preview before the allocating
     * preparation call. The existing preview remains owned by Player. */
    if (preview->steps) return 0;
    return carCrashCameraPrepare(preview, player->car);
}

int32_t carFramePlayerValidateCurrentCrashCamera(CarFramePlayerState* player)
{
    if (!player || !player->cameras ||
        player->cameras->current.family != CAR_PLAYER_CAMERA_CRASH ||
        !player->cameras->context->terrain) {
        playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
        return 1;
    }
    CarCrashCamera* crash = player->cameras->current.owner.crash;
    return carCameraPathIntersectsTerrain(&crash->ground,
        player->cameras->context->terrain);
}

static CarCinematicResult playerCameraCoordinatorFailure(CarFramePlayerState* player,
    CarViewMoverResult result)
{
    if (result == CAR_VIEW_MOVER_ALLOCATION_FAILURE)
        return playerCameraFailure(player, CAR_CINEMATIC_ALLOCATION_FAILURE);
    return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
}

CarCinematicResult carFramePlayerSetScriptedCamera(CarFramePlayerState* player,
    CarMovement* movement)
{
    if (!player || !player->cameras)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras = player->cameras;
    if (cameras->scripted) return CAR_CINEMATIC_OK;
    if (!cameras->context->viewMovers)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarCinematicResult result = carFramePlayerChangeCamera(player, 0, 0);
    if (result != CAR_CINEMATIC_OK) return result;
    if (!movement) movement = player->car;
    CarViewMoverResult created;
    cameras->scripted = carViewMoverCreateForCar(cameras->context->viewMovers,
        cameras->viewNumber, movement, &created);
    if (!cameras->scripted) return playerCameraCoordinatorFailure(player, created);
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerSetCountdownCamera(CarFramePlayerState* player)
{
    if (!player || !player->cameras)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras = player->cameras;
    if (cameras->countdown) return CAR_CINEMATIC_OK;
    if (!cameras->context->viewMovers)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarCinematicResult result = carFramePlayerChangeCamera(player, 0, 0);
    if (result != CAR_CINEMATIC_OK) return result;
    CarViewMoverResult created;
    cameras->countdown = carViewMoverCreateForCountdown(cameras->context->viewMovers,
        cameras->viewNumber, player, &created);
    if (!cameras->countdown) return playerCameraCoordinatorFailure(player, created);
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerSetAttachedCamera(CarFramePlayerState* player,int32_t enabled)
{
    if (!player || !player->cameras)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarFramePlayerCameras* cameras = player->cameras;
    if (cameras->overlayViewNumber == 0) return CAR_CINEMATIC_OK;
    carAttachedCameraDispose(cameras->overlay);
    cameras->overlay = NULL;
    const CarFramePlayerCameraContext* context = cameras->context;
    CarCinematicBase* previous = context->services->screens[cameras->overlayViewNumber].last;
    if (previous) {
        const CarCinematicResult disposed = carCinematicDestroyOwned(previous);
        if (disposed != CAR_CINEMATIC_OK) return playerCameraFailure(player, disposed);
    }
    if (!enabled) return CAR_CINEMATIC_OK;
    if (!player->registry || !player->registry->world || !player->car)
        return playerCameraFailure(player, CAR_CINEMATIC_INVALID_BINDING);
    CarAttachedCameraContext binding = {context->services, player->car,
        player->registry->world, context->shake, context->attachedSettings,
        cameras->overlayViewNumber,context->terrain};
    CarCinematicResult result;
    cameras->overlay = carAttachedCameraCreate(&binding, &result);
    return playerCameraFailure(player, result);
}

static CarCinematicResult playerCameraSetCameraInputEnabled(CarPlayerOwnedCamera* camera, int32_t enabled)
{
    if (!camera->base) return CAR_CINEMATIC_OK;
    switch (camera->family) {
    case CAR_PLAYER_CAMERA_FIXED: {
        CarPlayerFixedCamera* input = carFixedCameraInput(camera->owner.fixed);
        if (!enabled) carPlayerFixedCameraDeactivateInput(input);
        else if (carPlayerFixedCameraActivateInput(input) != CAR_PLAYER_CAMERA_OK)
            return CAR_CINEMATIC_BACKEND_CAPACITY;
        break;
    }
    case CAR_PLAYER_CAMERA_ORBIT: {
        CarPlayerOrbitCamera* input = carOrbitCameraInput(camera->owner.orbit);
        if (!enabled) carPlayerOrbitCameraDeactivateInput(input);
        else if (carPlayerOrbitCameraActivateInput(input) != CAR_PLAYER_CAMERA_OK)
            return CAR_CINEMATIC_BACKEND_CAPACITY;
        break;
    }
    case CAR_PLAYER_CAMERA_CHASE:
    case CAR_PLAYER_CAMERA_DRIVE:
    case CAR_PLAYER_CAMERA_ZONE:
    case CAR_PLAYER_CAMERA_CRASH:
    case CAR_PLAYER_CAMERA_AIRBORNE:
    case CAR_PLAYER_CAMERA_VELOCITY:
        /* These concrete tables inherit the empty input hooks
         * 15FD40/15FD38. Drive activation owns Editor input separately. */
        break;
    default:
        return CAR_CINEMATIC_INVALID_BINDING;
    }
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carFramePlayerCamerasSetInputEnabled(CarFramePlayerState* player,
    int32_t enabled)
{
    if (!player) return CAR_CINEMATIC_INVALID_BINDING;
    if (!player->cameras) return CAR_CINEMATIC_OK;
    CarFramePlayerCameras* cameras = player->cameras;
    CarCinematicResult result = playerCameraSetCameraInputEnabled(&cameras->current, enabled);
    if (result != CAR_CINEMATIC_OK) return playerCameraFailure(player, result);
    for (int index = 0; index < 2; ++index) {
        result = playerCameraSetCameraInputEnabled(&cameras->suspended[index], enabled);
        if (result != CAR_CINEMATIC_OK) return playerCameraFailure(player, result);
    }
    return CAR_CINEMATIC_OK;
}

void carFramePlayerResetCurrentCamera(CarFramePlayerState* player)
{
    if (!player || !player->cameras) return;
    CarPlayerOwnedCamera* camera = &player->cameras->current;
    if (camera->family == CAR_PLAYER_CAMERA_CHASE) {
        carChaseCameraReset(camera->owner.chase);
        carChaseCameraSetFacingTarget(camera->owner.chase, player->lookBack != 0);
    }
    /* The other concrete main-camera tables inherit empty Reset15FD10. */
}

static void playerInputRegisterPlayerEvent(CarFramePlayerState* player, int32_t device,
    int32_t event, GameInputOwnerKind kind)
{
    GameInputTarget target{};target.kind=kind;target.owner.player=player;
    if (gameInputRegister(player->registry->services->input, device, event, target) != GAME_INPUT_OK)
        player->result = 1;
}

static void playerInputRemovePlayerEvent(CarFramePlayerState* player, int32_t event)
{
    gameInputUnregister(player->registry->services->input,
        player->movement.controllerPort, event);
}

static int32_t playerInputSelectedPursuitCar(CarFramePlayerState* player)
{
    PursuitManager* pursuit = player->registry->services->pursuit;
    if (!pursuit) {
        player->result = 1;
        return -1;
    }
    const PursuitCollisionEntry* selected = pursuitManagerSelectedEntry(pursuit);
    if (!selected) {
        player->result = 1;
        return -1;
    }
    return selected->movement == player->car;
}

static int32_t playerInputUpdateDrivingEvents(CarFramePlayerState* player, int32_t mode,
    int32_t enabled)
{
    int32_t events[8];
    size_t count = 0;
    if (mode == 2) {
        events[count++] = 0x83;
        events[count++] = 0x5D;
        events[count++] = 0x82;
    }
    events[count++] = 0x5E;
    /* Preserve callback registration/removal order around the pursuit lookup. */
    for (size_t index = 0; index < count; ++index) {
        if (enabled) playerInputRegisterPlayerEvent(player, player->movement.controllerPort,
            events[index], GAME_INPUT_PLAYER);
        else playerInputRemovePlayerEvent(player, events[index]);
    }
    count = 0;
    if (*player->registry->pursuitEnabled) {
        const int32_t selected = playerInputSelectedPursuitCar(player);
        if (selected < 0) return 0;
        if (selected) {
            events[count++] = 0x80;
            events[count++] = 0x81;
        }
    } else {
        events[count++] = 0x5F;
        events[count++] = 0x60;
        events[count++] = 0x61;
        events[count++] = 0x62;
    }
    for (size_t index = 0; index < count; ++index) {
        if (enabled) playerInputRegisterPlayerEvent(player, player->movement.controllerPort,
            events[index], GAME_INPUT_PLAYER);
        else playerInputRemovePlayerEvent(player, events[index]);
    }
    Car* car = player->car ? player->car->bindings.owner : NULL;
    CarDriver* driver = car ? (CarDriver*)car->controller : NULL;
    if (driver && (!enabled || player->car->bindings.ownerMotion->inputCategory == 1)) {
        if (carDriverSetInputEnabled(driver, enabled, player->movement.controllerPort) != CAR_DRIVER_OK)
            player->result = 1;
    }
    if (carFramePlayerCamerasSetInputEnabled(player, enabled) != CAR_CINEMATIC_OK)
        player->result = 1;
    return 1;
}

static int32_t playerInputHasCameraCycleRegistration(const GameInputRouter* input)
{
    for (size_t index = 0; index < input->count; ++index) {
        const GameInputRegistration* registration = &input->registrations[index];
        if (registration->device == 1 && registration->event == 0x2D) return 1;
    }
    return 0;
}

void carFramePlayerSetInputMode(CarFramePlayerState* player, int32_t mode)
{
    if (!player || !player->registry || !player->registry->services ||
        !player->registry->services->input || !player->registry->pursuitEnabled ||
        (player->car && !player->car->bindings.ownerMotion)) {
        if (player) player->result = 1;
        return;
    }
    GameInputRouter* input = player->registry->services->input;
    switch (player->inputMode) {
    case 2:
    case 7:
        if (!playerInputUpdateDrivingEvents(player, player->inputMode, 0)) return;
        break;
    case 3:
        playerInputRemovePlayerEvent(player, 0x83);
        break;
    case 5:
    case 6:
        playerInputRemovePlayerEvent(player, 0x84);
        playerInputRemovePlayerEvent(player, 0x83);
        break;
    default:
        break;
    }
    if (player->movement.seat == 0 && playerInputHasCameraCycleRegistration(input))
        gameInputUnregister(input, 1, 0x2D);
    player->inputMode = mode;
    switch (mode) {
    case 2:
    case 7:
        if (!playerInputUpdateDrivingEvents(player, mode, 1)) return;
        break;
    case 3:
        playerInputRegisterPlayerEvent(player, player->movement.controllerPort, 0x83, GAME_INPUT_PLAYER);
        break;
    case 5:
    case 6: {
        const GameInputOwnerKind target = mode == 5 ? GAME_INPUT_PLAYER_MENU : GAME_INPUT_PLAYER_RACE;
        playerInputRegisterPlayerEvent(player, player->movement.controllerPort, 0x84, target);
        playerInputRegisterPlayerEvent(player, player->movement.controllerPort, 0x83, target);
        break;
    }
    default:
        break;
    }
    if ((mode == 2 || mode == 4) && player->registry->count < 2 && player->movement.seat == 0)
        playerInputRegisterPlayerEvent(player, 1, 0x2D, GAME_INPUT_PLAYER_CAMERA);
}

void carFramePlayerReconcileInput(CarFramePlayerState* player)
{
    static const int32_t modes[10] = {0, 5, 5, 2, 7, 7, 1, 7, 4, 7};
    static const int8_t cancelCurrent[8] = {0, 1, 1, 0, 0, 0, 0, 0};
    if (!player || !player->registry || !player->registry->services ||
        !player->registry->services->session) {
        if (player) player->result = 1;
        return;
    }
    const int32_t sessionState = player->registry->services->session->state;
    const int32_t cinematicState = player->movement.steeringFeedbackBusy;
    if (cinematicState < 0 || cinematicState >= 8) {
        player->result = 1;
        return;
    }
    int32_t mode = sessionState >= 0 && sessionState < 10 ? modes[sessionState] : 0;
    if (player->drivingLocked) mode = 6;
    if (cancelCurrent[cinematicState]) mode = 3;
    if (mode != player->inputMode) carFramePlayerSetInputMode(player, mode);
}

void carFramePlayerCycleCameraInput(CarFramePlayerState* player,int32_t device,
    int32_t event,uint8_t pressed)
{
    (void)device;
    (void)event;
    (void)pressed;
    if (!player || !player->registry || !player->registry->services) {
        if (player) player->result = 1;
        return;
    }
    CarFramePlayerServices* services = player->registry->services;
    if (!services->inputBindings || !services->inputBindings->cameraCyclingEnabled) {
        player->result = 1;
        return;
    }
    if (!*services->inputBindings->cameraCyclingEnabled) return;
    static const int32_t sessionModes[3] = {0,4,2};
    static const int32_t playerModes[4] = {1,4,2,3};
    const int32_t sessionMode = player->inputMode == 4;
    const int32_t* modes = sessionMode ? sessionModes : playerModes;
    const int32_t count = sessionMode ? 3 : 4;
    const int32_t current = carFramePlayerCameraKind(player);
    int32_t nextIndex = -1;
    for (int32_t index = 0; index < count; ++index) {
        if (modes[index] == current) {
            nextIndex = (index + 1) % count;
            break;
        }
    }
    if (nextIndex < 0) return;
    const int32_t next = modes[nextIndex];
    if (sessionMode && (!services->session || !services->session->cameras)) {
        player->result = 1;
        return;
    }
    if (sessionMode && next) gameSessionCameraDeactivate(services->session->cameras);
    if (carFramePlayerChangeCamera(player,next,0) != CAR_CINEMATIC_OK) {
        player->result = 1;
        return;
    }
    if (sessionMode && !next &&
        gameSessionCameraActivate(services->session->cameras) != CAR_VIEW_MOVER_OK)
        player->result = 1;
}

int32_t carFramePlayerClearTransientPresentation(CarFramePlayerState* player)
{
    CarFramePresentationEvent* event = carFramePresentationAppend(player->presentation,
        CAR_FRAME_UI_CLEAR_TRANSIENT, player->car, 0);
    if (!event) {
        player->result = 1;
        return 0;
    }
    /* UI animation/text objects belong to the native presentation owner.
     * The request orders their cancellation before the camera transition. */
    event->port = player->movement.controllerPort;
    return 1;
}

/* Player state and controls. */
int32_t carFramePlayerWrongWay(const CarFramePlayerState* player, int32_t wasWrongWay)
{
    const VehicleState* state = player->car->bindings.state;
    const uint16_t routeHeading = vectorAtan2Angle(player->car->routeLocation.direction[0],
                                                   player->car->routeLocation.direction[1]);
    uint16_t difference = (uint16_t)(routeHeading - state->heading);
    if (32768u < difference) difference = (uint16_t)(0u - difference);
    if (wasWrongWay) difference = (uint16_t)(difference + 1820u);
    if (difference < 20024u) return 0;
    if (10.0f < fabsf(state->forwardSpeed * 2.236999988555908f)) {
        const uint16_t movingHeading = vectorAtan2Angle(state->velocity[0], state->velocity[1]);
        difference = (uint16_t)(routeHeading - movingHeading);
        if (32768u < difference) difference = (uint16_t)(0u - difference);
        if (wasWrongWay) difference = (uint16_t)(difference + 1820u);
        if (difference < 20024u) return 0;
    } else if (!wasWrongWay) {
        return 0;
    }
    return wasWrongWay ? 1 : state->activeContactCount > 3;
}

void carFramePlayerClearPreview(CarFramePlayerState* player)
{
    carRoadPreviewDispose(&player->airbornePreview);
}

int32_t carFramePlayerPrepareAirborne(CarFramePlayerState* player)
{
    carFramePlayerClearPreview(player);
    if (player->registry->trackId == 21) {
        const float excluded[2] = {1646.0f, -884.0f};
        if (vectorNear2(player->car->bindings.state->position, excluded, 40.0f)) return 0;
    }
    const CarRecoveryPose* pose = player->car->bindings.driver ?
                                 player->car->bindings.driver->recoveryPose : NULL;
    const CarRecoveryGroup* group = NULL;
    const CarRecoveryPointRegistry* road = player->car->bindings.recoveryPoints;
    if (!road) { player->result = 1; return 0; }
    if (pose)
        for (size_t index = 0; index < road->groupCount; ++index)
            if (road->groups[index].id == pose->groupId) { group = &road->groups[index]; break; }
    if (!carRoadPreviewInitialize(&player->airbornePreview, road, group, pose,
        player->car->bindings.state->position, player->car->routeLocation.direction,
        50.0f, 75.0f, player->car->info.routeVehicleClass, 0)) return 0;
    if (player->airbornePreview.count >= 5) return 1;
    carFramePlayerClearPreview(player);
    return 0;
}

float carFramePlayerAverageSpeedRatio(const CarFramePlayerState* player)
{
    return player->speedHistory.value;
}

int32_t carFramePlayerExceededSpeedRatio(const CarFramePlayerState* player, float limit)
{
    return player->speedHistory.count == player->speedHistory.capacity &&
        limit < player->maximumSpeedRatio;
}


CarFramePresentationEvent* carFramePresentationAppend(CarFramePresentation* presentation,
    CarFramePresentationKind kind, struct CarMovement* car, int32_t code)
{
    if (!presentation) return NULL;
    if (!presentation->events || presentation->count >= presentation->capacity) {
        presentation->failed = 1;
        return NULL;
    }
    CarFramePresentationEvent* event = &presentation->events[presentation->count++];
    memset(event, 0, sizeof(*event));
    event->kind = kind;
    event->car = car;
    event->code = code;
    return event;
}

int32_t carFramePresentationCopyPreview(CarFramePresentationEvent* event, const CarRoadPreview* preview)
{
    if (!event || !preview || !preview->steps || preview->count <= 0) return 0;
    event->preview = *preview;
    event->preview.steps = (CarRoadPreviewStep*)malloc((size_t)preview->count * sizeof(*preview->steps));
    if (!event->preview.steps) return 0;
    memcpy(event->preview.steps, preview->steps, (size_t)preview->count * sizeof(*preview->steps));
    event->hasPreview = 1;
    return 1;
}

void carFramePresentationReset(CarFramePresentation* presentation)
{
    if (!presentation) return;
    for (size_t index = 0; index < presentation->count; ++index)
        if (presentation->events[index].hasPreview) carRoadPreviewDispose(&presentation->events[index].preview);
    presentation->count = 0;
    presentation->failed = 0;
}

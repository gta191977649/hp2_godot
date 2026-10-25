// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/inc/point.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/frontend/selection_camera.hpp"
#include "game_core/port/frontend/selection_render.hpp"
#include "game_core/feAnimAlphaElement.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace hp2::debug {struct VisibleCar;}

namespace port::selection {
// Port services bind original globals; missing required evidence/services fail explicitly.
struct ListServices {
    const std::array<hp2::debug::CarTypeInfo,64>& cars;
    std::function<bool(int,bool)> is_car_unlocked;
    std::function<std::string(int)> full_name;
    bool all_cars_available=false;
};
// Semantic popup submission, not an original PopupConfig ABI.
struct Popup {
    std::uint32_t screen_hash=0;
    int port=0,flags=3,dismiss_param=6,cancel_param=0,n_buttons=0;
    int message_id=0,start_button=0;
    std::array<int,3> button_ids{},button_params{};
};
struct SelectorServices {
    std::function<int(const Popup&)> summon_popup;
    std::function<bool()> maybe_continue;
};
struct OrbitServices {
    const std::array<hp2::debug::CarTypeInfo,64>& cars;
    int real_time_frames=0;
    bool help_active=false,version_e3=false;
    CameraServices camera;
    std::function<void()> adjust_help;
    std::function<int(int)> car_description;
    // Original SPCH_AddEventV event/argument vector; no synthesized speech.
    std::function<void(int,const std::vector<int>&)> speech_event;
    std::function<void(float)> speech_fade;
};
// Explicit semantic adapter of original Flailer/CarTypeInfo/SkinInfo globals.
// Authored screen fields stay in existing frontend storage; no ABI overlay.
struct WidgetContext {
    hp2::frontend::FeScreen& screen;
    const std::array<hp2::debug::CarTypeInfo,64>& cars;
    hp2::frontend::WidgetMeasure measure;
    std::function<std::uint32_t(int,int)> skin_swatch;
    std::function<std::string(int)> translated_id;
    std::function<std::string(const std::string&)> translated_key;
};
struct ScreenServices {
    SelectorServices state;
    WidgetContext& widgets;
    const RenderServices& rendering;
    const OrbitServices& orbit;
    std::function<void(hp2::debug::VisibleCar*,int,int)> set_locked_car;
    std::function<void(int)> sound;
    std::function<void()> update_info,tick,render_platform;
    std::function<void(int)> render_locked;
};
// Required original globals/services for001947D0. These are semantic bindings,
// not a CarLoader ABI; every loader request uses RideInfo(type,-1,0,0).
struct TickServices {
    ::port::driving::Memory& memory;
    WidgetContext& widgets;
    const RenderServices& rendering;
    const OrbitServices& orbit;
    hp2::debug::feAlphaAnimation& fade;
    hp2::debug::feAlphaAnimation& orbit_fade;
    hp2::debug::feAlphaAnimation& snap_fade;
    float elapsed=0;
    bool all_cars_available=false;
    std::function<bool(int)> is_loaded;
    std::function<void(int)> unload,load;
    std::function<void()> begin_loading;
    // Original ImageData[3] palette sample; absent/unloaded texture returns nullopt.
    std::function<std::optional<std::array<std::uint8_t,3>>(std::uint32_t)> swatch_rgb;
    std::function<void(int,float,int)> set_lighting;
    std::function<void()> update_locked,update_nebula;
};
// Bounded vector-store part of Debug00196688; call once for a fresh selection
// memory image, before Tick. ELF zero-fill is not C++ static initialization.
void initialize_camera_offsets(::port::driving::Memory&);
}
namespace hp2::debug {
// Debug ScreenMessage/ScreenCallbackData: decoded fields only, no original layout.
enum ScreenMessage { MESSAGE_NONE=0,MENUITEM_SELECT=1,MENUITEM_ACTIVATE=2,MENUITEM_UNACTIVATE=3,
    MENUITEM_HIGHLIGHT=4,MENUITEM_UNHIGHLIGHT=5,MENUITEM_PAD_LEFT=6,MENUITEM_PAD_RIGHT=7,
    MENUITEM_PAD_UP=8,MENUITEM_PAD_DOWN=9,MENUITEM_SQUARE=10,SCREEN_ACTIVATE=11,
    SCREEN_UNACTIVATE=12,SCREEN_START_BUTTON=13,SCREEN_SELECT_BUTTON=14,SCREEN_L1=15,
    SCREEN_R1=16,SCREEN_BACK_BUTTON=17,SCREEN_TICK=18,SCREEN_RENDER=19,POPUP_ACTIVE=20,
    POPUP_DISMISS=21,POPUP_NEXT=22,POPUP_PREV=23,ANY_BUTTON_PRESSED=24 };
struct ScreenCallbackData {int Param=0,Noise=0;std::uint32_t ItemHash=0;};
// E-0098-S01. Decoded Debug names/state only, NOT original object ABI.
struct VisibleCar {
    CarStatus* pCarStatus=nullptr;
    int eCarType=0;
    bool CarUnlocked=false;
    int SkinIndex=0;
    std::uint32_t ThumbnailHash=0;
    int CarIndex=0;
    VisibleCar(int,CarStatus&,int,const CarTypeInfo&);
    void SetCurrentSkinNumber(int);
    int NextSkin();
    int PrevSkin();
};
int CarListSort(const VisibleCar&,const VisibleCar&,const ::port::selection::ListServices&);
struct VisibleCarSet {
    // Host ownership substitutes intrusive list allocation; stable pointee identity.
    std::vector<std::unique_ptr<VisibleCar>> VisibleCarList;
    VisibleCar* pCurrentVisibleCar=nullptr;
    int TotalNumCars=0,NumberUnlocked=0;
    VisibleCarSet(FrontendPlayerInfo&,int game_type,bool be_a_cop,const ::port::selection::ListServices&);
    void SortVisibleCarList(const ::port::selection::ListServices&);
    VisibleCar* GetNextCar(VisibleCar*);
    VisibleCar* GetPrevCar(VisibleCar*);
};
enum eSelectorState { SS_SELECT=0,SS_TRANSMISSION=1,SS_HANDLING=2,SS_READY=3,SS_NUM_STATES=4 };
enum TransmissionTypes { TT_AUTO=0,TT_MANUAL=1,TT_SPORT=2 };
enum HandlingModes { HM_CLASSIC=0,HM_EXTREME=1 };
struct CarSelector {
    // State projection of00192B78 defaults; original presentation constructor is NOT implemented.
    // Host caller explicitly binds the current list. No Godot types or allocations per update.
    VisibleCarSet& VisibleCars;
    int IsTwoPlayerScreen=0,Port=0,AllCarsUnlocked=0,ViewID=1;
    std::uint32_t ScreenHash=0xb5074ab4;
    eSelectorState SelectorState=SS_SELECT;
    TransmissionTypes TransmissionType=TT_AUTO;
    HandlingModes HandlingMode=HM_CLASSIC;
    float SELECTOR_MODE_CHOOSE=0,SELECTOR_MODE_ORBIT=1,DesiredSelectorMode=0;
    float SelectorMode=0;
    int CAR_FADED_ALPHA=130,FadeCarOverlayAlpha=130,ManualOrbitEngaged=0,OrbitActiveTime=0;
    int PopupID=-1;
    int NumSwatches=0;
    std::array<std::uint32_t,10> SwatchFlailerHashes{},SwatchTextureHashes{};
    tCubic1D ThumbnailHorizCubic;
    VisibleCar* LoadedCar=nullptr;
    VisibleCar* LoadingCar=nullptr;
    FrontEndRenderingCar* pFrontEndRenderingCar=nullptr;
    FrontEndRenderingCar* pFrontEndRenderingCarReflection=nullptr;
    SelectCarCameraMover* pSelectCarCameraMover=nullptr;
    eModel CloakedCarModel{hp2::frontend::bStringHash("CARCOVER")};
    std::uint16_t CarRotationZ=0x3efb;
    std::uint16_t StatCamFOV=0x1c44;
    int LightsOn=0,CopLightsOn=0;
    void Tick(const ::port::selection::TickServices&);
    void SetupCarOverrideModel();
    void ShowLoadedCar(const ::port::selection::RenderServices&);
    void UpdateRenderingCarParameters(::port::driving::Memory&,FrontEndRenderingCar&,int mirror_image,
        const std::array<CarTypeInfo,64>&);
    void CloseSwatches(::port::selection::WidgetContext&);
    void SetupSwatches(::port::selection::WidgetContext&);
    void UpdateSwatches(::port::selection::WidgetContext&);
    void SetupThumbnails(::port::selection::WidgetContext&);
    void UpdateThumbnails(::port::selection::WidgetContext&,float elapsed);
    std::uint32_t CalculateCarLogoTexture(const CarTypeInfo&) const;
    std::uint32_t CalculateNFSLogoTexture(const CarTypeInfo&) const;
    void SetLogo(::port::selection::WidgetContext&);
    // Service receives a Debug help-table address, zero for original default override.
    void AdjustHelpMessage(const FrontendDatabase&,const std::function<void(std::uint32_t)>&);
    bool CanEnterOrbitMode(const VisibleCar*,bool help_active) const;
    void EnterOrbitMode(const ::port::selection::OrbitServices&);
    void EnterChooseMode(const ::port::selection::OrbitServices&);
    void JoyHandler(int port,int event,std::uint8_t param,const ::port::selection::OrbitServices&);
    void UpdateFadeGroups(feAlphaAnimation&,feAlphaAnimation&,feAlphaAnimation&,
        const std::array<CarTypeInfo,64>&);
    void AdvanceState(const ::port::selection::SelectorServices&);
    void DevanceState(const ::port::selection::SelectorServices&);
    void SummonPopup(const ::port::selection::SelectorServices&);
    // Only MENUITEM_SELECT(1) and POPUP_DISMISS(21) are implemented here.
    // Other reachable messages retain an explicit gap; no presentation no-ops.
    void ScreenCallback(int msg,int param,std::uint32_t item_hash,const ::port::selection::SelectorServices&);
    void ScreenCallback(ScreenMessage,ScreenCallbackData&,const ::port::selection::ScreenServices&);
};
struct SelectCarScreen {
    std::array<CarSelector*,2> CarSelectors{};
    int NumCarSelectors=0;
    int AreAllSelectorsReady() const;
    void SetSelectedCars(FrontendDatabase&,const std::array<CarTypeInfo,64>&);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class CarSelector {
public:
    // ELF 0x00195C40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00195c40.md.
    // Original Debug reference: CarSelector::SetupCarOverrideModel(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetupCarOverrideModel();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class SelectCarScreen {
public:
    // ELF 0x00198878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00198878.md.
    // Original Debug reference: SelectCarScreen::AreAllSelectorsReady(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AreAllSelectorsReady();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VisibleCar {
public:
    // ELF 0x001942D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001942d0.md.
    // Original Debug reference: VisibleCar::SetCurrentSkinNumber(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetCurrentSkinNumber();
    // ELF 0x00194350; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00194350.md.
    // Original Debug reference: VisibleCar::NextSkin(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void NextSkin();
    // ELF 0x00194380; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00194380.md.
    // Original Debug reference: VisibleCar::PrevSkin(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PrevSkin();
};
} // namespace hp2::recovered

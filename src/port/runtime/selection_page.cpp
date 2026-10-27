#include "selection_page.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/src/bMath.hpp"
#include <limits>
#include <stdexcept>

namespace port::selection {
namespace {
PageHandle required(PageHandle value,const char* address){
    if(!value)hp2::not_implemented(address,"selection resource binding returned no lifetime lease");
    return value;
}
void initialize_car(hp2::debug::FrontEndRenderingCar& car){
    // FrontEndRenderingCar0011E7E8: original null-RideInfo matrices/defaults.
    car.BodyMatrix=hp2::recovered::bIdentity();
    for(auto& matrix:car.TireMatrices)matrix=hp2::recovered::bIdentity();
    for(auto& matrix:car.BrakeMatrices)matrix=hp2::recovered::bIdentity();
}
}
void SelectionPage::NebulaOwner::release(){
    for(auto handle:handles)effects.remove_auto_emitter(handle);
    handles.clear();effects.kill_all_active_effects(pools);
}
SelectionPage::NebulaOwner::~NebulaOwner(){
    // Port construction rollback: no whole-system kill unless this owner actually
    // published emitters. Normal close explicitly calls release even for zero.
    if(!handles.empty())release();
}
SelectionPage::SelectionPage(const hp2::debug::VisibleCarSet& source,PageServices services,
    int frames,std::uint32_t timer,float elapsed)
    :services_(std::move(services)),cars_(source),selector_{cars_},
     colours_(load_nebula_colours(services_.memory)),timer_(timer),elapsed_(elapsed),
     nebula_{services_.effects,services_.pools,{}} {
    auto& s=services_;
    if(!s.points || !s.spotlight_colours || !s.model || !s.car || !s.camera || !s.joy || !s.load || !s.is_loaded || !s.begin_loading
       || !s.preload_selected || !s.dismiss_popup || !s.sound || !s.stop_sound || !s.help_override
       || !s.lighting || !s.swatch_rgb || !s.render_platform)
        hp2::not_implemented("0x00192B78","selection page requires model/car/camera/input/loader/popup/audio/help/render services");
    if(!cars_.pCurrentVisibleCar)hp2::not_implemented("0x00192B78","selection page requires an original current car");
    if(*s.spotlight_colours>std::numeric_limits<std::size_t>::max()/3)
        throw std::length_error("Selection spotlight allocation overflow");
    // Debug00192B78, E-0098-O03: single-player constructor order. No two-player
    // entry, original allocator/object ABI or production host readiness claim.
    selector_.ScreenHash=s.widgets.screen.name_hash_0x08;
    s.orbit.adjust_help=[this]{selector_.AdjustHelpMessage(services_.fred,services_.help_override);};
    fade_=std::make_unique<hp2::debug::feAlphaAnimation>(s.widgets.screen,0x15678f);
    orbit_fade_=std::make_unique<hp2::debug::feAlphaAnimation>(s.widgets.screen,0x36e80bf);
    snap_=std::make_unique<hp2::debug::feAlphaAnimation>(s.widgets.screen,0x961a600);
    selector_.ThumbnailHorizCubic.flags=1;selector_.ThumbnailHorizCubic.duration=1;
    selector_.ThumbnailHorizCubic.arrivalThreshold=1;
    locked_=std::make_unique<LockedDisplay>(s.widgets.screen);
    input_.reserve(22);
    for(int port=0;port<2;++port)for(int event=33;event<=43;++event)
        input_.push_back(required(s.joy(port,event,[this](int port,int event,std::uint8_t value){joy(port,event,value);}),"0x00192B78"));
    initialize_car(main_);initialize_car(reflected_);
    main_handle_=required(s.car(main_,selector_.ViewID),"0x00192B78");
    reflected_handle_=required(s.car(reflected_,selector_.ViewID),"0x00192B78");reflected_.Reflection=1;
    selector_.pFrontEndRenderingCar=&main_;selector_.pFrontEndRenderingCarReflection=&reflected_;
    cover_=required(s.model(selector_.CloakedCarModel.NameHash),"0x00192B78");
    camera_handle_=required(s.camera(camera_,selector_.ViewID),"0x00192B78");
    selector_.pSelectCarCameraMover=&camera_;
    selector_.LoadingCar=cars_.pCurrentVisibleCar;load(selector_.LoadingCar->eCarType);s.begin_loading();
    locked_car(cars_.pCurrentVisibleCar,0,0);
    selector_.SetupCarOverrideModel();selector_.SetLogo(s.widgets);info();
    s.lighting(selector_.ViewID,0,0);
    selector_.AdjustHelpMessage(s.fred,s.help_override);
    platform_=required(s.model(hp2::frontend::bStringHash("PLATFORM")),"0x00192B78");
    selector_.SetupSwatches(s.widgets);
    spotlight_alpha_.resize(*s.spotlight_colours);spotlight_rgb_.resize(*s.spotlight_colours*3);
    selector_.SetupThumbnails(s.widgets);selector_.UpdateFadeGroups(*fade_,*orbit_fade_,*snap_,s.widgets.cars);
    if(!s.widgets.translated_id)hp2::not_implemented("0x00192B78","selection points translation missing");
    set_widget_string(s.widgets,0x736163fc,s.widgets.translated_id(0xbfb0)+" "+std::to_string(*s.points));
    nebula_.handles.reserve(20);
    for(int i=0;i<20;++i){
        port::track::ParticleEmitter emitter;emitter.name="XE_SelectCarNebula";
        emitter.data_hash=emitter.name_hash=hp2::frontend::bStringHash(emitter.name.c_str());emitter.state=1;
        const auto handle=s.effects.add_auto_emitter(emitter,s.pools);
        nebula_.handles.push_back(handle);selector_.NebulaEmitters[i]=handle;
    }
    selector_.NumNebulaEmitters=20;selector_.UpdateNebula(s.memory,s.effects,colours_);
    tick(frames,timer,elapsed);
}
SelectionPage::~SelectionPage(){
    // Port abnormal-disposal ordering also prevents render-state dangling pointers.
    main_.OverrideModel=nullptr;reflected_.OverrideModel=nullptr;
    camera_handle_.reset();main_handle_.reset();reflected_handle_.reset();
}
void SelectionPage::require_open()const{if(closed_ || closing_)throw std::logic_error("Selection page is closed or closing");}
void SelectionPage::load(int type){
    if(loads_.count(type))throw std::logic_error("Duplicate selector load ownership");
    loads_.emplace(type,required(services_.load(type),"0x001947D0"));
}
void SelectionPage::unload(int type){
    auto found=loads_.find(type);if(found==loads_.end())throw std::logic_error("Missing selector load ownership");
    loads_.erase(found);
}
void SelectionPage::info(){selector_.UpdateInfoBarAtTop(services_.widgets,services_.memory,
    services_.metric_units,services_.fred.GameType==8 || services_.fred.GameType==9);}
void SelectionPage::locked_car(hp2::debug::VisibleCar* car,int unlocked,int player){
    locked_->SetCurrentCar(car,unlocked,player,services_.fred.GameType,timer_,services_.locked);
}
void SelectionPage::tick(int frames,std::uint32_t timer,float elapsed){
    require_open();timer_=timer;elapsed_=elapsed;auto& s=services_;s.orbit.real_time_frames=frames;
    TickServices tick{s.memory,s.widgets,s.rendering,s.orbit,*fade_,*orbit_fade_,*snap_,
        elapsed,s.all_cars_available,s.is_loaded,[this](int type){unload(type);},
        [this](int type){load(type);},s.begin_loading,s.swatch_rgb,s.lighting,
        [this]{locked_->Update(timer_,elapsed_);},
        [this]{selector_.UpdateNebula(services_.memory,services_.effects,colours_);}};
    selector_.Tick(tick);
}
void SelectionPage::update_camera(float elapsed){require_open();camera_.Update(services_.memory,elapsed,services_.orbit.camera);}
void SelectionPage::joy(int port,int event,std::uint8_t value){require_open();selector_.JoyHandler(port,event,value,services_.orbit);}
void SelectionPage::message(hp2::debug::ScreenMessage message,hp2::debug::ScreenCallbackData& data){
    require_open();auto& s=services_;
    ScreenServices screen{s.state,s.widgets,s.rendering,s.orbit,
        [this](auto* car,int unlocked,int player){locked_car(car,unlocked,player);},
        s.sound,[this]{info();},
        [this]{tick(services_.orbit.real_time_frames,timer_,elapsed_);},s.render_platform,
        [this](int unlocked){locked_->Render(unlocked,services_.locked);}};
    selector_.ScreenCallback(message,data,screen);
}
void SelectionPage::after_display(){
    require_open();auto& s=services_;
    s.effects.step_shared(s.pools,s.memory,s.memory.scalar(0x324a48),{},true);
}
// Debug00193278, E-0098-O03. Preload's8-byte00182430 wrapper tail-jumps
// to00192580; both original calls must remain. SetEmitterState0016B670 is empty.
void SelectionPage::close(){
    if(closed_)return;
    if(closing_)throw std::logic_error("Selection close is already in progress or failed; dispose the owner");
    closing_=true; // Port policy: no Tick/input after partial teardown failure.
    auto& s=services_;
    nebula_.release();selector_.NebulaEmitters.fill(std::nullopt);selector_.NumNebulaEmitters=0;
    s.dismiss_popup(selector_.PopupID);selector_.PopupID=-1;
    platform_.reset();selector_.CloseSwatches(s.widgets);
    s.preload_selected();s.preload_selected();
    if(selector_.LoadedCar)unload(selector_.LoadedCar->eCarType);
    if(selector_.LoadingCar)unload(selector_.LoadingCar->eCarType);
    selector_.LoadedCar=selector_.LoadingCar=nullptr;
    main_.OverrideModel=nullptr;reflected_.OverrideModel=nullptr;
    camera_handle_.reset();main_handle_.reset();reflected_handle_.reset();cover_.reset();
    for(auto& input:input_)input.reset();
    input_.clear();
    std::vector<std::uint8_t>().swap(spotlight_alpha_);
    std::vector<std::uint8_t>().swap(spotlight_rgb_);s.stop_sound(12);
    locked_.reset();snap_.reset();orbit_fade_.reset();fade_.reset();
    selector_.pSelectCarCameraMover=nullptr;selector_.pFrontEndRenderingCar=nullptr;
    selector_.pFrontEndRenderingCarReflection=nullptr;
    cars_.VisibleCarList.clear();cars_.pCurrentVisibleCar=nullptr;
    closed_=true;
}
}

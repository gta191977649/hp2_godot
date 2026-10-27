#pragma once
// TASK-0098 E-0098-O03. Port lifetime binder, not an original class/ABI.
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/frontend/selection_locked.hpp"
#include "game_core/port/track/particles.hpp"
#include <map>

namespace port::selection {
// Host resources must release without throwing, cancel pending uploads and wait
// for rendering before destroying native state. No job receives a page pointer.
struct PageLease {virtual ~PageLease()=default;};
using PageHandle=std::unique_ptr<PageLease>;
using PageJoy=std::function<void(int,int,std::uint8_t)>;
struct PageServices {
    port::driving::Memory& memory;
    WidgetContext widgets;
    hp2::debug::FrontendDatabase& fred;
    port::track::ParticleSimulation& effects;
    port::track::SharedParticleState& pools;
    RenderServices rendering;
    OrbitServices orbit;
    SelectorServices state;
    LockedDisplayServices locked;
    // Each successful registration returns its required owning cancellation lease.
    std::function<PageHandle(std::uint32_t)> model;
    std::function<PageHandle(hp2::debug::FrontEndRenderingCar&,int)> car;
    std::function<PageHandle(hp2::debug::SelectCarCameraMover&,int)> camera;
    // Numeric Debug event IDs33..43 (Retail39/40 differ).
    std::function<PageHandle(int,int,const PageJoy&)> joy;
    std::function<PageHandle(int)> load;
    std::function<bool(int)> is_loaded;
    std::function<void()> begin_loading,preload_selected,render_platform;
    std::function<void(int)> dismiss_popup,sound,stop_sound;
    std::function<void(std::uint32_t)> help_override;
    std::function<void(int,float,int)> lighting;
    std::function<std::optional<std::array<std::uint8_t,3>>(std::uint32_t)> swatch_rgb;
    bool metric_units=false,all_cars_available=false;
    std::optional<int> points;
    std::optional<std::size_t> spotlight_colours;
};
class SelectionPage {
public:
    SelectionPage(const hp2::debug::VisibleCarSet&,PageServices,int frames,
                  std::uint32_t timer,float elapsed);
    // Abnormal host disposal only releases owned resources. Normal screen exit
    // must call close(), which performs the recovered preload/audio/popup sequence.
    ~SelectionPage();
    SelectionPage(const SelectionPage&)=delete;
    SelectionPage& operator=(const SelectionPage&)=delete;
    void tick(int frames,std::uint32_t timer,float elapsed);
    void update_camera(float elapsed);
    void message(hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&);
    void joy(int port,int event,std::uint8_t value);
    // Caller invokes once at the original AFTER-display boundary, never in tick.
    void after_display();
    void close();
    hp2::debug::CarSelector& selector(){return selector_;}
    const hp2::debug::FrontEndRenderingCar& main_car()const{return main_;}
    const hp2::debug::FrontEndRenderingCar& reflected_car()const{return reflected_;}
    bool closed()const{return closed_;}
private:
    PageServices services_;
    hp2::debug::VisibleCarSet cars_;
    hp2::debug::CarSelector selector_;
    hp2::debug::FrontEndRenderingCar main_,reflected_;
    hp2::debug::SelectCarCameraMover camera_;
    std::unique_ptr<hp2::debug::feAlphaAnimation> fade_,orbit_fade_,snap_;
    std::unique_ptr<LockedDisplay> locked_;
    std::vector<PageHandle> input_;
    PageHandle main_handle_,reflected_handle_,cover_,camera_handle_,platform_;
    std::map<int,PageHandle> loads_;
    std::vector<std::uint8_t> spotlight_alpha_,spotlight_rgb_;
    NebulaColours colours_;
    std::uint32_t timer_=0;
    float elapsed_=0;
    bool closed_=false,closing_=false;
    struct OverrideClear {
        hp2::debug::FrontEndRenderingCar& main;
        hp2::debug::FrontEndRenderingCar& reflected;
        ~OverrideClear(){main.OverrideModel=nullptr;reflected.OverrideModel=nullptr;}
    } clear_overrides_{main_,reflected_};
    // This member is constructed before registrations and rolls them back even
    // when construction fails. Whole-system kill is original normal-exit behavior.
    struct NebulaOwner {
        port::track::ParticleSimulation& effects;
        port::track::SharedParticleState& pools;
        std::vector<std::size_t> handles;
        ~NebulaOwner();
        void release();
    } nebula_;
    void require_open()const;
    void load(int);
    void unload(int);
    void info();
    void locked_car(hp2::debug::VisibleCar*,int,int);
};
}

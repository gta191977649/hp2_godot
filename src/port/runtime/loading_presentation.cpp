#include "loading_presentation.hpp"
#include "event_language_asset.hpp"
#include "game_core/port/race/loading_screen.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/port/race/frontend_language_resources.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/indep/src/timer.hpp"
#include "port/runtime/race_presentation.hpp"
#include "port/assets/catalog.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <stdexcept>
namespace port::runtime {
struct LoadingPresentation::Impl {
 driving::Memory m;track::TextureBindings textures;race::FlailerResources ui;race::FrontendLanguageResources language;
 std::uint32_t animation=0,hash=0;bool visible=false;std::uint64_t revision=0;
 std::vector<hp2::frontend::FeScreen> committed;
 explicit Impl(const std::string& start,int selected_language):m(driving::read_debug_image(driving::debug_image_path(start))),textures(m),ui(m),language(m){const auto root=hp2::platform::resolve_zzdata_root(start);if(!root)throw std::runtime_error("Original loading screen requires configured ZZDATA");assets::DirectorySource source(*root);hp2::debug::Sub_001462B0(m);textures.init();const auto global=source.read("GLOBAL/GLOBALB.BUN");textures.load_bundle(global);ui.load_bundle(global);language.load(source.read(event_language_asset(selected_language)));}
 void publish(){committed=visible?race::snapshot_visible_flailer(m,hash,true):std::vector<hp2::frontend::FeScreen>{};if(visible&&committed.size()!=1)throw std::runtime_error("Original loading screen missing or ambiguous");++revision;}
 void end(){if(animation){const auto screen=hp2::debug::FindFlailerScreen(m,hash);if(screen)hp2::debug::FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,1);hp2::debug::tLoadingScreenAnimation(m,animation).Sub_0016ED80(3);animation=0;}visible=false;publish();}
 // This isolated logical Memory owns every source allocation. Explicit end()
 // performs original animation/hide work at the guarded public boundary. A
 // destructor never executes guest logic, publishes or throws during teardown.
 ~Impl()=default;
};
LoadingPresentation::LoadingPresentation(const std::string& start,int language):p_(std::make_unique<Impl>(start,language)){}
LoadingPresentation::~LoadingPresentation()=default;
void LoadingPresentation::begin(int game,int transmission,bool demo){p_->end();auto& m=p_->m;m.word(0x3275ac,4);m.word(0x3275a8,0);m.word(0x2e78e0,demo?1:0);m.word(0x2f3890+0x1072c,std::uint32_t(game));m.byte(0x2f3890+0x10d7f,std::uint8_t(transmission));p_->hash=demo?0xf46de7f7u:0xd0131dd1u;p_->animation=m.allocate(12);hp2::debug::tLoadingScreenAnimation animation(m,p_->animation);animation.Sub_0016ECB0(int(0xd0131dd1));const auto screen=hp2::debug::FindFlailerScreen(m,p_->hash);if(!screen)throw std::runtime_error("Actual authored loading resource is absent");hp2::debug::FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,0);p_->visible=true;animation.Tick();p_->publish();}
void LoadingPresentation::tick(double elapsed){if(!std::isfinite(elapsed)||elapsed<0)throw std::invalid_argument("Loading elapsed time must be finite/nonnegative");if(!p_->visible)return;hp2::debug::PrepareRealTimestep(p_->m,float(elapsed));hp2::debug::tLoadingScreenAnimation(p_->m,p_->animation).Tick();hp2::recovered::AdvanceRealTime(p_->m);p_->publish();}
void LoadingPresentation::end(){p_->end();}
void LoadingPresentation::close(){p_->end();p_->language.close();p_->ui.close();p_->textures.unload_all();}
bool LoadingPresentation::active()const{return p_->visible;}
std::uint32_t LoadingPresentation::screen_hash()const{return p_->hash;}
std::uint64_t LoadingPresentation::revision()const{return p_->revision;}
const std::vector<hp2::frontend::FeScreen>& LoadingPresentation::documents()const{return p_->committed;}
std::array<float,4> LoadingPresentation::clip_bounds()const{return {{p_->m.scalar(0x3271e8),p_->m.scalar(0x3271ec),p_->m.scalar(0x3271f0),p_->m.scalar(0x3271f4)}};}
}

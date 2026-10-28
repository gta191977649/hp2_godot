#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include "game_core/src/bMath.hpp"
#include "selection_runtime.hpp"
#include "frontend_runtime.hpp"
#include "selection_page.hpp"
#include "race_launch_catalog.hpp"
#include "game_core/port/frontend/event_entry.hpp"
#include "game_core/port/frontend/composition.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "port/vehicle/loader.hpp"
#include "port/vehicle/texture_state.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <future>
#include <cstdio>
namespace port::runtime {
namespace {
using Matrix=std::array<float,16>;
std::array<float,3> point(const Matrix& m,const std::array<float,3>& v){
    std::array<float,3> out;for(int i=0;i<3;++i)out[i]=((v[0]*m[i]+v[1]*m[4+i])+v[2]*m[8+i])+m[12+i];return out;
}
Matrix multiply(const Matrix& a,const Matrix& b){Matrix out;hp2::recovered::eMulMatrix(out,a,b);return out;}
struct Lease final:port::selection::PageLease {
    std::function<void()> release;explicit Lease(std::function<void()> f):release(std::move(f)){}
    ~Lease(){if(release)release();}
};
std::string number_format(std::string format,int value){
    const auto at=format.find("%d");if(at==std::string::npos)return format;
    format.replace(at,2,std::to_string(value));return format;
}
std::shared_ptr<const port::render::Bitmap> bitmap(const hp2::assets::DecodedTexture& image){
    auto b=std::make_shared<port::render::Bitmap>();b->width=image.width;b->height=image.height;b->rgba=image.rgba;return b;
}
}
struct SelectionRuntime::Impl {
    FrontEndRuntime& host;
    port::driving::Memory memory;
    std::vector<std::uint8_t> global;
    std::unique_ptr<port::race::LaunchCatalog> catalogue;
    std::vector<hp2::debug::EventTree> trees;
    hp2::frontend::EventTree_001FFC40 retail_tree;
    int record=0,challenge_mode=0,frames=0;
    std::uint32_t help=0;float elapsed=0,clock=0;
    hp2::debug::FrontendDatabase fred;
    hp2::debug::RaceEvent race_event;
    std::vector<std::pair<hp2::debug::EventGoal,int>> unlocks;
    std::unique_ptr<hp2::debug::VisibleCarSet> source_list;
    port::selection::UnlockTables unlock_tables;
    port::selection::LightingDefaults lighting_defaults;
    std::array<hp2::debug::SelectCarLights,2> lights{};
    port::selection::CameraFrame camera;
    port::track::SharedParticleState pools;
    std::unique_ptr<port::track::ParticleSimulation> effects;
    std::map<std::uint32_t,std::unique_ptr<port::vehicle::Model>> solids;
    struct Job {
        int type=0,skin=0;std::uint64_t generation=0;
        std::future<std::unique_ptr<port::vehicle::Model>> pending;
        std::shared_ptr<port::vehicle::Model> model;
    };
    std::map<int,std::shared_ptr<Job>> jobs;
    std::uint64_t generation=0;
    std::map<hp2::debug::FrontEndRenderingCar*,std::shared_ptr<port::vehicle::Model>> cars;
    std::map<std::pair<int,int>,port::selection::PageJoy> joy;
    hp2::debug::SelectCarCameraMover* camera_mover=nullptr;
    std::array<int,2> preloaded{{-1,-1}};
    std::map<int,port::selection::PageHandle> preload_leases;
    bool loading_frontend=false,roundrobin=false;
    int platform_registrations=0;
    std::map<std::uint64_t,std::shared_ptr<const port::render::Bitmap>> textures;
    std::unique_ptr<port::selection::SelectionPage> page;
    explicit Impl(FrontEndRuntime& h):host(h),memory(port::driving::read_debug_image(port::driving::debug_image_path(h.profile_search_start_))){
        if(!host.event_tree_.loaded())throw std::logic_error("Selection entry requires an active original event tree");
        retail_tree=*host.event_tree_.selected_tree();record=host.event_tree_.cursor();
        global=host.asset_source_->read("GLOBAL/GLOBALB.BUN");
        catalogue=std::make_unique<port::race::LaunchCatalog>(global,memory);
        trees=port::race::parse_debug_event_tables(host.asset_source_->read(host.event_tree_bundle_));
        port::selection::initialize_camera_offsets(memory);
        lighting_defaults=port::selection::load_lighting_defaults(memory);unlock_tables=port::selection::decode_unlock_tables(memory);
        // Host-owned projection of the shared original default RNG across pages.
        pools.seed=host.random_state_;memory.word(0x329060,pools.seed);
        auto resources=port::track::decode_particles(global);resources.emitters.clear();
        effects=std::make_unique<port::track::ParticleSimulation>(std::move(resources),pools);
        // E-0098-V02: individually reviewed settings and per-mode RideInfo fields.
        for(int p=0;p<2;++p){auto& player=fred.FrontendPlayers[p];const auto base=std::size_t(p)*0x6d0;
            player.mPlayerSettings.DrivingStyle=host.profile_.read_u8(base+0x10d7e);
            player.mPlayerSettings.TransmissionType=host.profile_.read_u8(base+0x10d7f);
            player.mPlayerSettings.PreferredGender=host.profile_.read_u8(base+0x10d83);
            for(int mode=0;mode<11;++mode){auto& ride=player.RideInfoByGameMode[mode];auto at=base+0x10dac+mode*8;
                ride.Type=host.profile_.read_i32(at);ride.InstanceIndex=host.profile_.read_u8(at+4);ride.SkinNumber=host.profile_.read_u8(at+5);
                ride.HasDash=host.profile_.read_u8(at+6);ride.CanBeVertexDamaged=host.profile_.read_u8(at+7);}
        }
        auto& tree=trees.at(retail_tree.type_0x08);const auto& event=tree.pEvents.at(record);
        // Only reviewed common time-state slices are shared; KO layouts are not overlaid.
        const auto layout=hp2::frontend::build_profile_layout_001FE988(retail_tree);
        const auto offset=layout.offsets.at(record);
        if(event.nType!=5 || tree.nType!=1 || event.nNumber!=1 || offset<0)
            hp2::not_implemented("0x001FA378","production event state adapter outside reviewed first delivery");
        auto* state=host.profile_.EventBlock_001FE988(tree.nType)+offset;
        port::race::EventLaunchServices launch{fred,
            [this](const auto& key){return translate_key(key);},[this](int id){return host.event_localize(id);},
            [this](const auto&,const auto&,const auto& text){return hp2::frontend::Sub_001F5F88(retail_tree,record,text,[this](auto ticks){return host.event_timer(ticks);});}};
        launch.car_full_name=[this](int type){return catalogue->cars().at(type).GetFullName([this](int id){return host.event_localize(id);});};
        launch.track_language_id=[this](int number,int alternate){return host.event_names_.track_name_id(number,alternate!=0);};
        hp2::debug::RaceParameters parameters(memory,0x311f78);parameters.InitWithDefaults({});
        port::selection::EventEntryServices entry{fred,memory.view(0x311f78,0x310),challenge_mode,catalogue->cars(),host.port_language_};
        entry.get_status=[this](const auto&){return static_cast<hp2::debug::eEventStatus>(retail_tree.Status_002010B8(host.profile_,record));};
        entry.set_race=[&](auto& t,const auto& e){t.SetRace(memory.view(0x311f78,0x310),e,race_event,{state,4},launch);};
        // Debug001F3630/001F3460: direct children, matching prerequisite goals, capacity4.
        entry.build_unlock_list=[this]{unlocks.clear();
            for(int child:retail_tree.leaves.at(record).outgoing){const auto& e=trees.at(retail_tree.type_0x08).pEvents.at(child);
                for(int i=0;i<e.nGoals;++i){const auto* g=e.GetGoal(trees.at(retail_tree.type_0x08),i);
                    if(g && g->nEvent==retail_tree.records.at(record).number_0x28 && unlocks.size()<4)unlocks.emplace_back(*g,int(e.nNumber));}}
        };
        entry.change_screen=[this](const char* name,bool push){host.pending_class_name_=name;host.pending_push_history_=push;};
        entry.push_screen=[this](const char* name){host.navigation_history_.emplace_back(name);};
        entry.continue_event=[]{hp2::not_implemented("0x001828E8","production multitrack continuation not bound");};
        hp2::debug::tTreeDetails details{&tree,&event};hp2::debug::ScreenCallbackData data;
        details.HandleMessages(hp2::debug::MENUITEM_SELECT,data,entry);
        auto list=lists();source_list=std::make_unique<hp2::debug::VisibleCarSet>(fred.FrontendPlayers[0],fred.GameType,false,list);
    }
    std::string translate_key(const std::string& key){std::string out;
        if(!host.language_ || !host.language_->lookup_hash(hp2::frontend::bStringHash(key.c_str()),out))return key;
        return out; // Original string-key translation returns its input when absent (E-0022-R05).
    }
    port::selection::ListServices lists(){return {catalogue->cars(),{},[this](int type){return catalogue->cars().at(type).GetFullName([this](int id){return host.event_localize(id);});},false};}
    port::selection::PageHandle lease(std::function<void()> f){return std::make_unique<Lease>(std::move(f));}
    int wanted_skin(int type){
        const auto* selected=page?page->selector().VisibleCars.pCurrentVisibleCar:source_list->pCurrentVisibleCar;
        if(selected && selected->eCarType==type)return selected->pCarStatus->aSkins.at(selected->SkinIndex);
        return fred.FrontendPlayers[0].aCarStatus.at(type).aSkins[0];
    }
    port::selection::PageHandle load(int type){
        auto& slot=jobs[type];if(!slot){slot=std::make_shared<Job>();slot->type=type;slot->skin=wanted_skin(type);slot->generation=++generation;}
        auto job=slot;
        return lease([this,type,job]{auto found=jobs.find(type);if(found!=jobs.end()&&found->second==job)jobs.erase(found);});
    }
    void begin_loading(){
        for(auto& entry:jobs){auto& job=*entry.second;if(job.model||job.pending.valid())continue;
            const auto root=host.asset_source_->root();const int type=job.type,skin=job.skin;
            job.pending=std::async(std::launch::async,[root,type,skin]{port::assets::DirectorySource source(root);
                auto result=std::make_unique<port::vehicle::Model>();result->include_lods(false);result->load_id(source,type,skin);return result;});
        }
    }
    bool loaded(int type){auto found=jobs.find(type);if(found==jobs.end())throw std::logic_error("Missing production load lease");auto& job=*found->second;
        if(!job.model && job.pending.valid() && job.pending.wait_for(std::chrono::seconds(0))==std::future_status::ready)job.model=job.pending.get();
        return bool(job.model);}
    void reinit(hp2::debug::FrontEndRenderingCar& car,const hp2::debug::RideInfo& ride){
        auto found=jobs.find(ride.Type);if(found==jobs.end()||!loaded(ride.Type))throw std::logic_error("Reinit before original type completion");
        auto model=found->second->model;
        if(model->skin_id()!=ride.SkinNumber){
            // The completed job/cache is private to this generation; no worker mutates it.
            auto updated=std::make_shared<port::vehicle::Model>(model->cache());updated->include_lods(false);
            updated->load_id(*host.asset_source_,ride.Type,ride.SkinNumber);found->second->model=updated;model=std::move(updated);
        }
        cars.at(&car)=std::move(model);
    }
    void preload(){auto l=lists();port::selection::PreloadServices s;
        s.unload=[this](int t){preload_leases.erase(t);};
        s.load=[this](int t){preload_leases[t]=load(t);};s.begin_loading=[this]{begin_loading();};
        s.set_frontend_roundrobin=[this](int fe,int rr){loading_frontend=fe!=0;roundrobin=rr!=0;};
        hp2::debug::PreloadCurrentSelectedCars(fred,preloaded,l,s);
    }
    void activate(){
        auto& screen=host.screens_.at(host.menu_screen_index_);
        port::selection::WidgetContext widgets{screen,catalogue->cars(),[this](const auto& w){return host.measure_widget(w);},
            [this](int type,int skin){const auto at=catalogue->skin_info(type,skin,0);return at?memory.word(at+0x30):0u;},
            [this](int id){return host.event_localize(id);},[this](const auto& s){return translate_key(s);}};
        port::selection::PageServices s{memory,widgets,fred,*effects,pools,{},port::selection::OrbitServices{catalogue->cars()}};
        // Retail00198518 reads player points0030CE54; profile base002FC050.
        s.points=host.profile_.read_i32(0x10e04);s.spotlight_colours=memory.word(0x327af4);
        s.rendering.get_skin_info=[this](int type,int skin,int fallback){if(!catalogue->skin_info(type,skin,fallback))hp2::not_implemented("0x00186330","selected original skin record absent");};
        s.rendering.reinit=[this](auto& car,const auto& ride){reinit(car,ride);};
        s.model=[this](std::uint32_t hash){if(!solids.count(hash)){auto model=std::make_unique<port::vehicle::Model>();model->load_frontend_solid(*host.asset_source_,hash);solids.emplace(hash,std::move(model));}
            ++platform_registrations;return lease([this]{--platform_registrations;});};
        s.car=[this](auto& car,int view){if(view!=1)hp2::not_implemented("0x00192B78","production selection is single-player only");cars.emplace(&car,nullptr);return lease([this,p=&car]{cars.erase(p);});};
        s.camera=[this](auto& c,int view){if(view!=1)throw std::logic_error("Unexpected selection view");camera_mover=&c;return lease([this]{camera_mover=nullptr;});};
        s.joy=[this](int p,int e,const auto& callback){const auto key=std::make_pair(p,e);if(!joy.emplace(key,callback).second)throw std::logic_error("Duplicate production input lease");return lease([this,key]{joy.erase(key);});};
        s.load=[this](int t){return load(t);};s.is_loaded=[this](int t){return loaded(t);};s.begin_loading=[this]{begin_loading();};
        s.preload_selected=[this]{preload();};
        s.dismiss_popup=[](int id){if(id!=-1)hp2::not_implemented("0x00193278","active popup teardown requires production popup owner");};
        s.sound=[](int sound){hp2::frontend::PlayMenuSound_0016E038(sound);};
        s.stop_sound=[this](int sound){SoundRequest request;request.event=hp2::frontend::menu_sound_name_002FB0A8(sound);request.operation=1;host.pending_sound_controls_.push_back(std::move(request));};
        s.help_override=[this](std::uint32_t address){if(address)memory.view(address,4);help=address;};
        s.lighting=[this](int view,float intensity,int silhouette){hp2::debug::SetSelectCarLighting(lights,view,intensity,silhouette,lighting_defaults);};
        s.swatch_rgb=[this](std::uint32_t hash)->std::optional<std::array<std::uint8_t,3>>{auto image=host.catalog_->texture(hash);if(!image||image->rgba.size()<3)return {};return std::array<std::uint8_t,3>{{image->rgba[0],image->rgba[1],image->rgba[2]}};};
        s.render_platform=[this]{if(!solids.count(hp2::frontend::bStringHash("PLATFORM")))throw std::logic_error("Platform model lease absent");};
        s.orbit.camera.publish=[this](const auto& view,float,int fov,float distance){camera=port::selection::camera_frame_from_view(view,fov,distance);};
        s.orbit.camera.sound_enabled=true;
        s.orbit.camera.audio=[this](const auto& r){using Op=port::selection::CameraAudioOp;
            if(r.operation==Op::play)host.pending_volume_feedback_.emplace_back(r.sample,r.value);
            else if(r.operation==Op::stop){SoundRequest out;out.event=r.sample;out.operation=1;host.pending_sound_controls_.push_back(std::move(out));}
            else hp2::not_implemented("0x0015A668","production orbit pitch/volume device conversion not translated");};
        s.orbit.speech_event=[](int,const auto&){hp2::not_implemented("0x001942B0","production speech scheduler not translated");};
        s.orbit.speech_fade=[](float){hp2::not_implemented("0x001942E0","production speech fade scheduler not translated");};
        s.orbit.car_description=[this](int type){return catalogue->cars().at(type).pLanguageInfo->LanguageSpecFullName;};
        s.state.summon_popup=[](const auto&)->int{hp2::not_implemented("0x00194628","production transmission/handling popup owner not translated");};
        s.state.maybe_continue=[]{hp2::not_implemented("0x00196348","production race handoff not yet bound");return false;};
        s.locked.format_unlock_blurb=[this](auto& out,int type,int game,int player){port::selection::UnlockBlurbServices u{unlock_tables,trees,
            [this](int id,std::optional<int> n){auto s=host.event_localize(id);return n?number_format(s,*n):s;}};
            port::selection::FormatUnlockBlurbForCar(out,type,game,player,u);};
        s.locked.render_text=[](auto,auto,auto,std::string_view,float){hp2::not_implemented("0x001929D0","production locked-car rectangle publication not bound");};
        page=std::make_unique<port::selection::SelectionPage>(*source_list,std::move(s),frames,std::uint32_t(clock*4000),elapsed);
        host.random_state_=pools.seed;
    }
    void tick(float dt){elapsed=dt;clock+=dt;++frames;memory.scalar(0x324a48,dt);
        page->tick(frames,std::uint32_t(clock*4000),dt);page->update_camera(dt);}
    void close(){if(!page)return;auto& selector=page->selector();hp2::debug::SelectCarScreen screen;
        screen.NumCarSelectors=1;screen.CarSelectors[0]=&selector;screen.SetSelectedCars(fred,catalogue->cars());
        for(int p=0;p<2;++p){const auto base=std::size_t(p)*0x6d0;const auto& player=fred.FrontendPlayers[p];
            host.profile_.bytes()[base+0x10d7e]=player.mPlayerSettings.DrivingStyle;host.profile_.bytes()[base+0x10d7f]=player.mPlayerSettings.TransmissionType;
            const auto& ride=player.RideInfoByGameMode.at(fred.GameType);auto at=base+0x10dac+fred.GameType*8;
            host.profile_.write_u32(at,ride.Type);host.profile_.bytes()[at+4]=ride.InstanceIndex;host.profile_.bytes()[at+5]=ride.SkinNumber;
            host.profile_.bytes()[at+6]=ride.HasDash;host.profile_.bytes()[at+7]=ride.CanBeVertexDamaged;}
        page->close();page.reset();preload_leases.clear();jobs.clear();
    }
    port::render::FrontendPacket packet(const port::render::Frame&,port::render::Resources&);
};

// Port backend command preparation. Original ordering: E-0098-F03/F04;
// material/projection primitives retain their separately recorded ELF provenance.
port::render::FrontendPacket SelectionRuntime::Impl::packet(const port::render::Frame& ui,port::render::Resources& resources){
    using namespace port::render;FrontendPacket out;out.width=memory.word(0x326760);out.height=memory.word(0x326764);
    hp2::debug::ScreenCallbackData data;page->message(hp2::debug::SCREEN_RENDER,data);
    std::vector<FrontendDraw> deferred;std::vector<port::selection::AlphaKey> keys;
    const int tri[6]={0,1,2,0,2,3};
    const auto publish=[&](FrontendDraw d,bool alpha){if(alpha){deferred.push_back(d);keys.push_back({d.rendering_order,d.sort_depth});}else out.draws.push_back(d);};
    const auto ui_stage=[&](int stage){for(const auto& q:ui.quads){bool background=q.frontend_layer>=20&&q.frontend_layer<=24;if(background!=(stage==1))continue;
        if(q.frontend_layer<0)hp2::not_implemented("0x0010FE48","shared frontend quad is missing authored layer");
        const auto z=port::selection::flailer_depth_0010FE48(memory,q.frontend_layer,16777215.f);
        const float depth=float(port::track::normalized_gpu_depth((z.packed_depth>>4)&0xffffff,0x31));
        auto image=q.texture?resources.texture(q.texture):nullptr;if(q.texture&&!image)throw std::runtime_error("Missing frontend UI texture");if(image)out.textures[q.texture]=image;
        FrontendDraw d;d.first=out.vertices.size();d.count=6;d.texture=q.texture;d.stage=stage;
        for(int j:tri){const Vec2 p=q.has_vertex_data?q.positions[j]:Vec2{q.destination.x+((j==1||j==2)?q.destination.width:0),q.destination.y+(j>=2?q.destination.height:0)};
            const Vec2 uv=q.has_vertex_data?q.source_points[j]:Vec2{q.source.x+((j==1||j==2)?q.source.width:0),q.source.y+(j>=2?q.source.height:0)};
            const auto c=q.has_vertex_data?q.colors[j]:q.color;
            out.vertices.push_back({{{2*(p.x+ui.display_offset.x)/ui.logical_size.x-1,2*(p.y+ui.display_offset.y)/ui.logical_size.y-1,depth,1}},
                {image?uv.x/image->width:0,image?uv.y/image->height:0},{{c.r,c.g,c.b,c.a}}});}
        out.draws.push_back(d);
    }};
    const float focal=hp2::recovered::CalculateH(memory,std::uint16_t(camera.fov_bangle));
    // DebugCamera001566E0 + CreateViewMatricies0010EA18. CM_SELECT_CAR11
    // takes the view1 FOV-adjusted near branch; baseline init001149FC=0x238E.
    const auto tangent=[&](std::uint16_t fov){const auto half=std::uint16_t(fov>>1);return hp2::recovered::bSin(memory,half)/hp2::recovered::bCos(memory,half);};
    const float near_z=.5f*(tangent(0x238e)/tangent(std::uint16_t(camera.fov_bangle)));
    const float far_z=near_z+(memory.scalar(0x324f90)-.5f);
    const auto project=[&](const std::array<float,3>& world,int bias){auto v=point(camera.view,world);float z=v[2];
        const auto depth=port::track::screen_depth_to_gs(port::track::screen_depth(z,near_z,far_z,bias,true),0x31);
        return std::array<float,4>{{v[0]*focal*memory.scalar(0x326770)*2/out.width,v[1]*focal*memory.scalar(0x326774)*2/out.height,float(port::track::normalized_gpu_depth(depth,0x31))*z,z}};
    };
    const auto model_draw=[&](port::vehicle::Model& model,const Matrix& world,std::uint32_t flags,int stage,bool solids_only,bool alpha_only){
        model.select(120,0);
        for(const auto& texture:model.textures()){const TextureId id=0x200000000ull|texture.header.tex_hash;
            auto& cached=textures[id];if(!cached)cached=bitmap(texture.bitmap);out.textures[id]=cached;}
        for(const auto& part:model.parts()){if(!part.enabled||!part.selected)continue;const auto transform=multiply(part.transform,world);
            const auto& solid=model.geometry().solids.at(part.solid);const auto light_words=hp2::debug::elSetupLightContext(lights[0],camera.view,transform);
            port::vehicle::VuLightBlock lit;std::memcpy(lit.data(),light_words.data(),sizeof(lit));const auto inverse=hp2::recovered::bInvertMatrix(transform);
            const auto local_camera=point(inverse,camera.position);
            for(const auto& surface:solid.surfaces){const auto texture=std::find_if(model.textures().begin(),model.textures().end(),[&](const auto& t){return t.header.tex_hash==surface.texture_hash;});
                if(texture==model.textures().end())throw std::runtime_error("Selected solid texture missing");
                auto state=hp2::recovered::eModel::RenderStrips(texture->header,surface.render_flags,flags,true,true,false,false);
                // Debug0010BBF8 frontend default alpha pack is1 (pack2 stays2).
                if(state && state->alpha_pass && state->matrix_pack==0)state->matrix_pack=1;
                if(!state || (solids_only&&state->alpha_pass) || (alpha_only&&!state->alpha_pass))continue;
                FrontendDraw d;d.first=out.vertices.size();d.texture=0x200000000ull|surface.texture_hash;d.raw_gs=true;d.stage=stage;
                d.blend=state->texture.additive?2:state->texture.blend==port::vehicle::BlendMode::Copy?0:1;
                d.depth_write=state->test.write_depth;d.alpha_threshold=state->test.alpha_reference-(state->test.alpha_test==5?0.5f:0.f);
                d.repeat=(state->texture.repeat_u?1:0)|(state->texture.repeat_v?2:0);d.rendering_order=state->rendering_order;
                std::array<float,3> center;for(int i=0;i<3;++i){float a,b;std::memcpy(&a,surface.strip_entry.data()+0x10+i*4,4);std::memcpy(&b,surface.strip_entry.data()+0x20+i*4,4);center[i]=(a+b)*.5f;}
                d.sort_depth=point(camera.view,point(transform,center))[2];
                port::vehicle::VuMaterialBlock material{};if(state->flags&0x180){const auto& block=model.materials().at(surface.material_hash).blocks;std::memcpy(material.data(),block.data(),sizeof(material));}
                for(auto index:surface.indices){const auto& v=surface.vertices.at(index);std::array<float,4> colour{{float((v.raw_color&31)*8),float(((v.raw_color>>5)&31)*8),float(((v.raw_color>>10)&31)*8),float(v.raw_color&0x8000?128:0)}};
                    if(state->flags&0x80){auto reflected=port::vehicle::reflection_002A7F90(v.position,local_camera,v.normal,port::vehicle::reflection_coefficients_0010AC50());auto c=port::vehicle::base_rgba_002A8308(v.normal,reflected,v.raw_color,material,lit,(state->flags&4)!=0);for(int k=0;k<4;++k)colour[k]=c[k];}
                    out.vertices.push_back({project(point(transform,v.position),state->matrix_pack),{v.uv[0],v.uv[1]},colour});}
                d.count=out.vertices.size()-d.first;publish(d,state->alpha_pass);
                if(state->second_pass_320)hp2::not_implemented("0x0010AC50","frontend second-context320 command not translated");
                if(state->environment_pass){
                    port::vehicle::Textures loader(*host.asset_source_,model.cache()->textures);const auto env=loader.frontend_environment();const TextureId id=0x200000000ull|env.header.tex_hash;auto& cached=textures[id];if(!cached)cached=bitmap(env.bitmap);out.textures[id]=cached;
                    auto environment=port::eCreateLookAtMatrix(camera.position,{{world[12],world[13],world[14]}},{{0,0,1}});environment=multiply(transform,environment);
                    FrontendDraw e=d;e.first=out.vertices.size();e.texture=id;e.blend=1;e.depth_write=false;e.alpha_threshold=-1;
                    for(auto index:surface.indices){const auto& v=surface.vertices.at(index);auto reflected=port::vehicle::reflection_002A7F90(v.position,local_camera,v.normal,port::vehicle::reflection_coefficients_0010AC50());auto uv=port::vehicle::environment_002A8AD8(reflected,environment,material);
                        out.vertices.push_back({project(point(transform,v.position),state->matrix_pack),{uv[0],uv[1]},{{128,128,128,uv[2]}}});}
                    publish(e,state->alpha_pass);
                }
            }
        }
    };
    const auto car=[&](const hp2::debug::FrontEndRenderingCar& c,int stage,bool opaque,bool alpha){if(!c.Visible)return;
        auto found=cars.find(const_cast<hp2::debug::FrontEndRenderingCar*>(&c));port::vehicle::Model* model=c.OverrideModel?solids.at(c.OverrideModel->NameHash).get():found==cars.end()?nullptr:found->second.get();if(!model)return;
        const auto body=std::find_if(model->parts().begin(),model->parts().end(),[](const auto& p){return p.group==port::vehicle::Part::Group::Body&&p.lod==0;});
        if(body==model->parts().end())throw std::runtime_error("Selection Body0 missing");
        const auto& bounds=model->geometry().solids.at(body->solid);
        const auto world=port::selection::frontend_render_matrix(c,hp2::recovered::bIdentity(),bounds.bounds_min,bounds.bounds_max);
        if(!c.OverrideModel)model->apply_frontend_pose(c.TireMatrices,c.BrakeMatrices);
        model_draw(*model,world,port::selection::frontend_render_flags(c),stage,opaque,alpha);
    };
    car(page->main_car(),0,true,false);ui_stage(1);
    model_draw(*solids.at(hp2::frontend::bStringHash("PLATFORM")),hp2::recovered::bIdentity(),0,2,false,false);
    // Particles share the platform/main-alpha domain before the stage5 flush.
    port::track::ParticleView pv;pv.focal_h=focal;pv.z_shift=4;
    pv.outside_clip=[&](const auto& p){const auto v=point(camera.view,p);return v[2]<near_z || v[2]>far_z;};
    pv.project=[&](const auto& p){const auto v=point(camera.view,p);const float z=port::track::screen_depth(v[2],near_z,far_z,1,true);std::uint32_t bits;std::memcpy(&bits,&z,4);
        return port::track::ParticleProjection{v[2],2048+v[0]*focal*memory.scalar(0x326770)/v[2],2048+v[1]*focal*memory.scalar(0x326774)/v[2],bits};};
    pv.texture_available=[&](auto hash){return bool(resources.texture(hash));};
    for(const auto& draw:hp2::recovered::AcidEffectSystem::RenderEffects(memory,*effects,pv)){
        FrontendDraw d;d.first=out.vertices.size();d.texture=draw.texture;d.stage=2;d.depth_write=false;d.sort_depth=draw.depth;out.textures[d.texture]=resources.texture(d.texture);
        const int particle_tri[6]={0,1,3,0,3,2};for(const auto& q:draw.quads)for(int i:particle_tri){out.vertices.push_back({{{(q.xy[i][0]/16.f-2048)*2/out.width,(q.xy[i][1]/16.f-2048)*2/out.height,float(port::track::normalized_gpu_depth(q.native_depth,0x31)),1}},
            {q.uv[i][0],q.uv[i][1]},{{q.colour[0]/128.f,q.colour[1]/128.f,q.colour[2]/128.f,q.colour[3]/128.f}}});}d.count=out.vertices.size()-d.first;publish(d,true);
    }
    car(page->reflected_car(),4,true,false);car(page->main_car(),4,false,true);
    for(auto i:port::selection::alpha_order_001053B8(keys)){auto d=deferred[i];d.stage=5;out.draws.push_back(d);}ui_stage(7);
    return out;
}
SelectionRuntime::SelectionRuntime(FrontEndRuntime& host):p_(std::make_unique<Impl>(host)){}
SelectionRuntime::~SelectionRuntime()=default;
void SelectionRuntime::activate(){p_->activate();}
void SelectionRuntime::tick(float dt){if(p_->page)p_->tick(dt);}
void SelectionRuntime::close(){p_->close();}
SelectionObservation SelectionRuntime::observation()const{
    SelectionObservation out;if(!p_->page)return out;const auto& s=p_->page->selector();out.active=true;
    const auto* car=s.VisibleCars.pCurrentVisibleCar;out.car=car?car->eCarType:-1;
    out.skin=car?car->pCarStatus->aSkins.at(car->SkinIndex):-1;out.visible_cars=s.VisibleCars.TotalNumCars;
    out.loaded=s.LoadedCar==car && s.LoadedCar && !s.LoadingCar;out.overlay=s.FadeCarOverlayAlpha;out.generation=p_->generation;return out;
}
bool SelectionRuntime::input(int port,int event,bool pressed){
    if(!p_->page)return false;
    using namespace hp2::input;
    const int mapped=event>=33&&event<=43?event:-1;
    if(mapped>=0){auto found=p_->joy.find({port,mapped});if(found!=p_->joy.end())found->second(port,mapped,pressed?1:0);return true;}
    if(!pressed)return true;
    hp2::debug::ScreenMessage msg=hp2::debug::MESSAGE_NONE;
    switch(event){case JOY_EVENT_MENU_SELECT:msg=hp2::debug::MENUITEM_SELECT;break;
    case JOY_EVENT_SCROLL_LEFT:case JOY_EVENT_MENU_ANALOG_LEFT:msg=hp2::debug::MENUITEM_PAD_LEFT;break;
    case JOY_EVENT_SCROLL_RIGHT:case JOY_EVENT_MENU_ANALOG_RIGHT:msg=hp2::debug::MENUITEM_PAD_RIGHT;break;
    case JOY_EVENT_SCROLL_UP:case JOY_EVENT_MENU_ANALOG_UP:msg=hp2::debug::MENUITEM_PAD_UP;break;
    case JOY_EVENT_SCROLL_DOWN:case JOY_EVENT_MENU_ANALOG_DOWN:msg=hp2::debug::MENUITEM_PAD_DOWN;break;
    case JOY_EVENT_MENU_BACK:msg=hp2::debug::SCREEN_BACK_BUTTON;break;
    case JOY_EVENT_START:msg=hp2::debug::SCREEN_START_BUTTON;break;
    case JOY_EVENT_SELECT:msg=hp2::debug::SCREEN_SELECT_BUTTON;break;
    case JOY_EVENT_MENU_HELP:hp2::not_implemented("0x002491B0","production selection Help overlay not translated");
    default:return true;}
    hp2::debug::ScreenCallbackData data;data.ItemHash=0x52e039f3;data.Port=port;data.Param=1;
    p_->page->message(msg,data);
    if(msg==hp2::debug::SCREEN_BACK_BUTTON && data.Param!=0){p_->host.pending_class_name_="View Event Tree";p_->host.pending_push_history_=false;
        if(!p_->host.navigation_history_.empty()&&p_->host.navigation_history_.back()=="View Event Tree")p_->host.navigation_history_.pop_back();}
    return true;
}
void SelectionRuntime::after_display(){if(p_->page){p_->page->after_display();p_->host.random_state_=p_->pools.seed;}}
port::render::FrontendPacket SelectionRuntime::packet(const port::render::Frame& ui,port::render::Resources& resources){return p_->packet(ui,resources);}
}

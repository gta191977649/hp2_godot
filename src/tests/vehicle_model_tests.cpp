#include "port/vehicle/model.hpp"
#include "port/vehicle/loader.hpp"
#include <chrono>
#include "platform/ps2_asset_root.hpp"
#include "port/assets/vehicle_geometry.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <set>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Expected explicit vehicle failure");}

class CountingSource final : public port::assets::Source {
    port::assets::Source& original_;
public:
    std::map<std::string,std::size_t> reads;
    std::string capture_path;
    std::vector<std::uint8_t> geometry_bytes;
    explicit CountingSource(port::assets::Source& original):original_(original){}
    const std::string& root()const override{return original_.root();}
    std::vector<std::uint8_t> read(const std::string& path)override {
        ++reads[path];
        auto bytes=original_.read(path);
        if(path==capture_path && geometry_bytes.empty())geometry_bytes=bytes;
        return bytes;
    }
    std::size_t geometry_reads()const {
        std::size_t total=0;
        for(const auto& entry:reads)if(entry.first.find("/GEOMETRY.BIN")!=std::string::npos)total+=entry.second;
        return total;
    }
};

const port::vehicle::CarChoice& by_code(const port::vehicle::Catalog& catalog,const char* code) {
    const auto found=std::find_if(catalog.cars().begin(),catalog.cars().end(),
        [&](const auto& car){return car.code==code;});
    check(found!=catalog.cars().end(),"Required original catalog code is absent");
    check(found->available && !found->skins.empty(),"Required catalog sample has no available original geometry/skin");
    return *found;
}

struct BodySnapshot {
    std::vector<std::uint32_t> textures,materials,indices;
    std::vector<float> positions,uv;
};
BodySnapshot body_snapshot(const port::vehicle::Model& model) {
    BodySnapshot result;
    for(const auto& part:model.parts())if(part.group==port::vehicle::Part::Group::Body) {
        for(const auto& surface:model.geometry().solids.at(part.solid).surfaces) {
            if((surface.render_flags&0x400)||surface.indices.empty())continue;
            result.textures.push_back(surface.texture_hash);result.materials.push_back(surface.material_hash);
            result.indices.insert(result.indices.end(),surface.indices.begin(),surface.indices.end());
            for(const auto& vertex:surface.vertices) {
                result.positions.insert(result.positions.end(),vertex.position.begin(),vertex.position.end());
                result.uv.insert(result.uv.end(),vertex.uv.begin(),vertex.uv.end());
            }
        }
    }
    check(!result.positions.empty(),"Catalog load has no original body vertices");
    return result;
}

void check_skin_bindings(const port::vehicle::Model& model,const port::vehicle::CarChoice& car,
                         const port::vehicle::SkinChoice& skin) {
    check(model.car_id()==static_cast<int>(car.id) && model.skin_id()==static_cast<int>(skin.id),
          "Model identity differs from selected original car/skin IDs");
    check(model.path()==car.resource_path,"Model did not use the original catalog resource mapping");
    const auto body=body_snapshot(model);
    const std::string selected=car.prefix+"_SKIN"+std::to_string(skin.id);
    const auto primary=hp2::frontend::bStringHash(selected.c_str());
    const auto secondary=hp2::frontend::bStringHash((selected+"B").c_str());
    check(std::find(body.textures.begin(),body.textures.end(),primary)!=body.textures.end() ||
          std::find(body.textures.begin(),body.textures.end(),secondary)!=body.textures.end(),
          "Selected original skin hash is absent from bound body surfaces");
    check(std::find(body.materials.begin(),body.materials.end(),skin.material_hash)!=body.materials.end(),
          "Selected catalog material hash is absent from bound body surfaces");
    check(model.materials().count(skin.material_hash)==1,"Selected original material was not resolved");
    for(auto hash:body.textures) {
        const auto texture=std::find_if(model.textures().begin(),model.textures().end(),
            [&](const auto& entry){return entry.header.tex_hash==hash;});
        check(texture!=model.textures().end(),"A bound body texture is stale or unresolved");
        if(texture->header.name.rfind(car.prefix+"_SKIN",0)==0)
            check(texture->header.name==selected || texture->header.name==selected+"B",
                  "Body LOD retained a texture from an earlier skin selection");
    }
}

void check_original_uv_crop(const port::vehicle::Model& model,const port::render::VehicleGeometry& original) {
    // E-0033-B11: composing the original resize factor and host crop yields
    // raw UV * 32768 / stored scale. This checks integration, not the helper itself.
    for(const auto& part:model.parts())if(part.group==port::vehicle::Part::Group::Body) {
        const auto& solid=model.geometry().solids.at(part.solid);
        const auto raw=std::find_if(original.solids.begin(),original.solids.end(),
            [&](const auto& entry){return entry.hash==solid.hash && entry.name==solid.name;});
        check(raw!=original.solids.end(),"Bound body no longer identifies its original geometry");
        check(raw->surfaces.size()==solid.surfaces.size(),"Skin change altered original surface grouping");
        for(std::size_t i=0;i<solid.surfaces.size();++i) {
            const auto& surface=solid.surfaces[i];const auto& source=raw->surfaces[i];
            if((surface.render_flags&0x400)||surface.indices.empty())continue;
            check(surface.vertices.size()==source.vertices.size() && surface.indices==source.indices,
                  "Skin binding changed original topology");
            for(int axis=0;axis<2;++axis) {
                const auto at=4+axis*2;
                const auto scale=unsigned(source.strip_entry[at])|(unsigned(source.strip_entry[at+1])<<8);
                check(scale!=0,"Original crop scale is zero");
                for(std::size_t vertex=0;vertex<surface.vertices.size();++vertex) {
                    const float before=source.vertices[vertex].uv[axis];
                    const float expected=before*(32768.0f/static_cast<float>(scale));
                    const float actual=surface.vertices[vertex].uv[axis];
                    check(std::isfinite(actual) && std::fabs(actual-expected)<=1e-5f*std::max(1.0f,std::fabs(expected)),
                          "Original UV crop adjustment is missing, stale or applied repeatedly");
                }
            }
        }
    }
}

void check_catalog_changes(port::assets::Source& original) {
    port::vehicle::Catalog catalog(original);catalog.load();
    const auto& player=by_code(catalog,"911TURBO");
    const auto& hp=by_code(catalog,"911TURBOHP");
    check(player.skins.size()>=2,"Player sample needs at least two original skins");
    const auto& first=player.skins.front();
    const auto second=std::find_if(player.skins.begin()+1,player.skins.end(),
        [&](const auto& skin){return skin.material_hash!=first.material_hash;});
    check(second!=player.skins.end(),"Player sample needs two distinct original paint materials");
    CountingSource source(original);source.capture_path=player.resource_path;
    port::vehicle::Model model;
    port::vehicle::Model blurred;blurred.include_lods(false);blurred.include_wheel_blur(true);
    blurred.load(original,"CARS/911TURBO/GEOMETRY.BIN");blurred.select(120,0);
    int blur_parts=0;
    for(const auto& part:blurred.parts())if(part.group==port::vehicle::Part::Group::WheelBlur){
        ++blur_parts;check(part.selected&&part.lights[15]==90,"Forced original blur layer selection/alpha");
        bool blur_texture=false;
        for(const auto& surface:blurred.geometry().solids[part.solid].surfaces)
            for(const auto& texture:blurred.textures())if(texture.header.tex_hash==surface.texture_hash&&texture.header.name.find("WHEEL_BLUR")!=std::string::npos)blur_texture=true;
        check(blur_texture,"Original blur texture must be bound");
    }
    check(blur_parts==4,"Four original wheel blur instances");
    const auto cached_reads=blurred.cache()->reads;
    const auto static_parts=blurred.parts();blurred.advance_wheels(.01);
    bool tire_changed=false;
    for(std::size_t i=0;i<blurred.parts().size();++i){const auto& part=blurred.parts()[i];if(part.group==port::vehicle::Part::Group::Tire)tire_changed|=part.transform!=static_parts[i].transform;if(part.group==port::vehicle::Part::Group::WheelBlur)check(part.transform==static_parts[i].transform,"Blur layer must not spin with original tire");}
    check(tire_changed&&blurred.cache()->reads==cached_reads,"Animation must rotate tires without asset reads");
    blurred.advance_wheels(100000);check(blurred.wheel_phase()>=0&&blurred.wheel_phase()<6.284,"Preview phase must wrap");
    const auto speed_phase=blurred.wheel_phase();blurred.set_wheel_speed(0);blurred.advance_wheels(.1);
    check(blurred.wheel_phase()==speed_phase,"Zero speed must preserve phase");blurred.select(120,0);
    for(const auto& part:blurred.parts())if(part.group==port::vehicle::Part::Group::WheelBlur)check(!part.selected&&part.lights[15]==0,"Zero speed suppresses original blur submit");
    blurred.set_wheel_speed(-25);blurred.advance_wheels(1);blurred.select(120,0);
    check(blurred.wheel_phase()>=0&&blurred.wheel_phase()<6.284,"Reverse preview phase must wrap");
    for(const auto& part:blurred.parts())if(part.group==port::vehicle::Part::Group::WheelBlur)check(part.selected&&part.lights[15]==45,"Original per-wheel strength follows preview angular velocity");
    check(blurred.cache()->reads==cached_reads,"Speed adjustment must not reload resources");
    rejects([&]{blurred.set_wheel_speed(151);});
    blurred.set_wheel_blur(false);check(blurred.wheel_phase()==0,"Off resets phase");
    blurred.advance_wheels(.01);check(blurred.wheel_phase()==0,"Off stops animation");
    // E-0038-W01: raw zero-spin native matrices and the known-correct static
    // preview must give the same four mesh bases, including wheels0/3.
    const auto static_preview=blurred.parts();
    const auto render_config=port::read_car_render_config(blurred.cache()->global,"911TURBO");
    std::array<port::driving::WheelSnapshot,4> static_native;
    for(unsigned i=0;i<4;++i){
        auto raw_pose=std::array<float,16>{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
        for(unsigned lane=0;lane<3;++lane)raw_pose[12+lane]=render_config.Field_0x120_0x180[i][lane];
        static_native[i].tire=static_native[i].brake=static_native[i].blur=raw_pose;
    }
    blurred.apply_driving_pose(static_native);
    for(std::size_t i=0;i<static_preview.size();++i)if(static_preview[i].wheel>=0)
        for(unsigned lane=0;lane<16;++lane)check(std::abs(static_preview[i].transform[lane]-blurred.parts()[i].transform[lane])<1e-5f,
                                               "Driving raw wheel bases must match the static preview render bases");
    // TASK-0038: absolute independent poses, no preview integration or asset reload.
    std::array<port::driving::WheelSnapshot,4> poses;
    for(int i=0;i<4;++i){
        auto identity=std::array<float,16>{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
        identity[12]=float(i+1);
        poses[i].tire=identity;identity[13]=2;poses[i].brake=identity;
        identity[14]=3;poses[i].blur=identity;poses[i].blur_alpha=float(i*15);
    }
    port::CarAttachmentMatrices raw_poses;
    for(unsigned i=0;i<4;++i){raw_poses.tire[i]=poses[i].tire;raw_poses.brake[i]=poses[i].brake;raw_poses.blur[i]=poses[i].blur;}
    const auto rendered=hp2::recovered::CarRenderInfo::Sub_00120EF8(raw_poses);
    blurred.apply_driving_pose(poses);blurred.select(120,0);
    for(const auto& part:blurred.parts())if(part.wheel>=0){
        if(part.group==port::vehicle::Part::Group::Tire)check(part.transform==rendered.tire[part.wheel],"Independent driving tire pose");
        if(part.group==port::vehicle::Part::Group::Brake)check(part.transform==rendered.brake[part.wheel],"Independent driving brake pose");
        if(part.group==port::vehicle::Part::Group::WheelBlur){check(part.transform==rendered.blur[part.wheel],"Independent driving blur pose");check(part.selected==(part.wheel>0),"Driving per-wheel opacity selection");}
    }
    const auto first_driving=blurred.parts();blurred.apply_driving_pose(poses);
    for(std::size_t i=0;i<first_driving.size();++i)check(first_driving[i].transform==blurred.parts()[i].transform,
                                                      "Repeated driving poses must not accumulate render basis correction");
    const auto before_bad=blurred.parts();poses[3].tire[0]=std::numeric_limits<float>::quiet_NaN();
    rejects([&]{blurred.apply_driving_pose(poses);});
    for(std::size_t i=0;i<before_bad.size();++i)check(before_bad[i].transform==blurred.parts()[i].transform,"Invalid snapshot cannot partially publish");
    rejects([&]{blurred.advance_wheels(.01);});rejects([&]{blurred.set_wheel_speed(10);});
    check(blurred.cache()->reads==cached_reads,"Driving publications do not load assets");
    model.load_id(source,int(player.id),int(first.id));check_skin_bindings(model,player,first);
    const auto baseline=body_snapshot(model);
    const auto raw=port::assets::decode_vehicle_geometry(source.geometry_bytes);
    check_original_uv_crop(model,raw);
    const auto reads=source.geometry_reads();
    model.load_id(source,int(player.id),int(second->id));check_skin_bindings(model,player,*second);
    check(source.geometry_reads()==reads,"Changing only skin reread original geometry bytes");
    const auto repainted=body_snapshot(model);
    check(repainted.textures!=baseline.textures && repainted.materials!=baseline.materials,
          "Skin change failed to rebind original texture/material hashes");
    check(repainted.positions==baseline.positions && repainted.indices==baseline.indices && repainted.uv==baseline.uv,
          "Skin change altered geometry or accumulated UV scaling");
    check_original_uv_crop(model,raw);
    model.load_id(source,int(player.id),int(first.id));check_skin_bindings(model,player,first);
    const auto restored=body_snapshot(model);
    check(source.geometry_reads()==reads,"Restoring a skin reread original geometry bytes");
    check(restored.textures==baseline.textures && restored.materials==baseline.materials && restored.uv==baseline.uv,
          "Restoring the first skin retained stale texture/material/UV state");

    check(hp.resource_path==player.resource_path,"HP sample must exercise original shared geometry mapping");
    model.load_id(source,int(hp.id),int(hp.skins.front().id));check_skin_bindings(model,hp,hp.skins.front());
    check(source.geometry_reads()==reads,"Switching to an HP entry with shared geometry reread that geometry");
    const auto& spider_hp=by_code(catalog,"SPIDERHP");
    check(spider_hp.resource_path=="CARS/CHALLENGE/GEOMETRY.BIN",
          "Original SPIDERHP resource mapping was replaced by a folder-name heuristic");
    model.load_id(source,int(spider_hp.id),int(spider_hp.skins.front().id));
    check_skin_bindings(model,spider_hp,spider_hp.skins.front());
    for(const char* code:{"BMWM5COP","4DR_SEDAN"}) {
        const auto& car=by_code(catalog,code);
        model.load_id(source,int(car.id),int(car.skins.front().id));check_skin_bindings(model,car,car.skins.front());
    }
    // E-0035-M01: separate mirror solids must follow both original skin pairs.
    const auto& mclaren=by_code(catalog,"MCLAREN");
    for(const auto& skin:mclaren.skins){
        model.load_id(source,int(mclaren.id),int(skin.id));
        int mirrors=0,paint_surfaces=0;
        const auto selected=mclaren.prefix+"_SKIN"+std::to_string(skin.id);
        for(const auto& part:model.parts())if(part.group==port::vehicle::Part::Group::Mirror){
            ++mirrors;
            for(const auto& surface:model.geometry().solids[part.solid].surfaces)
                for(const auto& texture:model.textures())if(texture.header.tex_hash==surface.texture_hash&&texture.header.name.rfind(mclaren.prefix+"_SKIN",0)==0){
                    ++paint_surfaces;check(texture.header.name==selected||texture.header.name==selected+"B","Mirror retained an earlier skin texture");
                }
        }
        check(mirrors==2&&paint_surfaces>=2,"McLaren mirror regression requires both painted mirror solids");
    }
    const auto last_car=model.car_id(),last_skin=model.skin_id();const auto last_path=model.path();
    const auto last=body_snapshot(model);const auto final_reads=source.reads;
    rejects([&]{model.load_id(source,-1,last_skin);});
    rejects([&]{model.load_id(source,int(catalog.cars().size()),last_skin);});
    rejects([&]{model.load_id(source,last_car,-1);});
    rejects([&]{model.load_id(source,last_car,256);});
    check(model.car_id()==last_car && model.skin_id()==last_skin && model.path()==last_path &&
          body_snapshot(model).textures==last.textures && body_snapshot(model).materials==last.materials &&
          body_snapshot(model).uv==last.uv && source.reads==final_reads,
          "Rejected catalog IDs changed the current model or read unrelated original assets");
    std::cout<<"VEHICLE_MODEL_CATALOG_CHANGES_OK\n";
}
}
int main() {
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){
        const auto* configured=std::getenv(hp2::platform::kAssetRootEnvVar);
        check(!(configured&&*configured)&&!hp2::platform::resolve_local_config_value(".","root"),"Configured original asset root is invalid");
        std::cout<<"SKIPPED: original vehicle assets not configured\n";return 77;
    }
    port::assets::DirectorySource input(*root);
    for(const auto* code:{"911TURBO","BMWM5COP","4DR_SEDAN"}) {
        port::vehicle::Model model;
        model.load(input,std::string("CARS/")+code+"/GEOMETRY.BIN");
        check(!model.textures().empty(),"Vehicle must bind original textures");
        check(!model.parts().empty(),"Vehicle parts must be bound");
        const auto normal=model.select(200,-1);
        check(normal.visible&&normal.body==0,"Near vehicle must use body LOD 0");
        auto active_body=[&]{return std::count_if(model.parts().begin(),model.parts().end(),[](const auto& p){return p.selected&&p.group==port::vehicle::Part::Group::Body;});};
        check(active_body()==1,"Exactly one original body must be visible");
        const auto part=std::find_if(model.parts().begin(),model.parts().end(),[](const auto& p){return p.selected&&p.group==port::vehicle::Part::Group::Body;});
        const auto name=part->name;
        model.set_part_visible(name,false);model.select(200,-1);check(active_body()==0,"User part visibility override");
        model.set_part_visible(name,true);model.select(200,-1);check(active_body()==1,"Part visibility restore");
        model.select(3,-1);check(std::none_of(model.parts().begin(),model.parts().end(),[](const auto& p){return p.selected;}),"Original projected-size cull");
        model.select(0,0);check(active_body()==1,"Manual debug LOD override");
        rejects([&]{model.select(120,4);});
        rejects([&]{model.set_part_visible("nonexistent",false);});
        for(const auto& p:model.parts())for(const auto& s:model.geometry().solids[p.solid].surfaces) {
            check(!s.indices.empty()&&s.indices.size()%3==0,"Recovered topology must produce triangles");
            for(const auto i:s.indices)check(i<s.vertices.size(),"Triangle index bound");
        }
        std::cout<<code<<" parts="<<model.parts().size()<<" textures="<<model.textures().size()<<"\n";
    }
    port::vehicle::Model model;
    rejects([&]{model.load(input,"../CARS/911TURBO/GEOMETRY.BIN");});
    rejects([&]{model.load(input,"CARS/911TURBO/GEOMETRY.LZC");});
    rejects([&]{model.load(input,"CARS/missing/GEOMETRY.BIN");});
    check_catalog_changes(input);
    auto cache=std::make_shared<port::vehicle::ModelCache>();
    port::vehicle::Model near(cache);near.include_lods(false);near.load_id(input,39,1);
    const auto near_count=near.geometry().solids.size();
    check(std::all_of(near.parts().begin(),near.parts().end(),[](const auto& p){return p.lod<=0;}),"Near-only load decoded lower LOD parts");
    const auto decodes=cache->textures->decodes,reads=cache->reads;
    port::vehicle::Model again(cache);again.include_lods(false);again.load_id(input,39,1);
    check(cache->reads==reads&&cache->textures->decodes==decodes,"Warm load reread geometry or decoded cached textures");
    again.load_id(input,40,int(cache->catalog[40].skins.front().id));check(cache->reads==reads,"Shared HP geometry was reread");
    const auto color=near.preview_color();for(float component:color)check(std::isfinite(component)&&component>=0&&component<=1,"Material preview color must be finite and bounded");
    port::vehicle::Model all(cache);all.load_id(input,39,1);check(all.geometry().solids.size()>near_count,"Lazy full-LOD load must materialize additional solids");
    port::vehicle::Loader loader;
    auto cancelled=loader.request(*root,"",39,1,true);*cancelled.cancelled=true;
    auto latest=loader.request(*root,"",39,1,false);
    check(latest.result.wait_for(std::chrono::seconds(10))==std::future_status::ready,"Native loader timed out");
    auto loaded=latest.result.get();check(loaded.geometry_reads==1&&loaded.file_reads==4,"Cold load reads global, geometry, car textures and frontend environment once");check(loaded.error.empty()&&loaded.model&&!loaded.model->includes_lods(),"Latest async near request failed");
    std::cout<<"VEHICLE_LOADER cold_cpu_ms="<<loaded.cpu_ms<<" reads="<<loaded.geometry_reads<<" decodes="<<loaded.texture_decodes<<"\n";
    auto warm=loader.request(*root,"",39,1,false);auto warmed=warm.result.get();
    check(warmed.model&&warmed.error.empty()&&warmed.geometry_reads==0&&warmed.texture_decodes==0,"Warm loader must avoid geometry IO and cached pixel decode");
    std::cout<<"VEHICLE_LOADER warm_cpu_ms="<<warmed.cpu_ms<<" reads="<<warmed.geometry_reads<<" decodes="<<warmed.texture_decodes<<"\n";
    auto missing=loader.request(*root,"CARS/missing/GEOMETRY.BIN",-1,1,false);auto rejected=missing.result.get();check(!rejected.error.empty()&&!rejected.model,"Async failure must report an error without a model");
    std::cout<<"VEHICLE_MODEL_OK\n";
}

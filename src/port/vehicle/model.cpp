#include "model.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "port/assets/vehicle_geometry.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <algorithm>
#include <stdexcept>
#include <set>
#include "port/vehicle/base_material.hpp"
#include "port/vehicle/texture_state.hpp"
#include <cstring>
#include <cmath>
#include <chrono>
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"

namespace port::vehicle {
namespace {
std::array<float,16> identity() {return {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};}
}
void Model::load_id(assets::Source& source,int car_id,int skin_id) {
    if(cache_->root!=source.root()){*cache_=ModelCache{};cache_->root=source.root();}
    if(cache_->catalog.empty()){Catalog catalog(source);catalog.load();cache_->catalog=catalog.cars();}
    catalog_=cache_->catalog;
    const auto car=std::find_if(catalog_.begin(),catalog_.end(),[&](const auto& c){return int(c.id)==car_id;});
    if(car==catalog_.end())throw std::invalid_argument("Unknown original car ID");
    const auto skin=std::find_if(car->skins.begin(),car->skins.end(),[&](const auto& s){return int(s.id)==skin_id;});
    if(skin==car->skins.end())throw std::invalid_argument("Color does not belong to this original car");
    if(!car->available)throw std::runtime_error(car->availability_error);
    load(source,car->resource_path,car->code,skin_id);
}
void Model::load(assets::Source& source,const std::string& geometry_path,const std::string& selected_code,int skin) {
    // The viewer entry is intentionally narrower than the original stream loader.
    if(geometry_path.rfind("CARS/",0)!=0 || geometry_path.size()<19 ||
       geometry_path.substr(geometry_path.size()-13)!="/GEOMETRY.BIN")
        throw std::invalid_argument("Expected CARS/<model>/GEOMETRY.BIN; LZC and dashboard input are not supported");
    const auto code=selected_code.empty()?geometry_path.substr(5,geometry_path.size()-5-13):selected_code;
    if(code.empty()||code.find('/')!=std::string::npos||code.find('\\')!=std::string::npos||code.find("..")!=std::string::npos)
        throw std::invalid_argument("Invalid vehicle folder");
    if(skin<0||skin>255)throw std::invalid_argument("Original skin ID outside byte range");
    if(cache_->root!=source.root()){*cache_=ModelCache{};cache_->root=source.root();}
    if(cache_->global.empty())cache_->global=source.read("GLOBAL/GLOBALB.BUN");
    const auto& global=cache_->global;
    const auto config=port::read_car_render_config(global,code,std::uint8_t(skin));
    config_=config;phase_=0;driving_pose_=false;wipers_active_=false;
    const auto bindings=hp2::recovered::CarRenderInfo::Sub_0011E860(config.prefix,config.Field_0x538,std::uint8_t(skin));
    const auto attachments=hp2::recovered::CarRenderInfo::Sub_00120EF8(config,0);
    const auto binding_start=std::chrono::steady_clock::now();double geometry_phase_ms=0;
    std::set<std::uint32_t> wanted;
    auto want=[&](const port::CarGeometryBinding& b){for(const auto& name:{b.name,b.fallback})if(!name.empty())wanted.insert(hp2::frontend::bStringHash(name.c_str()));};
    if(!include_lods_){want(bindings.body[0]);want(bindings.tire_front[0]);want(bindings.tire_rear[0]);for(const auto& b:bindings.accessories)want(b);}
    if(!include_lods_)for(const auto& binding:bindings.wheel_blur)want(binding);
    const auto key=geometry_path+(include_lods_?":all":":near");
    auto cached=cache_->geometry.find(key);
    if(cached==cache_->geometry.end()){
        const auto geometry_start=std::chrono::steady_clock::now();const auto bytes_in=source.read(geometry_path);const auto decode_start=std::chrono::steady_clock::now();
        auto decoded=assets::decode_vehicle_geometry(bytes_in,wanted);++cache_->reads;
        auto volumes=port::track::load_collision_volumes(bytes_in);std::vector<std::array<float,3>> collision_vertices;
        if(const auto* cv=hp2::recovered::FindPipelinePolyhedron(volumes,hp2::frontend::bStringHash((config.prefix+"_CV").c_str()),nullptr))for(const auto& vertex:cv->shape.vertices)collision_vertices.push_back(vertex.position);
        cache_->geometry_decode_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-decode_start).count();
        geometry_phase_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-geometry_start).count();
        std::size_t bytes=decoded.position_markers.size()*sizeof(render::VehiclePositionMarker);for(const auto& solid:decoded.solids)if(solid.rigid_animation)bytes+=solid.rigid_animation->matrices.size()*sizeof(std::array<float,16>);for(const auto& solid:decoded.solids)for(const auto& surface:solid.surfaces)bytes+=surface.vertices.size()*sizeof(render::VehicleVertex)+surface.indices.size()*sizeof(std::uint32_t);
        cache_->bytes+=bytes;cached=cache_->geometry.emplace(key,ModelCache::Geometry{std::move(decoded),std::move(collision_vertices),++cache_->clock,bytes}).first;
    }else{++cache_->hits;cached->second.age=++cache_->clock;}
    geometry_=cached->second.data;collision_vertices_=cached->second.collision_vertices;path_=geometry_path;car_id_=int(config.car_index);skin_id_=skin;
    // Debug00120558 selects all markers for CarTypeInfo.BaseModelName (+0x20),
    // already projected as config.prefix; name filtering belongs to flare logic.
    position_markers_.clear();marker_pivot_={};
    const auto marker_type=hp2::frontend::bStringHash(config.prefix.c_str());
    for(const auto& marker:geometry_.position_markers)if(marker.type_hash==marker_type)position_markers_.push_back(marker);
    while(cache_->bytes>256u*1024u*1024u&&cache_->geometry.size()>1){
        auto oldest=cache_->geometry.end();for(auto it=cache_->geometry.begin();it!=cache_->geometry.end();++it)if(it!=cached&&(oldest==cache_->geometry.end()||it->second.age<oldest->second.age))oldest=it;
        cache_->bytes-=oldest->second.bytes;cache_->geometry.erase(oldest);
    }
    auto find=[&](const port::CarGeometryBinding& binding)->std::size_t {
        for(const auto& name:{binding.name,binding.fallback}) {
            if(name.empty())continue;
            const auto hash=hp2::frontend::bStringHash(name.c_str());
            for(std::size_t i=0;i<geometry_.solids.size();++i)if(geometry_.solids[i].hash==hash)return i;
        }
        return geometry_.solids.size();
    };
    parts_.clear();presence_.fill(false);wheel_blur_available_.fill(false);textures_.clear();materials_.clear();diagnostic_.clear();light_roles_.clear();
    for(const auto& solid:geometry_.solids)if(solid.rigid_animation_unresolved)diagnostic_+="Unresolved original external rigid animation for passive solid "+solid.name+"; ";
    const auto light_hash=[&](const char* suffix){return hp2::frontend::bStringHash((config.prefix+suffix).c_str());};
    light_hashes_={{light_hash("_HEADLIGHT"),light_hash("_HEADLIGHT_ON"),light_hash("_BRAKELIGHT"),light_hash("_BRAKELIGHT_ON")}};
    auto append=[&](const port::CarGeometryBinding& b,int lod,Part::Group group,
                    const std::array<float,16>& matrix,const std::string& suffix={}) {
        auto index=find(b);if(index==geometry_.solids.size())return false;
        {
            // Preserve the original solid while projecting constructor texture rebinding.
            auto projected=geometry_.solids[index];bool changed=false;
            std::vector<int> light_roles(projected.surfaces.size(),-1);
            // E-0082-L01: only Body0..2 receive the dynamic replacement table.
            if(group==Part::Group::Body&&lod<3)for(std::size_t s=0;s<projected.surfaces.size();++s){
                const auto hash=projected.surfaces[s].texture_hash;
                const char* names[]={"_HEADLIGHT_LEFT","_HEADLIGHT_RIGHT","_BRAKELIGHT_LEFT","_BRAKELIGHT_RIGHT"};
                for(int role=0;role<4;++role)if(hash==light_hash(names[role]))light_roles[s]=role;
            }
            for(auto& surface:projected.surfaces)for(const auto& replacement:b.texture_replacements)
                if(surface.texture_hash==replacement.source_hash){surface.texture_hash=replacement.target_hash;changed=true;break;}
            for(auto& surface:projected.surfaces)if(surface.material_hash==0xd6d6080a){surface.material_hash=config.material_hash;changed=true;}
            if(changed){index=geometry_.solids.size();geometry_.solids.push_back(std::move(projected));}
            for(std::size_t s=0;s<light_roles.size();++s)if(light_roles[s]>=0)light_roles_[{index,s}]=light_roles[s];
        }
        const auto label=b.name+(lod>=0?" [LOD "+std::to_string(lod)+"]":std::string{})+suffix;
        parts_.push_back({label,index,matrix,lod,group,true,false,hp2::recovered::CarRenderInfo::Sub_00120EF8(matrix)});return true;
    };
    for(int lod=0;lod<(include_lods_?4:1);++lod)presence_[lod]=append(bindings.body[lod],lod,Part::Group::Body,identity());
    if(!presence_[0])throw std::runtime_error("Original body LOD 0 is missing: "+bindings.body[0].name);
    // Debug0011EA88 takes PivotPosition from Body[0].Solid.PivotMatrix.v3.
    for(const auto& part:parts_)if(part.group==Part::Group::Body&&part.lod==0){const auto& transform=geometry_.solids[part.solid].transform;for(unsigned axis=0;axis<3;++axis)marker_pivot_[axis]=transform[12+axis];break;}
    // E-0033-B01/B02/B04: four original config vectors; static wheel matrices
    // are provided by the recovered null-car path. No physics object is created.
    for(int wheel=0;wheel<4;++wheel) {
        const auto& matrix=attachments.tire[wheel];
        const auto& variants=wheel<2?bindings.tire_front:bindings.tire_rear;
        for(int lod=0;lod<(include_lods_?3:1);++lod)if(append(variants[lod],lod,Part::Group::Tire,matrix," [wheel "+std::to_string(wheel)+"]"))parts_.back().wheel=wheel;
        if(append(bindings.accessories[wheel<2?0:1],-1,Part::Group::Brake,attachments.brake[wheel]," [wheel "+std::to_string(wheel)+"]"))parts_.back().wheel=wheel;
        if(append(bindings.wheel_blur[wheel<2?0:1],-1,Part::Group::WheelBlur,attachments.blur[wheel]," [blur wheel "+std::to_string(wheel)+"]")){
            wheel_blur_available_[wheel<2?0:1]=true;
            parts_.back().wheel=wheel;parts_.back().lights[15]=90*Sub_00120EA8(80);
        }
    }
    // E-0087-B01: constructor0011EA88 permits null after shared/F/R lookup;
    // eModel::Render0010E048 returns for that null solid, even with blur alpha.
    for(unsigned axle=0;axle<2;++axle)if(!wheel_blur_available_[axle])diagnostic_+="Original optional wheel-blur solid absent: "+bindings.wheel_blur[axle].name+" / "+bindings.wheel_blur[axle].fallback+"; ";
    for(const auto& b:bindings.accessories) {
        if(b.object_offset==0x190||b.object_offset==0x1a8){
            // Debug0011EA88 attaches the two-entry SkinReplacementTextureTable.
            auto wiper=b;wiper.texture_replacements.assign(bindings.body[0].texture_replacements.begin(),bindings.body[0].texture_replacements.begin()+2);
            append(wiper,-1,Part::Group::Wiper,identity());
        }
        if(b.object_offset==0x1c0||b.object_offset==0x1d8)append(b,-1,Part::Group::Mirror,identity());
        if(b.object_offset==0x1f0||b.object_offset==0x208)append(b,-1,Part::Group::Plate,identity());
    }
    Textures textures(source,cache_->textures);
    if(std::any_of(parts_.begin(),parts_.end(),[](const auto& p){return p.group==Part::Group::Wiper;})){
        // Debug0010C1B0 ->001A95C8(hash,1,0): absent original wiper texture
        // resolves to the original default19006835, never generated pixels.
        textures.load({0x19006835});
        for(const auto& part:parts_)if(part.group==Part::Group::Wiper)
            for(auto& surface:geometry_.solids[part.solid].surfaces)
                if(!cache_->textures->index.count(surface.texture_hash)){
                    diagnostic_+="Original wiper texture absent; GetTextureInfo001A95C8 default19006835 used; ";
                    surface.texture_hash=0x19006835;
                }
    }
    std::vector<std::uint32_t> hashes;
    for(const auto& part:parts_)for(const auto& surface:geometry_.solids[part.solid].surfaces) {
        if(surface.render_flags&0x400)continue;
        if(surface.indices.empty())continue;
        hashes.push_back(surface.texture_hash);
        if(!materials_.count(surface.material_hash))materials_.emplace(surface.material_hash,port::read_car_material(global,surface.material_hash));
    }
    const auto textures_start=std::chrono::steady_clock::now();
    // Original GetUsedCarTextureInfo loads both light states. Resolve them while
    // loading the model so a frame switch performs no IO or texture decoding.
    bool heads=false,brakes=false;for(const auto& role:light_roles_){heads|=role.second<2;brakes|=role.second>=2;}
    if(heads){hashes.push_back(light_hashes_[0]);hashes.push_back(light_hashes_[1]);}
    if(brakes){hashes.push_back(light_hashes_[2]);hashes.push_back(light_hashes_[3]);}
    textures_=textures.load(hashes);
    // E-0086-M01: Debug LightFlareTextureHashes002CD730 and
    // eLightUpdateTextures00116880 resolve these three optional original hashes.
    // All three are present in the shipped GLOBAL/GLOBALB.BUN TPK. Missing
    // resources report a gap; no substituted texture or invented lamp is used.
    for(auto hash:{std::uint32_t(0x8868656f),std::uint32_t(0x88d8e8a2),std::uint32_t(0x3394fe62)}) {
        if(cache_->textures->index.count(hash)) {
            if(std::none_of(textures_.begin(),textures_.end(),[&](const auto& t){return t.header.tex_hash==hash;})) {
                auto extra=textures.load({hash});textures_.insert(textures_.end(),extra.begin(),extra.end());
            }
        }else diagnostic_+="Original vehicle flare texture missing: "+std::to_string(hash)+"; ";
    }
    if(position_markers_.empty())diagnostic_+="Original BaseModelName position markers missing; ";
    for(unsigned pair=0;pair<2;++pair)if(pair==0?heads:brakes){
        const auto find_header=[&](std::uint32_t hash)->const hp2::assets::TpkTextureHeader&{
            return std::find_if(textures_.begin(),textures_.end(),[&](const auto& t){return t.header.tex_hash==hash;})->header;
        };
        const auto& off=find_header(light_hashes_[2*pair]);const auto& on=find_header(light_hashes_[2*pair+1]);
        const auto a=texture_state_00108CC0(off),b=texture_state_00108CC0(on);
        const auto x=test_state_00105AC0(off),y=test_state_00105AC0(on);
        if(off.width!=on.width||off.height!=on.height||off.shift_width!=on.shift_width||off.shift_height!=on.shift_height
           ||a.blend!=b.blend||a.deferred!=b.deferred||a.alpha_register!=b.alpha_register
           ||x.test_register!=y.test_register||x.write_depth!=y.write_depth)
            hp2::not_implemented("0x00120350","Light texture swap changes UV/GS pass state outside the current binding");
    }
    // Original CarRenderInfo constructor resolves optional shadow textures with
    // GetTextureInfo(...,0,0); missing resources disable their respective branch.
    for(auto hash:{std::uint32_t(0xbadb4475),hp2::frontend::bStringHash((config.prefix+"_SHADOWIG").c_str())})if(cache_->textures->index.count(hash)){auto extra=textures.load({hash});textures_.insert(textures_.end(),extra.begin(),extra.end());}
    // 00103E68 ->00112858 uploads this exact frontend environment at GS TBP0=2800.
    if(std::any_of(parts_.begin(),parts_.end(),[&](const auto& part){const auto& surfaces=geometry_.solids[part.solid].surfaces;return std::any_of(surfaces.begin(),surfaces.end(),[](const auto& surface){return (surface.render_flags&0x100)&&!(surface.render_flags&0x400);});}))
        textures_.push_back(textures.frontend_environment());
    const auto textures_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-textures_start).count();
    std::set<std::size_t> adjusted;
    for(const auto& part:parts_)if(adjusted.insert(part.solid).second)
        for(auto& surface:geometry_.solids[part.solid].surfaces) {
            if((surface.render_flags&0x400)||surface.indices.empty())continue;
            const auto texture=std::find_if(textures_.begin(),textures_.end(),[&](const auto& t){return t.header.tex_hash==surface.texture_hash;});
            if(texture==textures_.end())throw std::runtime_error("Original strip texture was not resolved");
            const auto& entry=surface.strip_entry;
            const auto sx=std::uint16_t(entry[4])|(std::uint16_t(entry[5])<<8);
            const auto sy=std::uint16_t(entry[6])|(std::uint16_t(entry[7])<<8);
            const auto factors=port::FixStripSTs(texture->header.width,texture->header.height,sx,sy);
            for(auto& vertex:surface.vertices)for(int axis=0;axis<2;++axis)vertex.uv[axis]*=factors.cropped_image[axis];
        }
    for(const auto& gap:geometry_.gaps)diagnostic_+=gap+"; ";
    diagnostic_+="Undamaged exterior, selected original skin; original dynamic head/brake textures and marker/pivot/flare texture assets available to the driving flare adapter. Original frontend VU lighting and CARSELECTENVMAP additive reflection; optional port-driven wheel animation. Original runtime equivalence unverified. Full render entry, live game environment capture, alternate material routes, GS destination-read alpha>128 and partial-frustum cone culling remain incomplete";
    select(120,include_lods_?-1:0);
    set_wheel_blur(wheel_blur_);
    cache_->binding_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-binding_start).count()-geometry_phase_ms-textures_ms;
}
void Model::set_wheel_blur(bool enabled){
    if(driving_pose_)throw std::logic_error("Preview controls are disabled for driving snapshots");
    if(enabled&&std::count_if(parts_.begin(),parts_.end(),[](const auto& part){return part.group==Part::Group::WheelBlur;})!=4)
        throw std::runtime_error("Original four-wheel blur geometry is not available");
    wheel_blur_=enabled;phase_=0;advance_wheels(0);
}
void Model::advance_wheels(double delta){
    if(driving_pose_)throw std::logic_error("Preview integration is disabled for driving snapshots");
    if(!std::isfinite(delta)||delta<0)throw std::invalid_argument("Invalid vehicle animation delta");
    if(!wheel_blur_&&delta>0)return;
    // Port-side demonstration input, NOT recovered vehicle speed or simulation.
    // Original 00219A60 publishes phase + delta * angular velocity.
    const float preview_velocity=float(wheel_speed_);
    constexpr double turn=65536.0/10430.3779296875;
    if(wheel_blur_)phase_=std::fmod(phase_+delta*preview_velocity,turn);
    if(phase_<0)phase_+=turn;
    const auto phase=wheel_blur_?float(phase_):0.f;
    const auto matrices=wheel_blur_?CalculateWheelMatrices(config_,{{phase,phase,phase,phase}},0):hp2::recovered::CarRenderInfo::Sub_00120EF8(config_,0);
    for(auto& part:parts_)if(part.wheel>=0){
        if(part.group==Part::Group::Tire)part.transform=matrices.tire[part.wheel];
        else if(part.group==Part::Group::Brake)part.transform=matrices.brake[part.wheel];
        else part.transform=matrices.blur[part.wheel];
        if(part.group==Part::Group::WheelBlur)part.lights[15]=90*Sub_00120EA8(wheel_blur_?preview_velocity:0);
    }
}
void Model::set_wheel_speed(double radians_per_second){
    if(driving_pose_)throw std::logic_error("Preview controls are disabled for driving snapshots");
    if(!std::isfinite(radians_per_second)||std::abs(radians_per_second)>150)
        throw std::invalid_argument("Preview wheel speed must be within -150..150 rad/s");
    wheel_speed_=radians_per_second;advance_wheels(0);
}
std::array<float,3> Model::preview_color()const {
    // Port-side swatch: average actual body paint texels after the recovered VU
    // material calculation at a fixed inspection camera. Not original menu RGB.
    std::array<double,3> sum{};std::size_t count=0;std::vector<std::array<double,3>> samples;
    const std::array<float,3> camera{{6,4,8}};
    for(const auto& part:parts_)if(part.group==Part::Group::Body&&part.lod==0)
        for(const auto& surface:geometry_.solids[part.solid].surfaces){
            const auto texture=std::find_if(textures_.begin(),textures_.end(),[&](const auto& t){return t.header.tex_hash==surface.texture_hash&&t.header.name.find("_SKIN")!=std::string::npos;});
            if(texture==textures_.end()||(surface.render_flags&0x400))continue;
            vehicle::VuMaterialBlock block{};vehicle::VuLightBlock lights{};
            std::memcpy(block.data(),materials_.at(surface.material_hash).blocks.data(),sizeof(block));std::memcpy(lights.data(),part.lights.data(),sizeof(lights));
            for(std::size_t triangle=0;triangle+2<surface.indices.size();triangle+=3){
                std::array<float,2> uv{};std::array<double,4> rgba{};float upward=0;
                for(int corner=0;corner<3;++corner){const auto& vertex=surface.vertices.at(surface.indices[triangle+corner]);
                    uv[0]+=vertex.uv[0]/3;uv[1]+=vertex.uv[1]/3;upward+=vertex.normal[2]/3;
                    std::array<unsigned,4> color{{unsigned((vertex.raw_color&31)*8),unsigned(((vertex.raw_color>>5)&31)*8),unsigned(((vertex.raw_color>>10)&31)*8),128}};
                    if(surface.render_flags&0x80){const auto reflected=vehicle::reflection_002A7F90(vertex.position,camera,vertex.normal,vehicle::reflection_coefficients_0010AC50());const auto lit=vehicle::base_rgba_002A8308(vertex.normal,reflected,vertex.raw_color,block,lights,bool(surface.render_flags&4)||texture->header.Field_0x74!=0);for(int i=0;i<4;++i)color[i]=lit[i];}
                    for(int i=0;i<4;++i)rgba[i]+=double(color[i])/3;
                }
                // Upper-facing paint avoids underbody/window/border samples in this
                // port-only representative tile; no original menu RGB is inferred.
                if(upward<.25f)continue;
                const auto& bitmap=texture->bitmap;
                const int x=std::clamp(int(uv[0]*bitmap.width),0,bitmap.width-1),y=std::clamp(int(uv[1]*bitmap.height),0,bitmap.height-1);
                const auto at=std::size_t(y*bitmap.width+x)*4;if(!bitmap.rgba[at+3])continue;
                std::array<double,3> sample{};for(int i=0;i<3;++i)sample[i]=std::min(255.,std::floor(bitmap.rgba[at+i]*rgba[i]/128));samples.push_back(sample);
            }
        }
    if(samples.empty())throw std::runtime_error("No supported body paint samples for color preview");
    // A representative bright quartile avoids atlas borders dominating the tile.
    // This selection is a documented viewing heuristic, not recovered menu logic.
    std::sort(samples.begin(),samples.end(),[](const auto& a,const auto& b){return a[0]+a[1]+a[2]>b[0]+b[1]+b[2];});
    count=std::max(std::size_t(1),samples.size()/4);for(std::size_t i=0;i<count;++i)for(int channel=0;channel<3;++channel)sum[channel]+=samples[i][channel];
    return {{float(sum[0]/count/255),float(sum[1]/count/255),float(sum[2]/count/255)}};
}
port::CarLodSelection Model::select(int projected_size,int forced_lod) {
    if(forced_lod < -1 || forced_lod>3)throw std::invalid_argument("LOD must be Auto or 0..3");
    auto selection=hp2::recovered::CarRenderInfo::Sub_00120EF8(projected_size,presence_);
    if(forced_lod>=0) {
        if(!presence_[forced_lod])throw std::runtime_error("Requested body LOD is absent in this original resource");
        selection={forced_lod,std::min(forced_lod,2),true,forced_lod<2,forced_lod<3};
    }
    for(auto& part:parts_) {
        bool selected=false;
        switch(part.group) {
        case Part::Group::Body:selected=part.lod==selection.body;break;
        case Part::Group::Tire:selected=selection.body<3 && part.lod==selection.tire;break;
        case Part::Group::Brake:selected=selection.body<2;break;
        case Part::Group::Mirror:selected=selection.mirrors;break;
        case Part::Group::Plate:selected=selection.license_plates;break;
        case Part::Group::Wiper:selected=wipers_active_&&projected_size>40&&geometry_.solids[part.solid].rigid_animation.has_value();break;
        case Part::Group::WheelBlur:selected=(wheel_blur_||driving_pose_)&&part.lights[15]>.1f&&selection.body<2&&selection.tire<2;break;
        }
        part.selected=selection.visible&&selected&&part.enabled;
    }
    return selection;
}
// E0089W04, Debug00123580/0010DEB0/0010DF98/001A4CF8.
// The car body/world transform belongs to the renderer parent. This method
// publishes precisely its local PivotMatrix * authored animation matrix.
void Model::apply_wiper_state(const driving::WiperSnapshot& state) {
    const auto world=static_cast<std::int32_t>(state.world_frame);
    const bool active=state.state!=0||(state.on_time<=world&&world<state.off_time);
    if(active)for(const auto& part:parts_)if(part.group==Part::Group::Wiper&&geometry_.solids[part.solid].rigid_animation_unresolved)
        hp2::not_implemented("0x00103588","Active wiper external rigid animation is outside bound vehicle resources or cyclic");
    const auto start=(static_cast<std::uint32_t>(state.on_time)&0x3fffu)|0xc000u;
    const auto age=(state.world_frame-start)&0x3fffu;
    for(auto& part:parts_)if(part.group==Part::Group::Wiper){
        const auto& solid=geometry_.solids[part.solid];
        if(!solid.rigid_animation)continue;
        const auto& animation=*solid.rigid_animation;
        const auto duration=std::uint32_t(animation.num_frames)*std::uint32_t(animation.frame_rate_scale);
        const auto frame=(age%duration)/std::uint32_t(animation.frame_rate_scale);
        part.transform=hp2::recovered::bMulMatrix(solid.transform,animation.matrices.at(frame));
        part.lights=hp2::recovered::CarRenderInfo::Sub_00120EF8(part.transform);
    }
    wipers_active_=active;
}
void Model::set_part_visible(const std::string& name,bool visible) {
    const auto it=std::find_if(parts_.begin(),parts_.end(),[&](const auto& part){return part.name==name;});
    if(it==parts_.end())throw std::invalid_argument("Unknown vehicle part: "+name);
    it->enabled=visible;
}
std::uint32_t Model::driving_texture(std::size_t solid,std::size_t surface,
                                   const driving::VehicleLightState& state)const{
    const auto& original=geometry_.solids.at(solid).surfaces.at(surface);
    const auto role=light_roles_.find({solid,surface});if(role==light_roles_.end())return original.texture_hash;
    if(role->second<2)return light_hashes_[(role->second==0?state.headlight_left:state.headlight_right)?1:0];
    return light_hashes_[state.brakelights?3:2];
}
}

namespace port::vehicle {
void Model::apply_driving_pose(const std::array<driving::WheelSnapshot,4>& wheels){
    // Validate the whole publication before changing the last completed display.
    driving::validate_wheels(wheels);
    // E-0087-B01: absent original optional blur solids submit nothing through
    // eModel::Render's null-solid guard. Opacity does not require their presence.
    // E-0038-W01: snapshots contain physics publication matrices, while mesh
    // instances need the same render-only basis composition used by preview.
    port::CarAttachmentMatrices source;
    for(std::size_t i=0;i<4;++i){source.tire[i]=wheels[i].tire;source.brake[i]=wheels[i].brake;source.blur[i]=wheels[i].blur;}
    const auto rendered=hp2::recovered::CarRenderInfo::Sub_00120EF8(source);
    driving_pose_=true;
    for(auto& part:parts_)if(part.wheel>=0){
        const auto& wheel=wheels.at(std::size_t(part.wheel));
        if(part.group==Part::Group::Tire)part.transform=rendered.tire[part.wheel];
        else if(part.group==Part::Group::Brake)part.transform=rendered.brake[part.wheel];
        else if(part.group==Part::Group::WheelBlur){part.transform=rendered.blur[part.wheel];part.lights[15]=wheel.blur_alpha;}
    }
}
}

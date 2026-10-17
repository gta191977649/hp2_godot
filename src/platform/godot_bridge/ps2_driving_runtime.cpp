#include "ps2_driving_runtime.hpp"
#include "boundary.hpp"
#include "platform/shaders/car_base_shader.hpp"
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include "platform/ps2_asset_root.hpp"
#include "game_core/port/driving/native_backend.hpp"
#include "game_core/port/driving/surface.hpp"
#include "game_core/port/driving/track_environment.hpp"
#include "port/assets/catalog.hpp"
#include "port/vehicle/catalog.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <stdexcept>
#include <algorithm>
#include "game_core/port/not_implemented.hpp"
namespace port::godot_bridge {
namespace {
godot::Vector3 host(const std::array<float,3>& v){return {v[1],v[2],v[0]};}
godot::Transform3D host_matrix(const port::driving::Matrix& m){
    godot::Basis b;b.set_column(0,host({{m[4],m[5],m[6]}}));
    b.set_column(1,host({{m[8],m[9],m[10]}}));b.set_column(2,host({{m[0],m[1],m[2]}}));
    return {b,host({{m[12],m[13],m[14]}})};
}
}
void Ps2DrivingRuntime::require_requests() const {if(!requests_enabled())throw std::logic_error("DrivingLab is disabled in Release builds");}
void Ps2DrivingRuntime::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("load_config","code"),&Ps2DrivingRuntime::load_config);
    godot::ClassDB::bind_method(godot::D_METHOD("action","name"),&Ps2DrivingRuntime::action);
    godot::ClassDB::bind_method(godot::D_METHOD("set_input","throttle","brake","steering"),&Ps2DrivingRuntime::set_input);
    godot::ClassDB::bind_method(godot::D_METHOD("set_driving_input","throttle","brake","steering","handbrake","transmission","shift","neutral"),&Ps2DrivingRuntime::set_driving_input,DEFVAL(-1),DEFVAL(0),DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("set_surface","mode","terrain"),&Ps2DrivingRuntime::set_surface);
    godot::ClassDB::bind_method(godot::D_METHOD("set_track","number"),&Ps2DrivingRuntime::set_track);
    godot::ClassDB::bind_method(godot::D_METHOD("tick","elapsed"),&Ps2DrivingRuntime::tick,DEFVAL(0.0));
    godot::ClassDB::bind_method(godot::D_METHOD("render_skids","camera","clip","H"),&Ps2DrivingRuntime::render_skids);
    godot::ClassDB::bind_method(godot::D_METHOD("get_state"),&Ps2DrivingRuntime::get_state);
}
godot::Dictionary Ps2DrivingRuntime::load_config(const godot::String& code){
    const auto error=catch_all([&]{
        require_requests();
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
        auto simulation=std::make_shared<port::driving::NativeBackend>(port::driving::debug_image_path(start.utf8().get_data()));
        backend_=simulation;code_=code.utf8().get_data();
        session_=port::driving::Session{std::move(simulation)};
        skid_textures_.instantiate();skid_materials_.clear();
        session_.skid_textures([this](const std::string& name,std::uint32_t hash){
            hp2::assets::TpkTextureHeader header;
            const auto image=skid_textures_->load_texture_for_3d(hash,header);
            return port::driving::SkidTexture{name,hash,image.is_valid(),header.is_any_semitransparency!=0,header.Field_0x4d!=0,header.RenderingOrder,header.Field_0x74!=0};
        });
        const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
        if(!root)throw std::runtime_error("Original ZZDATA root is not configured");
        port::assets::DirectorySource source(*root);
        session_.configure(port::driving::read_config(source.read("GLOBAL/GLOBALB.BUN"),code.utf8().get_data()));
    });
    if(!error.empty())session_.fail(error);return get_state();
}
godot::Dictionary Ps2DrivingRuntime::action(const godot::String& name){
    const auto error=catch_all([&]{
        require_requests();const auto command=std::string(name.utf8().get_data());
        if(command=="run")session_.run();else if(command=="pause")session_.pause();
        else if(command=="step")session_.single_step();else if(command=="restart")session_.reset();
        else if(command=="record")session_.recording(true);else if(command=="stop_record")session_.recording(false);
        else if(command=="replay")session_.replay();
        else throw std::invalid_argument("Unknown driving action: "+command);
    });
    auto result=get_state();if(!error.empty()){result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}
godot::Dictionary Ps2DrivingRuntime::set_input(double throttle,double brake,double steering){
    const auto error=catch_all([&]{require_requests();session_.input({float(throttle),float(brake),float(steering)});});
    auto result=get_state();if(!error.empty()){session_.pause();result=get_state();result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}
godot::Dictionary Ps2DrivingRuntime::set_driving_input(double throttle,double brake,double steering,double handbrake,int transmission,int shift,bool neutral){
    const auto error=catch_all([&]{require_requests();port::driving::Input input;
        input.throttle=float(throttle);input.brake=float(brake);input.steering=float(steering);
        input.handbrake=float(handbrake);input.transmission=transmission;input.shift=shift;input.neutral=neutral;session_.input(input);
    });
    auto result=get_state();if(!error.empty()){result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}

godot::Dictionary Ps2DrivingRuntime::set_surface(int mode,int terrain){
    const auto error=catch_all([&]{require_requests();session_.surface({static_cast<port::driving::SurfaceMode>(mode),terrain});});
    auto result=get_state();if(!error.empty()){result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}
godot::Dictionary Ps2DrivingRuntime::set_track(int number){
    const auto error=catch_all([&]{require_requests();
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
        const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
        if(!root)throw std::runtime_error("Original ZZDATA root is not configured");
        port::assets::DirectorySource source(*root);const auto catalogue=port::track::load_catalog(source);
        const auto entry=std::find_if(catalogue.begin(),catalogue.end(),[&](const auto& t){return t.number==number;});
        if(entry==catalogue.end())throw std::runtime_error("Unknown original track number");
        if(!backend_||!session_.configured())throw std::logic_error("Load an original vehicle before selecting a track");
        port::vehicle::Catalog vehicles(source);vehicles.load();
        const auto vehicle=std::find_if(vehicles.cars().begin(),vehicles.cars().end(),[&](const auto& c){return c.code==code_;});
        if(vehicle==vehicles.cars().end()||!vehicle->available)throw std::runtime_error("Original vehicle geometry is unavailable for track collision: "+code_);
        backend_->collision_geometry(source.read(vehicle->resource_path));
        session_.track(port::driving::load_track_environment(source,*entry));
    });
    if(!error.empty())session_.fail(error);auto result=get_state();
    if(!error.empty()){result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}

godot::Dictionary Ps2DrivingRuntime::tick(double elapsed){
    const auto error=catch_all([&]{require_requests();session_.tick(static_cast<float>(elapsed));});
    if(!error.empty())session_.fail(error);return get_state();
}
godot::Dictionary Ps2DrivingRuntime::render_skids(const godot::Transform3D& camera,const godot::Projection& clip,double H){
    godot::Dictionary result;godot::Array commands;
    const auto error=catch_all([&]{require_requests();
        port::driving::SkidView view;view.position={{float(camera.origin.z),float(camera.origin.x),float(camera.origin.y)}};view.H=float(H);
        // Port camera boundary: Godot Projection is symmetric clip-space; native
        // (x,y,z)->host(y,z,x). Original row-vector m[4*basis+component].
        constexpr int columns[]{2,0,1,3};
        for(int basis=0;basis<4;++basis)for(int component=0;component<4;++component)view.clip_matrix[basis*4+component]=float(clip.columns[columns[basis]][component]);
        const auto quads=session_.render_skids(view);
        for(const auto& quad:quads){
            godot::Dictionary command;godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedColorArray colours;
            for(unsigned i=0;i<4;++i){positions.push_back(host(quad.vertices[i]));uv.push_back({quad.uv[i][0],quad.uv[i][1]});
                const auto& c=quad.colours[i];colours.push_back({c[0]/255.f,c[1]/255.f,c[2]/255.f,c[3]/255.f});}
            command["vertices"]=positions;command["uv"]=uv;command["colours"]=colours;
            command["terrain"]=quad.terrain;command["texture_hash"]=int64_t(quad.texture_hash);
            command["texture_name"]=godot::String::utf8(quad.texture_name.c_str());
            hp2::assets::TpkTextureHeader header;
            const auto texture=skid_textures_->load_texture_for_3d(quad.texture_hash,header);
            if(texture.is_null())throw std::runtime_error(std::string("Original skid texture unavailable: ")+quad.texture_name+" / "+skid_textures_->get_last_error().utf8().get_data());
            const auto material_key=std::uint64_t(quad.texture_hash)|(std::uint64_t(quad.draw_context)<<32);
            auto material=skid_materials_.find(material_key);
            if(material==skid_materials_.end()){
                // E-0046-S01: Debug CreateTextureContext00108AE0 independently
                // confirms the raw metadata consumed by the shared host shader.
                if(header.texture_fx!=0 || header.alpha_fix!=0 || (header.alpha_bits!=0x0a&&header.alpha_bits!=0x44))
                    hp2::not_implemented("0x00108AE0","Skid texture TFX/ALPHA outside the recovered host copy/source-over slice");
                if(header.Field_0x6e!=0||header.Field_0x6f!=1||header.Field_0x70!=3||header.Field_0x71!=0||header.Field_0x72!=0)
                    hp2::not_implemented("0x00108AE0","Skid TEX1 outside the recovered MXL0/MMAG1/MMIN3 slice");
                const bool repeat_u=(header.Field_0x4c&1)!=0,repeat_v=(header.Field_0x4c&2)!=0;
                const bool blend=quad.semitransparent&&header.alpha_bits==0x44;
                if(blend&&godot::RenderingServer::get_singleton()->get_current_rendering_method()!="gl_compatibility")
                    hp2::not_implemented("0x00108AE0","Encoded skid GS blending requires Compatibility renderer");
                if(blend){const auto bytes=texture->get_image()->get_data();for(int64_t i=3;i<bytes.size();i+=4)
                    if(bytes[i]>128)hp2::not_implemented("0x00108AE0","Skid GS alpha above128 requires destination-read rendering");}
                // Debug AttachSolidPolys/AttachAlphaPolys select these states;
                // unlike Retail, Debug does not use TextureInfo.bWriteZ here.
                if((quad.alpha_test!=5&&quad.alpha_test!=6)||(quad.alpha_test==6&&quad.alpha_reference!=48)||(quad.alpha_test==5&&quad.alpha_reference!=0))
                    hp2::not_implemented("0x001058D8","Skid draw target alpha test outside the normal recovered target");
                std::string modes="fog_disabled, shadows_disabled, ";modes+=quad.write_depth?"depth_draw_always":"depth_draw_never";
                if(blend||quad.alpha_sort)modes+=", blend_mix";
                godot::Ref<godot::Shader> shader;shader.instantiate();
                auto fragment=car_fragment_source(false,int(quad.alpha_test),blend,quad.alpha_sort);
                // Independent GS WMS/WMT: minification uses nearest integer wrap
                // or clamp; magnification clamps only the requested axes to texel
                // centers while the host's repeat sampler handles the other axis.
                std::string nearest;
                nearest+=repeat_u?"coord.x=((coord.x%int(tex_size.x))+int(tex_size.x))%int(tex_size.x);\n":"coord.x=clamp(coord.x,0,int(tex_size.x)-1);\n";
                nearest+=repeat_v?"coord.y=((coord.y%int(tex_size.y))+int(tex_size.y))%int(tex_size.y);\n":"coord.y=clamp(coord.y,0,int(tex_size.y)-1);\n";
                nearest+="texel=texelFetch(base_texture,coord,0);";
                replace_shader_token(fragment,"texel=texelFetch(base_texture,clamp(coord,ivec2(0),ivec2(tex_size)-ivec2(1)),0);",nearest);
                std::string linear="vec2 sample_uv=UV;\n";
                if(!repeat_u)linear+="sample_uv.x=clamp(sample_uv.x,0.5/tex_size.x,1.0-0.5/tex_size.x);\n";
                if(!repeat_v)linear+="sample_uv.y=clamp(sample_uv.y,0.5/tex_size.y,1.0-0.5/tex_size.y);\n";
                linear+="texel=texture(base_texture,sample_uv);";
                replace_shader_token(fragment,"texel=texture(base_texture,UV);",linear);
                shader->set_code(godot::String::utf8(car_base_shader_source(modes,fragment,"filter_linear, repeat_enable").c_str()));
                godot::Ref<godot::ShaderMaterial> value;value.instantiate();value->set_shader(shader);value->set_shader_parameter("base_texture",texture);
                value->set_shader_parameter("base_lit",false);value->set_render_priority(quad.alpha_sort?2:0);
                material=skid_materials_.emplace(material_key,value).first;
            }
            command["material"]=material->second;
            command["texture"]=texture;
            command["semitransparent"]=quad.semitransparent;command["alpha_sort"]=quad.alpha_sort;
            commands.append(command);
        }
    });
    result["ok"]=error.empty();if(error.empty())result["commands"]=commands;else{session_.fail(error);result["error"]=godot::String::utf8(error.c_str());}return result;
}

godot::Dictionary Ps2DrivingRuntime::get_state()const{
    godot::Dictionary result;
    const auto error=catch_all([&]{
        const auto& s=session_.snapshot();const auto& i=session_.controls();
        result["real_track"]=!session_.track_label().empty();result["track_name"]=godot::String::utf8(session_.track_label().c_str());
        result["enabled"]=requests_enabled();result["configured"]=session_.configured();
        result["paused"]=session_.paused();result["status"]=godot::String::utf8(session_.status().c_str());
        result["diagnostic"]=godot::String::utf8(session_.diagnostic().c_str());
        result["step"]=int64_t(s.step);result["dt_known"]=s.dt.has_value();if(s.dt)result["dt"]=*s.dt;
        result["body_known"]=s.body.has_value();result["wheels_known"]=s.wheels.has_value();
        result["throttle"]=i.throttle;result["brake"]=i.brake;result["steering"]=i.steering;
        result["handbrake"]=i.handbrake;
        result["powertrain_known"]=s.powertrain.has_value();
        if(s.powertrain){const auto& p=*s.powertrain;godot::Dictionary powertrain;
            powertrain["rpm"]=p.rpm;powertrain["speed"]=p.speed;powertrain["forward_speed"]=p.forward_speed;
            powertrain["gear"]=p.gear;powertrain["manual_transmission"]=p.manual_transmission;powertrain["handbrake"]=p.handbrake;
            result["powertrain"]=powertrain;
        }
        result["recording"]=session_.is_recording();result["replaying"]=session_.is_replaying();
        result["recorded_steps"]=int64_t(session_.tape().size());result["replay_position"]=int64_t(session_.replay_position());
        result["surface_known"]=s.surface.has_value();
        if(s.surface){godot::Dictionary surface;surface["mode"]=int(s.surface->mode);surface["terrain"]=s.surface->terrain;result["surface"]=surface;}
        if(s.surface_catalog||!s.track_material_catalog.empty()){godot::Array catalog;
        const auto materials=!s.track_material_catalog.empty()?s.track_material_catalog:std::vector<port::driving::SurfaceMaterial>(s.surface_catalog->begin(),s.surface_catalog->end());
        for(const auto& material:materials){
            godot::Dictionary entry;entry["id"]=material.id;entry["name"]=godot::String::utf8(material.name.c_str());
            entry["static_friction"]=material.static_friction;entry["dynamic_friction"]=material.dynamic_friction;entry["rolling_friction"]=material.rolling_friction;
            entry["noise_spacing_inverse"]=material.noise_spacing_inverse;entry["noise_scale"]=material.noise_scale;catalog.append(entry);
        }result["surface_catalog"]=catalog;}
        godot::Array boundaries,ids,colors;
        for(float boundary:port::driving::surface_boundaries)boundaries.append(boundary);
        for(int id:port::driving::surface_terrain_ids)ids.append(id);
        for(const auto& color:port::driving::surface_colors)colors.append(godot::Color(color[0],color[1],color[2],1));
        result["surface_boundaries"]=boundaries;result["surface_terrain_ids"]=ids;result["surface_colors"]=colors;
        if(s.body)result["body"]=host_matrix(*s.body);
        if(s.velocity)result["velocity"]=host(*s.velocity);
        if(s.angular_velocity)result["angular_velocity"]=host(*s.angular_velocity);
        if(s.wheels){godot::Array wheels;for(const auto& wheel:*s.wheels){
            godot::Dictionary value;value["tire"]=host_matrix(wheel.tire);value["brake"]=host_matrix(wheel.brake);value["blur"]=host_matrix(wheel.blur);
            value["phase"]=wheel.phase;value["angular_velocity"]=wheel.angular_velocity;value["blur_alpha"]=wheel.blur_alpha;
            if(wheel.contact)value["contact"]=*wheel.contact;
            if(wheel.grip){const auto& g=*wheel.grip;godot::Dictionary grip;grip["ground_point"]=host(g.ground_point);grip["ground_plane"]=godot::Vector4(g.ground_plane[0],g.ground_plane[1],g.ground_plane[2],g.ground_plane[3]);grip["terrain"]=g.terrain;grip["slipping"]=g.slipping;
                grip["z_force"]=g.z_force;grip["current_slip"]=g.current_slip;grip["traction_usage"]=g.traction_usage;grip["traction_boost"]=g.traction_boost;
                grip["catchup_friction"]=g.catchup_friction;grip["road_noise"]=g.road_noise;grip["slip"]=godot::Vector2(g.slip[0],g.slip[1]);
                grip["ground_force"]=godot::Vector2(g.ground_force[0],g.ground_force[1]);grip["static_coefficient"]=g.static_coefficient;grip["dynamic_coefficient"]=g.dynamic_coefficient;value["grip"]=grip;
            }wheels.append(value);
        }result["wheels"]=wheels;}
        result["driving_available"]=s.body.has_value()&&s.wheels.has_value()&&s.dt.has_value();result["comparison"]= "Not verified";
    });
    result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;
}
}

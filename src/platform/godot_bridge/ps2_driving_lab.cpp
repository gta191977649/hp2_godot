#include "ps2_driving_lab.hpp"
#include "boundary.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/port/driving/native_backend.hpp"
#include "port/assets/catalog.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
void require_debug(){if(!HP2_DEBUG_BUILD)throw std::logic_error("DrivingLab is disabled in Release builds");}
godot::Vector3 host(const std::array<float,3>& v){return {v[1],v[2],v[0]};}
godot::Transform3D host_matrix(const port::driving::Matrix& m){
    godot::Basis b;b.set_column(0,host({{m[4],m[5],m[6]}}));
    b.set_column(1,host({{m[8],m[9],m[10]}}));b.set_column(2,host({{m[0],m[1],m[2]}}));
    return {b,host({{m[12],m[13],m[14]}})};
}
}
void Ps2DrivingLab::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("load_config","code"),&Ps2DrivingLab::load_config);
    godot::ClassDB::bind_method(godot::D_METHOD("action","name"),&Ps2DrivingLab::action);
    godot::ClassDB::bind_method(godot::D_METHOD("set_input","throttle","brake","steering"),&Ps2DrivingLab::set_input);
    godot::ClassDB::bind_method(godot::D_METHOD("tick","elapsed"),&Ps2DrivingLab::tick,DEFVAL(0.0));
    godot::ClassDB::bind_method(godot::D_METHOD("get_state"),&Ps2DrivingLab::get_state);
}
godot::Dictionary Ps2DrivingLab::load_config(const godot::String& code){
    const auto error=catch_all([&]{
        require_debug();
        const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");
        auto simulation=std::make_shared<port::driving::NativeBackend>(port::driving::original_image_path(start.utf8().get_data()));
        session_=port::driving::Session{std::move(simulation)};
        const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());
        if(!root)throw std::runtime_error("Original ZZDATA root is not configured");
        port::assets::DirectorySource source(*root);
        session_.configure(port::driving::read_config(source.read("GLOBAL/GLOBALB.BUN"),code.utf8().get_data()));
    });
    if(!error.empty())session_.fail(error);return get_state();
}
godot::Dictionary Ps2DrivingLab::action(const godot::String& name){
    const auto error=catch_all([&]{
        require_debug();const auto command=std::string(name.utf8().get_data());
        if(command=="run")session_.run();else if(command=="pause")session_.pause();
        else if(command=="step")session_.single_step();else if(command=="restart")session_.reset();
        else if(command=="record")session_.recording(true);else if(command=="stop_record")session_.recording(false);
        else if(command=="replay")session_.replay();
        else throw std::invalid_argument("Unknown driving action: "+command);
    });
    auto result=get_state();if(!error.empty()){result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}
godot::Dictionary Ps2DrivingLab::set_input(double throttle,double brake,double steering){
    const auto error=catch_all([&]{require_debug();session_.input({float(throttle),float(brake),float(steering)});});
    auto result=get_state();if(!error.empty()){session_.pause();result=get_state();result["ok"]=false;result["error"]=godot::String::utf8(error.c_str());}return result;
}
godot::Dictionary Ps2DrivingLab::tick(double elapsed){
    const auto error=catch_all([&]{require_debug();session_.tick(static_cast<float>(elapsed));});
    if(!error.empty())session_.fail(error);return get_state();
}
godot::Dictionary Ps2DrivingLab::get_state()const{
    godot::Dictionary result;
    const auto error=catch_all([&]{
        const auto& s=session_.snapshot();const auto& i=session_.controls();
        result["enabled"]=bool(HP2_DEBUG_BUILD);result["configured"]=session_.configured();
        result["paused"]=session_.paused();result["status"]=godot::String::utf8(session_.status().c_str());
        result["diagnostic"]=godot::String::utf8(session_.diagnostic().c_str());
        result["step"]=int64_t(s.step);result["dt_known"]=s.dt.has_value();if(s.dt)result["dt"]=*s.dt;
        result["body_known"]=s.body.has_value();result["wheels_known"]=s.wheels.has_value();
        result["throttle"]=i.throttle;result["brake"]=i.brake;result["steering"]=i.steering;
        result["recording"]=session_.is_recording();result["replaying"]=session_.is_replaying();
        result["recorded_steps"]=int64_t(session_.tape().size());result["replay_position"]=int64_t(session_.replay_position());
        if(s.body)result["body"]=host_matrix(*s.body);
        if(s.velocity)result["velocity"]=host(*s.velocity);
        if(s.angular_velocity)result["angular_velocity"]=host(*s.angular_velocity);
        if(s.wheels){godot::Array wheels;for(const auto& wheel:*s.wheels){
            godot::Dictionary value;value["tire"]=host_matrix(wheel.tire);value["brake"]=host_matrix(wheel.brake);value["blur"]=host_matrix(wheel.blur);
            value["phase"]=wheel.phase;value["angular_velocity"]=wheel.angular_velocity;value["blur_alpha"]=wheel.blur_alpha;
            if(wheel.contact)value["contact"]=*wheel.contact;wheels.append(value);
        }result["wheels"]=wheels;}
        result["driving_available"]=s.body.has_value()&&s.wheels.has_value()&&s.dt.has_value();result["comparison"]= "Not verified";
    });
    result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;
}
}

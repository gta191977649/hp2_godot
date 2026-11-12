#include "event_settings_transport.hpp"
#include "event_reward_transport.hpp"
#include <cmath>
#include "event_score_transport.hpp"
#include "event_profile_transport.hpp"
#include "port/runtime/event_attempt.hpp"
#include "game_core/port/race/route_observation.hpp"
// TASK-0107. Port transport for one actual recovered World, never a per-car backend.
#include "ps2_driving_runtime.hpp"
#include "boundary.hpp"
#include "championship_timing.hpp"
#include "platform/ps2_asset_root.hpp"
#include "platform/shaders/car_base_shader.hpp"
#include "port/assets/catalog.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "game_core/port/track/particles.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace port::godot_bridge {
struct WorldLoadJob {
 std::mutex mutex;
 std::atomic<bool> cancelled{false};
 bool done=false;
 port::race::WorldLaunchRequest request;
 std::unique_ptr<port::race::WorldSession> result;
 std::string error;
};
namespace {
// Port identity survives replacement of the RefCounted bridge on UI restart.
std::atomic<std::uint64_t> next_world_epoch{1};
// One bootstrap worker serializes source construction. It never touches Godot.
class WorldLoadQueue {
 std::mutex mutex_;std::condition_variable changed_;
 std::deque<std::shared_ptr<WorldLoadJob>> queue_;
 std::thread worker_;bool stopping_=false;
 void work(){
  for(;;){std::shared_ptr<WorldLoadJob> job;
   {std::unique_lock<std::mutex> lock(mutex_);changed_.wait(lock,[&]{return stopping_||!queue_.empty();});if(stopping_&&queue_.empty())return;job=queue_.front();queue_.pop_front();}
   if(job->cancelled)continue;
   std::unique_ptr<port::race::WorldSession> candidate;std::string error;
   try{error=hp2::guard([&]{candidate=std::make_unique<port::race::WorldSession>();candidate->open(job->request);});}
   catch(const std::exception& e){error=e.what();}catch(...){error="Unexpected native World construction failure";}
   if(job->cancelled)continue;
   std::lock_guard<std::mutex> lock(job->mutex);job->result=std::move(candidate);job->error=std::move(error);job->done=true;
  }
 }
public:
 WorldLoadQueue():worker_([this]{work();}){}
 ~WorldLoadQueue(){shutdown();}
 void submit(std::shared_ptr<WorldLoadJob> job){std::lock_guard<std::mutex> lock(mutex_);if(stopping_)throw std::logic_error("World loader is shutting down");queue_.push_back(std::move(job));changed_.notify_one();}
 void shutdown(){ {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;for(auto& job:queue_)job->cancelled=true;queue_.clear();changed_.notify_all();}if(worker_.joinable())worker_.join();}
};
WorldLoadQueue& loader(){static WorldLoadQueue queue;return queue;}
godot::String text(const std::string& value){return godot::String::utf8(value.c_str());}
template<class T>void optional_field(godot::Dictionary& row,const char* key,const std::optional<T>& value){if(value)row[key]=*value;}
godot::Dictionary rule_state(const port::race::RaceRulesView& source){
 godot::Dictionary out,hud;optional_field(out,"tree_type",source.tree_type);optional_field(out,"event_type",source.event_type);optional_field(out,"event_number",source.event_number);
 out["race_type"]=source.race_type;out["nfs_race_mode"]=source.nfs_race_mode;out["challenge_race_mode"]=source.challenge_race_mode;out["selected_language"]=source.selected_language;out["loaded_language"]=source.loaded_language;out["event_name"]=text(source.event_name);out["objective_text"]=text(source.objective_text);
 optional_field(out,"objective_type",source.objective_type);optional_field(out,"objective_data",source.objective_data);
 optional_field(out,"event_status",source.event_status);optional_field(out,"old_event_status",source.old_event_status);optional_field(out,"aborted",source.aborted);
 optional_field(out,"stage_index",source.stage_index);optional_field(out,"stage_count",source.stage_count);optional_field(out,"has_next_race",source.has_next_race);optional_field(out,"result_should_be_discarded",source.result_should_be_discarded);
 const auto& s=source.hud;hud["race_stage"]=s.race_stage;hud["countdown_number"]=s.countdown_number;hud["countdown_progress"]=s.countdown_progress;
 hud["rank"]=s.rank;hud["participant_count"]=s.participant_count;hud["finish_reason"]=s.finish_reason;hud["completed_laps"]=s.completed_laps;hud["lap_limit"]=s.lap_limit;hud["display_lap"]=s.display_lap;hud["point_to_point"]=s.point_to_point;hud["progress_percent"]=s.progress_percent;hud["score"]=s.score;
 hud["race_elapsed_ticks"]=s.race_elapsed_ticks;hud["sample_world_ticks"]=s.sample_world_ticks;hud["clock_display_ticks"]=s.clock_display_ticks;hud["clock_target_applied"]=s.clock_target_applied;hud["clock_overrun"]=s.clock_overrun;hud["clock_text"]=text(s.clock_text);hud["race_elapsed_text"]=text(s.race_elapsed_text);
 optional_field(hud,"lap_elapsed_ticks",s.lap_elapsed_ticks);optional_field(hud,"target_ticks",s.target_ticks);optional_field(hud,"clock_signed_delta_ticks",s.clock_signed_delta_ticks);optional_field(hud,"hud_features",s.hud_features);out["hud"]=hud;
 if(source.police){const auto& p=*source.police;godot::Dictionary police;police["being_pursued"]=p.being_pursued;police["be_a_cop"]=p.be_a_cop;police["hud_level"]=p.hud_level;police["tickets"]=p.tickets;police["ticket_limit"]=p.ticket_limit;optional_field(police,"pursuit_state",p.pursuit_state);optional_field(police,"pursuit_substate",p.pursuit_substate);optional_field(police,"be_a_cop_seconds",p.be_a_cop_seconds);optional_field(police,"arrests",p.arrests);optional_field(police,"arrest_quota",p.arrest_quota);optional_field(police,"roadblocks_used",p.roadblocks_used);optional_field(police,"choppers_used",p.choppers_used);out["police"]=police;}
 godot::Array standings;for(const auto& s:source.standings){godot::Dictionary row;row["racer_index"]=s.racer_index;row["driver_number"]=s.driver_number;optional_field(row,"points",s.points);optional_field(row,"standing_rank",s.standing_rank);optional_field(row,"recorded_rank",s.recorded_rank);standings.append(row);}out["standings"]=standings;return out;
}
godot::Vector3 host(const std::array<float,3>& v){return {v[1],v[2],v[0]};}
godot::Transform3D host_matrix(const port::driving::Matrix& m){godot::Basis b;b.set_column(0,host({{m[4],m[5],m[6]}}));b.set_column(1,host({{m[8],m[9],m[10]}}));b.set_column(2,host({{m[0],m[1],m[2]}}));return {b,host({{m[12],m[13],m[14]}})};}
godot::Dictionary actor_state(const port::race::WorldActorFrame& actor){
 godot::Dictionary d;const auto& s=actor.state;
 d["visible"]=actor.visible;
 d["address"]=int64_t(actor.address);d["driver_number"]=actor.driver_number;d["car_index"]=actor.car_index;d["car_id"]=actor.car_id;d["skin_id"]=actor.skin_id;d["car_code"]=text(actor.car_code);d["step"]=int64_t(s.step);
 d["body_known"]=s.body.has_value();d["wheels_known"]=s.wheels.has_value();d["dt_known"]=s.dt.has_value();if(s.dt)d["dt"]=*s.dt;
 if(s.body)d["body"]=host_matrix(*s.body);if(s.velocity)d["velocity"]=host(*s.velocity);if(s.angular_velocity)d["angular_velocity"]=host(*s.angular_velocity);
 d["powertrain_known"]=s.powertrain.has_value();if(s.powertrain){const auto& p=*s.powertrain;godot::Dictionary v;v["rpm"]=p.rpm;v["speed"]=p.speed;v["forward_speed"]=p.forward_speed;v["gear"]=p.gear;v["manual_transmission"]=p.manual_transmission;v["handbrake"]=p.handbrake;v["horn"]=p.horn;d["powertrain"]=v;}
 if(s.lights){godot::Dictionary v;v["headlight_left"]=s.lights->headlight_left;v["headlight_right"]=s.lights->headlight_right;v["brakelights"]=s.lights->brakelights;d["lights"]=v;}
 if(s.flares){godot::PackedFloat32Array a;for(float v:s.flares->intensity)a.push_back(v);d["flare_intensities"]=a;}
 if(s.wipers){godot::Dictionary v;v["state"]=s.wipers->state;v["on_time"]=s.wipers->on_time;v["off_time"]=s.wipers->off_time;v["world_frame"]=int64_t(s.wipers->world_frame);d["wipers"]=v;}
 if(s.wheels){godot::Array a;for(const auto& w:*s.wheels){godot::Dictionary v;v["tire"]=host_matrix(w.tire);v["brake"]=host_matrix(w.brake);v["blur"]=host_matrix(w.blur);v["phase"]=w.phase;v["angular_velocity"]=w.angular_velocity;v["blur_alpha"]=w.blur_alpha;if(w.contact)v["contact"]=*w.contact;a.append(v);}d["wheels"]=a;}
 d["wheel_blur_policy"]="Port debug opacity from source angular speed; original wheel Render not recovered";d["driving_available"]=s.body.has_value()&&s.wheels.has_value();return d;
}
}
Ps2DrivingRuntime::Ps2DrivingRuntime()=default;
Ps2DrivingRuntime::~Ps2DrivingRuntime(){release_world();}
void Ps2DrivingRuntime::shutdown_loader(){loader().shutdown();}
void Ps2DrivingRuntime::require_requests()const{if(!requests_enabled())throw std::logic_error("Debug driving requests are disabled in this build");}
void Ps2DrivingRuntime::release_world(){if(loading_)loading_->cancelled=true;loading_.reset();if(world_){const auto failure=catch_all([&]{world_->close();});last_stats_=world_->stats();last_cleanup_diagnostic_=failure.empty()?world_->diagnostic():failure;if(!failure.empty())error_=failure;}world_.reset();presented_.reset();audio_step_=0;}
void Ps2DrivingRuntime::begin_load(){release_world();generation_=next_world_epoch.fetch_add(1);error_.clear();phase_="Loading";request_.project_root=godot::ProjectSettings::get_singleton()->globalize_path("res://").utf8().get_data();
 loading_=std::make_shared<WorldLoadJob>();loading_->request=request_;loader().submit(loading_);
 if(skid_textures_.is_null())skid_textures_.instantiate();skid_materials_.clear();
}
void Ps2DrivingRuntime::poll_load(){if(!loading_)return;std::unique_lock<std::mutex> lock(loading_->mutex);if(!loading_->done)return;world_=std::move(loading_->result);error_=loading_->error;lock.unlock();loading_.reset();if(!error_.empty()){phase_="Blocked";return;}if(!world_){phase_="Failed";error_="World loader completed without a session";return;}phase_.clear();request_=world_->request();}
void Ps2DrivingRuntime::_bind_methods(){
 using namespace godot;
 ClassDB::bind_method(D_METHOD("configure_world","car_id","skin_id","track","reverse","car_code"),&Ps2DrivingRuntime::configure_world,DEFVAL(false),DEFVAL(String()));
 ClassDB::bind_method(D_METHOD("configure_event_world","request"),&Ps2DrivingRuntime::configure_event_world);
 ClassDB::bind_method(D_METHOD("load_config","code"),&Ps2DrivingRuntime::load_config);
 ClassDB::bind_method(D_METHOD("set_track","number"),&Ps2DrivingRuntime::set_track);
 ClassDB::bind_method(D_METHOD("action","name"),&Ps2DrivingRuntime::action);
 ClassDB::bind_method(D_METHOD("set_input","throttle","brake","steering"),&Ps2DrivingRuntime::set_input);
 ClassDB::bind_method(D_METHOD("set_driving_input","throttle","brake","steering","handbrake","transmission","shift","neutral","horn"),&Ps2DrivingRuntime::set_driving_input,DEFVAL(-1),DEFVAL(0),DEFVAL(false),DEFVAL(false));
 ClassDB::bind_method(D_METHOD("tick","elapsed"),&Ps2DrivingRuntime::tick,DEFVAL(0.0));
 ClassDB::bind_method(D_METHOD("get_state"),&Ps2DrivingRuntime::get_state);ClassDB::bind_method(D_METHOD("get_world_state"),&Ps2DrivingRuntime::get_world_state);
 ClassDB::bind_method(D_METHOD("acknowledge_event_attempt","baseline"),&Ps2DrivingRuntime::acknowledge_event_attempt);
 ClassDB::bind_method(D_METHOD("tick_ui","elapsed"),&Ps2DrivingRuntime::tick_ui);
 ClassDB::bind_method(D_METHOD("menu_input","event","pressed","port"),&Ps2DrivingRuntime::menu_input,DEFVAL(0));
 ClassDB::bind_method(D_METHOD("get_world_route"),&Ps2DrivingRuntime::get_world_route);
 ClassDB::bind_method(D_METHOD("get_world_road_course"),&Ps2DrivingRuntime::get_world_road_course);
 ClassDB::bind_method(D_METHOD("get_world_road_path"),&Ps2DrivingRuntime::get_world_road_path);
 ClassDB::bind_method(D_METHOD("take_world_pcm"),&Ps2DrivingRuntime::take_world_pcm);
 ClassDB::bind_method(D_METHOD("commit_presentation","step","ui_revision"),&Ps2DrivingRuntime::commit_presentation,DEFVAL(-1));
 ClassDB::bind_method(D_METHOD("report_presentation_failure","reason"),&Ps2DrivingRuntime::report_presentation_failure);
 ClassDB::bind_method(D_METHOD("set_surface","mode","terrain"),&Ps2DrivingRuntime::set_surface);
 ClassDB::bind_method(D_METHOD("update_driving_camera","elapsed","look_back"),&Ps2DrivingRuntime::update_driving_camera,DEFVAL(false));ClassDB::bind_method(D_METHOD("cycle_driving_camera_pov"),&Ps2DrivingRuntime::cycle_driving_camera_pov);
 ClassDB::bind_method(D_METHOD("get_driving_audio_bank"),&Ps2DrivingRuntime::get_driving_audio_bank);ClassDB::bind_method(D_METHOD("get_driving_audio_state"),&Ps2DrivingRuntime::get_driving_audio_state);
 ClassDB::bind_method(D_METHOD("take_collision_audio_events"),&Ps2DrivingRuntime::take_collision_audio_events);ClassDB::bind_method(D_METHOD("take_splash_audio_events"),&Ps2DrivingRuntime::take_splash_audio_events);
 ClassDB::bind_method(D_METHOD("render_skids","camera","clip","H"),&Ps2DrivingRuntime::render_skids);ClassDB::bind_method(D_METHOD("present_particles","owner","camera"),&Ps2DrivingRuntime::present_particles);ClassDB::bind_method(D_METHOD("clear_particles"),&Ps2DrivingRuntime::clear_particles);
}
godot::Dictionary Ps2DrivingRuntime::configure_world(int car,int skin,int track,bool reverse,const godot::String& code){
 const auto error=catch_all([&]{require_requests();release_world();request_={};request_.car_id=car;request_.skin_id=skin;request_.track=track;request_.reverse=reverse;request_.car_code=code.utf8().get_data();
 port::race::WorldResearchOperands policy;policy.construction_w=0;policy.physics_w=0;policy.ai_w=0;for(auto& lane:policy.racer_start_w)lane=0;policy.event_disc_w=0;policy.terrain_disc_w=0;policy.camera_shake_w=0;policy.acid_w=0;policy.negative_override_route=0;policy.negative_override_point=0;policy.cop_teleport_position_w=0;policy.car_reset_route_newpos_w=0;policy.car_reset_velocity_w=0;policy.cop_speedtrap_velocity_w=0;policy.cop_mover_ground_normal_w=0;policy.cop_mover_invalid_ground_normal_w=0;policy.cop_mover_airborne_angular_w=0;policy.mellow_jump_up_w=0;policy.pursuit_add_point_w=0;policy.pursuit_set_point_w=0;policy.average_air_position_w=0;policy.average_air_acceleration_w=0;policy.cop_desired_velocity_w=0;policy.cop_target_point_w=0;policy.cop_bump_arm_w=0;policy.cop_roadblock_velocity_w=0;policy.nis_placement_route_position_w=0;request_.research_operands=policy;
 request_.minimal_driving=true; // TASK-0111 explicit default host preset.
 const auto args=godot::OS::get_singleton()->get_cmdline_user_args();
 for(int i=0;i<args.size();++i)if(args[i]=="--world-strict")request_.minimal_driving=false;for(int i=0;i<args.size();++i)if(args[i].begins_with("--world-publication-test-fault-after=")){const auto value=args[i].trim_prefix("--world-publication-test-fault-after=").to_int();if(value<=0)throw std::invalid_argument("Port publication fault step must be positive");request_.publication_test_fault_after=std::uint64_t(value);}
 if(track<=0||car<0&&request_.car_code.empty())throw std::invalid_argument("Select an original car and track");request_ready_=true;phase_="Configured";error_.clear();});
 if(!error.empty()){error_=error;phase_="Failed";}return get_state();
}
godot::Dictionary Ps2DrivingRuntime::load_config(const godot::String& code){return configure_world(-1,-1,request_.track>0?request_.track:11,request_.reverse,code);}
godot::Dictionary Ps2DrivingRuntime::configure_event_world(const godot::Dictionary& input){
 auto result=configure_world(input.get("car_id",-1),input.get("skin_id",-1),input.get("track",-1),input.get("reverse",false));
 if(!bool(result.get("ok",false)))return result;
 const auto error=catch_all([&]{port::race::EventLaunchRequest event;
  const auto generation=int64_t(input.get("generation",0));if(generation<=0)throw std::invalid_argument("Production event launch identity is absent");event.generation=std::uint64_t(generation);
  event.tree_type=input.get("tree_type",-1);event.event_index=input.get("event_index",-1);event.event_number=input.get("event_number",-1);
  event.car_id=request_.car_id;event.skin_id=request_.skin_id;event.track=request_.track;event.reverse=request_.reverse;
  event.language=input.get("language",0);event.driving_style=input.get("driving_style",0);event.transmission=input.get("transmission",0);event.preferred_gender=input.get("preferred_gender",0);
  event.has_dash=input.get("has_dash",false);event.vertex_damage=input.get("vertex_damage",false);event.player_points=int64_t(input.get("player_points",0));
  if(!input.has("random_state"))throw std::invalid_argument("Production event launch requires its original RNG state");event.random_state=int64_t(input["random_state"]);
  const godot::PackedByteArray progress=input.get("event_progress",godot::PackedByteArray());if(progress.size()!=256)throw std::invalid_argument("Production event launch requires selected-tree progress");event.event_progress.assign(progress.ptr(),progress.ptr()+progress.size());
  const godot::PackedInt32Array offsets=input.get("event_progress_offsets",godot::PackedInt32Array());if(offsets.size()!=32)throw std::invalid_argument("Production event launch requires per-version state offsets");for(int i=0;i<offsets.size();++i)event.event_progress_offsets.push_back(offsets[i]);
  if(input.has("event_profile"))event.event_profile=profile_transfer(godot::Dictionary(input["event_profile"]));
  if(input.has("prepared_event"))event.prepared_event=prepared_transfer(godot::Dictionary(input["prepared_event"]));
  if(input.has("event_scores"))event.event_scores=score_transfer(godot::Dictionary(input["event_scores"]));
  if(input.has("event_settings"))event.event_settings=settings_transfer(godot::Dictionary(input["event_settings"]));
  if(input.has("event_rewards"))event.event_rewards=reward_transfer(godot::PackedByteArray(input["event_rewards"]));
  request_.event=std::move(event);request_.original_ui=true;
  if(input.has("hud_camera")){const godot::Dictionary camera=input["hud_camera"];port::race::PlayerHudCameraContext context;context.selected_pov_available=camera.get("selected_pov_available",false);context.outside_pov=camera.get("outside_pov",false);context.selected_pov_kind=camera.get("selected_pov_kind",0);request_.hud_camera=context;}
 });if(!error.empty()){phase_="Failed";error_=error;request_ready_=false;}return get_state();
}
godot::Dictionary Ps2DrivingRuntime::set_track(int track){const auto error=catch_all([&]{require_requests();if(world_||loading_)throw std::logic_error("Change track before opening the World");if(track<=0)throw std::invalid_argument("Invalid original track number");request_.track=track;});auto d=get_state();if(!error.empty()){d["ok"]=false;d["error"]=text(error);}return d;}
godot::Dictionary Ps2DrivingRuntime::action(const godot::String& name){
 const auto error=catch_all([&]{require_requests();poll_load();const std::string action=name.utf8().get_data();
 if(action=="close"){release_world();phase_="Empty";error_.clear();return;}
 if(action=="restart"){if(!request_ready_)throw std::logic_error("World launch selection is absent");begin_load();return;}
 if(action=="run"||action=="resume"){if(phase_=="Blocked"||phase_=="Failed")throw std::logic_error("A blocked World requires restart or close");if(!world_&&!loading_){if(!request_ready_)throw std::logic_error("Select car/track before opening World");begin_load();}else if(world_)world_->resume();return;}
 if(action=="acknowledge_autosave"){if(world_)world_->acknowledge_autosave();return;}
 if(action=="open_pause"){if(world_)world_->open_pause();return;}
 if(action=="pause"){if(world_)world_->pause();return;}
 if(action=="step"){if(world_&&phase_!="Blocked"&&phase_!="Failed")world_->single_step();return;}
 throw std::invalid_argument("Unsupported real World action: "+action);
 });auto d=get_state();if(!error.empty()){d["ok"]=false;d["error"]=text(error);}return d;
}
godot::Dictionary Ps2DrivingRuntime::set_input(double t,double b,double s){return set_driving_input(t,b,s,0);}
godot::Dictionary Ps2DrivingRuntime::set_driving_input(double t,double b,double s,double h,int tr,int sh,bool n,bool horn){
 const auto error=catch_all([&]{require_requests();if(world_&&phase_!="Blocked"&&phase_!="Failed")world_->set_input({float(t),float(b),float(s),float(h),tr,sh,n,horn});});auto d=get_state();if(!error.empty()){d["ok"]=false;d["error"]=text(error);}return d;
}
godot::Dictionary Ps2DrivingRuntime::tick(double elapsed) {
    double advance_ms = 0;
    double bridge_result_ms = 0;
    port::race::WorldAdvanceTiming world_timing;
    const bool profile = championship_timing_enabled();
    const auto error = catch_all([&] {
        require_requests();
        poll_load();
        if (world_ && phase_ != "Blocked" && phase_ != "Failed") {
            if (!std::isfinite(elapsed) || elapsed < 0)
                throw std::invalid_argument("Invalid World elapsed time");
            ChampionshipTimingScope timing(advance_ms);
            world_->advance(float(elapsed), profile ? &world_timing : nullptr);
        }
    });
    if (!error.empty()) {
        error_ = error;
        phase_ = "Failed";
        if (world_) world_->pause();
    }
    godot::Dictionary result;
    {
        ChampionshipTimingScope timing(bridge_result_ms);
        result = get_state();
    }
    if (profile) {
        result["profile_world_advance_ms"] = advance_ms;
        result["profile_world_bridge_result_ms"] = bridge_result_ms;
        result["profile_world_replay_ms"] = world_timing.replay_ms;
        result["profile_world_timestep_ms"] = world_timing.timestep_ms;
        result["profile_world_publish_ms"] = world_timing.publish_ms;
    }
    return result;
}
godot::Dictionary Ps2DrivingRuntime::tick_ui(double elapsed){const auto error=catch_all([&]{require_requests();if(world_&&phase_!="Failed"&&phase_!="Blocked")world_->advance_ui(float(elapsed));});if(!error.empty()){error_=error;phase_="Failed";if(world_)world_->pause();}return get_state();}
godot::Dictionary Ps2DrivingRuntime::menu_input(int event,bool pressed,int port){const auto error=catch_all([&]{require_requests();if(world_&&phase_!="Failed"&&phase_!="Blocked")world_->menu_input(event,pressed,port);});if(!error.empty()){error_=error;phase_="Failed";if(world_)world_->pause();}return get_state();}
godot::Dictionary Ps2DrivingRuntime::acknowledge_event_attempt(const godot::Dictionary& input){
 const auto error=catch_all([&]{require_requests();if(!world_||loading_)throw std::logic_error("No native event continuation is available");if(!bool(input.get("storage_baseline_only",false))||input.has("prepared_event"))throw std::invalid_argument("Continuation requires a storage-only seal");
  ::port::runtime::EventAttemptBaseline baseline;
  const auto previous=int64_t(input.get("previous_generation",0)),generation=int64_t(input.get("generation",0));if(previous<=0||generation<=previous)throw std::invalid_argument("Continuation generation is invalid");baseline.previous_generation=std::uint64_t(previous);baseline.generation=std::uint64_t(generation);baseline.tree_type=input.get("tree_type",-1);baseline.event_index=input.get("event_index",-1);baseline.event_number=input.get("event_number",-1);baseline.player_points=int64_t(input.get("player_points",0));baseline.random_state=int64_t(input.get("random_state",0));
  const godot::PackedByteArray progress=input.get("event_progress",godot::PackedByteArray());if(progress.size()!=256)throw std::invalid_argument("Continuation requires the sealed progress block");baseline.event_progress.assign(progress.ptr(),progress.ptr()+progress.size());
  if(input.has("event_profile"))baseline.event_profile=profile_transfer(godot::Dictionary(input["event_profile"]));if(input.has("event_scores"))baseline.event_scores=score_transfer(godot::Dictionary(input["event_scores"]));if(input.has("event_settings"))baseline.event_settings=settings_transfer(godot::Dictionary(input["event_settings"]));
  if(input.has("event_rewards"))baseline.event_rewards=reward_transfer(godot::PackedByteArray(input["event_rewards"]));
  world_->acknowledge_event_attempt(baseline);request_=world_->request();if(auto reload=world_->take_continuation_launch()){request_=std::move(*reload);request_ready_=true;begin_load();}
 });auto result=get_state();if(!error.empty()){result["ok"]=false;result["error"]=text(error);}return result;
}
godot::Dictionary Ps2DrivingRuntime::get_state()const{
 auto* self=const_cast<Ps2DrivingRuntime*>(this);self->poll_load();godot::Dictionary d;
 const auto error=catch_all([&]{if(world_&&world_->player())d=actor_state(*world_->player());const auto phase=phase_.empty()&&world_?world_->phase():phase_;const auto reason=!error_.empty()?error_:world_?world_->diagnostic():std::string();
 d["phase"]=text(phase);d["status"]=text(phase);d["diagnostic"]=text(reason);d["backend"]="source_world";d["generation"]=int64_t(generation_);d["configured"]=request_ready_;d["world_ready"]=world_&&world_->configured();d["paused"]=!world_||world_->paused()||phase=="Blocked"||phase=="Failed";d["real_track"]=world_&&world_->configured();d["track_id"]=request_.track;d["track_name"]=text("Track "+std::to_string(request_.track));d["car_id"]=request_.car_id;d["skin_id"]=request_.skin_id;d["loading"]=bool(loading_);d["comparison"]="Not verified";
 if(world_){d["step"]=int64_t(world_->frame().step);const auto& in=world_->controls();d["throttle"]=in.throttle;d["brake"]=in.brake;d["steering"]=in.steering;d["handbrake"]=in.handbrake;d["horn"]=in.horn;}else d["step"]=int64_t(0);
 d["runtime_mode"]=request_.event?"authored_event":request_.minimal_driving?"minimal_driving":"strict";
 d["authored_event"]=request_.event.has_value();d["source_ui_enabled"]=request_.original_ui;
 d["skid_trails_enabled"]=!request_.minimal_driving;
 d["ok"]=phase!="Failed"&&phase!="Blocked";if(!reason.empty())d["error"]=text(reason);
 });if(!error.empty()){d["ok"]=false;d["error"]=text(error);}return d;
}
godot::Dictionary Ps2DrivingRuntime::get_world_state()const{
 auto d=get_state();godot::Array actors,objects;const auto error=catch_all([&]{godot::Array warnings;
 for(const auto& warning:port::diagnostics::warnings_snapshot()){godot::Dictionary row;row["address"]=text(warning.address);row["reason"]=text(warning.reason);row["action"]=text(warning.action);row["count"]=int64_t(warning.count);warnings.append(row);}
 d["runtime_warnings"]=warnings;
 godot::Dictionary event_result;event_result["ready"]=false;
 if(world_)if(const auto settled=world_->event_settlement()){
  const auto outcome=*world_->event_results();event_result["ready"]=true;event_result["generation"]=int64_t(settled->generation);event_result["tree_type"]=settled->tree_type;event_result["event_index"]=settled->event_index;event_result["event_number"]=settled->event_number;event_result["aborted"]=settled->aborted;event_result["player_points"]=int64_t(settled->player_points);
  event_result["random_state"]=int64_t(settled->random_state);event_result["event_name"]=text(world_->frame().event_name);event_result["event_status"]=outcome.event_status;
  godot::PackedByteArray progress;progress.resize(settled->event_progress.size());std::memcpy(progress.ptrw(),settled->event_progress.data(),settled->event_progress.size());event_result["event_progress"]=progress;if(settled->event_profile)event_result["event_profile"]=profile_blocks(*settled->event_profile);if(settled->event_scores)event_result["event_scores"]=score_profile(*settled->event_scores);if(settled->event_settings)event_result["event_settings"]=settings_transfer(*settled->event_settings);
  godot::Array rows;for(const auto& source:outcome.rows){godot::Dictionary row;row["driver_number"]=source.driver_number;row["rank"]=source.position;row["time"]=text(source.time_text);row["packed_time"]=source.packed_time;row["reason"]=source.finish_reason;row["score"]=source.score;row["points"]=source.total_points;row["bonus"]=source.bonus;row["tickets"]=source.tickets;rows.append(row);}event_result["rows"]=rows;if(settled->event_rewards)event_result["event_rewards"]=reward_transfer(*settled->event_rewards);event_result["final_reward_awarded"]=settled->final_reward_awarded;
 }d["event_result"]=event_result;
 if(world_)if(const auto seed=world_->current_random_state())d["random_state"]=int64_t(*seed);
 if(world_){const auto& source=world_->frame();if(source.rule_view)d["rule_view"]=rule_state(*source.rule_view);godot::Dictionary race;race["stage"]=source.race_stage;race["rank"]=source.rank;race["score"]=source.player_score;race["lap"]=source.lap;race["lap_limit"]=source.lap_limit;race["elapsed_ticks"]=source.elapsed_ticks;race["elapsed_text"]=text(source.elapsed_text);race["event_name"]=text(source.event_name);race["track_percent"]=source.track_percent;race["progress"]=source.race_progress;race["finish_reason"]=source.finish_reason;race["player_finished"]=source.player_finished;d["race"]=race;}
 const auto stats=world_?world_->stats():last_stats_;godot::Dictionary footprint;footprint["regions"]=int64_t(stats.regions);footprint["payload_bytes"]=int64_t(stats.payload_bytes);footprint["owned_allocations"]=int64_t(stats.owned_allocations);d["resource_footprint"]=footprint;d["source_audio_commands"]=int64_t(stats.audio_commands);d["source_cleanup_attempted"]=stats.source_cleanup_attempted;d["source_cleanup_ok"]=stats.source_cleanup_ok;d["last_source_cleanup_diagnostic"]=text(last_cleanup_diagnostic_);d["research_operands"]="Explicit PC stack-lane policy: independent zero words; original values unknown";if(!world_)return;const auto& frame=world_->frame();d["player_car"]=int64_t(frame.player_car);d["source_generation"]=int64_t(frame.generation);d["source_ui_revision"]=int64_t(frame.ui_revision);d["source_menu_screen"]=int64_t(frame.menu_screen);d["source_menu_item"]=int64_t(frame.menu_item);d["source_menu_popup"]=frame.menu_popup;d["source_menu_paused"]=frame.menu_paused;d["source_return_requested"]=frame.return_requested;d["source_autosave_requested"]=frame.autosave_requested;d["source_attempt_pending"]=frame.awaiting_attempt;d["source_attempt_requires_saved"]=frame.attempt_requires_saved;d["source_continuation_reload"]=frame.continuation_reload;godot::Array menu_actions;for(const auto& action:frame.result_menu_actions){godot::Dictionary item;item["item"]=int64_t(action.first);item["action"]=action.second;menu_actions.append(item);}d["source_menu_actions"]=menu_actions;d["source_geometry_count"]=int64_t(frame.hud_geometry.size());d["source_ui_screen_count"]=int64_t(frame.ui_screens.size());d["source_driver_info_count"]=frame.source_driver_info_count;d["world_dt"]=frame.dt;
 if(frame.camera){const auto& source=*frame.camera;godot::Dictionary camera;camera["position"]=host(source.position);camera["target"]=host(source.target);camera["up"]=host(source.up);camera["fov_bangle"]=int(source.fov_bangle);camera["near_z"]=source.near_z;camera["hide_car"]=source.hide_car;camera["render_dash"]=source.render_dash;camera["pov_index"]=source.pov_index;camera["pov_type"]=source.pov_type;camera["lookback_param"]=source.lookback_param;d["camera"]=camera;}
 if(frame.mirror_camera&&frame.mirror_view){const auto& source=*frame.mirror_camera;const auto& view=*frame.mirror_view;godot::Dictionary mirror;mirror["position"]=host(source.position);mirror["target"]=host(source.target);mirror["up"]=host(source.up);mirror["fov_bangle"]=int(source.fov_bangle);mirror["near_z"]=source.near_z;mirror["far_z"]=frame.mirror_far_z;mirror["hide_car"]=source.hide_car;mirror["width"]=view.texture_size[0];mirror["height"]=view.texture_size[1];mirror["aspect_x"]=view.aspect_ratio[0];mirror["aspect_y"]=view.aspect_ratio[1];mirror["projection_aspect"]=view.projection_aspect;mirror["vertical_fov"]=view.vertical_fov_degrees;d["mirror_camera"]=mirror;}
 for(const auto& actor:frame.actors)actors.append(actor_state(actor));for(const auto& object:frame.objects){godot::Dictionary row;row["address"]=int64_t(object.address);row["model_hash"]=int64_t(object.model_hash);row["source_instance"]=int64_t(object.source_instance);row["source_section"]=object.source_section;row["source_record"]=object.source_record;row["visible"]=object.visible;row["transform"]=host_matrix(object.transform);objects.append(row);}
 const auto& ui_clip=frame.ui_clip_bounds;d["source_ui_clip_bounds"]=godot::Vector4(ui_clip[0],ui_clip[1],ui_clip[2],ui_clip[3]);
 godot::Array nis;for(const auto& packet:frame.nis_draws){godot::Dictionary row;row["model_hash"]=int64_t(packet.model_hash);row["world"]=host_matrix(packet.world);godot::PackedFloat32Array bones;bones.resize(packet.bones.size()*16);for(unsigned i=0;i<packet.bones.size();++i)for(unsigned j=0;j<16;++j)bones.set(i*16+j,packet.bones[i][j]);row["bones"]=bones;nis.append(row);}d["nis_draws"]=nis;
 });
 godot::Array flags;if(world_)for(const auto& entry:world_->frame().scenery_flags){godot::Dictionary row;row["source_section"]=entry.source_section;row["source_record"]=entry.source_record;row["flags"]=int(entry.flags);if(entry.transform)row["transform"]=host_matrix(*entry.transform);flags.append(row);}d["scenery_flags"]=flags;d["actors"]=actors;d["objects"]=objects;d["actor_count"]=actors.size();if(!error.empty()){d["ok"]=false;d["error"]=text(error);}return d;
}
godot::PackedVector2Array Ps2DrivingRuntime::take_world_pcm(){
 godot::PackedVector2Array out;
 const auto error=catch_all([&]{
  poll_load();
  if(!presented_||phase_=="Blocked"||phase_=="Failed")return;
  const auto& frame=*presented_;
  if(frame.step==audio_step_)return;
  const auto frames=frame.pcm.size()/2;
  if(out.resize(frames)!=godot::OK)throw std::runtime_error("World PCM host allocation failed");
  auto* samples=out.ptrw();
  for(std::size_t i=0;i<frames;++i)samples[i]={frame.pcm[2*i],frame.pcm[2*i+1]};
  audio_step_=frame.step;
 });
 if(!error.empty()){phase_="Failed";error_=error;}
 return out;
}
godot::Dictionary Ps2DrivingRuntime::commit_presentation(std::int64_t step,std::int64_t ui_revision){godot::Dictionary d;const auto error=catch_all([&]{if(!world_||step<0||std::uint64_t(step)!=world_->frame().step)throw std::logic_error("Stale World presentation commit");if(ui_revision>=0&&std::uint64_t(ui_revision)!=world_->frame().ui_revision)throw std::logic_error("Stale World UI revision commit");if(phase_=="Blocked"||phase_=="Failed")throw std::logic_error("Blocked World cannot commit presentation");presented_=world_->frame();});d["ok"]=error.empty();if(!error.empty())d["error"]=text(error);return d;}
std::shared_ptr<const port::track::ParticleSimulation> Ps2DrivingRuntime::world_particle_snapshot()const{return presented_?presented_->particles:std::shared_ptr<const port::track::ParticleSimulation>{};}
godot::Array Ps2DrivingRuntime::get_world_road_path()const{
 godot::Array out;const auto error=catch_all([&]{if(world_&&world_->configured())for(const auto& point:world_->road_points()){
  godot::Dictionary row;row["route"]=point.route;row["point"]=point.point;row["x"]=point.x;row["y"]=point.y;row["z"]=point.z;row["dx"]=point.dx;row["dy"]=point.dy;row["length"]=point.length;row["width_left"]=point.width_left;row["width_right"]=point.width_right;row["distance"]=point.distance;out.append(row);
 }});if(!error.empty())out.clear();return out;
}
godot::Dictionary Ps2DrivingRuntime::get_world_road_course()const{
 godot::Dictionary out;godot::Array points;const auto error=catch_all([&]{
  if(!world_||!world_->configured())return;const auto course=world_->road_course();float distance=0;std::optional<port::race::RoadRoutePoint> previous;
  for(const auto& p:course.points){if(previous)distance+=std::hypot(p.x-previous->x,p.y-previous->y);godot::Array row;row.append(p.x);row.append(p.y);row.append(distance);row.append(p.z);row.append(p.route);row.append(p.point);points.append(row);previous=p;}
  out["ended"]=course.ended;out["looped"]=course.looped;out["loop_entry_index"]=course.loop_entry_index?std::int64_t(*course.loop_entry_index):-1;
 });out["ok"]=error.empty();out["points"]=points;if(!error.empty())out["error"]=text(error);return out;
}
godot::Array Ps2DrivingRuntime::get_world_route()const{godot::Array out;const auto error=catch_all([&]{if(world_&&world_->configured())for(const auto& point:world_->route_points()){godot::Array row;for(auto value:point)row.append(value);out.append(row);}});if(!error.empty())out.clear();return out;}
godot::Dictionary Ps2DrivingRuntime::report_presentation_failure(const godot::String& reason){if(loading_){loading_->cancelled=true;loading_.reset();}if(world_)world_->pause();phase_="Blocked";error_=reason.utf8().get_data();return get_state();}
godot::Dictionary Ps2DrivingRuntime::set_surface(int,int){godot::Dictionary d;d["ok"]=false;d["error"]="Real World uses original track materials; synthetic surface overrides are laboratory-only";return d;}
godot::Dictionary Ps2DrivingRuntime::update_driving_camera(double,bool look_back){godot::Dictionary d;const auto error=catch_all([&]{if(!world_)throw std::logic_error("World camera is not loaded");world_->camera_input(look_back);});d["ok"]=error.empty();if(!error.empty())d["error"]=text(error);return d;}
godot::Dictionary Ps2DrivingRuntime::cycle_driving_camera_pov(){godot::Dictionary d;const auto error=catch_all([&]{if(!world_)throw std::logic_error("World camera is not loaded");world_->cycle_camera_pov();});d["ok"]=error.empty();if(!error.empty())d["error"]=text(error);return d;}
godot::Dictionary Ps2DrivingRuntime::get_driving_audio_bank()const{godot::Dictionary d;d["ok"]=false;d["error"]="Real World publishes original shared-source PCM through take_world_pcm";return d;}
godot::Dictionary Ps2DrivingRuntime::get_driving_audio_state()const{return get_driving_audio_bank();}
godot::Array Ps2DrivingRuntime::take_collision_audio_events(){return {};}
godot::Array Ps2DrivingRuntime::take_splash_audio_events(){return {};}
void Ps2DrivingRuntime::clear_particles(){}
float Ps2DrivingRuntime::particle_timestep(std::uint64_t last)const{return presented_&&presented_->step!=last?presented_->dt:0;}
std::uint64_t Ps2DrivingRuntime::particle_step()const{return presented_?presented_->step:0;}
std::array<float,3> Ps2DrivingRuntime::particle_velocity()const{return world_&&world_->player()?world_->player()->state.velocity.value_or(std::array<float,3>{}):std::array<float,3>{};}
void Ps2DrivingRuntime::emit_wheel_particles(port::track::ParticleSimulation&,port::driving::WheelParticleEffects&,const std::array<std::array<float,3>,3>&){throw std::logic_error("Legacy wheel-effect retrigger is not valid for a source World; consume its committed effect state");}
godot::Dictionary Ps2DrivingRuntime::present_particles(godot::Node3D*,godot::Camera3D*){godot::Dictionary d;d["ok"]=false;d["error"]="Synthetic-plane particle display is not a World entry";return d;}
godot::Dictionary Ps2DrivingRuntime::render_skids(const godot::Transform3D& camera,const godot::Projection& clip,double H){
    godot::Dictionary result;godot::Array commands;
    const auto error=catch_all([&]{require_requests();
        port::driving::SkidView view;view.position={{float(camera.origin.z),float(camera.origin.x),float(camera.origin.y)}};view.H=float(H);
        // Port camera boundary: Godot Projection is symmetric clip-space; native
        // (x,y,z)->host(y,z,x). Original row-vector m[4*basis+component].
        constexpr int columns[]{2,0,1,3};
        for(int basis=0;basis<4;++basis)for(int component=0;component<4;++component)view.clip_matrix[basis*4+component]=float(clip.columns[columns[basis]][component]);
        const auto quads=world_?world_->render_skids(view):std::vector<port::driving::SkidQuad>{};
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
    result["ok"]=error.empty();if(error.empty())result["commands"]=commands;else{phase_="Blocked";error_=error;if(world_)world_->pause();result["error"]=godot::String::utf8(error.c_str());}return result;
}

}

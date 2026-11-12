#include "event_runtime_continuation.hpp"
#include "event_reward_codec.hpp"
#include "game_core/port/race/nis_services.hpp"
#include "game_core/port/race/nis_resources.hpp"
#include "game_core/port/race/nis_camera.hpp"
// TASK-0107. Host composition of existing recovered World/source-resource calls.
#include "game_core/indep/inc/Scenery.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/CopChase.hpp"
#include "game_core/indep/inc/CarController.hpp"
// TASK-0099 E-0099-W32. Actual World + complete Car construction resource corpus.
// Driver/start and temporary padding operands are explicit fixture inputs.
// --sound preserves original enabled sound flags; participant movement, driver
// and optional collision stages do not claim a full scheduler/original oracle.
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/indep/inc/AIPlan.hpp"
#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/Contact.hpp"
#include "game_core/indep/inc/Skids.hpp"
#include "game_core/indep/src/collision.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/race/car_reset_services.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/race/cop_resources.hpp"
#include "game_core/port/race/cop_speech.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/race/events.hpp"
#include "game_core/port/race/munitions.hpp"
#include "game_core/port/race/smokeables.hpp"
#include "game_core/port/track/assets.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/eAnimation.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "event_world_launch.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/race/settlement.hpp"
#include "game_core/port/race/player_hud_gameplay.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include "game_core/port/race/race_construction.hpp"
#include "game_core/port/race/car_sound_resources.hpp"
#include "game_core/port/race/car_sound_construction.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/src/bList.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/inc/RaceCoordinator.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/port/race/pcm_command_device.hpp"
#include "game_core/port/race/joylog.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/port/driving/player_services.hpp"
#include "game_core/port/driving/horn.hpp"
#include "game_core/indep/inc/NIS.hpp"
#include "game_core/port/race/player_construction.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/port/race/traffic_construction.hpp"
#include "game_core/port/race/coordinator_construction.hpp"
#include "game_core/ResourceFile.hpp"
#include "game_core/PauseMenu.hpp"
#include "game_core/indep/src/soundcollision.hpp"
#include "game_core/indep/src/sound3dsample.hpp"
#include "game_core/indep/src/joystick.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/src/bDebug.hpp"
#include "game_core/indep/src/quickspline.hpp"
#include "game_core/port/race/route_observation.hpp"
#include "game_core/indep/src/rumble.hpp"
#include "game_core/src/logitechwheel.hpp"
#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/port/race/world_frame.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/port/race/frontend_event_resources.hpp"
#include "game_core/port/race/frontend_language_resources.hpp"
#include "port/runtime/world_session.hpp"
#include "port/runtime/race_presentation.hpp"
#include "port/runtime/race_menu_host.hpp"
#include "game_core/port/race/pause_services.hpp"
#include "game_core/port/race/pause_submenus.hpp"
#include "game_core/TrackMap.hpp"
#include "game_core/port/race/end_game_screens.hpp"
#include "game_core/port/race/race_speech.hpp"
#include "game_core/indep/inc/EndGameScreens.hpp"
#include "game_core/indep/src/gameflow.hpp"
#include "port/runtime/event_language_asset.hpp"
#include "port/runtime/free_run_launch.hpp"
#include "port/runtime/race_launch_catalog.hpp"
#include "game_core/port/track/position_markers.hpp"
#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/port/driving/vehicle_lights.hpp"
#include "game_core/port/driving/vehicle_flares.hpp"
#include "game_core/port/driving/horn.hpp"
#include "game_core/port/track/particles.hpp"
#include <atomic>
#include <limits>
#include <map>

namespace port::race {
namespace {
std::atomic<std::uint64_t> next_generation{1};
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
// Port presentation decoder shared with track/assets.cpp's raw scenery layout.
// Read the source-mutated record; no host inference of backwards-track rotation.
driving::Matrix scenery_matrix(const driving::Memory& m,std::uint32_t address){
 driving::Matrix result{};result[15]=1;
 if(m.half(address+0xe)&0x200)result[0]=result[5]=result[10]=1;
 else {for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)result[col*4+row]=float(static_cast<std::int16_t>(m.half(address+0x1c+(col*3+row)*2)))*(1.f/16384.f);for(unsigned row=0;row<3;++row)result[12+row]=m.scalar(address+0x10+row*4);}
 return result;
}
void validate_research(const WorldLaunchRequest& r){
 if(!r.research_operands)throw std::invalid_argument("World debug requires explicitly declared PC research stack operands");
 const auto& v=*r.research_operands;
 if(!v.construction_w||!v.physics_w||!v.ai_w||!v.event_disc_w||!v.terrain_disc_w||!v.camera_shake_w||!v.acid_w||!v.negative_override_route||!v.negative_override_point)
  throw std::invalid_argument("World debug research stack operands are incomplete; no original values inferred");
 for(const auto& w:v.racer_start_w)if(!w)throw std::invalid_argument("World debug racer start stack operands are incomplete");
}
}
struct WorldSession::Impl {
 driving::Memory m;
 WorldLaunchRequest request;
 WorldFrame published;
 std::vector<NisDrawPacket> prepared_nis_draws;
 std::optional<RaceRulesView> sampled_rules;
 driving::Input input;
 std::string phase="Empty", diagnostic;
 WorldSessionStats last_stats;
 bool rc_complete=false, boot_complete=false, horn=false;
 bool look_back=false,applied_look_back=false;
 unsigned camera_cycles=0;
 bool collision_audio_registered=false,oneshot_audio_initialized=false;
 double pending_elapsed=0;
 std::uint32_t rc=0,world=0,race=0,replay=0,player_address=0,registration_pool=0;
 std::filesystem::path base;
 std::unique_ptr<assets::DirectorySource> source;
 std::unique_ptr<track::TextureBindings> textures;
 std::unique_ptr<track::EmitterBindings> emitters;
 std::unique_ptr<CarRenderResources> common;
 std::unique_ptr<FrontendLanguageResources> language;
 std::unique_ptr<FrontendEventResources> frontend_events;
 CharacterResources characters;
 track::RouteData routes;
 track::TopologyResources topology;
 // CopResources retains this decoded topology beyond bootstrap.
 track::Topology decoded_topology;
 EventResources events;
 SmokeableResources smokeables;
 struct SceneryBaseline {int section,record;std::uint32_t address;std::uint16_t flags;driving::Matrix transform;};
 std::vector<SceneryBaseline> scenery_baseline;
 CopResources cops;
 std::unique_ptr<CopSpeechBindings> speech;
 std::unique_ptr<FlailerResources> ui;
 std::unique_ptr<NisInfoResources> nis;
 std::unique_ptr<NisResources> nis_files;
 NisServices nis_services;
 std::unique_ptr<CarSoundResources> audio;
 std::unique_ptr<AiPlanningResources> planning;
 track::PositionMarkerResources markers;
 CollisionResources collisions;
 std::vector<std::unique_ptr<CarRenderResources>> cars;
 std::unique_ptr<MemoryAvailability> budget;
 PcmCommandDevice pcm;
 PhysicsConstructionServices physics;
 AiDriveTargetServices ai;
 driving::PlayerServices hardware;
 std::array<PlayerHudGameplayState,2> hud_gameplay;
 PlayerHudPresentationServices hud_services;
 PauseServices pause_services;
 EndGameScreenServices result_services;
 RewardAwardReceipt reward_receipt;
 std::unique_ptr<PauseSubmenus> pause_submenus;
 std::unique_ptr<ResultSubmenus> result_submenus;
 bool autosave_requested=false;
 bool awaiting_attempt=false,attempt_requires_saved=false,attempt_reload=false,continuation_action=false;
 std::optional<EventSettlement> previous_settlement;
 std::optional<SettlementSnapshot> previous_results;
 std::optional<WorldLaunchRequest> continuation_launch;
 void clear_hud_presentation(){if(request.original_ui)for(int i=0;i<m.signed_word(0x327780);++i)hp2::debug::Player(m,m.word(0x327778+4*unsigned(i))).SetHudFeatures(0,hud_services);}
 std::optional<EventSettlement> source_settlement(){
  if(!request.event||!rc||m.word(rc)!=7)return {};
  const auto& event=*request.event;EventSettlement out;out.generation=event.generation;out.tree_type=event.tree_type;out.event_index=event.event_index;out.event_number=event.event_number;
  out.aborted=read_settlement(m,rc).aborted;out.player_points=m.word(0x2f3890+0x10e04);out.random_state=m.word(0x329060);out.event_progress=export_event_progress(m,event,*frontend_events);out.event_profile=export_event_profile(m,event,*frontend_events);out.event_scores=export_event_scores(m,event);out.event_settings=export_event_settings(m,event);if(event.event_rewards){out.event_rewards=::port::runtime::read_debug_event_rewards(m,0x2f3890);out.final_reward_awarded=reward_receipt.valid&&reward_receipt.source_completed&&reward_receipt.point_stores[19];}return out;
 }
 void retain_result(){if(request.event&&rc&&m.word(rc)==7){auto settled=source_settlement();auto results=read_settlement(m,rc);previous_settlement=std::move(settled);previous_results=std::move(results);}}

 std::unique_ptr<RaceMenuHost> menus;
 track::Entry track_entry;
 std::chrono::steady_clock::time_point epoch=std::chrono::steady_clock::now();
 std::uint32_t ticker()const{return static_cast<std::uint32_t>(static_cast<std::uint64_t>(std::chrono::duration<double>(std::chrono::steady_clock::now()-epoch).count()*m.scalar(0x326254)));}
 WorldConstructionServices world_services(){
  WorldConstructionServices s;s.routes=&routes;s.smokeables=&smokeables;s.cops.resources=&cops;
  s.texture_info=[this](auto h,int d,int u){return textures->lookup(h,d,u);};
  s.emitter_group=[this](auto&,auto list,auto hash){emitters->populate(list,hash);};
  s.cops.allocate_voices=[this](auto&,auto owner){speech->allocate_voices(owner);};
  s.cops.flush_speech_queue=[this](auto&,auto owner){speech->flush_queue(owner);};return s;
 }
 CarConstructionServices car_services(){CarConstructionServices s;s.routes=&routes;s.topology=&topology;s.collisions=&collisions;s.characters=&characters;s.audio=audio.get();s.port_stack_vector_lane=request.research_operands->construction_w;return s;}
 RaceStartOperands start_operands(){RaceStartOperands s;for(unsigned i=0;i<8;++i)s.racer_start_w[i]=request.research_operands->racer_start_w[i];s.negative_override_starting_nri=hp2::debug::NearestRoutePointInfo{*request.research_operands->negative_override_route,*request.research_operands->negative_override_point};return s;}
 template<class Fn>void coordinator(Fn&& fn){NisServicesScope nis_scope(m,nis_services);auto ws=world_services();auto cs=car_services();auto starts=start_operands();CoordinatorConstructionServices s{ws,cs,physics,ai,*audio,starts,*budget,*speech};s.minimal_settlement_presentation=(request.event.has_value()||request.original_ui)&&request.minimal_driving;s.pause=request.original_ui?&pause_services:nullptr;s.end_game_screens=request.original_ui?&result_services:nullptr;s.reward_receipt=&reward_receipt;fn(s);}
 // Port binding of the actual pause callbacks to authored Flailer documents.
 void initialize_menus(){
  hud_services=source_player_hud_services();
  hud_services.track_map=[](auto& memory,std::uint32_t player,bool enabled){
   if(enabled){const auto map=memory.allocate(0xb0);hp2::debug::TrackMap(memory,map).Sub_001B7FD0(memory.word(0x327398),memory.word(player+4),player);memory.word(player+0x5ec,map);int mode=memory.signed_byte(memory.word(0x3121ec)?0x312268:memory.signed_word(0x327780)>1?0x312267:0x312266);if(!memory.word(0x3121ec)&&memory.signed_byte(player+0x1c)==1&&mode==6)mode=2;hp2::debug::Player(memory,player).SetTrackMapMode(mode);}
   else{if(const auto map=memory.word(player+0x5ec))hp2::debug::TrackMap(memory,map).Sub_001B81E0(3);memory.word(player+0x5ec,0);}
  };
  for(auto& hud:hud_gameplay){hud.source_hud=true;hud.presentation=&hud_services;}
  menus=std::make_unique<RaceMenuHost>(m);
  pause_submenus=std::make_unique<PauseSubmenus>(m);result_submenus=std::make_unique<ResultSubmenus>(m);
  initialize_end_game_presentation(m);
  // E-0112-P03/R07: source menu registration names differ from Flailer hashes.
  for(const auto& page:std::array<std::pair<std::uint32_t,std::uint32_t>,7>{{{0x3276d8,0x605d2bf1},{0x3276dc,0x2b5a03a8},{0x3276e0,0x6b321e4e},{0x3276e4,0xcc36fab2},{0x3276e8,0xd4742cd7},{0x3276ec,0x887b4d37},{0x3276f0,0xb5ecd7a0}}})menus->register_page(m.word(page.first),page.second);
  for(const auto& page:std::array<std::pair<std::uint32_t,std::uint32_t>,13>{{{0x328480,0x7e420558},{0x328484,0x7e420558},{0x328488,0x7e420558},{0x32848c,0x7e420558},{0x328490,0xab049b2b},{0x328494,0x0532b6ff},{0x328498,0xacb2a4ca},{0x32849c,0xf5a3cf51},{0x3284b4,0x1a64e903},{0x3284a4,0x67eb8d12},{0x3284a8,0x67eb8d33},{0x3284ac,0xc4881cbd},{0x3284b0,0xc4881cde}}})menus->register_page(m.word(page.first),page.second);
  menus->bind([this](std::uint32_t screen,hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& data){
   if(screen==0x605d2bf1)hp2::debug::PauseMenu(m,m.word(0x3276fc)).PauseMenuCallback(msg,data,pause_services);
   else if(!pause_submenus->dispatch(screen,msg,data,pause_services))result_submenus->dispatch(screen,msg,data,result_services);
  },[](int sound){hp2::frontend::PlayMenuSound_0016E038(sound);});
  pause_services.change_screen=[this](auto name,int initial){menus->change_screen(name,initial);};
  // The port input primitive represents a keyboard pad, never a PS2 wheel.
  // PortStopForce0012C398 has no force write for GetWheelId()==2.
  pause_services.stop_force=[this](int port){hp2::debug::PortStopForce(m,port);};
  pause_services.rumble_impact=[this](int port,float magnitude){hp2::debug::PortRumbleImpact(m,port,magnitude);};
  pause_services.joystick_type=[this](int port){return hp2::debug::GetJoystickType(m,port);};
  pause_services.all_joysticks_plugged=[this]{for(int i=0;i<m.signed_word(0x327780);++i)if(!hardware.IsJoystickPluggedIn(m.signed_word(m.word(0x327778+4*i)+8)))return false;return true;};
  pause_services.set_screen_filter_port=[this](std::uint32_t name,int port){menus->filter_port(name,port);};
  pause_services.current_screen=[this]{return menus->current_screen();};
  pause_services.gray_menu_item=[this](auto item,int mode,auto screen){menus->gray(item,mode,screen);};
  pause_services.request_unpause=[this](int reason){if(reason==2){clear_hud_presentation();continuation_action=true;}hp2::debug::RaceCoordinator(m,rc).PostMessage(6,reason);};
  pause_services.mute=[this](bool value){if(m.word(0x326f40))m.word(0x327f08,value?1:0);::port::diagnostics::warn("0x001ACE38","PS2 sound.Mute children are handled by the PC output boundary","source pause freezes World audio production; host silences and restores the committed PCM stream");};
  pause_services.summon_popup=[this](const PauseConfirmation& input){
   ::port::selection::Popup config;config.screen_hash=input.screen_hash;config.port=input.port;config.flags=input.flags;config.cancel_param=input.cancel_param;config.n_buttons=input.n_buttons;config.start_button=input.start_button;config.title_id=input.title_id;config.button_ids=input.button_ids;config.button_params=input.button_params;
   // Neither unknown caller stack operand is consumed by ordinary flag3 input.
   // External DismissPopup is deliberately not exposed by this carrier.
   config.dismiss_param=0;config.message_id=0;
   std::string message;for(unsigned i=0;i<4096;++i){const auto c=m.byte(input.message+i);if(!c)break;message.push_back(char(c));if(i==4095)throw std::runtime_error("Source pause message lacks a terminator");}
   menus->summon(config,message);
  };
  result_services.change_screen=[this](auto name,int root,bool){menus->change_screen(name,root);};
  result_services.screen_visibility=[this](auto hash,int value){const auto screen=hp2::debug::FindFlailerScreen(m,hash);if(screen)hp2::debug::FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,value);};
  result_services.current_screen=pause_services.current_screen;
  result_services.gray_menu_item=pause_services.gray_menu_item;
  result_services.summon_popup=pause_services.summon_popup;
  result_services.post_message=[this](int msg,int param){if(msg==8){clear_hud_presentation();continuation_action=true;}hp2::debug::RaceCoordinator(m,rc).PostMessage(msg,param);};
  menus->register_page(m.word(0x3284a0),0xfa2ac5c6);
  result_services.clear_autosave_popup=[this]{menus->dismiss_popup(0);};
  result_services.set_autosave_callback=[this]{autosave_requested=true;};
  bind_source_result_speech_services(m,result_services,request.minimal_driving);

 }
 void capture_menu_actions(WorldFrame& frame){
  frame.result_menu_actions.clear();if(!menus||!result_submenus||menus->current_screen()!=0x1a64e903)return;
  const auto instance=result_submenus->instance();if(!instance)return;const auto head=instance+104;unsigned count=0;
  for(auto node=m.word(head);node!=head;node=m.word(node)){require(++count<=6,"Original result menu exceeded six source actions");frame.result_menu_actions.emplace_back(m.word(node+12),m.signed_word(node+16));}
 }
 void capture_mirror(WorldFrame& frame){
  frame.mirror_view.reset();frame.mirror_camera.reset();
  if(!request.original_ui)return;
  const auto view=source_hud_mirror_view(m,player_address);if(!view.camera)return;
  driving::DrivingCameraFrame camera;for(unsigned axis=0;axis<3;++axis){camera.position[axis]=m.scalar(view.camera+0x40+axis*4);camera.target[axis]=m.scalar(view.camera+0x60+axis*4);camera.up[axis]=-m.scalar(view.camera+4+axis*16);}camera.fov_bangle=m.half(view.camera+0x84);camera.near_z=m.scalar(view.camera+0x7c);camera.hide_car=m.word(view.camera+0x1bc)==m.word(player_address+4);frame.mirror_camera=camera;frame.mirror_view=view;frame.mirror_far_z=m.scalar(view.camera+0x80);
 }
 void publish_ui(){
  WorldFrame next=published;next.dt=0;next.pcm.clear();++next.ui_revision;
  for(unsigned i=0;i<4;++i)next.ui_clip_bounds[i]=m.scalar(0x3271e8+4*i);
  if(request.original_ui)prepare_source_hud_geometry(m);next.ui_screens=snapshot_visible_flailer(m,0,true,request.original_ui);if(pause_submenus)pause_submenus->annotate(next.ui_screens);next.menu_screen=menus?menus->current_screen():0;next.menu_item=menus?menus->selected_item():0;next.menu_popup=menus&&menus->popup_active();next.menu_paused=m.word(rc)==6;
  if(request.original_ui)update_source_hud_mirror(m,player_address);capture_menu_actions(next);capture_mirror(next);next.autosave_requested=autosave_requested;
  next.return_requested=m.word(0x3275b0)==hp2::debug::GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC&&m.word(0x3275b4)!=hp2::debug::GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC;
  next.awaiting_attempt=awaiting_attempt;next.attempt_requires_saved=attempt_requires_saved;next.continuation_reload=attempt_reload;
  if(request.event)next.rule_view=capture_rules();
  next.nis_draws=prepared_nis_draws;
  published=std::move(next);
 }
 void reconcile_menu(bool from_results=false){
  const bool rebuilt=m.word(0x3272f0)!=world;
  if(rebuilt){
   prepared_nis_draws.clear();
   world=m.word(0x3272f0);race=m.word(0x327258);replay=m.word(0x3271a8);player_address=m.word(0x327778);
   require(world&&race&&replay&&player_address,"Restart did not restore source owners");
   for(auto& hud:hud_gameplay){const auto source_hud=hud.source_hud;const auto presentation=hud.presentation;hud={};hud.source_hud=source_hud;hud.presentation=presentation;}
   hp2::debug::Player(m,player_address).ChangeCameraMover(1,false);update_camera();published.generation=next_generation++;published.step=0;publish(0);
  }
  input={};pending_elapsed=0;look_back=applied_look_back=false;camera_cycles=0;
  const bool reload=m.word(0x3275b0)==hp2::debug::GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC&&m.word(0x3275b4)==hp2::debug::GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC;
  if(reload)prepared_nis_draws.clear();
  if(request.event&&(reload||rebuilt||continuation_action)){
   awaiting_attempt=true;attempt_requires_saved=from_results;attempt_reload=reload;phase="AwaitingAttempt";
  }else if(m.word(rc)==6)phase="Paused";else if(m.word(rc)==7)phase="Results";else if(m.word(rc)==3)phase="Running";
  continuation_action=false;
  publish_ui();
 }
 void bootstrap(){
  NisServicesScope nis_scope(m,nis_services);
  driving::PcSqrtCarrierScope numerical_model(request.minimal_driving);
  for(auto& state:hud_gameplay)state.camera=request.hud_camera;
  validate_research(request);
  physics.enable_skid_trails=!request.minimal_driving;
  physics.disable_smokeable_collision_sound=request.minimal_driving;
  physics.disable_race_lap_speech=request.minimal_driving;
  physics.disable_pursuit_speech_emission=request.minimal_driving;
  physics.disable_pursuit_nis_asset_load=request.minimal_driving&&!request.original_ui;
  physics.cop_speedtrap_velocity_w=request.research_operands->cop_speedtrap_velocity_w;
  physics.cop_mover_ground_normal_w=request.research_operands->cop_mover_ground_normal_w;
  physics.cop_mover_invalid_ground_normal_w=request.research_operands->cop_mover_invalid_ground_normal_w;physics.cop_mover_airborne_angular_w=request.research_operands->cop_mover_airborne_angular_w;
  physics.mellow_jump_up_w=request.research_operands->mellow_jump_up_w;
  physics.pursuit_add_point_w=request.research_operands->pursuit_add_point_w;
  physics.pursuit_set_point_w=request.research_operands->pursuit_set_point_w;
  physics.average_air_position_w=request.research_operands->average_air_position_w;
  physics.average_air_acceleration_w=request.research_operands->average_air_acceleration_w;
  physics.cop_desired_velocity_w=request.research_operands->cop_desired_velocity_w;physics.cop_target_point_w=request.research_operands->cop_target_point_w;physics.cop_bump_arm_w=request.research_operands->cop_bump_arm_w;physics.cop_roadblock_velocity_w=request.research_operands->cop_roadblock_velocity_w;
  physics.disable_jump_camera_presentation=request.minimal_driving;
  if(request.research_operands->mellow_jump_up_w)::port::diagnostics::warn("0x00214DE8","Original airborne world-up vector W padding is unknown","independent explicit PC stack-lane operand supplied; no original value inferred");
  if(request.minimal_driving){
   ::port::diagnostics::warn("0x00202108","Original lap speech voice presentation is not bound","lap/timer/record bookkeeping, speech selection and shared RNG remain active; only SPCH submission is disabled");
   ::port::diagnostics::warn("0x001E1B68","Original smokeable collision sound presentation is not reconstructed","collision sound disabled by the minimal-driving PC preset; physical anchor/break/awake/impulse behavior remains active");
   ::port::diagnostics::warn("0x001A81DC","Shared skid generation has unresolved original VU operands","skid trails disabled by the minimal-driving PC preset; physics continues");
   ::port::diagnostics::warn("0x001A88C8","Shared World skid rendering is not implemented","skid display disabled by the minimal-driving PC preset; physics continues");
  }
  const auto root=hp2::platform::resolve_zzdata_root(request.project_root.string());require(root.has_value(),"Original ZZDATA root is not configured");base=*root;source=std::make_unique<assets::DirectorySource>(*root);
  m=driving::read_debug_image(driving::debug_image_path(request.project_root));
  initialize_camera_view_state(m);hp2::debug::Sub_0013A978(m);hp2::recovered::Sub_0013C750(m);hp2::debug::Sub_00136628(m);hp2::recovered::Sub_0019B358(m);hp2::recovered::Sub_00139510(m);hp2::recovered::Sub_001A2FC0(m);hp2::recovered::Sub_00254350(m);hp2::debug::Sub_00216250(m);hp2::debug::Sub_002517F0(m);hp2::debug::Sub_001A4990(m);hp2::debug::Sub_001D1280(m);hp2::debug::Sub_001D70D0(m);hp2::debug::Sub_001D58C8(m);hp2::debug::Sub_002005D8(m);
  hp2::debug::Sub_001BCDC0(m);hp2::debug::Sub_001A8B30(m);hp2::debug::Sub_001A90F0(m);hp2::debug::Sub_0021C5C8(m);hp2::debug::Sub_001B7DE0(m);hp2::debug::Sub_002141F8(m);hp2::debug::Sub_0022B320(m);hp2::debug::Sub_001CACF0(m);hp2::debug::Sub_0012CA50(m);hp2::debug::Sub_00156478(m);hp2::debug::Sub_001674A8(m);
  initialize_damage_resource_lists(m);initialize_render_resource_state(m);hp2::debug::Sub_001E2348(m);hp2::debug::Sub_00155228(m);hp2::debug::Sub_001AD710(m);hp2::debug::Sub_00204C40(m);hp2::debug::Joylog::Init(m);
  for(unsigned p=0;p<2;++p)m.word(0x2f0da8+204*p+184,p);
  registration_pool=hp2::debug::bNewSlotPool(m,0x24,0x60,0x2a8160,0x10);m.word(0x3273a8,registration_pool);
  hp2::debug::InitCollisionBody(m);hp2::debug::InitContact(m);hp2::debug::InitCollision(m);hp2::debug::emEventManagerInit(m);eInitSlotPools(m);hp2::debug::InitWorldObjects(m);
  const auto global=source->read("GLOBAL/GLOBALB.BUN"),ingame=source->read("GLOBAL/INGAMEB.BUN"),character_bytes=source->read("GLOBAL/CHARACTERSA.BIN"),front=source->read("FRONTEND/FRONTB.BUN");
  textures=std::make_unique<track::TextureBindings>(m);textures->init();textures->load_bundle(global);textures->load_bundle(front);
  language=std::make_unique<FrontendLanguageResources>(m);language->load(source->read(request.event?::port::runtime::event_language_asset(request.event->language):"LANGUAGES/FE_ENGLISH.BIN"));language->load_track_names(global);
  common=std::make_unique<CarRenderResources>(m);common->load_bundle(global);common->load_bundle(ingame);common->load_bundle(character_bytes);hp2::debug::InitCarInfo(m);characters=load_character_resources(m,{global,character_bytes});
  planning=std::make_unique<AiPlanningResources>(m);planning->load_retail_difficulty_info(global);
  LaunchCatalog launch(global,m);
  if(request.event){initialize_frontend_event_resource_state(m);frontend_events=std::make_unique<FrontendEventResources>(m);frontend_events->load_bundle(front,0);prepare_event_world_launch(m,request,launch,*frontend_events);}
  else prepare_free_run_launch(m,request,launch);
  hp2::debug::RaceParameters parameters(m,0x311f78);const unsigned driver_count=m.word(0x312280);require(driver_count>0&&driver_count<=14,"Original FreeRun roster count outside source World capacity");hp2::debug::InitPhysics(m);
  const auto catalog=track::load_catalog(*source);const auto selected=std::find_if(catalog.begin(),catalog.end(),[this](const auto& row){return row.number==request.track;});require(selected!=catalog.end(),"Selected original track is unavailable");track_entry=*selected;
  const auto track_bytes=track::decompress_bundle(source->read(track_entry.resources.at(1)));
  events=load_retail_event_resources(m,track_bytes);routes=track::load_retail_route_data(m,track_bytes);hp2::debug::InitTrackRoutes(m,routes,!request.reverse);
  hp2::debug::TopologyTree tree(m,0x2f0d10);tree.Sub_00152760();tree.InitTree();topology=track::load_retail_topology_resources(m,track_bytes);
  if(request.original_ui)textures->load_bundle(source->read("GLOBAL/HUDTEX.BIN"));
  for(const auto* p:{"GLOBAL/INGAMEB.BUN","GLOBAL/DYNTEX.BIN","GLOBAL/DRIVTEX.BIN","CARS/TEXTURES.BIN"})textures->load_bundle(source->read(p));textures->load_bundle(source->read(track_entry.resources.at(2)));textures->load_bundle(source->read(track_entry.resources.at(3)));
  emitters=std::make_unique<track::EmitterBindings>(m);emitters->init();emitters->load_bundle(global);emitters->load_bundle(ingame);emitters->load_bundle(track_bytes);common->load_bundle(track_bytes);
  // Capture the immutable forward asset baseline before the source loader's
  // backwards-track mutation, then perform that same recovered loader call.
  smokeables=load_smokeable_resources(m,global,track_bytes,false);
  for(const auto& section:smokeables.section_headers){const auto records=m.word(section.second+0x1c);const auto count=m.signed_word(section.second+0x20);require(count>=0,"Original scenery record count is negative");for(int i=0;i<count;++i){const auto address=records+std::uint32_t(i)*0x30;scenery_baseline.push_back({section.first,i,address,m.half(address+0xe),scenery_matrix(m,address)});}}
  if(request.reverse)for(const auto header:smokeables.section_load_order)hp2::recovered::ScenerySectionHeader::FlipObjectsOnBackwardsTrack(m,header);
  initialize_scenery_models(m,smokeables);decoded_topology.load(track_bytes);cops=load_cop_resources(m,track_bytes,routes,decoded_topology);speech=std::make_unique<CopSpeechBindings>(m);speech->init();
  initialize_radial_blur_resource_state(m);hp2::debug::bSample::Initialize(m,1024);hp2::debug::Sub_001B1100(m);hp2::debug::Sub_001462B0(m);
  ui=std::make_unique<FlailerResources>(m);ui->load_bundle(global);ui->load_bundle(ingame);ui->load_bundle(front);nis=std::make_unique<NisInfoResources>(m);nis->load_bundle(global);
  hp2::debug::initialize_source_nis_camera_state(m);hp2::debug::Sub_001A53E8(m);nis_files=std::make_unique<NisResources>(m,*source,hp2::debug::LoaderCameras,hp2::debug::UnloaderCameras);
  nis_services.physics=&physics;nis_services.placement_route_position_w=request.research_operands->nis_placement_route_position_w;nis_services.load_resource=[this](const auto& name){return nis_files->load(name);};nis_services.unload_resource=[this](auto token){nis_files->unload(token);};
  // Loading is synchronous and fully validated before its token is returned;
  // this device has no queued transfers for the original wait-loop pump.
  nis_services.service_resource_loading=[]{};nis_services.read_local_clock=host_nis_clock;
  nis_services.finished_callback=[this](std::uint32_t callback,int param){if(callback==0x002200e0)hp2::debug::EndPulloverBridge(m,param,physics.car_reset);else hp2::not_implemented("0x00174F48","unbound original NIS completion callback");};
  audio=std::make_unique<CarSoundResources>(m);audio->initialize_lookup_state();audio->load_region(base,"InGame");const auto track_bank=audio->load_track_sound_info(global,track_entry.number);if(request.original_ui)audio->enable_track_hud_info();hp2::debug::Sub_0018D100(m);audio->load_engine_definitions(base);
  std::vector<std::string> banks{"InGame"};auto load_bank=[&](const std::string& bank){if(!bank.empty()&&std::find(banks.begin(),banks.end(),bank)==banks.end()){audio->load_region(base,bank);banks.push_back(bank);}};load_bank(track_bank);
  std::vector<unsigned> asset_types;std::vector<std::vector<std::uint8_t>> geometry;
  for(unsigned i=0;i<driver_count;++i){const auto d=parameters.GetDriverInfo(int(i));const auto type=m.word(d+12);require(type<64,"Original FreeRun driver car type outside loaded source table");load_bank(original_sound_text(m,m.word(0x3278e0)+type*0x560+0x330));if(std::find(asset_types.begin(),asset_types.end(),type)==asset_types.end())asset_types.push_back(type);}
  audio->initialize_software_voices();
  if(request.continuation)restore_event_continuation_audio(m,*request.continuation);
  // E-0110-AU01: original LoadSoundRegionsInGame component initialization.
  // The synchronous host owns region loading; no SPU/async completion is faked.
  hp2::debug::collision_sound::Register(m);collision_audio_registered=true;
  ::port::diagnostics::warn("0x00210388","Original SpeechNoise sample-request presentation is not bound in this World host","deferred speech presentation disabled; collision and one-shot owners initialize in original order");
  hp2::debug::sound3d_oneshot::Init(m);oneshot_audio_initialized=true;
  for(const auto address:{"0x00204EC8","0x002011F8","0x00275F88","0x0018CA20","0x001CBBC0","0x002103D8","0x001E71D0","0x0026F9A0","0x001AB790"})
   ::port::diagnostics::warn(address,"Full original sound.Update presentation producer is not bound by the partial World audio host","producer disabled; reviewed source collision/one-shot lifecycle and existing PC sample mixer remain active");
  hp2::debug::InitAIPlans(m);hp2::debug::Sub_00218AD8(m);
  track::initialize_position_marker_list(m);markers=track::load_retail_position_markers(m,track_bytes);hp2::debug::bPListInit(m,32);
  std::array<int,64> slots;slots.fill(-1);
  for(auto type:asset_types){const auto info=m.word(0x3278e0)+type*0x560;auto name=render_resource_text(m,info+64);std::replace(name.begin(),name.end(),'\\','/');geometry.push_back(source->read(name));slots[type]=int(cars.size());auto owner=std::make_unique<CarRenderResources>(m);owner->load(base/render_resource_text(m,info+64));const auto dash=render_resource_text(m,info+128);if(!dash.empty())owner->load(base/dash);cars.push_back(std::move(owner));}
  collisions=load_collision_resources(m,geometry);for(unsigned i=0;i<driver_count;++i){const auto d=parameters.GetDriverInfo(int(i));cars.at(std::size_t(slots[m.word(d+12)]))->prepare_ride(d+12);}
  physics.port_stack_vector_lane=request.research_operands->physics_w;physics.ticker=[this]{return ticker();};ai.routes=&routes;ai.planning=planning.get();ai.port_stack_vector_lane=request.research_operands->ai_w;
  // The host semantic controls represent the actual Player's input device.
  // Source RealDriver::Drive resets all analog values for an absent device.
  hardware.IsJoystickPluggedIn=[this](int port){return player_address&&port==m.signed_word(player_address+8);};hardware.IsJoystickTypeWheel=[this](int port){return hp2::debug::IsJoystickTypeWheel(m,port);};
  hp2::debug::Sub_00197508(m);const auto available=MemoryAvailability::from_host(m);budget=std::make_unique<MemoryAvailability>(m,available.capacity());
  if(request.original_ui)initialize_menus();
  rc=m.allocate(0x90);coordinator([this](auto& s){hp2::debug::RaceCoordinator(m,rc).Sub_001495B0(s);});rc_complete=true;
  world=m.word(0x3272f0);race=m.word(0x327258);replay=m.word(0x3271a8);player_address=m.word(0x327778);require(world&&race&&replay&&player_address,"Original FreeRun construction did not publish source owners");require(m.word(0x327780)==1,"World debug supports one actual source Player");
  // TASK-0111 PC preset: Q/E are advertised as manual gear controls. Automatic
  // mode reroutes these callbacks into unrecovered Player preview/freeze UI.
  // Invoke the recovered setter rather than bypassing that source branch.
  if(request.minimal_driving&&!request.event){const auto car=m.word(player_address+4);hp2::recovered::RealDriver(m,m.word(car+0x2c)).SetManualTransmission(1);}
  hp2::debug::Player(m,player_address).ChangeCameraMover(1,false);
  hp2::debug::PrepareRealTimestep(m,1.f/60.f);
  update_camera();publish(0);boot_complete=true;phase="ReadyPaused";
 }
 // Port-owned reviewed subset of sound.Update001ACAA8. Original ticker and
 // source gates/order are retained; this does not claim the full sound driver.
 void update_audio(){
  if(hp2::debug::IsSoundRegionLoadingInProgress(m))hp2::not_implemented("0x001AC8F0","World synchronous sound owner reached an unbound asynchronous region request");
  if(!m.word(0x326f40))return;
  const auto now=ticker();if(!(hp2::debug::bGetTickerDifference(m,m.word(0x327f94),now)>8.f))return;
  m.word(0x327f94,now);
  const auto coordinator=m.word(0x327290);if((coordinator&&m.word(coordinator)==6)||m.word(0x3275ac)==3||m.word(0x327f08))return;
  hp2::debug::collision_sound::Update(m);
  hp2::debug::sound3d_oneshot::Update(m);
 }
 void apply_input(){
  // Source finish owns this car's transition to AI. Human input cannot be
  // delivered through its former RealDriver after that source transition.
  if(m.word(player_address+0x4c))return;
  const auto car=m.word(player_address+4),driver=m.word(car+0x2c);require(m.word(car+0x20)==1,"Actual Player RealDriver control mode is required");hp2::recovered::RealDriver real(m,driver);
  if(input.horn!=horn)real.JoyHandler_HonkHorn(0,0,input.horn?1:0);if(input.transmission>=0)real.SetManualTransmission(input.transmission);if(input.shift>0)real.JoyHandler_ShiftUp(0,0,1);if(input.shift<0)real.JoyHandler_ShiftDown(0,0,1);if(input.neutral&&m.word(driver+0x24c)==1)real.SetDesiredGear(1);
  // Original RealDriver primitive scanner inputs, same E-0046-C01 transport as
  // the old laboratory. Game code derives throttle/brake/steering and motion.
  m.scalar(driver+0x25c,input.throttle);m.scalar(driver+0x264,input.brake);m.scalar(driver+0x254,input.steering);m.scalar(driver+0x274,input.handbrake);
 }
 // E-0112-C01: source mover and Camera belong to this exact World/Player.
 void dispatch_camera_input(){
  const auto mover=m.word(player_address+0xc4);
  require(mover&&m.word(mover+8)==1,"World Player driving camera is unavailable");
  hp2::debug::Player player(m,player_address);
  while(camera_cycles){player.RacingJoyHandler(m.signed_word(player_address+8),93,1);--camera_cycles;}
  if(look_back!=applied_look_back){player.RacingJoyHandler(m.signed_word(player_address+8),94,look_back?1:0);applied_look_back=look_back;}
 }
 std::uint32_t main_view(){return eGetView(m,m.signed_word(player_address+0x14));}
 std::uint32_t active_main_mover(){const auto view=main_view(),head=view+0x3c,mover=m.word(head);return mover==head?0:mover;}
 std::uint32_t main_camera(){return m.word(main_view()+0x38);}
 void update_camera(){
  // Original UpdateCameraMovers0015E950 updates only each view's front mover.
  // An attached NIS mover supersedes, rather than also updating, DriveCamera.
  const auto mover=active_main_mover();require(mover,"World main view has no source camera mover");
  if(m.word(mover+8)==1)hp2::debug::DriveCameraMover(m,mover).Update();
  else if(m.word(mover+8)==10)hp2::debug::MaxCameraMover(m,mover).Update();
  else hp2::not_implemented("0x0015E950","active main-view camera mover type is not bound");
  for(auto& hud:hud_gameplay)hud.camera.reset(); // Actual Player camera dispatch supersedes semantic fixture.
 }
 RaceRulesView capture_rules(){
  RaceRulesViewSource source;source.race=race;source.player=player_address;
  source.event=0x312cd0;source.tree_type=request.event->tree_type;
  if(request.original_ui)source.hud_features=m.word(player_address+0x2fc);
  if(const auto manager=m.word(0x328c1c))source.cop_manager=manager;
  return read_race_rules_view(m,source);
 }
 void publish(float dt){
  // Strict reconstruction validates the unsupported skid consumer. TASK-0111
  // minimal mode explicitly disables this presentation; source physics still runs.
  // Never reinterpret the actual clan-owned list as the old synthetic arena.
  if(!request.minimal_driving&&m.word(0x327e60)!=0x327e60)hp2::not_implemented("0x001A88C8","Shared World skid render traversal/publication is not yet recovered; source simulation has not been disabled");
  WorldFrame next;next.ui_texture_bundles={track_entry.resources.at(2),track_entry.resources.at(3)};next.ui_revision=published.ui_revision+1;next.generation=published.generation;next.step=published.step+(dt>0);next.track=request.track;next.reverse=request.reverse;next.dt=dt;next.player_car=m.word(player_address+4);next.player_finished=m.word(player_address+0x4c)!=0;
  next.source_driver_info_count=m.signed_word(0x311f78+0x308);
  for(unsigned i=0;i<4;++i)next.ui_clip_bounds[i]=m.scalar(0x3271e8+4*i);
  capture_menu_actions(next);capture_mirror(next);next.autosave_requested=autosave_requested;
  if(request.event)next.rule_view=(dt>0&&sampled_rules)?sampled_rules:std::optional<RaceRulesView>(capture_rules());
  {const auto mover=active_main_mover();if(mover){const auto camera=main_camera();driving::DrivingCameraFrame value;for(unsigned axis=0;axis<3;++axis){value.position[axis]=m.scalar(camera+0x40+axis*4);value.target[axis]=m.scalar(camera+0x60+axis*4);value.up[axis]=-m.scalar(camera+4+axis*16);}value.fov_bangle=m.half(camera+0x84);value.near_z=m.scalar(camera+0x7c);value.hide_car=m.word(camera+0x1bc)==next.player_car;value.render_dash=m.word(camera+0x1c0)!=0;if(m.word(mover+8)==1){value.pov_index=m.signed_word(mover+0x78);value.lookback_param=m.scalar(mover+0x80);const auto pov=hp2::debug::DriveCameraMover(m,mover).GetPOV(value.pov_index);value.pov_type=pov?m.signed_word(pov):0;}next.camera=value;}}
  const auto count=m.word(world+0x1c);require(count<=14,"Original World car table exceeds its source capacity");
  for(unsigned i=0;i<count;++i){const auto car=m.word(world+0x2c+4*i),driver_info=m.word(car+0x14),driver=m.word(car+0x2c),state=car+0x40;WorldActorFrame actor;actor.address=car;actor.driver_number=m.signed_half(car+0xa);actor.car_index=m.signed_half(car+8);actor.car_id=m.signed_word(driver_info+12);actor.skin_id=m.signed_byte(driver_info+17);const auto info=m.word(0x3278e0)+std::uint32_t(actor.car_id)*0x560;actor.car_code=render_resource_text(m,info);auto& output=actor.state;output.step=next.step;output.dt=dt;output.track_label=track_entry.name;
   actor.visible=((m.word(car+0x514)>>(m.signed_word(player_address+0x14)&31))&1)!=0;const bool nis_override=(m.word(car+0x510)>>(m.signed_word(player_address+0x14)&31))&1;auto body=m.matrix(car+(nis_override?0x4c0:0x90));const auto position=m.vector(car+(nis_override?0x500:0x50));for(unsigned j=0;j<3;++j)body[12+j]=position[j];body[15]=1;output.body=body;
   const auto v=m.vector(car+0x70),av=m.vector(car+0xd0);output.velocity=std::array<float,3>{{v[0],v[1],v[2]}};output.angular_velocity=std::array<float,3>{{av[0],av[1],av[2]}};
   // Port debug preview inherits laboratory blur opacity; tire/brake matrices,
   // phase and angular velocity below are source state, not a recovered wheel Render.
   std::array<driving::WheelSnapshot,4> wheels;for(unsigned wheel=0;wheel<4;++wheel){auto& w=wheels[wheel];w.tire=m.matrix(state+0xb0+wheel*0x40);w.brake=m.matrix(state+0x1b0+wheel*0x40);w.blur=w.brake;w.phase=m.scalar(state+0x2c0+4*wheel);w.angular_velocity=m.scalar(state+0x2b0+4*wheel);w.contact=m.byte(state+0x310+wheel)!=0;w.blur_alpha=std::min(std::fabs(w.angular_velocity)*.02f,1.f)*90.f;}output.wheels=wheels;
   driving::PowertrainSnapshot p;p.rpm=m.scalar(car+0x3d0);p.speed=m.scalar(car+0x3c4);p.forward_speed=m.scalar(car+0x3c0);p.gear=m.signed_word(car+0x3c8);if(const auto sound=m.word(car+0x850))p.horn=driving::horn_activation(m,sound);if(m.word(car+0x20)==1){p.manual_transmission=m.signed_word(driver+0x24c);p.handbrake=m.scalar(driver+0x21c)>0;}output.powertrain=p;output.lights=driving::vehicle_light_state(m,car,track_entry.time_of_day);output.flares=driving::vehicle_flare_state(m,car,track_entry.time_of_day);driving::validate_matrix(*output.body);driving::validate_wheels(*output.wheels);next.actors.push_back(std::move(actor));
  }
  if(next.camera&&next.camera->render_dash){
   if(request.minimal_driving)::port::diagnostics::warn("0x001236E8","Original RenderDashboard GPU binding is not implemented","dashboard presentation disabled by the PC policy; actual source POV, camera and exclusion flags retained");
   else hp2::not_implemented("0x001236E8","Original RenderDashboard GPU binding is required by the selected source POV");
  }
  std::map<std::uint32_t,std::pair<int,int>> origins;
  for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.signed_byte(body+12)==1){const auto spawner=m.word(body+0x128);if(spawner)origins.emplace(body+0x3f0,std::make_pair(m.signed_half(spawner+4),m.signed_half(spawner+6)));}
  for(auto object=m.word(0x328290);object!=0x328290;object=m.word(object)){WorldObjectFrame o;o.address=object;o.model_hash=hp2::debug::WorldObject(m,object).GetNameHash();o.visible=m.word(object+12)!=0;if(o.visible)o.transform=m.matrix(object+0x20);const auto found=origins.find(object);if(found!=origins.end()){o.source_section=found->second.first;o.source_record=found->second.second;o.source_instance=std::uint32_t(o.source_record);}next.objects.push_back(o);}
  for(const auto& base:scenery_baseline){const auto flags=m.half(base.address+0xe);const auto transform=scenery_matrix(m,base.address);if(flags!=base.flags||transform!=base.transform){WorldSceneryFrame row;row.source_section=base.section;row.source_record=base.record;row.flags=flags;if(transform!=base.transform)row.transform=transform;next.scenery_flags.push_back(std::move(row));}}
  auto particles=std::make_shared<track::ParticleSimulation>(track::ParticleResources{},0);particles->import_committed_world(m);next.particles=std::move(particles);
  if(dt>0){audio->flush_audio(pcm,ticker());next.pcm=pcm.render(static_cast<std::size_t>(std::llround(double(m.scalar(0x327ce4))*48000)),48000);}
  next.race_stage=m.signed_word(race);next.elapsed_ticks=m.signed_word(race+0x10);next.lap_limit=m.signed_word(0x311fa4);
  next.player_score=m.signed_word(player_address+0x5e8);
  if(request.original_ui)if(const auto map=m.word(player_address+0x5ec))hp2::debug::TrackMap(m,map).Render(next.hud_geometry);
  if(request.event)for(unsigned i=0;i<32&&m.byte(0x312cd0+0x28+i);++i)next.event_name.push_back(char(m.byte(0x312cd0+0x28+i)));
  {driving::Memory::Temporary text(m,64);hp2::debug::Timer{next.elapsed_ticks}.PrintToString(m,text.address(),0);for(unsigned i=0;i<64&&m.byte(text.address()+i);++i)next.elapsed_text.push_back(char(m.byte(text.address()+i)));}
  if(const auto player=m.word(player_address+4))if(const auto racing=m.word(player+0x1c)){next.rank=m.signed_byte(racing+0xa);next.lap=m.signed_byte(racing+9);next.track_percent=m.scalar(player+0x7f0)*100.f;next.race_progress=m.scalar(racing+0x20);next.finish_reason=m.signed_half(racing+0x12);}
  if(request.original_ui)prepare_source_hud_geometry(m);next.ui_screens=snapshot_visible_flailer(m,0,true,request.original_ui);if(pause_submenus)pause_submenus->annotate(next.ui_screens);next.menu_screen=menus?menus->current_screen():0;next.menu_item=menus?menus->selected_item():0;next.menu_popup=menus&&menus->popup_active();next.menu_paused=m.word(rc)==6;
  next.nis_draws=prepared_nis_draws;
  published=std::move(next);
 }
 void step(float elapsed, WorldAdvanceTiming* timing = nullptr){
  const auto measure=[timing](double WorldAdvanceTiming::* field, auto&& run){
   if(!timing){run();return;}
   const auto started=std::chrono::steady_clock::now();
   run();
   timing->*field+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
  };
  NisServicesScope nis_scope(m,nis_services);
  driving::PcSqrtCarrierScope numerical_model(request.minimal_driving);
  coordinator([this](auto& s){hp2::debug::RaceCoordinator coordinator(m,rc);coordinator.ExecuteQueuedMessages(s);coordinator.SendMessage(3,0,s);});
  if(request.event&&m.word(rc)==7){if(request.original_ui)for(int i=0;i<m.signed_word(0x327780);++i)hp2::debug::Player(m,m.word(0x327778+4*unsigned(i))).UpdateHud(hud_gameplay[unsigned(i)],request.minimal_driving);publish(0);input={};phase="Results";return;}
  hp2::debug::DetermineAllPlayersInputModes(m);dispatch_camera_input();apply_input();
  hp2::debug::PrepareRealTimestep(m,elapsed);hp2::debug::World state(m,world);state.UpdateWorldPaused();if(state.IsWorldPaused())hp2::not_implemented("0x0014E450","World debug reached original paused-World render branch");
  const SnapshotServices snapshots{&routes,&physics};
  measure(&WorldAdvanceTiming::replay_ms,[&]{hp2::recovered::ReplayManager(m,replay).BeginFrame(snapshots);});
  const float dt=state.GetTimestep(physics.ticker);const bool replay_paused=m.word(replay+0x14)!=0;hp2::recovered::PrepareWorldTimestep(m,replay_paused?0.f:dt);
  auto frame=WorldFrameServices{ai,routes,physics,&hardware};frame.acid.ticker=physics.ticker;frame.event_disc_w=request.research_operands->event_disc_w;frame.terrain_disc_w=request.research_operands->terrain_disc_w;frame.camera_shake_w=request.research_operands->camera_shake_w;frame.acid.port_stack_vector_lane=request.research_operands->acid_w;frame.cop_teleport_position_w=request.research_operands->cop_teleport_position_w;frame.car_reset_route_newpos_w=request.research_operands->car_reset_route_newpos_w;frame.car_reset_velocity_w=request.research_operands->car_reset_velocity_w;
  if(!replay_paused)measure(&WorldAdvanceTiming::timestep_ms,[&]{state.DoTimestep(dt,frame);});
  measure(&WorldAdvanceTiming::replay_ms,[&]{hp2::recovered::ReplayManager(m,replay).EndFrame();});
  update_camera();
  if(request.original_ui)prepare_source_hud_tracking(m,world,main_camera());
  if(request.event)for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)throw std::logic_error("Authored event HUD exceeds the original two-player domain");hp2::debug::Player(m,m.word(0x327778+4*unsigned(i))).UpdateHud(hud_gameplay[unsigned(i)],request.minimal_driving);}
  if(request.original_ui){for(int i=0;i<m.signed_word(0x327780);++i)update_source_hud_mirror(m,m.word(0x327778+4*unsigned(i)));}
  if(request.event)sampled_rules=capture_rules(); // Same source HUD sample, before clocks advance.
  update_audio();
  // MainLoop0012F4C8 draws NIS after camera/HUD/sound, before clock advance.
  prepared_nis_draws.clear();
  for(int i=0;i<m.signed_word(0x327780);++i){const auto player=m.word(0x327778+4*unsigned(i)),manager=m.word(player+0xf0);if(manager){auto packets=snapshot_source_nis(m,manager,unsigned(m.signed_word(player_address+0x14)));prepared_nis_draws.insert(prepared_nis_draws.end(),std::make_move_iterator(packets.begin()),std::make_move_iterator(packets.end()));}}
  hp2::recovered::AdvanceRealTime(m);hp2::recovered::AdvanceWorldTime(m);
  if(request.publication_test_fault_after&&published.step+1==*request.publication_test_fault_after)throw std::runtime_error("Injected port World publication test fault after source mutation; not an original game gap");
  measure(&WorldAdvanceTiming::publish_ms,[&]{publish(dt);});
  horn=input.horn;input.shift=0;input.neutral=false;input.transmission=-1;
 }
 void shutdown(){
  NisServicesScope nis_scope(m,nis_services);
  last_stats.source_cleanup_attempted=rc_complete;
  if(rc&&!rc_complete){diagnostic="Partially constructed original RC; native candidate released without claiming source destructor success";return;}
  if(rc_complete){if(request.original_ui)for(int i=0;i<m.signed_word(0x327780);++i)hp2::debug::Player(m,m.word(0x327778+4*unsigned(i))).SetHudFeatures(0,hud_services);coordinator([this](auto& s){hp2::debug::RaceCoordinator(m,rc).Sub_001496C8(3,s);});require(!m.word(0x327780)&&!m.word(0x327258)&&!m.word(0x3272f0)&&!m.word(0x3271a8)&&!m.word(0x3276fc),"Original RC cleanup retained source owners");m.word(0x327290,0);m.word(0x3272a8,0);rc=world=race=replay=player_address=0;rc_complete=false;
   unload_collision_resources(m,collisions);for(auto& car:cars)car->close();hp2::debug::bPListClose(m);track::unload_position_markers(m,markers);hp2::debug::CloseAIPlans(m);planning->close();hp2::debug::bDeleteSlotPool(m,registration_pool);m.word(0x3273a8,0);registration_pool=0;
   if(oneshot_audio_initialized){hp2::debug::sound3d_oneshot::Close(m);oneshot_audio_initialized=false;}
   if(collision_audio_registered){hp2::debug::collision_sound::UnRegister(m);collision_audio_registered=false;}
   audio->stop_resident_samples();audio->flush_audio(pcm,ticker());audio->flush_audio(pcm,ticker());audio->close();if(nis_files)nis_files->close();nis->close();ui->close();speech->shutdown();unload_cop_resources(m,cops);close_scenery_models(m,smokeables);unload_smokeable_resources(m,smokeables);unload_event_resources(m,events);hp2::debug::TopologyTree(m,0x2f0d10).CloseTree();track::unload_topology_resources(m,topology);if(frontend_events){frontend_events->release_race_event();frontend_events->close();}unload_character_resources(m,characters);common->close();language->close();textures->unload_all();emitters->shutdown();hp2::debug::CloseWorldObjects(m);hp2::debug::emEventManagerClose(m);hp2::debug::CloseCollision(m);hp2::debug::CloseContact(m);hp2::debug::CloseCollisionBody(m);hp2::debug::ClosePhysics(m);eCloseSlotPools(m);for(auto pool:{0x3266fcu,0x326708u}){hp2::debug::bDeleteSlotPool(m,m.word(pool));m.word(pool,0);}last_stats.source_cleanup_ok=true;
  }
 }
};
WorldSession::WorldSession():impl_(std::make_unique<Impl>()){}
WorldSession::~WorldSession(){try{close();}catch(const std::exception&){/* Host-owned candidate still releases its native resource storage. */}}
void WorldSession::open(const WorldLaunchRequest& request){close();auto candidate=std::make_unique<Impl>();candidate->request=request;candidate->published.generation=next_generation++;candidate->phase="Loading";try{const auto gap=hp2::guard([&]{candidate->bootstrap();});if(!gap.empty()){candidate->phase="Blocked";candidate->diagnostic=gap;}}catch(const std::exception& error){candidate->phase="Failed";candidate->diagnostic=error.what();}impl_=std::move(candidate);}
void WorldSession::close(){if(!impl_)return;auto old=std::move(impl_);impl_=std::make_unique<Impl>();std::string gap;try{gap=hp2::guard([&]{old->shutdown();});}catch(const std::exception& error){gap=error.what();}impl_->last_stats=old->last_stats;if(!gap.empty())impl_->diagnostic="Source cleanup blocked: "+gap;else if(!old->diagnostic.empty()&&!old->last_stats.source_cleanup_ok)impl_->diagnostic=old->diagnostic;}
void WorldSession::set_input(const driving::Input& input){driving::validate_input(input);impl_->input=input;}
void WorldSession::camera_input(bool look_back,bool cycle){if(!configured()||impl_->phase=="Blocked"||impl_->phase=="Failed"||impl_->phase=="Results"||impl_->awaiting_attempt||impl_->phase=="AwaitingReload")return;impl_->look_back=look_back;if(cycle&&impl_->camera_cycles<16)++impl_->camera_cycles;}
void WorldSession::cycle_camera_pov(){camera_input(impl_->look_back,true);}
bool WorldSession::advance(float elapsed, WorldAdvanceTiming* timing){
 if(timing)*timing={};
 if(impl_->phase!="Running")return false;
 if(!std::isfinite(elapsed)||elapsed<0)throw std::invalid_argument("World elapsed input must be nonnegative finite");
 impl_->pending_elapsed+=elapsed;
 const float minimum=impl_->m.scalar(impl_->m.word(0x327cd4)==0?0x3256d0:0x3256d8);
 if(impl_->pending_elapsed<minimum)return false;
 const float video=static_cast<float>(impl_->pending_elapsed);
 impl_->pending_elapsed=0;
 try{
  const auto gap=hp2::guard([&]{impl_->step(video,timing);});
  if(!gap.empty()){impl_->phase="Blocked";impl_->diagnostic=gap;impl_->input={};return false;}
 }catch(const std::exception& error){
  impl_->phase="Failed";impl_->diagnostic=error.what();impl_->input={};return false;
 }
 return true;
}
bool WorldSession::single_step(float elapsed){if(impl_->phase!="Paused"&&impl_->phase!="ReadyPaused")return false;impl_->phase="Running";const bool committed=advance(elapsed);if(committed&&impl_->phase=="Running")impl_->phase="Paused";return committed;}
void WorldSession::pause(){if(impl_->phase=="Running"){impl_->phase="Paused";impl_->input={};impl_->pending_elapsed=0;}}
void WorldSession::resume(){if(impl_->phase=="Paused"||impl_->phase=="ReadyPaused")impl_->phase="Running";}
void WorldSession::open_pause(int port){
 if(!impl_->request.original_ui||!impl_->menus)throw std::logic_error("Original race menus were not configured");
 if(impl_->phase!="Running")return;
 const auto gap=hp2::guard([&]{impl_->coordinator([&](auto& services){hp2::debug::RaceCoordinator(impl_->m,impl_->rc).SendMessage(5,port,services);});impl_->reconcile_menu();});
 if(!gap.empty()){impl_->phase="Blocked";impl_->diagnostic=gap;impl_->input={};}
}
void WorldSession::menu_input(int event,bool pressed,int port){
 NisServicesScope nis_scope(impl_->m,impl_->nis_services);
 if(!impl_->menus||impl_->phase=="Failed"||impl_->phase=="Blocked"||impl_->awaiting_attempt||impl_->phase=="AwaitingReload")return;
 const bool from_results=impl_->phase=="Results";if(from_results)impl_->retain_result();
 const auto gap=hp2::guard([&]{impl_->menus->input(port,event,pressed);impl_->coordinator([&](auto& services){hp2::debug::RaceCoordinator(impl_->m,impl_->rc).ExecuteQueuedMessages(services);});impl_->reconcile_menu(from_results);});
 if(!gap.empty()){impl_->phase="Blocked";impl_->diagnostic=gap;impl_->input={};}
}
bool WorldSession::advance_ui(float elapsed){
 NisServicesScope nis_scope(impl_->m,impl_->nis_services);
 if(!std::isfinite(elapsed)||elapsed<0)throw std::invalid_argument("UI elapsed time must be finite and nonnegative");
 if(!impl_->menus||!impl_->menus->current_screen()||(impl_->phase!="Paused"&&impl_->phase!="Results"))return false;
 const auto gap=hp2::guard([&]{hp2::debug::PrepareRealTimestep(impl_->m,elapsed);impl_->menus->tick(impl_->m.scalar(0x327ce4));hp2::recovered::AdvanceRealTime(impl_->m);impl_->publish_ui();});
 if(!gap.empty()){impl_->phase="Blocked";impl_->diagnostic=gap;impl_->input={};return false;}return true;
}
void WorldSession::acknowledge_autosave(){
 if(!impl_->autosave_requested||impl_->phase!="Results")throw std::logic_error("No source result autosave is pending");
 const auto gap=hp2::guard([&]{const auto owner=impl_->m.word(0x32847c);if(!owner)throw std::logic_error("Source autosave owner is absent");impl_->result_services.clear_autosave_popup();hp2::debug::EndGameScreens(impl_->m,owner).AutosaveCompletedCallback(impl_->result_services);impl_->autosave_requested=false;impl_->publish_ui();});
 if(!gap.empty()){impl_->phase="Blocked";impl_->diagnostic=gap;}
}
void WorldSession::acknowledge_event_attempt(const ::port::runtime::EventAttemptBaseline& baseline){
 if(!impl_->awaiting_attempt||impl_->phase!="AwaitingAttempt"||!impl_->request.event)throw std::logic_error("No native authored continuation awaits acknowledgement");
 auto next=*impl_->request.event;::port::runtime::apply_event_attempt_baseline(next,baseline);
 std::optional<WorldLaunchRequest> launch;
 if(impl_->attempt_reload){
  if(!impl_->frontend_events)throw std::logic_error("Source continuation event resources absent");
  auto capsule=std::make_shared<EventRuntimeContinuation>(capture_event_runtime_continuation(impl_->m,next,*impl_->frontend_events));
  auto r=impl_->request;r.event=next;r.continuation=std::move(capsule);r.track=r.continuation->track;r.reverse=r.continuation->reverse;r.car_id=r.continuation->car_id;r.skin_id=r.continuation->skin_id;r.car_code.clear();launch=std::move(r);
 }
 impl_->request.event=std::move(next);impl_->continuation_launch=std::move(launch);impl_->awaiting_attempt=false;impl_->previous_settlement.reset();impl_->previous_results.reset();impl_->autosave_requested=false;impl_->input={};impl_->pending_elapsed=0;impl_->look_back=impl_->applied_look_back=false;impl_->camera_cycles=0;
 impl_->phase=impl_->attempt_reload?"AwaitingReload":"Running";impl_->publish_ui();
}
std::optional<WorldLaunchRequest> WorldSession::take_continuation_launch(){auto out=std::move(impl_->continuation_launch);impl_->continuation_launch.reset();return out;}
void WorldSession::restart(){const auto r=impl_->request;open(r);}
const WorldFrame& WorldSession::frame()const{return impl_->published;}
const WorldLaunchRequest& WorldSession::request()const{return impl_->request;}
const std::string& WorldSession::phase()const{return impl_->phase;}
const std::string& WorldSession::diagnostic()const{return impl_->diagnostic;}
bool WorldSession::configured()const{return impl_->boot_complete;}
bool WorldSession::paused()const{return impl_->phase!="Running";}
const driving::Input& WorldSession::controls()const{return impl_->input;}
const WorldActorFrame* WorldSession::player()const{for(const auto& actor:impl_->published.actors)if(actor.address==impl_->published.player_car)return &actor;return nullptr;}
std::vector<driving::SkidQuad> WorldSession::render_skids(const driving::SkidView&)const{
 // Strict frames proved the source list empty. Minimal driving explicitly
 // disables skid presentation with console warnings. Neither synthesizes skids.
 return {};
}
WorldSessionStats WorldSession::stats()const{auto result=impl_->last_stats;const auto f=impl_->m.footprint();result.regions=f.regions;result.payload_bytes=f.payload_bytes;result.owned_allocations=f.owned_allocations;result.audio_commands=impl_->pcm.consumed_commands();return result;}
std::optional<EventSettlement> WorldSession::event_settlement()const{if(impl_->previous_settlement)return impl_->previous_settlement;if(impl_->phase!="Results")return {};return impl_->source_settlement();}
std::optional<::port::runtime::EventSettingsTransfer> WorldSession::current_event_settings()const{if(!impl_->boot_complete||!impl_->request.event)return {};return export_event_settings(impl_->m,*impl_->request.event);}
std::optional<::port::runtime::EventPlayerSettings> WorldSession::current_player_settings()const{if(!impl_->boot_complete||!impl_->player_address)return {};return ::port::runtime::read_debug_event_player_settings(impl_->m,impl_->player_address+0x1c);}
std::optional<SettlementSnapshot> WorldSession::event_results()const{if(impl_->previous_results)return impl_->previous_results;if(!impl_->request.event||impl_->phase!="Results"||!impl_->rc||impl_->m.word(impl_->rc)!=7)return {};return read_settlement(impl_->m,impl_->rc);}
std::optional<::port::runtime::EventScoreProfile> WorldSession::current_event_scores()const{if(!impl_->boot_complete||!impl_->request.event)return {};return export_event_scores(impl_->m,*impl_->request.event);}
std::optional<std::array<std::int32_t,2>> WorldSession::event_record_times_to_beat()const{if(!impl_->boot_complete||!impl_->request.event)return {};return std::array<std::int32_t,2>{{impl_->m.signed_word(0x312224),impl_->m.signed_word(0x312228)}};}
std::vector<std::array<float,3>> WorldSession::route_points()const{
 // Read-only external-driver observation. Original control points are spline
 // controls, not straight road segments. Evaluate the existing recovered XY
 // spline; the third value is PC planning arc distance, not source race state.
 std::vector<std::array<float,3>> points;const auto& paths=impl_->routes.paths;
 if(paths.geometry.paths.empty())return points;
 const auto& spline=paths.geometry.paths.front().spline;
 const auto segments=spline.looping?spline.points.size():spline.points.size()-1;
 float distance=0;std::array<float,2> previous{};
 for(std::size_t i=0;i<=segments*16;++i){const auto xy=hp2::recovered::QuickSpline::GetPoint(spline,float(i)/16.f,0);
  if(i){const float x=xy[0]-previous[0],y=xy[1]-previous[1];distance+=std::sqrt(x*x+y*y);}
  points.push_back({{xy[0],xy[1],distance}});previous=xy;
 }
 return points;
}
std::vector<RoadRoutePoint> WorldSession::road_points()const{
 if(!impl_->boot_complete||!impl_->player_address||impl_->published.player_finished)return {};
 auto& m=impl_->m;const auto car=m.word(impl_->player_address+4);
 const hp2::recovered::Car player(m,car);
 return observe_route_ahead(m,impl_->routes,player.GetRoute(),player.GetRoutePoint()).points;
}
RoadRouteObservation WorldSession::road_course()const{
 if(!impl_->boot_complete||!impl_->player_address||impl_->published.player_finished){RoadRouteObservation out;out.ended=true;return out;}
 auto& m=impl_->m;const hp2::recovered::Car player(m,m.word(impl_->player_address+4));
 return observe_route_course(m,impl_->routes,player.GetRoute(),player.GetRoutePoint());
}
std::optional<std::uint32_t> WorldSession::current_random_state()const{if(!impl_->boot_complete)return {};return impl_->m.word(0x329060);}
}

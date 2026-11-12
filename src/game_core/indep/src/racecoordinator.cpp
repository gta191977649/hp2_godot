#include "game_core/Chopper.hpp"
#include "game_core/indep/src/copmgr.hpp"
// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\racecoordinator.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/indep/inc/RaceCoordinator.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/port/race/settlement.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include <stdexcept>

#include "game_core/port/race/race_construction.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/gameflow.hpp"
namespace hp2::debug {
// Debug00149E80, TASK-0099 E-0099-W34. Source-backed authored grid; fallback
// retains actual warm-up10..100, alternating +/-3 and17-unit row spacing.
void CalculateRacerPositions(::port::driving::Memory& m,int num,std::uint32_t locs,const ::port::track::RouteData& routes){
 const auto hash=hp2::frontend::bStringHash(m.signed_word(0x311f84)==0?"ForwardStartingPosition":"BackwardStartingPosition");
 const int markers=GetNumTrackPositionMarkers(m,hash);bool found=num<=markers;
 if(found)for(int i=0;i<num;++i){bool row=false;for(int index=0;index<markers;++index){const auto marker=GetTrackPositionMarker(m,hash,index);if(m.signed_word(marker+12)==i){row=true;m.word(locs+16*i,m.word(marker+16));m.word(locs+16*i+4,m.word(marker+20));break;}}if(!row){found=false;break;}}
 if(!found){::port::driving::Memory::Temporary tc(m,40);TrackPathCoordinate::Sub_002035A0(m,tc.address());for(float d=10;;d=d+10){TrackPathCoordinate::UpdatePosition(m,tc.address(),routes.paths,d,0);if(!(d<100))break;}for(int i=0;i<num;++i){TrackPathCoordinate::UpdatePosition(m,tc.address(),routes.paths,float(i/2)*17+100,(i&1)?-3.0f:3.0f);m.word(locs+16*i,m.word(tc.address()));m.word(locs+16*i+4,m.word(tc.address()+4));}TrackPathCoordinate::Sub_00203630(m,tc.address(),2);}
 ::port::driving::Memory::Temporary coordinate(m,0x2c0);TopologyCoordinate topology(m,coordinate.address());topology.Sub_00151878();float z=0;for(int i=0;i<num;++i){const auto a=locs+16*i;m.scalar(a+8,z);z=topology.GetElevation({m.scalar(a),m.scalar(a+4),m.scalar(a+8)});m.scalar(a+8,z);}topology.Sub_001518C0(2);
}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/bList.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug00149D78; TASK-0099 E-0099-W34. Actual shared Car construction; missing
// positive sound/controllers/movers remain the actual callees' explicit gaps.
void AddRacingCar(::port::driving::Memory& m,std::uint32_t driver,std::uint32_t start,int control,const ::port::race::CarConstructionServices& services,::port::race::PhysicsConstructionServices& physics){
 if(!services.routes)hp2::not_implemented("0x00149D78","original AddRacingCar requires loaded shared TrackPath resources");
 ::port::driving::Memory::Temporary tc(m,40);TrackPathCoordinate::Sub_002035A0(m,tc.address());TrackPathCoordinate::UpdatePosition(m,tc.address(),services.routes->paths,std::array<float,2>{{m.scalar(start),m.scalar(start+4)}});const auto angle=static_cast<std::uint16_t>(hp2::recovered::bATan(m,m.scalar(tc.address()+0x18),m.scalar(tc.address()+0x1c)));
 const auto car=m.allocate(0x890);hp2::recovered::Car view(m,car);view.Sub_00130B90(driver,start,angle,services);view.SetControlMode(control,*services.routes);::port::race::SharedTopologyContact contact(m,car+0x520);auto bound_physics=physics;bound_physics.contact=&contact;view.SetMovementMode(control==2?m.signed_word(0x326fa0):1,bound_physics);
 const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00149D78","original World owner is not constructed");const auto tail=m.word(world+4);m.word(tail,car);m.word(world+4,car);m.word(car+4,tail);m.word(car,world);
 if(control==2){const auto node=bPNode::Malloc(m),head=world+8,previous=m.word(head+4);m.word(node+8,car);m.word(previous,node);m.word(head+4,node);m.word(node,head);m.word(node+4,previous);}
 TrackPathCoordinate::Sub_00203630(m,tc.address(),2);
}
}

namespace hp2::debug {
// Debug0014A430; E-0099-W34. Forward original CarList deletion; AICarList
// pointer-node lifetime is separate and belongs to the later World destructor.
void RaceCoordinator::CleanupCars(){const auto world=m_.word(0x3272f0);if(!world)hp2::not_implemented("0x0014A430","original World is not constructed");for(auto car=m_.word(world);car!=world;car=m_.word(world)){const auto next=m_.word(car),previous=m_.word(car+4);m_.word(previous,next);m_.word(next+4,previous);hp2::recovered::Car(m_,car).Sub_00130FF8(3);}if(const auto chopper=m_.word(world+0x18)){Chopper(m_,chopper).Sub_001E40D0(3);m_.word(world+0x18,0);}}
}

#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/car_sound_resources.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug0014A128; E-0099-W56. Original rank-first roster scan, actual shared Car
// construction and original AI route dispatch. Independent unknown locals are
// explicit port operands only when the source consumes them.
void RaceCoordinator::InitCars(const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics,const ::port::race::AiDriveTargetServices& ai,const ::port::race::CarSoundResources& tracks,const ::port::race::RaceStartOperands& operands){auto& m=m_;if(!construction.routes||ai.routes!=construction.routes)hp2::not_implemented("0x0014A128","actual shared route owner must bind original grid/Car/AI callees");::port::driving::Memory::Temporary locations(m,8*16);CalculateRacerPositions(m,8,locations.address(),*construction.routes);RaceParameters parameters(m,0x311f78);for(int rank=1;rank<9;++rank){for(int i=0;i<m.signed_word(0x311f78+0x308);++i){const auto driver=parameters.GetDriverInfo(i);if(m.signed_byte(driver)>=1||m.signed_byte(driver+4)!=rank)continue;const auto type=m.word(driver+8);int control;if(type==1)control=1;else if(type==2)control=2;else break;if(!operands.racer_start_w[static_cast<unsigned>(rank-1)])hp2::not_implemented("0x00149D78","original RacerStartLocation fourth lane requires its own explicit port operand");m.word(locations.address()+16*(rank-1)+12,*operands.racer_start_w[static_cast<unsigned>(rank-1)]);AddRacingCar(m,driver,locations.address()+16*(rank-1),control,construction,physics);break;}}
 const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0014A128","actual World is required for original table publication");World(m,world).BuildCarTable();const auto track=tracks.track_info(m.signed_word(0x311f78));const int override_route=m.signed_word(track+0x7c);auto starting=operands.negative_override_starting_nri;if(override_route>=0){if(override_route>=100)hp2::not_implemented("0x0014A128","authored AI override exceeds original100-route table");const auto route=m.word(0x30bb40+4*override_route);if(!route)hp2::not_implemented("0x0014A128","authored positive AI override references an unloaded route");starting=NearestRoutePointInfo{route,route+0x428};}
 for(int index=0;index<m.signed_word(world+0x24)-m.signed_word(world+0x20);++index){const auto car=m.word(world+0x2c+4*(m.signed_word(world+0x20)+index)),controller=m.word(car+0x28);auto table=m.word(controller+8);if(index==0){if(!starting)hp2::not_implemented("0x0014A2B0","original negative-override starting NRI requires its separate explicit port operand");if(!starting->IsValid(m)){auto target=m.word(table+0x4c);auto self=controller+static_cast<std::int32_t>(m.signed_half(table+0x48));if(target!=0x190d48)hp2::not_implemented("0x0014A128","original FindWhereIAm virtual target is not reconstructed");AICarController(m,self).FindWhereIAm(ai);table=m.word(controller+8);target=m.word(table+0x34);self=controller+static_cast<std::int32_t>(m.signed_half(table+0x30));if(target!=0x18e6e8)hp2::not_implemented("0x0014A128","original GetCurrentRoute virtual target is not reconstructed");starting->Route=AICarController(m,self).GetCurrentRoute(ai);table=m.word(controller+8);target=m.word(table+0x3c);self=controller+static_cast<std::int32_t>(m.signed_half(table+0x38));if(target!=0x18e740)hp2::not_implemented("0x0014A128","original GetCurrentRoutePoint virtual target is not reconstructed");starting->Point=AICarController(m,self).GetCurrentRoutePoint(ai);continue;}}
 table=m.word(controller+8);const auto target=m.word(table+0x2c),self=controller+static_cast<std::int32_t>(m.signed_half(table+0x28));if(target!=0x18e650)hp2::not_implemented("0x0014A128","original SetCurrentRoute virtual target is not reconstructed");AICarController(m,self).SetCurrentRoute(starting->Route,starting->Point,ai);}
 int cops=0;for(int i=0;i<m.signed_word(0x311f78+0x308);++i){const auto driver=parameters.GetDriverInfo(i);const int type=m.signed_word(driver+8);if(type==3)World(m,m.word(0x3272f0)).AddTrafficCar(driver,construction,physics,ai);else if(type>2&&type<6){++cops;AddCopCarToRace(m,driver,false,construction,physics);}}
 World(m,world).BuildCarTable();if(cops){const auto chopper=m.allocate(0x3c0);Chopper(m,chopper).Sub_001E4018(*construction.routes);m.word(world+0x18,chopper);if(m.word(0x3121ec))AddCopCarToRace(m,0,true,construction,physics);}}
// Debug00149B28; E-0099-W56. Player creation remains the original caller's
// responsibility. Bind each actual W55 owner to the correct actual World car.
void RaceCoordinator::InitPlayers(){auto& m=m_;const auto world=m.word(0x3272f0);for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x00149B28","original player index exceeds two-owner carrier");const auto player=m.word(0x327778+4*i);const int number=m.signed_word(player+0x10);const auto car=m.word(0x311f78+0x18)?m.word(world+0x2c+4*(m.signed_word(world+0x20)+number)):World(m,world).GetCarByNumber(number);Player(m,player).InitForRace(car);}}
}

// TASK-0099 E-0099-W62. Debug source owner racecoordinator.cpp; authority
//815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/coordinator_construction.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/PauseMenu.hpp"
#include "game_core/ResourceFile.hpp"
#include "game_core/indep/src/soundcollision.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Header inline emitted at0014C8F8 -> original hash overload001459E8 -> pointer00145A40.
void SetFlailerScreenVisibility(::port::driving::Memory& m,std::uint32_t hash,int state){const auto screen=FindFlailerScreen(m,hash);if(screen)FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,state);}
void RaceCoordinator::Sub_001495B0(::port::race::CoordinatorConstructionServices& services){
 auto& m=m_;const auto a=address_;m.word(a+0xc,0);m.word(a+4,a+4);m.word(a,0);m.word(a+0x18,0);m.word(a+0x10,0);m.word(a+8,a+4);
 if(services.memory.free_payload()<0x7d000){const auto resource=FindResourceFile(m,0x2a7798);if(resource)hp2::not_implemented("0x001971E8","named PRERACEANIM resource requires actual asynchronous unload/destruction");(void)services.memory.free_payload();}
 m.word(0x3272a8,bNewSlotPool(m,16,8,0x2a77b0,0x10));for(auto offset:{0x1cu,0x20u,0x24u,0x28u,0x2cu})m.word(a+offset,0);
 const auto pause=m.allocate(0x38);PauseMenu(m,pause).Sub_0016F660();m.word(0x3276fc,pause);m.word(a+0x14,0xffffffff);
 RaceParameters(m,0x311f78).Print();InitForRace(services);(void)services.memory.free_payload();(void)services.memory.largest_admitted(0x10);
 for(auto offset:{0x34u,0x38u,0x78u,0x88u,0x74u,0x70u})m.word(a+offset,0);
}
void RaceCoordinator::InitForRace(::port::race::CoordinatorConstructionServices& s){
 auto& m=m_;RaceParameters parameters(m,0x311f78);
 if(parameters.GetDriverInfoByDriverNumber(0)||m.word(0x311f90)){const auto p=m.allocate(0xaa0);Player(m,p).Sub_00173AB0(0,0);}
 if(parameters.GetDriverInfoByDriverNumber(1)){const auto p=m.allocate(0xaa0);Player(m,p).Sub_00173AB0(1,1);}
 (void)s.tracks.track_info(m.signed_word(0x311f78));
 const auto replay=m.allocate(0x50b4);hp2::recovered::ReplayManager(m,replay).Sub_0013D248();m.word(0x3271a8,replay);
 if(!s.cars.routes||s.world.routes!=s.cars.routes)hp2::not_implemented("0x00149800","actual shared loaded RouteData is required");
 const auto race=m.allocate(0x6b4);Race(m,race).Sub_001473F0(*s.cars.routes);m.word(0x327258,race);
 const auto world=m.allocate(0x1730);World(m,world).Sub_0014CA28(s.world);InitCars(s.cars,s.physics,s.ai,s.tracks,s.start);InitPlayers();
 int stage=4,state;
 if(m.word(0x311f90)){hp2::not_implemented("0x00149800","AI demo branch requires original DisableRenderEggs");}
 else if(m.word(0x311f94))state=9;
 else if(m.word(0x326e9c))state=3;
 else if(!m.word(0x3121ec)&&!FindResourceFile(m,0x2a7798)){stage=3;state=3;}
 else {stage=1;state=1;}
 Race(m,race).Init(stage,*s.cars.routes,s.physics);Race(m,race).SetupGhostCars();SetAIDriverModesFromPlayers(m);collision_sound::Reset(m);s.speech.flush_queue(s.speech.manager());
 m.word(0x327290,address_);ChangeState(state,s);
}
void RaceCoordinator::PostMessage(int msg,int param){
 const auto node=bOMalloc(m_,m_.word(0x3272a8));m_.word(node+8,static_cast<std::uint32_t>(msg));const auto tail=m_.word(address_+8);m_.word(node+0xc,static_cast<std::uint32_t>(param));m_.word(tail,node);m_.word(address_+8,node);m_.word(node,address_+4);m_.word(node+4,tail);
}
void RaceCoordinator::SendMessage(int msg,int param,::port::race::CoordinatorConstructionServices& s){
 const auto node=bOMalloc(m_,m_.word(0x3272a8));m_.word(node+0xc,static_cast<std::uint32_t>(param));const auto tail=m_.word(address_+8),executing=m_.word(address_+0xc);m_.word(tail,node);m_.word(address_+8,node);m_.word(node,address_+4);m_.word(node+8,static_cast<std::uint32_t>(msg));m_.word(node+4,tail);if(!executing)ExecuteQueuedMessages(s);
}
void RaceCoordinator::ExecuteQueuedMessages(::port::race::CoordinatorConstructionServices& s){
 for(auto node=m_.word(address_+4);node!=address_+4;node=m_.word(address_+4)){
  const auto next=m_.word(node),previous=m_.word(node+4);m_.word(previous,next);m_.word(next+4,previous);m_.word(address_+0xc,1);
  const int msg=m_.signed_word(node+8),param=m_.signed_word(node+0xc);if(msg==1)m_.word(address_,static_cast<std::uint32_t>(param));
  const int state=m_.signed_word(address_);if(state<0||state>=10)hp2::not_implemented("0x001493F0","RC state is outside original callback table");
  const auto descriptor=0x2e8468u+8*state;const auto target=m_.word(descriptor+4);const auto self=address_+m_.signed_half(descriptor);RaceCoordinator owner(m_,self);
  if(target==0x14a4a8)owner.RCStateNoneCallback(msg,param,s);else if(target==0x14ac60)owner.RCStateRaceInProgressCallback(msg,param,s);else if(target==0x14b1b8)owner.RCStatePauseCallback(msg,param,s);else if(target==0x14bba8)owner.RCStatePostRaceCallback(msg,param,s);else hp2::not_implemented("0x001493F0","required original RC state callback is not reconstructed");
  if(m_.word(node+8)==2)m_.word(address_,0);
  m_.word(address_+0xc,0);SlotPool(m_,m_.word(0x3272a8)).Free(node);
 }
}
void RaceCoordinator::ChangeState(int state,::port::race::CoordinatorConstructionServices& s){SendMessage(2,m_.signed_word(address_),s);SendMessage(1,state,s);SendMessage(3,0,s);}
// Debug0014A4A8, E-0101-RC01. ChangeGameFlow publishes a source request only;
// its asynchronous resource transition must run after this queue is drained.
int RaceCoordinator::RCStateNoneCallback(int msg,int param,::port::race::CoordinatorConstructionServices& s){
 if(msg>=1&&msg<=3)return 1;
 if(msg==8){
  CleanupAfterRace();InitForRace(s);
  if(m_.word(0x326f40)){
   if(!s.minimal_settlement_presentation)hp2::not_implemented("0x001ACE38","original sound.Mute restoration children are not reconstructed");
   m_.word(0x327f08,0);
   ::port::diagnostics::warn("0x001ACE38","Original sound.Mute restoration children are not implemented","PC settlement policy restores source IsSoundMuted; host audio restoration remains presentation-owned");
  }
  m_.word(0x327f14,m_.word(0x324f3c));return 1;
 }
 if(msg==9){
  if(param)FrontendDatabase::SetupNextRace(m_,0x2f3890);
  ChangeGameFlow(m_,GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC,GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC);return 1;
 }
 if(msg==10){
  if(m_.word(0x2e78e0))hp2::not_implemented("0x0014A4A8","actual DemoDiscManager quit/attract end-reason path is required");
  ChangeGameFlow(m_,GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC,GAMEFLOW_STATE_NONE);return 1;
 }
 return 0;
}
void RaceCoordinator::DisplayCountdownNumber(int number){
 const auto race=m_.word(0x327258);if(!race)hp2::not_implemented("0x0014B040","actual current Race is required");const auto progress=Race(m_,race).GetCountdownProgress();
 if(number==4&&progress<m_.scalar(0x324f44)){const auto sample=GetInGameSample(m_,0x2a7988);if(sample&&!bSample(m_,sample).IsPlaying())bSample(m_,sample).Play(1,0,0);}
 if(progress<=1&&number&&number!=4){SetFlailerScreenVisibility(m_,0x936ace42,0);for(unsigned i=0;i<3;++i){const float minimum=0.25f*i;const auto object=0x93667af3u-i;Animate321GoElement(m_,minimum,minimum+.25f,progress,0x936ace42,object);}}
 else SetFlailerScreenVisibility(m_,0x936ace42,1);
}
int RaceCoordinator::RCStateRaceInProgressCallback(int msg,int param,::port::race::CoordinatorConstructionServices& s){
 if(msg==1){if(m_.word(address_+0x24))hp2::not_implemented("0x0014AC60","actual PreRaceCameraCoordinator destruction");return 1;}if(msg==13)return 1;
 if(msg==2){DisplayCountdownNumber(0);return 1;}
 if(msg==3){const auto race=m_.word(0x327258),replay=m_.word(0x3271a8);DisplayCountdownNumber(Race(m_,race).GetCountdownNumberToDisplay());
  // Stage4's original replay/jukebox transition must not be suppressed.
  const auto beginning=m_.word(replay+0x58);if((beginning==0||beginning==0x7fffffff)&&m_.word(race)==4){const auto memory=s.memory.replay_services();hp2::recovered::ReplayManager(m_,replay).MarkTheBeginningOfTime(memory);(void)s.memory.free_payload();sound::jukebox::DelayedStart(m_,6);}
  if(Race(m_,race).AreAllPlayersFinishedRacing()){
   const auto player=Race(m_,race).GetMostRecentlyFinishedPlayer();
   if(!player)hp2::not_implemented("0x0014AC60","source finished-player pointer is absent");
   if(static_cast<std::int32_t>(m_.word(player+0x4c)+m_.word(player+0x50))<m_.signed_word(0x327d04))TheRaceHasFinished(s);
  }
  if(m_.signed_word(0x327780)>1){
   for(int i=0;i<m_.signed_word(0x327780);++i){
    const auto player=m_.word(0x327778+4*i);const auto finished=m_.word(player+0x4c);const auto until=static_cast<std::int32_t>(finished+m_.word(player+0x50));
    if(finished&&until<m_.signed_word(0x327d04)&&!Race(m_,race).AreAllPlayersFinishedRacing())
     for(int j=0;j<m_.signed_word(0x327780);++j)if(j!=i&&!m_.word(m_.word(0x327778+4*j)+0x4c))hp2::not_implemented("0x0014AC60","actual Player.BeginDemoCameras finished-driver transition");
   }
  }
  return 1;
 }
 if(msg==4){if(Race(m_,m_.word(0x327258)).AreAllPlayersFinishedRacing())TheRaceHasFinished(s);return 1;}
 if(msg==5){m_.word(address_+0x14,static_cast<std::uint32_t>(param));m_.word(address_+0x18,m_.word(address_));ChangeState(6,s);return 1;}
 if(msg==7){ChangeState(5,s);return 1;}if(msg==9){ChangeState(0,s);SendMessage(9,param,s);return 1;}return 0;
}
}

#include "game_core/indep/src/fred.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/sound3dsample.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/indep/inc/EndGameScreens.hpp"
namespace hp2::debug {
// Debug0014BAB0; E-0110-F04. All original scoring/progression runs before the
// presentation boundary. Strict mode retains missing original screen failure.
void RaceCoordinator::TheRaceHasFinished(::port::race::CoordinatorConstructionServices& s){
 if(!s.cars.routes)hp2::not_implemented("0x0014BAB0","original finish requires actual shared TrackPath resources");
 const int continue_type=::port::race::finalize_race_data(m_,*s.cars.routes,s.reward_receipt);
 if(m_.word(0x2e78e0))hp2::not_implemented("0x0012EC50","actual DemoDiscManager playable-complete quit sequence is not reconstructed");
 if(s.end_game_screens){const auto screens=m_.allocate(0x48);EndGameScreens(m_,screens).Sub_001D97C8(continue_type,*s.end_game_screens);m_.word(address_+0x28,screens);ChangeState(7,s);return;}
 if(!s.minimal_settlement_presentation)hp2::not_implemented("0x001D97C8","original EndGameScreens menu services are required");
 // User-authorized PC presentation only: do not allocate a fake original
 // EndGameScreens or enter its untranslated constructor. Native RC POSTRACE
 // value7 is from0014BB94; the host displays copied genuine outcome data.
 ::port::diagnostics::warn("0x001D97C8","Original EndGameScreens presentation is not implemented","source screen construction disabled; PC settlement displays actual native results");
 ChangeState(7,s);
}
// Debug0014BBA8; E-0110-F04. Source screen/highlight presentation is disabled
// before entry only under the explicit minimal policy. Full callback is not
// claimed: ordinary strict mode still fails at its actual missing dependency.
int RaceCoordinator::RCStatePostRaceCallback(int msg,int param,::port::race::CoordinatorConstructionServices& s){
 if(s.end_game_screens){
  const auto screens=m_.word(address_+0x28);if(!screens)hp2::not_implemented("0x0014BBA8","original EndGameScreens owner is absent");
  if(msg==1){
   if(!s.minimal_settlement_presentation)hp2::not_implemented("0x0013E108","original race highlight replay/demo camera services are required");
   ::port::diagnostics::warn("0x0013E108","Original race-highlight replay presentation is not implemented","existing PC optional highlight policy retains completed World; original result menus remain enabled");
   EndGameScreens(m_,screens).ResumeScreens(*s.end_game_screens);return 1;
  }
  if(msg==2){EndGameScreens(m_,screens).SuspendScreens(*s.end_game_screens);if(m_.word(address_+0x1c))hp2::not_implemented("0x0014BBA8","actual postrace DemoCameraCoordinator destruction is required");m_.word(address_+0x1c,0);return 1;}
  if(msg==3||msg==13)return 1;
  if(msg==8||msg==9||msg==10){if(msg==8)FrontendDatabase::TheRaceWasRestarted(m_,0x2f3890);ChangeState(0,s);SendMessage(msg,msg==9?param:0,s);return 1;}
  hp2::not_implemented("0x0014BBA8","source postrace replay message requires highlight replay services");
 }
 if(!s.minimal_settlement_presentation)hp2::not_implemented("0x0014BBA8","original postrace screens, highlight replay and demo-camera dependencies are not reconstructed");
 if(msg==1){
  ::port::diagnostics::warn("0x0013E108","Original race-highlight replay presentation is not implemented","highlight replay and demo cameras disabled; completed source World remains frozen for PC settlement");
  ::port::diagnostics::warn("0x001D99D8","Original EndGameScreens resume presentation is not implemented","source result-screen activation disabled; PC settlement uses actual source result data");
  return 1;
 }
 if(msg==2){if(m_.word(address_+0x1c))hp2::not_implemented("0x0014BBA8","actual postrace DemoCameraCoordinator destruction is required");m_.word(address_+0x1c,0);return 1;}
 if(msg==3||msg==13)return 1;
 if(msg==8||msg==9||msg==10){
  if(msg==8)FrontendDatabase::TheRaceWasRestarted(m_,0x2f3890);
  ChangeState(0,s);SendMessage(msg,msg==9?param:0,s);return 1;
 }
 hp2::not_implemented("0x0014BBA8","requested source postrace message is outside minimal presentation policy");
}
void RaceCoordinator::CleanupPlayers(){for(int i=0;i<m_.signed_word(0x327780);++i)Player(m_,m_.word(0x327778+4*i)).CleanupAfterRace();}
void RaceCoordinator::CleanupAfterRace(){
 for(auto offset:{0x24u,0x20u})if(m_.word(address_+offset))hp2::not_implemented("0x001499F8","active PreRace/FlyBy camera coordinator destruction");
 World(m_,m_.word(0x3272f0)).FlushCollisions();CleanupPlayers();CleanupCars();KillAllActiveEffects(m_,0x2e6ee0);
 if(const auto race=m_.word(0x327258))Race(m_,race).Sub_001474E8(3);
 m_.word(0x327258,0);
 if(const auto world=m_.word(0x3272f0))World(m_,world).Sub_0014CC80(3);
 m_.word(0x3272f0,0);
 sound::jukebox::Stop(m_);sound3d_oneshot::Reset(m_);
 if(const auto replay=m_.word(0x3271a8))hp2::recovered::ReplayManager(m_,replay).Sub_0013D3C0(3);
 m_.word(0x3271a8,0);
 if(const auto screens=m_.word(address_+0x28))EndGameScreens(m_,screens).Sub_001D9848(3);
 m_.word(address_+0x28,0);
 while(m_.signed_word(0x327780)!=0){const auto player=m_.word(0x327778+4*(m_.signed_word(0x327780)-1));if(!player)hp2::not_implemented("0x001499F8","source nonempty Player index tail is null");Player(m_,player).Sub_00174448(3);}
}
void RaceCoordinator::CleanUpAutoReplay(){
 if(const auto camera=m_.word(address_+0x34)){const auto table=m_.word(camera+0x28),target=m_.word(table+0xc),self=camera+static_cast<std::int32_t>(m_.signed_half(table+8));if(target==0x15beb8)DriveCameraMover(m_,self).Sub_0015BEB8(3);else if(target==0x157280)CameraMover(m_,self).Sub_00157280(3);else hp2::not_implemented("0x0014BE48","actual derived auto-replay CameraMover destructor");}m_.word(address_+0x34,0);
 if(const auto data=m_.word(address_+0x38))m_.release(data);
 m_.word(address_+0x38,0);
 if(const auto fov=m_.word(address_+0x78))m_.release(fov);
 m_.word(address_+0x78,0);
 if(m_.word(address_+0x88))hp2::not_implemented("0x00164208","actual auto-replay PreviewRoute destructor");
 m_.word(address_+0x88,0);
 if(const auto bank=m_.word(address_+0x74))m_.release(bank);
 m_.word(address_+0x74,0);
 if(const auto eye=m_.word(address_+0x70))m_.release(eye);
 m_.word(address_+0x70,0);
}
void RaceCoordinator::Sub_001496C8(int flags,::port::race::CoordinatorConstructionServices& s){
 if(m_.word(address_))ChangeState(0,s);
 FrontendDatabase::TheRaceIsLeaving(m_,0x2f3890);CleanupAfterRace();CleanUpAutoReplay();
 if(const auto pause=m_.word(0x3276fc))PauseMenu(m_,pause).Sub_0016F6C0(3);
 m_.word(0x3276fc,0);
 bDeleteSlotPool(m_,m_.word(0x3272a8));
 if(m_.word(address_+4)!=address_+4)hp2::not_implemented("0x0014C888","nonempty source queued-message list after slot-pool destruction");
 if(flags&1)m_.release(address_);
}
}

#include "game_core/port/driving/projections.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0014AEE8, E-0099-W73. Exact low-sample/visible/alpha/scale order.
void Animate321GoElement(::port::driving::Memory& m,float minimum,float maximum,float time,std::uint32_t screen,std::uint32_t name){
 if(!(minimum<time)){FlailerSetVisibility(m,screen,name,0);return;}
 float fraction=1;if(time<maximum){const float divisor=maximum-minimum;if(!std::isfinite(divisor)||divisor==0)hp2::not_implemented("0x0014AF2C","exceptional EE countdown interpolation division");fraction=(time-minimum)/divisor;}
 float amplitude=fraction*6.f;if(1.f<amplitude)amplitude=1.f;
 const auto object=FindFlailerObject(m,screen,name,0);if(object&&!m.byte(object+0x29)){const auto sample=GetInGameSample(m,0x2a7978);if(sample)bSample(m,sample).Play(1,0,0);}
 FlailerSetVisibility(m,screen,name,1);const float alpha=amplitude*128.f;if(!std::isfinite(alpha)||alpha<-2147483648.f||alpha>=2147483648.f)hp2::not_implemented("0x0014AFC4","exceptional EE countdown CVT.W.S operand");FlailerSetAlpha(m,screen,name,::port::driving::cvt_w_s(alpha));FlailerScaleAboutCenter(m,screen,name,(amplitude*-3.f)+4.f);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace port::race {
// E-0110-F04, exact original0014BAB0..0014BB1C data phase. Neither a forced
// finish trigger nor an alternate result algorithm is introduced here.
int finalize_race_data(::port::driving::Memory& m,const ::port::track::RouteData& routes,RewardAwardReceipt* receipt){
 if(receipt)*receipt={};
 const auto race=m.word(0x327258),replay=m.word(0x3271a8);
 if(!race||!replay)hp2::not_implemented("0x0014BAB0","actual current Race and ReplayManager are required");
 hp2::recovered::ReplayManager(m,replay).MarkTheEndOfTime();
 hp2::debug::FrontendDatabase::CheckForAbortion(m,0x2f3890);
 hp2::debug::Race(m,race).SimulateRaceFinish(routes);
 hp2::debug::Race(m,race).NotifyTheRaceIsOver();
 RewardAwardScope awards;
 hp2::debug::FrontendDatabase::TheRaceIsOver(m,0x2f3890,race);
 const auto sealed=awards.finish();if(receipt)*receipt=sealed;
 if(hp2::debug::FrontendDatabase::IsThereAnotherRace(m,0x2f3890))return 1;
 return hp2::debug::FrontendDatabase::IsMultiTrack(m,0x2f3890)?0:2;
}
SettlementSnapshot read_settlement(::port::driving::Memory& m,std::uint32_t coordinator){
 if(m.signed_word(coordinator)!=7)throw std::logic_error("Settlement publication requires actual RC POSTRACE state");
 SettlementSnapshot out;out.aborted=hp2::debug::FrontendDatabase::RaceWasAborted(m,0x2f3890);out.track=m.signed_word(0x311f78);out.race_type=m.signed_word(0x311f98);out.event_type=m.signed_half(0x312cd0+0x6a);
 out.continue_race_type=hp2::debug::FrontendDatabase::IsThereAnotherRace(m,0x2f3890)?1:(hp2::debug::FrontendDatabase::IsMultiTrack(m,0x2f3890)?0:2);
 if(hp2::debug::FrontendDatabase::UseTheRaceEvent(m,0x2f3890))out.event_status=int(hp2::debug::RaceEvent::GetStatus(m,0x312cd0));
 const int count=m.signed_word(0x2f3890+0x388);if(count<0||count>8)throw std::runtime_error("Source transient settlement row count exceeds eight source racers");
 for(int i=0;i<count;++i){const auto row=0x2f3890u+0x388+12+100u*std::uint32_t(i);SettlementRow result;
  result.position=m.signed_word(row);result.driver_number=m.signed_word(row+4);result.finish_reason=m.signed_word(row+8);result.packed_time=m.signed_word(row+0xc);result.packed_best_lap=m.signed_word(row+0x10);result.score=m.signed_word(row+0x14);result.total_points=m.signed_word(row+0x18);result.bonus=m.signed_word(row+0x1c);result.tickets=m.signed_word(row+0x20);
  ::port::driving::Memory::Temporary text(m,64);hp2::debug::Timer{result.packed_time}.PrintToString(m,text.address(),0);for(unsigned c=0;c<64&&m.byte(text.address()+c);++c)result.time_text.push_back(char(m.byte(text.address()+c)));
  out.rows.push_back(result);
 }
 return out;
}
}
namespace hp2::debug {
// Debug00149768; E-0099-W74; replay-frame/00149768.json; SnapshotServices are port operands only.
void RaceCoordinator::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){
 ReplaySnapshot s(m_,snapshot);s.BeginNamedSection(0x2a77d8);World(m_,m_.word(0x3272f0)).DoSnapshot(snapshot,services);
 Race(m_,m_.word(0x327258)).DoSnapshot(snapshot);
 for(int i=0;i<m_.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x00149768","Player snapshot index exceeds original two players");Player(m_,m_.word(0x327778+4*i)).DoSnapshot(snapshot);}
 s.EndNamedSection();
}
}

#include "game_core/port/race/pause_services.hpp"
namespace hp2::debug {
// Debug0014B1B8; E-0112-P02, reverse/task0112/pause-integration.md.
int RaceCoordinator::RCStatePauseCallback(int msg,int param,::port::race::CoordinatorConstructionServices& s){
 if(!s.pause)hp2::not_implemented("0x0014B1B8","original pause screen/device services are absent");
 const auto& svc=*s.pause;auto& m=m_;const auto pause=m.word(0x3276fc);
 if(!pause)hp2::not_implemented("0x0014B1B8","original PauseMenu owner is absent");
 const auto change=[&](std::uint32_t name,int initial){if(!svc.change_screen)hp2::not_implemented("0x0014B1B8","original ChangeToScreen service is absent");svc.change_screen(name,initial);};
 const auto mute=[&](bool value){if(!svc.mute)hp2::not_implemented("0x001ACE38","original sound.Mute service is absent");svc.mute(value);};
 if(msg==2){
  change(0,0);PauseMenu(m,pause).ExitPause();
  // TellPlayersToUnPause00180340 -> Player.EndPause00180230.
  if(!svc.joystick_type)hp2::not_implemented("0x00180230","original GetJoystickType device service is absent");
  for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x00180340","original two-player index limit");const auto player=m.word(0x327778+4*i);m.word(player+0xc,std::uint32_t(std::int32_t(std::int8_t(svc.joystick_type(m.signed_word(player+8))))));}
  // World.EndPause0014D360 is instruction-proven empty.
 }else if(msg<3){
  if(msg!=1)return 0;
  // World.BeginPause0014D358 is instruction-proven empty.
  // TellPlayersToPause001802E0 -> Player.BeginPause00180228's inlined Clear.
  for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x001802E0","original two-player index limit");Player(m,m.word(0x327778+4*i)).ClearHudAnimations();}
  PauseMenu(m,pause).EnterPause(m.signed_word(address_+0x14),svc);change(0x2a7998,1);mute(true);
 }else if(msg!=3){
  if(msg!=6)return 0;
  if(param==1){mute(false);ChangeState(m.signed_word(address_+0x18),s);}
  else if(param==4)ChangeState(5,s);
  else if(param==3){FrontendDatabase::TheRaceWasQuit(m,0x2f3890);if(m.word(0x2e78e0))hp2::not_implemented("0x0014B1B8","actual DemoDiscManager playable quit end reason");ChangeState(0,s);SendMessage(10,0,s);}
  else if(param==2){ChangeState(0,s);SendMessage(8,0,s);}
 }
 return 1;
}
}

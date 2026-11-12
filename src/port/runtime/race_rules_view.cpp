// TASK-0101 E-0101-FR01. See reverse/task0101/race-ui-rule-evidence.md.
#include "race_rules_view.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstring>
#include <limits>
namespace port::race {
namespace {
bool timer_set(std::int32_t value){return value!=0&&value!=0x7fffffff;}
std::int32_t difference(std::uint32_t first,std::uint32_t second){const std::uint32_t bits=first-second;std::int32_t result;std::memcpy(&result,&bits,4);return result;}
std::string text(const ::port::driving::Memory& m,std::uint32_t at,unsigned size){std::string out;for(unsigned i=0;i<size&&m.byte(at+i);++i)out.push_back(char(m.byte(at+i)));return out;}
std::string time_text(::port::driving::Memory& m,std::int32_t ticks){::port::driving::Memory::Temporary buffer(m,64);hp2::debug::Timer{ticks}.PrintToString(m,buffer.address(),0);return text(m,buffer.address(),64);}
}
RaceRulesView read_race_rules_view(::port::driving::Memory& m,const RaceRulesViewSource& source){
    using namespace hp2::debug;
    if(!source.race||!source.player||m.word(0x327258)!=source.race)
        hp2::not_implemented("0x0017D738","Rule view requires the actual current Race and Player owners");
    const auto race=source.race,player=source.player,car=m.word(player+4);
    if(!car||!m.word(car+0x1c))hp2::not_implemented("0x0017D738","Rule view requires actual Player Car and RacingCar");
    const auto row=m.word(car+0x1c);
    RaceRulesView out;out.tree_type=source.tree_type;out.race_type=m.signed_word(0x311f98);out.nfs_race_mode=m.signed_word(0x311f78+0x254);
    out.challenge_race_mode=m.signed_word(0x2f3890+0x10);out.selected_language=m.signed_word(0x2f3890+0x170);out.loaded_language=m.signed_word(0x313898);
    if(out.race_type<0||out.race_type>5)hp2::not_implemented("0x001FA378","Unknown original RaceTypes value in rule view");
    out.objective_text=text(m,0x311f78+0x54,512);
    auto& hud=out.hud;hud.hud_features=source.hud_features;
    hud.race_stage=m.signed_word(race);hud.countdown_number=Race(m,race).GetCountdownNumberToDisplay();hud.countdown_progress=Race(m,race).GetCountdownProgress();
    hud.rank=m.signed_byte(row+0xa);hud.participant_count=m.signed_word(race+0x18);hud.finish_reason=m.signed_half(row+0x12);
    hud.completed_laps=m.signed_byte(row+9);hud.lap_limit=m.signed_word(race+0x644);hud.display_lap=hud.completed_laps+1;
    hud.point_to_point=m.word(0x311fa0)!=0;hud.score=m.signed_word(player+0x5e8);
    hud.race_elapsed_ticks=m.signed_word(race+0x10);hud.sample_world_ticks=m.signed_word(0x327cf8);
    // Source RenderHud0017E720..770: P2P display differs from raw track percent.
    if(hud.point_to_point){const int boundary=m.signed_byte(race+0x652);if(boundary<0||boundary>=5)hp2::not_implemented("0x0017E720","Unbound source start-line boundary index");
        const float start=Race(m,race).GetLapNumberExact(m.scalar(race+0x654+4*unsigned(boundary))),denominator=1.f-start;
        if(!std::isfinite(start)||denominator==0)hp2::not_implemented("0x0017E760","Unreviewed source P2P progress division domain");
        float progress=(m.scalar(row+0x14)-start)/denominator;if(!std::isfinite(progress))hp2::not_implemented("0x0017E760","Unreviewed source P2P progress operands");
        if(progress<0)progress=0;if(progress>1)progress=1;hud.progress_percent=progress*100.f;
    }else hud.progress_percent=m.scalar(car+0x7f0)*100.f;
    // Source147348 uses START boundary652; finish/lap helpers use653.
    const int start_boundary=m.signed_byte(race+0x652);if(start_boundary<0||start_boundary>=5)hp2::not_implemented("0x00147348","Unbound source HUD start-line timer index");
    const auto crossing=m.word(row+0x24+4*unsigned(start_boundary));
    std::int32_t displayed=0;
    if(timer_set(static_cast<std::int32_t>(crossing))){const auto elapsed=difference(m.word(0x327cf8),crossing);hud.lap_elapsed_ticks=elapsed;displayed=elapsed;}
    const auto target=m.signed_word(0x311f9c);if(timer_set(target))hud.target_ticks=target;
    if(hud.lap_elapsed_ticks&&timer_set(*hud.lap_elapsed_ticks)&&hud.target_ticks){
        auto measured=*hud.lap_elapsed_ticks;const auto last=m.signed_word(RacingCar(m,row).GetLastLapTime());
        if(float(measured)*m.scalar(0x325388)<3.f&&timer_set(last))measured=last;
        displayed=difference(std::uint32_t(target),std::uint32_t(measured));hud.clock_signed_delta_ticks=displayed;hud.clock_target_applied=true;hud.clock_overrun=displayed<0;
        if(displayed==std::numeric_limits<std::int32_t>::min())hp2::not_implemented("0x0017DF38","HUD signed timer negation outside reviewed native projection");
        if(displayed<0)displayed=-displayed;
    }
    hud.clock_display_ticks=displayed;hud.clock_text=time_text(m,displayed);hud.race_elapsed_text=time_text(m,hud.race_elapsed_ticks);
    if(source.event){const auto event=*source.event;if(!event||!m.word(event+4))hp2::not_implemented("0x001FD6C8","Rule view lacks bound native event state");
        const int type=m.signed_half(event+0x6a);if(type<0||type>6)hp2::not_implemented("0x001FD598","Unknown native EventType in rule view");
        out.event_type=type;out.event_number=m.signed_half(event+0x68);out.event_name=text(m,event+0x28,32);
        out.objective_type=m.half(event+0x7c);out.objective_data=m.word(event+0x80);
        out.event_status=int(RaceEvent::GetStatus(m,event));out.old_event_status=int(RaceEvent::GetOldStatus(m,event));out.aborted=RaceEvent::RaceWasAborted(m,event);
        out.stage_index=RaceEvent::GetRaceIndex(m,event);out.stage_count=RaceEvent::GetRaceCount(m,event);out.has_next_race=RaceEvent::IsThereAnotherRace(m,event);out.result_should_be_discarded=RaceEvent::ResultShouldBeDiscarded(m,event);
        if(type==3||type==4){const int count=m.signed_half(event+0x76)+m.signed_half(event+0x74);if(count<0||count>8)hp2::not_implemented("0x001FC6D8","Original event standings exceed eight source racers");const auto state=m.word(event+4);
            for(int i=0;i<count;++i){RaceStageStanding standing;standing.racer_index=i;standing.driver_number=RaceEvent::DriverNumberFromRacerIndex(m,event,i);
                if(type==3)standing.recorded_rank=EventKoState::GetRank(m,state,i);else{standing.points=m.byte(state+unsigned(i));standing.standing_rank=int(EventTournamentState::RacerRank(m,state,i));}out.standings.push_back(standing);}
        }
    }
    if(source.cop_manager&&*source.cop_manager){const auto manager=*source.cop_manager;PoliceHudView police;const auto pursuit=tCopManager(m,manager).IsBeingPursued(car);police.being_pursued=pursuit!=0;
        // GetPursuitLevelForHUD221CC0 returns zero for this confirmed no-pursuit arm.
        if(pursuit){police.hud_level=m.signed_word(pursuit+0x5d4);police.pursuit_state=m.signed_word(pursuit+0x5cc);police.pursuit_substate=m.signed_word(pursuit+0x5d0);}
        const int index=m.signed_half(car+8);if(index<0||index>=8)hp2::not_implemented("0x00222DA8","Rule view ticket row exceeds actual eight CarPullovers");
        police.tickets=tCopManager(m,manager).GetNumTickets(car);police.ticket_limit=m.signed_word(0x311f78+0x27c);police.be_a_cop=m.word(0x3121ec)!=0;
        if(police.be_a_cop){const float time=m.scalar(manager+0x28f0);if(!std::isfinite(time))hp2::not_implemented("0x00223698","Unknown Be A Cop clock numeric domain");police.be_a_cop_seconds=time;police.arrests=m.signed_word(manager+0x2900);police.arrest_quota=hud.participant_count-1;police.roadblocks_used=m.signed_word(manager);police.choppers_used=m.signed_word(manager+0x28fc);}
        out.police=police;
    }
    return out;
}
}

// TASK-0099 E-0099-W70. Exact Debug mdebug definition/TU owner
// ../indep/src/speech/speechfn.cpp. Authority SHA256
//815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/race_speech.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/inc/Fred.hpp"
namespace hp2::debug {
bool WeDontWantToSayAnything(::port::driving::Memory& m,std::uint32_t car){
 if(!m.word(car+0x18)||!m.word(0x311fa8))return true;
 const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x00201930","actual current Race is required");
 if(m.signed_word(race+0x18)<2)return true;
 const auto coordinator=m.word(0x327290);
 if(coordinator){const auto state=m.word(coordinator);if(state==4||state==7||state==5||state==9)return true;}
 if(m.word(0x311f98)!=2){if(!coordinator)hp2::not_implemented("0x00201930","source non-time-trial branch dereferences actual RaceCoordinator");if(m.word(coordinator)-8u<2)return true;if(m.word(0x311f98)!=0)return false;}
 return true;
}
// Debug00201910,E-0110-F17. This consumes the actual shared bRandom seed.
bool FlipCoinNoReplay(::port::driving::Memory& m){return hp2::recovered::bRandom(m,2)==0;}
// Debug00201D08,E-0110-F17. Original four-word DuringRaceSpeech and LUT
// selection. Keep both random draws when rank1/8 chooses a place announcement.
void OnePlayerLapSpeech(::port::driving::Memory& m,std::uint32_t speech,std::uint32_t car,int laps){
 if(laps==m.signed_word(0x311fa4)-1){m.word(speech+12,0x400);return;}
 const bool coin=FlipCoinNoReplay(m);
 if((coin||laps>19)&&m.signed_word(0x311fac)>0){const int rank=m.signed_byte(m.word(car+0x1c)+0xa);if(rank==1)m.word(speech+12,FlipCoinNoReplay(m)?1:0x200);else if(rank==8)m.word(speech+12,FlipCoinNoReplay(m)?0x80:0x100);else m.word(speech+12,m.word(0x313140+4u*std::uint32_t(rank-2)));}
 else m.word(speech,m.word(0x313158+4u*std::uint32_t(laps-1)));
}
// Debug00201F80,E-0110-F17. Confirmed original JR RA/zero-return leaf.
bool LapKnockoutSpeech(::port::driving::Memory&,std::uint32_t,int){return false;}
// Debug00202058,E-0110-F17. Reads original timing/highscore only; no lap,
// record, points or rank mutation occurs in this announcement selector.
bool LapRecordSpeech(::port::driving::Memory& m,std::uint32_t car,bool disabled){const auto row=m.word(car+0x1c),race=m.word(0x327258);const auto lap=m.signed_word(RacingCar(m,row).GetLastLapTime());const auto table=FrontendDatabase::GetBestLapTimesTable(m,0x2f3890,m.signed_word(0x311f78),m.signed_byte(m.word(car+0xc)+0xe6),m.signed_word(0x311f84));const auto best=m.signed_word(table);if(lap!=m.signed_word(race+0x14)||!(lap<best)||best==0||best==0x7fffffff)return false;
 if(!disabled)hp2::not_implemented("0x00289918","Original SPCH_AddEventV lap-record announcement queue/playback is not reconstructed");
 ::port::diagnostics::warn("0x00289918","Original lap-record SPCH announcement is not implemented","audio eventA4 submission disabled before entry; actual timing/highscores and shared RNG remain source-owned");return true;}
// Debug00202108,E-0110-F17. Selection and global RNG remain mandatory under
// the explicit PC audio policy; only actual SPCH_AddEventV submission is disabled.
void LapFinishedSpeech(::port::driving::Memory& m,std::uint32_t car,int laps,bool disabled){::port::driving::Memory::Temporary local(m,16);const auto speech=local.address();for(unsigned i=0;i<4;++i)m.word(speech+4*i,0);if(WeDontWantToSayAnything(m,car))return;if(LapRecordSpeech(m,car,disabled))return;if(m.word(0x311f98)==3&&LapKnockoutSpeech(m,car,laps))return;
 if(m.word(0x311fa8)==2)hp2::not_implemented("0x00201E28","Original two-player lap speech selection is outside the single-player reconstruction scope");
 OnePlayerLapSpeech(m,speech,car,laps);
 if(!disabled)hp2::not_implemented("0x00289918","Original SPCH_AddEventV lap announcement queue/playback is not reconstructed");
 ::port::diagnostics::warn("0x00289918","Original lap SPCH announcement is not implemented","audio eventA0 submission disabled before entry; actual lap data, selection and shared RNG remain source-owned");
}
void SplitTimeSpeech(::port::driving::Memory&,std::uint32_t){}
}


#include "game_core/indep/src/speech/conversions.hpp"
#include "game_core/port/race/end_game_screens.hpp"
#include <sstream>
namespace port::race::speech_detail {
// Port device boundary for original SPCH_AddEventV00289918. Every original
// selection/read/write/RNG draw above the submit point has already occurred.
static void result_submit(const ResultSpeechServices& s,int event,std::vector<int> arguments){if(s.submit){s.submit(event,arguments);return;}if(!s.defer_submission)hp2::not_implemented("0x00289918","original result SPCH_AddEventV device queue is not bound");std::ostringstream description;description<<"audio event0x"<<std::hex<<event<<" submission deferred at original SPCH device boundary; source speech selection/state/RNG retained";::port::diagnostics::warn("0x00289918","Original result SPCH audio event queue/playback is not implemented",description.str().c_str());}
// Debug002026D8; E-0112-R08. Source TheRaceParameters.NumPlayerCars predicate.
static int postgame_player_flag(const driving::Memory& m,int player,int single,int first,int second){return m.word(0x311fa8)==2?(player==0?first:second):single;}
// Debug00201A00; E-0112-R08. Source transient row reason2 alone means busted.
static bool busted(const driving::Memory& m,std::uint32_t row){return m.word(row+8)==2;}
}
namespace hp2::debug {
// Debug002023F0; E-0112-R08. Original car/HP conversion before SPCH boundary.
bool UnlockCarSpeech(::port::driving::Memory& m,int car,const ::port::race::ResultSpeechServices& s){const auto info=m.word(0x3278e0)+0x560*std::uint32_t(car);int event,flag;if(m.word(info+0x538)==1){flag=Speech::ConvertCarTypeToUnlockSpeech(m,car);if(!flag)return false;event=0x96;}else if(!m.byte(info+0xe4)){flag=Speech::ConvertCarTypeToUnlockSpeech(m,car);if(!flag)return false;::port::race::speech_detail::result_submit(s,0x93,{flag});return true;}else{flag=Speech::ConvertCarTypeToUnlockSpeech(m,car);event=0xcc;if(flag==0x80)flag=1;else{event=0x95;if(!flag)return false;}}::port::race::speech_detail::result_submit(s,event,{flag});return true;}
// Debug002024B8; E-0112-R08. Source direction2 only for actual reverse value1.
bool UnlockTrackSpeech(::port::driving::Memory&,int track,int direction,const ::port::race::ResultSpeechServices& s){const int flag=Speech::ConvertTrackNumberToUnlockSpeech(track);if(flag)::port::race::speech_detail::result_submit(s,0x97,{flag,direction==1?2:1});return flag!=0;}
// Debug00201BD8; E-0112-R08. Original fullword congratulations store and flags.
void RaceEventSpeech(::port::driving::Memory& m,std::uint32_t player,const ::port::race::ResultSpeechServices& s){if(!player)return;if(RaceEvent::GetStatus(m,0x312cd0)==eEVENT_PASSED){const bool congratulations=m.word(player+0xa6c)==0;if(congratulations)m.word(player+0xa6c,1);int first=0,last=0;const int event=m.signed_half(0x312cd0+0x68);if(std::uint32_t(event-1)<20)first=Speech::ConvertEventNumber1To20(m,event);else last=Speech::ConvertEventNumber21ToFinal(m,event);const bool superbonus=m.word(0x312cd0+0x78)!=0;const int id=last==0x800?0xd8:m.word(0x2f3890+0x1072c)==FRED_HOTPURSUIT_GAME?0xd8:0xd9;::port::race::speech_detail::result_submit(s,id,{int(congratulations),first,last==0x800?0x800:last,int(superbonus)});}else ::port::race::speech_detail::result_submit(s,0xa8,{0x40});}
// Debug002023B8; E-0112-R08. Source player argument is intentionally unused.
void RewardScreenSpeech(::port::driving::Memory& m,std::uint32_t,bool succeeded,std::uint32_t reward,const ::port::race::ResultSpeechServices& s){if(!succeeded||!reward)return;if(m.byte(reward))UnlockTrackSpeech(m,m.byte(reward+1),m.byte(reward+2),s);else UnlockCarSpeech(m,m.byte(reward+1),s);}
// Debug00202208; E-0112-R08. Busted/event-status gates precede any submission.
void PostGameLapRecord(::port::driving::Memory& m,int player,int place,std::uint32_t row,const ::port::race::ResultSpeechServices& s){if(::port::race::speech_detail::busted(m,row))return;if(m.word(0x311f78+600)&&RaceEvent::GetStatus(m,0x312cd0)!=eEVENT_PASSED)return;const int flag=place==0?::port::race::speech_detail::postgame_player_flag(m,player,512,1024,2048) : ::port::race::speech_detail::postgame_player_flag(m,player,64,128,256);::port::race::speech_detail::result_submit(s,0xa6,{flag});}
// Debug002022A0; E-0112-R08. Preserve exactly one shared FlipCoinNoReplay draw
// for an eligible first-place total-time record; deferred audio never skips it.
void PostGameTimeRecord(::port::driving::Memory& m,int player,int place,std::uint32_t row,const ::port::race::ResultSpeechServices& s){if(::port::race::speech_detail::busted(m,row))return;if(m.word(0x311f78+600)&&RaceEvent::GetStatus(m,0x312cd0)!=eEVENT_PASSED)return;int flag;if(place==0){if(FlipCoinNoReplay(m))flag=::port::race::speech_detail::postgame_player_flag(m,player,1,2,4);else flag=::port::race::speech_detail::postgame_player_flag(m,player,8,16,32);}else flag=::port::race::speech_detail::postgame_player_flag(m,player,64,128,256);::port::race::speech_detail::result_submit(s,0xa6,{flag});}
// Debug00202360; E-0112-R08. Actual source TournamentLUT3132E8.
void TournamentRaceSpeech(::port::driving::Memory& m,bool done,int place,const ::port::race::ResultSpeechServices& s){if(done){if(place<0||place>=8)hp2::not_implemented("0x00202360","source tournament speech place outside eight-row LUT");::port::race::speech_detail::result_submit(s,0xa7,{m.signed_word(0x3132e8+4*unsigned(place))});}else if(place==0)::port::race::speech_detail::result_submit(s,0xa7,{0x4000});}
}
namespace port::race {
// Explicit PC adapter; original pre-submit behavior remains in speechfn.cpp.
void bind_source_result_speech_services(driving::Memory& m,EndGameScreenServices& destination,bool defer){const ResultSpeechServices services{defer,{}};destination.unlock_car_speech=[&m,services](int car){(void)hp2::debug::UnlockCarSpeech(m,car,services);};destination.race_event_speech=[&m,services](std::uint32_t player){hp2::debug::RaceEventSpeech(m,player,services);};destination.reward_screen_speech=[&m,services](std::uint32_t player,bool passed,std::uint32_t reward){hp2::debug::RewardScreenSpeech(m,player,passed,reward,services);};destination.post_lap_record_speech=[&m,services](int player,int place,std::uint32_t row){hp2::debug::PostGameLapRecord(m,player,place,row,services);};destination.post_time_record_speech=[&m,services](int player,int place,std::uint32_t row){hp2::debug::PostGameTimeRecord(m,player,place,row,services);};destination.tournament_speech=[&m,services](bool done,int place){hp2::debug::TournamentRaceSpeech(m,done,place,services);};}
}

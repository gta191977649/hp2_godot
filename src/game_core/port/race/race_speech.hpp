#pragma once
// Port declaration surface for original speech/speechfn.cpp Memory callees.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <vector>
namespace port::race {
struct EndGameScreenServices;
struct ResultSpeechServices {
 bool defer_submission=false;
 std::function<void(int,const std::vector<int>&)> submit;
};
void bind_source_result_speech_services(driving::Memory&,EndGameScreenServices&,bool defer_submission);
}
namespace hp2::debug {
bool WeDontWantToSayAnything(::port::driving::Memory&,std::uint32_t car); //00201930,W70.
bool FlipCoinNoReplay(::port::driving::Memory&); //00201910,E-0110-F17.
void OnePlayerLapSpeech(::port::driving::Memory&,std::uint32_t speech,std::uint32_t car,int laps); //00201D08.
bool LapKnockoutSpeech(::port::driving::Memory&,std::uint32_t,int); //00201F80.
bool LapRecordSpeech(::port::driving::Memory&,std::uint32_t,bool disable_announcement=false); //00202058.
void LapFinishedSpeech(::port::driving::Memory&,std::uint32_t car,int laps,bool disable_announcement=false); //00202108.
void SplitTimeSpeech(::port::driving::Memory&,std::uint32_t car); //002021D8, empty.
bool UnlockCarSpeech(::port::driving::Memory&,int,const ::port::race::ResultSpeechServices&); //002023F0,E-0112-R08.
bool UnlockTrackSpeech(::port::driving::Memory&,int,int,const ::port::race::ResultSpeechServices&); //002024B8.
void RaceEventSpeech(::port::driving::Memory&,std::uint32_t,const ::port::race::ResultSpeechServices&); //00201BD8.
void RewardScreenSpeech(::port::driving::Memory&,std::uint32_t,bool,std::uint32_t,const ::port::race::ResultSpeechServices&); //002023B8.
void PostGameLapRecord(::port::driving::Memory&,int,int,std::uint32_t,const ::port::race::ResultSpeechServices&); //00202208.
void PostGameTimeRecord(::port::driving::Memory&,int,int,std::uint32_t,const ::port::race::ResultSpeechServices&); //002022A0.
void TournamentRaceSpeech(::port::driving::Memory&,bool,int,const ::port::race::ResultSpeechServices&); //00202360.
}

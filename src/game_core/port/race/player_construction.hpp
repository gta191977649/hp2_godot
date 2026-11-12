#pragma once
// Port declarations for original Player-owned constructor dependencies.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
int LoaderNisInfo(::port::driving::Memory&,std::uint32_t); //001B07E8 E-0099-W55.
int UnloaderNisInfo(::port::driving::Memory&,std::uint32_t); //001B0830.
int GetNumNisInfo(::port::driving::Memory&,int type); //001B0920.
int NisSupportsPlayerBodyType(::port::driving::Memory&,int,std::uint32_t); //001B0858,E-0100-PA01.
int NisSupportsOtherBodyType(::port::driving::Memory&,int,std::uint32_t); //001B08B8,E-0100-PA01.
int IsNisSuitable(::port::driving::Memory&,int,std::uint32_t,std::uint32_t,int); //001B0A38,E-0100-PA01.
std::uint32_t GetNisInfo(::port::driving::Memory&,int number); //001B0A98.
std::uint32_t GetNisInfo(::port::driving::Memory&,int index,int type); //001B0AC8.
class NISOverlayManager:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001809A8(); //E-0099-W51, explicit original construction.
 void Sub_00180A88(int); //Original destruction.
 void Start();void StartTicket();void StartArrest();void Finish(); //00180AE0/00181850/00181840/00180F88.
 void UpdateDateAndTime(); //00180D70, actual host RTC service.

};
}
namespace port::race {
// Owns full authored37010 chunks; no host struct layout or synthetic NIS rows.
class NisInfoResources {
 ::port::driving::Memory& memory_;std::vector<std::uint32_t> chunks_;
public:
 explicit NisInfoResources(::port::driving::Memory& memory):memory_(memory){}
 void load_bundle(const std::vector<std::uint8_t>&);
 void close();
};
}

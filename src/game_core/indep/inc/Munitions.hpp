#pragma once
// TASK-0099 W79. Original inline definition marker ../indep/inc/Munitions.hpp.
// Logical Memory-view declarations remain port bindings, no native original ABI.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/port/race/snapshot_services.hpp"
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
// Exact Debug SmokeableType names/values, E-0099-W09.
enum SmokeableType {ST_BASIC_SMOKEABLE=0,ST_ROCKET=1,ST_SMOKEBOMB=2,
    ST_DROPPED_SPIKES=3,ST_NUM_SMOKEABLE_TYPES=4};
class SmokeBomb :public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
    SmokeableType GetSmokeType()const{return ST_SMOKEBOMB;} //001BCF28 original inline, W79.
    void Init();
    void Sub_001BBD40(int flags); //E-0099-W79.
    // Debug001BBDB0, E-0099-W09; pool, creator, required ROCKETEXPLD group.
    static void InitSmokeBomb(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
    // Debug001BBE08, E-0099-W12; actual pool and copied-emitter cleanup.
    static void FiniSmokeBomb(::port::driving::Memory&);
};
class Rocket :public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
    SmokeableType GetSmokeType()const{return ST_ROCKET;} //001BCE28 original inline, W79.
    void DoSnapshot(std::uint32_t,const ::port::race::SnapshotServices& ={});
    void Init();
    void Sub_001BC4F0(int flags); //E-0099-W79.
    // Debug001BC560, E-0099-W09.
    static void InitRocket(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
    // Debug001BC5B8, E-0099-W12.
    static void FiniRocket(::port::driving::Memory&);
};
class DroppedSpikes :public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
    SmokeableType GetSmokeType()const{return ST_DROPPED_SPIKES;} //001BCEA8 original inline, W79.
    void DoSnapshot(std::uint32_t,const ::port::race::SnapshotServices& ={});
    void Init();
    void Sub_001BC038(int flags); //E-0099-W79.
    // Debug001BC108, E-0099-W09.
    static void InitDroppedSpikes(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
    // Debug001BC160, E-0099-W12.
    static void FiniDroppedSpikes(::port::driving::Memory&);
};

class Munition:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_001BB830(int,std::uint32_t,std::uint32_t);void Sub_001BB878(int);};
class DroppedBomb:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_001BBBF8(int,std::uint32_t,std::uint32_t);void Sub_001BBC38(int);};
}

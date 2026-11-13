#include "indep/inc/Pursuit.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Player.hpp"
#include "TrackedController.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Race.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Player.hpp"
#include "Chopper.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/src/speech/mgr.hpp"
#include <cmath>

// Complete released2289F8: eight canonical racer rows, resetPending+64 and
// restoreOverlay+70. These are not officer-entry states or Player copies.
int32_t pursuitManagerShouldDisplayBustedText(const PursuitManager* manager)
{
    if (!manager)return 0;
    for (const auto& row:manager->participantReset)
        if (row.resetPending && row.restoreOverlay)return 1;
    return 0;
}

// Complete released2274A8. The nine-state switch is the final table2BD210,
// and both distance tests use the actual observer/officer Car state.
float pursuitManagerRadarIntensity(const PursuitManager* manager,const Car* car,int32_t* activePursuit)
{
    if (!manager || !manager->collision || !car || !car->movement || !activePursuit ||
        !manager->race || !manager->race->route)
        throw std::invalid_argument("Radar intensity has no canonical pursuit/route owners");
    const auto& observer=*static_cast<const CarMovement*>(car->movement);
    if (!observer.bindings.state)throw std::invalid_argument("Radar observer has no live CarState");
    float closest=500.0f;
    for (size_t index=0;index<manager->collision->entryCount;++index) {
        const auto* entry=manager->collision->entries[index];
        if (!entry || !entry->movement || !entry->movement->bindings.state)
            throw std::invalid_argument("Radar officer has no live CarState");
        bool eligible=!pursuitCollisionEntryEligible(entry),active=false;
        if (!eligible) {
            switch (entry->state) {
            case 3:case 5:case 7:case 8:eligible=true;break;
            case 4:eligible=true;active=true;break;
            default:break;
            }
        }
        if (!eligible)continue;
        const auto& officer=*entry->movement;
        const float x=car->position[0]-officer.bindings.owner->position[0];
        const float y=car->position[1]-officer.bindings.owner->position[1];
        const float distance=std::sqrt(x*x+y*y);
        if (distance<250.0f && distance<closest &&
            driverCarInHeadingCone(observer.bindings.state,officer.bindings.state,16384) &&
            std::fabs(trackWrappedDelta(manager->race->route->length,
                observer.routeLocation.distance,officer.routeLocation.distance))<=250.0f) {
            closest=distance;*activePursuit=active?1:0;
        }
    }
    return closest<=250.0f?(250.0f-closest)*0.004000000189989805f:0.0f;
}
#include "indep/inc/AICarController.hpp"
#include "indep/inc/PhysicsCar.hpp"

#include <limits.h>
#include <math.h>
#include <string.h>
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>
#include <vector>
#include <memory>
#include <stdexcept>
#include <array>
#include <cassert>
#include "indep/inc/Replay.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CarDriver.hpp"

int32_t pursuitLimitAiPursuitsToOne=0;

// The authored resource, native live blocks and original-index lookup are
// three views of one catalogue allocation. No copied per-group site arrays.
struct PursuitRoadBlockCatalog {
    std::vector<PursuitRoadBlockSite> sites;
    std::vector<PursuitRoadBlock> blocks;
    std::vector<int32_t> bySite;
    size_t count=0;
};

struct PursuitSpeedTrapCatalog {
    std::vector<PursuitSpeedTrapSite> sites;
    std::vector<PursuitSpeedTrap> traps;
    std::vector<int32_t> bySite;
    size_t count=0;
};

struct PursuitManagerOwnedState {
    PursuitCollisionContext collision{};
    std::array<PursuitCollisionEntry,7> entries{};
    std::array<PursuitCollisionEntry*,7> indexed{};
    std::array<PursuitSpikeStrip,PURSUIT_SPIKE_INITIAL_COUNT> strips{};
    PursuitSpikeRegistry spikes{};
};

// CopManager PAL2235F8..2239E4. All three source pools and their canonical
// views are retained once. Runtime source fields are initialized in source
// order, including the8 actual RNG draws and the live speech flush.
PursuitManager::PursuitManager(const hp2::PursuitConstructionBindings& b)
{
    hp2::Require(b.world && b.clock && b.simulation && b.race && b.parameters && b.launch &&
        b.audio && b.messages && b.speech && b.speech->session && b.groups && b.road && b.terrain &&
        b.trackBytes && b.trackSize && b.race->route && b.objects,
        "CopManager has incomplete loaded World owners");
    auto state=std::make_unique<PursuitManagerOwnedState>();
    world=b.world;clock=b.clock;simulation=b.simulation;race=b.race;parameters=b.parameters;
    // PAL225664/22567C reads the live RaceCoordinator at GP-56C8, then
    // its state at +0. Frontend flow state4 is ordinary gameplay, while
    // session state4 is replay and suppresses speech. Borrow the same
    // session already published to the speech owner before World creation.
    launch=b.launch;applicationState=&b.speech->session->state;audio=b.audio;speech=b.messages;
    collision=&state->collision;pursuitCollisionInitialize(collision);
    collision->entries=state->indexed.data();collision->parameters=parameters;
    collision->speech=b.speech;collision->manager=this;
    ownedGroups=nullptr;groups=nullptr;groupCount=0;groupCapacity=0;
    roadBlockCatalog=nullptr;speedTrapCatalog=nullptr;result=WORLD_SERVICES_OK;
    spikes=&state->spikes;
    pursuitSpikeRegistryInitialize(spikes,state->strips.data(),state->strips.size(),b.terrain,
        b.objects);
    try {
        hp2::Require(pursuitManagerBindGroupStorage(this,b.groups)==WORLD_SERVICES_OK,
            "CopManager pursuit pool could not be constructed");
        hp2::Require(pursuitManagerLoadRoadBlockSites(this,b.trackBytes,b.trackSize,b.road,b.terrain)==WORLD_SERVICES_OK,
            "CopManager authored road-block sites are invalid");
        hp2::Require(pursuitManagerLoadSpeedTrapSites(this,b.trackBytes,b.trackSize,b.road,b.terrain)==WORLD_SERVICES_OK,
            "CopManager authored speed-trap sites are invalid");
        // The source inserts0..3 at the front of its circular deque.
        for(int32_t channel=3;channel>=0;--channel)pursuitChannelPush(&collision->channels,channel);
        speechElapsed=0.0f;supervisorRemaining=-50.0f;speechGap=12.0f;
        // PAL223848 precedes FlushQueue223850: initialize the same speaker
        // identity tables consumed by the real police speech FSM.
        hp2::Require(worldSpeechAllocateVoices(b.speech)==WORLD_SPEECH_OK,
            "CopManager cannot initialize the real speech identity owner");
        hp2::Require(worldSpeechFlush(b.speech)==WORLD_SPEECH_OK,"CopManager cannot flush the real speech owner");
        for(auto& participant:participantReset){participant={};participant.selectedOfficer=-1;}
        for(auto& participant:participantReset)
            participant.preventPursuitSeconds=pursuitBaseSpeedTrapSpawnTime(world,parameters,race->route,launch);
        completedPullovers=0;
        selectedRadioChannel=0;
        if(parameters->specialPursuit) {
            hp2::Require(pursuitChannelPop(&collision->channels,&selectedRadioChannel)!=0,
                "CopManager radio channel pool is empty");
            roadBlocksRemaining=parameters->initialRoadBlockRequests;
            chopperRequestsRemaining=parameters->initialChopperRequests;
            remainingSeconds=static_cast<float>(parameters->timeLimitTicks)*0.0002500000118743628f;
        } else {roadBlocksRemaining=0;chopperRequestsRemaining=0;remainingSeconds=0.0f;}
        collision->severity=parameters->pursuitDifficulty==0?0:(parameters->pursuitDifficulty==1?1:2);
        beginSpeechSent=0;introSpeechSent=0;chopperReserved=0;lastFillerEvent=0;
        copDimensions[0]=0.0f;copDimensions[1]=0.0f;
    } catch(...) {
        if(ownedGroups)pursuitManagerDisposeGroupStorage(this);
        pursuitSpikeRegistryDispose(spikes);
        pursuitManagerDisposeRoadBlockSites(this);pursuitManagerDisposeSpeedTrapSites(this);
        collision=nullptr;spikes=nullptr;throw;
    }
    ownedState=state.release();
}

PursuitManager::~PursuitManager()
{
    if(!ownedState)return;
    if(ownedGroups) {
        const auto status=pursuitManagerDisposeGroupStorage(this);
        assert(status==WORLD_SERVICES_OK);(void)status;
    }
    // Concrete tAICop/tPlayerCop destructors own no Car. Cars are retired
    // by RC/World; do not dereference their borrowed movement here.
    for(size_t i=0;i<collision->entryCount;++i) {
        auto* entry=collision->entries[i];
        if(entry && entry->speedTrap)entry->speedTrap->officer=nullptr;
        collision->entries[i]=nullptr;
    }
    pursuitSpikeRegistryDispose(spikes);
    const auto blocks=pursuitManagerDisposeRoadBlockSites(this);
    const auto traps=pursuitManagerDisposeSpeedTrapSites(this);
    assert(blocks==WORLD_SERVICES_OK && traps==WORLD_SERVICES_OK);(void)blocks;(void)traps;
    delete ownedState;ownedState=nullptr;collision=nullptr;spikes=nullptr;
}

namespace hp2 {
// CreateCop PAL2272E8..227438. The external branch ignores DriverInfo and
// borrows exactly World.carTable[0]; AI construction publishes its own Car.
PursuitCollisionEntry* PursuitCreateCop(PursuitManager& manager,RaceDriverInfo* driver,bool playerCop)
{
    Require(manager.ownedState && manager.collision && manager.world,"CreateCop has no owning CopManager");
    auto& context=*manager.collision;const size_t index=context.primaryEntryCount;
    Require(index<(playerCop?7u:6u) && !context.entries[index],"CopManager officer pool is exhausted");
    auto& entry=manager.ownedState->entries[index];
    if(playerCop) {
        Require(manager.world->totalCars>0 && manager.world->carTable[0],"PlayerCop has no first World Car");
        Car* car=manager.world->carTable[0];context.activeEntry=index;
        PursuitConstructPlayerCop(entry,*car,static_cast<int32_t>(index));
        context.entries[index]=&entry;
        const uint32_t wanted=manager.parameters->reverse?2u:1u;
        if(manager.speedTrapCatalog)for(size_t i=0;i<manager.speedTrapCatalog->count;++i) {
            const auto& trap=manager.speedTrapCatalog->traps[i];if(!(trap.flags&wanted))continue;
            const float position[3]={trap.position[0],trap.position[1],0.0f};
            const auto heading=static_cast<uint16_t>(trap.heading-0x7d00u);
            Require(carMovementPlaceAndNotify(car->Movement(),car->Driver(),position,heading,0.0f)==CAR_MOVEMENT_OK,
                "PlayerCop speed-trap placement failed");break;
        }
    } else {
        Require(driver,"AICop creation requires its actual DriverInfo");
        PursuitConstructOwnedCop(manager,entry,*driver,static_cast<int32_t>(index));
        context.entries[index]=&entry;++context.primaryEntryCount;
    }
    ++context.entryCount;
    // PAL CreateCop wrapper227D78 tail-calls UpdateCopDimensions2276A0
    // after publishing each officer. The native creation boundary must keep
    // that call: the manager constructor deliberately starts this cache at
    // zero, and every road-block car/gap width consumes it later.
    const auto* render=WorldCarBindings(*manager.world).render;
    Require(render && pursuitManagerUpdateCopDimensions(&manager,render->cars)==WORLD_SERVICES_OK,
        "CreateCop cannot update road-block dimensions from the first officer's canonical CarInfo");
    return &entry;
}
}

// PAL2269A8..226BB0. Grounded authored points become live traps in the
// source order; invalid/ungrounded sites retain their original-index -1.
WorldServicesResult pursuitManagerInitializeSpeedTraps(PursuitManager* manager,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain)
{
    if(!manager)return WORLD_SERVICES_INVALID_BINDING;
    PursuitSpeedTrapCatalog* catalog=manager->speedTrapCatalog;
    if(!catalog)return manager->result=WORLD_SERVICES_OK;
    if(!road || !road->route || !terrain)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    for(const auto& trap:catalog->traps)if(trap.officer)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    catalog->count=0;
    for(size_t i=0;i<catalog->sites.size();++i) {
        catalog->bySite[i]=-1;
        const auto& site=catalog->sites[i];
        TerrainSurface ground;terrainSurfaceInitialize(&ground,terrain);
        if(!terrainSurfaceHasGround(&ground,site.position)){terrainSurfaceDispose(&ground);continue;}
        if(catalog->count>=catalog->traps.size()) {
            terrainSurfaceDispose(&ground);return manager->result=WORLD_SERVICES_CAPACITY;
        }
        PursuitSpeedTrap& trap=catalog->traps[catalog->count];
        TrackRouteLocation location;trackRouteLocationAtPoint(road->route,&location,site.position);
        trap.routeDistance=location.distance;trap.flags=0;
        trap.position[0]=site.position[0];trap.position[1]=site.position[1];trap.officer=nullptr;
        if(site.authoredFlagFirst)trap.flags|=1u;
        else if(site.authoredFlagSecond)trap.flags|=2u;
        CarRecoverySelection selection{};
        if(!carRecoverySelectPose(&selection,road,site.position,0,0,0.0f) || !selection.pose) {
            terrainSurfaceDispose(&ground);return manager->result=WORLD_SERVICES_INVALID_BINDING;
        }
        VectorPolar2 heading;vectorPolarFromCartesian2(&heading,selection.pose->direction);
        trap.heading=heading.angle;trap.site=&site;
        catalog->bySite[i]=static_cast<int32_t>(catalog->count++);
        terrainSurfaceDispose(&ground);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// Native32-byte34510 decoding boundary. No guessed mutable usage flags or
// second road catalogue is introduced; live traps borrow these stable sites.
WorldServicesResult pursuitManagerLoadSpeedTrapSites(PursuitManager* manager,const uint8_t* track,size_t size,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain)
{
    if(!manager || !track || !size || manager->speedTrapCatalog)return WORLD_SERVICES_INVALID_BINDING;
    try {
        const std::vector<uint8_t> bytes(track,track+size);hp2::Chunk encoded{};
        for(const auto& container:hp2::ReadChunks(bytes)) {
            if(container.id!=0x80034500u)continue;
            for(const auto& child:hp2::ReadChunks(bytes,container.payloadOffset,container.size)) {
                if(child.id!=0x34510u)continue;
                hp2::Require(!encoded.id,"Track repeats its speed-trap catalogue");encoded=child;
            }
        }
        if(!encoded.id)return pursuitManagerInitializeSpeedTraps(manager,road,terrain);
        hp2::Require(encoded.size>=4,"Truncated speed-trap catalogue");
        const uint8_t* data=bytes.data()+encoded.payloadOffset;const uint32_t count=hp2::ReadU32(data);
        hp2::Require(count<=64 && encoded.size==4+static_cast<size_t>(count)*32,
            "Speed-trap catalogue exceeds source capacity or has a partial record");
        std::unique_ptr<PursuitSpeedTrapCatalog> catalog(new PursuitSpeedTrapCatalog);
        catalog->sites.resize(count);catalog->traps.resize(count);catalog->bySite.resize(count,-1);
        for(size_t i=0;i<count;++i) {
            const uint8_t* record=data+4+i*32;auto& site=catalog->sites[i];
            memcpy(site.name,record,16);site.name[16]=0;
            for(size_t axis=0;axis<2;++axis) {
                const uint32_t bits=hp2::ReadU32(record+16+axis*4);memcpy(&site.position[axis],&bits,sizeof(bits));
                hp2::Require(std::isfinite(site.position[axis]),"Non-finite speed-trap coordinate");
            }
            const uint32_t first=hp2::ReadU32(record+24),second=hp2::ReadU32(record+28);
            memcpy(&site.authoredFlagFirst,&first,sizeof(first));memcpy(&site.authoredFlagSecond,&second,sizeof(second));
        }
        manager->speedTrapCatalog=catalog.release();
        const auto result=pursuitManagerInitializeSpeedTraps(manager,road,terrain);
        if(result!=WORLD_SERVICES_OK){delete manager->speedTrapCatalog;manager->speedTrapCatalog=nullptr;}
        return result;
    } catch(const std::exception&) {return manager->result=WORLD_SERVICES_INVALID_BINDING;}
}

// PAL226BB0..226BE0, original authored-index lookup.
PursuitSpeedTrap* pursuitManagerSpeedTrapBySite(const PursuitManager* manager,int32_t authoredIndex)
{
    if(!manager || !manager->speedTrapCatalog || authoredIndex<0)return nullptr;
    auto& catalog=*manager->speedTrapCatalog;
    if(static_cast<size_t>(authoredIndex)>=catalog.bySite.size())return nullptr;
    const int32_t index=catalog.bySite[static_cast<size_t>(authoredIndex)];
    return index<0 ? nullptr : &catalog.traps[static_cast<size_t>(index)];
}

WorldServicesResult pursuitManagerDisposeSpeedTrapSites(PursuitManager* manager)
{
    if(!manager)return WORLD_SERVICES_INVALID_BINDING;
    if(manager->speedTrapCatalog)for(const auto& trap:manager->speedTrapCatalog->traps)
        if(trap.officer)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    delete manager->speedTrapCatalog;manager->speedTrapCatalog=nullptr;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL226C30..226E6C. One World RNG draw selects cyclic evaluation order;
// the final strict-best scan remains in original authored live-trap order.
WorldServicesResult pursuitManagerFindSuitableSpeedTrap(PursuitManager* manager,PursuitSpeedTrap** result)
{
    if(!manager || !result)return WORLD_SERVICES_INVALID_BINDING;
    *result=nullptr;
    auto* catalog=manager->speedTrapCatalog;
    if(!catalog || !catalog->count)return manager->result=WORLD_SERVICES_OK;
    if(!manager->world || !manager->race || !manager->race->route || !(manager->race->route->length>0.0f) ||
        catalog->count>64)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    const float length=manager->race->route->length;
    std::array<float,64> scores{};
    size_t index=worldRandomBounded(manager->world,static_cast<uint32_t>(catalog->count));
    for(size_t step=0;step<catalog->count;++step) {
        index=(index+1)%catalog->count;scores[index]=0.0f;
        const auto& trap=catalog->traps[index];
        if(trap.officer)continue;
        if(worldGetDistanceToClosestPlayer2D(manager->world,trap.position)<250.0f)continue;
        int32_t clear;
        const auto status=pursuitPositionClear(manager->world,trap.position,&clear);
        if(status!=WORLD_SERVICES_OK)return manager->result=status;
        if(!clear)continue;
        scores[index]+=10.0f;
        float nearest=length;
        for(size_t other=0;other<catalog->count;++other) {
            const auto& occupied=catalog->traps[other];
            if(!occupied.officer)continue;
            // Complete226BE0: a single forward wrap, then the nearer arc.
            float separation=occupied.routeDistance-trap.routeDistance;
            if(separation<0.0f)separation+=length;
            if(length*0.5f<separation)separation=length-separation;
            if(separation<nearest)nearest=separation;
        }
        const float fraction=finiteRatio(nearest,length);
        const float contribution=fraction*100.0f;
        scores[index]+=contribution;
    }
    float best=1.0f;
    for(size_t i=0;i<catalog->count;++i)if(best<scores[i]){best=scores[i];*result=&catalog->traps[i];}
    return manager->result=WORLD_SERVICES_OK;
}

// PAL225EC8..226238, InitRoadBlockStructures. The resource reader below
// supplies only actual34520 records; road selection and terrain are real.
WorldServicesResult pursuitManagerInitializeRoadBlocks(PursuitManager* manager,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain)
{
    if(!manager || !manager->parameters)return WORLD_SERVICES_INVALID_BINDING;
    PursuitRoadBlockCatalog* catalog=manager->roadBlockCatalog;
    if(catalog)catalog->count=0;
    if(!catalog || !manager->parameters->roadBlocksEnabled)return manager->result=WORLD_SERVICES_OK;
    if(!road || !road->route || !terrain)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    for(size_t i=0;i<catalog->sites.size();++i) {
        catalog->bySite[i]=-1;
        const PursuitRoadBlockSite& site=catalog->sites[i];
        if(site.directionEligibility!=0 && !
            (site.directionEligibility==1 && manager->parameters->reverse==0) && !
            (site.directionEligibility==2 && manager->parameters->reverse!=0))continue;
        CarRecoverySelection selection{};
        if(!carRecoverySelectPose(&selection,road,site.position,0,0,0.0f))continue;
        const CarRecoveryPose* pose=selection.pose;
        if(!pose || !selection.group)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        const float width=pose->halfWidths[0]+pose->halfWidths[1];
        if(width<9.0f)continue;
        TerrainSurface surface;terrainSurfaceInitialize(&surface,terrain);
        const bool grounded=terrainSurfaceHasGround(&surface,site.position);
        if(!grounded){terrainSurfaceDispose(&surface);continue;}
        if(catalog->count==catalog->blocks.size()) {
            terrainSurfaceDispose(&surface);return manager->result=WORLD_SERVICES_CAPACITY;
        }
        PursuitRoadBlock& block=catalog->blocks[catalog->count];
        block.roadGroup=selection.group;block.roadPose=pose;block.width=width;
        block.site=&site;block.pattern=site.authoredPattern;
        block.siteIndex=static_cast<int32_t>(i);block.flags=site.authoredOption6C;
        block.active=site.initiallyActive;
        const float dx=site.position[0]-pose->position[0];
        const float dy=site.position[1]-pose->position[1];
        const float along=dx*pose->direction[0]+dy*pose->direction[1];
        const float across=(pose->halfWidths[1]-pose->halfWidths[0])*0.5f;
        carRoadPreviewPointXY(pose,block.groundPoint,along,across);
        block.across[0]=pose->direction[1];block.across[1]=-pose->direction[0];
        block.entries[0]=nullptr;block.entries[1]=nullptr;
        TrackRouteLocation location;
        trackRouteLocationAtPoint(road->route,&location,block.groundPoint);
        block.routeDistance=location.distance;
        catalog->bySite[i]=static_cast<int32_t>(catalog->count++);
        terrainSurfaceDispose(&surface);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// Native file/chunk ownership boundary. The28-byte wire record is decoded
// explicitly, never cast into a runtime object or kept as a packed blob.
WorldServicesResult pursuitManagerLoadRoadBlockSites(PursuitManager* manager,const uint8_t* track,size_t size,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain)
{
    if(!manager || !track || !size || manager->roadBlockCatalog || manager->groupCount)
        return WORLD_SERVICES_INVALID_BINDING;
    try {
        const std::vector<uint8_t> bytes(track,track+size);hp2::Chunk encoded{};
        for(const auto& container:hp2::ReadChunks(bytes)) {
            if(container.id!=0x80034500u)continue;
            for(const auto& child:hp2::ReadChunks(bytes,container.payloadOffset,container.size)) {
                if(child.id!=0x34520u)continue;
                hp2::Require(encoded.id==0,"Track repeats its road-block site catalogue");encoded=child;
            }
        }
        if(!encoded.id)return pursuitManagerInitializeRoadBlocks(manager,road,terrain);
        hp2::Require(encoded.size>=4,"Truncated road-block catalogue header");
        const uint8_t* data=bytes.data()+encoded.payloadOffset;const uint32_t count=hp2::ReadU32(data);
        hp2::Require(count<=(encoded.size-4)/28 && encoded.size==4+static_cast<size_t>(count)*28,
            "Partial authored road-block record");
        std::unique_ptr<PursuitRoadBlockCatalog> catalog(new PursuitRoadBlockCatalog);
        catalog->sites.resize(count);catalog->blocks.resize(count<64 ? count : 64);catalog->bySite.resize(count,-1);
        for(size_t i=0;i<count;++i) {
            const uint8_t* record=data+4+i*28;auto& site=catalog->sites[i];
            memcpy(site.name,record,16);site.name[16]=0;
            for(size_t axis=0;axis<2;++axis) {
                const uint32_t bits=hp2::ReadU32(record+16+axis*4);memcpy(site.position+axis,&bits,4);
                hp2::Require(std::isfinite(site.position[axis]),"Non-finite authored road-block point");
            }
            const uint32_t flags=hp2::ReadU32(record+24);
            site.directionEligibility=flags&3u;site.authoredPattern=(flags>>2)&3u;
            site.authoredOption6C=(flags>>4)&3u;site.initiallyActive=(flags>>6)&1u;
        }
        manager->roadBlockCatalog=catalog.release();
        const auto result=pursuitManagerInitializeRoadBlocks(manager,road,terrain);
        if(result!=WORLD_SERVICES_OK){delete manager->roadBlockCatalog;manager->roadBlockCatalog=nullptr;}
        return result;
    } catch(const std::exception&) {return manager->result=WORLD_SERVICES_INVALID_BINDING;}
}

// PAL226448..226478: original authored-index view, not active-list ordinal.
PursuitRoadBlock* pursuitManagerRoadBlockBySite(const PursuitManager* manager,int32_t authoredIndex)
{
    if(!manager || !manager->roadBlockCatalog || authoredIndex<0)return nullptr;
    auto& catalog=*manager->roadBlockCatalog;
    if(static_cast<size_t>(authoredIndex)>=catalog.bySite.size())return nullptr;
    const int32_t index=catalog.bySite[static_cast<size_t>(authoredIndex)];
    return index<0 ? nullptr : &catalog.blocks[static_cast<size_t>(index)];
}

WorldServicesResult pursuitManagerDisposeRoadBlockSites(PursuitManager* manager)
{
    if(!manager || manager->groupCount)return WORLD_SERVICES_INVALID_BINDING;
    delete manager->roadBlockCatalog;manager->roadBlockCatalog=nullptr;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL226810..226968. Only the original primary officer cohort participates;
// live mode-transition state and 250m visibility gate keep their source order.
WorldServicesResult pursuitManagerFindCopsForRoadBlock(PursuitManager* manager,
    PursuitCollisionEntry* output[2],int32_t requested,int32_t* count)
{
    if(!manager || !manager->world || !manager->collision || !output || !count || requested<0 || requested>2 ||
        manager->collision->primaryEntryCount>manager->collision->entryCount ||
        (manager->collision->primaryEntryCount && !manager->collision->entries))return WORLD_SERVICES_INVALID_BINDING;
    output[1]=nullptr;output[0]=nullptr;*count=0;
    for(size_t i=0;i<manager->collision->primaryEntryCount && *count!=requested;++i) {
        PursuitCollisionEntry* entry=manager->collision->entries[i];
        if(!entry || !entry->movement || !entry->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
        if(entry->lifecycleState!=entry->state)continue;
        if(worldGetDistanceToClosestPlayer2D(manager->world,entry->movement->bindings.state->position)<250.0f)continue;
        if((entry->state==1 || entry->state==3) && entry->drivingState!=13)output[(*count)++]=entry;
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL226298..226448. Availability, real officer selection, graph search,
// layout, concrete controller assignment and spike creation remain ordered.
WorldServicesResult pursuitManagerRequestBlock(PursuitManager* manager,PursuitCollisionGroup* group,
    float direction,PursuitRoadBlock** output)
{
    if(!manager || !manager->parameters || !output)return WORLD_SERVICES_INVALID_BINDING;
    *output=nullptr;
    if(!manager->roadBlockCatalog || !manager->parameters->roadBlocksEnabled)
        return manager->result=WORLD_SERVICES_OK;
    if(!group || !group->owner || group->owner->manager!=manager || !group->target ||
        !group->target->bindings.owner || !manager->simulation || !manager->simulation->pursuitServices ||
        manager->simulation->pursuitServices->manager!=manager ||
        manager->simulation->pursuitServices->world!=manager->world ||
        !manager->simulation->pursuitServices->trackGraph || !group->owner->bindings ||
        !group->owner->bindings->route)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    PursuitCollisionEntry* selected[2];int32_t selectedCount=0;
    auto result=pursuitManagerFindCopsForRoadBlock(manager,selected,group->owner->allocationMode,&selectedCount);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(selectedCount!=group->owner->allocationMode)return manager->result=WORLD_SERVICES_OK;
    PursuitRoadBlock* block=nullptr;
    result=trackGraphFindRoadBlock(manager->simulation->pursuitServices->trackGraph,manager,
        group->target->bindings.owner,600.0f,group->owner->bindings->route->length*0.5f,direction==1.0f,&block);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(!block)return manager->result=WORLD_SERVICES_OK;
    int32_t cars=0;
    result=pursuitManagerCalculateRoadBlock(manager,block,group,&cars);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(cars>selectedCount)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    for(int32_t i=0;i<cars;++i) {
        block->entries[i]=selected[i];
        if(!selected[i] || pursuitEntryAssignRoadBlock(selected[i],block,group,i)!=CAR_DRIVER_OK)
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    }
    if(group->requestFlags&0x10u) {
        if(!manager->spikes)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        const float first[2]={bScalarAdd(block->center[0],-block->halfSpan[0]),bScalarAdd(block->center[1],-block->halfSpan[1])};
        const float second[2]={bScalarAdd(block->center[0],block->halfSpan[0]),bScalarAdd(block->center[1],block->halfSpan[1])};
        if(!pursuitSpikeCreate(manager->spikes,first,second,block->siteIndex,-1.0f))
            return manager->result=WORLD_SERVICES_CAPACITY;
    }
    *output=block;return manager->result=WORLD_SERVICES_OK;
}

// PAL225D30..225EC8. Special mode uses the actual selected officer; the
// ordinary scan keeps the source state table and skips active Player NIS.
WorldServicesResult pursuitManagerAnyCopNear(PursuitManager* manager,CarMovement* target,float distance,int32_t* near)
{
    if(!manager || !manager->parameters || !manager->collision || !target || !target->bindings.state || !near ||
        (manager->collision->entryCount && !manager->collision->entries))return WORLD_SERVICES_INVALID_BINDING;
    *near=0;
    const float* point=target->bindings.state->position;
    const auto within=[&](const CarMovement* movement) {
        const float* p=movement->bindings.state->position;
        const float dx=p[0]-point[0],dy=p[1]-point[1];const float x2=dx*dx,y2=dy*dy;
        return std::sqrt(x2+y2)<=distance;
    };
    if(manager->parameters->specialPursuit) {
        const PursuitCollisionEntry* selected=pursuitManagerSelectedEntry(manager);
        if(!selected || !selected->movement || !selected->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
        *near=within(selected->movement);return manager->result=WORLD_SERVICES_OK;
    }
    for(size_t i=0;i<manager->collision->entryCount;++i) {
        const PursuitCollisionEntry* entry=manager->collision->entries[i];
        if(!entry || !entry->movement || !entry->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
        if(entry->state!=2 && entry->state!=3 && entry->state!=4 && entry->state!=5 && entry->state!=6 && entry->state!=8)continue;
        const CarMovement* movement=entry->movement;
        if(movement->bindings.player) {
            if(!movement->frame || !movement->frame->environment.player ||
                &movement->frame->environment.player->movement!=movement->bindings.player)return WORLD_SERVICES_INVALID_BINDING;
            if(carFramePlayerReplayActive(movement->frame->environment.player))continue;
        }
        if(within(movement)){*near=1;break;}
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL2286B8..228750, all officer cohorts, using the real contact query.
WorldServicesResult pursuitManagerAnyCopContact(PursuitManager* manager,CarMovement* target,int32_t* collided)
{
    if(!manager || !manager->collision || !target || !collided ||
        (manager->collision->entryCount && !manager->collision->entries))return WORLD_SERVICES_INVALID_BINDING;
    *collided=0;
    for(size_t i=0;i<manager->collision->entryCount;++i) {
        const PursuitCollisionEntry* entry=manager->collision->entries[i];
        if(!entry || !entry->movement)return WORLD_SERVICES_INVALID_BINDING;
        int32_t hit=0;
        if(pursuitCarsCollided(target,entry->movement,&hit)!=CAR_DRIVER_OK)return WORLD_SERVICES_INVALID_BINDING;
        if(hit){*collided=1;break;}
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL226478..226810. Select an authored pattern in exact World-RNG order,
// build its real command queue, then position actual cars/barrels on the road.
WorldServicesResult pursuitManagerCalculateRoadBlock(PursuitManager* manager,PursuitRoadBlock* block,
    PursuitCollisionGroup* group,int32_t* carCount)
{
    if(!manager || !manager->world || !manager->parameters || !group || !group->owner ||
        group->owner->manager!=manager || !group->owner->bindings || !block || !block->site ||
        !block->roadPose || !carCount)return WORLD_SERVICES_INVALID_BINDING;
    // Exact final data31E938..31EA30. Each complete pattern has a real6
    // terminator; only the four-entry one-car patterns omit trailing words.
    static constexpr int32_t oneFirst[]={0,2,3,6};
    static constexpr int32_t oneSecond[]={3,2,1,6};
    static constexpr int32_t ordinary[]={0,3,4,3,1,6};
    static constexpr int32_t ordinaryFill[]={0,3,5,3,1,6};
    static constexpr int32_t leftGap[]={0,2,3,4,3,6};
    static constexpr int32_t rightGap[]={3,4,3,2,1,6};
    static constexpr int32_t leftFill[]={0,2,3,5,3,6};
    static constexpr int32_t rightFill[]={3,5,3,2,1,6};
    static constexpr int32_t middleFirst[]={3,2,3,1,6};
    static constexpr int32_t middleSecond[]={0,3,2,3,6};
    const int32_t* pattern=ordinary;size_t count=6;
    float spacing=0.0f;
    auto result=pursuitGroupRoadBlockSpacing(group,&spacing);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if((group->requestFlags&0x10u) || group->owner->allocationMode==1) {
        // Re-read the immutable authored value on EVERY reuse. The live
        // block.pattern is a randomized result and cannot serve as its input.
        block->pattern=block->site->authoredPattern;
        if(!block->pattern)block->pattern=worldRandomBounded(manager->world,2) ? 1 : 2;
        if(group->owner->allocationMode==1) {
            pattern=block->pattern==1 ? oneFirst : oneSecond;count=4;
        } else if(worldRandomBounded(manager->world,4)==0) {
            pattern=block->pattern==1 ? middleFirst : middleSecond;count=5;
        } else if(manager->parameters->authoredPursuitOption268 && worldRandomBounded(manager->world,2)==0) {
            pattern=block->pattern==1 ? leftFill : rightFill;
        } else pattern=block->pattern==1 ? leftGap : rightGap;
    } else if(manager->parameters->authoredPursuitOption268)pattern=ordinaryFill;
    PursuitWordQueue queue;
    result=pursuitBuildRoadBlockLayout(queue,pattern,count,block,manager,spacing);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    float cursor=-block->roadPose->halfWidths[0];
    int32_t cars=0;uint32_t element=0;
    while(queue.Remove(element)) {
        uint32_t parameter=0;
        switch(element) {
        case 0:case 1: {
            if(!queue.Remove(parameter))return manager->result=WORLD_SERVICES_INVALID_BINDING;
            float distance;memcpy(&distance,&parameter,4);cursor=bScalarAdd(cursor,distance);
            // Exact226664/226674 ordering uses the advanced cursor plus
            // half the just-consumed width for the actual world gap point.
            vectorMultiplyAdd2(block->along,block->roadPose->position,block->across,bScalarAdd(cursor,bScalarMultiply(distance,0.5f)));
            break;
        }
        case 4:cursor=bScalarAdd(cursor,0.800000011920929f);break;
        case 2: {
            if(!queue.Remove(parameter))return manager->result=WORLD_SERVICES_INVALID_BINDING;
            int32_t barrels;memcpy(&barrels,&parameter,4);
            const float width=bScalarMultiply(static_cast<float>(barrels),0.6499999761581421f);
            const float half=bScalarMultiply(width,0.5f);
            vectorMultiplyAdd2(block->center,block->roadPose->position,block->across,bScalarAdd(cursor,half));
            cursor=bScalarAdd(cursor,width);
            block->halfSpan[0]=bScalarMultiply(block->across[0],half);
            block->halfSpan[1]=bScalarMultiply(block->across[1],half);
            break;
        }
        case 3: {
            if(cars>=2 || !queue.Remove(parameter))return manager->result=WORLD_SERVICES_INVALID_BINDING;
            const uint16_t angle=static_cast<uint16_t>(parameter);
            const float width=pursuitProjectedCopWidth(manager,angle);
            vectorMultiplyAdd2(block->carPositions[cars],block->roadPose->position,block->across,bScalarAdd(cursor,bScalarMultiply(width,0.5f)));
            cursor=bScalarAdd(cursor,width);
            VectorPolar2 polar;vectorPolarFromCartesian2(&polar,block->across);
            block->carHeadings[cars]=static_cast<uint16_t>(cars==0 ? polar.angle+angle : polar.angle+0x8000u-angle);
            ++cars;break;
        }
        case 5: {
            if(!group->owner->bindings->topology || !group->owner->barricades)
                return manager->result=WORLD_SERVICES_INVALID_BINDING;
            float point[2];
            vectorMultiplyAdd2(point,block->roadPose->position,block->across,bScalarAdd(cursor,bScalarMultiply(pursuitRoadBlockFillWidth,0.5f)));
            float grounded[4];pursuitGroundPoint(group->owner->bindings->topology,grounded,point);
            int16_t id=-1;
            result=pursuitBarricadesCreateAt(group->owner->barricades,grounded,&id);
            if(result!=WORLD_SERVICES_OK)return manager->result=result;
            cursor=bScalarAdd(cursor,pursuitRoadBlockFillWidth);break;
        }
        }
    }
    *carCount=cars;return manager->result=WORLD_SERVICES_OK;
}

// PAL2276A0..2276D0. The cache derives from the SAME first officer's
// canonical model resource; the two source floats are not body half-sizes.
WorldServicesResult pursuitManagerUpdateCopDimensions(PursuitManager* manager,
    const hp2::CarInfoResource* resources)
{
    if(!manager || !manager->collision || !manager->collision->entries ||
        !manager->collision->entryCount || !resources || !resources->CarsLoaded())
        return WORLD_SERVICES_INVALID_BINDING;
    const PursuitCollisionEntry* entry=manager->collision->entries[0];
    if(!entry || !entry->movement || !entry->movement->bindings.owner ||
        !entry->movement->bindings.owner->driverInfo)return WORLD_SERVICES_INVALID_BINDING;
    const int32_t model=entry->movement->bindings.owner->driverInfo->appearance.modelCode;
    if(model<0 || static_cast<size_t>(model)>=resources->Cars().size())return WORLD_SERVICES_INVALID_BINDING;
    const auto& info=resources->Car(static_cast<size_t>(model));
    if(entry->movement->info.body!=&info.rigidBodyInfo)return WORLD_SERVICES_INVALID_BINDING;
    manager->copDimensions[0]=info.roadBlockDimensions[0]*1.100000023841858f;
    manager->copDimensions[1]=info.roadBlockDimensions[1]*1.100000023841858f;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL224EC0..224EFC, IsChopperAvailable. This is a query: reservation and
// allowance consumption occur only in the distinct RequestChopper body.
WorldServicesResult pursuitManagerChopperAvailable(const PursuitManager* manager,int32_t* available)
{
    if(!manager || !manager->simulation || !manager->parameters || !available)
        return WORLD_SERVICES_INVALID_BINDING;
    const Chopper* chopper=manager->simulation->chopper;
    *available=chopper && manager->parameters->chopperEnabled &&
        !chopper->forceFirstPlayerTarget && !manager->chopperReserved;
    return WORLD_SERVICES_OK;
}

// PAL224F00..224F6C. The result is the world's SAME Chopper; a special
// pursuit needs an allowance, while an ordinary pursuit may use zero.
WorldServicesResult pursuitManagerAcquireChopper(PursuitManager* manager,Chopper** output)
{
    if(!manager || !manager->simulation || !manager->parameters || !output)
        return WORLD_SERVICES_INVALID_BINDING;
    *output=nullptr;
    Chopper* chopper=manager->simulation->chopper;
    if(chopper && manager->parameters->chopperEnabled && !manager->chopperReserved &&
        (!manager->parameters->specialPursuit || manager->chopperRequestsRemaining)) {
        *output=chopper;
        manager->chopperReserved=1;
        if(manager->chopperRequestsRemaining)--manager->chopperRequestsRemaining;
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL225078..225308 (RequestPursuit). A successful request may create a
// group or join an existing one; the output describes actual acceptance.
WorldServicesResult pursuitManagerCreatePursuit(PursuitManager* manager,
    PursuitCollisionEntry* entry,Car* targetCar,int32_t* accepted)
{
    if(!accepted)return WORLD_SERVICES_INVALID_BINDING;
    *accepted=0;
    if(!manager || !manager->collision || !manager->parameters || !manager->world ||
        !entry || !entry->movement || !entry->movement->bindings.owner || !targetCar ||
        !targetCar->raceEntry || !targetCar->movement || !manager->ownedGroups ||
        manager->groupCount>manager->groupCapacity || (manager->groupCount && !manager->groups))
        return WORLD_SERVICES_INVALID_BINDING;
    if(targetCar->raceEntry->completionReason)return manager->result=WORLD_SERVICES_OK;
    auto* target=static_cast<CarMovement*>(targetCar->movement);
    if(target->bindings.owner!=targetCar)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    const bool player=target->bindings.player!=nullptr;
    bool found=false,permitted=false;
    PursuitCollisionGroup* group=nullptr;
    int32_t copLimit=0,allocationMode=0;
    if(manager->parameters->specialPursuit){
        if(pursuitManagerSelectedEntry(manager)!=entry)return manager->result=WORLD_SERVICES_OK;
        copLimit=1;allocationMode=2;permitted=true;
    }else{
        int32_t aiGroups=0;
        for(size_t index=0;index<manager->groupCount;++index){
            PursuitCollisionGroup* candidate=manager->groups[index];
            if(!candidate || !candidate->target || !candidate->owner)
                return manager->result=WORLD_SERVICES_INVALID_BINDING;
            if(!candidate->target->bindings.player)++aiGroups;
            if(candidate->target==target){found=true;group=candidate;}
        }
        if(group && group->phase==5)return manager->result=WORLD_SERVICES_OK;
        if(manager->parameters->mode>=2){
            copLimit=1;
            allocationMode=player ? 1 : 0;
            permitted=found ? group->activeCount<1 : player || aiGroups==0;
        }else if(player){
            copLimit=manager->world->secondGroupEnd<3 ? 2 : 1;
            allocationMode=2;
            if(!found || group->activeCount==0)permitted=true;
            else permitted=group->activeCount<copLimit && group->owner->calledForBackup!=0;
        }else{
            copLimit=1;
            allocationMode=0;
            permitted=found ? group->activeCount<1 :
                (pursuitLimitAiPursuitsToOne ? aiGroups==0 : aiGroups<2);
        }
    }
    if(!permitted)return manager->result=WORLD_SERVICES_OK;
    if(!found){
        int32_t id;
        auto result=pursuitManagerNextPursuitId(manager,&id);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(id<0)return manager->result=WORLD_SERVICES_CAPACITY;
        result=pursuitManagerAllocateGroup(manager,id,target,&group);
        if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupDeclareSpeech(manager,group);
        if(result!=WORLD_SERVICES_OK)return result;
        CarMovement* notified=manager->parameters->specialPursuit ? entry->movement : group->target;
        if(notified->bindings.player){
            CarFrame* frame=notified->frame;
            if(!frame || !frame->environment.player ||
                &frame->environment.player->movement!=notified->bindings.player)
                return manager->result=WORLD_SERVICES_INVALID_BINDING;
            int32_t tickets;
            result=pursuitManagerGetPulloverCount(manager,group->target->bindings.owner,&tickets);
            if(result!=WORLD_SERVICES_OK)return manager->result=result;
            // Original LoadNextNis return is ignored. A native missing-owner
            // error still propagates separately through the Player result.
            carFramePlayerLoadNextNis(frame->environment.player,group->target,entry->movement,tickets);
            if(frame->environment.player->result)return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        }
    }else{
        const auto result=pursuitGroupJoinSpeech(manager,group);
        if(result!=WORLD_SERVICES_OK)return result;
    }
    if(!group || !group->owner || group->activeCount<0 || group->activeCount>=2)
        return manager->result=WORLD_SERVICES_CAPACITY;
    group->owner->copLimit=copLimit;
    group->owner->allocationMode=allocationMode;
    group->active[group->activeCount++]=entry;
    pursuitCollisionSetGroup(entry,group);
    *accepted=1;
    return manager->result=WORLD_SERVICES_OK;
}

int32_t pursuitManagerRoadBlockCount(const PursuitManager* manager)
{
    int32_t count = 0;
    for (size_t index = 0; index < manager->groupCount; ++index)
        if (manager->groups[index]->reserved) ++count;
    return count;
}

PursuitRoadBlock* pursuitManagerRoadBlockAt(const PursuitManager* manager, int32_t index)
{
    int32_t current = 0;
    for (size_t group = 0; group < manager->groupCount; ++group) {
        PursuitRoadBlock* block = manager->groups[group]->reserved;
        if (!block) continue;
        if (current == index) return block;
        ++current;
    }
    return NULL;
}

void pursuitChannelPush(PursuitChannelPool* pool, int32_t channel)
{
    if (pool->full) return;
    pool->channels[pool->writeIndex] = channel;
    pool->writeIndex = (pool->writeIndex + 1) & 3;
    pool->full = pool->writeIndex == pool->readIndex;
}

int32_t pursuitChannelPop(PursuitChannelPool* pool, int32_t* channel)
{
    if (!pool->full && pool->readIndex == pool->writeIndex) return 0;
    *channel = pool->channels[pool->readIndex];
    pool->readIndex = (pool->readIndex + 1) & 3;
    pool->full = 0;
    return 1;
}

PursuitCollisionEntry* pursuitCollisionFind(const PursuitCollisionContext* context, const CarMovement* movement)
{
    for (size_t index = 0; index < context->entryCount; ++index)
        if (context->entries[index]->movement == movement) return context->entries[index];
    return NULL;
}

// PAL227BE8..227C48. Snapshot the next link before destroying the current
// strip, exactly as the source intrusive-list traversal does.
WorldServicesResult pursuitManagerRemoveSpikes(PursuitManager* manager,int32_t selector)
{
    if(!manager || !manager->spikes)return WORLD_SERVICES_INVALID_BINDING;
    for(PursuitSpikeStrip* strip=manager->spikes->first;strip;){
        PursuitSpikeStrip* next=strip->next;
        if(strip->ownerSelector==selector)pursuitSpikeDestroy(strip);
        strip=next;
    }
    return manager->result=WORLD_SERVICES_OK;
}


PursuitCollisionEntry* carCameraClosestPursuer(const PursuitManager* manager,
    const CarMovement* target)
{
    const PursuitCollisionContext* context = manager->collision;
    PursuitCollisionEntry* result = NULL;
    float nearest = 100000.0f;
    for (size_t index = 0; index < context->entryCount; ++index) {
        PursuitCollisionEntry* entry = context->entries[index];
        if (!entry || !entry->group || entry->group->target != target) continue;
        const float* first = target->bindings.state->position;
        const float* second = entry->movement->bindings.state->position;
        const float x = first[0] - second[0], y = first[1] - second[1], z = first[2] - second[2];
        const float xx = x * x, yy = y * y, zz = z * z;
        const float distance = sqrtf((xx + yy) + zz);
        if (distance < nearest) { result = entry; nearest = distance; }
    }
    return result;
}

struct PursuitLandmarkCatalog {
    std::vector<PursuitLandmark> landmarks;
};

PursuitLandmarkCatalog* pursuitLandmarksCreate(const uint8_t* track, size_t size)
{
    if (!track || !size) return nullptr;
    try {
        const std::vector<std::uint8_t> data(track,track+size);
        hp2::Chunk encoded{};
        for (const auto& root:hp2::ReadChunks(data)) {
            if (root.id!=0x80034500u) continue;
            for (const auto& child:hp2::ReadChunks(data,root.payloadOffset,root.size)) {
                if (child.id!=0x34530u) continue;
                hp2::Require(encoded.id==0,"Track repeats its pursuit landmark catalog");
                encoded=child;
            }
        }
        hp2::Require(encoded.id!=0 && encoded.size>=4,"Track lacks its pursuit landmark catalog");
        const auto* bytes=data.data()+encoded.payloadOffset;
        const std::uint32_t count=hp2::ReadU32(bytes);
        hp2::Require(count<=(encoded.size-4)/60 && encoded.size==4+static_cast<std::size_t>(count)*60,
            "Pursuit landmark catalog contains a partial record");
        std::unique_ptr<PursuitLandmarkCatalog> catalog(new PursuitLandmarkCatalog);
        catalog->landmarks.resize(count);
        for (std::size_t index=0;index<count;++index) {
            const auto* record=bytes+4+index*60;
            auto& landmark=catalog->landmarks[index];
            const std::uint32_t vertices=hp2::ReadU32(record);
            hp2::Require(vertices<=6,"Pursuit landmark exceeds its authored polygon extent");
            landmark.vertexCount=static_cast<std::int32_t>(vertices);
            for (std::size_t vertex=0;vertex<vertices;++vertex) {
                for (std::size_t axis=0;axis<2;++axis) {
                    const auto bits=hp2::ReadU32(record+4+8*vertex+4*axis);
                    float value;
                    memcpy(&value,&bits,sizeof(value));
                    hp2::Require(std::isfinite(value),"Pursuit landmark has a non-finite coordinate");
                    landmark.vertices[vertex][axis]=value;
                }
            }
            landmark.tag=hp2::ReadU32(record+52);
            const auto uncloak=hp2::ReadU32(record+56);
            memcpy(&landmark.uncloakRoads,&uncloak,sizeof(uncloak));
        }
        return catalog.release();
    } catch (const std::exception&) {
        return nullptr;
    }
}

void pursuitLandmarksDispose(PursuitLandmarkCatalog* catalog) { delete catalog; }
size_t pursuitLandmarkCount(const PursuitLandmarkCatalog* catalog) { return catalog->landmarks.size(); }
const PursuitLandmark* pursuitLandmarkAt(const PursuitLandmarkCatalog* catalog,size_t index)
{ return index<catalog->landmarks.size() ? &catalog->landmarks[index] : nullptr; }

const PursuitLandmark* pursuitLandmarkFind(const PursuitLandmarkCatalog* catalog,const float point[2])
{
    for (const auto& landmark:catalog->landmarks)
        if (pursuitPointInPolygon(point,landmark.vertices,landmark.vertexCount)) return &landmark;
    return nullptr;
}

float pursuitBaseSpeedTrapSpawnTime(World* world,const RaceParameters* parameters,
    const TrackRoute* route,const ApplicationLaunchOptions* launch)
{
    const float laps=parameters->pointToPoint ? 1.0f : static_cast<float>(parameters->totalLaps);
    const float distance=laps*route->length;
    float interval=45.0f;
    if (distance<15000.0f) interval=15.0f;
    else if (distance<30000.0f) interval=30.0f;
    if (parameters->fastSpeedTrapSpawns) return 1.0f;
    if (launch->skipFrontend) return launch->timeUntilCopsAppear;
    return interval+worldRandomFloat(world,interval);
}

static WorldServicesResult resetResult(PursuitManager* manager, WorldServicesResult result)
{
    if (manager) manager->result = result;
    return result;
}

/* Resolve the original car through the one World/frame table. No movement,
 * input or Player state is copied into the manager. */
static CarFrame* resetFrame(const PursuitManager* manager, const Car* car)
{
    if (!manager || !manager->simulation || !manager->world || !car) return nullptr;
    const WorldSimulation* simulation = manager->simulation;
    const size_t index = car->tableIndex;
    if (simulation->world != manager->world || !simulation->frames ||
        index >= simulation->frameCount || index >= WORLD_MAX_CARS ||
        manager->world->carTable[index] != car) return nullptr;
    CarFrame* frame = simulation->frames[index];
    if (!frame || !frame->movement || frame->movement->bindings.owner != car) return nullptr;
    return frame;
}

static DriverAIBase* resetAiDriver(const CarFrame* frame)
{
    CarDriver* driver = frame->environment.driver;
    if (!driver) return nullptr;
    if (driver->kind == CAR_DRIVER_AI && driver->owner.ai) return &driver->owner.ai->base;
    if (driver->kind == CAR_DRIVER_TRAFFIC && driver->owner.traffic) return &driver->owner.traffic->base;
    return nullptr;
}

static WorldServicesResult resetRealHandbrake(CarFrame* frame)
{
    CarDriver* driver = frame->environment.driver;
    if (!driver) return WORLD_SERVICES_INVALID_BINDING;
    if (driver->kind == CAR_DRIVER_REAL) {
        if (!driver->owner.real) return WORLD_SERVICES_INVALID_BINDING;
        realDriverSetHandbrakeTime(&driver->owner.real->input, 0.0f);
    }
    return WORLD_SERVICES_OK;
}

WorldServicesResult pursuitManagerResetCarDamage(PursuitManager* manager, Car* car)
{
    CarFrame* frame = resetFrame(manager, car);
    if (!frame || !manager->collision || !frame->movement->bindings.ownerMotion ||
        car->tableIndex >= WORLD_MAX_CARS)
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    carFrameClearDisabledWheels(frame);
    carMovementSetGripOverride(frame->movement, 1.0f);
    manager->collision->damageLevels[car->tableIndex] = 0.0f;
    if (frame->movement->bindings.damage) carDamageSetZero(frame->movement->bindings.damage);
    return resetResult(manager, resetRealHandbrake(frame));
}

static WorldServicesResult resetFinishCar(PursuitManager* manager, Car* car,
    int32_t transition, int32_t reason)
{
    RaceFrame* race = manager->race;
    if (!race || !race->entries) return WORLD_SERVICES_INVALID_BINDING;
    for (size_t index = 0; index < race->entryCount; ++index) {
        if (race->entries[index] && race->entries[index]->car == car)
            return raceFrameFinishParticipant(race, index, -1, transition, reason);
    }
    return WORLD_SERVICES_INVALID_BINDING;
}

static CarFrame* resetSelectedFrame(PursuitManager* manager)
{
    PursuitCollisionContext* collision = manager->collision;
    if (!collision->entries || collision->activeEntry >= collision->entryCount) return nullptr;
    PursuitCollisionEntry* entry = collision->entries[collision->activeEntry];
    if (!entry || !entry->movement) return nullptr;
    return resetFrame(manager, entry->movement->bindings.owner);
}

static int32_t incrementResetCounter(int32_t value)
{
    const uint32_t bits = static_cast<uint32_t>(value) + 1u;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

WorldServicesResult pursuitManagerResetParticipant(PursuitManager* manager, int32_t index)
{
    if (!manager || !manager->world || !manager->collision || !manager->parameters ||
        !manager->launch || !manager->race || !manager->race->route ||
        index < 0 || index >= 8 || index >= manager->world->totalCars)
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    Car* car = manager->world->carTable[index];
    CarFrame* frame = resetFrame(manager, car);
    if (!frame || !car->driverInfo || !frame->movement->bindings.ownerMotion)
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    PursuitParticipantReset& reset = manager->participantReset[index];
    reset.resetPending = 0;
    reset.spawnDelay = 8.0f;
    reset.preventPursuitSeconds = 0.75f * pursuitBaseSpeedTrapSpawnTime(manager->world,
        manager->parameters, manager->race->route, manager->launch);
    carFrameClearDisabledWheels(frame);
    if (resetRealHandbrake(frame) != WORLD_SERVICES_OK)
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    if (reset.selectedOfficer != -1) {
        if (reset.selectedOfficer < 0 || !manager->collision->entries ||
            static_cast<size_t>(reset.selectedOfficer) >= manager->collision->entryCount)
            return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        PursuitCollisionEntry* officer = manager->collision->entries[reset.selectedOfficer];
        if (!officer) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        if (pursuitCollisionEntryEligible(officer)) pursuitCollisionDoneWritingTicket(officer);
    }

    int32_t finishedAll = 0;
    WorldServicesResult status = WORLD_SERVICES_OK;
    if (reset.finishOnReset) {
        manager->completedPullovers = incrementResetCounter(manager->completedPullovers);
        if (car->driverInfo->driverClass == 2) {
            DriverAIBase* driver = resetAiDriver(frame);
            if (!driver) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
            driverAISetStopDelay(driver, -1.0f);
        }
        int32_t finishSelected = 0;
        int32_t transition = 0;
        int32_t reason = 2;
        if (manager->parameters->specialPursuit) {
            if (frame->environment.player) {
                finishedAll = 1;
                finishSelected = 1;
                reason = 5;
            } else {
                /* PAL2259D4 reads Race+18, the participant count. Race+1C
                 * is the independent finished counter; using it here makes
                 * the final arrested racer fail to finish the human cop. */
                const uint32_t previous = static_cast<uint32_t>(manager->race->entryCount) - 1u;
                finishedAll = previous == static_cast<uint32_t>(manager->completedPullovers);
                transition = !finishedAll;
                finishSelected = finishedAll;
            }
        }
        if (resetFinishCar(manager, car, transition, reason) != WORLD_SERVICES_OK)
            return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
        frame->movement->bindings.ownerMotion->transitionCounter = 0;
        car->driverVisible = 0;
        if (finishSelected) {
            CarFrame* selected = resetSelectedFrame(manager);
            if (!selected || !selected->environment.player || !manager->speech ||
                !manager->race->services)
                return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
            Car* selectedCar = selected->movement->bindings.owner;
            if (resetFinishCar(manager, selectedCar, 0, 4) != WORLD_SERVICES_OK)
                return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
            /* PAL ignores the audio return. Publish score27 even if the
             * native audio queue reports pressure; retain that error after
             * the remaining source work instead of dropping the score. */
            if (worldAudioPlayPursuitFinish(manager->speech, manager->race->services->session,
                manager->parameters->mode, 0, 1, manager->completedPullovers) != WORLD_AUDIO_MESSAGES_OK)
                status = WORLD_SERVICES_CAPACITY;
            carFramePlayerStartScore(selected->environment.player, 27);
            if (selected->environment.player->result != CAR_DRIVER_OK)
                status = WORLD_SERVICES_OWNER_FAILURE;
        }
    } else if (car->driverInfo->driverClass == 2) {
        DriverAIBase* driver = resetAiDriver(frame);
        if (!driver) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        driver->actionCode = 5;
        driverAIReleaseBoost(driver);
        if (driver->result != CAR_DRIVER_OK) return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
    } else if (frame->environment.player) {
        CarDriver* driver = frame->environment.driver;
        if (!driver || !frame->movement->bindings.recoveryPoints)
            return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        CarRecoverySelection road;
        road.group = carDriverCurrentGroup(driver, 1);
        road.pose = carDriverCurrentPose(driver, 1);
        if (!road.group || !road.pose ||
            driverRoadNextPose(frame->movement->bindings.recoveryPoints, &road) != CAR_DRIVER_OK)
            return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
        carMovementRecover(frame->movement, road.group, road.pose, 15.0f, 1.0f, 1, 0);
        if (frame->movement->result != CAR_MOVEMENT_OK)
            return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
    }

    if (manager->parameters->specialPursuit) {
        CarFrame* selected = resetSelectedFrame(manager);
        if (!selected) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        Car* selectedCar = selected->movement->bindings.owner;
        if (reset.restoreOverlay) {
            if (!selected->environment.player || !selected->environment.player->nisOverlay)
                return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
            carNisOverlayFinish(selected->environment.player->nisOverlay);
        }
        if (selectedCar->tableIndex == index) return resetResult(manager, status);
        selected->state.signalEnabled = 0;
        selected->state.signalMode = 0;
        if (finishedAll) return resetResult(manager, status);
        if (manager->parameters->resetPursuitTimerAfterPullover)
            manager->remainingSeconds = static_cast<float>(manager->parameters->timeLimitTicks) *
                0.0002500000118743628f;
        if (frame->environment.player) {
            if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK ||
                pursuitManagerResetCarDamage(manager, selectedCar) != WORLD_SERVICES_OK)
                return resetResult(manager, WORLD_SERVICES_OWNER_FAILURE);
        }
    }
    return resetResult(manager, status);
}

WorldServicesResult pursuitManagerMonitorPullovers(PursuitManager* manager)
{
    if (!manager || !manager->clock) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    for (int32_t index = 0; index < 8; ++index) {
        PursuitParticipantReset& reset = manager->participantReset[index];
        if (!reset.resetPending || reset.waitForCinematic) continue;
        reset.resetDelaySeconds = reset.resetDelaySeconds - manager->clock->worldTimeElapsed;
        if (reset.resetDelaySeconds < 0.0f &&
            pursuitManagerResetParticipant(manager, index) != WORLD_SERVICES_OK)
            return manager->result;
    }
    return resetResult(manager, WORLD_SERVICES_OK);
}

WorldServicesResult pursuitManagerReclaimPatrollers(PursuitManager* manager)
{
    if (!manager || !manager->world || !manager->collision ||
        manager->collision->primaryEntryCount > manager->collision->entryCount ||
        (manager->collision->primaryEntryCount && !manager->collision->entries))
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    for (size_t index = 0; index < manager->collision->primaryEntryCount; ++index) {
        PursuitCollisionEntry* entry = manager->collision->entries[index];
        if (!entry) return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        if (entry->state != 5) continue;
        if (!entry->movement || !entry->movement->bindings.owner)
            return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        const float distance = worldGetDistanceToClosestPlayer2D(manager->world,
            entry->movement->bindings.owner->position);
        if (!(distance < 250.0f)) pursuitCollisionSetState(entry, 1);
    }
    return resetResult(manager, WORLD_SERVICES_OK);
}

WorldServicesResult pursuitManagerGetPulloverCount(const PursuitManager* manager, const Car* car, int32_t* count)
{
    if (!manager || !car || !count || car->tableIndex >= 8)
        return WORLD_SERVICES_INVALID_BINDING;
    *count = manager->participantReset[car->tableIndex].pulloverCount;
    return WORLD_SERVICES_OK;
}

// PAL2289B8..2289F8. This gate belongs to the current Race stage, not the
// session mode. The source selected index addresses the SAME officer table.
WorldServicesResult pursuitManagerSignalHorn(PursuitManager* manager)
{
    if(!manager || !manager->race)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    if(manager->race->state!=4)return resetResult(manager,WORLD_SERVICES_OK);
    if(!manager->collision || !manager->collision->entries ||
        manager->collision->activeEntry>=manager->collision->entryCount ||
        !manager->collision->entries[manager->collision->activeEntry])
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    return resetResult(manager,pursuitExternalEntryChangeChaseTarget(manager,
        manager->collision->entries[manager->collision->activeEntry]));
}

// PAL224A20..224DFC. Preserve the authored race-entry order and World RNG
// draws, including retries which change a participant's timer without spawning.
WorldServicesResult pursuitManagerSpawnSpeedTraps(PursuitManager* manager)
{
    if(!manager || !manager->world || !manager->clock || !manager->collision || !manager->parameters ||
        !manager->race || manager->race->entryCount>8 ||
        (manager->race->entryCount && !manager->race->entries))
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    const int32_t difficulty=manager->collision->severity;
    if(difficulty<0 || difficulty>2 || manager->collision->primaryEntryCount>manager->collision->entryCount ||
        (manager->collision->primaryEntryCount && !manager->collision->entries))
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    static const float retryDelay[3]={10.0f,5.0f,1.0f};
    static const float drawRange[3]={2.0f,1.5f,1.0f};
    const size_t initialGroupCount=manager->groupCount;
    for(size_t index=0;index<manager->race->entryCount;++index) {
        RaceCarEntry* entry=manager->race->entries[index];
        if(!entry || entry->completionReason)continue;
        Car* car=entry->car;
        CarFrame* frame=resetFrame(manager,car);
        if(!frame || car->tableIndex>=8)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        auto& participant=manager->participantReset[car->tableIndex];
        if(!(participant.spawnDelay<=0.0f))continue;
        if(0.0f<participant.preventPursuitSeconds) {
            participant.preventPursuitSeconds-=manager->clock->worldTimeElapsed;continue;
        }
        const float draw=worldRandomFloat(manager->world,drawRange[difficulty]);
        if(1.0f<draw) {participant.preventPursuitSeconds=retryDelay[difficulty];continue;}
        PursuitCollisionGroup* group=nullptr;
        auto result=pursuitManagerFindPursuitForCar(manager,car,&group);
        if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
        bool wanted=false;
        if(group) {
            if(!group->owner)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            wanted=group->activeCount==0 || (group->activeCount<group->owner->copLimit && group->owner->calledForBackup);
        } else if(initialGroupCount<2 || frame->environment.player) {
            RaceCarEntry* leader=manager->race->entries[0];
            if(!leader)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            int32_t gap,behind;
            result=raceFrameCalculateSplitTime(manager->race,&gap,leader,entry);
            if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
            physicsSecondsToTicks(&behind,-15.0f);
            bool ready=behind<gap || leader->completionReason!=0;
            if(!ready)ready=worldRandomBounded(manager->world,20)==19;
            if(!ready)participant.preventPursuitSeconds=1.0f;
            else if(raceParametersIsFreeRun(manager->parameters) || frame->environment.player)wanted=true;
            else {
                const float laps=manager->parameters->pointToPoint ? 1.0f : static_cast<float>(manager->parameters->totalLaps);
                const float progress=finiteRatio(entry->displayProgress,laps);
                const int32_t limit=incrementResetCounter(manager->parameters->pulloverLimit);
                const float used=finiteRatio(static_cast<float>(participant.pulloverCount),static_cast<float>(limit));
                wanted=used<progress;
            }
        }
        if(!wanted)continue;
        // Complete224838 helper, inlined here without an additional body
        // claim: the first primary limbo officer requesting driving state14.
        PursuitCollisionEntry* officer=nullptr;
        for(size_t i=0;i<manager->collision->primaryEntryCount;++i) {
            auto* candidate=manager->collision->entries[i];
            if(!candidate)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            if(candidate->state==1 && candidate->drivingState==14){officer=candidate;break;}
        }
        if(!officer)continue;
        if(!manager->race->route || !manager->simulation || !manager->simulation->pursuitServices ||
            manager->simulation->pursuitServices->world!=manager->world ||
            manager->simulation->pursuitServices->manager!=manager ||
            !manager->simulation->pursuitServices->trackGraph)
            return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        PursuitSpeedTrap* trap=nullptr;
        result=trackGraphFindSpeedTrap(manager->simulation->pursuitServices->trackGraph,manager,car,
            250.0f,manager->race->route->length*0.5f,1,&trap);
        if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
        if(!trap || trap->officer)continue;
        int32_t clear;
        result=pursuitPositionClear(manager->world,trap->position,&clear);
        if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
        if(!clear)continue;
        if(pursuitEntryAssignSpeedTrap(officer,trap)!=CAR_DRIVER_OK)
            return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
        participant.preventPursuitSeconds=20.0f;
        break;
    }
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL2246D0..224838. The mandatory global clock supplies elapsed time;
// the World timestep's float parameter is not substituted for this owner.
WorldServicesResult pursuitManagerThink(PursuitManager* manager)
{
    if(!manager || !manager->world || !manager->clock || !manager->parameters || !manager->race ||
        !manager->collision || manager->collision->manager!=manager)
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    auto result=pursuitManagerTimeBasedSpeech(manager);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitManagerActivateSupervisor(manager);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitManagerPursuitManagement(manager);if(result!=WORLD_SERVICES_OK)return result;
    if(manager->parameters->specialPursuit) {
        result=pursuitManagerReclaimPatrollers(manager);if(result!=WORLD_SERVICES_OK)return result;
    } else {
        result=pursuitManagerSpawnSpeedTraps(manager);if(result!=WORLD_SERVICES_OK)return result;
        int32_t zero;physicsSecondsToTicks(&zero,0.0f);
        if(zero<manager->race->elapsedTicks)for(auto& participant:manager->participantReset) {
            if(0.0f<participant.spawnDelay) {
                participant.spawnDelay-=manager->clock->worldTimeElapsed;
                if(participant.spawnDelay<0.0f)participant.spawnDelay=0.0f;
            }
        }
    }
    result=pursuitManagerMonitorSpikeBelts(manager);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitManagerMonitorPullovers(manager);if(result!=WORLD_SERVICES_OK)return result;
    for(size_t i=0;i<manager->collision->entryCount;++i) {
        if(!manager->collision->entries || !manager->collision->entries[i])
            return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        auto* entry=manager->collision->entries[i];
        CarDriverResult thought;
        if(entry->kind==PURSUIT_COLLISION_OWNED_CAR)thought=pursuitOwnedEntryThink(manager,entry);
        else if(entry->kind==PURSUIT_COLLISION_EXTERNAL_CAR)thought=pursuitExternalEntryThink(manager,entry);
        else return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        if(thought!=CAR_DRIVER_OK)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
    }
    if(manager->parameters->specialPursuit) {
        result=pursuitManagerBeACopThink(manager);if(result!=WORLD_SERVICES_OK)return result;
    }
    // Source227C48 and224498 are proven empty routines, retired with0 count.
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL227F30..228310. The final build removes beta's early victory/score27
// block. Timer completion, damage and mode2's two-car finish remain actual
// calls into the same Race, Player, damage, driver and physical owners.
WorldServicesResult pursuitManagerBeACopThink(PursuitManager* manager)
{
    if(!manager || !manager->race || !manager->parameters || !manager->clock || !manager->collision ||
        !manager->world || manager->world->secondGroupEnd<0 ||
        manager->world->secondGroupEnd>manager->world->totalCars || manager->world->totalCars>WORLD_MAX_CARS)
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    CarFrame* selected=resetSelectedFrame(manager);
    if(!selected || !selected->movement->bindings.owner)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    Car* selectedCar=selected->movement->bindings.owner;
    RaceCarEntry* selectedRaceEntry=selectedCar->raceEntry;
    CarFramePlayerState* selectedPlayer=selected->environment.player;
    PursuitCollisionEntry* selectedOfficer=manager->collision->entries[manager->collision->activeEntry];
    const RaceParameters* parameters=manager->parameters;
    RaceFrame* race=manager->race;
    if(race->state==1 && parameters->mode==2 && !manager->introSpeechSent) {
        manager->introSpeechSent=1;
        if(worldAudioPlayBeACopIntro(manager->speech)!=WORLD_AUDIO_MESSAGES_OK)
            return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
    }
    if(race->state==2 && !manager->beginSpeechSent) {
        manager->beginSpeechSent=1;
        if(!manager->simulation || !manager->simulation->pursuitServices ||
            !manager->simulation->pursuitServices->landmarks || !manager->collision->speech)
            return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        const auto* landmark=pursuitLandmarkFind(manager->simulation->pursuitServices->landmarks,selectedCar->position);
        const auto spoken=worldSpeechRaiseDispatchBeOnLookout(manager->collision->speech,
            landmark ? static_cast<int32_t>(landmark->tag) : 0);
        if(spoken!=WORLD_SPEECH_OK && spoken!=WORLD_SPEECH_SUPPRESSED && spoken!=WORLD_SPEECH_DROPPED)
            return resetResult(manager,spoken==WORLD_SPEECH_INVALID_BINDING ? WORLD_SERVICES_INVALID_BINDING : WORLD_SERVICES_OWNER_FAILURE);
        if(parameters->mode==2) {
            if(!race->timeTable)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            const float laps=parameters->pointToPoint ? 1.0f : static_cast<float>(parameters->totalLaps);
            const float lapTime=race->timeTable->lapSeconds*laps;
            manager->remainingSeconds=lapTime*1.5f;
        }
    }
    int32_t zero;physicsSecondsToTicks(&zero,0.0f);
    if(!(zero<race->elapsedTicks))return resetResult(manager,WORLD_SERVICES_OK);
    if(selectedRaceEntry && !selectedRaceEntry->completionReason) {
        bool otherResetPending=false;
        for(size_t i=0;i<8;++i)if(manager->participantReset[i].resetPending && i!=selectedCar->tableIndex)
            otherResetPending=true;
        if(selectedPlayer && !otherResetPending) {
            manager->remainingSeconds-=manager->clock->worldTimeElapsed;
            if(manager->remainingSeconds<=0.0f) {
                manager->remainingSeconds=0.0f;
                auto status=resetFinishCar(manager,selectedCar,0,5);
                if(status!=WORLD_SERVICES_OK)return resetResult(manager,status);
                if(!manager->collision->speech)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
                if(worldAudioPlayPursuitFinish(manager->speech,manager->collision->speech->session,
                    parameters->mode,1,2,manager->completedPullovers)!=WORLD_AUDIO_MESSAGES_OK)
                    return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
                if(selectedOfficer->group) {
                    status=pursuitManagerDeleteGroup(manager,selectedOfficer->group);
                    if(status!=WORLD_SERVICES_OK)return status;
                    pursuitCollisionSetGroup(selectedOfficer,nullptr);
                }
                if(parameters->mode==2) {
                    Car* other=manager->world->carTable[1];
                    if(other) {
                        if(!other->raceEntry)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
                        if(!other->raceEntry->completionReason) {
                            status=resetFinishCar(manager,other,0,5);
                            if(status!=WORLD_SERVICES_OK)return resetResult(manager,status);
                        }
                    }
                }
            }
        }
    }
    for(int32_t i=0;i<manager->world->secondGroupEnd;++i) {
        Car* car=manager->world->carTable[i];
        if(!car || car->tableIndex>=8)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        const float damage=manager->collision->damageLevels[car->tableIndex];
        if(!(1.0f<=damage))continue;
        CarFrame* frame=resetFrame(manager,car);
        if(!frame || !car->driverInfo)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        if(frame->movement->bindings.damage)carDamageSetFull(frame->movement->bindings.damage);
        if(car->driverInfo->driverClass==2) {
            static const float force[3]={0.3f,0.4f,0.5f};
            const int32_t difficulty=manager->collision->severity;
            DriverAIBase* driver=resetAiDriver(frame);
            if(difficulty<0 || difficulty>2 || !driver)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            driverAIForceBoost(driver,force[difficulty]);
            if(driver->result!=CAR_DRIVER_OK)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        } else if(frame->environment.player && car!=selectedCar) {
            const float loss=damage*0.012500000186264515f;
            const float response=1.0f-loss;
            if(!carSetPursuitResponse(car,response,0.75f))return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        }
    }
    if(parameters->mode==2) {
        if(manager->world->totalCars<2)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        Car* first=manager->world->carTable[0];Car* second=manager->world->carTable[1];
        if(!first || !second || !first->raceEntry || !second->raceEntry)
            return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        if(!first->raceEntry->completionReason && second->raceEntry->completionReason) {
            const auto status=resetFinishCar(manager,first,0,5);
            if(status!=WORLD_SERVICES_OK)return resetResult(manager,status);
            if(!manager->collision->speech)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            if(worldAudioPlayPursuitFinish(manager->speech,manager->collision->speech->session,
                parameters->mode,0,2,manager->completedPullovers)!=WORLD_AUDIO_MESSAGES_OK)
                return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
        }
    }
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL227CF8..227D78. The track-uncloak pass remains live even with no
// manager. It returns after the first matching authored landmark, as source.
WorldServicesResult pursuitManagerPreThink(WorldPursuitServices* services)
{
    if(!services || !services->world || !services->trackGraph || !services->landmarks)
        return WORLD_SERVICES_INVALID_BINDING;
    if(services->manager) {
        if(services->manager->world!=services->world)return WORLD_SERVICES_INVALID_BINDING;
        const auto result=pursuitManagerThink(services->manager);
        if(result!=WORLD_SERVICES_OK)return result;
    }
    if(!trackGraphHasCloakedEdges(services->trackGraph))return WORLD_SERVICES_OK;
    const World* world=services->world;
    if(world->secondGroupEnd<0 || world->secondGroupEnd>world->totalCars || world->totalCars>WORLD_MAX_CARS)
        return WORLD_SERVICES_INVALID_BINDING;
    for(int32_t i=0;i<world->secondGroupEnd;++i) {
        const Car* car=world->carTable[i];if(!car)return WORLD_SERVICES_INVALID_BINDING;
        const auto* landmark=pursuitLandmarkFind(services->landmarks,car->position);
        if(landmark && landmark->uncloakRoads) {trackGraphUncloakAllEdges(services->trackGraph);break;}
    }
    return WORLD_SERVICES_OK;
}

// PAL225C40..225D2C. Prefer a current officer, then an officer assigned to
// this group's road block, then the first primary speed-trap/patrol entry.
WorldServicesResult pursuitManagerSelectArrestingOfficer(PursuitManager* manager,
    const PursuitCollisionGroup* group,PursuitCollisionEntry** officer)
{
    if(!manager || !manager->collision || !officer)return WORLD_SERVICES_INVALID_BINDING;
    *officer=nullptr;
    if(group) {
        if(group->activeCount<0 || group->activeCount>2)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        for(int32_t i=0;i<group->activeCount && !*officer;++i)*officer=group->active[i];
        if(!*officer && group->reserved) {
            for(int32_t i=0;i<2 && !*officer;++i)*officer=group->reserved->entries[i];
        }
    }
    const auto* collision=manager->collision;
    if(collision->primaryEntryCount>collision->entryCount ||
        (collision->primaryEntryCount && !collision->entries))return manager->result=WORLD_SERVICES_INVALID_BINDING;
    for(size_t i=0;i<collision->primaryEntryCount && !*officer;++i) {
        PursuitCollisionEntry* entry=collision->entries[i];
        if(!entry)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        if(entry->state==3 || entry->state==5)*officer=entry;
    }
    if(!*officer && collision->primaryEntryCount)*officer=collision->entries[0];
    return manager->result=*officer ? WORLD_SERVICES_OK : WORLD_SERVICES_INVALID_BINDING;
}

// PAL228538..2286B8. The target supplies the participant timer index; it
// does not supply the positions used to retire distant speed-trap entries.
WorldServicesResult pursuitManagerRemoveSpeedTrapsForCar(PursuitManager* manager,Car* car)
{
    if(!manager || !car || car->tableIndex>=8 || !manager->collision || !manager->world ||
        !manager->parameters || !manager->race || !manager->race->route || !manager->launch ||
        manager->world->firstGroupEnd<0 || manager->world->firstGroupEnd>manager->world->totalCars ||
        manager->world->totalCars>WORLD_MAX_CARS)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    const auto* collision=manager->collision;
    if(collision->primaryEntryCount>collision->entryCount ||
        (collision->primaryEntryCount && !collision->entries))return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    for(size_t i=0;i<collision->primaryEntryCount;++i) {
        PursuitCollisionEntry* entry=collision->entries[i];
        if(!entry || entry->state!=3)continue;
        if(!entry->movement || !entry->movement->bindings.state)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        bool distant=true;
        for(int32_t j=0;j<manager->world->firstGroupEnd;++j) {
            const Car* other=manager->world->carTable[j];
            CarFrame* frame=resetFrame(manager,other);
            if(!frame)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            CarFramePlayerState* player=frame->environment.player;
            if(!player || carFramePlayerReplayActive(player))continue;
            const float* point=frame->movement->bindings.state->position;
            const float* trap=entry->movement->bindings.state->position;
            const float dx=point[0]-trap[0],dy=point[1]-trap[1];
            const float xx=dx*dx,yy=dy*dy;
            if(std::sqrt(xx+yy)<250.0f)distant=false;
        }
        if(distant)pursuitCollisionSetState(entry,1);
    }
    manager->participantReset[car->tableIndex].preventPursuitSeconds=pursuitBaseSpeedTrapSpawnTime(
        manager->world,manager->parameters,manager->race->route,manager->launch);
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL227758..227BE4. Strip expiry, wheel contacts, actual controller
// response and capture all run in one ordered pass over the live owners.
WorldServicesResult pursuitManagerMonitorSpikeBelts(PursuitManager* manager)
{
    if(!manager || !manager->spikes || !manager->world || !manager->clock || !manager->parameters ||
        !manager->collision || manager->world->totalCars<0 || manager->world->totalCars>WORLD_MAX_CARS)
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    const int32_t carCount=manager->world->totalCars;
    for(PursuitSpikeStrip* strip=manager->spikes->first;strip;) {
        PursuitSpikeStrip* next=strip->next;
        if(pursuitSpikeStripAdvance(strip,manager->clock->worldTimeElapsed)) {
            pursuitSpikeDestroy(strip);strip=next;continue;
        }
        const float direction[2]={strip->second[0]-strip->first[0],strip->second[1]-strip->first[1]};
        const float halfX=direction[0]*0.5f,halfY=direction[1]*0.5f;
        const float center[2]={strip->first[0]+halfX,strip->first[1]+halfY};
        for(int32_t index=0;index<carCount;++index) {
            Car* car=manager->world->carTable[index];
            CarFrame* frame=resetFrame(manager,car);
            if(!frame || !frame->movement->bindings.state || !frame->movement->bindings.ownerMotion ||
                !car->driverInfo)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            CarMovement* movement=frame->movement;
            VehicleState* state=movement->bindings.state;
            CarMotionState* motion=movement->bindings.ownerMotion;
            if(!(worldGetDistanceToClosestPlayer2D(manager->world,state->position)<250.0f))continue;
            if(carMotionWindowActive(motion,manager->clock->worldTimer))continue;
            const float dx=center[0]-state->position[0],dy=center[1]-state->position[1];
            const float xx=dx*dx,yy=dy*dy;
            if(!(std::sqrt(xx+yy)<40.0f))continue;
            const float displacement[2]={state->velocity[0]*manager->clock->worldTimeElapsed,
                state->velocity[1]*manager->clock->worldTimeElapsed};
            float worldFrame[16];memcpy(worldFrame,state->orientation,sizeof(worldFrame));
            for(size_t lane=0;lane<4;++lane)worldFrame[12+lane]=state->position[lane];
            worldFrame[15]=1.0f;
            uint8_t hitMask=0;
            for(int32_t wheel=0;wheel<4;++wheel) {
                if(!state->wheelContact[wheel])continue;
                float wheelFrame[16];
                // Source11B248(right,left) composes left*right; the native
                // matrix kernel takes its two operands in result order.
                vectorMatrixMultiply(wheelFrame,worldFrame,state->wheelVisualTransform[wheel]);
                const float wheelPoint[2]={wheelFrame[12],wheelFrame[13]};
                if(pursuitMotionSegmentsIntersect(wheelPoint,displacement,strip->first,direction))
                    hitMask=static_cast<uint8_t>(hitMask | static_cast<uint8_t>(1u<<wheel));
            }
            if((motion->disabledWheelMask&hitMask)==hitMask)continue;
            const int8_t oldCount=frame->state.disabledWheelCount;
            if(carFrameDisableWheels(frame,hitMask)!=CAR_FRAME_OK)
                return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
            if(oldCount)continue;
            if(car->driverInfo->driverClass==2) {
                DriverAIBase* driver=resetAiDriver(frame);
                if(!driver)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
                if(manager->parameters->specialPursuit) {
                    static const float scale[3]={0.3f,0.4f,0.5f};
                    const int32_t difficulty=manager->collision->severity;
                    if(difficulty<0 || difficulty>2)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
                    driverAIForceBoost(driver,scale[difficulty]);
                } else {
                    driverAISetStopDelay(driver,3.0f);driverAIForceBoost(driver,0.4f);
                }
                if(driver->result!=CAR_DRIVER_OK)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            } else if(frame->environment.player && 20.0f<state->forwardSpeed) {
                (void)carFramePlayerStartState(frame->environment.player,4,0,1,1);
                if(frame->environment.player->result)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
            }
            if(manager->parameters->specialPursuit) {
                CarFrame* selected=resetSelectedFrame(manager);
                if(!selected || !selected->environment.player)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
                CarFramePlayerState* selectedPlayer=selected->environment.player;
                if(selected->movement->bindings.owner==car) {
                    const auto result=pursuitManagerCreatePullover(manager,nullptr,car,0);
                    if(result!=WORLD_SERVICES_OK)return result;
                    carFramePlayerStartScore(selectedPlayer,29);
                    if(selectedPlayer->result)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
                } else if(car->raceEntry) {
                    PursuitCollisionGroup* group=nullptr;
                    const auto result=pursuitManagerFindPursuitForCar(manager,car,&group);
                    if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
                    if(group) {
                        carFramePlayerStartScore(selectedPlayer,28);
                        if(selectedPlayer->result)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
                    }
                }
            } else if(frame->environment.player || car->driverInfo->driverClass==2) {
                PursuitCollisionGroup* group=nullptr;
                auto result=pursuitManagerFindPursuitForCar(manager,car,&group);
                if(result!=WORLD_SERVICES_OK)return resetResult(manager,result);
                if(!group) {
                    result=pursuitManagerCreatePullover(manager,nullptr,car,0);
                    if(result!=WORLD_SERVICES_OK)return result;
                }
            }
        }
        strip=next;
    }
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL226F30..2272E4. Timed AI pullovers and actual Player NIS capture share
// this participant row; the byte count is incremented only at the final gate.
WorldServicesResult pursuitManagerCreatePullover(PursuitManager* manager,
    PursuitCollisionGroup* group,Car* car,int32_t countPullover)
{
    if(!manager || !manager->collision || !manager->parameters || !manager->world ||
        !car || car->tableIndex>=8 || !car->driverInfo)
        return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    CarFrame* frame=resetFrame(manager,car);
    if(!frame)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    PursuitParticipantReset& row=manager->participantReset[car->tableIndex];
    row.resetPending=1;row.waitForCinematic=0;row.restoreOverlay=0;
    if(car->driverInfo->driverClass==2) {
        DriverAIBase* driver=resetAiDriver(frame);
        if(!driver)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        driverAISetStopDelay(driver,group ? -1.0f : 3.0f);
    }
    PursuitCollisionEntry* officer=nullptr;
    CarFramePlayerState* player=nullptr;
    if(countPullover) {
        if(manager->parameters->specialPursuit) {
            CarFrame* selected=resetSelectedFrame(manager);
            if(!selected || !selected->environment.player || !manager->race)
                return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            officer=manager->collision->entries[manager->collision->activeEntry];
            player=selected->environment.player;
            if(static_cast<uint32_t>(manager->race->entryCount)-2u==
                static_cast<uint32_t>(manager->completedPullovers) || manager->world->firstGroupEnd==2)
                row.waitForCinematic=1;
            else {
                if(!player->nisOverlay || carNisOverlayStartBusted(player->nisOverlay)!=CAR_NIS_OK)
                    return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
                row.restoreOverlay=1;
            }
        } else {
            player=frame->environment.player;
            if(player) {
                const auto result=pursuitManagerSelectArrestingOfficer(manager,group,&officer);
                if(result!=WORLD_SERVICES_OK)return result;
                row.waitForCinematic=1;
            }
        }
        if(row.pulloverCount>=manager->parameters->pulloverLimit) {
            if(manager->parameters->specialPursuit) {
                row.finishOnReset=1;
                carFramePlayerStartScore(player,26);
                if(player->result)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
            } else if(!raceParametersIsFreeRun(manager->parameters))row.finishOnReset=1;
        }
    }
    if(row.waitForCinematic) {
        if(!officer || !officer->movement || !player)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        if(group) {
            WorldSpeechManager* speech=manager->collision->speech;
            if(!speech)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
            if(worldSpeechFlush(speech)!=WORLD_SPEECH_OK)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
            if(row.finishOnReset) {
                int32_t speaker;
                auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
                if(result!=WORLD_SERVICES_OK)return result;
                const auto spoken=worldSpeechRaiseRacerCaught(speech,static_cast<uint32_t>(speaker),4u,
                    group->hudLevel==4 ? 1 : 2);
                if(spoken!=WORLD_SPEECH_OK && spoken!=WORLD_SPEECH_SUPPRESSED && spoken!=WORLD_SPEECH_DROPPED)
                    return resetResult(manager,spoken==WORLD_SPEECH_INVALID_BINDING ?
                        WORLD_SERVICES_INVALID_BINDING : WORLD_SERVICES_OWNER_FAILURE);
            } else if(worldAudioPlayPullover(manager->speech,speech->session)!=WORLD_AUDIO_MESSAGES_OK)
                return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
        }
        if(pursuitCollisionEntryEligible(officer)) {
            if(group)pursuitCollisionGroupRemove(group,officer);
            pursuitCollisionSetState(officer,7);
            row.selectedOfficer=officer->officerIndex;
        }
        if(!carFramePlayerPlayNextNis(player,frame->movement,officer->movement,
            CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET,static_cast<int32_t>(car->tableIndex)))
            return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
        auto result=pursuitManagerRemoveSpeedTrapsForCar(manager,car);
        if(result!=WORLD_SERVICES_OK)return result;
        if(manager->parameters->specialPursuit && frame->environment.player) {
            carFramePlayerClearPreview(frame->environment.player);
            if(frame->environment.player->result)return resetResult(manager,WORLD_SERVICES_OWNER_FAILURE);
        }
    } else {
        if(group && countPullover) {
            const auto result=pursuitGroupAiTicketSpeech(manager,group);
            if(result!=WORLD_SERVICES_OK)return result;
        }
        row.resetDelaySeconds=row.restoreOverlay ? 2.0f : 7.0f;
        row.selectedOfficer=-1;
    }
    if(countPullover) {
        const uint8_t bits=static_cast<uint8_t>(static_cast<uint8_t>(row.pulloverCount)+1u);
        memcpy(&row.pulloverCount,&bits,sizeof(bits));
    }
    return resetResult(manager,WORLD_SERVICES_OK);
}

// PAL225BD0..225C40. Snapshot the next stable group before Update/Delete.
// Deleting the current ordered-list element never skips its successor.
WorldServicesResult pursuitManagerPursuitManagement(PursuitManager* manager)
{
    if(!manager || manager->groupCount>manager->groupCapacity ||
        (manager->groupCount && !manager->groups))return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
    PursuitCollisionGroup* current=manager->groupCount ? manager->groups[0] : nullptr;
    while(current) {
        size_t index=0;
        while(index<manager->groupCount && manager->groups[index]!=current)++index;
        if(index==manager->groupCount)return resetResult(manager,WORLD_SERVICES_INVALID_BINDING);
        PursuitCollisionGroup* next=index+1<manager->groupCount ? manager->groups[index+1] : nullptr;
        int32_t finished=0;
        auto result=pursuitGroupUpdate(manager,current,&finished);
        if(result!=WORLD_SERVICES_OK)return result;
        if(finished) {
            result=pursuitManagerDeleteGroup(manager,current);
            if(result!=WORLD_SERVICES_OK)return result;
        }
        current=next;
    }
    return resetResult(manager,WORLD_SERVICES_OK);
}

WorldServicesResult pursuitManagerSetNextSpeechGap(PursuitManager* manager)
{
    if (!manager || !manager->parameters)
        return resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
    const float gap = vectorDefaultRandomFloat(3.0f) + 9.0f;
    manager->speechGap = gap;
    if (manager->parameters->specialPursuit) manager->speechGap = gap + 4.0f;
    return resetResult(manager, WORLD_SERVICES_OK);
}

// PAL225660..22577C. Speaking is the real speech-manager/FSM voice state,
// never a queue-nonempty approximation or a copied manager boolean.
WorldServicesResult pursuitManagerTimeBasedSpeech(PursuitManager* manager)
{
    if(!manager || !manager->applicationState || !manager->clock || !manager->collision ||
        !manager->collision->speech || (manager->groupCount && !manager->groups))
        return WORLD_SERVICES_INVALID_BINDING;
    const int32_t state=*manager->applicationState;
    if(state==4 || state==7 || state==5 || state==9)return manager->result=WORLD_SERVICES_OK;
    if(manager->collision->speech->speaking || !manager->groupCount) {
        manager->speechElapsed=0.0f;return manager->result=WORLD_SERVICES_OK;
    }
    manager->speechElapsed+=manager->clock->worldTimeElapsed;
    if(!(manager->speechGap<manager->speechElapsed))return manager->result=WORLD_SERVICES_OK;
    PursuitCollisionGroup* group=pursuitManagerFindPlayerPursuit(manager);
    if(manager->result!=WORLD_SERVICES_OK)return manager->result;
    if(!group || 0.0f<manager->supervisorRemaining) {
        manager->speechElapsed=0.0f;return manager->result=WORLD_SERVICES_OK;
    }
    int32_t imminent=0;
    const auto result=pursuitGroupAboutToSpeak(group,&imminent);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(imminent || group->phase==4 || group->phase==5) {
        manager->speechElapsed=0.0f;return manager->result=WORLD_SERVICES_OK;
    }
    const auto spoken=pursuitSayFillerSpeech(manager,group,&manager->lastFillerEvent);
    if(spoken!=WORLD_SERVICES_OK)return manager->result=spoken;
    manager->speechElapsed=0.0f;
    return pursuitManagerSetNextSpeechGap(manager);
}

PursuitCollisionGroup* pursuitManagerFindPlayerPursuit(PursuitManager* manager)
{
    if (!manager || !manager->parameters || !manager->collision) {
        resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        return nullptr;
    }
    resetResult(manager, WORLD_SERVICES_OK);
    if (manager->parameters->specialPursuit) {
        const PursuitCollisionContext* collision = manager->collision;
        if (collision->activeEntry == SIZE_MAX) return nullptr;
        if (!collision->entries || collision->activeEntry >= collision->entryCount ||
            !collision->entries[collision->activeEntry]) {
            resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
            return nullptr;
        }
        return collision->entries[collision->activeEntry]->group;
    }
    if (manager->groupCount > manager->groupCapacity || (manager->groupCount && !manager->groups)) {
        resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
        return nullptr;
    }
    for (size_t index = 0; index < manager->groupCount; ++index) {
        PursuitCollisionGroup* group = manager->groups[index];
        if (!group || !group->target) {
            resetResult(manager, WORLD_SERVICES_INVALID_BINDING);
            return nullptr;
        }
        if (group->target->bindings.player) return group;
    }
    return nullptr;
}

WorldServicesResult carRecoveryFind(const PursuitManager* manager,
    const Car* car, PursuitCollisionGroup** result)
{
    if (!result) return WORLD_SERVICES_INVALID_BINDING;
    *result = nullptr;
    if (!manager || !car || manager->groupCount > manager->groupCapacity ||
        (manager->groupCount && !manager->groups))
        return WORLD_SERVICES_INVALID_BINDING;
    for (size_t index = 0; index < manager->groupCount; ++index) {
        PursuitCollisionGroup* group = manager->groups[index];
        if (!group || (group->target && !group->target->bindings.owner))
            return WORLD_SERVICES_INVALID_BINDING;
        if (group->target && group->target->bindings.owner == car) {
            *result = group;
            break;
        }
    }
    return WORLD_SERVICES_OK;
}

WorldServicesResult carRecoveryStatus(const PursuitManager* manager,
    const Car* car, int32_t* hudLevel)
{
    if (!hudLevel) return WORLD_SERVICES_INVALID_BINDING;
    PursuitCollisionGroup* group = nullptr;
    const WorldServicesResult result = carRecoveryFind(manager, car, &group);
    if (result != WORLD_SERVICES_OK) return result;
    *hudLevel = group ? group->hudLevel : 0;
    return WORLD_SERVICES_OK;
}

WorldServicesResult pursuitManagerFindPursuitForCar(const PursuitManager* manager,
    const Car* car, PursuitCollisionGroup** result)
{
    return carRecoveryFind(manager, car, result);
}

int32_t carRecoveryFindForMotion(const PursuitManager* manager,
    const CarMotionState* motion, PursuitCollisionGroup** result)
{
    if (!result) return 0;
    *result = nullptr;
    if (!manager || !manager->world || !motion || manager->world->totalCars < 0 ||
        manager->world->totalCars > WORLD_MAX_CARS)
        return 0;
    for (int32_t index = 0; index < manager->world->totalCars; ++index) {
        const Car* car = manager->world->carTable[index];
        if (!car) return 0;
        const CarMovement* movement = static_cast<const CarMovement*>(car->movement);
        if (movement && movement->bindings.ownerMotion == motion)
            return carRecoveryFind(manager, car, result) == WORLD_SERVICES_OK;
    }
    return 0;
}


float driverPursuitRecoveryDirection(const PursuitCollisionContext* pursuit)
{
    if(pursuit->activeEntry>=pursuit->entryCount)return 1.0f;
    const PursuitCollisionEntry* entry=pursuit->entries[pursuit->activeEntry];
    if(!entry||!entry->group)return 1.0f;
    const CarMovement* movement=entry->movement;
    const VehicleState* vehicle=movement->bindings.state;
    const VehicleState* target=entry->group->target->bindings.state;
    const float x=target->position[0]-vehicle->position[0];
    const float y=target->position[1]-vehicle->position[1];
    const float xx=x*x;
    const float yy=y*y;
    if(20.0f<sqrtf(xx+yy))
        return driverCarInHeadingCone(target,vehicle,16384u)?-1.0f:1.0f;
    const CarDriver* controller=(const CarDriver*)movement->bindings.owner->controller;
    const CarRecoveryPose* pose=carDriverCurrentPose(controller,1);
    if(!pose)return 1.0f;
    const float first=vehicle->orientation[0]*pose->direction[0];
    const float second=vehicle->orientation[1]*pose->direction[1];
    return first+second<0.0f?-1.0f:1.0f;
}

PursuitCollisionEntry* pursuitManagerSelectedEntry(const PursuitManager* manager)
{
    const PursuitCollisionContext* collision;
    if (!manager) return NULL;
    collision = manager->collision;
    if (!collision || collision->activeEntry >= collision->entryCount || !collision->entries) return NULL;
    return collision->entries[collision->activeEntry];
}

static WorldServicesResult playRequestResult(PursuitManager* manager, int32_t accepted)
{
    WorldAudioResult result = worldAudioPlayInGame(manager->audio,
        accepted ? "BeACopRequestOk" : "BeACopRequestFailed", 1.0f, 0.0f, 0.0f);
    if (result == WORLD_AUDIO_CAPACITY) manager->result = WORLD_SERVICES_CAPACITY;
    else if (result != WORLD_AUDIO_OK) manager->result = WORLD_SERVICES_INVALID_BINDING;
    return manager->result;
}

WorldServicesResult pursuitManagerRequestChopper(PursuitManager* manager)
{
    PursuitCollisionEntry* entry = pursuitManagerSelectedEntry(manager);
    PursuitCollisionGroup* group;
    int32_t accepted = 0;
    if (!entry || !manager->audio) {
        if (manager) manager->result = WORLD_SERVICES_INVALID_BINDING;
        return WORLD_SERVICES_INVALID_BINDING;
    }
    group = entry->group;
    if (group && !group->chopper && !(group->requestFlags & 0x20u)) {
        group->requestFlags |= 0x1e0u;
        accepted = manager->chopperRequestsRemaining != 0;
    }
    return playRequestResult(manager, accepted);
}

WorldServicesResult pursuitManagerRequestRoadBlock(PursuitManager* manager)
{
    PursuitCollisionEntry* entry = pursuitManagerSelectedEntry(manager);
    PursuitCollisionGroup* group;
    int32_t accepted = 0;
    if (!entry || !manager->audio) {
        if (manager) manager->result = WORLD_SERVICES_INVALID_BINDING;
        return WORLD_SERVICES_INVALID_BINDING;
    }
    group = entry->group;
    if (group && !group->reserved && !(group->requestFlags & 8u)) {
        group->requestFlags |= 0x18u;
        accepted = manager->roadBlocksRemaining != 0;
    }
    return playRequestResult(manager, accepted);
}

// PAL224358..2243F4 and2243F8..224494. The source writes signed catalogue
// indices in one byte. Native pointers and authored site IDs are different.
void pursuitManagerSnapshotRoadBlock(PursuitManager* manager,ReplaySnapshot* snapshot,
    PursuitRoadBlock** block)
{
    hp2::Require(manager && manager->roadBlockCatalog && snapshot && block,
        "Road-block pointer snapshot has no catalogue");
    auto& catalog=*manager->roadBlockCatalog;
    int32_t index=-1;
    if(*block){
        for(size_t at=0;at<catalog.count;++at)if(&catalog.blocks[at]==*block)index=static_cast<int32_t>(at);
        hp2::Require(index>=0,"Road-block snapshot pointer is outside the canonical catalogue");
    }
    replaySnapshotMoveInteger(snapshot,&index,1);
    if(snapshot->restoring){
        hp2::Require(index<0 || static_cast<size_t>(index)<catalog.count,"Road-block snapshot index is invalid");
        *block=index<0?nullptr:&catalog.blocks[static_cast<size_t>(index)];
    }
}

void pursuitManagerSnapshotSpeedTrap(PursuitManager* manager,ReplaySnapshot* snapshot,
    PursuitSpeedTrap** trap)
{
    hp2::Require(manager && manager->speedTrapCatalog && snapshot && trap,
        "Speed-trap pointer snapshot has no catalogue");
    auto& catalog=*manager->speedTrapCatalog;
    int32_t index=-1;
    if(*trap){
        for(size_t at=0;at<catalog.count;++at)if(&catalog.traps[at]==*trap)index=static_cast<int32_t>(at);
        hp2::Require(index>=0,"Speed-trap snapshot pointer is outside the canonical catalogue");
    }
    replaySnapshotMoveInteger(snapshot,&index,1);
    if(snapshot->restoring){
        hp2::Require(index<0 || static_cast<size_t>(index)<catalog.count,"Speed-trap snapshot index is invalid");
        *trap=index<0?nullptr:&catalog.traps[static_cast<size_t>(index)];
    }
}

// PAL2245B8..224644. Groups are reconstructed before this signed-ID lookup;
// the source pool slot/address is never used as a stable group identity.
void pursuitManagerSnapshotGroup(PursuitManager* manager,ReplaySnapshot* snapshot,
    PursuitCollisionGroup** group)
{
    hp2::Require(manager && snapshot && group,"Pursuit pointer snapshot has no manager");
    int8_t id=-1;
    if(!snapshot->restoring && *group){
        hp2::Require((*group)->owner,"Pursuit pointer has no canonical ID owner");
        id=(*group)->owner->id;
    }
    replaySnapshotMoveData(snapshot,&id,1,1,1);
    if(snapshot->restoring){
        *group=nullptr;
        if(id>=0){
            hp2::Require(id<6,"Pursuit snapshot ID exceeds the six-slot source table");
            for(size_t index=0;index<manager->groupCount;++index)
                if(manager->groups[index]->owner->id==id)*group=manager->groups[index];
        }
    }
}

// PAL224648..2246CC. The officer number belongs to the actual manager entry.
void pursuitManagerSnapshotCop(PursuitManager* manager,ReplaySnapshot* snapshot,
    PursuitCollisionEntry** entry)
{
    hp2::Require(manager && manager->collision && snapshot && entry,
        "Cop pointer snapshot has no indexed officer table");
    int32_t index=*entry?(*entry)->officerIndex:-1;
    replaySnapshotMoveInteger(snapshot,&index,1);
    if(snapshot->restoring){
        hp2::Require(index<0 || static_cast<size_t>(index)<manager->collision->entryCount,
            "Cop snapshot index exceeds the source officer table");
        *entry=index<0?nullptr:manager->collision->entries[static_cast<size_t>(index)];
    }
}

// PAL223B70..224354. Complete ordered pursuit/officer/strip graph and the
// real speaker deque. Read-only catalogue counts participate ONLY in checksum
// modes; ordinary restore must not overwrite them with serialized values.
void pursuitManagerDoSnapshot(PursuitManager* manager,ReplaySnapshot* snapshot)
{
    hp2::Require(manager && snapshot && manager->collision && manager->world && manager->parameters &&
        manager->roadBlockCatalog && manager->speedTrapCatalog && manager->spikes,
        "CopManager snapshot has incomplete canonical owners");
    auto& collision=*manager->collision;
    auto& blocks=*manager->roadBlockCatalog;
    auto& traps=*manager->speedTrapCatalog;
    replaySnapshotBeginSection(snapshot,manager,"tCopManager");
    int32_t aiCount=static_cast<int32_t>(collision.primaryEntryCount);
    int32_t blockCount=static_cast<int32_t>(blocks.count),trapCount=static_cast<int32_t>(traps.count);
    replaySnapshotVerifyData(snapshot,&aiCount,4,4,1);
    replaySnapshotVerifyData(snapshot,&blockCount,4,4,1);
    replaySnapshotVerifyData(snapshot,&trapCount,4,4,1);
    if(snapshot->restoring){
        for(size_t index=0;index<blocks.count;++index)
            blocks.blocks[index].entries[0]=blocks.blocks[index].entries[1]=nullptr;
        for(size_t index=0;index<traps.count;++index)traps.traps[index].officer=nullptr;
    }
    uint8_t groupCount=static_cast<uint8_t>(manager->groupCount);
    replaySnapshotMoveData(snapshot,&groupCount,1,1,0);
    hp2::Require(groupCount<=6,"CopManager snapshot has too many pursuits");
    if(snapshot->restoring){
        while(manager->groupCount)
            hp2::Require(pursuitManagerDeleteGroup(manager,manager->groups[0])==WORLD_SERVICES_OK,
                "CopManager snapshot cannot destroy the previous pursuit graph");
        for(uint8_t index=0;index<groupCount;++index){
            int8_t id=-1;
            replaySnapshotMoveData(snapshot,&id,1,1,1);
            PursuitCollisionGroup* group=nullptr;
            hp2::Require(pursuitManagerAllocateGroup(manager,id,nullptr,&group)==WORLD_SERVICES_OK,
                "CopManager snapshot cannot reconstruct its original pursuit pool");
        }
    } else {
        for(size_t index=0;index<manager->groupCount;++index){
            int8_t id=manager->groups[index]->owner->id;
            replaySnapshotMoveData(snapshot,&id,1,1,1);
        }
    }
    for(size_t index=0;index<manager->groupCount;++index)
        pursuitGroupDoSnapshot(manager->groups[index],snapshot);
    // Source's inter-stage CheckPoint13D0F8 is JRRA/NOP, with no state change.
    for(size_t index=0;index<collision.entryCount;++index){
        auto* entry=collision.entries[index];
        hp2::Require(entry,"CopManager snapshot contains an absent actual officer");
        if(entry->kind==PURSUIT_COLLISION_OWNED_CAR)pursuitAiCopDoSnapshot(manager,entry,snapshot);
        else pursuitPlayerCopDoSnapshot(manager,entry,snapshot);
    }
    int32_t stripCount=0;
    for(auto* strip=manager->spikes->first;strip;strip=strip->next)++stripCount;
    replaySnapshotMoveInteger(snapshot,&stripCount,4);
    hp2::Require(stripCount>=0,"CopManager snapshot has a negative spike strip count");
    if(snapshot->restoring){
        // The source recreates the recorded count through the expandable pool,
        // not a10-strip limit. Bound native allocation by the actual source
        // payload (8+8+4+4+1+4 bytes each), preserving malformed-data checks.
        hp2::Require(snapshot->result==REPLAY_SNAPSHOT_OK && snapshot->cursor>=0 &&
            snapshot->dataSize>=snapshot->cursor && static_cast<size_t>(snapshot->dataSize)<=snapshot->capacity &&
            static_cast<size_t>(stripCount)<=static_cast<size_t>(snapshot->dataSize-snapshot->cursor)/29u,
            "CopManager spike strip count exceeds its captured data");
        while(manager->spikes->first)pursuitSpikeDestroy(manager->spikes->first);
        for(int32_t index=0;index<stripCount;++index)
            hp2::Require(pursuitSpikeAllocate(manager->spikes)!=nullptr,
                "CopManager snapshot cannot allocate its actual spike/model pool");
    }
    for(auto* strip=manager->spikes->first;strip;strip=strip->next)pursuitSpikeStripDoSnapshot(strip,snapshot);
    for(auto& row:manager->participantReset){
        replaySnapshotMoveData(snapshot,&row.resetPending,1,4,0);
        replaySnapshotMoveData(snapshot,&row.waitForCinematic,1,4,0);
        replaySnapshotMoveData(snapshot,&row.finishOnReset,1,4,0);
        replaySnapshotMoveFloat(snapshot,&row.resetDelaySeconds);
        replaySnapshotMoveData(snapshot,&row.pulloverCount,1,1,1);
        replaySnapshotMoveInteger(snapshot,&row.selectedOfficer,4);
    }
    replaySnapshotMoveData(snapshot,&manager->chopperReserved,1,4,0);
    auto& channels=collision.channels;
    hp2::Require(channels.readIndex>=0 && channels.readIndex<4 && channels.writeIndex>=0 && channels.writeIndex<4,
        "CopManager snapshot speaker deque is invalid");
    if(snapshot->restoring){
        channels.readIndex=channels.writeIndex;channels.full=0;
        for(int index=0;;++index){
            uint32_t channel=6;
            replaySnapshotMoveUnsigned(snapshot,&channel,4);
            if(channel==6)break;
            hp2::Require(index<4 && channel<4,"CopManager snapshot speaker sequence is malformed");
            // PAL224060..224098 restores at the front of the deque.
            channels.readIndex=(channels.readIndex+3)&3;
            channels.channels[channels.readIndex]=static_cast<int32_t>(channel);
            channels.full=channels.readIndex==channels.writeIndex;
        }
    } else {
        const int32_t count=channels.full?4:((channels.writeIndex-channels.readIndex)&3);
        for(int index=0;index<count;++index){
            // PAL224144..2241C8 emits back-to-front, leaving the logical
            // deque unchanged after capture. Reserve/release remain FIFO.
            uint32_t value=static_cast<uint32_t>(channels.channels[(channels.writeIndex+3-index)&3]);
            replaySnapshotMoveUnsigned(snapshot,&value,4);
        }
        uint32_t terminator=6;replaySnapshotMoveUnsigned(snapshot,&terminator,4);
    }
    // PAL224214/224248 read World+24: the boundary after human and AI
    // racers. World+20 (firstGroupEnd) includes only human drivers.
    hp2::Require(manager->world->secondGroupEnd>=0 && manager->world->secondGroupEnd<=8,
        "CopManager snapshot World racer count is invalid");
    for(int index=0;index<manager->world->secondGroupEnd;++index){
        // PAL224228/22422C transfers +2874 (spawn), then224238/22423C
        // transfers +2894 (prevent). Reset2258D4/2258F0 and Spawn224B30/
        // 224B48 establish these owners independently of the host layout.
        replaySnapshotMoveFloat(snapshot,&manager->participantReset[index].spawnDelay);
        replaySnapshotMoveFloat(snapshot,&manager->participantReset[index].preventPursuitSeconds);
    }
    if(manager->parameters->specialPursuit){
        for(float& damage:collision.damageLevels)replaySnapshotMoveFloat(snapshot,&damage);
        replaySnapshotMoveFloat(snapshot,&manager->remainingSeconds);
    }
    replaySnapshotMoveData(snapshot,&manager->beginSpeechSent,1,4,0);
    replaySnapshotMoveData(snapshot,&manager->introSpeechSent,1,4,0);
    replaySnapshotMoveInteger(snapshot,&manager->roadBlocksRemaining,4);
    replaySnapshotMoveInteger(snapshot,&manager->chopperRequestsRemaining,4);
    replaySnapshotMoveInteger(snapshot,&manager->completedPullovers,4);
    replaySnapshotEndSection(snapshot);
}

// Native typed projection initialization under the actual CopManager constructor; no separate PAL body claim.
void pursuitCollisionInitialize(PursuitCollisionContext* context)
{
    memset(context, 0, sizeof(*context));
    context->activeEntry = SIZE_MAX;
    context->timing = &pursuitTimingTuning;
}

// Shared owner-local XY query used by MonitorImpact; final PAL operands retain scalar order.
static float distance(const CarMovement* first, const CarMovement* second)
{
    const float* a = first->bindings.state->position;
    const float* b = second->bindings.state->position;
    const float x = a[0] - b[0], y = a[1] - b[1];
    const float squaredX = x * x, squaredY = y * y;
    return sqrtf(squaredX + squaredY);
}

// PAL228310..228520 MonitorImpact. Only the selected chase target receives BeACop damage; mode2 and the final grace gate remain source-defined.
PursuitCollisionResult pursuitCollisionRecord(PursuitCollisionContext* context, CarMovement* movement,
                                             float magnitude, int32_t carContact)
{
    if (!context) return PURSUIT_COLLISION_OK;
    context->result = PURSUIT_COLLISION_OK;
    if (!context->parameters) return context->result = PURSUIT_COLLISION_INVALID_BINDING;
    if (context->severity < 0 || context->severity > 2) return context->result = PURSUIT_COLLISION_INVALID_BINDING;
    static const float thresholds[3] = {40.0f, 60.0f, 75.0f};
    const float threshold = thresholds[context->severity];
    if (context->parameters->specialPursuit) {
        if (context->activeEntry >= context->entryCount) return context->result = PURSUIT_COLLISION_INVALID_BINDING;
        PursuitCollisionEntry* entry = context->entries[context->activeEntry];
        if (!entry || !entry->group || entry->group->target != movement) return context->result;
        if (context->parameters->mode != 2 && !(distance(entry->movement, movement) < 50.0f)) return context->result;
        if (!(5.0f <= magnitude)) return context->result;
        const uint16_t index = movement->bindings.owner->tableIndex;
        if (index >= WORLD_MAX_CARS) return context->result = PURSUIT_COLLISION_INVALID_BINDING;
        if (threshold <= magnitude) context->damageLevels[index] = 1.0f;
        else {
            static const float factors[3] = {100.0f, 4.0f, 2.5f};
            float factor = factors[context->severity] * 0.004999999888241291f;
            if (context->parameters->mode == 2) factor = 0.009999999776482582f;
            const float increase = minimumFinite(magnitude * factor, 1.0f);
            context->damageLevels[index] = minimumFinite(context->damageLevels[index] + increase, 1.0f);
        }
    } else if (movement->bindings.ownerMotion->inputCategory == 4) {
        if (carContact) magnitude = magnitude * 1.5f;
        if (threshold <= magnitude) {
            PursuitCollisionEntry* entry = pursuitCollisionFind(context, movement);
            if (entry && pursuitCollisionEntryEligible(entry) && entry->graceRemaining <= 0.0f)
                pursuitCollisionDisable(context, entry, 1);
        }
    }
    return context->result;
}

// Actual manager group pools have six slots. The native owning slots and
// ordered list below retain two views of the same objects, never copies.
struct PursuitGroupStorage {
    PursuitGroupBindings bindings;
    std::array<PursuitGroupState,6> objects;
    std::array<PursuitGroupState*,6> byId;
    std::array<PursuitCollisionGroup*,6> ordered;
    std::array<int32_t,6> nextFree;
    int32_t firstFree;
};

WorldServicesResult pursuitManagerBindGroupStorage(PursuitManager* manager,
    const PursuitGroupBindings* bindings)
{
    if(!manager || !bindings || manager->ownedGroups || manager->groupCount ||
        !bindings->collision || !bindings->topology || !bindings->collisionResources ||
        !bindings->carResources || !bindings->language || !bindings->route ||
        !manager->collision || !manager->parameters || !manager->applicationState || !manager->clock)
        return WORLD_SERVICES_INVALID_BINDING;
    auto* storage=new(std::nothrow) PursuitGroupStorage{};
    if(!storage)return manager->result=WORLD_SERVICES_CAPACITY;
    storage->bindings=*bindings;
    storage->firstFree=0;
    for(int32_t index=0;index<6;++index)storage->nextFree[static_cast<size_t>(index)]=index==5 ? -1 : index+1;
    manager->ownedGroups=storage;
    manager->groups=storage->ordered.data();
    manager->groupCount=0;
    manager->groupCapacity=storage->ordered.size();
    return manager->result=WORLD_SERVICES_OK;
}

// PAL2244A0..2244E0. Signed IDs are independent of physical pool slots.
WorldServicesResult pursuitManagerNextPursuitId(const PursuitManager* manager,int32_t* id)
{
    if(!manager || !manager->ownedGroups || !id)return WORLD_SERVICES_INVALID_BINDING;
    for(int32_t index=0;index<6;++index){
        if(!manager->ownedGroups->byId[static_cast<size_t>(index)]){*id=index;return WORLD_SERVICES_OK;}
    }
    *id=-1;
    return WORLD_SERVICES_OK;
}

// PAL2244E0..22455C. Allocation, constructor, indexed publication and
// ordered tail publication retain the actual source order.
WorldServicesResult pursuitManagerAllocateGroup(PursuitManager* manager,int32_t id,
    CarMovement* target,PursuitCollisionGroup** output)
{
    if(!manager || !output || !manager->ownedGroups || id<0 || id>=6 ||
        manager->groupCount>=6 || manager->ownedGroups->byId[static_cast<size_t>(id)])
        return WORLD_SERVICES_INVALID_BINDING;
    *output=nullptr;
    auto* storage=manager->ownedGroups;
    const int32_t slot=storage->firstFree;
    if(slot<0)return manager->result=WORLD_SERVICES_CAPACITY;
    storage->firstFree=storage->nextFree[static_cast<size_t>(slot)];
    PursuitGroupState* state=&storage->objects[static_cast<size_t>(slot)];
    // Original CreatePursuit calls bOMalloc, whose default pool flags clear
    // the entire allocation before the constructor (including reused slots).
    // The constructor itself intentionally leaves several fields unwritten.
    *state=PursuitGroupState{};
    const auto result=pursuitGroupInitialize(manager,state,&storage->bindings,id,target);
    if(result!=WORLD_SERVICES_OK){
        storage->nextFree[static_cast<size_t>(slot)]=storage->firstFree;
        storage->firstFree=slot;
        return manager->result=result;
    }
    PursuitCollisionGroup* group=&state->group;
    storage->ordered[manager->groupCount++]=group;
    storage->byId[static_cast<size_t>(id)]=state;
    *output=group;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL224560..2245B8 followed by the actual group destructor229198..229288.
// Slot memory is cleared by the manager allocation boundary before reuse.
WorldServicesResult pursuitManagerDeleteGroup(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!manager || !manager->ownedGroups)return WORLD_SERVICES_INVALID_BINDING;
    if(!group)return manager->result=WORLD_SERVICES_OK;
    auto* storage=manager->ownedGroups;
    PursuitGroupState* state=group->owner;
    if(!state || state->manager!=manager || state->id<0 || state->id>=6 ||
        storage->byId[static_cast<size_t>(state->id)]!=state)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    size_t order=0;
    while(order<manager->groupCount && storage->ordered[order]!=group)++order;
    if(order==manager->groupCount)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    for(size_t index=order+1;index<manager->groupCount;++index)storage->ordered[index-1]=storage->ordered[index];
    storage->ordered[--manager->groupCount]=nullptr;
    storage->byId[static_cast<size_t>(state->id)]=nullptr;
    const auto released=pursuitGroupReleaseAll(manager,group);
    if(released!=WORLD_SERVICES_OK)return released;
    if(group->target->bindings.player){
        pursuitChannelPush(&manager->collision->channels,group->currentChannel);
        if(group->nextChannel!=5)pursuitChannelPush(&manager->collision->channels,group->nextChannel);
    }
    state->id=-2;
    if(state->barricades){
        const auto result=pursuitBarricadesDispose(state->barricades);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        state->barricades=nullptr;
    }
    const auto slot=state-storage->objects.data();
    if(slot<0 || slot>=6)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    storage->nextFree[static_cast<size_t>(slot)]=storage->firstFree;
    storage->firstFree=static_cast<int32_t>(slot);
    return manager->result=WORLD_SERVICES_OK;
}

WorldServicesResult pursuitManagerDisposeGroupStorage(PursuitManager* manager)
{
    if(!manager || !manager->ownedGroups)return WORLD_SERVICES_INVALID_BINDING;
    while(manager->groupCount){
        const auto result=pursuitManagerDeleteGroup(manager,manager->groups[0]);
        if(result!=WORLD_SERVICES_OK)return result;
    }
    delete manager->ownedGroups;
    manager->ownedGroups=nullptr;manager->groups=nullptr;manager->groupCapacity=0;
    return manager->result=WORLD_SERVICES_OK;
}

#pragma once
// Port-owned source-data adapter for original Smokeable loading/spawn filtering.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include <map>
#include <vector>
#include "snapshot_services.hpp"

namespace port::race {
struct SmokeableResources {
    // Original SceneryInstance ExcludeFlags in original per-section index order.
    // This is metadata, not proof that eModel/collision scenery objects exist.
    std::map<int,std::vector<std::uint16_t>> section_exclude_flags;
    // Full source-derived header/SceneryInfo/SceneryInstance records in shared
    // Memory. Model-pointer fixups/rendering are outside this collision ctor.
    std::map<int,std::uint32_t> section_headers;
    std::vector<std::uint32_t> section_load_order;
    std::vector<std::uint32_t> info_records;
    std::vector<std::uint32_t> owned_allocations;
    bool spawner_data_loaded=false;
    bool source_data_loaded=false;
    bool scenery_models_initialized=false;
};
// Port selective original LoaderScenery0014E920 model-construction component.
// Call after actual solid resources are loaded; retained raw section ownership
// remains with SmokeableResources. Not whole scenery renderer/cull initialization.
void initialize_scenery_models(::port::driving::Memory&,SmokeableResources&);
void close_scenery_models(::port::driving::Memory&,SmokeableResources&);
// Walk both complete original sources. Original chunks34026/34027 and section
// metadata34101/34103 establish counts; an absent chunk is not a made-up spawner.
// All original pointers/Info records are mapped into the same World Memory.
SmokeableResources load_smokeable_resources(::port::driving::Memory&,
    const std::vector<std::uint8_t>& global_bundle,const std::vector<std::uint8_t>& track_bundle,bool reverse=false);
// Port-only source ownership teardown, after original ClearSmokeables. Uses
// original Info detachment/spawner reset00219DE0 and section-table reset0014EBD8;
// no rendering/model ownership or full original UnloaderScenery is asserted.
void unload_smokeable_resources(::port::driving::Memory&,SmokeableResources&);
}
namespace hp2::debug {
// Debug001D1280 /001D1248; E-0099-W12. Explicit original static terrain hashes.
void Sub_001D1280(::port::driving::Memory&);
int MatchTerrainType(const ::port::driving::Memory&,std::uint32_t hash);
// Debug0021C5C8 /0021A4B0; E-0099-W12.
void Sub_0021C5C8(::port::driving::Memory&);
// Debug0021A1F8/0021A220, E-0099-W57. Real source periodic collector.
void SmokeableThink(::port::driving::Memory&);
void DeleteFarAwaySmokeableCollisionBodies(::port::driving::Memory&);
std::uint32_t FindSmokeableInfo(const ::port::driving::Memory&,std::uint32_t hash);
class VirginSmokeable : public StaticCollisionBody {
public:
    using StaticCollisionBody::StaticCollisionBody;
    // Debug0021A9F8 /0021ABB8, E-0099-W15. Real shared static collision object.
    void Sub_0021A9F8(std::uint32_t spawner);
    void Sub_0021ABB8(int in_chrg);
    void DoSnapshot(std::uint32_t); //0021AC40 E-0099-W76.
    std::uint32_t ConvertToSmokeable(const ::port::race::PhysicsConstructionServices&); //0021AC90,W83.
};
// Debug0021A4E8; E-0099-W12/W15. Actual pool-backed VirginSmokeable objects,
// collision registration, original bounding-box ordering and scenery links.
void SpawnSmokeables(::port::driving::Memory&,const ::port::race::SmokeableResources&,
    const ::port::race::EmitterGroupLookup&);
// Debug0021A118 /0021A658; E-0099-W15/W57. Actual Virgin and known Smokeable
// destruction; unresolved derived targets/child cleanup retain reached guards.
void DeleteSmokeableCollisionBodies(::port::driving::Memory&);
void ClearSmokeables(::port::driving::Memory&);
std::uint32_t ConvertToSmokeable(::port::driving::Memory&,std::uint32_t,const ::port::race::PhysicsConstructionServices&); //0021A9A8,W83.
}

namespace hp2::debug {
int GetSmokeableSpawnerNumber(const ::port::driving::Memory&,std::uint32_t); //00219C28, E-0099-W76.
std::uint32_t GetSmokeableSpawner(const ::port::driving::Memory&,int); //00219C08.
void SnapshotAllSmokeables(::port::driving::Memory&,std::uint32_t,const ::port::race::SnapshotServices& ={}); //0021A6E0.
}

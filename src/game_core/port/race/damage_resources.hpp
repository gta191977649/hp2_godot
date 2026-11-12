#pragma once
// TASK-0099 E-0099-W22; explicit host ownership of loaded damage/marker chunks.
// Debug NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/object_view.hpp"
#include <filesystem>
#include <vector>
namespace hp2::debug {
using port::driving::Memory;
class BodyDamage:public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //001B20E8, E-0099-W74.
 void Sub_001B2030(); // Debug001B2030; name is unused in original constructor.
 void SetALevelBodyDamageInfo(const char*); // Debug001B2168.
 void AddModel(std::uint32_t model,const char* damage_info_name,int damage_polys_only); // Debug001B21A0.
 int GetSectorNumber(std::uint32_t position); // Debug001B22A0.
 float GetVisibleTotalDamage()const; //Debug001B2558, E-0099-W42.
 void SetDamage(int sector,float amount); //001B21D8, E-0099-W38
 void RepairAllDamage(); //001B2250
 float GetVisibleDamageAmount(int sector)const; //001B24F8
 int GetCompressedVisibleDamageAmount(int sector)const; //001B2538
 void SetEnvironmentMapping(std::uint32_t,int); //001B26E0, E-0099-W38
 void ApplyDamage(); //001B2598; actual W25 model vertex callback
 void Sub_001B20A0(); // Debug001B20A0 embedded destructor; actual RepairAllDamage/ApplyDamage.
};
class CarDamage:public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //001B2AC8, E-0099-W74.
 void Sub_001B2940(std::uint32_t car); // Debug001B2940.
 void InitSectorPositions(); // Debug001B2A28.
 int GetDamageAmount(std::uint32_t breakable_bits,int item)const; //inline00124EB8, E-0099-W68.
 //001B2B40, E-0099-W38. Real sector spread, breakable effects and application.
 void AddDamage(std::uint32_t position,float intensity);
 void ApplyDamage(); //001B2F38, E-0099-W38
 void AddDamageToSector(int,float,float); //001B2D58, E-0099-W38
 void RethinkDamage(); //001B2F08, E-0099-W81.
 void SetDamageToFull(); //001B3058,E-0112-COP02, actual32-sector mutation/rethink chain.
 void SetDamageToZero(); //001B30B0,E-0110-P01.
 std::uint32_t RecalculateBreakableDamage(); //001B2C30, E-0099-W38
 void Sub_001B29C8(); // Debug001B29C8 actual breakable/vertex restoration; does not free this.
};
std::uint32_t eFindPositionMarker(Memory&,std::uint32_t name_hash,std::uint32_t type_hash,std::uint32_t previous_marker=0); //001A5758.
int LoaderPositionMarkers(Memory&,std::uint32_t chunk); //001A5670; full original chunk mapped.
int UnloaderPositionMarkers(Memory&,std::uint32_t chunk); //001A56E8.
int LoaderBodyDamage(Memory&,std::uint32_t chunk); //001B1BF0.
int UnloaderBodyDamage(Memory&,std::uint32_t chunk); //001B1C40.
std::uint32_t FindBodyDamageInfo(Memory&,std::uint32_t hash); //001B1C78.
}
namespace port::race {
// Port explicit process initialization: complete original001B28D0 damage lists,
// and PositionMarkerList's required stores from001A5950. Other frame lists excluded.
void initialize_damage_resource_lists(port::driving::Memory&);
class DamageResourceBindings {
 port::driving::Memory& m_;
 std::vector<std::uint32_t> allocations_,chunks_;
public:
 explicit DamageResourceBindings(port::driving::Memory& m):m_(m){}
 void load(const std::filesystem::path&); // Original raw eLab geometry bundle, no alternate records.
 void close(); // Unlinks chunks before releasing owned bytes; damage objects must be closed first.
 std::size_t loaded_chunks()const{return chunks_.size();}
};
}

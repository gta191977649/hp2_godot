#pragma once
// Port-only ownership context for authored car/in-game sound metadata and data.
// TASK-0099 E-0099-W31. No replacement AudioMixer/control behavior is implied.
#include "game_core/port/driving/memory.hpp"
#include <filesystem>
#include <string>
#include <memory>
namespace port::race {
class PcmCommandDevice;struct DecodedSoundPatch;
class CarSoundResources {
 struct Bank {std::string name;std::uint32_t region=0,header=0;std::vector<std::uint8_t> metadata,data;std::vector<std::uint32_t> samples;};
 driving::Memory& memory_;std::vector<Bank> banks_;std::vector<std::uint32_t> allocations_,sounds_;std::vector<std::uint8_t> definitions_[2],retail_track_info_;std::vector<std::shared_ptr<const DecodedSoundPatch>> decoded_patches_;std::uint32_t track_sound_info_=0;int selected_track_=0;bool initialized_=false,software_voices_=false;
 bool track_hud_fields_=false;
 std::uint32_t copy_bytes(const std::vector<std::uint8_t>&);
 void initialize_vector(std::uint32_t);void append_pointer(std::uint32_t,std::uint32_t);
public:
 explicit CarSoundResources(driving::Memory& m):memory_(m){}
 CarSoundResources(const CarSoundResources&)=delete;
 CarSoundResources& operator=(const CarSoundResources&)=delete;
 // Import complete authored MSH/MSB records into real region/sample lookup
 // objects. This is a port data owner, not a claim of asynchronous SPU loading.
 // Requires the actual bSample pool already initialized by sound startup.
 void initialize_lookup_state();
 // Original software components and consumed raw SDK lookup tables only.
 // No RPC, SPU transfer, flush or device consumption is performed.
 void initialize_software_voices();
 bool software_voices_ready()const{return software_voices_;}
 std::shared_ptr<const DecodedSoundPatch> decode_physical_patch(std::uint16_t);
 // Explicit PC replacement for RPC consumption; caller supplies the original
 // hardware ticker operand. EE counters reset only after concrete consumption.
 int flush_audio(PcmCommandDevice&,std::uint32_t ticker,int transfer_flag=0);
 void load_region(const std::filesystem::path& zzdata_root,const std::string& original_name);
 void load_engine_definitions(const std::filesystem::path& zzdata_root);
 // Sound-only Debug projection: complete Retail34201 retained; only the
 // reviewed OverrideStartingRouteForAI/SoundRegion fields are mapped.
 std::uint32_t track_info(int one_based_track)const;
 std::string load_track_sound_info(const std::vector<std::uint8_t>& global_bundle,int one_based_track);
 // E-0112-M01: additive reviewed prefix for actual map/record consumers;
 // retain the sparse sound-only mapping for independent sound fixtures.
 void enable_track_hud_info();
 std::uint32_t in_game_sample(const std::string&);
 std::uint32_t region(const std::string& name)const;
 std::uint32_t sample(const std::string& bank,const std::string& name)const;
 std::uint16_t channels(const std::string& bank,const std::string& name)const;
 std::uint32_t engine_definition(unsigned index);
 bool source_ready()const;
 void register_sound(std::uint32_t);void unregister_sound(std::uint32_t);
 // Port owner teardown preparation, not an original global StopAll routine.
 // After game sound objects close, invoke actual bSample.Stop on each owned
 // resident source sample; callers consume those real Stop packets before close.
 void stop_resident_samples();
 void close(); // Actual sound objects and voices must close before borrowed data.
};
}
namespace hp2::debug::sound {
// Debug0018CB30/0018CB78/0018CBC0. Actual logical BaseCarSound object addresses.
std::uint32_t CreatePlayerCarSound(::port::driving::Memory&,std::uint32_t car,::port::race::CarSoundResources&);
std::uint32_t CreateAICarSound(::port::driving::Memory&,std::uint32_t car,::port::race::CarSoundResources&);
std::uint32_t CreateAICopCarSound(::port::driving::Memory&,std::uint32_t car,::port::race::CarSoundResources&);
void DeleteCarSound(::port::driving::Memory&,std::uint32_t sound);
void DeleteCarSound(::port::driving::Memory&,std::uint32_t sound,::port::race::CarSoundResources&);
}

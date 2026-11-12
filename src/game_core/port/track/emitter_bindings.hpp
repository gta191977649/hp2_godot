#pragma once
// Port-owned lifetime of original AcidFX resource records in shared Memory.
// TASK-0099 E-0099-W10; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/memory.hpp"
#include <vector>

namespace hp2::debug {
// Debug00126EC8/00126F60; E-0099-W10. afxClose intentionally retains
// AcidEmitterSlotPool, exactly as the binary does.
void afxInit(port::driving::Memory&);
void afxClose(port::driving::Memory&);
// Debug00127278/001272F0/00127380/00127438/001274C0; E-0099-W10.
std::uint32_t afxGetEmitterData(port::driving::Memory&,std::uint32_t name,
                              std::uint32_t group,std::uint32_t previous=0);
std::uint32_t afxGetEmitter(port::driving::Memory&,std::uint32_t name,
                          std::uint32_t group,std::uint32_t previous=0);
std::uint32_t afxCopyEmitter(port::driving::Memory&,std::uint32_t source);
void afxGetEmitters(port::driving::Memory&,std::uint32_t list,std::uint32_t group);
void afxDeleteEmitters(port::driving::Memory&,std::uint32_t list);
// Debug00127800/0012A3B0/0012A418; E-0099-W10. Actual Memory group/particle lists.
void KillAllActiveEffects(port::driving::Memory&,std::uint32_t system);
}
namespace port::track {
class EmitterBindings {
    port::driving::Memory& memory_;
    struct Records {std::uint32_t address=0,count=0,stride=0;};
    std::vector<Records> records_;
    bool initialized_=false;
public:
    explicit EmitterBindings(port::driving::Memory& memory):memory_(memory){}
    EmitterBindings(const EmitterBindings&)=delete;
    EmitterBindings& operator=(const EmitterBindings&)=delete;
    // Explicit process-static list/system construction followed by original afxInit.
    // The shared SlotPoolManager must have been initialized by its real owner.
    void init();
    // Debug00126FA8 schema/order. Complete original bytes are retained, including
    // padding; source emitter state/RNG/list insertion follows original registration.
    void load_bundle(const std::vector<std::uint8_t>& original_bytes);
    void populate(std::uint32_t list,std::uint32_t group);
    void erase(std::uint32_t list);
    // Original001270B8 detaches records, then port storage is released. Call only
    // after every copied emitter list has been deleted and active effects killed.
    void unload_all();
    // Port session teardown: actual afxClose plus owned static-resource release.
    // Explicitly also deletes the retained emitter pool, which afxClose does not.
    // No ParticleSimulation mirror is advanced or claimed to share this state.
    void shutdown();
    std::size_t definition_count()const;
    std::size_t source_count()const;
};
}

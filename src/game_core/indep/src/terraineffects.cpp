// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\terraineffects.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/TerrainEffectEmitters.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
// Debug0021CB40, E-0099-W06. Original source ../indep/src/terraineffects.cpp.
TerrainEffectEmitters::TerrainEffectEmitters(::port::driving::Memory& memory,std::uint32_t address,const ::port::race::EmitterGroupLookup& lookup){
    for(unsigned effect=0;effect<7;++effect)for(unsigned terrain=0;terrain<33;++terrain){
        const auto list=address+(effect*33+terrain)*8;
        memory.word(list,list);memory.word(list+4,list);
    }
    // Lookup order is terrain first, effect second, after *all* lists exist.
    for(unsigned terrain=0;terrain<33;++terrain)for(unsigned effect=0;effect<7;++effect){
        const auto name=memory.word(0x003102e8+terrain*0x90+0x3c+effect*0xc);
        if(!name||!memory.byte(name))continue;
        std::string text;
        for(unsigned offset=0;;++offset){
            if(offset==4096)hp2::not_implemented("0x0024C8B0","unterminated TerrainTypeInfo emitter group");
            const auto c=memory.byte(name+offset);if(!c)break;text.push_back(static_cast<char>(c));
        }
        const auto hash=hp2::frontend::bStringHash(text.c_str());
        if(!lookup)hp2::not_implemented("0x00127438","original afxGetEmitters service is not bound");
        lookup(memory,address+(effect*33+terrain)*8,hash);
    }
}
// Debug0021CE28, E-0099-W06. Publish only after original construction returns.
void InitTerrainEffectEmitters(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& lookup){
    if(memory.word(0x00328b58)==0){
        const auto address=memory.allocate(0x738);
        TerrainEffectEmitters emitters(memory,address,lookup);
        memory.word(0x00328b58,address);
    }
}
}

#include "game_core/port/track/emitter_bindings.hpp"
namespace hp2::debug {
// Debug0021CE68 ->0021CCA8, E-0099-W18. Reverse array/list destruction.
void CloseTerrainEffectEmitters(::port::driving::Memory& m){
 const auto object=m.word(0x328b58);if(!object)return;
 for(int i=230;i>=0;--i)afxDeleteEmitters(m,object+8*i);
 m.release(object);m.word(0x328b58,0);
}
}

#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug0021CD30, E-0099-W38. Exact actual list heads and emitter state writes,
// followed by original WorldEffects call; no ParticleSimulation mirror.
void TerrainEffectEmitters::TriggerEffect(::port::driving::Memory& m,std::uint32_t object,int effect,int terrain,float intensity,std::uint32_t position,std::uint32_t velocity){
 if(effect<0||effect>=7||terrain<0||terrain>=33)throw std::out_of_range("Original terrain effect list index");const auto head=object+static_cast<std::uint32_t>(effect*33+terrain)*8;if(m.word(head)==head)return;
 auto matrix=hp2::recovered::bIdentity();for(unsigned i=0;i<3;++i)matrix[12+i]=m.scalar(position+i*4);matrix[15]=1;
 for(auto emitter=m.word(head);emitter!=head;emitter=m.word(emitter)){m.matrix(emitter+0x30,matrix);m.vector(emitter+0x70,{{m.scalar(velocity),m.scalar(velocity+4),m.scalar(velocity+8),0}});AcidEffectSystem(m,0x2e6f40).TriggerEmitter(emitter,-intensity,0);}
}
}

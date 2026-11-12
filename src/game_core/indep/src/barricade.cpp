// TASK-0099 E-0099-W119. Original ..\indep\src\barricade.cpp, Debug
// NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/src/barricade.hpp"
#include "game_core/indep/inc/Replay.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug00248418. Exact10 handle/timer fields, no inferred padding stores.
void tBarricadeManager::Sub_00248418(){m_.word(address_+0x3c,0x2b6ec8);for(unsigned i=0;i<10;++i){m_.half(address_+2*i,0xffff);m_.scalar(address_+0x14+4*i,-1);}}
// Debug00248468. Undeployed handles genuinely require no smokeable deletion.
void tBarricadeManager::Sub_00248468(int flags){m_.word(address_+0x3c,0x2b6ec8);for(unsigned i=0;i<10;++i)if(m_.half(address_+2*i)!=0xffff)hp2::not_implemented("0x00248468","actual deployed barricade hash/info/DeleteSmokeable ownership chain is required");if(flags&1)m_.release(address_);}
// Debug002486F8, E-0100-PU02. Preserve every actual timer before the positive
// deletion child; fresh undeployed slots require no synthetic empty manager.
void tBarricadeManager::Update(float dt){for(unsigned i=0;i<10;++i){const auto timer=address_+0x14+4*i;if(m_.half(address_+2*i)!=0xffff&&m_.scalar(timer)!=-1.f){const float next=m_.scalar(timer)-dt;m_.scalar(timer,next);if(next<0)hp2::not_implemented("0x002486F8","actual timed barricade FindSmokeableInfo/DeleteSmokeable child required");}}}
// Debug002487E8. Exact timer then signed-short handle order for all10 rows.
void tBarricadeManager::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2b6eb0);for(unsigned i=0;i<10;++i){s.MoveData(address_+0x14+4*i,4);s.MoveData(address_+2*i,2,2,1);}s.EndSection();}
}

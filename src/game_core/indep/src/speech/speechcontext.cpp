// TASK-0099 E-0099-W16; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact original source ownership: local/task0099/cop-speech/source-ownership.json.
// Host Memory projection, no native ABI or whole-version equivalence claim.
#include "game_core/port/race/cop_speech.hpp"
#include "game_core/src/bMath.hpp"
#include <array>
#include <algorithm>
namespace port::race {
// Debug0020A748 ShuffleCopIds /0020A6E0 ShuffleArray; E-0099-W16.
void ShuffleCopIds_0020A748(port::driving::Memory& memory_){
 constexpr std::array<std::uint32_t,4> cop_ids{{2,4,8,32}};
 for(unsigned i=0;i<4;++i)memory_.word(0x313340+4*i,cop_ids[i]);
 for(unsigned i=0;i<3;++i){const auto j=i+hp2::recovered::bRandom(memory_,int(4-i));
  const auto old=memory_.word(0x313340+4*i);memory_.word(0x313340+4*i,memory_.word(0x313340+4*j));memory_.word(0x313340+4*j,old);}
 std::array<std::uint32_t,6> ids{{1,2,4,8,16,32}};
 for(unsigned i=0;i<5;++i){const auto j=i+hp2::recovered::bRandom(memory_,int(6-i));std::swap(ids[i],ids[j]);}
 for(unsigned i=0;i<4;++i)memory_.word(0x313330+4*i,ids[i]);
 for(unsigned i=0;i<4;++i)memory_.word(0x313320+4*i,ids[i]);
}
}

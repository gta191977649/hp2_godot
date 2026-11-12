// TASK-0099 E-0099-W86. Exact original definition owner indep/src/gameflow.cpp.
// Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/src/gameflow.hpp"
namespace hp2::debug {
// Debug001675A0. Source request publication only; ServiceGameFlow and actual
// loading/transition execution are separate original dependencies.
void ChangeGameFlow(::port::driving::Memory& m,GameFlowState state,GameFlowState pending){m.word(0x3275b0,std::uint32_t(state));m.word(0x3275b4,std::uint32_t(pending));}
}

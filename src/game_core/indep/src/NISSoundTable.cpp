// TASK-0099 E-0099-W51. Exact definition marker ../indep/Src/NISSoundTable.cpp;
// FDR78 contains this and nis.cpp definition markers; PC compiles both files.
#include "game_core/indep/inc/NIS.hpp"
namespace hp2::debug {
// Debug001AE5B0. Fresh listing is JR RA/NOP, not an inferred missing sample table.
// Debug001AE5B0, E-0099-W51.
void NisManager::InitSoundEvents(){}
}
namespace hp2::debug {void NisManager::PlaySoundEvents(int){} } //Debug001AE5B8; original empty NISSoundTable body.

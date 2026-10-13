// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "game_core/port/engine/world_connections.hpp"
namespace hp2::engine {
const std::vector<WorldConnection>& world_connections() {
    static const std::vector<WorldConnection> connections={
        {0x14dcd8,0x14ddb0,0x131a30,"VehicleController","indices [world+20,world+24); a0=vehicle, f12=dt","high","E-0010-03"},
        {0x14dcd8,0x14de00,0x131a30,"VehicleController","indices [world+28,world+1c); a0=vehicle, f12=dt","high","E-0010-03"},
        {0x14dcd8,0x14de44,0x131a30,"VehicleController","indices [0,world+20); a0=vehicle, f12=dt","high","E-0010-03"},
        {0x14dcd8,0x14de88,0x131a30,"VehicleController","indices [world+24,world+28); a0=vehicle, f12=dt","high","E-0010-03"},
        {0x131a30,0x131a44,0x1319a0,"UnresolvedCalls","f0 stored to vehicle+458 before controller callback","unknown","E-0010-03"},
        {0x131a30,0x131a64,0,"ControllerThinkVirtual","car+28 -> table+8; function+24, signed this-adjust+20; CopDriver candidate 001a2110","high","E-0010-03"},
        {0x14dcd8,0x14dec8,0x1346d8,"UnresolvedCalls","indices [0,world+20); exact method meaning unknown","unknown","E-0010-03"},
        {0x14dcd8,0x14df04,0x131a80,"SecondaryVehicleObject","all vehicles; this is NOT the mover wrapper","high","E-0010-03"},
        {0x131a80,0x131aa0,0,"SecondaryObjectVirtual","car+2c -> table+22c; function+14, signed this-adjust+10","unknown","E-0010-03"},
        {0x14dcd8,0x14df5c,0x131cc8,"VehiclePreparation","all vehicles; vector snapshots/proximity preparation, meaning provisional","medium","E-0010-03"},
        {0x131cc8,0x131da0,0x1c0850,"TailConnection","tail J rather than JAL; exact method unknown","unknown","E-0010-03"},
        {0x14dcd8,0x14df78,0x14d898,"EntityAdvance","active entity list; indirect dispatch establishes mover path","high","E-0010-03"},
        {0x14d898,0x14d918,0,"EntityAdvanceVirtual","object table+50; function+44, signed this-adjust+40; table 002b87a8 candidate method 001d81f0","high","E-0010-03"},
        {0x1d81f0,0x1d8214,0x131ac8,"VehicleMovement","a0=entity owner car at+100; f12=dt","high","E-0010-03"},
        {0x1d81f0,0x1d823c,0,"EntityPartialStepVirtual","if mover f0<dt, callback through entity slot+74","high","E-0010-03"},
        {0x1d81f0,0x1d8248,0x131ac8,"VehicleMovementRemainder","distinct retry call; f12=dt-returned f0","high","E-0010-03"},
        {0x131ac8,0x131b04,0,"MoverVirtual","car+30 -> table+4; function+14, signed this-adjust+10; CopMover candidate 001a1030","high","E-0010-03"},
        {0x14dcd8,0x14df88,0x14d940,"InteractionWorklist","pair/interaction worklist; precise collision semantics unknown","medium","E-0010-03"},
        {0x14dcd8,0x14dfb4,0x131da8,"VehiclePostUpdate","all vehicles; derived motion metrics and conditional events","medium","E-0010-03"},
    };
    return connections;
}
}

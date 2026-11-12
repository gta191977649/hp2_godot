#pragma once
// Port binding of required original callees/resources. No replacement race
// lifecycle callback or synthetic state pointer is accepted.
#include "race_construction.hpp"
#include "memory_availability.hpp"
#include "world_construction.hpp"
#include "cop_speech.hpp"
namespace port::race {
struct PauseServices;
struct EndGameScreenServices;
struct RewardAwardReceipt;
struct CoordinatorConstructionServices {
 const WorldConstructionServices& world;
 const CarConstructionServices& cars;
 PhysicsConstructionServices& physics;
 const AiDriveTargetServices& ai;
 const CarSoundResources& tracks;
 const RaceStartOperands& start;
 MemoryAvailability& memory;
 CopSpeechBindings& speech;
 // TASK-0110 explicit PC presentation policy: original outcome data remains
 // mandatory; optional source EndGameScreens/highlight cameras are not entered.
 bool minimal_settlement_presentation=false;
 const PauseServices* pause=nullptr; //E-0112-P02, real screen/device operations.
 const EndGameScreenServices* end_game_screens=nullptr; //E-0112-R02.
 RewardAwardReceipt* reward_receipt=nullptr; //Port observation of actual source award stores.
};
}

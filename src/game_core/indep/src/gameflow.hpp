#pragma once
// Port companion API for original indep/src/gameflow.cpp; no original header
// path or C++ ABI asserted. E-0099-W86, Debug SHA815b60c7...332f1b.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
// Exact original enum names/values from Debug GameFlowState datatype.
enum GameFlowState {
 GAMEFLOW_STATE_NONE=0,GAMEFLOW_STATE_LOADING_FRONTEND=1,
 GAMEFLOW_STATE_UNLOADING_FRONTEND=2,GAMEFLOW_STATE_IN_FRONTEND=3,
 GAMEFLOW_STATE_LOADING_TRACK_COMMON=4,GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC=5,
 GAMEFLOW_STATE_RACING=6,GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC=7,
 GAMEFLOW_STATE_UNLOADING_TRACK_COMMON=8,GAMEFLOW_STATE_EXIT_DEMO_DISC=9
};
void ChangeGameFlow(::port::driving::Memory&,GameFlowState,GameFlowState); //001675A0.
}

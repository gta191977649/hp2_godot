#pragma once
// TASK-0101 E-0101-PL01. Authorized Debug status authority over the reviewed
// Retail profile codec; no Retail opaque event31 globals or unlock overrides.
#include "selection_event_resources.hpp"
#include "game_core/indep/src/eventtree.hpp"
#include "port/assets/catalog.hpp"
#include <filesystem>
namespace port::runtime {
class SourceEventStatus {
 mutable driving::Memory memory_;
 SelectionEventResources resources_;
 std::uint32_t explicit_goal_=0;
 std::vector<hp2::frontend::EventTree_001FFC40> retail_trees_;
public:
 SourceEventStatus(const std::filesystem::path& project,assets::Source&,const std::string& front,
                   const hp2::memorycard::Profile_002FC050&,int language);
 ~SourceEventStatus();
 int status(int tree_type,int number,const hp2::frontend::EventLink_002008B8* goal=nullptr)const;
 // Presentation-only temporary for the legacy per-tree record formatter:
 // rebind its event32 slice to the actual shared state. Never store this copy.
 hp2::memorycard::Profile_002FC050 formatter_profile(const hp2::memorycard::Profile_002FC050&)const;
};
}

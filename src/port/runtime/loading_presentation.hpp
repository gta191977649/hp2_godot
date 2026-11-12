#pragma once
// TASK-0113. Port ownership/publication around actual original loading widgets.
#include "game_core/indep/src/fmenus.hpp"
#include <memory>
#include <array>
#include <string>
namespace port::runtime {
class LoadingPresentation {
 struct Impl;std::unique_ptr<Impl> p_;
public:
 explicit LoadingPresentation(const std::string& search_start,int language);
 ~LoadingPresentation();
 LoadingPresentation(const LoadingPresentation&)=delete;
 LoadingPresentation& operator=(const LoadingPresentation&)=delete;
 void begin(int game_type,int transmission,bool demo=false);
 void tick(double elapsed);
 void end();
 void close(); //Explicit original borrower-before-texture teardown at guarded boundary.
 bool active()const;
 std::uint32_t screen_hash()const;
 std::uint64_t revision()const;
 const std::vector<hp2::frontend::FeScreen>& documents()const;
 std::array<float,4> clip_bounds()const;
};
}

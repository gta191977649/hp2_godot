#include "game_core/port/engine/engine_shell.hpp"
#include "game_core/port/engine/world_connections.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace hp2::engine;
namespace {
using PCs=std::vector<std::uint32_t>;
void require(bool value,const char* reason) {if (!value) throw std::runtime_error(reason);}
PCs executed(const std::vector<TraceEvent>& trace) {
    PCs pcs;
    for (const auto& e:trace) if(e.kind==EventKind::Executed || e.kind==EventKind::SyntheticPlaceholder) pcs.push_back(e.pc);
    return pcs;
}
// Manually reviewed against the listing, independent of the scheduler table/next().
const PCs prefix={0x12f3b0,0x12f3c0,0x12f3d4,0x12f3dc,0x12f3ec,0x12f3f4,0x12f3fc,0x12f404,
    0x12f418,0x12f420,0x12f434,0x12f43c,0x12f448,0x12f454,0x12f45c,0x12f464};
const PCs suffix={0x12f56c,0x12f574,0x12f57c,0x12f584,0x12f5a4,0x12f5b0,0x12f5b8,0x12f5c0,
    0x12f5c8,0x12f5d0,0x12f5d8,0x12f5e0,0x12f5f4,0x12f5fc,0x12f604,0x12f60c,0x12f614,
    0x12f61c,0x12f624,0x12f62c,0x12f634,0x12f63c,0x12f664,0x12f66c,0x12f674};
PCs expected(unsigned scenario) {
    PCs out=prefix;
    const auto remove=[&](std::uint32_t pc) {out.erase(std::remove(out.begin(),out.end(),pc),out.end());};
    if (scenario==1) for(auto pc:{0x12f3d4u,0x12f3dcu,0x12f3ecu,0x12f434u}) remove(pc);
    if (scenario==5) remove(0x12f418);
    PCs middle;
    if (scenario==1 || scenario==4) middle={0x12f550,0x12f558,0x12f560};
    else if(scenario==2) middle={0x12f488,0x12f490,0x12f4a4,0x12f4ac,0x12f4b4,0x12f4c0};
    else if(scenario==3) middle={0x12f488,0x12f490,0x12f4d0,0x12f4ec,0x12f4f4,0x12f4fc,0x12f540};
    else middle={0x12f488,0x12f490,0x12f4d0,0x12f50c,0x12f518,0x12f520,0x12f528,0x12f538,0x12f540};
    out.insert(out.end(),middle.begin(),middle.end());out.insert(out.end(),suffix.begin(),suffix.end());
    if(scenario==1) {remove(0x12f5a4);remove(0x12f5f4);}
    if(scenario==5) remove(0x12f5a4);
    return out;
}
void paths_and_coverage() {
    std::set<std::uint32_t> observed;
    for(unsigned scenario=0;scenario<6;++scenario) {
        EngineShell shell;
        require(shell.initialize(Mode::Observation,scenario_state(scenario),scenario_fixtures(scenario))==Status::Ready,"initialize");
        require(executed(shell.startup_trace())==PCs({0x12f014,0x12f020,0x12f048,0x12f064,0x12f3a0}),"startup order");
        require(shell.step_frame({0.123,true})==Status::Completed,"scenario completion");
        const auto got=executed(shell.context().trace);
        require(got==expected(scenario),"manual scenario call sequence differs");
        observed.insert(got.begin(),got.end());observed.insert(0x12f3a0);
        require(shell.context().completed_frames==1,"completion count");
        require(shell.context().timing.base_step==0.02f,"host delta replaced fixture base time");
        const auto want=(scenario==2 || scenario==3 || scenario==4) ? 0.0f : scenario==1 ? 0.02f : 0.01f;
        require(shell.context().timing.effective_step==want,"effective step path");
        for(const auto& stamp:shell.context().timing.stamps) require(stamp.has_value(),"missing profiling stamp");
        require(shell.step_frame({0.005,true})==Status::Completed,"second frame");
        require(executed(shell.context().trace)==expected(scenario),"one-time timer repeated/frame ordering changed");
        require(shell.context().completed_frames==2,"second completion count");
    }
    std::set<std::uint32_t> sites,targets;
    for(const auto& s:call_sites()) if(s.caller==0x12f378) {sites.insert(s.pc);targets.insert(s.target);}
    require(sites.size()==61 && targets.size()==39 && observed==sites,"static call coverage");
    require(world_connections().size()==19,"world connection inventory");
}
void wait_resume_shutdown() {
    EngineShell shell;shell.initialize();
    require(shell.step_frame({0.01,false})==Status::WaitingForFrameBoundary,"must wait");
    auto calls=executed(shell.context().trace);
    auto want=expected(0);want.resize(want.size()-3);
    require(calls==want && shell.context().completed_frames==0,"tail ran before boundary");
    const auto trace_size=shell.context().trace.size();
    for(unsigned i=0;i<10;++i) require(shell.step_frame({0.01,false})==Status::WaitingForFrameBoundary,"wait state");
    require(executed(shell.context().trace)==calls && shell.context().trace.size()==trace_size,"waiting repeats work/grows trace");
    require(shell.step_frame({0.01,true})==Status::Completed,"resume");
    require(executed(shell.context().trace)==expected(0),"resume tail exactly once");
    shell.step_frame({0,false});shell.shutdown();
    const auto stopped=executed(shell.context().trace);
    require(shell.step_frame({0,true})==Status::Stopped && executed(shell.context().trace)==stopped,"shutdown must cancel pending frame");
    require(shell.initialize()==Status::Ready && shell.context().completed_frames==0,"reset");
    RawState raw;raw.gp_b360=0xffffffffu;
    shell.initialize(Mode::Observation,raw);
    require(shell.step_frame({0,true})==Status::Completed && shell.context().raw.gp_b360==0,"boundary wrap");
    EngineShell negative;
    negative.modules().bind_site(0x12f63c,[](FrameContext& c,const CallSite&,const CallArguments&) {c.raw.gp_b360=c.raw.gp_a544-1;return CallResult{};});
    negative.initialize();
    require(negative.step_frame({0,false})==Status::WaitingForFrameBoundary,"negative difference incorrectly satisfied boundary");
    require(negative.step_frame({0,true})==Status::WaitingForFrameBoundary,"zero difference incorrectly satisfied boundary");
    require(negative.step_frame({0,true})==Status::Completed,"positive difference failed boundary");
}
void handler_contracts() {
    EngineShell missing;
    require(missing.initialize(Mode::Strict)==Status::Unimplemented,"strict startup missing");
    require(missing.context().current_pc==0x12f014 && missing.context().completed_frames==0,"missing startup pc");
    const auto missing_size=missing.context().trace.size();
    require(missing.step_frame({0,true})==Status::Unimplemented && missing.context().trace.size()==missing_size,"terminal failure repeats");
    EngineShell strict;
    auto success=[](FrameContext&,const CallSite&,const CallArguments&) {return CallResult{};};
    for(auto pc:{0x12f014u,0x12f020u,0x12f048u,0x12f064u}) strict.modules().bind_site(pc,success);
    strict.modules().bind_site(0x12f3a0,[](FrameContext&,const CallSite&,const CallArguments&) {CallResult r;r.integer=1;return r;});
    require(strict.initialize(Mode::Strict)==Status::Ready,"strict init bound handlers");
    require(strict.step_frame({0,true})==Status::Unimplemented && strict.context().current_pc==0x12f3b0,"strict frame exact missing pc");
    EngineShell failed;failed.modules().bind_site(0x12f404,[](FrameContext&,const CallSite&,const CallArguments&) {CallResult r;r.success=false;r.error="module failed";return r;});
    failed.initialize();require(failed.step_frame({0,true})==Status::Error,"module failure");
    require(failed.context().current_pc==0x12f404 && failed.context().error=="module failed","error location");
    require(executed(failed.context().trace)==PCs(prefix.begin(),prefix.begin()+7),"failure must retain prefix and stop");
    EngineShell bad_result;bad_result.modules().bind_site(0x12f3b0,success);bad_result.initialize();
    require(bad_result.step_frame({0,true})==Status::Error && bad_result.context().current_pc==0x12f3b0,"missing return contract");
    EngineShell throwing;throwing.modules().bind_site(0x12f404,[](FrameContext&,const CallSite&,const CallArguments&)->CallResult {throw std::runtime_error("handler exception");});
    throwing.initialize();require(throwing.step_frame({0,true})==Status::Error && throwing.context().error=="handler exception","throwing handler");
    EngineShell host;host.initialize();require(host.step_frame({-1,true})==Status::Error && host.context().trace.back().kind==EventKind::Error,"host error trace");
}
void live_gate_and_argument_contracts() {
    EngineShell startup;
    startup.modules().bind_site(0x12f020,[](FrameContext& c,const CallSite&,const CallArguments& a) {require(a.a[0]==7 && a.a[1]==9,"bootstrap args");c.raw.gp_a54c=1;return CallResult{};});
    startup.modules().bind_site(0x12f064,[](FrameContext&,const CallSite&,const CallArguments& a) {require(a.a[0]==0x12f378 && a.a[1]==0x70004000 && a.a[2]==0 && a.a[3]==0,"registration args");return CallResult{};});
    startup.initialize(Mode::Observation,{}, {},7,9);
    require(executed(startup.startup_trace())==PCs({0x12f014,0x12f020,0x12f034,0x12f064,0x12f3a0}),"startup stale gate");
    EngineShell dynamic;
    dynamic.modules().bind_site(0x12f3c0,[](FrameContext& c,const CallSite&,const CallArguments&) {c.raw.gp_a938=0;c.raw.gp_a998=0;c.raw.gp_a858=0;return CallResult{};});
    dynamic.initialize();dynamic.step_frame({0,true});
    require(executed(dynamic.context().trace)==expected(1),"gates were frozen before state callback");
    EngineShell priority;unsigned target_calls=0,site_calls=0;
    priority.modules().bind_target(0x1680e8,[&](FrameContext&,const CallSite&,const CallArguments&) {++target_calls;return CallResult{};});
    priority.modules().bind_site(0x12f3c0,[&](FrameContext&,const CallSite&,const CallArguments&) {++site_calls;return CallResult{};});
    priority.modules().bind_site(0x12f538,[](FrameContext&,const CallSite&,const CallArguments& a) {require(a.a[0]==1 && a.f12==0.01f,"world caller ABI projection");return CallResult{};});
    priority.modules().bind_site(0x12f5f4,[](FrameContext&,const CallSite&,const CallArguments& a) {require(a.a[0]==1,"known world a0");return CallResult{};});
    priority.initialize();priority.step_frame({0,true});require(target_calls==1 && site_calls==1,"site override priority/repeated target");
    EngineShell captured;
    captured.modules().bind_site(0x12f518,[](FrameContext& c,const CallSite&,const CallArguments& a) {require(a.f12==0.01f,"passed f12");c.timing.effective_step=99.0f;return CallResult{};});
    captured.initialize();captured.step_frame({0,true});require(captured.context().timing.effective_step==0.01f,"argument readback after handler mutation");
    EngineShell saved;
    saved.modules().bind_site(0x12f434,[](FrameContext& c,const CallSite&,const CallArguments&) {c.timing.previous_stamp=999999;return CallResult{};});
    saved.modules().bind_site(0x12f448,[](FrameContext&,const CallSite&,const CallArguments& a) {require(a.a[0]==1000,"callee-saved s2 was overwritten");CallResult r;r.scalar=10.0f;return r;});
    saved.modules().bind_site(0x12f520,[](FrameContext& c,const CallSite&,const CallArguments&) {c.timing.effective_step=42.0f;return CallResult{};});
    saved.modules().bind_site(0x12f538,[](FrameContext&,const CallSite&,const CallArguments& a) {require(a.f12==0.01f,"callee-saved f20 was overwritten");return CallResult{};});
    saved.initialize();require(saved.step_frame({0,true})==Status::Completed,"saved caller operand fixture");
    EngineShell unresolved;
    unresolved.modules().bind_site(0x12f454,[](FrameContext& c,const CallSite&,const CallArguments&) {c.timing.base_step.reset();return CallResult{};});
    unresolved.initialize();require(unresolved.step_frame({0,true})==Status::Error && unresolved.context().current_pc==0x12f488,"unresolved consumed timing must stop");
    EngineShell invalid;auto raw=scenario_state(0);raw.gp_a998=0;invalid.initialize(Mode::Observation,raw);
    require(invalid.step_frame({0,true})==Status::Error,"invalid world context");
}
}
int main() {
    try {paths_and_coverage();wait_resume_shutdown();handler_contracts();live_gate_and_argument_contracts();}
    catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
    std::cout<<"ENGINE_SHELL_TESTS_OK: 61 sites/39 targets; manual branches, live gates, ABI projection, wait/resume/wrap, strict failures\n";
}

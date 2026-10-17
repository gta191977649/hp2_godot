#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace port::driving;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool near(float a, float b) { return std::abs(a - b) < 0.00001f; }
SolverMatrix identity() { SolverMatrix m{}; m[0] = m[5] = m[10] = m[15] = 1; return m; }

}
int main() {
    // E-0038-S01: transpose is a raw permutation, including in-place use.
    SolverMatrix matrix{};
    for (unsigned i = 0; i < 16; ++i) matrix[i] = float(i);
    bTransposeMatrix(matrix, matrix);
    check(matrix[1] == 4 && matrix[4] == 1 && matrix[15] == 15, "In-place transpose");
    // E-0038-S02: input W is ignored, mutable global W selects column 4.
    matrix = identity(); matrix[12] = 5; matrix[13] = 6; matrix[14] = 7;
    SolverVector value{{1,2,3,99}};
    bMulMatrix(value, matrix, value, {{{0,0,0,1}}});
    check(value == SolverVector{{6,8,10,1}}, "Transform global W and aliased output");
    bMulMatrixVector(value, matrix, {{1,2,3,99}}, {{{0,0,0,0}}});
    check(value == SolverVector{{1,2,3,0}}, "Transform thunk uses explicit global");
    // E-0038-S03: 1200 cap affects all force lanes; cross uses uncapped input.
    SolverBodyProjection body{};
    ApplyForce(body, {{2400,0,0,8}}, {{0,0.02f,0,9}});
    check(body.Field_0xf0 == SolverVector{{1200,0,0,4}}, "Force cap and fourth lane");
    check(near(body.Field_0x100[2], -48) && body.Field_0x100[3] == 9,
          "Cross uses original force and retains position W");
    body = {};
    ApplyForce(body, {{100,0,0,1}}, {{0,2,0,10}});
    check(body.Field_0x100 == SolverVector{{0,0,-100,5}}, "Separate cross cap scales retained W");
    body = {}; body.Field_0xf0 = {{1,0,0,0}};
    ApplyForce(body, body.Field_0xf0, {{0,1,0,0}});
    check(body.Field_0x100[2] == -2, "Late original A1 reload preserves aliasing");
    ApplyMoment(body, {{1,2,3,4}});
    check(body.Field_0x100 == SolverVector{{1,2,1,4}}, "Direct four-lane accumulation");
    body = {}; body.Field_0x20 = identity();
    ApplyForceInFrame(body, {{10,0,0,7}}, {{0,2,0,8}}, {{{0,0,0,0}}});
    check(body.Field_0xf0[0] == 10 && body.Field_0x100[2] == -20,
          "Body transform wrapper delegates both vectors");
    // E-0038-S06: three paired products, preserving the fourth destination lane.
    SolverWheelProjection wheel{};
    wheel.Field_0xb0=1;wheel.Field_0xb4=2;wheel.Field_0xb8=3;
    wheel.Field_0xc0=4;wheel.Field_0xc4=5;wheel.Field_0xc8=6;
    wheel.Field_0x114=2;wheel.Field_0x118=-1;value={{0,0,0,123}};
    GetHorizontalGroundForceInCarFrame(wheel,value);
    check(value == SolverVector{{-2,-1,0,123}}, "Wheel basis stores XYZ only");
    // E-0038-S11: strict first threshold, second strict1 threshold, distinct stores.
    wheel={};wheel.config_0x14_Field_0x4=1;wheel.config_0x14_Field_0x8=1;
    wheel.config_Field_0xc=2;wheel.Field_0x170=2;wheel.Field_0x174=3;wheel.Field_0x20=-7;
    CheckForBrakeLock(2.5f,wheel,{1,1,1,0});
    check(wheel.Field_0x10c==0 && wheel.Field_0x20==-7,"Threshold equality retains +0x20");
    CheckForBrakeLock(1,wheel,{1,1,1,0});
    check(wheel.Field_0x10c==1 && wheel.Field_0x20==0,"Strict threshold sets flag and zeroes +0x20");
    wheel.Field_0x170=1;wheel.Field_0x174=0;wheel.Field_0x20=-7;
    CheckForBrakeLock(0,wheel,{1,1,1,0});
    check(wheel.Field_0x10c==0 && wheel.Field_0x20==0,"Second threshold equality clears flag and +0x20");
    // E-0038-S07/S08: double cross gives scale 1 for perpendicular unit inputs,
    // or 0.3 for parallel inputs. Contact flag is written even for zero request.
    wheel = {};wheel.config_Field_0xc=2;wheel.Field_0x80={{1,0,0,7}};
    wheel.Field_0x140={{0,0,1,2}};wheel.Field_0xa0={{3,4,5,6}};
    check(ExchangeZForces(10,wheel,257)==0 && wheel.Field_0x155==1,
          "Flag truncates to stored byte and returns zero");
    check(wheel.Field_0x158==10 && wheel.Field_0x130==SolverVector{{3,4,3,2}},
          "Positive request and four-lane offset");
    wheel.Field_0x140={{1,0,0,0}};
    ExchangeZForces(10,wheel,0);
    check(near(wheel.Field_0x158,3), "ELF 0x3e99999a lower bound");
    SolverSuspensionProjection suspension{};suspension.Field_0x90=&wheel;
    suspension.Field_0x1c=-1;suspension.Field_0x70=1;
    Suspension::ResolveForces(3,suspension);
    check(near(wheel.Field_0x158,0.6f) && suspension.Field_0xa0==0,
          "Suspension forwards +0x70 and conditionally adds request");
    suspension.Field_0x70=0;Suspension::ResolveForces(3,suspension);
    check(wheel.Field_0x158==0 && wheel.Field_0x155==0, "Zero flag omits added request");
    // E-0038-S09: no-contact and explicit suppression perform no accumulation.
    body={};suspension.Field_0x94=&body;wheel.Field_0x120={{10,0,0,0}};
    wheel.Field_0x130={{0,1,0,0}};
    ResolveMoreForces(suspension,0);check(body.Field_0xf0[0]==0,"No-contact return");
    suspension.Field_0x70=1;ResolveMoreForces(suspension,1);
    check(body.Field_0xf0[0]==0,"Global suppression");
    ResolveMoreForces(suspension,0);
    check(body.Field_0xf0[0]==10 && body.Field_0x100[2]==-10,"Contact offset accumulation");
    // TASK-0044 E-0044-S01: canonical Memory bindings share the leaf implementation.
    // Full force-loop acceptance lives in driving_backend_tests with the identity-bound Debug ELF.
    Memory memory;const auto object=memory.allocate(0x180);
    hp2::recovered::RigidBody canonical_body(memory,object);
    canonical_body.ApplyForce({{2400,0,0,8}},{{0,0.02f,0,9}});
    check(memory.vector(object+0xf0)==SolverVector{{1200,0,0,4}},"Canonical body force accumulation");
    check(near(memory.scalar(object+0x108),-48),"Canonical body uses uncapped force for moment");
    const auto state=memory.allocate(0x80),copy=memory.allocate(0x80);
    for(unsigned i=0;i<32;++i)memory.word(state+4*i,0x80000000u+i);
    canonical_body.SetState(state);canonical_body.GetState(copy);
    check(canonical_body.GetState()==object+0x10 && canonical_body.GetSize()==32,"Canonical body state contract");
    for(unsigned i=0;i<32;++i)check(memory.word(copy+4*i)==0x80000000u+i,"Canonical state transfer preserves all bits");
    std::cout << "driving_solver_tests passed (bounded host fixtures, not original equivalence)\n";
}

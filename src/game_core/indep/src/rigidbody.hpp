#pragma once
// TASK-0044 E-0044-S01/S02; Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact owner/source RigidBody, indep/src/rigidbody.cpp. Host storage binds explicit PS2 Memory.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::recovered {
// Debug0019ac98; E-0044-S01. Bounded vector/matrix host values.
port::driving::SolverMatrix VectorStarMatrix(const port::driving::SolverVector&,const port::driving::SolverMatrix&);
class RigidBody : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug0019a2d8 constructor body; original allocation separate from host lifetime.
 void port_initialize(std::uint32_t config);
 // Debug0019a568/0019a580.
 void SetState(std::uint32_t position,std::uint32_t matrix,std::uint32_t velocity,std::uint32_t angular);
 void Calibrate(std::uint32_t cog);void CalculateInertiaTensor();
 // Debug0019a6b8/0019a7a8/0019a820/0019a838.
 void ApplyForce(const port::driving::SolverVector&,const port::driving::SolverVector&);
 void ApplyForceInFrame(const port::driving::SolverVector&,const port::driving::SolverVector&);
 void ApplyMoment(const port::driving::SolverVector&);
 void ApplyImpulse(const port::driving::SolverVector&,const port::driving::SolverVector&);
 // Debug0019A920; E-0048-D04. Original capped momentum and scaled angular impulse.
 void ApplyImpulseWithScaledInverseInertiaTensor(const port::driving::SolverVector&,const port::driving::SolverVector&,float);
 // Debug0019adf8/0019b170/0019b2a0. Last float explicitly supplies original uninitialized stack W.
 void GetDerivative(std::uint32_t output);void ComputeAuxiliaries(float unused_stack_w=0);void ResolveForces();
 // Debug0019b558/0019b5a0/0019b5e8/0019b5f0.
 void SetState(std::uint32_t state);void GetState(std::uint32_t state);std::uint32_t GetState();int GetSize();
 // Debug0019b2a8 explicit untranslated printing service.
 void PrintCurrentState();
 static void port_initialize(port::driving::init::ByteView,std::uint32_t,port::driving::init::ByteView);
 static void CalculateInertiaTensor(port::driving::init::ByteView,port::driving::init::ByteView);
 static void Calibrate(port::driving::init::ByteView,const std::array<std::uint32_t,4>*);
 // Port-side bounded leaf bindings, one canonical algorithm body.
 static void ApplyForce(port::driving::SolverBodyProjection&,const port::driving::SolverVector&,const port::driving::SolverVector&);
 static void ApplyForceInFrame(port::driving::SolverBodyProjection&,const port::driving::SolverVector&,const port::driving::SolverVector&,const port::driving::SolverMathContext&);
 static void ApplyMoment(port::driving::SolverBodyProjection&,const port::driving::SolverVector&);
};
}

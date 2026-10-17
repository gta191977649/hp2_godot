#pragma once
// TASK-0044 E-0044-S01/S02; NFSHP2.ELF SHA256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact owner/source Suspension, indep/src/suspension.cpp. Encoded pointers bind explicit port Memory.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::recovered {
// Debug001a3bb0; E-0044-S01. Bounded vector host values.
std::array<float,4> GetPointVelocity(const std::array<float,4>&,const std::array<float,4>&,const std::array<float,4>&);
class Suspension : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug001a3680; original initialization separated from host lifetime.
 void port_initialize(int position,std::uint32_t body,std::uint32_t wheel,std::uint32_t steering,std::uint32_t info);
 // Debug001a3768/3790/37a0.
 void Calibrate(float load);void SetState();static void SetState(port::driving::init::ByteView);void ComputeAuxiliaries(port::driving::ContactProvider&);
 // Debug001a3b10/3b50/3ba0/3be8.
 void ResolveForces(float request);void ResolveMoreForces();std::uint32_t GetDerivative(std::uint32_t out);float ApplyGroundPenaltyImpulse();
 // Debug001a3e08/3e20/3e38/3e40.
 void SetState(std::uint32_t state);void GetState(std::uint32_t state);std::uint32_t GetState();int GetSize();
 static void port_initialize(port::driving::init::ByteView,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
 static void Calibrate(port::driving::init::ByteView,float,const std::array<float,4>&,const std::array<float,4>&);
 // Port-side bounded leaf bindings, single canonical algorithm body.
 static void ResolveForces(float,port::driving::SolverSuspensionProjection&);
 static void ResolveMoreForces(port::driving::SolverSuspensionProjection&,std::int32_t);
};
}

namespace hp2::recovered {
// E-0044-E01/E-0044-S01. Original declaring owner, explicit port-side binding.
// ECOFF definition001A3540: indep/src/suspension.cpp; no native ABI assertion.
class TwoWheelAckermanSteering : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_001A3F30(std::uint32_t config);
 void SetSteering(std::uint16_t value); //001A3540
 void ComputeAuxiliaries(); //001A3EC8 original empty entry
};
}

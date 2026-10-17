#pragma once
// TASK-0044 E-0044-S01/S02. NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact declaring owner/source: Wheel, indep/src/wheel.cpp.
// Host binding and encoded PS2 pointer parameters are port-side; no host ABI claim.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/skid_services.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::recovered {
// Debug001a16d8; E-0044-S01: exact three-instruction empty function.
void SetSliderValue(int,float,float,float);
class Wheel : public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug001a16e8: original constructor body, explicit host lifetime adapter.
    void port_initialize(std::uint32_t wi,std::uint32_t ti,std::uint32_t ci,std::uint32_t car_state);
    // Debug001a1b20.
    void Calibrate();
    // Debug001a1930 /001a1940.
    void SetState(float angle,float angular_velocity);
    void SetState(float angular_velocity);
    // Debug001a1948. Encoded CarState*/CarTypeInfo*; contact is a port service.
    void SetState(std::uint32_t car,std::uint32_t info,float angle,float velocity,std::uint16_t steering,float suspension,port::driving::ContactProvider&);
    // Debug001a1b30.
    void ComputeAuxiliaries();
    // Debug001a1c80.
    float ExchangeZForces(int touching_ground,float zforce);
    // Debug001a1d58/001a1d80. Encoded vector/matrix/enum pointers.
    float FindDrivingSurface(std::uint32_t point,std::uint32_t rot,port::driving::ContactProvider&);
    float CheckTerrain(std::uint32_t normal,std::uint32_t terrain,std::uint32_t point,std::uint32_t rot,port::driving::ContactProvider&);
    // Debug001a1e88: encoded float* return, original returns null.
    std::uint32_t GetDerivative(std::uint32_t deriv);
    // Debug001a1f50/001a1ff0.
    void CheckForBrakeLock(float ground_force);
    float GetPilotFactor();
    // Debug001a2108: gf is unused by the captured body.
    void GuessGroundForce(std::uint32_t gf,int wascollision);
    // Debug001a2500/001a2660/001a26b8/001a2c00.
    float GetLateralForce(float load,float slip_angle);
    void ModifyGroundForce(std::uint32_t acceleration);
    void FinalizeGroundForce(int wheel);
    void GetHorizontalGroundForceInCarFrame(std::uint32_t force);
    // Debug001a33d0/33e8/3400/3408. Encoded float*.
    void SetState(std::uint32_t state);
    void GetState(std::uint32_t state);
    std::uint32_t GetState();
    int GetSize();
    static void Calibrate(port::driving::init::ByteView,std::uint32_t);
    static void SetState(port::driving::init::ByteView,float,float);
    // Port-side bounded leaf bindings; one canonical algorithm body, no PS2 ABI claim.
    static void GetHorizontalGroundForceInCarFrame(const port::driving::SolverWheelProjection&,port::driving::SolverVector&);
    static void CheckForBrakeLock(float,port::driving::SolverWheelProjection&,const port::driving::SolverWheelGlobals&);
    static float ExchangeZForces(float,port::driving::SolverWheelProjection&,std::int32_t);
    static void ModifyGroundForce(port::driving::WheelIterationProjection&,const std::array<float,2>&);
    // Debug001a2cf8; E-0046-S03. Explicit skid/topology service binding.
    void MaybeMakeSkids(float dt,std::uint32_t car,port::driving::SkidServices&);
    // Unbound legacy host entry reports the missing service, never silently skips effects.
    void MaybeMakeSkids(float dt,std::uint32_t car);
};
}

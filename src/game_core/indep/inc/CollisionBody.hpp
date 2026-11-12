// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/CollisionBody.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/topology.hpp"
#include "game_core/port/track/voronoi.hpp"
namespace port::driving {struct CollisionMotion;class Memory;}
namespace port::race {struct CollisionResources;}
namespace port::race {struct PhysicsConstructionServices;}
#include "game_core/port/driving/memory.hpp"
#include <string>
#include <optional>

namespace hp2::debug {
// Exact Debug names/values, E-0099-W15; no Retail/native ABI assertion.
enum CollisionBodyType {CAR_COLLISION_BODY=0,SMOKEABLE_COLLISION_BODY=1,
    VIRGIN_SMOKEABLE_COLLISION_BODY=2,TRACK_POLYGON_COLLISION_BODY=3};
class CollisionBody {
protected:
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    static bool AnySmokeablesInBox(::port::driving::Memory&,const std::array<float,4>&,const std::array<float,4>&); //001D39A0,E-0112-NIS03.
    CollisionBody(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug001D3520 /001D35E8, E-0099-W15. Constructor/destructor entries
    // are explicit operations; this C++ memory-view lifetime does not invoke them.
    void Sub_001D3520(int,CollisionBodyType);
    void DoSnapshot(std::uint32_t); //001D36D0 E-0099-W76.
    static void SnapshotEverything(::port::driving::Memory&,std::uint32_t); //001D3AA8.
    static std::uint32_t FindCollisionBody(::port::driving::Memory&,int); //001D3B20.

    void Sub_001D35E8(int in_chrg);
    void DisableCollisions();void EnableCollisions(); //001D3B40/001D3B50, E-0099-W96; original byte counter.
    // Debug001D37D0 /001D3888 /001D3A68, E-0099-W15.
    void UpdateMaxCollisionBodyWidth(float);
    void SetBBoxMinMax(const std::array<float,2>&,const std::array<float,2>&);
    void SetDebugName(const std::string&);
    std::uint32_t GetCollision(int index)const; //001D3B70, E-0099-W38
    // Debug001D3A48, E-0099-W15, instruction-confirmed empty original body.
    void PlotToCaffeine(const std::array<float,16>&,const std::array<float,3>&);
};
class StaticCollisionBody : public CollisionBody {
public:
    float GetMassInverse()const; //001D5CF8,E-0110-F09.
    using CollisionBody::CollisionBody;
    // Debug001D5310 /001D5370, E-0099-W15.
    void Sub_001D5310(int,CollisionBodyType);void Sub_001D5370(int);
    void DoSnapshot(std::uint32_t); //001D5398 E-0099-W76.
    int AreCollisionsEnabled() const; //Debug001D5C98, E-0099-W38
    std::uint32_t GetRotation() const; //Debug001D5DB8, E-0099-W38
    std::uint32_t GetVelocity() const; //Debug001D5DC8
    float GetMaxBackupTime() const; //Debug001D5DA0
};
class TrackPolygonCollisionBody : public StaticCollisionBody {
public:
    using StaticCollisionBody::StaticCollisionBody;
    void Sub_001D53A0(int id,std::uint32_t topology_polygon); //E-0099-W38
    void Sub_001D54B8(int); //E-0099-W38
    void DoSnapshot(std::uint32_t); //001D5550 E-0099-W76.
    static void SnapshotEverything(::port::driving::Memory&,std::uint32_t); //001D55B8.
    std::uint32_t GetPosition()const; //001D5750
    int GetTerrainType()const; //001D5760
};
class DynamicCollisionBody : public CollisionBody {
public:
    float GetMassInverse()const; //001D5BE8,E-0110-F09.
    using CollisionBody::CollisionBody;
    // Debug001D3BE0/001D3C60; E-0099-W19. Original raw dynamic base lifecycle.
    void Sub_001D3BE0(int,CollisionBodyType,std::uint32_t topology_coordinate);
    void Sub_001D3C60(int);
    // Debug001D3D90/001D3F28; E-0099-W23. Actual shared geometry/bounds.
    void BuildBoundingVolumes();void UpdatePosition();
    void DoSnapshot(std::uint32_t); //001D3C88 E-0099-W76.
    void BeginMove(float); //Debug001D3FA0, E-0099-W38
    void Interpolate(std::uint32_t position,std::uint32_t rotation,float backup_time); //001D4040
    float GetMaxBackupTime() const; //001D5C00
    void InterpolateVelocity(std::uint32_t output,float backup_time); //001D41E8, E-0099-W38
    void Backup(float backup_time); //001D4298
    void SetPosition(std::uint32_t position); //001D5C08
    void GetCurrentVelocities(std::uint32_t velocity,std::uint32_t angular_velocity); //001D4378
    void RecomputeAuxilliaries(::port::race::PhysicsConstructionServices&); //001D4348
    void SetToPhysicsMode(std::uint32_t); //001D5BF8, E-0110-E03; original JR/NOP.
    int TrackPolygonCheck(std::uint32_t track_polygon); //001D4920, E-0099-W38
    int CreateTrackPolygonCollisionBodies(); //001D46D8
    void MaybeCreateWitnesses(const ::port::race::PhysicsConstructionServices* =nullptr); //001D4468;E-0099-W83.

};
class CarCollisionBody : public DynamicCollisionBody {
public:
    using DynamicCollisionBody::DynamicCollisionBody;
    // Debug001D4A80/001D4BD8; E-0099-W23. Requires complete authored sources.
    void Sub_001D4A80(std::uint32_t car,const ::port::race::CollisionResources&);
    void Sub_001D4BD8(int);
    // Debug001D5B68; E-0099-W23. Original logical pointer in both movement modes.
    void DoSnapshot(std::uint32_t); //001D4C90 E-0099-W76.
    void SetupPhysicsPointers(); // Debug001D4D70, E-0099-W33
    void SetToPhysicsMode(std::uint32_t hitter,::port::race::PhysicsConstructionServices&); //Debug001D4DD0
    void Move(float timestep,::port::race::PhysicsConstructionServices&); //001D4E28, E-0099-W38
    std::uint32_t GetPosition() const;
    std::uint32_t GetRotation() const; //Debug001D5B28, E-0099-W38
    std::uint32_t GetVelocity() const; //Debug001D5B48, E-0099-W38
    int AreCollisionsEnabled() const; //Debug001D4EA8, E-0099-W38
    int IsBlinking(); //001D5AB8, E-0099-W38
    float GetMassInverse()const; //001D4ED0, E-0099-W38
    std::uint32_t GetInertiaTensorInverse(); //001D4F08
    void ApplyImpulse(std::uint32_t impulse,std::uint32_t arm); //001D4F50, E-0099-W38
    void AddDamage(std::uint32_t arm,float magnitude); //001D5008; required live callees retain guards
    void AnimateCollisionReaction(std::uint32_t impulse,float magnitude); //001D5188
};
// Debug001D3788 /001D58C8, E-0099-W15.
int CollisionBodyFlipFunction(const ::port::driving::Memory&,std::uint32_t,std::uint32_t);
void Sub_001D58C8(::port::driving::Memory&);
// Debug001D2938/001D2A58/001D3A58; E-0099-W19.
void InitCollisionBody(::port::driving::Memory&);void CloseCollisionBody(::port::driving::Memory&);
int IsYawControlInducingCollisionBody(const ::port::driving::Memory&,std::uint32_t); //001D31E8, E-0099-W38
std::uint16_t GetNextCollisionBodyID(const ::port::driving::Memory&);
class CollisionBodyWitness {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    CollisionBodyWitness(::port::driving::Memory& m,std::uint32_t a):memory_(m),address_(a){}
    // Debug001D2BF0; E-0099-W19. Inactive collision/Caffeine and non-track bodies.
    std::uint32_t GetCollision(std::uint32_t contact_point); //001D2FB8, E-0099-W38
    void BeginCollision(std::uint32_t contact_point); //001D2FF0
    void EndCollision(); //001D3080
    void DoSnapshot(std::uint32_t); //001D2DA0 E-0099-W76.
    static void SnapshotEverything(::port::driving::Memory&,std::uint32_t); //001D2E40.
    void TestForContact(std::uint32_t contact_list,std::optional<std::uint32_t> port_padding); //001D3230
    void Sub_001D2AC8(std::uint32_t body1,std::uint32_t body2); //Debug001D2AC8, E-0099-W38
    void Sub_001D2BF0(int);
    int HasExpired() const; //Debug001D30F0, E-0099-W38
    static int DoesWitnessExist(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //001D34D0
};
// Debug001D3480; E-0099-W19.
void EliminateAllWitnesses(::port::driving::Memory&);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarCollisionBody::Move() [0x002B8700]
namespace hp2::recovered {
class CarCollisionBody {
public:
    // Debug001D4ED0/001D4F08/001D4F50; E-0048-D04. Explicit native-object bindings.
    static float GetMassInverse(port::driving::Memory&,std::uint32_t rigid_body);
    static std::array<float,16> GetInertiaTensorInverse(port::driving::Memory&,std::uint32_t rigid_body);
    static void ApplyImpulse(port::driving::Memory&,std::uint32_t rigid_body,std::uint32_t car,const std::array<float,4>& impulse,const std::array<float,4>& arm);
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001D7FA0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7fa0.md.
    // Original Debug reference: CarCollisionBody::~CarCollisionBody(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D7FA0();
    // ELF 0x001D8058; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8058.md.
    // Original Debug reference: CarCollisionBody::DoSnapshot(ReplaySnapshot *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void DoSnapshot();
    // ELF 0x001D8138; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8138.md.
    // Original Debug reference: CarCollisionBody::SetupPhysicsPointers(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetupPhysicsPointers();
    // ELF 0x001D8198; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8198.md.
    // Original Debug reference: CarCollisionBody::SetToPhysicsMode(CollisionBody *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetToPhysicsMode();
    // ELF 0x001D81F0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d81f0.md.
    // Original Debug reference: CarCollisionBody::Move(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Move();
    // ELF 0x001D8270; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8270.md.
    // Original Debug reference: CarCollisionBody::AreCollisionsEnabled(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AreCollisionsEnabled();
    // ELF 0x001D8298; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8298.md.
    // Original Debug reference: CarCollisionBody::GetMassInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMassInverse();
    // ELF 0x001D82D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d82d0.md.
    // Original Debug reference: CarCollisionBody::GetInertiaTensorInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetInertiaTensorInverse();
    // ELF 0x001D8318; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8318.md.
    // Original Debug reference: CarCollisionBody::ApplyImpulse(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ApplyImpulse();
    // ELF 0x001D8E58; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e58.md.
    // Original Debug reference: CarCollisionBody::GetFriction(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetFriction();
    // ELF 0x001D8E68; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e68.md.
    // Original Debug reference: CarCollisionBody::GetBounciness(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetBounciness();
    // ELF 0x001D8E78; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e78.md.
    // Original Debug reference: CarCollisionBody::GetTerrainType(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTerrainType();
    // ELF 0x001D8E80; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8e80.md.
    // Original Debug reference: CarCollisionBody::IsBlinking(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsBlinking();
    // ELF 0x001D8ED8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ed8.md.
    // Original Debug reference: CarCollisionBody::IsMoveable(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsMoveable();
    // ELF 0x001D8EE0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ee0.md.
    // Original Debug reference: CarCollisionBody::ConsiderImpulse(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ConsiderImpulse();
    // ELF 0x001D8EF0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8ef0.md.
    // Original Debug reference: CarCollisionBody::GetRotation(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRotation();
    // ELF 0x001D8F10; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f10.md.
    // Original Debug reference: CarCollisionBody::GetVelocity(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetVelocity();
    // ELF 0x001D8F30; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f30.md.
    // Original Debug reference: CarCollisionBody::GetPosition(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetPosition();
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DynamicCollisionBody::Move() [0x002B8658]
// Diagnostic text (not a callable signature): DynamicCollisionBody::MaybeCreateWitnesses() [0x002B8678]
// Diagnostic text (not a callable signature): DynamicCollisionBody::CreateTrackPolygonCollisionBodies() [0x002B86A8]
namespace hp2::recovered {
class DynamicCollisionBody {
public:
    // Debug001D3D90/001D3FA0/001D4040/001D4298; E-0048-T06; host value bindings.
    static port::track::CollisionCylinder BuildBoundingVolumes(port::driving::Memory&,const port::track::VoronoiPolyhedron&);
    static port::driving::CollisionMotion BeginMove(const port::driving::Memory&,std::uint32_t,float);
    static port::track::VoronoiPose Interpolate(const port::driving::Memory&,std::uint32_t,const port::driving::CollisionMotion&,float);
    static void Backup(port::driving::Memory&,std::uint32_t,const port::driving::CollisionMotion&,float);

    // Debug 0x001D4920; E-0048-T02; host-owned cylinder/polygon binding.
    static bool TrackPolygonCheck(const port::track::CollisionCylinder&,const port::track::Polygon&);
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001D7028; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7028.md.
    // Original Debug reference: DynamicCollisionBody::~DynamicCollisionBody(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D7028();
    // ELF 0x001D7050; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7050.md.
    // Original Debug reference: DynamicCollisionBody::DoSnapshot(ReplaySnapshot *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void DoSnapshot();
    // ELF 0x001D7408; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7408.md.
    // Original Debug reference: DynamicCollisionBody::Interpolate(bVector3 *, bMatrix4 *, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Interpolate();
    // ELF 0x001D7660; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7660.md.
    // Original Debug reference: DynamicCollisionBody::Backup(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Backup();
    // ELF 0x001D7710; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7710.md.
    // Original Debug reference: DynamicCollisionBody::RecomputeAuxilliaries(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RecomputeAuxilliaries();
    // ELF 0x001D7740; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7740.md.
    // Original Debug reference: DynamicCollisionBody::GetCurrentVelocities(bVector3 &, bVector3 &); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetCurrentVelocities();
    // ELF 0x001D7760; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7760.md.
    // Original Debug reference: DynamicCollisionBody::GetTestImpulseVelocities(bVector3 &, bVector3 &, bVector3 &, bVector3 &); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTestImpulseVelocities();
    // ELF 0x001D7E08; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d7e08.md.
    // Original Debug reference: DynamicCollisionBody::ChangeInEnergy(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ChangeInEnergy();
    // ELF 0x001D8F98; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8f98.md.
    // Original Debug reference: DynamicCollisionBody::ApplyImpulse(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ApplyImpulse();
    // ELF 0x001D8FA0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fa0.md.
    // Original Debug reference: DynamicCollisionBody::GetInertiaTensorInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetInertiaTensorInverse();
    // ELF 0x001D8FB0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fb0.md.
    // Original Debug reference: DynamicCollisionBody::GetMassInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMassInverse();
    // ELF 0x001D8FC0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fc0.md.
    // Original Debug reference: DynamicCollisionBody::SetToPhysicsMode(CollisionBody *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetToPhysicsMode();
    // ELF 0x001D8FC8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fc8.md.
    // Original Debug reference: DynamicCollisionBody::GetMaxBackupTime(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMaxBackupTime();
    // ELF 0x001D8FD0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8fd0.md.
    // Original Debug reference: DynamicCollisionBody::SetPosition(bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetPosition();
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackPolygonCollisionBody::TrackPolygonCollisionBody() [0x002B8720]
namespace hp2::recovered {
class TrackPolygonCollisionBody {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001D8880; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8880.md.
    // Original Debug reference: TrackPolygonCollisionBody::~TrackPolygonCollisionBody(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D8880();
    // ELF 0x001D8918; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8918.md.
    // Original Debug reference: TrackPolygonCollisionBody::DoSnapshot(ReplaySnapshot *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void DoSnapshot();
    // ELF 0x001D8B18; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8b18.md.
    // Original Debug reference: TrackPolygonCollisionBody::GetPosition(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetPosition();
    // ELF 0x001D8B28; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d8b28.md.
    // Original Debug reference: TrackPolygonCollisionBody::GetTerrainType(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTerrainType();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class StaticCollisionBody {
public:
    // ELF 0x001D9048; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9048.md.
    // Original Debug reference: StaticCollisionBody::ChangeInEnergy(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ChangeInEnergy();
    // ELF 0x001D9058; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9058.md.
    // Original Debug reference: StaticCollisionBody::IsMoveable(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsMoveable();
    // ELF 0x001D9060; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9060.md.
    // Original Debug reference: StaticCollisionBody::AreCollisionsEnabled(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AreCollisionsEnabled();
    // ELF 0x001D9070; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9070.md.
    // Original Debug reference: StaticCollisionBody::ApplyImpulse(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ApplyImpulse();
    // ELF 0x001D9078; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9078.md.
    // Original Debug reference: StaticCollisionBody::ConsiderImpulse(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ConsiderImpulse();
    // ELF 0x001D9088; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9088.md.
    // Original Debug reference: StaticCollisionBody::SetToPhysicsMode(CollisionBody *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetToPhysicsMode();
    // ELF 0x001D9090; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9090.md.
    // Original Debug reference: StaticCollisionBody::GetFriction(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetFriction();
    // ELF 0x001D90A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90a0.md.
    // Original Debug reference: StaticCollisionBody::GetBounciness(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetBounciness();
    // ELF 0x001D90B0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90b0.md.
    // Original Debug reference: StaticCollisionBody::GetInertiaTensorInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetInertiaTensorInverse();
    // ELF 0x001D90C0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90c0.md.
    // Original Debug reference: StaticCollisionBody::GetMassInverse(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMassInverse();
    // ELF 0x001D90D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90d0.md.
    // Original Debug reference: StaticCollisionBody::GetTestImpulseVelocities(bVector3 &, bVector3 &, bVector3 &, bVector3 &); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTestImpulseVelocities();
    // ELF 0x001D90F0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d90f0.md.
    // Original Debug reference: StaticCollisionBody::GetCurrentVelocities(bVector3 &, bVector3 &); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetCurrentVelocities();
    // ELF 0x001D9110; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9110.md.
    // Original Debug reference: StaticCollisionBody::RecomputeAuxilliaries(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RecomputeAuxilliaries();
    // ELF 0x001D9118; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9118.md.
    // Original Debug reference: StaticCollisionBody::Interpolate(bVector3 *, bMatrix4 *, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Interpolate();
    // ELF 0x001D9158; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9158.md.
    // Original Debug reference: StaticCollisionBody::Backup(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Backup();
    // ELF 0x001D9160; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9160.md.
    // Original Debug reference: StaticCollisionBody::Move(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Move();
    // ELF 0x001D9168; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9168.md.
    // Original Debug reference: StaticCollisionBody::GetMaxBackupTime(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMaxBackupTime();
    // ELF 0x001D9178; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9178.md.
    // Original Debug reference: StaticCollisionBody::SetPosition(bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetPosition();
    // ELF 0x001D9180; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9180.md.
    // Original Debug reference: StaticCollisionBody::GetRotation(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRotation();
    // ELF 0x001D9190; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d9190.md.
    // Original Debug reference: StaticCollisionBody::GetVelocity(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetVelocity();
};
} // namespace hp2::recovered

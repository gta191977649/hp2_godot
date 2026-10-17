#pragma once
// TASK-0042: host projection interfaces; no original header/ABI claim.
#include "game_core/port/driving/byte_view.hpp"
#include "game_core/port/driving/memory.hpp"

namespace port::driving { struct ContactProvider; }

// Host declarations formerly partitioned in Sub_00137100.hpp
// Port-side byte views of initialization dependencies; original source path unknown.
// Not an implementation of the whole 00137100 entry, which still throws.
#include <array>
#include <cstddef>
#include <cstdint>

namespace port::driving::init {
// Explicit PS2 little-endian images retain untouched bytes and encoded addresses.
// Bounds checks belong to the port adapter, not the original algorithm.

// All entries below: SLUS_203.62, revision unknown, SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0038-I01: ELF 0025AC68, four quadword stores.
std::array<float,16> bIdentity();
// E-0038-I02: ELF 0025AC98, scalar 4x4 affine inverse; ordinary finite scope.
std::array<float,16> bInvertMatrix(const std::array<float,16>&);
// E-0038-I03: ELF 0019CC58, config+0 scalar / config+0x10 matrix.
void CalculateInertiaTensor(ByteView,ByteView);
// E-0038-I04: ELF 0019C9B0, config pointer supplied as encoded PS2 word.
void Sub_0019C9B0(ByteView,std::uint32_t,ByteView);
// E-0038-I05: ELF 001A3F10, arguments a1/a2/a3/t0 are encoded words.
void Sub_001A3F10(ByteView,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// E-0038-I06: ELF 001A5EA0, arguments a1/a2/a3/t0/t1 are encoded words.
void Sub_001A5EA0(ByteView,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// E-0038-I07: ELF 0019C170; argument is encoded address of list head.
void Sub_0019C170(ByteView,std::uint32_t);
// E-0038-I08: ELF 0019CC40; null second argument leaves +0x160 unchanged.
void Sub_0019CC40(ByteView,const std::array<std::uint32_t,4>*);
// E-0038-I09: ELF 001A5F88; explicit resolved vectors replace pointer walks.
void Sub_001A5F88(ByteView,float,const std::array<float,4>&,const std::array<float,4>&);
// E-0038-I10: ELF 001A4348; explicit word replaces global DAT_00330CA0.
void Sub_001A4348(ByteView,std::uint32_t);
// E-0038-I11: ELF 001A4158; f12/f13 are stored to +0x1c/+0x20.
void Sub_001A4158(ByteView,float,float);
// E-0038-I12: ELF 001A5FB0, two zero stores.
void Sub_001A5FB0(ByteView);
}

// Host declarations formerly partitioned in Sub_00137100_runtime.hpp
#include <functional>
#include <optional>

namespace port::driving::native {
// Port services for two performance-counter calls and the external 00227438
// predicate. No absent callback silently succeeds. Original source path unknown.
struct InitServices {
    std::function<std::uint64_t()> Sub_00250F38;
    std::function<std::uint32_t(std::uint64_t,std::uint64_t)> Sub_00250F48;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> Sub_00227438;
    std::function<void(Memory&,std::uint32_t)> ResolveForces;
    // Explicit projection of the original uninitialized temporary's fourth lane.
    std::optional<std::uint32_t> Field_00137938_stack_w;
    ContactProvider* contact=nullptr;
};
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0038-I14: full list append, signed virtual adjustor, known component counts.
void AddComponent(Memory&,std::uint32_t,std::uint32_t);
// E-0038-I15: list reset dispatch and exact performance-counter call order.
void Sub_0019C808(Memory&,std::uint32_t,InitServices&);
// E-0038-I16: max.S f12,0 (decompiler int-cast is incorrect).
float GetDownforceAtSpeed(Memory&,std::uint32_t,float);
// E-0038-I17/I18: complete late constructor field/config synchronization.
void Sub_001375F0(Memory&,std::uint32_t);
void InitializeStateFromCar(Memory&,std::uint32_t);
// E-0038-I19/I20: complete wheel/controller reset paths.
void Sub_001A4358(Memory&,std::uint32_t);
void Sub_001A5FC0(Memory&,std::uint32_t,ContactProvider&);
// E-0038-I21/I22: vector velocity helper and contact response.
std::array<float,4> GetPointVelocity(const std::array<float,4>&,const std::array<float,4>&,const std::array<float,4>&);
float ApplyGroundPenaltyImpulse(Memory&,std::uint32_t);
// E-0038-I23/I24: predicate and three-way state classification.
bool IsBlinking(Memory&,std::uint32_t);
// E-0038-I29: conditional external predicate and +0x858 store.
void PleaseResetMeLater(Memory&,std::uint32_t,InitServices&);
std::uint32_t CheckDampingSuitability(Memory&,std::uint32_t);
// E-0038-I25/I26: post-reset state routes and final adjusted virtual dispatch.
void BottomOut(Memory&,std::uint32_t,InitServices&);
void Sub_00137788(Memory&,std::uint32_t,InitServices&);
// E-0038-I27: complete initialization call/store graph. Memory allocation is a
// port-owned logical arena; original pool allocator implementation is not claimed.
// All object/config/global/table dependencies must be mapped before entry.
std::uint32_t Sub_00137100(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,InitServices&);
}

// Host declarations formerly partitioned in Sub_00137C88.hpp
// TASK-0038, bounded native projection; original source path and parameter names unknown.
namespace port::driving {
// ELF 00137C88; E-0038-003; SLUS_203.62 revision unknown;
// SHA256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Explicit arguments replace f12..f17, a0 and *a1. Ordinary finite float scope.
float YawFrictionBoost(float, float, float, float, float, float, int, float&);
}

// Host declarations formerly partitioned in Sub_00137D90.hpp
// TASK-0041 E-0041-R01; retail 0x00137D90: PhysicsCar::Move(float).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
// TASK-0038 logical-memory projections. Original source path unknown.
#include <functional>
#include <optional>
namespace port::driving::native {
// Explicit port-side services for external calls; missing required callbacks throw.
struct DrivingServices {
    std::function<std::uint64_t()> Sub_00250F38;
    std::function<std::uint32_t(std::uint64_t,std::uint64_t)> Sub_00250F48;
    std::function<void(Memory&,std::uint32_t,float)> IntegrateEuler;
    std::function<void(Memory&,std::uint32_t)> eOrthonormalizeMatrix;
    std::function<void(Memory&,std::uint32_t)> Sub_0019D980;
    std::function<void(Memory&,std::uint32_t)> Sub_0018C030;
    std::function<void(Memory&,std::uint32_t,float)> UpdateCar;
    std::function<void(Memory&,std::uint32_t,std::uint32_t,float)> Sub_001A5520;
    std::function<float(Memory&,std::uint32_t)> Sub_001B50C8;
    std::optional<std::uint32_t> Field_001389A4_stack_w;
};
// SLUS_203.62 revision unknown; ELF SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// ELF001A5D60; E-0038-E07, original16-bit control and two matrix stores.
void Sub_001A5D60(Memory&,std::uint32_t,std::uint16_t);
// ELF00139490; E-0038-E08, component scale publication.
void SetBoost(Memory&,std::uint32_t,float);
// ELF001C0888/001C08D0/00135EB8/00135E18; E-0038-E09.
float GetYawAlways(Memory&,std::uint32_t);
float GetYaw(Memory&,std::uint32_t);
float GetPostCollisionYawControlBoost(Memory&,std::uint32_t);
bool ShouldVisualEffectsBeGenerated(Memory&,std::uint32_t);
// ELF0025A3D0/A410/A4A8; E-0038-E11, original unsigned-modulo RNG chain.
std::uint32_t bRandom(Memory&,std::uint32_t,std::uint32_t);
float bRandom(Memory&,std::uint32_t,float);
float bRandom(Memory&,float);
// ELF00137D90; E-0038-E10/E13. car+20!=1; Race state4 requires A758==0.
// Unsupported original mode branches and absent services throw at their address.
float Move(Memory&,std::uint32_t,float,DrivingServices&);
}

// Host declarations formerly partitioned in Sub_0019D4D0.hpp
// TASK-0041 E-0041-R03; retail 0x0019D4D0: RigidBody::GetDerivative(float *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
// TASK-0038, E-0038-B01..B07. Port-side logical-address projection; original path unknown.
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include <functional>
namespace port::driving::native {
using Vector=std::array<float,4>;
using Matrix=std::array<float,16>;
// ELF0025AE18, E-0038-B01. Column-major product, preserves instruction grouping.
Matrix bMulMatrix(const Matrix&,const Matrix&);
Matrix transpose(const Matrix&);
Vector transform(Memory&,const Matrix&,const Vector&);
// ELF0019CD90/0019CE80, E-0038-S03/S04 logical-address adapters.
void ApplyForce(Memory&,std::uint32_t,const Vector&,const Vector&);
void ApplyForceInFrame(Memory&,std::uint32_t,const Vector&,const Vector&);
// ELF0019D370, E-0038-B02; caller supplies original angular vector and matrix.
Matrix VectorStarMatrix(const Vector&,const Matrix&);
// ELF0019CC88/0019D848, E-0038-B03/B04. Unconsumed stack W is explicit tool input.
void Sub_0019CC88(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
void Sub_0019D848(Memory&,std::uint32_t,float unused_stack_w=0);
// ELF0019D4D0, E-0038-B05. Writes the original derivative buffer, including affine lane15.
void GetDerivative(Memory&,std::uint32_t,std::uint32_t);
// Port dispatch seam, original adjusted virtual targets resolved by its implementation.
struct IntegrationCalls {
    std::function<void(Memory&,std::uint32_t,std::uint32_t)> derivative;
    std::function<void(Memory&,std::uint32_t)> after;
};
// ELF0019C670/0019C290, E-0038-B06/B07.
void Sub_0019C670(Memory&,std::uint32_t,std::uint32_t,const IntegrationCalls&);
void IntegrateEuler(Memory&,float,std::uint32_t,const IntegrationCalls&);
}

// Host declarations formerly partitioned in Sub_00138E08.hpp
// TASK-0041 E-0041-V01; retail 0x00138E08: PhysicsCar::ResolveForces(void).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
// TASK-0038 E-0038-B09..B13: native adapters over one authoritative logical state.
namespace port::driving::native {
void eOrthonormalizeMatrix(Memory&,std::uint32_t);
void Sub_001ADD70(Memory&,std::uint32_t,std::uint32_t);
void JumpStabilizer(Memory&,std::uint32_t);
void IterateForces(Memory&,std::uint32_t);
void ResolveForces(Memory&,std::uint32_t);
void UpdateCar(Memory&,std::uint32_t,float);
void integrate(Memory&,std::uint32_t,float,InitServices&);
}

// Host declarations formerly partitioned in Sub_001A4E88.hpp
// TASK-0041 E-0041-M01; retail 0x001A4E88: Wheel::ModifyGroundForce(bVector2 *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include <array>
namespace port::driving {
// Port-side field projection, not an original type/layout. Original source path unknown.
struct WheelIterationProjection {
    float config_Field_0xc=0,Field_0x24=0,Field_0xb0=0,Field_0xc0=0,Field_0x108=0,Field_0x114=0;
};
// ELF 001A4E88; E-0038-005; SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Finite ordinary-float slice. No physical field names or PS2 exceptional FPU equivalence.
void ModifyGroundForce(WheelIterationProjection&,const std::array<float,2>&);
}

// Host declarations formerly partitioned in Sub_00138F38.hpp
#include <array>
#include <cstdint>

namespace port::driving {
// Port-side projections only: these are not original C++ types or byte layouts.
// SLUS_203.62, revision unknown, SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Finite ordinary-float domain; PS2 VU/FPU exceptional values and exact rounding
// remain unverified. Original source paths and field semantics are unknown.
using SolverVector = std::array<float, 4>;
using SolverMatrix = std::array<float, 16>;
struct SolverMathContext {
    // Explicit mutable original global, initialized by 0025AFA8; no assumed value.
    SolverVector Field_00320A60;
};
struct SolverBodyProjection {
    SolverVector Field_0x10{};
    SolverMatrix Field_0x20{};
    SolverVector Field_0xf0{}, Field_0x100{};
    float Field_0x118 = 0, Field_0x148 = 0;
};
struct SolverWheelProjection : WheelIterationProjection {
    float Field_0xb4 = 0, Field_0xb8 = 0, Field_0xc4 = 0, Field_0xc8 = 0;
    float Field_0x118 = 0, Field_0x158 = 0;
    float Field_0x20 = 0, Field_0x170 = 0, Field_0x174 = 0;
    float config_0x14_Field_0x4 = 0, config_0x14_Field_0x8 = 0;
    std::uint32_t Field_0x10c = 0;
    std::uint8_t Field_0x154 = 0, Field_0x155 = 0;
    SolverVector Field_0x80{}, Field_0xa0{}, Field_0x120{}, Field_0x130{}, Field_0x140{};
};
struct SolverSuspensionProjection {
    float Field_0x1c = 0, Field_0x80 = 0, Field_0x84 = 0, Field_0xa0 = 0;
    std::int32_t Field_0x70 = 0;
    SolverWheelProjection* Field_0x90 = nullptr;
    SolverBodyProjection* Field_0x94 = nullptr;
};
// ELF 0025AF20; E-0038-S01. Snapshot all inputs before stores; supports aliasing.
void bTransposeMatrix(SolverMatrix&, const SolverMatrix&);
// ELF 0025AED0 and thunk 0019D368; E-0038-S02. Uses explicit mutable global.
void bMulMatrix(SolverVector&, const SolverMatrix&, const SolverVector&, const SolverMathContext&);
void bMulMatrixVector(SolverVector&, const SolverMatrix&, const SolverVector&, const SolverMathContext&);
// ELF 0019CD90/0019CE80/0019CEF8; E-0038-S03/S04/S05.
void ApplyForce(SolverBodyProjection&, const SolverVector&, const SolverVector&);
void ApplyForceInFrame(SolverBodyProjection&, const SolverVector&, const SolverVector&, const SolverMathContext&);
void ApplyMoment(SolverBodyProjection&, const SolverVector&);
// ELF 001A5428; E-0038-S06. Writes XYZ only, preserving destination W.
void GetHorizontalGroundForceInCarFrame(const SolverWheelProjection&, SolverVector&);
struct SolverWheelGlobals {
    float Field_00330CC0 = 0, Field_00330CC4 = 0, Field_00330CC8 = 0, Field_00330CCC = 0;
};
// ELF 001A4778; E-0038-S11. Dynamic globals are explicit inputs.
void CheckForBrakeLock(float, SolverWheelProjection&, const SolverWheelGlobals&);
// ELF 001A44A8/001A6330/001A6370; E-0038-S07/S08/S09.
float ExchangeZForces(float, SolverWheelProjection&, std::int32_t);
void Sub_001A6330(float, SolverSuspensionProjection&);
void ResolveMoreForces(SolverSuspensionProjection&, std::int32_t Field_00330D10);

// Port-side dependency interface, not a recovered original class. Untranslated
// whole dependencies throw at their ELF address; fixtures may override them.
struct SolverDependencies {
    virtual ~SolverDependencies() = default;
    virtual void GuessClutchTorque();
    virtual void GuessGroundForce(SolverWheelProjection&, int, std::int8_t);
    virtual void ModifyClutchTorque();
    virtual void FinalizeGroundForce(SolverWheelProjection&, int);
    virtual void FinalizeClutchTorque();
};
// ELF 00138F38..00139183; E-0038-S10. Exact five-iteration orchestration
// with explicit dependency seams. This does not implement the driving closure.
void IterateForces(SolverBodyProjection&, std::array<SolverWheelProjection, 4>&,
                 const std::array<SolverSuspensionProjection, 4>&, std::int8_t,
                 SolverDependencies&, const SolverMathContext&);
}

// Host declarations formerly partitioned in Sub_00152070.hpp
// TASK-0038 E-0038-C01/C02. Port-side environment query adapter and recovered roughness.
namespace port::driving {
struct ContactProvider {
    virtual ~ContactProvider()=default;
    virtual float GetElevation(Memory&,std::uint32_t owner,std::uint32_t point,
        std::uint32_t material,std::uint32_t plane,std::uint32_t found)=0;
};
// Explicit synthetic environment geometry, not a recovered original road query.
// Material1 is the ELF table row with SKID_ROAD at00318CB0 (E-0038-C01).
struct PlaneContact final:ContactProvider {
    float height=0;
    float GetElevation(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
};
// ELF00153458/00153498; E-0038-C02; native offsets/data, not host random noise.
float Sub_00153458(Memory&,std::int32_t,std::int32_t);
float Sub_00153498(Memory&,std::uint32_t,std::int32_t);
}

// Host declarations formerly partitioned in Sub_0018BCE0.hpp
// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include <array>
#include <cstdint>
#include <functional>

namespace port::driving {
// Port-side projections, NOT original C++ types or byte layouts. Original path unknown.
// All field suffixes describe original offsets; nested pointers are resolved by the host.
struct DrivetrainWheelProjection {
    float Field_0x20=0, Field_0x24=0, Field_0x108=0, Field_0x114=0;
    std::int32_t Field_0x10c=0;
    std::int8_t Field_0x155=0;
    float Field_0x160=0, config_Field_0xc=0;
};
struct DrivetrainProjection {
    float Field_0xc=0;
    std::int32_t Field_0x2c=0, Field_0x30=0, Field_0x34=0, Field_0x38=0;
    float Field_0x40=0, Field_0x44=0;
    std::int32_t Field_0x48=0;
    float Field_0x4c=0;
    std::array<float,9> Field_0x50{}, Field_0x74{};
    float config_Field_0x8=0, config_Field_0xc=0, config_Field_0x10=0, config_Field_0x14=0;
    std::int32_t config_Field_0x18=0;
    std::array<float,9> config_Field_0x20{};
    float engine_Field_0xc=0, engine_Field_0x44=0, engine_Field_0x48=0;
    float engine_config_Field_0x0=0, engine_config_Field_0x8=0, engine_config_Field_0xc=0;
    std::int32_t car_Field_0x3a4=0;
    std::array<DrivetrainWheelProjection,4> wheels{};
};
// Dynamic gp-relative values are explicit inputs, not assumed stock constants.
struct DrivetrainGlobals {
    float Field_0032e358=0, Field_0032e35c=0, Field_0032e370=0;
    float Field_0032e374=0, Field_0032e378=0;
    float Field_0032e388=0, Field_0032e38c=0, Field_0032e390=0, Field_0032e394=0;
    float Field_003308b8=0, Field_003308bc=0, Field_003308c4=0;
    float Field_00330c88=0;
};
struct EngineTimerProjection { std::int32_t Field_0x50=0; float Field_0x54=0; };
// ELF addresses below; E-0038-D01..D07. SLUS_203.62 executable revision unknown,
// SHA256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Ordinary finite scalar projection only; PS2 exceptional FPU behavior unverified.
void DecrementNitro(EngineTimerProjection&,std::int32_t,float);
std::int32_t SetSportGear(DrivetrainProjection&,std::int32_t);
std::int32_t SetDesiredGear(DrivetrainProjection&,std::int32_t,const DrivetrainGlobals&);
float GetDifferentialAngularAcceleration(const DrivetrainProjection&,const DrivetrainGlobals&);
void ApplyClutchTorque(float,DrivetrainProjection&,const DrivetrainGlobals&);
// The unresolved 0018AAB0 evaluation must be supplied explicitly. Empty hook throws.
using EngineEvaluation = std::function<float()>;
void GuessClutchTorque(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation& = {});
void ModifyClutchTorque(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation& = {});
void FinalizeClutchTorque();
}

// Host declarations formerly partitioned in Sub_0018BCE0_memory.hpp
// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
// Port-side adapters to the E-0038-D01..D07 scalar projections. No state is retained here.
namespace port::driving::native {
std::int32_t SetSportGear(Memory&,std::uint32_t,std::int32_t);
std::int32_t SetDesiredGear(Memory&,std::uint32_t,std::int32_t);
void DecrementNitro(Memory&,std::uint32_t);
void GuessClutchTorque(Memory&,std::uint32_t);
void ModifyClutchTorque(Memory&,std::uint32_t);
}

// Host declarations formerly partitioned in Sub_0018F808.hpp
// TASK-0038, E-0038-E01..E06. Port-side logical-address projection, not original ABI.
// SLUS_203.62 executable revision unknown; ELF SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include <cstdint>
namespace port::driving {
// Port-side finite CVT.W.S projection (truncate). Exceptional operands throw.
std::int32_t cvt_w_s(float);
// ELF00189168/00189120/0018F808; E-0038-E01. Original table+0 pointer/+4 count.
void CalcIndexMultiplier(Memory&,std::uint32_t);
void SetMinMax(Memory&,std::uint32_t,float,float);
void Sub_0018F808(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,float,float);
// ELF0018F838; E-0038-E01, bounded ordinary finite scalar interpolation.
float Sub_0018F838(Memory&,std::uint32_t,float);
// ELF0018F9F0; E-0038-E01, irregular x/y pairs.
float Sub_0018F9F0(Memory&,std::uint32_t,float);
// ELF0018AC38; E-0038-E02, direct logical addresses replace stack table carrier.
void Sub_0018AC38(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,float);
// ELF0018A7E8/0018AE70; E-0038-E03, preserve fields not written by constructors.
void Sub_0018A7E8(Memory&,std::uint32_t,std::uint32_t,std::uint32_t);
void Sub_0018AE70(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// ELF0018AA28/0018AAB0/0018A988/0018A9D8; E-0038-E04.
float GetTorque(Memory&,std::uint32_t);
float GetTotalEngineTorque(Memory&,std::uint32_t);
void Sub_0018A988(Memory&,std::uint32_t);
std::uint32_t Sub_0018A9D8(Memory&,std::uint32_t,std::uint32_t);
// ELF0018B008/0018B0D8/0018B178/0018BC18/0018BC68/0018B4A0; E-0038-E05.
float Sub_0018B008(Memory&,std::uint32_t);
float Sub_0018B0D8(Memory&,std::uint32_t);
void Sub_0018B178(Memory&,std::uint32_t,float);
std::uint32_t Sub_0018BC18(Memory&,std::uint32_t);
std::uint32_t Sub_0018BC68(Memory&,std::uint32_t);
void Sub_0018B4A0(Memory&,std::uint32_t);
// ELF0018B698; E-0038-E05, state-indexed derivative scalar.
std::uint32_t Sub_0018B698(Memory&,std::uint32_t,std::uint32_t);
// ELF0018C368/0018C1C8; E-0038-E06, exact constant-return leaves.
std::uint32_t Sub_0018C368();
std::uint32_t Sub_0018C1C8();
}

// Host declarations formerly partitioned in Sub_0019DA30.hpp
// TASK-0038, E-0038-I30..I32, original source paths unknown.
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include <functional>
namespace port::driving::native {
// ELF0019DA30 initializes body damping headers, not their ELF-resident sample arrays.
void Sub_0019DA30(Memory&);
// ELF001395B0 initializes irregular table330060, including mutable points2F0570.
void Sub_001395B0(Memory&);
// Port-side observation/replay seam; each actual original call must be supplied.
struct ScheduleServices {
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> Sub_00183358;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> Sub_00183498;
};
// ELF001A33A8/33C0 and00136DD0, exact quantization constants/instruction grouping.
float Sub_001A33A8(Memory&,std::uint32_t);
std::uint32_t GetQuantizedRealTimeElapsed(float);
std::uint32_t Sub_00136DD0(float);
// ELF001A33F0 chooses bounded quantized frame delta from actual elapsed input.
void Sub_001A33F0(Memory&,float,ScheduleServices&);
// ELF001A3500,001A3598,001A35E0,001A35F0, frame/game clock stores.
void Sub_001A3500(Memory&);
void Sub_001A3598(Memory&);
void Sub_001A35E0(Memory&,float);
void Sub_001A35F0(Memory&);
}

// Host declarations formerly partitioned in Sub_001A4930.hpp
#include <cstdint>
namespace port::driving {
// TASK-0038; address-anchored ordinary finite PS2 arithmetic projections.
// SLUS_203.62 revision unknown, SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Original field meanings/source paths unknown. Globals/tables reside in Memory;
// no host sine/asin approximation or invented stock tire parameters are used.
// ELF0025A540/A5E0/A748/A890; E-0038-T01..T04.
float bSin(Memory&,std::uint16_t);
float bSin(Memory&,float);
std::uint16_t bASin(Memory&,float);
std::int16_t bATan(Memory&,float,float);
// ELF0025AB50/AA78/A300; E-0038-T14. Original fixed-point heading route.
std::int32_t bDiv(std::int32_t,std::int32_t);
std::uint16_t bFixATan(Memory&,std::int32_t);
std::int16_t bFixATan(Memory&,std::int32_t,std::int32_t);
// ELF001A46B0/001A63C0; E-0038-T15. Derivative buffer callbacks.
std::uint32_t Sub_001A46B0(Memory&,std::uint32_t,std::uint32_t);
std::uint32_t Sub_001A63C0(Memory&,std::uint32_t,std::uint32_t);
// ELF0025AFA8; E-0038-T17. Original mutable math-global initializer.
void Sub_0025AFA8(Memory&);
// ELF001A57E0; E-0038-T16. Required original static tire table initialization.
void Sub_001A57E0(Memory&);
// ELF001A4818/4D28/4930/4EE0; E-0038-T05..T08.
float GetPilotFactor(Memory&,std::uint32_t);
float GetLateralForce(Memory&,float,float,std::uint32_t);
void GuessGroundForce(Memory&,std::uint32_t,std::int32_t,std::int32_t);
void FinalizeGroundForce(Memory&,std::uint32_t,std::int32_t);
// ELF0011B248,0011B6E0,001C0D78; E-0038-T09..T11.
void eMulMatrix(SolverMatrix&,const SolverMatrix&,const SolverMatrix&);
float Sub_0011B6E0(Memory&,float);
void CalculateWheelMatrices(Memory&,std::uint32_t,std::int32_t);
// ELF001A4580/45A8; E-0038-T13. Original memory-pointer signatures.
float Sub_001A4580(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,ContactProvider&);
float Sub_001A45A8(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,ContactProvider&);
// ELF001A4170; E-0038-T12. f12/f13/f14, A0/A1/A2/A3 actual ABI.
void Sub_001A4170(Memory&,float,float,float,std::uint32_t,std::uint32_t,
                 std::uint32_t,std::uint16_t,ContactProvider&);
}

namespace port::driving::native {
void ApplyImpulse(Memory& m,std::uint32_t body,const std::array<float,4>& input,const std::array<float,4>& point);
}

namespace port::driving {
SolverMatrix Sub_0011B7D8(Memory&m,std::uint16_t angle);
}

namespace port::driving {
SolverMatrix Sub_0011B950(Memory&m,std::uint16_t angle);
}

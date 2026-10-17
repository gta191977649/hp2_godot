#pragma once
#include "Sub_001A4E88.hpp"
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
void Sub_0025AF20(SolverMatrix&, const SolverMatrix&);
// ELF 0025AED0 and thunk 0019D368; E-0038-S02. Uses explicit mutable global.
void Sub_0025AED0(SolverVector&, const SolverMatrix&, const SolverVector&, const SolverMathContext&);
void Sub_0019D368(SolverVector&, const SolverMatrix&, const SolverVector&, const SolverMathContext&);
// ELF 0019CD90/0019CE80/0019CEF8; E-0038-S03/S04/S05.
void Sub_0019CD90(SolverBodyProjection&, const SolverVector&, const SolverVector&);
void Sub_0019CE80(SolverBodyProjection&, const SolverVector&, const SolverVector&, const SolverMathContext&);
void Sub_0019CEF8(SolverBodyProjection&, const SolverVector&);
// ELF 001A5428; E-0038-S06. Writes XYZ only, preserving destination W.
void Sub_001A5428(const SolverWheelProjection&, SolverVector&);
struct SolverWheelGlobals {
    float Field_00330CC0 = 0, Field_00330CC4 = 0, Field_00330CC8 = 0, Field_00330CCC = 0;
};
// ELF 001A4778; E-0038-S11. Dynamic globals are explicit inputs.
void Sub_001A4778(float, SolverWheelProjection&, const SolverWheelGlobals&);
// ELF 001A44A8/001A6330/001A6370; E-0038-S07/S08/S09.
float Sub_001A44A8(float, SolverWheelProjection&, std::int32_t);
void Sub_001A6330(float, SolverSuspensionProjection&);
void Sub_001A6370(SolverSuspensionProjection&, std::int32_t Field_00330D10);

// Port-side dependency interface, not a recovered original class. Untranslated
// whole dependencies throw at their ELF address; fixtures may override them.
struct SolverDependencies {
    virtual ~SolverDependencies() = default;
    virtual void Sub_0018B948();
    virtual void Sub_001A4930(SolverWheelProjection&, int, std::int8_t);
    virtual void Sub_0018BAA0();
    virtual void Sub_001A4EE0(SolverWheelProjection&, int);
    virtual void Sub_0018BC10();
};
// ELF 00138F38..00139183; E-0038-S10. Exact five-iteration orchestration
// with explicit dependency seams. This does not implement the driving closure.
void Sub_00138F38(SolverBodyProjection&, std::array<SolverWheelProjection, 4>&,
                 const std::array<SolverSuspensionProjection, 4>&, std::int8_t,
                 SolverDependencies&, const SolverMathContext&);
}

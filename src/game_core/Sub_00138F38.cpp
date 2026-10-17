#include "Sub_00138F38.hpp"
#include "Sub_0018BCE0.hpp"
#include "port/not_implemented.hpp"
#include <cmath>

namespace port::driving {
namespace {
float xyz_length(const SolverVector& v) {
    const float x = v[0] * v[0], y = v[1] * v[1], z = v[2] * v[2];
    const float xy = x + y;
    return std::sqrt(xy + z);
}
// VOPMSUB writes XYZ only. The caller supplies the surviving destination W.
SolverVector cross_xyz(const SolverVector& a, const SolverVector& b, float w) {
    const float x0 = a[1] * b[2], x1 = a[2] * b[1];
    const float y0 = a[2] * b[0], y1 = a[0] * b[2];
    const float z0 = a[0] * b[1], z1 = a[1] * b[0];
    return {{x0 - x1, y0 - y1, z0 - z1, w}};
}
}
// ELF 0025AF20..0025AFA7; E-0038-S01.
void Sub_0025AF20(SolverMatrix& out, const SolverMatrix& in) {
    const SolverMatrix copy = in;
    for (unsigned r = 0; r < 4; ++r)
        for (unsigned c = 0; c < 4; ++c) out[r * 4 + c] = copy[c * 4 + r];
}
// ELF 0025AED0..0025AF1B; E-0038-S02. VADD.XYZ retains global W.
void Sub_0025AED0(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context) {
    SolverVector v = context.Field_00320A60;
    for (unsigned i = 0; i < 3; ++i) v[i] = v[i] + in[i];
    SolverVector result{};
    for (unsigned i = 0; i < 4; ++i) {
        float acc = matrix[i] * v[0];
        const float y = matrix[4 + i] * v[1];
        acc = acc + y;
        const float z = matrix[8 + i] * v[2];
        acc = acc + z;
        const float w = matrix[12 + i] * v[3];
        result[i] = acc + w;
    }
    out = result;
}
// ELF 0019D368..0019D36F; E-0038-S02.
void Sub_0019D368(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context) {
    Sub_0025AED0(out, matrix, in, context);
}
// ELF 0019CD90..0019CE7B; E-0038-S03.
void Sub_0019CD90(SolverBodyProjection& body, const SolverVector& a1, const SolverVector& a2) {
    // First A1 load precedes the +0xf0 stores; cross inputs load afterwards.
    const SolverVector original = a1;
    SolverVector limited = original;
    const float length = xyz_length(original);
    if (1200.0f < length) {
        const float factor = 1200.0f / length;
        for (float& lane : limited) lane = lane * factor;
    }
    for (unsigned i = 0; i < 4; ++i) body.Field_0xf0[i] = body.Field_0xf0[i] + limited[i];
    // Original A1, rather than capped force, feeds the cross product.
    const SolverVector position = a2;
    SolverVector value = cross_xyz(position, a1, position[3]);
    const float cross_length = xyz_length(value);
    if (100.0f < cross_length) {
        const float factor = 100.0f / cross_length;
        for (float& lane : value) lane = lane * factor;
    }
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = body.Field_0x100[i] + value[i];
}
// ELF 0019CE80..0019CEF3; E-0038-S04.
void Sub_0019CE80(SolverBodyProjection& body, const SolverVector& a1,
                 const SolverVector& a2, const SolverMathContext& context) {
    SolverVector first{}, second{};
    Sub_0019D368(first, body.Field_0x20, a1, context);
    Sub_0019D368(second, body.Field_0x20, a2, context);
    Sub_0019CD90(body, first, second);
}
// ELF 0019CEF8..0019CF0F; E-0038-S05.
void Sub_0019CEF8(SolverBodyProjection& body, const SolverVector& a1) {
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = body.Field_0x100[i] + a1[i];
}
// ELF 001A5428..001A5483; E-0038-S06.
void Sub_001A5428(const SolverWheelProjection& wheel, SolverVector& out) {
    const float f7 = wheel.Field_0xc8 * wheel.Field_0x118;
    const float f4 = wheel.Field_0xb0 * wheel.Field_0x114;
    const float f3 = wheel.Field_0xc0 * wheel.Field_0x118;
    const float f2 = wheel.Field_0xb4 * wheel.Field_0x114;
    const float f1 = wheel.Field_0xc4 * wheel.Field_0x118;
    const float f0 = wheel.Field_0xb8 * wheel.Field_0x114;
    out[0] = f4 + f3; out[1] = f2 + f1; out[2] = f0 + f7;
}
// ELF 001A4778..001A4817; E-0038-S11.
void Sub_001A4778(float f12, SolverWheelProjection& wheel, const SolverWheelGlobals& globals) {
    float f1 = globals.Field_00330CC0 * wheel.Field_0x170;
    float f2 = globals.Field_00330CC4 * wheel.Field_0x174;
    f2 = f2 * wheel.config_0x14_Field_0x8;
    f1 = f1 * wheel.config_0x14_Field_0x4;
    f1 = f1 + f2;
    f2 = f1 * globals.Field_00330CC8;
    float f0 = std::abs(wheel.Field_0x20);
    f12 = f12 * wheel.config_Field_0xc;
    f0 = f0 * globals.Field_00330CCC;
    f12 = f12 + f0;
    if (f12 < f2) {
        wheel.Field_0x10c = 1.0f < f2 ? 1u : 0u;
        wheel.Field_0x20 = 0.0f;
    } else {
        wheel.Field_0x10c = 0;
    }
}
// ELF 001A44A8..001A457B; E-0038-S07. Constant 0032E644 = 0x3e99999a.
float Sub_001A44A8(float f12, SolverWheelProjection& wheel, std::int32_t a1) {
    wheel.Field_0x155 = static_cast<std::uint8_t>(a1);
    if (a1 != 0) {
        const float factor = -wheel.config_Field_0xc;
        for (unsigned i = 0; i < 4; ++i) {
            const float product = wheel.Field_0x140[i] * factor;
            wheel.Field_0x130[i] = product + wheel.Field_0xa0[i];
        }
    }
    if (0.0f < f12) {
        const auto first = cross_xyz(wheel.Field_0x80, wheel.Field_0x140, wheel.Field_0x80[3]);
        const auto second = cross_xyz(first, wheel.Field_0x80, first[3]);
        const float x = second[0] * wheel.Field_0x140[0];
        const float y = second[1] * wheel.Field_0x140[1];
        const float z = second[2] * wheel.Field_0x140[2];
        const float xy = x + y;
        float f0 = (xy + z) * 4.0f;
        f0 = f0 - 3.0f;
        if (f0 < 0.300000011920928955078125f) f0 = 0.300000011920928955078125f;
        wheel.Field_0x158 = f12 * f0;
    } else {
        wheel.Field_0x158 = 0.0f;
    }
    return 0.0f;
}
// ELF 001A6330..001A636F; E-0038-S08. A1 is inherited from +0x70,
// correcting the decompiler's spurious third argument in 001A44A8.
void Sub_001A6330(float f12, SolverSuspensionProjection& suspension) {
    float f0 = suspension.Field_0x1c;
    if (suspension.Field_0x70 != 0) f0 = f0 + f12;
    if (!suspension.Field_0x90) hp2::not_implemented("0x001A6330", "missing +0x90 wheel projection");
    suspension.Field_0xa0 = Sub_001A44A8(f0, *suspension.Field_0x90, suspension.Field_0x70);
}
// ELF 001A6370..001A63BF; E-0038-S09. GP-0x4BE0 = 00330D10.
void Sub_001A6370(SolverSuspensionProjection& suspension, std::int32_t Field_00330D10) {
    if (suspension.Field_0x70 == 0) return;
    if (!suspension.Field_0x90 || !suspension.Field_0x94)
        hp2::not_implemented("0x001A6370", "missing wheel/body projections");
    SolverVector delta{};
    for (unsigned i = 0; i < 4; ++i)
        delta[i] = suspension.Field_0x90->Field_0x130[i] - suspension.Field_0x94->Field_0x10[i];
    if (Field_00330D10 == 0)
        Sub_0019CD90(*suspension.Field_0x94, suspension.Field_0x90->Field_0x120, delta);
}
void SolverDependencies::Sub_0018B948() { hp2::not_implemented("0x0018B948", "solver dependency"); }
void SolverDependencies::Sub_001A4930(SolverWheelProjection&, int, std::int8_t) {
    hp2::not_implemented("0x001A4930", "wheel solver initialization");
}
void SolverDependencies::Sub_0018BAA0() { hp2::not_implemented("0x0018BAA0", "solver iteration dependency"); }
void SolverDependencies::Sub_001A4EE0(SolverWheelProjection&, int) {
    hp2::not_implemented("0x001A4EE0", "wheel solver finalization");
}
// ELF 0018BC10..0018BC17; E-0038-D07. Confirmed original empty body.
void SolverDependencies::Sub_0018BC10() { port::driving::Sub_0018BC10(); }
// ELF 00138F38..00139183; E-0038-S10.
void Sub_00138F38(SolverBodyProjection& body, std::array<SolverWheelProjection, 4>& wheels,
                 const std::array<SolverSuspensionProjection, 4>& suspensions, std::int8_t a2,
                 SolverDependencies& dependencies, const SolverMathContext& context) {
    dependencies.Sub_0018B948();
    for (auto& wheel : wheels) dependencies.Sub_001A4930(wheel, 0, a2);
    SolverMatrix matrix{};
    Sub_0025AF20(matrix, body.Field_0x20);
    SolverVector base{};
    Sub_0019D368(base, matrix, body.Field_0xf0, context);
    for (unsigned iteration = 0; iteration < 5; ++iteration) {
        SolverVector sum = base;
        float f20 = 0.0f;
        for (unsigned i = 0; i < 4; ++i) {
            SolverVector value{}; // W is not consumed by the recovered scalar solver.
            Sub_001A5428(wheels[i], value);
            for (unsigned lane = 0; lane < 4; ++lane) sum[lane] = sum[lane] + value[lane];
            const float f2 = suspensions[i].Field_0x80 * value[1];
            const float f1 = suspensions[i].Field_0x84 * value[0];
            f20 = f20 + f2;
            f20 = f20 - f1;
        }
        const float x = sum[0] / body.Field_0x118;
        const float y = sum[1] / body.Field_0x118;
        const float divisor = 1.0f / body.Field_0x148;
        f20 = f20 / divisor;
        for (unsigned i = 0; i < 4; ++i) {
            if (wheels[i].Field_0x154 == 0 && wheels[i].Field_0x10c == 0) {
                const float f1 = f20 * suspensions[i].Field_0x80;
                const float f2 = f20 * suspensions[i].Field_0x84;
                const float vy = y + f1, vx = x - f2;
                ModifyGroundForce(wheels[i], {{vx, vy}});
            }
        }
        dependencies.Sub_0018BAA0();
    }
    for (int i = 0; i < 4; ++i) dependencies.Sub_001A4EE0(wheels[static_cast<unsigned>(i)], i);
    dependencies.Sub_0018BC10();
}
}

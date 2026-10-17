// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
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
void Sub_0018ABF0(EngineTimerProjection&,std::int32_t,float);
std::int32_t Sub_0018BCA0(DrivetrainProjection&,std::int32_t);
std::int32_t SetDesiredGear(DrivetrainProjection&,std::int32_t,const DrivetrainGlobals&);
float Sub_0018B360(const DrivetrainProjection&,const DrivetrainGlobals&);
void Sub_0018B6C8(float,DrivetrainProjection&,const DrivetrainGlobals&);
// The unresolved 0018AAB0 evaluation must be supplied explicitly. Empty hook throws.
using EngineEvaluation = std::function<float()>;
void Sub_0018B948(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation& = {});
void Sub_0018BAA0(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation& = {});
void Sub_0018BC10();
}

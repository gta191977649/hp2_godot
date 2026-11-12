// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include <optional>
namespace hp2::debug {
// Debug00218850, E-0099-W06. Original OBB default initializes WorldObj only;
// all geometry remains as supplied in logical storage.
class OBB {
public:
    OBB(::port::driving::Memory&,std::uint32_t);
};
// TASK-0099 E-0099-W02/W37, logical state only; no PS2 host ABI claim.
// Debug00218AD8, E-0099-W37. Explicit original static initializer.
void Sub_00218AD8(::port::driving::Memory&);
class DriveTarget {
public:
    // Port memory-view binding only; original construction is explicit below.
    DriveTarget(::port::driving::Memory& m,std::uint32_t a):memory_(m),address_(a){}
    void Sub_00216598(int); //Debug00216598,E-0099-W119.
    void Sub_00216528(); // Debug00216528, E-0099-W37 (original E-0099-W02).
    void Clear(std::uint32_t car,float distance_to_clear_past);
    void ShiftLeft(int positions,bool recalculate_length); //Debug00218538, W37.
    int InterpolatePoint(std::uint32_t car,float distance,std::uint32_t output); //Debug00216A18, W37.
    // Debug002165B8,002169D8,00217070, E-0099-W37.
    void Sub_002165B8(std::uint32_t source);
    void Sub_001E3EB8(std::uint32_t source); //Debug copy constructor; inline emitted carcollisiondetect.cpp, W37.
    int GetLastIndexToCheckForDrivability()const;
    int GetFirstPoint(std::uint32_t state);
    void RecalculateLength(); //Debug002186D8, E-0099-W37.
    void Resample(std::uint32_t car); //Debug002171D8, E-0099-W47.
    void SetPoint(int index,std::uint32_t position,std::int8_t route,std::int16_t point,std::int8_t line,float speed); //Debug00218660
    void SetPoint(int,const std::array<float,2>&,std::int8_t,std::int16_t,std::int8_t,float,std::optional<std::uint32_t> local_w={}); //00218618,PU02 original bVector2 wrapper.
    void BlendWithCurrentAcceleration(std::uint32_t car,std::uint32_t port_stack_vector_lane); //Debug00218168
    void FillWithCurrentAcceleration(std::uint32_t car,std::uint32_t port_stack_vector_lane); //Debug00217F40
    void DoSnapshot(std::uint32_t snapshot); //Debug00216660, E-0099-W74.
    // Debug00217530,00217B48,00217BF8,00217D40,00217EC0,00217ED0.
    float EstimateScore(std::uint32_t car);
    bool CalculateMaxSpeed(std::uint32_t car,const std::array<float,2>& point,float time,float& speed);
    bool CalculateMaxSpeed(std::uint32_t car,const std::array<float,2>& p1,const std::array<float,2>& p2,const std::array<float,2>& p3,float distance,float initial,float& final);
    bool CalculateMaxSpeedCore(std::uint32_t car,const std::array<float,2>& initial,const std::array<float,2>& desired,float distance,float speed,float& final);
    float GetMaxLateralAcceleration(std::uint32_t car)const;
    float CalculateTime(float distance,float initial,float final)const;
    // Debug00216EC8, E-0099-W37. Complete original16-byte point copy.
    void AddPointToTarget(std::uint32_t position,std::uint32_t driving_location,bool reject_double_back,std::uint32_t car=0); //00216CE0,E-0110-F12.
    void AddPointToTarget(std::uint32_t position,std::int8_t route,std::int16_t point,std::int8_t line,bool reject_double_back,std::uint32_t car=0);
    void AddPointToTarget(const std::array<float,2>&,std::int8_t,std::int16_t,std::int8_t,bool,std::uint32_t car=0,std::optional<std::uint32_t> local_w={}); //00216DD8,PU02 original bVector2 wrapper.
    // Debug00218738, E-0099-W37. Original metadata lookup, not nearest route.
    bool GetRouteAndRoutePoint(int index,std::uint32_t& route,std::uint32_t& point)const;
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class DriveTarget {
public:
    // ELF 0x0021BDC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021bdc8.md.
    // Original Debug reference: DriveTarget::GetLastIndexToCheckForDrivability(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetLastIndexToCheckForDrivability();
    // ELF 0x0021DAC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021dac8.md.
    // Original Debug reference: DriveTarget::RecalculateLength(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RecalculateLength();
};
} // namespace hp2::recovered

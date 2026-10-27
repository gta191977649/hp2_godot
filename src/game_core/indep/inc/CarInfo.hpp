#pragma once
// TASK-0098 E-0098-S01. Debug NFSHP2.ELF SHA815b60c7...332f1b.
// Recovered ../indep/inc/CarInfo.hpp names. Decoded host fields, NOT PS2 ABI.
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace hp2::debug {
enum CarUsageType { CAR_USAGE_TYPE_RACING=0, CAR_USAGE_TYPE_COP=1, CAR_USAGE_TYPE_TRAFFIC=2 };
struct CarLanguageInfo {
    int TheCarType=0,LanguageSpec0_60=0,LanguageSpecTopSpeed=0,LanguageSpecBHP=0,
        LanguageSpecFullName=0,LanguageSpecShortName=0;
};
const CarLanguageInfo& FindCarLanguageInfo(int car_type);
struct WheelInfo {
    std::array<float,3> Position{};
    float Radius=0,Width=0;
    int WheelNumber=0;
    float YRenderOffset=0;
};
struct ControlInfo {float SpeedLimiter=0;}; // Decoded accessed field only, not ABI.
struct CarTypeInfo {
    std::array<char,32> CarTypeName{},DashGeometryFilename{};
    std::int8_t HighPerformance=0,NumAvailableSkinNumbers=0,CarClass=0;
    int Type=0;
    CarUsageType UsageType=CAR_USAGE_TYPE_RACING;
    std::array<std::int8_t,10> AvailableSkinNumbers{};
    int DefaultSkinNumber=0;
    std::array<WheelInfo,4> Wheels{};
    ControlInfo Control;
    const CarLanguageInfo* pLanguageInfo=nullptr;
    int GetNumAvailableSkins() const;
    // Language is an explicit host binding of the original global language.
    int GetAvailableSkin(int index,int language) const;
    std::string GetFullName(const std::function<std::string(int)>& GetTranslatedString) const;
    std::string GetCarSpec0To60(const std::function<std::string(int)>&) const;
    std::string GetCarSpecBHP(const std::function<std::string(int)>&) const;
};
}
namespace port::selection {
// Decodes fields reviewed against Debug LoaderCarInfo00186148/type CarTypeInfo.
// Binds language identities as InitCarInfo00186EB8; excludes original relocation,
// acceleration-table creation and the remainder of the original loader lifecycle.
std::array<hp2::debug::CarTypeInfo,64> decode_car_info(const std::vector<std::uint8_t>&);
}

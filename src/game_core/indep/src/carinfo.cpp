#include "game_core/port/driving/projection_support.hpp"
#include "game_core/indep/inc/Table.hpp"
namespace port::driving {
void CalcIndexMultiplier(Memory& m,std::uint32_t s) {hp2::recovered::Table(m,s).CalcIndexMultiplier();}
void SetMinMax(Memory& m,std::uint32_t s,float lower,float upper) {hp2::recovered::Table(m,s).SetMinMax(lower,upper);}
}
#include "game_core/indep/src/carinfo.hpp"
#include "game_core/indep/inc/CarInfo.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <stdexcept>

#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
// Debug00186050, static00304DB8..00305267, E-0098-S03. IDs only, no asset text.
const CarLanguageInfo& FindCarLanguageInfo(int car_type){
    static constexpr std::array<CarLanguageInfo,50> CarLanguageInfoTable{{
        {-1,42002,43002,44002,41100,41300},
        {0,42002,43002,44002,41100,41300},
        {2,42003,43003,44003,41101,41301},
        {4,42004,43004,44004,41102,41302},
        {5,42006,43006,44006,41103,41303},
        {7,42007,43007,44007,41104,41304},
        {9,42008,43008,44008,41105,41305},
        {11,42009,43009,44009,41106,41306},
        {13,42010,43010,44010,41107,41307},
        {15,42011,43011,44011,41108,41308},
        {17,42012,43012,44012,41109,41309},
        {19,42013,43013,44013,41110,41310},
        {20,42014,43014,44014,41111,41311},
        {21,42015,43015,44015,41112,41312},
        {23,42016,43016,44016,41113,41313},
        {27,42017,43017,44017,41114,41314},
        {29,42018,43018,44018,41115,41315},
        {31,42019,43019,44019,41116,41316},
        {32,42020,43020,44020,41117,41317},
        {33,42021,43021,44021,41118,41318},
        {34,42022,43022,44022,41119,41319},
        {35,42023,43023,44023,41120,41320},
        {37,42024,43024,44024,41121,41321},
        {39,42025,43025,44025,41122,41322},
        {41,42026,43026,44026,41123,41323},
        {43,42027,43027,44027,41124,41324},
        {45,42028,43028,44028,41125,41325},
        {47,42029,43029,44029,41126,41326},
        {25,42041,43041,44041,41127,41327},
        {1,42002,43002,44002,41200,41400},
        {3,42003,43003,44003,41201,41401},
        {6,42006,43006,44006,41202,41402},
        {8,42007,43007,44007,41203,41403},
        {10,42008,43008,44008,41204,41404},
        {12,42009,43009,44009,41205,41405},
        {14,42010,43010,44010,41206,41406},
        {16,42011,43011,44011,41207,41407},
        {18,42012,43012,44012,41208,41408},
        {24,42016,43016,44016,41212,41412},
        {28,42017,43017,44017,41213,41413},
        {30,42018,43018,44018,41214,41414},
        {36,42023,43023,44023,41215,41415},
        {38,42024,43024,44024,41216,41416},
        {40,42025,43025,44025,41217,41417},
        {42,42026,43026,44026,41218,41418},
        {44,42027,43027,44027,41219,41419},
        {46,42028,43028,44028,41220,41420},
        {48,42029,43029,44029,41221,41421},
        {26,42041,43041,44041,41222,41422},
        {22,42042,43042,44042,41211,41411},
    }};
    for(const auto& info:CarLanguageInfoTable)if(info.TheCarType==car_type)return info;
    return CarLanguageInfoTable.front();
}
// Debug00186088, E-0098-S03. Translation resolves original numeric ID explicitly.
std::string CarTypeInfo::GetFullName(const std::function<std::string(int)>& GetTranslatedString) const{
    if(!pLanguageInfo || !GetTranslatedString)
        hp2::not_implemented("0x00186088","original car language/translation binding missing");
    return GetTranslatedString(pLanguageInfo->LanguageSpecFullName);
}
// Debug001860E8/00186128, E-0098-O02. Original numeric localization IDs.
std::string CarTypeInfo::GetCarSpec0To60(const std::function<std::string(int)>& translate)const{
    if(!pLanguageInfo || !translate)hp2::not_implemented("0x001860E8","original acceleration localization binding missing");
    return translate(pLanguageInfo->LanguageSpec0_60);
}
std::string CarTypeInfo::GetCarSpecBHP(const std::function<std::string(int)>& translate)const{
    if(!pLanguageInfo || !translate)hp2::not_implemented("0x00186128","original BHP localization binding missing");
    return translate(pLanguageInfo->LanguageSpecBHP);
}
// Debug00186668, E-0098-S01. Keep signed normal-car count and police special case.
int CarTypeInfo::GetNumAvailableSkins() const {
    return UsageType==CAR_USAGE_TYPE_COP?1:NumAvailableSkinNumbers;
}
// Debug00186688, E-0098-S01. Explicit language binds original global, not a locale guess.
int CarTypeInfo::GetAvailableSkin(int index,int language) const {
    if(UsageType==CAR_USAGE_TYPE_COP)return language==0?1:2;
    if(!NumAvailableSkinNumbers)
        hp2::not_implemented("0x00186688","zero original skin divisor");
    const int slot=index%NumAvailableSkinNumbers;
    if(slot<0 || slot>=static_cast<int>(AvailableSkinNumbers.size()))
        hp2::not_implemented("0x00186688","original skin lookup outside decoded storage");
    return AvailableSkinNumbers[slot];
}
}
namespace port::selection {
// E-0098-S01: host decoding only, reviewed Debug type offsets + LoaderCarInfo00186148.
std::array<hp2::debug::CarTypeInfo,64> decode_car_info(const std::vector<std::uint8_t>& bytes){
    const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* table=hp2::assets::find_first(chunks,0x34600);
    if(!table)throw std::runtime_error("Original car config chunk0x34600 missing");
    const auto base=(table->header_offset+0x17u)&~std::size_t(15);
    const auto end=table->payload_offset()+table->size;
    if(base>end || end>bytes.size() || 64u*0x560u>end-base)
        throw std::runtime_error("Original selection car table truncated");
    std::array<hp2::debug::CarTypeInfo,64> result{};
    for(std::size_t i=0;i<result.size();++i){
        const auto* row=bytes.data()+base+i*0x560;
        auto& car=result[i];
        const auto word=[&](int offset){return std::uint32_t(row[offset])|(std::uint32_t(row[offset+1])<<8)
            |(std::uint32_t(row[offset+2])<<16)|(std::uint32_t(row[offset+3])<<24);};
        std::copy_n(row,32,car.CarTypeName.begin());
          std::copy_n(row+0x40,32,car.GeometryFilename.begin());
        std::copy_n(row+0x80,32,car.DashGeometryFilename.begin());
        car.HighPerformance=static_cast<std::int8_t>(row[0xe4]);
        car.NumAvailableSkinNumbers=static_cast<std::int8_t>(row[0xe5]);
        car.CarClass=static_cast<std::int8_t>(row[0xe6]);
        // E-0098-R01: Debug WheelInfo type, car.Wheels+120/stride20hex.
        const auto scalar=[&](int offset){const auto bits=word(offset);float value;std::memcpy(&value,&bits,4);return value;};
        car.Control.SpeedLimiter=scalar(0x320); // E-0098-O02, Debug Control+20.
          for(int n=0;n<5;++n)car.LocationProb[n]=scalar(0x520+n*4); // E-0098-RP02.
        for(int n=0;n<4;++n){auto& wheel=car.Wheels[n];const int offset=0x120+n*0x20;
            for(int axis=0;axis<3;++axis)wheel.Position[axis]=scalar(offset+axis*4);
            wheel.Radius=scalar(offset+0xc);wheel.Width=scalar(offset+0x10);
            wheel.WheelNumber=static_cast<std::int32_t>(word(offset+0x14));wheel.YRenderOffset=scalar(offset+0x18);}
        car.Type=static_cast<std::int32_t>(word(0x534));
        car.UsageType=static_cast<hp2::debug::CarUsageType>(word(0x538));
        for(int n=0;n<10;++n)car.AvailableSkinNumbers[n]=static_cast<std::int8_t>(row[0x544+n]);
        car.DefaultSkinNumber=static_cast<std::int32_t>(word(0x554));
        // Original InitCarInfo00186EB8 binds by loop identity, independent of row.Type.
        car.pLanguageInfo=&hp2::debug::FindCarLanguageInfo(static_cast<int>(i));
    }
    return result;
}
}

namespace hp2::recovered {
// ELF 0x001884B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001884b8.md.
void CarTypeInfo::GetNumAvailableSkins() {
    hp2::not_implemented("0x001884B8", "CarTypeInfo::GetNumAvailableSkins (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Table.hpp"

namespace hp2::recovered {
// ELF 0x00189120; E-0042-C057; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00189120.md.
void Table::SetMinMax(float lower,float upper) {
    m_.scalar(address_+8,lower);m_.scalar(address_+0xc,upper);CalcIndexMultiplier();
}
// Debug00187318, full listing E-0044-E01.
void Table::CalcIndexMultiplier() {
    const auto count=port::driving::detail_group_0018f808::signed_bits(m_.word(address_+4)-1);
    m_.scalar(address_+0x10,static_cast<float>(count)/(m_.scalar(address_+0xc)-m_.scalar(address_+8)));
}
}

#include "game_core/indep/inc/Fred.hpp"
#include "game_core/src/bMath.hpp"
#include <cmath>
namespace hp2::debug {
// Debug00185FA0, E-0098-RP02. Original byte narrowing is preserved.
void RideInfo::Init(int type,int skin,int dash,int damage){
    Type=type;SkinNumber=static_cast<std::uint8_t>(skin);HasDash=static_cast<std::uint8_t>(dash);
    CanBeVertexDamaged=static_cast<std::uint8_t>(damage);InstanceIndex=0;
}
// Debug001867C0, E-0098-RP02. Padding+5..7 is intentionally untouched.
DriverInfo::DriverInfo(::port::driving::Memory& m,std::uint32_t a,int driver_number,int driver_type,
    const RideInfo& ride,int character,int skill,int start,const std::function<std::uint32_t(int)>& get_character){
    if(!get_character)hp2::not_implemented("0x001867C0","original GetCharacterInfo binding missing");
    m.byte(a+1,static_cast<std::uint8_t>(driver_number));m.byte(a+4,static_cast<std::uint8_t>(start));
    m.word(a+8,static_cast<std::uint32_t>(driver_type));m.byte(a,0);
    m.word(a+12,static_cast<std::uint32_t>(ride.Type));m.byte(a+16,ride.InstanceIndex);
    m.byte(a+17,ride.SkinNumber);m.byte(a+18,ride.HasDash);m.byte(a+19,ride.CanBeVertexDamaged);
    m.byte(a+2,static_cast<std::uint8_t>(character));m.word(a+20,get_character(character));
    m.byte(a+3,static_cast<std::uint8_t>(skill));
}
// Debug00186610, E-0098-RP02. Even a single non-cop skin consumes RNG.
int CarTypeInfo::GetRandomSkin(::port::driving::Memory& memory,int language) const{
    if(UsageType==CAR_USAGE_TYPE_COP)return GetAvailableSkin(0,language);
    if(NumAvailableSkinNumbers==0)return 0;
    if(NumAvailableSkinNumbers<0 || NumAvailableSkinNumbers>10)
        hp2::not_implemented("0x00186610","original skin list outside decoded host domain");
    return AvailableSkinNumbers[hp2::recovered::bRandom(memory,int(NumAvailableSkinNumbers))];
}
// Debug001863C8, E-0098-RP02. Original two-pass iterator, shared RNG and threshold.
// Location index/language/catalog bind original globals; no locally seeded random engine.
bool GenerateRandomRideInfo(::port::driving::Memory& memory,RideInfo& ride,CarUsageType usage,int car_class,
    const std::array<CarTypeInfo,64>& cars,int location,int language){
    if(location<0 || location>=5)hp2::not_implemented("0x001863C8","original LocationProb index outside decoded host domain");
    const auto eligible=[&](const CarTypeInfo& car){return car.UsageType==usage && car.GeometryFilename[0]
        && (car_class==-1 || car.CarClass==car_class) && car.GetNumAvailableSkins()>0;};
    float total=0;for(const auto& car:cars)if(eligible(car))total=total+car.LocationProb[location];
    if(!(memory.scalar(0x325400)<std::abs(total)))return false;
    float weight=hp2::recovered::bRandom(memory,total);
    for(const auto& car:cars)if(eligible(car)){
        weight=weight-car.LocationProb[location];
        if(!(0.f<weight)){ride.Init(car.Type,car.GetRandomSkin(memory,language),0,0);return true;}
    }
    return false;
}
}

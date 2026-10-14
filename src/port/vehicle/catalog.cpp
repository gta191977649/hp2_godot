#include "catalog.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace port::vehicle {
void Catalog::load() {
    const auto bytes=source_.read("GLOBAL/GLOBALB.BUN");
    const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* cars=hp2::assets::find_first(chunks,0x34600);
    const auto* skins=hp2::assets::find_first(chunks,0x34601);
    if(!cars || !skins)throw std::runtime_error("Original vehicle catalogs are missing");
    const auto base=(cars->header_offset+0x17u)&~std::size_t(15);
    if(base>cars->payload_offset()+cars->size || 64u*0x560u>cars->payload_offset()+cars->size-base)
        throw std::runtime_error("Original 64-car catalog is truncated");
    if(skins->size%64)throw std::runtime_error("Original skin catalog is truncated");
    const auto u32=[&](std::size_t at) {
        return std::uint32_t(bytes.at(at)) | std::uint32_t(bytes.at(at+1))<<8 |
            std::uint32_t(bytes.at(at+2))<<16 | std::uint32_t(bytes.at(at+3))<<24;
    };
    const auto text=[&](std::size_t at,std::size_t maximum) {
        std::size_t length=0;
        while(length<maximum && bytes.at(at+length))++length;
        if(length==maximum)throw std::runtime_error("Original vehicle catalog text is not terminated");
        return std::string(reinterpret_cast<const char*>(bytes.data()+at),length);
    };
    std::vector<SkinChoice> parsed_skins;
    for(std::size_t i=0;i<skins->size/64;++i) {
        const auto at=skins->payload_offset()+i*64;
        parsed_skins.push_back({u32(at),u32(at+4),text(at+8,32),u32(at+0x28),u32(at+0x2c),u32(at+0x30)});
    }
    std::vector<CarChoice> parsed_cars;
    // Cache probes for original shared resources. Availability does not mean decoded/verified.
    std::map<std::string,std::pair<bool,std::string>> probes;
    for(std::uint32_t i=0;i<64;++i) {
        const auto at=base+i*0x560;
        CarChoice car;
        car.id=i;car.code=text(at,32);car.prefix=text(at+0x20,32);
        car.original_resource_path=text(at+0x60,64);
        car.resource_path=car.original_resource_path;
        std::replace(car.resource_path.begin(),car.resource_path.end(),'\\','/');
        // The original points to LZC; this viewer intentionally reads the matching
        // player-owned BIN beside it. No decompression or arbitrary file fallback.
        const std::string suffix="/GEOMETRY.LZC";
        if(car.resource_path.rfind("CARS/",0)!=0 || car.resource_path.size()<suffix.size() ||
           car.resource_path.compare(car.resource_path.size()-suffix.size(),suffix.size(),suffix)!=0 ||
           car.resource_path.find("..")!=std::string::npos) {
            car.resource_path.clear();car.availability_error="Original geometry path is outside the supported BIN viewer input";
        } else {
            car.resource_path.replace(car.resource_path.size()-3,3,"BIN");
            auto found=probes.find(car.resource_path);
            if(found==probes.end()) {
                std::pair<bool,std::string> probe;
                try {
                    probe.first=source_.probe(car.resource_path);
                    if(!probe.first)probe.second="Original BIN resource is missing or empty";
                }catch(const std::exception& error){probe.second=error.what();}
                found=probes.emplace(car.resource_path,std::move(probe)).first;
            }
            car.available=found->second.first;car.availability_error=found->second.second;
        }
        for(const auto& skin:parsed_skins)if(skin.car_id==i)car.skins.push_back(skin);
        parsed_cars.push_back(std::move(car));
    }
    // Retain every original skin row, even if its car ID is not in the car table.
    skins_=std::move(parsed_skins);cars_=std::move(parsed_cars);
}
const CarChoice& Catalog::car(std::uint32_t id)const {
    if(id>=cars_.size())throw std::out_of_range("Vehicle catalog ID is absent");
    return cars_[id];
}
const SkinChoice& Catalog::skin(std::uint32_t car_id,std::uint32_t skin_id)const {
    const auto& choices=car(car_id).skins;
    const auto found=std::find_if(choices.begin(),choices.end(),[&](const auto& choice){return choice.id==skin_id;});
    if(found==choices.end())throw std::out_of_range("Vehicle catalog skin ID is absent");
    return *found;
}
}

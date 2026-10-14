#pragma once
#include "port/assets/catalog.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace port::vehicle {
// Port-side catalog values; original identifiers are retained as asset text.
struct SkinChoice {
    std::uint32_t car_id{}, id{};
    std::string name;
    std::uint32_t Field_0x28{}, material_hash{}, Field_0x30{};
};
struct CarChoice {
    std::uint32_t id{};
    std::string code, prefix;
    std::string original_resource_path; // Original catalog text, including LZC extension.
    std::string resource_path; // Explicit BIN-only native viewer adapter.
    bool available{};
    std::string availability_error;
    std::vector<SkinChoice> skins;
};
class Catalog {
    assets::Source& source_;
    std::vector<CarChoice> cars_;
    std::vector<SkinChoice> skins_;
public:
    explicit Catalog(assets::Source& source):source_(source){}
    // E-0033-B15: 00187F98 binds 64 aligned car rows and unaligned skin rows.
    // 001B0560 reads car+0x60 resource text; no HP suffix heuristic is used.
    void load();
    const std::vector<CarChoice>& cars()const{return cars_;}
    const std::vector<SkinChoice>& skins()const{return skins_;}
    const CarChoice& car(std::uint32_t id)const;
    const SkinChoice& skin(std::uint32_t car_id,std::uint32_t skin_id)const;
};
}

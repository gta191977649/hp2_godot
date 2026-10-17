// PORT-SIDE FILE: adapts original catalog data to native text access.
// TASK-0022, E-0022-C01..C03. SLUS_203.62 SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#pragma once
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace port::assets {
class EventNameCatalog {
    std::vector<std::array<std::uint32_t, 2>> tracks_;
    std::vector<std::uint8_t> track_logos_;
    std::vector<std::string> car_codes_;
public:
    // 0x00153608 loads chunk 0x34201 as 0xA4-byte rows; 0x00153420 uses
    // one-based indexes and rejects an empty first byte. The loader clears track 54.
    void load(const std::vector<std::uint8_t>& global_bundle) {
        const auto chunks = hp2::assets::parse_chunks_top(global_bundle);
        const auto* table = hp2::assets::find_first(chunks, 0x34201);
        if (!table || table->size % 0xa4 != 0)
            hp2::not_implemented("0x00153608", "track catalog chunk 0x34201 missing or malformed");
        tracks_.assign(table->size / 0xa4, {0, 0});
        track_logos_.assign(tracks_.size(), 0);
        const auto read_u32 = [&](std::size_t offset) {
            return std::uint32_t(global_bundle.at(offset)) |
                (std::uint32_t(global_bundle.at(offset + 1)) << 8) |
                (std::uint32_t(global_bundle.at(offset + 2)) << 16) |
                (std::uint32_t(global_bundle.at(offset + 3)) << 24);
        };
        for (std::size_t index = 0; index < tracks_.size(); ++index) {
            const auto row = table->payload_offset() + index * 0xa4;
            if (global_bundle.at(row) == 0 || index == 53) continue;
            tracks_[index] = {read_u32(row + 0x48), read_u32(row + 0x4c)};
            track_logos_[index] = global_bundle.at(row + 0x42); // 001F91F0
        }
        // 00187F98 binds aligned chunk34600 as 64 rows of stride560. 001F8E18
        // appends the first NUL-terminated row string to hash(CARLOGO_).
        const auto* cars = hp2::assets::find_first(chunks, 0x34600);
        if (!cars) hp2::not_implemented("0x00187F98", "car catalog chunk34600 missing");
        const std::size_t base = (cars->header_offset + 0x17u) & ~std::size_t(15);
        if (base + 64u * 0x560u > cars->payload_offset() + cars->size)
            throw std::runtime_error("car catalog stride560 table truncated");
        car_codes_.assign(64, "");
        for (std::size_t index = 0; index < 64; ++index) {
            const auto row = base + index * 0x560;
            std::size_t length = 0;
            while (length < 0x20 && global_bundle.at(row + length) != 0) ++length;
            if (length == 0x20) throw std::runtime_error("car catalog code is not NUL terminated");
            car_codes_[index].assign(reinterpret_cast<const char*>(global_bundle.data() + row), length);
        }
    }
    // Callers select the original row+0x48 or row+0x4C localized name ID.
    std::uint32_t track_name_id(int one_based_id, bool alternate = false) const {
        if (one_based_id <= 0 || static_cast<std::size_t>(one_based_id) > tracks_.size() ||
            tracks_[static_cast<std::size_t>(one_based_id - 1)][0] == 0)
            hp2::not_implemented("0x00153420", "requested track catalog row is absent");
        return tracks_[static_cast<std::size_t>(one_based_id - 1)][alternate ? 1 : 0];
    }
    // 001F8E18: original code string at car row base, prefix CARLOGO_ (002BA868).
    std::uint32_t car_logo_hash(int car_id) const {
        if (car_id < 0 || static_cast<std::size_t>(car_id) >= car_codes_.size())
            hp2::not_implemented("0x001F8E18", "car logo index outside catalog");
        return hp2::frontend::bStringHash((std::string("CARLOGO_") + car_codes_[car_id]).c_str());
    }
    // 001F91F0 /0019FB00: LOGOTRACK%2d with R direction and _FR locale1 suffix.
    std::uint32_t track_logo_hash(int one_based_id, bool alternate, int locale) const {
        (void)track_name_id(one_based_id, alternate); // original row existence contract
        const int number = track_logos_.at(static_cast<std::size_t>(one_based_id - 1));
        std::string texture = "LOGOTRACK";
        if (number < 10) texture += ' '; // %2d uses space rather than zero padding
        texture += std::to_string(number);
        if (alternate) texture += 'R';
        if (locale == 1) texture += "_FR";
        return hp2::frontend::bStringHash(texture.c_str());
    }
    // 0x00188D08 -> 0x00187E80 binds car+0x558 to the first matching
    // row at 0x0030D578. 0x00187EB8 localizes row+0x14. These are IDs,
    // not copied proprietary text. Unmatched IDs use the first row verbatim.
    static std::uint32_t car_name_id(int car_id) {
        static constexpr std::array<std::pair<int, std::uint32_t>, 50> table{{
            {-1, 0xa08cu},
            {0, 0xa08cu},
            {2, 0xa08du},
            {4, 0xa08eu},
            {5, 0xa08fu},
            {7, 0xa090u},
            {9, 0xa091u},
            {11, 0xa092u},
            {13, 0xa093u},
            {15, 0xa094u},
            {17, 0xa095u},
            {19, 0xa096u},
            {20, 0xa097u},
            {21, 0xa098u},
            {23, 0xa099u},
            {27, 0xa09au},
            {29, 0xa09bu},
            {31, 0xa09cu},
            {32, 0xa09du},
            {33, 0xa09eu},
            {34, 0xa09fu},
            {35, 0xa0a0u},
            {37, 0xa0a1u},
            {39, 0xa0a2u},
            {41, 0xa0a3u},
            {43, 0xa0a4u},
            {45, 0xa0a5u},
            {47, 0xa0a6u},
            {25, 0xa0a7u},
            {1, 0xa0f0u},
            {3, 0xa0f1u},
            {6, 0xa0f2u},
            {8, 0xa0f3u},
            {10, 0xa0f4u},
            {12, 0xa0f5u},
            {14, 0xa0f6u},
            {16, 0xa0f7u},
            {18, 0xa0f8u},
            {24, 0xa0fcu},
            {28, 0xa0fdu},
            {30, 0xa0feu},
            {36, 0xa0ffu},
            {38, 0xa100u},
            {40, 0xa101u},
            {42, 0xa102u},
            {44, 0xa103u},
            {46, 0xa104u},
            {48, 0xa105u},
            {26, 0xa106u},
            {22, 0xa0fbu},
        }};
        for (const auto& [id, text] : table) if (id == car_id) return text;
        return table.front().second;
    }
};
} // namespace port::assets


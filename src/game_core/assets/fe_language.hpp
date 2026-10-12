#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace hp2::port::assets {
// Port-side reader of the player's FE_*.BIN chunk 0x39000. E-0016-S01.
// Tables contain eight-byte (key, string-blob offset) records. Serialized pointer
// words are discarded; actual tables follow the 0x64-byte payload header.
class FeLanguage {
public:
    explicit FeLanguage(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {
        if (bytes_.size() < 108 || u32(0) != 0x39000 || u32(4) + 8 != bytes_.size())
            throw std::runtime_error("invalid FE language chunk");
        count_ = u32(0x58);
        table_ = 108 + static_cast<std::size_t>(u32(0x54)) * 8;
        blob_ = table_ + static_cast<std::size_t>(count_) * 8;
        length_ = u32(0x5c);
        if (blob_ > bytes_.size() || length_ > bytes_.size() - blob_)
            throw std::runtime_error("truncated FE language tables");
        for (std::size_t i = 0; i < count_; ++i) {
            if (i && u32(table_ + (i-1)*8) >= u32(table_ + i*8))
                throw std::runtime_error("unsorted FE language id table");
            (void)string_at(u32(table_ + i*8 + 4));
        }
    }
    // ELF FUN_0021E2E8 searches the second table for localised numeric IDs.
    bool lookup(std::uint32_t id, std::string& result) const {
        std::size_t lo = 0, hi = count_;
        while (lo < hi) {
            const auto mid = lo + (hi-lo)/2;
            const auto key = u32(table_ + mid*8);
            if (key < id) lo = mid+1; else hi = mid;
        }
        if (lo == count_ || u32(table_ + lo*8) != id) return false;
        result = string_at(u32(table_ + lo*8 + 4)); return true;
    }
private:
    std::uint32_t u32(std::size_t p) const {
        if (p > bytes_.size() || bytes_.size()-p < 4) throw std::runtime_error("truncated FE word");
        return std::uint32_t(bytes_[p]) | (std::uint32_t(bytes_[p+1])<<8) |
            (std::uint32_t(bytes_[p+2])<<16) | (std::uint32_t(bytes_[p+3])<<24);
    }
    std::string string_at(std::size_t offset) const {
        if (offset >= length_) throw std::runtime_error("invalid FE string offset");
        auto end = offset;
        while (end < length_ && bytes_[blob_+end]) ++end;
        if (end == length_) throw std::runtime_error("unterminated FE string");
        return std::string(reinterpret_cast<const char*>(bytes_.data()+blob_+offset), end-offset);
    }
    std::vector<std::uint8_t> bytes_;
    std::size_t table_=0, blob_=0, length_=0, count_=0;
};
}

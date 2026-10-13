// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

// TASK-0012: native, Godot-independent port of tools/asset_extractor/elab_chunks.py.
// Generic reader for the EA "eLab" nested chunk container format used across HP2's PS2 ZZDATA
// bundles. See tools/asset_extractor/elab_chunks.py and docs/ASSET_PIPELINE.md for the format
// evidence (byte-exact full-file consumption validated independently in this repo).
namespace hp2::assets {

struct Chunk {
    std::uint32_t tag = 0;
    std::uint32_t size = 0;          // payload byte length
    std::size_t header_offset = 0;    // file offset of this chunk's 8-byte header
    std::vector<Chunk> children;      // populated only when is_container()

    bool is_container() const { return (tag & 0x80000000u) != 0; }
    std::size_t payload_offset() const { return header_offset + 8; }
};

// Parses a flat sequence of sibling chunks in data[start:end), recursing into containers.
// Returns the chunks and sets *consumed to how far parsing reached (== end on full success).
std::vector<Chunk> parse_chunks(const std::uint8_t* data, std::size_t size,
                                 std::size_t start, std::size_t end, std::size_t* consumed = nullptr);

inline std::vector<Chunk> parse_chunks_top(const std::vector<std::uint8_t>& buffer, std::size_t* consumed = nullptr) {
    return parse_chunks(buffer.data(), buffer.size(), 0, buffer.size(), consumed);
}

// Depth-first search for the first chunk (at any depth) with the given tag, starting the search
// within `chunks` (does not look at siblings outside the given vector).
const Chunk* find_first(const std::vector<Chunk>& chunks, std::uint32_t tag);

} // namespace hp2::assets

// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "elab_chunks.hpp"

namespace hp2::assets {

std::vector<Chunk> parse_chunks(const std::uint8_t* data, std::size_t size,
                                 std::size_t start, std::size_t end, std::size_t* consumed) {
    std::vector<Chunk> chunks;
    std::size_t pos = start;
    while (pos + 8 <= end && pos + 8 <= size) {
        std::uint32_t tag = static_cast<std::uint32_t>(data[pos]) |
                             (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
                             (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
                             (static_cast<std::uint32_t>(data[pos + 3]) << 24);
        std::uint32_t chunk_size = static_cast<std::uint32_t>(data[pos + 4]) |
                                    (static_cast<std::uint32_t>(data[pos + 5]) << 8) |
                                    (static_cast<std::uint32_t>(data[pos + 6]) << 16) |
                                    (static_cast<std::uint32_t>(data[pos + 7]) << 24);
        std::size_t payload_start = pos + 8;
        std::size_t payload_end = payload_start + chunk_size;
        if (payload_end > end || payload_end > size) break; // hypothesis breaks down; stop here
        Chunk chunk;
        chunk.tag = tag;
        chunk.size = chunk_size;
        chunk.header_offset = pos;
        if (chunk.is_container()) {
            chunk.children = parse_chunks(data, size, payload_start, payload_end, nullptr);
        }
        chunks.push_back(std::move(chunk));
        pos = payload_end;
    }
    if (consumed) *consumed = pos;
    return chunks;
}

const Chunk* find_first(const std::vector<Chunk>& chunks, std::uint32_t tag) {
    for (const auto& chunk : chunks) {
        if (chunk.tag == tag) return &chunk;
        if (chunk.is_container()) {
            if (const Chunk* found = find_first(chunk.children, tag)) return found;
        }
    }
    return nullptr;
}

} // namespace hp2::assets

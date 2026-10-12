#include "fmenus.hpp"
#include "game_core/assets/elab_chunks.hpp"
#include <cstring>

namespace hp2::frontend {
namespace {

std::uint32_t read_u32(const std::uint8_t* d, std::size_t off) {
    return static_cast<std::uint32_t>(d[off]) | (static_cast<std::uint32_t>(d[off + 1]) << 8) |
           (static_cast<std::uint32_t>(d[off + 2]) << 16) | (static_cast<std::uint32_t>(d[off + 3]) << 24);
}

float read_f32(const std::uint8_t* d, std::size_t off) {
    std::uint32_t bits = read_u32(d, off);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string read_fixed_string(const std::uint8_t* d, std::size_t off, std::size_t max_len) {
    std::size_t len = 0;
    while (len < max_len && d[off + len] != 0) ++len;
    return std::string(reinterpret_cast<const char*>(d + off), len);
}

} // namespace

std::vector<FeScreen> parse_screens(const std::vector<std::uint8_t>& buffer) {
    std::vector<FeScreen> screens;
    auto chunks = hp2::assets::parse_chunks_top(buffer);
    const hp2::assets::Chunk* list = hp2::assets::find_first(chunks, kScreenListChunkId);
    if (!list) return screens;

    int index = 0;
    for (const auto& screen_chunk : list->children) {
        if (screen_chunk.tag != kScreenChunkId || !screen_chunk.is_container()) continue;
        FeScreen screen;
        screen.index = index++;
        for (const auto& child : screen_chunk.children) {
            const std::uint8_t* d = buffer.data() + child.payload_offset();
            if (child.tag == kScreenHeaderId && child.size >= 0x1c) {
                screen.width = static_cast<int>(d[0x18] | (d[0x19] << 8));
                screen.height = static_cast<int>(d[0x1a] | (d[0x1b] << 8));
                continue;
            }
            const bool is_quad = child.tag == kQuadWidgetId;
            const bool is_text = child.tag == kTextWidgetId;
            if (!is_quad && !is_text) continue;
            if (child.size < 0x5c) continue;

            FeWidget w;
            w.chunk_id = child.tag;
            w.name_hash = read_u32(d, 0x08);
            w.class_hash = read_u32(d, 0x0c);
            w.name = read_fixed_string(d, 0x10, 0x18);
            w.type = d[0x28];
            w.visible = d[0x29];
            w.layer = d[0x2a];
            w.x = read_f32(d, 0x30);
            w.y = read_f32(d, 0x34);
            w.scale_x = read_f32(d, 0x38);
            w.scale_y = read_f32(d, 0x3c);
            w.texture_hash = read_u32(d, 0x50);

            // Colour bytes are stored R,G,B,A with PS2 alpha (0x80 == opaque). Quads carry a
            // four-corner block starting at 0x58; text widgets carry a single colour at 0x5c.
            const std::size_t colour_off = is_quad ? 0x58 : 0x5c;
            if (child.size >= colour_off + 4) {
                w.r = d[colour_off + 0];
                w.g = d[colour_off + 1];
                w.b = d[colour_off + 2];
                w.a = d[colour_off + 3];
            }
            if (is_text) {
                w.text_capacity = read_u32(d, 0x54);
                w.text_align = read_u32(d, 0x58);
                if (child.size > 0x64) w.text = read_fixed_string(d, 0x64, child.size - 0x64);
            }
            screen.widgets.push_back(std::move(w));
        }
        screens.push_back(std::move(screen));
    }
    return screens;
}

namespace {
void collect_chunks(const std::vector<hp2::assets::Chunk>& chunks, std::uint32_t tag,
                     std::vector<const hp2::assets::Chunk*>& out) {
    for (const auto& chunk : chunks) {
        if (chunk.tag == tag) out.push_back(&chunk);
        if (chunk.is_container()) collect_chunks(chunk.children, tag, out);
    }
}
} // namespace

std::vector<FeFont> parse_fonts(const std::vector<std::uint8_t>& buffer) {
    std::vector<FeFont> fonts;
    auto chunks = hp2::assets::parse_chunks_top(buffer);
    std::vector<const hp2::assets::Chunk*> font_chunks;
    collect_chunks(chunks, kFontChunkId, font_chunks);

    for (const auto* chunk : font_chunks) {
        if (chunk->size < 0x14) continue;
        const std::uint8_t* d = buffer.data() + chunk->payload_offset();
        FeFont font;
        font.texture_hash = read_u32(d, 0x00);
        const std::uint32_t glyph_count = read_u32(d, 0x04);
        font.line_height = read_u32(d, 0x0c);
        const std::size_t table_bytes = static_cast<std::size_t>(glyph_count) * 32;
        if (glyph_count == 0 || 0x14 + table_bytes > chunk->size) continue;
        font.glyphs.reserve(glyph_count);
        for (std::uint32_t i = 0; i < glyph_count; ++i) {
            const std::size_t off = 0x14 + static_cast<std::size_t>(i) * 32;
            FeGlyph g;
            g.code = static_cast<std::uint16_t>(d[off] | (d[off + 1] << 8));
            g.flags = static_cast<std::uint16_t>(d[off + 2] | (d[off + 3] << 8));
            g.u = read_f32(d, off + 0x04);
            g.v = read_f32(d, off + 0x08);
            g.w = read_f32(d, off + 0x0c);
            g.h = read_f32(d, off + 0x10);
            g.advance = read_f32(d, off + 0x14);
            font.glyphs.push_back(g);
        }
        fonts.push_back(std::move(font));
    }
    return fonts;
}

} // namespace hp2::frontend

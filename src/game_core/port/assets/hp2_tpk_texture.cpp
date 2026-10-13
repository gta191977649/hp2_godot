// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "hp2_tpk_texture.hpp"
#include "elab_chunks.hpp"
#include <algorithm>
#include <array>
#include <cstring>

// Faithful port of tools/asset_extractor/hp2_tpk_texture.py; see hp2_tpk_texture.hpp for
// provenance. Keep this file's control flow identical to that Python source (and to
// D:\dev\eagl-godot\map_tools_ps2\src\map_tools_ps2\textures.py, which it was itself ported
// from); port fixes from there rather than editing the math here from first principles.

namespace hp2::assets {
namespace {

// Python's `%`/`//` always produce a non-negative remainder / floor toward -inf for a positive
// divisor; this matters for `_rotate_bits`'s explicit negative-shift call. C++'s `%` truncates
// toward zero (can be negative), so this helper is required for a faithful port.
int py_mod(int a, int b) {
    int r = a % b;
    if (r < 0) r += b;
    return r;
}

std::uint32_t adjust_with_mask(std::uint32_t src, int mask_width, int mask_position = 0, int adjustment = 0) {
    return ((src >> mask_position) & ((1u << mask_width) - 1u)) << adjustment;
}

std::uint32_t rotate_bits(std::uint32_t value, int shift, int width) {
    shift = py_mod(shift, width);
    std::uint32_t mask = (1u << width) - 1u;
    value &= mask;
    return ((value << shift) | (value >> (width - shift))) & mask;
}

int bit_length(std::uint32_t v) {
    int n = 0;
    while (v) { ++n; v >>= 1; }
    return n;
}

std::uint32_t align_power_of_two_max(std::uint32_t value) {
    if (value <= 1) return 1;
    return 1u << bit_length(value - 1);
}

std::uint32_t legacy_ps2_swizzle_psmt4(std::uint32_t pixel_index, bool mode_flag) {
    std::uint32_t ax = (pixel_index >> 0) & 1, ay = (pixel_index >> 1) & 1;
    std::uint32_t bx = (pixel_index >> 2) & 1, by = (pixel_index >> 3) & 1;
    std::uint32_t cx = (pixel_index >> 4) & 1, cy = (pixel_index >> 5) & 1;
    std::uint32_t dx = (pixel_index >> 6) & 1;
    std::uint32_t result = 0;
    result ^= dx << 0;
    result ^= by << 1;
    result ^= cx << 2;
    result ^= ax << 3;
    result ^= ay << 4;
    result ^= bx << 5;
    result ^= ax << 7;
    result ^= cy << (3 + 3 * (mode_flag ? 1 : 0));
    result >>= mode_flag ? 0 : 1;
    result ^= dx << 5;
    return result & 0x7F;
}

std::uint32_t legacy_ps2_swizzle_psmt8(std::uint32_t pixel_index, bool mode_flag) {
    std::uint32_t ax = (pixel_index >> 0) & 1, ay = (pixel_index >> 1) & 1;
    std::uint32_t bx = (pixel_index >> 2) & 1, by = (pixel_index >> 3) & 1;
    std::uint32_t cx = (pixel_index >> 4) & 1, cy = (pixel_index >> 5) & 1;
    std::uint32_t result = 0;
    result ^= ax << 0;
    result ^= cy << 1;
    result = rotate_bits(result, 5 + (mode_flag ? 1 : 0), 7);
    result ^= bx << (0 + 4 * (mode_flag ? 1 : 0));
    result ^= cx << (2 + 3 * (mode_flag ? 1 : 0));
    result ^= by << 1;
    result ^= ax << 2;
    result ^= ay << 3;
    result ^= cy << 4;
    return result & 0x3F;
}

std::uint32_t legacy_ps2_swizzle_psmt16(std::uint32_t pixel_index, bool mode_flag) {
    std::uint32_t ax = (pixel_index >> 0) & 1, ay = (pixel_index >> 1) & 1;
    std::uint32_t bx = (pixel_index >> 2) & 1, by = (pixel_index >> 3) & 1;
    std::uint32_t cx = (pixel_index >> 4) & 1;
    std::uint32_t result = 0;
    result ^= ay << 0;
    result ^= bx << 1;
    result ^= by << 2;
    result ^= ax << 3;
    result = rotate_bits(result, 2 * (mode_flag ? 1 : 0), 4);
    result ^= cx << 4;
    return result & 0x1F;
}

std::uint32_t legacy_ps2_swizzle_psmt32(std::uint32_t pixel_index, bool mode_flag) {
    std::uint32_t ax = (pixel_index >> 0) & 1, ay = (pixel_index >> 1) & 1;
    std::uint32_t bx = (pixel_index >> 2) & 1, by = (pixel_index >> 3) & 1;
    std::uint32_t result = 0;
    result ^= ay << 0;
    result ^= bx << 1;
    result ^= by << 2;
    result = rotate_bits(result, 2 + (mode_flag ? 1 : 0), 3);
    result = (result << 1) ^ ax;
    return result & 0x0F;
}

using SwizzleFn = std::uint32_t (*)(std::uint32_t, bool);

SwizzleFn legacy_swizzle_function(int bit_depth) {
    switch (bit_depth) {
        case 4: return legacy_ps2_swizzle_psmt4;
        case 8: return legacy_ps2_swizzle_psmt8;
        case 16: return legacy_ps2_swizzle_psmt16;
        case 24: case 32: return legacy_ps2_swizzle_psmt32;
        default: return nullptr;
    }
}

std::uint32_t legacy_ps2_block_address(std::uint32_t block_index, bool swap_xy, bool flip_xy, bool shifted) {
    int swap = swap_xy ? 1 : 0;
    swap = (swap << 0) ^ (swap << 1);
    block_index = rotate_bits(block_index, swap, 5);
    std::uint32_t ax = (block_index >> 0) & 1, ay = (block_index >> 3) & 1;
    std::uint32_t bx = (block_index >> 1) & 1, by = (block_index >> 4) & 1;
    std::uint32_t cx = (block_index >> 2) & 1;
    std::uint32_t result = 0;
    result ^= bx << 0;
    result ^= by << 1;
    result ^= cx << 2;
    result = rotate_bits(result, shifted ? 1 : 0, 3);
    result ^= (0x03u * (flip_xy ? 1u : 0u)) << 1;
    result = (result << 2) ^ (ax << 0) ^ (ay << 1);
    return result & 0x1F;
}

struct RwPlan {
    std::vector<int> targets, sources, source_shifts, target_shifts;
    std::size_t output_length = 0;
};

RwPlan legacy_ps2_rw_plan(bool read_mode, std::uint32_t pixel_storage_mode, int bit_depth, int width, int height) {
    int scale = 32 / bit_depth;
    int scale_mask = scale - 1;
    int scale_x = static_cast<int>(adjust_with_mask(scale_mask, 1, 2, 1) | adjust_with_mask(scale_mask, 1, 0, 0)) + 1;
    int scale_y = static_cast<int>(adjust_with_mask(scale_mask, 1, 3, 1) | adjust_with_mask(scale_mask, 1, 1, 0)) + 1;

    int physical_width = width / scale_y;
    int physical_height = height / scale_x;
    std::uint32_t physical_buffer_width = align_power_of_two_max(static_cast<std::uint32_t>(physical_width));
    std::uint32_t physical_buffer_height = align_power_of_two_max(static_cast<std::uint32_t>(physical_height));
    std::uint32_t buffer_width = align_power_of_two_max(static_cast<std::uint32_t>(width));
    std::uint32_t buffer_height = align_power_of_two_max(static_cast<std::uint32_t>(height));
    std::size_t output_length = static_cast<std::size_t>(physical_buffer_width) * physical_buffer_height;

    std::uint32_t type_index = adjust_with_mask(pixel_storage_mode, 3, 0);
    std::uint32_t type_mode = adjust_with_mask(pixel_storage_mode, 2, 4);
    bool type_flag = adjust_with_mask(pixel_storage_mode, 1, 3) != 0;

    bool swap_xy = (((type_mode == 0) || (type_mode == 3)) && type_index == 2) || (type_mode == 1 && type_index == 4);
    bool z_buffer = type_mode == 3; (void)z_buffer;
    bool shifted = ((type_mode == 0) || (type_mode == 3)) && type_index == 2 && type_flag;

    int column_width = 8 * scale_x;
    int column_height = 2 * scale_y;
    int page_height = 1 << (swap_xy ? 1 : 0);
    int page_width = (page_height ^ 0x03) << 2;
    page_height <<= 2;
    int texture_buffer_width = width / (page_width * column_width);
    if (texture_buffer_width <= 0) texture_buffer_width = 1;

    SwizzleFn swizzle_function = legacy_swizzle_function(bit_depth);

    RwPlan plan;
    plan.output_length = output_length;
    std::size_t total = static_cast<std::size_t>(buffer_width) * buffer_height;
    plan.targets.reserve(total);
    plan.sources.reserve(total);
    plan.source_shifts.reserve(total);
    plan.target_shifts.reserve(total);

    int input_address = 0;
    int from_offset_w = 0;

    for (std::size_t index = 0; index < total; ++index) {
        int y = static_cast<int>(index / buffer_width);
        int x = static_cast<int>(index - static_cast<std::size_t>(y) * buffer_width);

        int page_x = x / (page_width * column_width);
        int page_y = y / (page_height * 4 * column_height);
        int page = page_x + page_y * texture_buffer_width;

        int px = x - page_x * (page_width * column_width);
        int py = y - page_y * (page_height * 4 * column_height);
        int block_x = px / column_width;
        int block_y = py / (4 * column_height);
        std::uint32_t block = legacy_ps2_block_address(static_cast<std::uint32_t>(block_x + block_y * page_width),
                                                          swap_xy, z_buffer, shifted);

        int bx = px - block_x * column_width;
        int by = py - block_y * (4 * column_height);
        int column_y = by / column_height;
        int column = column_y;

        int cx = bx;
        int cy = by - column_y * column_height;
        std::uint32_t pixel = swizzle_function(static_cast<std::uint32_t>(cx + cy * column_width), true);

        int word = static_cast<int>(pixel) / scale;
        int offset = static_cast<int>(pixel) & scale_mask;
        if (bit_depth < 16) word ^= (column & 0x01) << 2;
        word = static_cast<int>((rotate_bits(static_cast<std::uint32_t>(word >> 1), -1, 3) << 1) |
                                 (static_cast<std::uint32_t>(word) & 0x01u));

        int output_address = (page << 11) | (static_cast<int>(block) << 6) | (column << 4) | word;

        int address_a, address_b, source_shift, target_shift;
        if (read_mode) {
            address_a = input_address; address_b = output_address;
            source_shift = bit_depth * offset;
            target_shift = bit_depth * from_offset_w;
        } else {
            address_a = output_address; address_b = input_address;
            source_shift = bit_depth * from_offset_w;
            target_shift = bit_depth * offset;
        }

        plan.targets.push_back(address_a);
        plan.sources.push_back(address_b);
        plan.source_shifts.push_back(source_shift);
        plan.target_shifts.push_back(target_shift);

        from_offset_w += 1;
        if (from_offset_w > 0 && (from_offset_w & scale_mask) == 0) input_address += 1;
        from_offset_w &= scale_mask;
    }

    return plan;
}

std::vector<std::uint32_t> legacy_ps2_rw_buffer(const std::vector<std::uint32_t>& image_data, bool read_mode,
                                                 std::uint32_t pixel_storage_mode, int bit_depth, int width, int height) {
    RwPlan plan = legacy_ps2_rw_plan(read_mode, pixel_storage_mode, bit_depth, width, height);
    std::vector<std::uint32_t> data(plan.output_length, 0);
    std::uint32_t mask = (bit_depth >= 32) ? 0xFFFFFFFFu : ((1u << bit_depth) - 1u);
    std::size_t available = image_data.size();
    for (std::size_t i = 0; i < plan.targets.size(); ++i) {
        int address_a = plan.targets[i];
        int address_b = plan.sources[i];
        if (address_a < 0 || static_cast<std::size_t>(address_a) >= plan.output_length) continue;
        std::uint32_t input_value = (address_b >= 0 && static_cast<std::size_t>(address_b) < available)
                                         ? image_data[static_cast<std::size_t>(address_b)] : 0u;
        data[static_cast<std::size_t>(address_a)] |=
            ((input_value >> plan.source_shifts[i]) & mask) << plan.target_shifts[i];
    }
    return data;
}

int psm_type_index(std::uint8_t pixel_storage_mode) { return pixel_storage_mode & 0x07; }

int indexed_bit_depth(std::uint8_t bit_depth, std::uint8_t pixel_storage_mode, std::size_t palette_size) {
    int psm_index = psm_type_index(pixel_storage_mode);
    if (psm_index > 0) return 32 >> std::max(psm_index - 1, 0);
    if (bit_depth == 4 || bit_depth == 8) return bit_depth;
    if (palette_size == 0x40) return 4;
    if (palette_size == 0x80 || palette_size == 0x400) return 8;
    return 0; // unsupported
}

std::uint8_t decode_ps2_alpha(std::uint8_t value) {
    int expanded = std::max((value << 1) - ((value ^ 1) & 0x01), 0);
    if (expanded <= 0xFF) return static_cast<std::uint8_t>(expanded);
    return value;
}

int unswizzle_palette_index(int index) {
    int block = index & ~0x1F;
    int pos = index & 0x1F;
    if (pos >= 8 && pos < 16) pos += 8;
    else if (pos >= 16 && pos < 24) pos -= 8;
    return block + pos;
}

std::vector<std::array<std::uint8_t, 4>> decode_palette(const std::uint8_t* palette, std::size_t palette_size, bool swizzle) {
    std::vector<std::array<std::uint8_t, 4>> colors;
    std::size_t count = palette_size / 4;
    colors.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        int source_index = swizzle ? unswizzle_palette_index(static_cast<int>(index)) : static_cast<int>(index);
        std::size_t off = static_cast<std::size_t>(source_index) * 4;
        colors.push_back({palette[off], palette[off + 1], palette[off + 2], decode_ps2_alpha(palette[off + 3])});
    }
    return colors;
}

std::vector<std::uint32_t> u32_words(const std::uint8_t* data, std::size_t size) {
    std::size_t padded_size = size + ((4 - (size % 4)) % 4);
    std::vector<std::uint32_t> words(padded_size / 4, 0);
    for (std::size_t i = 0; i < size; ++i) {
        words[i / 4] |= static_cast<std::uint32_t>(data[i]) << ((i % 4) * 8);
    }
    return words;
}

std::optional<std::vector<std::uint8_t>> decode_modelulator_indexed_texture(
    int width, int height, const std::uint8_t* image, std::size_t image_size,
    const std::uint8_t* palette, std::size_t palette_size, std::uint8_t bit_depth,
    std::uint8_t shift_width, std::uint8_t shift_height, std::uint8_t pixel_storage_mode, bool is_swizzled) {

    int depth = indexed_bit_depth(bit_depth, pixel_storage_mode, palette_size);
    if (depth != 4 && depth != 8) return std::nullopt;

    std::size_t color_count = palette_size / 4;
    if (color_count == 0) return std::nullopt;
    auto colors = decode_palette(palette, palette_size, psm_type_index(pixel_storage_mode) == 3);

    int buffer_width = 1 << shift_width;
    int buffer_height = 1 << shift_height;
    if (width <= 0 || height <= 0 || buffer_width <= 0 || buffer_height <= 0) return std::nullopt;

    std::vector<std::uint32_t> words = u32_words(image, image_size);
    if (is_swizzled) {
        int scale = 32 / depth;
        int scale_mask = scale - 1;
        int scale_x = static_cast<int>(adjust_with_mask(scale_mask, 1, 2, 1) | adjust_with_mask(scale_mask, 1, 0, 0)) + 1;
        int scale_y = static_cast<int>(adjust_with_mask(scale_mask, 1, 3, 1) | adjust_with_mask(scale_mask, 1, 1, 0)) + 1;
        words = legacy_ps2_rw_buffer(words, false, 0, 32, width / scale_y, height / scale_x);
        words = legacy_ps2_rw_buffer(words, true, pixel_storage_mode, depth, width, height);
    }

    std::vector<int> indices;
    std::uint32_t index_mask = (1u << depth) - 1u;
    indices.reserve(words.size() * (32 / depth));
    for (std::uint32_t word : words) {
        for (int shift = 0; shift < 32; shift += depth) {
            indices.push_back(static_cast<int>((word >> shift) & index_mask));
        }
    }

    if (is_swizzled) {
        std::vector<int> cropped;
        cropped.reserve(static_cast<std::size_t>(width) * height);
        for (int y = 0; y < height; ++y) {
            std::size_t row = static_cast<std::size_t>(y) * buffer_width;
            if (row >= indices.size()) break;
            std::size_t take = std::min<std::size_t>(width, indices.size() - row);
            cropped.insert(cropped.end(), indices.begin() + row, indices.begin() + row + take);
        }
        indices = std::move(cropped);
    }

    std::vector<int> final_indices;
    final_indices.reserve(static_cast<std::size_t>(width) * height);
    for (int y = height - 1; y >= 0; --y) {
        std::size_t row = static_cast<std::size_t>(y) * width;
        if (row >= indices.size()) continue;
        std::size_t take = std::min<std::size_t>(width, indices.size() - row);
        final_indices.insert(final_indices.end(), indices.begin() + row, indices.begin() + row + take);
    }

    std::vector<std::uint8_t> out;
    std::size_t pixel_count = static_cast<std::size_t>(width) * height;
    out.reserve(pixel_count * 4);
    for (std::size_t i = 0; i < pixel_count && i < final_indices.size(); ++i) {
        const auto& color = colors[static_cast<std::size_t>(final_indices[i]) % color_count];
        out.insert(out.end(), color.begin(), color.end());
    }
    while (out.size() < pixel_count * 4) out.push_back(0); // short buffers: pad rather than crash
    return out;
}

void flip_rgba_vertical(std::vector<std::uint8_t>& rgba, int width, int height) {
    std::size_t row_size = static_cast<std::size_t>(width) * 4;
    std::vector<std::uint8_t> flipped(rgba.size());
    for (int row = 0; row < height; ++row) {
        std::size_t src = static_cast<std::size_t>(height - 1 - row) * row_size;
        std::size_t dst = static_cast<std::size_t>(row) * row_size;
        std::memcpy(flipped.data() + dst, rgba.data() + src, row_size);
    }
    rgba = std::move(flipped);
}

std::string decode_name(const std::uint8_t* data, std::size_t size) {
    std::size_t len = 0;
    while (len < size && data[len] != 0) ++len;
    return std::string(reinterpret_cast<const char*>(data), len);
}

std::uint32_t read_u32(const std::uint8_t* data, std::size_t offset) {
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16) | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

std::uint16_t read_u16(const std::uint8_t* data, std::size_t offset) {
    return static_cast<std::uint16_t>(data[offset] | (data[offset + 1] << 8));
}

std::size_t align_up(std::size_t value, std::size_t boundary) {
    return (value + boundary - 1) & ~(boundary - 1);
}

TpkTextureHeader parse_header(const std::uint8_t* entry) {
    TpkTextureHeader header;
    header.name = decode_name(entry + 0x08, 0x18);
    header.tex_hash = read_u32(entry, 0x20);
    header.width = read_u16(entry, 0x24);
    header.height = read_u16(entry, 0x26);
    header.bit_depth = entry[0x28];
    header.image_placement = static_cast<std::int32_t>(read_u32(entry, 0x30));
    header.palette_placement = static_cast<std::int32_t>(read_u32(entry, 0x34));
    header.image_size = static_cast<std::int32_t>(read_u32(entry, 0x38));
    header.palette_size = static_cast<std::int32_t>(read_u32(entry, 0x3C));
    header.shift_width = entry[0x48];
    header.shift_height = entry[0x49];
    header.pixel_storage_mode = entry[0x4A];
    header.clut_pixel_storage_mode = entry[0x4B];
    header.texture_fx = entry[0x4E];
    header.is_any_semitransparency = entry[0x4F];
    header.is_swizzled = entry[0x55] != 0;
    header.alpha_bits = entry[0x76];
    header.alpha_fix = entry[0x77];
    return header;
}

} // namespace

std::optional<Hp2TpkArchive> Hp2TpkArchive::load(std::vector<std::uint8_t> buffer) {
    auto chunks = hp2::assets::parse_chunks_top(buffer);
    const hp2::assets::Chunk* main_chunk = nullptr;
    for (const auto& chunk : chunks) {
        if (chunk.tag == kMainChunkId && chunk.is_container()) { main_chunk = &chunk; break; }
    }
    if (!main_chunk) return std::nullopt;

    const hp2::assets::Chunk* entry_chunk = nullptr;
    const hp2::assets::Chunk* data_chunk = nullptr;
    for (const auto& child : main_chunk->children) {
        if (child.tag == kEntryTableChunkId) entry_chunk = &child;
        else if (child.tag == kDataPoolChunkId) data_chunk = &child;
    }
    if (!entry_chunk || !data_chunk) return std::nullopt;
    if (entry_chunk->size % kEntrySize != 0) return std::nullopt;

    Hp2TpkArchive archive;
    archive.data_base_ = align_up(data_chunk->payload_offset(), 0x80);
    std::size_t count = entry_chunk->size / kEntrySize;
    archive.headers_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        archive.headers_.push_back(parse_header(buffer.data() + entry_chunk->payload_offset() + i * kEntrySize));
    }
    archive.buffer_ = std::move(buffer);
    return archive;
}

const TpkTextureHeader* Hp2TpkArchive::find(const std::string& name) const {
    for (const auto& header : headers_) {
        if (header.name == name) return &header;
    }
    return nullptr;
}

std::optional<DecodedTexture> Hp2TpkArchive::decode(const TpkTextureHeader& header, bool flip_vertical) const {
    if (header.name.empty() || header.width == 0 || header.height == 0) return std::nullopt;
    if (header.image_placement < 0 || header.palette_placement < 0 || header.image_size < 0 || header.palette_size < 0)
        return std::nullopt;
    std::size_t image_start = data_base_ + static_cast<std::size_t>(header.image_placement);
    std::size_t image_end = image_start + static_cast<std::size_t>(header.image_size);
    std::size_t palette_start = data_base_ + static_cast<std::size_t>(header.palette_placement);
    std::size_t palette_end = palette_start + static_cast<std::size_t>(header.palette_size);
    if (image_end > buffer_.size() || palette_end > buffer_.size()) return std::nullopt;
    if (header.image_size == 0 || header.palette_size == 0) return std::nullopt;

    auto rgba = decode_modelulator_indexed_texture(
        header.width, header.height, buffer_.data() + image_start, static_cast<std::size_t>(header.image_size),
        buffer_.data() + palette_start, static_cast<std::size_t>(header.palette_size), header.bit_depth,
        header.shift_width, header.shift_height, header.pixel_storage_mode, header.is_swizzled);
    if (!rgba) return std::nullopt;

    DecodedTexture texture;
    texture.width = header.width;
    texture.height = header.height;
    texture.rgba = std::move(*rgba);
    if (flip_vertical) flip_rgba_vertical(texture.rgba, texture.width, texture.height);
    return texture;
}

std::optional<DecodedTexture> Hp2TpkArchive::decode_by_name(const std::string& name, bool flip_vertical) const {
    const TpkTextureHeader* header = find(name);
    if (!header) return std::nullopt;
    return decode(*header, flip_vertical);
}

} // namespace hp2::assets

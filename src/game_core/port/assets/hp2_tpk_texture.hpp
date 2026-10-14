// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// TASK-0012: native, Godot-independent runtime loader for HP2's PS2 "eLab"/.tpk texture
// packages (FRONTEND/*.BIN, GLOBAL/*TEX*.BIN, etc: any file whose top-level elab_chunks
// container is tagged kMainChunkId).
//
// This is a faithful C++ port of tools/asset_extractor/hp2_tpk_texture.py's decode path, which
// is itself ported verbatim (logic unchanged) from the user's existing, independently-validated
// project D:\dev\eagl-godot\map_tools_ps2\src\map_tools_ps2\textures.py (see that project's
// docs/hp2_ps2_texture_decoding.md, which in turn credits Modelulator's legacy PS2 texture path
// as the algorithm's origin). Do not re-derive this math from first principles; port changes
// from the Python reference. See docs/ASSET_PIPELINE.md for the evidence and verification
// history, including the 0x80 data-pool alignment bug this port already accounts for.
//
// Unlike the Python tool (which pre-exports PNGs to disk), this loader reads directly from the
// original PS2 asset files at runtime; see src/platform/ps2_asset_root.hpp for the one place
// that resolves the ZZDATA root (mirrors tools/ps2_assets.py's resolution order). No extracted
// image files are shipped or cached to disk by this loader.
namespace hp2::assets {

inline constexpr std::uint32_t kMainChunkId = 0xB0300001u;
inline constexpr std::uint32_t kEntryTableChunkId = 0x30300003u;  // texture entry table, 0xA4 bytes each
inline constexpr std::uint32_t kDataPoolChunkId = 0x30300004u;    // shared pixel+palette data pool
inline constexpr std::size_t kEntrySize = 0xA4;

struct TpkTextureHeader {
    std::string name;
    std::uint32_t tex_hash = 0;
    std::uint16_t width = 0, height = 0;
    std::uint8_t bit_depth = 0;
    std::int32_t image_placement = 0, palette_placement = 0;
    std::int32_t image_size = 0, palette_size = 0;
    std::uint8_t shift_width = 0, shift_height = 0;
    std::uint8_t pixel_storage_mode = 0, clut_pixel_storage_mode = 0;
    std::uint8_t Field_0x4c = 0; // 00108CC0 derives CLAMP.WMS/WMT; E-0033-T06.
    // Raw signed-byte zero/nonzero partition tested by 0010DED0 and 00104DF0.
    // E-0033-T06. Kept separate from the unrelated +0x4F metadata.
    std::uint8_t Field_0x4d = 0;
    std::uint8_t texture_fx = 0, is_any_semitransparency = 0;
    std::uint8_t Field_0x50 = 0; // Queue state-index bit1; 0010BB20/0010B838, E-0033-T07.
    bool is_swizzled = false;
    // Raw byte; 0010DEBC loads it with lb and gates render bit 4 on nonzero.
    // E-0033-T05. Original name and full meaning remain unknown.
    std::uint8_t Field_0x74 = 0;
    // Raw TEX1 inputs emitted by 00108CC0; E-0033-T06.
    std::uint8_t Field_0x6e = 0, Field_0x6f = 0, Field_0x70 = 0, Field_0x71 = 0;
    std::int16_t Field_0x72 = 0;
    std::uint8_t alpha_bits = 0, alpha_fix = 0;
};

// width*height RGBA8888 pixels, row-major, top-to-bottom (after the optional vertical flip).
struct DecodedTexture {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba; // size == width*height*4
};

// Parses the chunk container in `buffer` and locates the first kMainChunkId section's entry
// table and data pool. Returns false if no such section is found (e.g. unrelated ZZDATA file).
class Hp2TpkArchive {
public:
    static std::optional<Hp2TpkArchive> load(std::vector<std::uint8_t> buffer);

    const std::vector<TpkTextureHeader>& headers() const { return headers_; }
    const TpkTextureHeader* find(const std::string& name) const;

    // Decodes one entry. flip_vertical matches the Python tool's frontend_textures.py
    // convention (true) for standalone/UI display; pass false to match the 3D-mesh UV
    // convention used by map_tools_ps2's track texture path.
    // preserve_gs_alpha keeps the original CLUT alpha byte (0..255, unity=128).
    // Default false retains the established frontend RGBA expansion byte-for-byte.
    std::optional<DecodedTexture> decode(const TpkTextureHeader& header, bool flip_vertical = true,
                                         bool preserve_gs_alpha = false) const;
    std::optional<DecodedTexture> decode_by_name(const std::string& name, bool flip_vertical = true,
                                                 bool preserve_gs_alpha = false) const;
    // E-0035-R01: original 00109510 PSMCT32 host->local linear RGBA transfer.
    // Row zero retained; separate from the legacy indexed frontend flip convention.
    std::optional<DecodedTexture> decode_linear32(const TpkTextureHeader& header) const;

private:
    std::vector<std::uint8_t> buffer_;
    std::size_t data_base_ = 0;
    std::vector<TpkTextureHeader> headers_;
};

} // namespace hp2::assets

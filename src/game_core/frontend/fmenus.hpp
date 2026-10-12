#pragma once
#include <cstdint>
#include <string>
#include <vector>

// TASK-0013: FrontEnd screen/widget layout records from FRONTEND/FRONTB.BUN.
//
// Named after the original source file this logic belongs to: the ELF embeds the compile-time
// path "indep\src\fmenus.cpp" (see docs/ORIGINAL_SOURCE_TREE.md), and the widget name hashes in
// these records appear verbatim as immediates in the ELF's frontend code (e.g. 0xe0105663/4/5 =
// POPUP_1/2/3 and 0xab671c9a = ANIMATING_MENUITEM are passed to FUN_00169740/FUN_00248870 inside
// FUN_0016b478), which is what ties this file format to that translation unit.
//
// Record layout was derived from the data and validated by correlation, not guessed: offset 0x50
// holds a texture hash for 507/1015 of all 0x10210 records and 206/534 of all 0x10212 records
// across every screen, matching real TexturePack entry hashes; and TITLE_FULLSCREEN's
// 512x256 texture at scale (1.25, 1.88) yields exactly 640x480, the screen size declared in the
// 0x10203 screen header. See docs/FRONTEND_LAYOUT.md for the full field map and evidence.
namespace hp2::frontend {

inline constexpr std::uint32_t kScreenListChunkId = 0x80010200; // container of all screens
inline constexpr std::uint32_t kScreenChunkId     = 0x80010201; // one screen
inline constexpr std::uint32_t kScreenHeaderId    = 0x00010203; // 40-byte screen header
inline constexpr std::uint32_t kQuadWidgetId      = 0x00010210; // 144-byte textured quad
inline constexpr std::uint32_t kTextWidgetId      = 0x00010212; // variable-size text widget

struct FeWidget {
    std::uint32_t chunk_id = 0;     // kQuadWidgetId or kTextWidgetId
    std::uint32_t name_hash = 0;    // +0x08; appears as a literal immediate in the ELF
    std::uint32_t class_hash = 0;   // +0x0c; shared by widgets of the same class
    std::string name;               // +0x10, 24-byte field
    std::uint8_t type = 0;          // +0x28 (0x01 text, 0x02 quad)
    std::uint8_t visible = 0;       // +0x29
    std::uint8_t layer = 0;         // +0x2a; draw order candidate (see docs open items)
    float x = 0, y = 0;             // +0x30, +0x34 (top-left, 640x480 space)
    float scale_x = 1, scale_y = 1; // +0x38, +0x3c (multiplies the texture's native size)
    std::uint32_t texture_hash = 0; // +0x50 (TexturePack entry hash; 0 = untextured)
    std::uint8_t r = 128, g = 128, b = 128, a = 128; // RGBA bytes; PS2 alpha (0x80 = opaque)
    std::uint32_t text_capacity = 0; // +0x54 (text widgets): authored buffer length
    std::uint32_t text_align = 0;    // +0x58 (text widgets): 1/2/4 observed
    std::string text;                // +0x64 (text widgets): authored default string
};

// Bitmap font table (chunk 0x00015010). Header: u32 texture hash, u32 glyph count, u32 kerning
// pair count, u32 line height, u32 byte offset of the kerning table, then `glyph count` 32-byte
// glyph records. Glyph codes are sparse (e.g. '$' is absent from the Impact fonts), so look up by
// code rather than by index. Kerning pairs are parsed but not yet applied -- see docs open items.
struct FeGlyph {
    std::uint16_t code = 0;
    std::uint16_t flags = 0;
    float u = 0, v = 0, w = 0, h = 0, advance = 0; // atlas pixels
};

struct FeFont {
    std::uint32_t texture_hash = 0; // matches a TexturePack entry (FONT_IMPACTSMALL, BUTTONSOUT, ...)
    std::uint32_t line_height = 0;
    std::vector<FeGlyph> glyphs;
};

struct FeScreen {
    int index = 0;                  // position within the screen list
    int width = 640, height = 480;  // from the 0x10203 header
    std::vector<FeWidget> widgets;
};

// Parses every screen in the first kScreenListChunkId container found in `buffer`.
// Returns an empty vector when the buffer holds no frontend screen list.
std::vector<FeScreen> parse_screens(const std::vector<std::uint8_t>& buffer);

inline constexpr std::uint32_t kFontChunkId = 0x00015010;

// Parses every font table in `buffer` (a bundle may carry several, and fonts are spread across
// FRONTEND/FRONTB.BUN, GLOBAL/GLOBALB.BUN and GLOBAL/INGAMEB.BUN).
std::vector<FeFont> parse_fonts(const std::vector<std::uint8_t>& buffer);

} // namespace hp2::frontend

// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\fmenus.cpp`
// (string at 0x002B27F8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "frontendscreens/options.hpp"
#include "fmenus.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
#include <algorithm>

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
                screen.name_hash_0x08 = read_u32(d, 0x08);
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
            w.clip_hash_0x4c = read_u32(d, 0x4c);
            w.scale_y = read_f32(d, 0x3c);
            if (is_quad && child.size >= 0x90) {
                w.field_0x40 = static_cast<std::int32_t>(read_u32(d, 0x40));
                w.field_0x44 = read_u32(d, 0x44);
                for (std::size_t i = 0; i < 3; ++i) w.field_0x5c[i] = read_u32(d, 0x5c + i*4);
                for (std::size_t i = 0; i < 8; ++i) w.field_0x70[i] = read_f32(d, 0x70 + i*4);
            }
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
                if (child.size >= 0x64) w.string_id_0x60 = read_u32(d, 0x60);
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
        const auto glyph_offset = read_u32(d, 0x0c);
        const auto pair_offset = read_u32(d, 0x10);
        const auto pair_count = read_u32(d, 0x08);
        const std::size_t table_bytes = static_cast<std::size_t>(glyph_count) * 32;
        if (glyph_count == 0 || glyph_offset < 8 || (pair_count != 0 && pair_offset < 8)) continue;
        const std::size_t glyph_start = glyph_offset - 8;
        const std::size_t pair_start = pair_count == 0 ? 0 : pair_offset - 8;
        if (glyph_start > chunk->size || table_bytes > chunk->size - glyph_start ||
            pair_start > chunk->size || pair_count > (chunk->size - pair_start) / 8) continue;
        font.glyphs.reserve(glyph_count);
        for (std::uint32_t i = 0; i < glyph_count; ++i) {
            const std::size_t off = glyph_start + static_cast<std::size_t>(i) * 32;
            FeGlyph g;
            g.code = static_cast<std::uint16_t>(d[off] | (d[off + 1] << 8));
            g.flags = static_cast<std::uint16_t>(d[off + 2] | (d[off + 3] << 8));
            g.u = read_f32(d, off + 0x04);
            g.v = read_f32(d, off + 0x08);
            g.w = read_f32(d, off + 0x0c);
            g.h = read_f32(d, off + 0x10);
            g.advance = read_f32(d, off + 0x14);
            g.field_0x1c = read_u32(d, off + 0x1c);
            // 0013FFA8 stores max glyph height at runtime font+0x420;00140CD8 converts to int.
            if (g.h > static_cast<float>(font.line_height)) font.line_height = static_cast<std::uint32_t>(g.h);
            font.glyphs.push_back(g);
        }
        for (std::uint32_t i = 0; i < pair_count; ++i) {
            const auto off = pair_start + static_cast<std::size_t>(i) * 8;
            font.field_0x410.emplace_back(d[off], read_f32(d, off + 4));
        }
        fonts.push_back(std::move(font));
    }
    return fonts;
}

// =====================================================================================
// Recovered front-end menu runtime. Each function below is a translation of the ELF function
// named in its comment; the control flow, wrap-around behaviour, traversal order and fallback
// values are the original's. See docs/FRONTEND_MENU_INTERACTION.md for the evidence table.
// =====================================================================================

namespace {

// FUN_0016E038(sound_id) table at 0x002FB0A8, 16 bytes per record. Names verbatim. (E-0014-11)
struct MenuSoundRow { int id; const char* name; };
constexpr MenuSoundRow kMenuSoundTable_002FB0A8[] = {
    {1, "MenuLeftRight"},         {2, "MenuLeftRight"},
    {3, "MenuUpDown"},            {4, "MenuUpDown"},
    {5, "MenuSelect"},            {6, "MenuCancel"},
    {7, "MenuStart"},             {8, "MenuWrong"},
    {9, "MenuWrong"},             {10, "MenuTrackRightLeft"},
    {11, "MenuTrackRightLeft"},   {12, "MenuCarRight"},
    {13, "MenuCarLeft"},          {14, "MenuCarColour"},
    {15, "MenuCarDisplayOn"},     {16, "MenuCarDisplayActive"},
    {17, "MenuCarDisplayOff"},    {18, "MenuCarSelect"},
    {19, "TreeUpDown"},           {20, "TreeLeftRight"},
    {21, "TreeZoomInOut"},
};

MenuSoundSink g_menu_sound_sink;
MenuSoundRequest g_last_menu_sound_request;

// Packs a widget's stored RGBA bytes back into the single u32 the ELF keeps at widget +0x5C, so
// FUN_00169818's snapshot and FUN_00140CA0's masking operate on the same representation.
std::uint32_t pack_widget_colour(const FeWidget& widget) {
    return static_cast<std::uint32_t>(widget.r) | (static_cast<std::uint32_t>(widget.g) << 8) |
           (static_cast<std::uint32_t>(widget.b) << 16) |
           (static_cast<std::uint32_t>(widget.a) << 24);
}

void unpack_widget_colour(FeWidget& widget, std::uint32_t packed) {
    widget.r = static_cast<std::uint8_t>(packed & 0xffu);
    widget.g = static_cast<std::uint8_t>((packed >> 8) & 0xffu);
    widget.b = static_cast<std::uint8_t>((packed >> 16) & 0xffu);
    widget.a = static_cast<std::uint8_t>((packed >> 24) & 0xffu);
}

std::uint32_t pack_rgb(const MenuColour_00169BD0& colour) {
    return static_cast<std::uint32_t>(colour.r) | (static_cast<std::uint32_t>(colour.g) << 8) |
           (static_cast<std::uint32_t>(colour.b) << 16);
}

} // namespace

std::uint32_t StringHash_002534A8(const char* text) {
    // FUN_002534A8: h starts at -1 and folds each byte with h = h * 0x21 + byte.
    std::uint32_t hash = 0xffffffffu;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != 0; ++p) {
        hash = hash * 0x21u + static_cast<std::uint32_t>(*p);
    }
    return hash;
}

const char* menu_sound_name_002FB0A8(int sound_id) {
    for (const MenuSoundRow& row : kMenuSoundTable_002FB0A8) {
        if (row.id == sound_id) return row.name;
    }
    return nullptr;
}

void install_menu_sound_sink(MenuSoundSink sink) { g_menu_sound_sink = std::move(sink); }

void PlayMenuSound_0016E038(int sound_id) {
    // FUN_0016E038: record 0 is all zeroes, so id 0 plays nothing. The 34.0-unit retrigger guard
    // the original applies to a repeat of the same voice is NOT reproduced: the unit of that
    // constant was not recovered, and inventing one would be inventing behaviour.
    if (sound_id == kMenuSoundNone) return;
    g_last_menu_sound_request.sound_id = sound_id;
    g_last_menu_sound_request.name = menu_sound_name_002FB0A8(sound_id);
    if (g_menu_sound_sink) g_menu_sound_sink(g_last_menu_sound_request);
}

const MenuSoundRequest& last_menu_sound_request() { return g_last_menu_sound_request; }

FeWidget* FindWidget_00143D50(FeScreen& screen, std::uint32_t name_hash, int type) {
    // FUN_00143D50: linear walk of the screen's widget list; type 0 matches any type.
    for (FeWidget& widget : screen.widgets) {
        if (widget.name_hash != name_hash) continue;
        if (type == 0) return &widget;
        if (widget.type == static_cast<std::uint8_t>(type)) return &widget;
    }
    return nullptr;
}

void SetWidgetStringId_001449A8(FeScreen& screen, std::uint32_t name_hash,
                                std::uint32_t string_id) {
    // FUN_001449A8 -> FUN_00143D50(screen, hash, 1) -> FUN_00140EA8(widget, id). The type
    // argument is 1, so only a text widget matches.
    FeWidget* widget = FindWidget_00143D50(screen, name_hash, 1);
    if (widget == nullptr) return;
    // FUN_00140EA8, verbatim: the early-out on an unchanged id matters, because the dirty byte
    // and the placeholder clear only happen when the id actually changes.
    if (widget->string_id_0x60 == string_id) return;
    widget->string_id_0x60 = string_id;
    widget->dirty_0x2b = 5;
    widget->text.clear(); // `*(undefined1 *)(param_1 + 100) = 0`, i.e. +0x64 = '\0'
}

void SetWidgetVisible_00144968(FeScreen& screen, std::uint32_t name_hash, bool visible) {
    // FUN_00144968 -> FUN_00143D50(screen, hash, 0) -> FUN_00140728(widget, visible), which
    // writes +0x29. Type 0 here, so it matches a quad or a text widget.
    FeWidget* widget = FindWidget_00143D50(screen, name_hash, 0);
    if (widget == nullptr) return;
    widget->visible = visible ? 1 : 0;
}

void SetClassVisible_001454E8(FeScreen& screen, std::uint32_t class_hash, bool visible) {
    // FUN_001454E8: FUN_00143DD8 walks every widget of the class and each one takes the same
    // +0x29 write. The original's leading FUN_00143DD8(screen, hash, 0, 0) call discards its
    // result, so it is not reproduced.
    for (FeWidget& widget : screen.widgets) {
        if (widget.class_hash != class_hash) continue;
        widget.visible = visible ? 1 : 0;
    }
}

void SetWidgetColour_00140CA0(FeWidget& widget, const MenuColour_00169BD0& colour) {
    // FUN_00140CA0: keep the stored alpha byte, replace the low 24 bits, stamp +0x2b on change.
    const std::uint32_t current = pack_widget_colour(widget);
    const std::uint32_t updated = (current & 0xff000000u) | (pack_rgb(colour) & 0x00ffffffu);
    if (current != updated) {
        unpack_widget_colour(widget, updated);
        widget.dirty_0x2b = 7;
    }
}

int MenuItemIndexFromNameHash_0016A330(std::uint32_t name_hash) {
    // FUN_0016A330: sprintf/hash/compare over 0..15, falling back to 0.
    for (int index = 0; index < kMenuItemIndexLimit_0016A330; ++index) {
        char name[32];
        std::snprintf(name, sizeof(name), kMenuItemNameFormat_002B2760, index);
        if (StringHash_002534A8(name) == name_hash) return index;
    }
    return 0;
}

// ------------------------------------------------------------------- MenuThing_00169740

void MenuThing_00169740::Construct_00169740(MenuList_00169F78* owner, std::uint32_t name_hash,
                                            std::int32_t index) {
    name_hash_0x0C = name_hash;
    index_0x10 = index;
    state_0x08 = kMenuThingStateEnabled_bit0;
    if (owner == nullptr) {
        screen_0x24 = nullptr;
        pad_index_0x20 = -1;
    } else {
        pad_index_0x20 = owner->pad_index_0x18;
        screen_0x24 = owner->screen_0x14;
    }
    Resolve_00169818();
}

void MenuThing_00169740::Resolve_00169818() {
    // FUN_00169818: FindWidget(screen, name_hash, type 1), then snapshot the widget colour.
    if (screen_0x24 == nullptr) return;
    if (FeWidget* widget = FindWidget_00143D50(*screen_0x24, name_hash_0x0C, 1)) {
        authored_colour_0x1C = pack_widget_colour(*widget);
    }
}

void MenuThing_00169740::SetStateBits_00169BD0(std::uint32_t bits) {
    state_0x08 |= bits;
    ApplyStateColour_00169BD0();
}

void MenuThing_00169740::ClearStateBits_00169CB0(std::uint32_t bits) {
    // FUN_00169CB0 is FUN_00169BD0 with AND-NOT in place of OR; the tail is byte-identical.
    state_0x08 &= ~bits;
    ApplyStateColour_00169BD0();
}

void MenuThing_00169740::ApplyStateColour_00169BD0() {
    // The shared tail of FUN_00169BD0 / FUN_00169CB0: pick a colour from (state & 3), write it
    // through FUN_00143F88 -> FUN_00140CA0, then stamp the widget's +0x2b byte with 1.
    std::uint32_t packed = authored_colour_0x1C;
    const std::uint32_t selector = state_0x08 & 3u;
    if (selector == 0u || selector == 2u) {
        packed = pack_rgb(kDisabledColour_gp_0x5324);
    } else if (selector == 3u) {
        packed = pack_rgb(kHighlightColour_gp_0x5330) |
                 (static_cast<std::uint32_t>(kHighlightAlpha_00169BD0) << 24);
    }
    if (screen_0x24 == nullptr) return;
    FeWidget* widget = FindWidget_00143D50(*screen_0x24, name_hash_0x0C, 1);
    if (widget == nullptr) return;
    SetWidgetColour_00140CA0(*widget,
                             MenuColour_00169BD0{static_cast<std::uint8_t>(packed & 0xffu),
                                                 static_cast<std::uint8_t>((packed >> 8) & 0xffu),
                                                 static_cast<std::uint8_t>((packed >> 16) & 0xffu)});
    widget->dirty_0x2b = 1;
}

void MenuThing_00169740::Highlight_00169968(const MenuCallback& callback) {
    SetStateBits_00169BD0(kMenuThingStateHighlighted_bit1);
    if (!callback) return;
    MenuEvent_0016A458 event;
    event.field_0x00 = name_hash_0x0C;
    event.pad_index_0x04 = pad_index_0x20;
    callback(kMenuMessage0x04, event);
    PlayMenuSound_0016E038(event.sound_id_0x0C);
}

void MenuThing_00169740::Unhighlight_001699F8(const MenuCallback& callback) {
    ClearStateBits_00169CB0(kMenuThingStateHighlighted_bit1);
    if (!callback) return;
    MenuEvent_0016A458 event;
    event.field_0x00 = name_hash_0x0C;
    event.pad_index_0x04 = pad_index_0x20;
    callback(kMenuMessage0x05, event);
    PlayMenuSound_0016E038(event.sound_id_0x0C);
}

void MenuThing_00169740::Refresh_00169858(const MenuCallback& callback) {
    if (!callback) return;
    MenuEvent_0016A458 event;
    event.field_0x00 = name_hash_0x0C;
    event.pad_index_0x04 = pad_index_0x20;
    callback(kMenuMessage0x02, event);
    PlayMenuSound_0016E038(event.sound_id_0x0C);
}

// ------------------------------------------------------------------- FUN_00169D98

void Sub_00169D98(MenuThing_00169740& item, std::int32_t pad_index, std::int32_t joy_event,
                   bool pressed, const MenuCallback& list_callback) {
    if (!pressed) return; // the original's `param_4 == 0` gate

    MenuEvent_0016A458 event;
    event.field_0x00 = item.name_hash_0x0C;
    event.pad_index_0x04 = pad_index;
    event.result_0x08 = joy_event; // the original stores the event id in field +0x08
    event.sound_id_0x0C = 0;

    int message = 0;
    switch (joy_event) {
        case hp2::input::JOY_EVENT_SCROLL_LEFT:
        case hp2::input::JOY_EVENT_MENU_ANALOG_LEFT:
            message = kMenuMessage0x06;
            break;
        case hp2::input::JOY_EVENT_SCROLL_RIGHT:
        case hp2::input::JOY_EVENT_MENU_ANALOG_RIGHT:
            message = kMenuMessage0x07;
            break;
        case hp2::input::JOY_EVENT_START:
        case hp2::input::JOY_EVENT_MENU_SELECT:
            event.sound_id_0x0C = kMenuSoundMenuSelect_5;
            message = kMenuMessage0x01;
            break;
        case hp2::input::JOY_EVENT_MENU_SQUARE:
            // The original takes a separate exit for this case: callback message 10, then the
            // sound, and no shared tail.
            if (list_callback) list_callback(kMenuMessage0x0A, event);
            PlayMenuSound_0016E038(event.sound_id_0x0C);
            return;
        default:
            return; // every other event is dropped here
    }

    if (list_callback) list_callback(message, event);
    PlayMenuSound_0016E038(event.sound_id_0x0C);
}

// ------------------------------------------------------------------- MenuList_00169F78

void MenuList_00169F78::Construct_00169F78(MenuGroup_0016AA00* owner, MenuCallback callback,
                                            std::int32_t initial_index, const char* name_format) {
    callback_0x0C = std::move(callback);
    next_0x00 = sentinel();
    prev_0x04 = sentinel();
    owner_0x10 = owner;
    if (owner != nullptr) {
        pad_index_0x18 = owner->pad_index_0x10;
        screen_0x14 = owner->screen_0x14;
    }
    count_0x1C = 0;
    storage_.clear();

    for (int index = 0; index < kMenuItemIndexLimit_0016A330; ++index) {
        char name[32];
        std::snprintf(name, sizeof(name), name_format, index);
        const std::uint32_t name_hash = StringHash_002534A8(name);
        if (screen_0x14 == nullptr) continue;
        if (FindWidget_00143D50(*screen_0x14, name_hash, 1) == nullptr) continue;

        storage_.push_back(std::make_unique<MenuThing_00169740>());
        MenuThing_00169740* item = storage_.back().get();
        item->Construct_00169740(this, name_hash, index);
        // FUN_00169F78's append: tail->next = item, list->prev = item, item->prev = old tail,
        // item->next = the list head (sentinel).
        MenuThing_00169740* tail = prev_0x04;
        tail->next_0x00 = item;
        prev_0x04 = item;
        item->prev_0x04 = tail;
        item->next_0x00 = sentinel();
        ++count_0x1C;
    }

    SetCurrentIndex_0016A0F0(initial_index);
    highlight_bar_0x20 =
        screen_0x14 == nullptr
            ? nullptr
            : FindWidget_00143D50(*screen_0x14, kHighlightBarNameHash_00169F78, 2);
}

std::vector<MenuThing_00169740*> MenuList_00169F78::items() {
    std::vector<MenuThing_00169740*> out;
    for (MenuThing_00169740* node = next_0x00; node != nullptr && node != sentinel();
         node = node->next_0x00) {
        out.push_back(node);
    }
    return out;
}

MenuThing_00169740* MenuList_00169F78::Nth_002503F8(std::int32_t index) {
    // FUN_002503F8: walk `index` links forward from the head; the sentinel comes back when the
    // list is shorter than `index`.
    if (index < 0) return sentinel();
    MenuThing_00169740* node = next_0x00;
    if (node == nullptr) return sentinel();
    for (std::int32_t i = 0; i < index && node != sentinel(); ++i) node = node->next_0x00;
    return node;
}

MenuThing_00169740* MenuList_00169F78::FindByHash_0016A5B0(std::uint32_t name_hash) {
    // FUN_0016A5B0: the head is compared against the list address itself, so an empty list
    // returns 0 before any field is read.
    for (MenuThing_00169740* node = next_0x00; node != nullptr && node != sentinel();
         node = node->next_0x00) {
        if (node->name_hash_0x0C == name_hash) return node;
    }
    return nullptr;
}

void Sub_0016D2A8(MenuList_00169F78* list, FeScreen& screen, std::uint32_t name_hash, int mode) {
    // FUN_0016D2A8. The two lookups the port is handed the results of (FUN_0016C8C8 on the
    // class name, FUN_0016B340 on the group) both return 0 silently when they miss, and the
    // original then returns; so does this.
    MenuThing_00169740* item = list == nullptr ? nullptr : list->FindByHash_0016A5B0(name_hash);
    if (item == nullptr) return;
    if (mode == 0 || mode == 2) item->ClearStateBits_00169CB0(kMenuThingStateEnabled_bit0);
    else item->SetStateBits_00169BD0(kMenuThingStateEnabled_bit0);
    if (mode == 2) SetWidgetVisible_00144968(screen, name_hash, false);
    if (mode == 1) SetWidgetVisible_00144968(screen, name_hash, true);
}

void MenuList_00169F78::SetCurrentIndex_0016A0F0(std::int32_t index) {
    // FUN_0016A0F0: store the n-th node; when that is the sentinel, fall back to the first item,
    // and when the list is empty leave the current pointer null.
    MenuThing_00169740* node = Nth_002503F8(index);
    current_0x08 = node;
    if (node != sentinel()) return;
    MenuThing_00169740* head = node->next_0x00;
    current_0x08 = head;
    if (head == node) current_0x08 = nullptr;
}

std::int32_t MenuList_00169F78::CurrentIndex_0016A130() const {
    // FUN_0016A130: count forward from the head to the current item; 0 for an empty list or an
    // unreachable current pointer.
    const MenuThing_00169740* node = next_0x00;
    if (node == nullptr || node == sentinel()) return 0;
    std::int32_t index = 0;
    while (node != sentinel()) {
        if (node == current_0x08) return index;
        node = node->next_0x00;
        ++index;
    }
    return 0;
}

void MenuList_00169F78::RefreshAll_0016A160() {
    // FUN_0016A160: message 0x02 to every item first, then advance the current pointer onto the
    // first enabled item, wrapping over the head. When no item is enabled the original leaves the
    // pointer on the last node it examined -- reproduced, not tidied.
    MenuThing_00169740* node = next_0x00;
    if (node != nullptr && node != sentinel()) {
        while (node != sentinel()) {
            node->Refresh_00169858(callback_0x0C);
            node = node->next_0x00;
        }
    }
    MenuThing_00169740* base = current_0x08;
    if (base == nullptr || base == sentinel()) return;

    MenuThing_00169740* probe = base;
    MenuThing_00169740* chosen = base;
    do {
        chosen = probe;
        if (chosen->enabled()) break;
        probe = chosen->next_0x00;
        if (probe == sentinel()) probe = next_0x00;
    } while (probe != base);

    current_0x08 = chosen;
    chosen->Highlight_00169968(callback_0x0C);
}

void MenuList_00169F78::Next_0016A5E0() {
    // FUN_0016A5E0.
    MenuThing_00169740* current = current_0x08;
    if (current == nullptr || current == sentinel()) return;
    MenuThing_00169740* target = nullptr;
    if (!kSkipDisabledAndWrap_gp_0x533C) {
        target = current->next_0x00;
        if (target == sentinel()) target = current; // no wrap while the global is zero
    } else {
        target = current->next_0x00;
        if (target == sentinel()) target = next_0x00;
        if (target == current) return;
        if (!target->enabled()) {
            target = target->next_0x00;
            while (true) {
                if (target == sentinel()) target = next_0x00;
                if (target == current) return;
                if (target->enabled()) break;
                target = target->next_0x00;
            }
        }
    }
    if (target == current) return;
    current->Unhighlight_001699F8(callback_0x0C);
    current_0x08 = target;
    target->Highlight_00169968(callback_0x0C);
}

void MenuList_00169F78::Previous_0016A6C8() {
    // FUN_0016A6C8: FUN_0016A5E0 walked through the prev links, wrapping onto the tail.
    MenuThing_00169740* current = current_0x08;
    if (current == nullptr || current == sentinel()) return;
    MenuThing_00169740* target = nullptr;
    if (!kSkipDisabledAndWrap_gp_0x533C) {
        target = current->prev_0x04;
        if (target == sentinel()) target = current;
    } else {
        target = current->prev_0x04;
        if (target == sentinel()) target = prev_0x04;
        if (target == current) return;
        if (!target->enabled()) {
            target = target->prev_0x04;
            while (true) {
                if (target == sentinel()) target = prev_0x04;
                if (target == current) return;
                if (target->enabled()) break;
                target = target->prev_0x04;
            }
        }
    }
    if (target == current) return;
    current->Unhighlight_001699F8(callback_0x0C);
    current_0x08 = target;
    target->Highlight_00169968(callback_0x0C);
}

void MenuList_00169F78::HandleJoyEvent_0016A458(std::int32_t pad_index, std::int32_t joy_event,
                                                 bool pressed) {
    // FUN_0016A458.
    if (!pressed) return;

    MenuEvent_0016A458 event;
    if (current_0x08 != nullptr && current_0x08 != sentinel()) {
        event.field_0x00 = current_0x08->name_hash_0x0C;
    }
    event.pad_index_0x04 = pad_index;

    int message = 0;
    switch (joy_event) {
        case hp2::input::JOY_EVENT_SCROLL_UP:
        case hp2::input::JOY_EVENT_MENU_ANALOG_UP:
            message = kMenuMessage0x08;
            event.sound_id_0x0C = kMenuSoundMenuUpDown_3;
            break;
        case hp2::input::JOY_EVENT_SCROLL_DOWN:
        case hp2::input::JOY_EVENT_MENU_ANALOG_DOWN:
            message = kMenuMessage0x09;
            event.sound_id_0x0C = kMenuSoundMenuUpDown_4;
            break;
        default:
            break; // forwarded to the current item below, exactly as the original does
    }

    if (callback_0x0C && message != 0) callback_0x0C(message, event);

    if (event.result_0x08 == 0) {
        if (message == kMenuMessage0x08) {
            Previous_0016A6C8();
            PlayMenuSound_0016E038(event.sound_id_0x0C);
        } else if (message == kMenuMessage0x09) {
            Next_0016A5E0();
            PlayMenuSound_0016E038(event.sound_id_0x0C);
        }
    }

    if (current_0x08 != nullptr && current_0x08 != sentinel()) {
        Sub_00169D98(*current_0x08, pad_index, joy_event, pressed, callback_0x0C);
    }
}

// ------------------------------------------------------------------- MenuGroup_0016AA00

void MenuGroup_0016AA00::EnsureMenu_0016A858() {
    // FUN_0016A858.
    if (child_0x08 == nullptr) {
        owned_list_ = std::make_unique<MenuList_00169F78>();
        owned_list_->Construct_00169F78(this, callback_0x18, initial_index_0x0C);
        child_0x08 = owned_list_.get();
    }
    MenuEvent_0016A458 event;
    event.field_0x00 =
        screen_0x14 == nullptr ? 0u : static_cast<std::uint32_t>(screen_0x14->index);
    event.pad_index_0x04 = pad_index_0x10;
    event.result_0x08 = -1;
    if (callback_0x18) callback_0x18(kMenuMessage0x0B, event);
    if (event.result_0x08 != -1) initial_index_0x0C = event.result_0x08 - 1;
    PlayMenuSound_0016E038(event.sound_id_0x0C);
    child_0x08->SetCurrentIndex_0016A0F0(initial_index_0x0C);
    child_0x08->RefreshAll_0016A160();
}

void MenuGroup_0016AA00::HandleJoyEvent_0016AA00(std::int32_t pad_index, std::int32_t joy_event,
                                                  bool pressed) {
    // FUN_0016AA00: pad filter, four group-level messages, then forward to the child list.
    if (!(pad_index_0x10 == -1 || pad_index_0x10 == pad_index)) return;
    if (!pressed) return;

    auto fire = [&](int message) -> std::int32_t {
        MenuEvent_0016A458 event;
        event.field_0x00 =
            screen_0x14 == nullptr ? 0u : static_cast<std::uint32_t>(screen_0x14->index);
        event.pad_index_0x04 = pad_index;
        if (callback_0x18) callback_0x18(message, event);
        PlayMenuSound_0016E038(event.sound_id_0x0C);
        return event.result_0x08;
    };

    if (joy_event == hp2::input::JOY_EVENT_START) {
        if (fire(kMenuMessage0x0D) == 1) return; // the original consumes the event on result 1
    }
    if (joy_event == hp2::input::JOY_EVENT_SELECT) fire(kMenuMessage0x0E);
    if (joy_event == hp2::input::JOY_EVENT_MENU_L1) fire(kMenuMessage0x0F);
    if (joy_event == hp2::input::JOY_EVENT_MENU_R1) fire(kMenuMessage0x10);

    if (child_0x08 != nullptr) {
        child_0x08->HandleJoyEvent_0016A458(pad_index, joy_event, pressed);
    }
}


// ------------------------------------------- recovered but not translated (AGENT.md R3)

void Sub_0016BAC0(std::int32_t, std::int32_t, bool) {
    hp2::not_implemented("0x0016BAC0",
                        "pop-up / help menu joypad handling (POPUP_1..3 handler table unrecovered)");
}

void Sub_0016FBB0() {
    hp2::not_implemented("0x0016FBB0",
                        "global FMenus constructor bindings (main-menu pulse is implemented separately)");
}

std::string localised_text(std::uint32_t) {
    hp2::not_implemented("LANGUAGES/FE_*.BIN", "localised string table format not parsed");
}


// ------------------------------------------------------------- recovered screen (document) names
namespace {
// Confirmed by exact StringHash_002534A8 match against the hash each screen stores at
// 0x10203 +0x08. See docs/FRONTEND_MENU_INTERACTION.md for the method and the 23 open hashes.
constexpr RecoveredScreenName kRecoveredScreenNames[] = {
    {0, "NEW_MAIN_MENU"},      {4, "SELECT_CAR"},         {7, "SELECT_CAR_DIALOG"},
    {10, "SELECT_TRACK"},      {11, "CHALLENGE"},         {12, "RACE_SETUP"},
    {13, "JOYSTICK_CONFIG"},   {14, "POPUP"},             {16, "CREDITS"},
    {22, "TITLE_SCREEN"},      {24, "OPTIONS"},           {26, "DISPLAY_OPTIONS"},
    {28, "AUDIO_OPTIONS"},     {29, "USERNAME_OPTIONS"},  {30, "LANGUAGE_OPTIONS"},
    {35, "LOADSAVE"},          {36, "BLANK_SCREEN"},      {40, "CREDITS_SCREEN"},
    // E-0022-R02: exact hashes in original FRONTB.BUN, under StringHash_002534A8.
    {17, "TREE_DETAIL_VIEW"}, {19, "TREE_REWARD_VIEW"}, {39, "NEW_EVENT_TREE"},
};
} // namespace

const RecoveredScreenName* recovered_screen_names(std::size_t& count) {
    count = sizeof(kRecoveredScreenNames) / sizeof(kRecoveredScreenNames[0]);
    return kRecoveredScreenNames;
}

int screen_index_for_name_hash(const std::vector<FeScreen>& screens, std::uint32_t name_hash) {
    for (const FeScreen& screen : screens) {
        if (screen.name_hash_0x08 == name_hash) return screen.index;
    }
    return -1;
}

int screen_index_for_screen_name(const std::vector<FeScreen>& screens, const char* screen_name) {
    return screen_index_for_name_hash(screens, StringHash_002534A8(screen_name));
}


// ---------------------------------------------- recovered widget geometry + highlight animation

WidgetRect_00144280 GetWidgetRect_00144280(FeScreen& screen, std::uint32_t name_hash,
                                            const WidgetMeasure& measure) {
    WidgetRect_00144280 rect;
    FeWidget* widget = FindWidget_00143D50(screen, name_hash, 0);
    if (widget == nullptr) return rect;
    const WidgetMetrics metrics = measure ? measure(*widget) : WidgetMetrics{};
    rect.x0 = widget->x;
    rect.y0 = widget->y;
    rect.x1 = widget->x + metrics.width;
    rect.y1 = widget->y + metrics.height;
    rect.valid = true;
    return rect;
}

void SetWidgetY_00144458(FeScreen& screen, std::uint32_t name_hash, int y) {
    FeWidget* widget = FindWidget_00143D50(screen, name_hash, 0);
    if (widget == nullptr) return;
    widget->y = static_cast<float>(y);
    widget->dirty_0x2b = 3;
}

void SetWidgetAlpha_00140ED0(FeWidget& widget, int alpha) {
    const std::uint8_t value = static_cast<std::uint8_t>(alpha & 0xff);
    // Port dispatch for existing generic callers: quad alpha is001411D8(-1), E-0039-Q09.
    // Text00140ED0 touches only its one colour; quad setter preserves all four RGB words.
    if (widget.type == 2) {
        for (auto& corner : widget.field_0x5c)
            corner = (corner & 0xffffffu) | (static_cast<std::uint32_t>(value) << 24);
        widget.dirty_0x2b = 6;
    }
    if (widget.a != value) {
        widget.a = value;
        widget.dirty_0x2b = 6;
    }
}

void CopyWidgetTransform_001410A0(FeWidget& dst, const FeWidget& src) {
    dst.clip_hash_0x4c = src.clip_hash_0x4c;
    // The ELF copies the raw 0x40 bytes at +0x28 and restores dst+0x2C afterwards. The fields in
    // that window are modelled individually here; +0x2C is the one the original preserves, and it
    // is not part of this parser's field set, so nothing has to be restored.
    dst.type = src.type;
    dst.visible = src.visible;
    dst.layer = src.layer;
    dst.x = src.x;
    dst.y = src.y;
    dst.scale_x = src.scale_x;
    dst.scale_y = src.scale_y;
    dst.field_0x40 = src.field_0x40;
    dst.field_0x44 = src.field_0x44;
    dst.field_0x5c = src.field_0x5c; // within the recovered +28..+67 copy window
    dst.texture_hash = src.texture_hash;
    dst.r = src.r;
    dst.g = src.g;
    dst.b = src.b;
    dst.a = src.a;
    dst.text_capacity = src.text_capacity;
    dst.text_align = src.text_align;
    dst.string_id_0x60 = src.string_id_0x60;
    dst.text = src.text;
    dst.dirty_0x2b = 8;
}

void SetWidgetScale_00140FA8(FeWidget& widget, float scale, const WidgetMetrics& metrics) {
    widget.dirty_0x2b = 3;
    const float half_width_per_scale =
        widget.scale_x != 0.0f ? metrics.width / (widget.scale_x + widget.scale_x) : 0.0f;
    const float half_height_per_scale =
        widget.scale_y != 0.0f ? metrics.height / (widget.scale_y + widget.scale_y) : 0.0f;
    const std::uint32_t anchor = widget.text_align;
    if ((anchor & 2u) != 0u) {
        widget.y += half_height_per_scale * (widget.scale_y - scale);
        widget.x += half_width_per_scale * (widget.scale_x - scale);
        widget.scale_y = scale;
    } else if ((anchor & 4u) != 0u) {
        widget.y += half_height_per_scale * (widget.scale_y - scale);
        widget.scale_y = scale;
    } else if ((anchor & 1u) != 0u) {
        const float delta = widget.scale_x - scale;
        widget.y += half_height_per_scale * delta;
        widget.x -= half_width_per_scale * delta;
        widget.scale_y = scale;
    } else {
        // The original writes only +0x3C in this arm and then falls through to +0x38.
        widget.scale_y = scale;
    }
    widget.scale_x = scale;
}

void MenuHighlightAnim_00248870::Construct_00248870(FeScreen* screen, std::uint32_t template_hash,
                                                     std::int32_t colour, float now_seconds) {
    screen_0x00 = screen;
    target_hash_0x04 = 0;
    start_time_0x08 = now_seconds;
    template_hash_0x0C = template_hash;
    colour_0x10 = colour;
    scale_start_0x14 = kAnimScaleStart_gp_0x3AD8;
    scale_end_0x18 = kAnimScaleEnd_gp_0x3AD4;
    period_0x1C = kAnimDefaultPeriod_gp_0x3AC8;
    active_ = false;
}

void MenuHighlightAnim_00248870::Bind_002488D0(std::uint32_t target_hash, float now_seconds) {
    target_hash_0x04 = target_hash;
    start_time_0x08 = now_seconds;
    float target_scale = 1.0f;
    if (screen_0x00 != nullptr) {
        // FUN_001444E8 reads the widget's +0x38/+0x3C; the ELF uses +0x38 for both ends.
        if (const FeWidget* widget = FindWidget_00143D50(*screen_0x00, target_hash, 0)) {
            target_scale = widget->scale_x;
        }
    }
    scale_start_0x14 = target_scale * kAnimScaleStart_gp_0x3AD8;
    scale_end_0x18 = target_scale * kAnimScaleEnd_gp_0x3AD4;
    active_ = true;
}

void MenuHighlightAnim_00248870::Update_00248970(float now_seconds, const WidgetMeasure& measure) {
    if (screen_0x00 == nullptr) return;
    const float elapsed_s = now_seconds - start_time_0x08;
    if (elapsed_s < kAnimDuration_gp_0x3ADC) {
        // FUN_0025A5E0: sine of the scaled argument. The ELF reaches it through a 16-bit angle
        // table (factor 65536 / 2pi), which is a sine, so std::sin reproduces it.
        const float phase = (elapsed_s * kAnimQuarterTurn_gp_0x678C) / kAnimDuration_gp_0x3ADC;
        const float eased = std::sin(phase);
        const float scale = scale_start_0x14 + (scale_end_0x18 - scale_start_0x14) * eased;
        const float alpha = kAnimAlphaStart_gp_0x3AD0 +
                            (kAnimAlphaEnd_gp_0x3ACC - kAnimAlphaStart_gp_0x3AD0) * eased;

        FeWidget* target = FindWidget_00143D50(*screen_0x00, target_hash_0x04, 1);
        FeWidget* ghost = FindWidget_00143D50(*screen_0x00, template_hash_0x0C, 1);
        if (target == nullptr) {
            if (ghost != nullptr) ghost->visible = 0; // the original's only hide path
        } else if (ghost != nullptr) {
            CopyWidgetTransform_001410A0(*ghost, *target);
            const WidgetMetrics metrics = measure ? measure(*ghost) : WidgetMetrics{};
            SetWidgetScale_00140FA8(*ghost, scale, metrics);
            SetWidgetAlpha_00140ED0(*ghost, static_cast<int>(alpha));
            if (colour_0x10 != -1) {
                SetWidgetColour_00140CA0(
                    *ghost, MenuColour_00169BD0{
                                static_cast<std::uint8_t>(colour_0x10 & 0xff),
                                static_cast<std::uint8_t>((colour_0x10 >> 8) & 0xff),
                                static_cast<std::uint8_t>((colour_0x10 >> 16) & 0xff)});
            }
        }
    }
    if (period_0x1C < elapsed_s) start_time_0x08 = now_seconds;
}

void PlaceSelectionBars_00216880(FeScreen& screen, std::uint32_t mask_hash,
                                 const WidgetMeasure& measure) {
    const WidgetRect_00144280 rect = GetWidgetRect_00144280(screen, mask_hash, measure);
    if (!rect.valid) return;
    SetWidgetY_00144458(screen, StringHash_002534A8(kUnderscoreName),
                        static_cast<int>(rect.y1 + kUnderscoreOffset_gp_0x3EF0));
    SetWidgetY_00144458(screen, StringHash_002534A8(kOverscoreName),
                        static_cast<int>(rect.y0 - kOverscoreOffset_gp_0x3EF4));
}

WidgetRect_00144280 GetWidgetClip_00145F30(FeScreen& screen, const FeWidget& widget,
                                        const WidgetMeasure& measure) {
    if (!widget.clip_hash_0x4c || (widget.type == 1 && !(widget.text_align & 0x10))) return {};
    auto* mask = FindWidget_00143D50(screen, widget.clip_hash_0x4c, 0);
    if (!mask) return {};
    if (!measure) hp2::not_implemented("0x00145F30", "clip-mask measurement missing");
    const auto size = measure(*mask);
    if (widget.type == 1)
        return {mask->x+4,mask->y,mask->x+static_cast<int>(size.width),
                mask->y+static_cast<int>(size.height),true};
    return {mask->x,mask->y,mask->x+size.width,mask->y+size.height,true};
}

void Sub_001F4E68(FeScreen& screen, std::uint32_t mask_hash,
                               const WidgetMeasure& measure) {
    const auto rect=GetWidgetRect_00144280(screen,mask_hash,measure);
    if(!rect.valid)return;
    SetWidgetY_00144458(screen,0x32c91dae,static_cast<int>(rect.y1));
    SetWidgetY_00144458(screen,0xc671680c,static_cast<int>(rect.y0)-6);
}

void tScrollingMenuList::Construct_00249DE0(FeScreen& screen,std::uint32_t top_hash,
    std::uint32_t bottom_hash,std::uint32_t up_hash,std::uint32_t down_hash,std::uint32_t clip_hash) {
    field_0x00 = &screen; field_0x04.clear(); field_0x14 = -1; field_0x24 = 0;
    field_0x28=up_hash;field_0x2c=down_hash;field_0x30=clip_hash;field_0x34=0;
    auto* top = FindWidget_00143D50(screen, top_hash, 0);
    auto* bottom = FindWidget_00143D50(screen, bottom_hash, 0);
    field_0x0c = top ? top->y : 0; field_0x10 = bottom ? bottom->y : 0;
    for (auto hash : {field_0x28,field_0x2c})
        if(auto* arrow=FindWidget_00143D50(screen,hash,0))SetWidgetAlpha_00140ED0(*arrow,64);
}
void tScrollingMenuList::Sub_001EF3C8(FeScreen& screen) {
    Construct_00249DE0(screen,0x52e039f3,0x52e039f9,0xedf2e4ee,0x20d73d01,0xc67cfb4a);
    // ELF initialized data002B9C30..002B9C97, copied by001EF3C8.
    constexpr std::uint32_t hashes[]{
        0x52e039f3,0x52e039f4,0x52e039f5,0x52e039f6,0x52e039f7,0x52e039f8,
        0x52e039f9,0x52e039fa,0x52e039fb,0xaee77883,0xaee77884,
        0xa29f7a16,0xc866642b,0xa29f7a17,0xc866686c,0xa29f7a18,0xc8666cad,
        0xa29f7a19,0xc86670ee,0xa29f7a1a,0xc866752f,0xa29f7a1b,0xc8667970,
        0xa29f7a1c,0xc8667db1,0x2932dfca};
    Add_00249FF8(std::vector<std::uint32_t>(std::begin(hashes),std::end(hashes)));
    field_0x34=11;
}
void tScrollingMenuList::Sub_001F33B8(FeScreen& screen) {
    Construct_00249DE0(screen,0x52e039f3,0x52e039f9,0xedf2e4ee,0x20d73d01,0xc67cfb4a);
    // Exact15-element initialized list002BA178, passed by001F33B8.
    Add_00249FF8({0x52e039f3,0x52e039f4,0x52e039f5,0x52e039f6,0x52e039f7,
        0x52e039f8,0x52e039f9,0x52e039fa,0x52e039fb,0x55cb09f2,0x2e931f1b,
        0x03d90172,0x5610f2f4,0x8fe4086b,0x422313cb});
    field_0x34=9;
    for(auto hash:{0xd1cda594u,0x9345e4a7u})if(auto*w=FindWidget_00143D50(screen,hash,0)){
        w->clip_hash_0x4c=field_0x30;if(w->type==1)w->text_align|=0x10;
    }
}
void tScrollingMenuList::Add_00249FF8(const std::vector<std::uint32_t>& hashes) {
    if(!field_0x00)hp2::not_implemented("0x00249FF8","scrolling screen not bound");
    auto& screen=*field_0x00;
    for (auto hash : hashes) {
        auto* widget = FindWidget_00143D50(screen, hash, 0);
        if (!widget) continue;
        widget->clip_hash_0x4c = field_0x30;
        if (widget->type == 1) widget->text_align |= 0x10;
        widget->dirty_0x2b = widget->type == 1 ? 5 : 4;
        tMenuAnimElement node;
        node.field_0x3c = static_cast<std::size_t>(widget-screen.widgets.data());
        node.field_0x0c = node.field_0x14 = widget->y;
        field_0x04.push_back(node);
    }
}

void tScrollingMenuList::Handle_0024A480(int message, std::uint32_t item_hash) {
    if (!field_0x00) return;
    if (message == 8) { if (--field_0x14 < 0) field_0x14 = 0; return; }
    if (message == 9) {
        const int maximum=std::min(7,field_0x34);
        if (++field_0x14 >= maximum) field_0x14 = maximum-1;
        return;
    }
    if (message != 4) return;
    if (field_0x14 == -1) {
        const auto item = item_hash-0x52e039f3u;
        if (item <= 8) field_0x14 = static_cast<int>(item < 6 ? item : 6);
        else if (item_hash==0xaee77883 || item_hash==0xaee77884) field_0x14=6;
    }
    auto* selected = FindWidget_00143D50(*field_0x00,item_hash,0);
    if (selected && (selected->y < field_0x0c || selected->y > field_0x10)) {
        auto it = std::find_if(field_0x04.begin(),field_0x04.end(),[&](const auto& node){
            return field_0x00->widgets[node.field_0x3c].name_hash==item_hash;
        });
        if (it == field_0x04.end()) hp2::not_implemented("0x0024A090", "selected widget not in scrolling list");
        const float bound = selected->y < field_0x0c ? field_0x0c : field_0x10;
        const float offset = bound-it->field_0x14;
        for(auto& node:field_0x04) {
            node.field_0x14 += offset;
            if(node.field_0x14!=node.field_0x0c)node.field_0x38=2;
        }
    }
    bool above=false,below=false;
    for(const auto& node:field_0x04) if(field_0x34>7) {
        above |= node.field_0x14 < field_0x0c;
        below |= node.field_0x14 > field_0x10;
    }
    if(auto* a=FindWidget_00143D50(*field_0x00,field_0x28,0))a->visible=above?1:0;
    if(auto* a=FindWidget_00143D50(*field_0x00,field_0x2c,0))a->visible=below?1:0;
}

void tScrollingMenuList::Update_0024A1A8(float delta) {
    if (!field_0x00) return;
    for(auto& node:field_0x04) {
        if(node.field_0x38==2) {
            node.field_0x38=1;node.field_0x2c=0;
            const float d=node.field_0x14-node.field_0x0c;
            node.field_0x1c=node.field_0x10-2*d;
            node.field_0x20=3*d-2*node.field_0x10;
            node.field_0x24=node.field_0x10;node.field_0x28=node.field_0x0c;
        }
        if(node.field_0x38==1) {
            node.field_0x2c+=delta/0.5f;const float t=node.field_0x2c;
            if(t>=1){node.field_0x0c=node.field_0x14;node.field_0x10=0;node.field_0x38=0;}
            else {
                node.field_0x0c=((node.field_0x1c*t+node.field_0x20)*t+node.field_0x24)*t+node.field_0x28;
                node.field_0x10=(3*node.field_0x1c*t+2*node.field_0x20)*t+node.field_0x24;
            }
        }
        auto& w=field_0x00->widgets[node.field_0x3c];
        if(w.y!=node.field_0x0c){w.y=node.field_0x0c;w.dirty_0x2b=4;}
    }
    field_0x24 += delta*10;
    if(field_0x24>6.283f)field_0x24-=6.283f;
    // FUN0025A5E0 table quantization remains the existing port std::sin limitation.
    const int alpha=static_cast<int>(128*(std::sin(field_0x24)+1)*0.5f);
    for(auto hash:{field_0x28,field_0x2c})
        if(auto* a=FindWidget_00143D50(*field_0x00,hash,0))SetWidgetAlpha_00140ED0(*a,alpha);
}
std::uint32_t tScrollingMenuList::Sub_0024A368() const {
    if(field_0x14<0 || field_0x14>6)hp2::not_implemented("0x0024A368", "uninitialized viewport slot");
    constexpr std::uint32_t masks[]{0x6e34e350,0x708a0bf1,0x72df3492,0x75345d33,
        0x778985d4,0x79deae75,0x7c33d716};
    return masks[field_0x14];
}

void tLeftRightArrowScroll::Construct_0024A690(FeScreen& screen,std::uint32_t left,std::uint32_t right){
    field_0x00=&screen;field_0x04=left;field_0x08=right;field_0x1c=0;
    Bind_0024A860(0);
}
void tLeftRightArrowScroll::Bind_0024A860(std::uint32_t target){
    field_0x0c=target;
    if(!field_0x00)return;
    for(auto hash:{field_0x04,field_0x08})
        if(auto*w=FindWidget_00143D50(*field_0x00,hash,2))w->visible=target?1:0;
}
void tLeftRightArrowScroll::Update_0024A730(float delta,const WidgetMeasure& measure){
    if(!field_0x00)return;
    field_0x1c+=delta*10;
    if(field_0x1c>6.283f)field_0x1c-=6.283f;
    if(field_0x0c){
        const auto rect=GetWidgetRect_00144280(*field_0x00,field_0x0c,measure);
        if(!rect.valid)hp2::not_implemented("0x00144860","selector target bounds unavailable");
        // 001447E0 computes centre after bounding-box conversion to integer.
        const int top=static_cast<int>(rect.y0),bottom=static_cast<int>(rect.y1);
        const int centre=top+(bottom-top)/2;
        const int alpha=static_cast<int>(128*(std::sin(field_0x1c)+1)*0.5f);
        for(auto hash:{field_0x04,field_0x08})if(auto*w=FindWidget_00143D50(*field_0x00,hash,2)){
            const auto size=measure(*w);
            SetWidgetAlpha_00140ED0(*w,alpha);
            const float y=static_cast<float>(centre)-size.height*0.5f;
            if(w->y!=y){w->y=y;w->dirty_0x2b=4;}
        }
    }
    Bind_0024A860(field_0x0c);
}

void Sub_00249348() {
    hp2::not_implemented("0x00249348",
                        "scrolling-text update requires the bound tAnimatedBlurb screen state");
}

void tAnimatedBlurb::Construct_002491B0(FeScreen& screen) {
    Construct_002491B0(screen, 0x75980cf0, 0x75980cf0, 0x70a404ec);
}

void tAnimatedBlurb::Construct_002491B0(FeScreen& screen, std::uint32_t text_hash,
                                     std::uint32_t alpha_hash, std::uint32_t bar_hash) {
    field_0x04 = text_hash; port_alpha_hash = alpha_hash; port_bar_hash = bar_hash;
    field_0x00 = &screen; field_0x08 = field_0x198 = field_0x28 = 0;
    field_0x0c = field_0x19c = 0; field_0x94.clear();
    auto* text = FindWidget_00143D50(screen, field_0x04, 1);
    auto* bar = FindWidget_00143D50(screen, port_bar_hash, 0);
    field_0x194 = text ? text->x : 0;
    auto* alpha = FindWidget_00143D50(screen, port_alpha_hash, 0);
    field_0x5c_alpha = alpha ? alpha->a : 0;
    field_0x78_alpha = bar ? bar->a : 0;
    if (text) { text->x = 650; text->dirty_0x2b = 4; }
}

void tAnimatedBlurb::Bind_00249518(const std::string& text) {
    if (field_0x19c == 2 || field_0x19c == 3) {
        field_0x19c = 1; field_0x08 = 0; field_0x0c = 0;
    }
    field_0x198 = 0; field_0x94 = text;
}

void tAnimatedBlurb::Update_00249348(float delta_seconds) {
    if (!field_0x00) return;
    field_0x08 += delta_seconds; field_0x198 += delta_seconds;
    auto* text = FindWidget_00143D50(*field_0x00, field_0x04, 1);
    auto* alpha = FindWidget_00143D50(*field_0x00, port_alpha_hash, 0);
    auto* bar = FindWidget_00143D50(*field_0x00, port_bar_hash, 0);
    if (field_0x19c == 1) {
        const float t = field_0x08 < 0.6f ? field_0x08 / 0.6f : 1;
        if (alpha) SetWidgetAlpha_00140ED0(*alpha, static_cast<int>(field_0x5c_alpha * (1-t)));
        if (bar) SetWidgetAlpha_00140ED0(*bar, static_cast<int>(field_0x78_alpha * t));
        if (field_0x198 > 0.5f) { field_0x0c = 0; field_0x19c = 2; }
    }
    if (field_0x19c != 0 && field_0x19c != 2) return;
    if (alpha) SetWidgetAlpha_00140ED0(*alpha, field_0x5c_alpha);
    if (bar) SetWidgetAlpha_00140ED0(*bar, 0);
    if (field_0x0c == 0) {
        if (text) {
            text->string_id_0x60 = 0;
            text->text = text->text_capacity ? field_0x94.substr(0, text->text_capacity - 1) : field_0x94;
            text->x = 650; text->dirty_0x2b = 4;
        }
        field_0x28 = 0; field_0x0c = 1;
    } else if (field_0x28 >= 1 || field_0x194 == 650) {
        field_0x19c = 3;
    } else {
        field_0x28 += delta_seconds / 0.25f;
        const float t = field_0x28;
        const float x = t >= 1 ? field_0x194 :
            ((-2*(field_0x194-650)*t+3*(field_0x194-650))*t)*t+650;
        if (text) { text->x = static_cast<float>(static_cast<int>(x)); text->dirty_0x2b = 4; }
    }
}

std::int32_t bRandom(std::int32_t bound, std::uint32_t& state) {
    if (bound == 0) hp2::not_implemented("0x0025A3D0", "original zero-bound trap");
    const std::uint32_t old = state;
    const std::uint32_t a = old ^ 0x1d872b41u;
    const std::uint32_t b = a ^ (a >> 5);
    state = b ^ a ^ (b << 27);
    // Listing 0025A3E4 is DIVU, disproving Ghidra's signed modulo cast.
    return static_cast<std::int32_t>(old % static_cast<std::uint32_t>(bound));
}
void tGlobalBackgroundAnimation::Construct_002495F0() {
    field_0x00 = nullptr; field_0x70 = field_0x78 = field_0x30 = 0;
    field_0x10 = field_0x18 = 320; field_0x40 = field_0x48 = 240;
    for (auto& hash : field_0x7c) hash = 0;
    Add_00249730(StringHash_002534A8("GRID_BLUEPRINT1"));
    Add_00249730(StringHash_002534A8("GRID_BLUEPRINT2"));
}
void tGlobalBackgroundAnimation::Remove_00249790(std::uint32_t hash) {
    for (auto& slot : field_0x7c) if (slot == hash) slot = 0;
}
void tGlobalBackgroundAnimation::Add_00249730(std::uint32_t hash) {
    Remove_00249790(hash);
    for (auto& slot : field_0x7c) if (slot == 0) { slot = hash; return; }
}
void tGlobalBackgroundAnimation::Bind_00249A40(FeScreen* screen, const Random_0025A488& random,
                               const WidgetMeasure& measure) {
    field_0x00 = screen; port_measure_ = measure;
    if (screen) Reset_00249888(random);
}
void tGlobalBackgroundAnimation::Reset_00249888(const Random_0025A488& random) {
    auto* fading = field_0x00 ? FindWidget_00143D50(*field_0x00,
        StringHash_002534A8("FADING_BP_OBJECT"), 2) : nullptr;
    auto* master = field_0x00 ? FindWidget_00143D50(*field_0x00,
        StringHash_002534A8("MASTER_BP_OBJECT"), 2) : nullptr;
    if (fading && master) {
        if (!random) hp2::not_implemented("0x0025A488", "missing shared RNG input");
        auto range = [&](float lo, float hi) { return lo + static_cast<float>(random(10000))*0.0001f*(hi-lo); };
        field_0x10 = range(160, 165); field_0x40 = range(180, 300);
        field_0x18 = range(300, 400); field_0x48 = field_0x40; field_0x30 = 0;
        if (!port_measure_) hp2::not_implemented("0x00140638", "missing background widget measurement");
        // 0024AAE0 invokes the centered coordinate writers before changing texture.
        const auto metrics = port_measure_(*fading);
        fading->x = field_0x10 - metrics.width*0.5f;
        fading->y = field_0x40 - metrics.height*0.5f;
        SetWidgetAlpha_00140ED0(*fading, 0); SetWidgetAlpha_00140ED0(*master, 0);
        int count = 0; for (auto hash : field_0x7c) if (hash) ++count;
        int index = random(count);
        std::uint32_t hash = 0;
        for (auto slot : field_0x7c) if (slot && index-- == 0) { hash = slot; break; }
        fading->texture_hash = hash; fading->dirty_0x2b = 10;
    }
    field_0x70 = 0;
}
void tGlobalBackgroundAnimation::Update_00249AE8(float delta_seconds, const Random_0025A488& random) {
    field_0x78 += delta_seconds;
    // gp-0x676C at 0032F184 = 6.283 radians. GRID angle consumption is unknown.
    if (field_0x78 >= 6.283f) field_0x78 -= 6.283f;
    field_0x70 += delta_seconds;
    auto* fading = field_0x00 ? FindWidget_00143D50(*field_0x00,
        StringHash_002534A8("FADING_BP_OBJECT"), 2) : nullptr;
    auto* master = field_0x00 ? FindWidget_00143D50(*field_0x00,
        StringHash_002534A8("MASTER_BP_OBJECT"), 2) : nullptr;
    if (!fading || !master) return;
    if (field_0x70 < 1) SetWidgetAlpha_00140ED0(*fading, static_cast<int>(field_0x70*128));
    if (field_0x70 < 50) {
        field_0x30 += delta_seconds/50;
        const float t = field_0x30;
        const float d = field_0x18-field_0x10;
        const float x = t >= 1 ? field_0x18 : ((-2*d*t+3*d)*t)*t+field_0x10;
        if (!port_measure_) hp2::not_implemented("0x00140638", "missing background widget measurement");
        const auto metrics = port_measure_(*fading);
        fading->x = x - metrics.width*0.5f;
        fading->y = field_0x48 - metrics.height*0.5f;
        fading->dirty_0x2b = 4;
    }
    if (field_0x70 > 49 && field_0x70 < 50)
        SetWidgetAlpha_00140ED0(*fading, static_cast<int>((1-(field_0x70-49))*128));
    if (field_0x70 > 50) Reset_00249888(random);
}

// Main-menu action slice. Original TU is unknown; no invented source filename.
void Sub_00216638(int message, std::uint32_t item_hash, std::int32_t& dat_002FC070,
                  const ::port::frontend::ScreenActionServices& services) {
    if (message != 1) return;
    switch (item_hash) {
    case 0x52E039F3: dat_002FC070 = 1; services.request("Cops On Mode Select", true); break;
    case 0x52E039F4: dat_002FC070 = 0; services.request("Cops Off Mode Select", true); break;
    case 0x52E039F5: services.request("Options", true); break;
    default: break; // original switch ignores unmatched item hashes
    }
}


const char* Sub_0016EE90(int language,int index,bool demo) {
    static constexpr const char* logos[]={"MOVIES/INTRO/EALOGO_EN.AVI","MOVIES/INTRO/EALOGO_FR.AVI","MOVIES/INTRO/EALOGO_GE.AVI","MOVIES/INTRO/EALOGO_IT.AVI","MOVIES/INTRO/EALOGO_SP.AVI","MOVIES/INTRO/EALOGO_SW.AVI"};
    if(language<0||language>=6||index<0||index>=(demo?1:2)) throw std::out_of_range("0016EE90 movie table bounds");
    return index==0 ? logos[language] : language==1 ? "MOVIES/INTRO/NFSINTRO_FR.AVI" : "MOVIES/INTRO/NFSINTRO_EN.AVI";
}
::port::media::StartupResult TitleScreen::Sub_0016F000(bool skip,bool demo,int language,std::uint32_t ticks) {
    port_demo=demo;port_language=language;Field_0x08=0;Field_0x10=ticks;Field_0x14=0;Field_0x0c=skip?1:2;
    if(skip)return {::port::media::StartupAction::ShowTitle,nullptr,true};
    return {::port::media::StartupAction::PlayMovie,Sub_0016EE90(language,0,demo)};
}
::port::media::StartupResult TitleScreen::Sub_0016F1F0(bool done,std::uint32_t ticks) {
    ::port::media::StartupResult result;
    if(Field_0x0c==2 && done) {
        if(++Field_0x08==(port_demo?1:2)){Field_0x0c=1;Field_0x10=ticks;result.action=::port::media::StartupAction::ShowTitle;result.start_music=!port_demo;}
        else result={::port::media::StartupAction::PlayMovie,Sub_0016EE90(port_language,Field_0x08,port_demo)};
    }
    if(Field_0x0c!=2&&(Field_0x14==0||Field_0x14==0x7fffffffu))Field_0x14=ticks;
    return result;
}
bool TitleScreen::Sub_0016F4F8(std::uint32_t ticks) const {
    return Field_0x14!=0&&Field_0x14!=0x7fffffffu&&4.0f<static_cast<float>(static_cast<std::int32_t>(ticks-Field_0x14))*0.00025f;
}
::port::media::StartupResult TitleScreen::Sub_0016EEF8(std::uint32_t ticks) {
    if(Field_0x0c==2){
        if(Field_0x08==0)return {};
        Field_0x0c=1;Field_0x10=ticks;return {::port::media::StartupAction::ShowTitle,nullptr};
    }
    if(Field_0x0c==1&&Sub_0016F4F8(ticks)&&!port_demo){Field_0x10=ticks;return {::port::media::StartupAction::RequestMainMenu,nullptr};}
    return {};
}

} // namespace hp2::frontend


namespace hp2::frontend {
// ELF 0x0016F190; E-0016-V01; E-0028-C02; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0016f190.md.
void TitleScreen::Sub_0016F190() {
    hp2::not_implemented("0x0016F190", "TitleScreen::Sub_0016F190 (signature and full body unresolved)");
}
} // namespace hp2::frontend

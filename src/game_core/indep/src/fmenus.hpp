// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\fmenus.cpp`
// (string at 0x002B27F8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, and the `.hpp` extension is port-side -- only
// the `.cpp` path was recovered.
// `.cpp` path was recovered.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
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
namespace port::media {
enum class StartupAction { None, PlayMovie, ShowTitle, RequestMainMenu };
struct StartupResult {
    StartupAction action{StartupAction::None};
    const char* filename{};
    bool start_music{}; // port-side action for recovered 001AEAB0(1) callsites only
};
}

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
    std::uint32_t clip_hash_0x4c = 0; // E-0017-S02, clip-widget name hash; zero disables
    std::uint32_t texture_hash = 0; // +0x50 (TexturePack entry hash; 0 = untextured)
    std::uint8_t r = 128, g = 128, b = 128, a = 128; // RGBA bytes; PS2 alpha (0x80 = opaque)
    std::uint32_t text_capacity = 0; // +0x54 (text widgets): authored buffer length
    std::uint32_t text_align = 0;    // +0x58 (text widgets): a bitfield, not an enum --
                                     // FUN_00140FA8 tests bits 1, 2 and 4 of it when it rescales
                                     // a widget about its anchor. (E-0015-09)
    // +0x60 (text widgets): localised string id. FUN_00140D30 measures the widget using the
    // string FUN_0021E560 returns for this id whenever the id is non-zero and the localisation
    // global is set, falling back to the embedded placeholder at +0x64 otherwise. Screen 0 s
    // MENUITEM_1 carries 0x2727 (Championship) and OPTIONS MENUITEM_1 carries 0x186A1 (Audio).
    // (E-0015-10)
    std::uint32_t string_id_0x60 = 0;
    std::string text;                // +0x64 (text widgets): authored default string

    // +0x2b. The ELF treats this record as the live runtime widget node, not just a file record:
    // FUN_00143D50 walks these records as an intrusive linked list keyed on +0x08 (name hash) and
    // filtered on +0x28 (type), and both FUN_00140CA0 and FUN_00169BD0/FUN_00169CB0 stamp +0x2b
    // after mutating the record. 0x2b is unused by the parser but is written by the recovered menu
    // code, so it is modelled here with the same offset it has in the file. (E-0014-05)
    std::uint8_t dirty_0x2b = 0;
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
    int width = 640, height = 480;  // from the 0x10203 header (+0x18 u16, +0x1a u16)
    // +0x08 of the 0x10203 header: the screen's own name hash, under the same
    // StringHash_002534A8 the widget name hashes use. Unique across all 41 screens, and
    // consecutive for screens whose names differ only in their last character (screens 5/6 and
    // 8/9 differ by exactly 1). This is the handle the recovered code passes around as a
    // "document": FUN_0016FBB0 builds one with StringHash("LOADING_TRACK") and hands it to
    // FUN_00143D50. 18 of the 41 names are recovered -- see kRecoveredScreenNames. (E-0014-16)
    std::uint32_t name_hash_0x08 = 0;
    std::vector<FeWidget> widgets;
};

// Parses every screen in the first kScreenListChunkId container found in `buffer`.
// Returns an empty vector when the buffer holds no frontend screen list.
std::vector<FeScreen> parse_screens(const std::vector<std::uint8_t>& buffer);

inline constexpr std::uint32_t kFontChunkId = 0x00015010;

// Parses every font table in `buffer` (a bundle may carry several, and fonts are spread across
// FRONTEND/FRONTB.BUN, GLOBAL/GLOBALB.BUN and GLOBAL/INGAMEB.BUN).
std::vector<FeFont> parse_fonts(const std::vector<std::uint8_t>& buffer);

// =====================================================================================
// Recovered front-end menu runtime (fmenus.cpp, ELF 0x00140CA0 .. 0x0016CAB0)
// =====================================================================================
//
// Everything below is a translation of decompiled ELF code, not a design. Identifier naming
// follows AGENT.md R2: recovered original text is used verbatim (JOY_EVENT_*, "MENUITEM_%d",
// HIGHLIGHT_BAR, the menu sound names), and every symbol whose original identifier was NOT
// recovered carries the ELF entry address it was translated from.
//
// Call graph as recovered (E-0014-03):
//
//   FUN_0016CAB0 (manager joypad dispatch)
//     -> FUN_0016BAC0  popup/help menu handling            [NOT IMPLEMENTED, throws]
//     -> FUN_0016B140  for each group on the current screen
//          -> MenuGroup_0016AA00::HandleJoyEvent_0016AA00
//               -> MenuList_00169F78::HandleJoyEvent_0016A458
//                    -> Next_0016A5E0 / Previous_0016A6C8
//                         -> MenuThing_00169740::Highlight_00169968 / Unhighlight_001699F8
//                              -> SetStateBits_00169BD0 / ClearStateBits_00169CB0
//                                   -> SetWidgetColour_00140CA0 (stamps widget +0x2b)
//                    -> FUN_00169D98  slider item handling  [NOT IMPLEMENTED, throws]

// The original builds menu item widget names with this exact format string, stored at ELF
// 0x002B2760 and passed to sprintf by both FUN_0016A330 and FUN_00169F78. Verbatim. (E-0014-04)
inline constexpr const char* kMenuItemNameFormat_002B2760 = "MENUITEM_%d";

// Both FUN_0016A330 and FUN_00169F78 iterate indices 0..15 inclusive and stop; a screen can
// therefore declare at most 16 menu items, and index 0 is included. (E-0014-04)
inline constexpr int kMenuItemIndexLimit_0016A330 = 0x10;

// FUN_00169F78 finishes by resolving one fixed quad-widget hash, 0x27C5F4AB, into the menu list.
// The name behind that hash is recovered rather than guessed: hashing candidate strings with the
// recovered StringHash_002534A8 gives StringHash("HIGHLIGHT_BAR") == 0x27C5F4AB exactly, and no
// other tested name collides. (E-0014-06)
inline constexpr std::uint32_t kHighlightBarNameHash_00169F78 = 0x27c5f4abu;
inline constexpr const char* kHighlightBarName = "HIGHLIGHT_BAR";

// ------------------------------------------------------------------ string hash (FUN_002534A8)
// int StringHash(const char* s) { int h = -1; while (*s) h = h * 0x21 + *s++; return h; }
//
// Confirmed against the literal immediates the ELF front-end code passes around (E-0014-07):
// StringHash("POPUP_1") == 0xE0105663, "POPUP_2" == 0xE0105664, "POPUP_3" == 0xE0105665 and
// "ANIMATING_MENUITEM" == 0xAB671C9A, the four constants FUN_0016B478 hardcodes. It also
// reproduces 1531 of the 1549 widget name hashes stored in FRONTB.BUN; the 18 that differ are
// exactly the records whose stored name is 23 characters long, i.e. truncated by the 24-byte
// on-disk name field, which is itself evidence that the hash was computed before truncation.
std::uint32_t StringHash_002534A8(const char* text);

// Item state bits, held in item +0x08. FUN_00169740 initialises the field to 1;
// FUN_00169968/FUN_001699F8 set and clear bit 1; FUN_0016A5E0/FUN_0016A6C8/FUN_0016A160 skip
// items whose bit 0 is clear. (E-0014-08)
inline constexpr std::uint32_t kMenuThingStateEnabled_bit0 = 0x1u;
inline constexpr std::uint32_t kMenuThingStateHighlighted_bit1 = 0x2u;

// Front-end widget colour sets, read out of the ELF's initialised data through the gp register.
// gp == 0x003358F0 (entry point 0x00100008: "lui a0,0x33; addiu a0,a0,0x58f0; move gp,a0"), so
// the byte components FUN_00169BD0 ORs together resolve to concrete stored values.
//
// FUN_00169BD0/FUN_00169CB0 select on (state & 3):
//   1      -> the widget's own authored colour, snapshotted from widget +0x5C by FUN_00169818
//   3      -> kHighlightColour below, with the alpha byte forced to 0x80 by "lui a1,0x8000"
//   0 or 2 -> kDisabledColour below (both arms load the same three globals; checked in the
//             disassembly at 0x00169C1C and 0x00169C38, so it is not a decompiler artifact)
// Combined with bit 0 meaning "enabled", that gives: enabled and unhighlighted = authored
// colour, enabled and highlighted = highlight colour, disabled = disabled colour. (E-0014-09)
struct MenuColour_00169BD0 {
    std::uint8_t r = 0, g = 0, b = 0;
};
// gp-0x5330 / -0x532C / -0x5328 == 0x003305C0 / C4 / C8 == 0x80, 0x40, 0x00
inline constexpr MenuColour_00169BD0 kHighlightColour_gp_0x5330{0x80, 0x40, 0x00};
// gp-0x5324 / -0x5320 / -0x531C == 0x003305CC / D0 / D4 == 0x18, 0x18, 0x18
inline constexpr MenuColour_00169BD0 kDisabledColour_gp_0x5324{0x18, 0x18, 0x18};
inline constexpr std::uint8_t kHighlightAlpha_00169BD0 = 0x80;

// gp-0x533C == 0x003305B4 == 1 in the shipped image. FUN_0016A5E0/FUN_0016A6C8 branch on it:
// zero means "stop at the ends of the list", non-zero means "wrap around and skip items whose
// enabled bit is clear". The shipped value is non-zero, so the retail menus wrap. (E-0014-10)
inline constexpr bool kSkipDisabledAndWrap_gp_0x533C = true;

// ------------------------------------------------------------------- menu sounds (FUN_0016E038)
// FUN_0016E038(id) indexes a 16-byte-per-record table whose voice field sits at
// 0x002FB0A4 + id*0x10; the records start at 0x002FB0A8 as { u32 id, u32 flag, const char* name,
// u32 voice }. The names below are the original bSound event names, read verbatim from the table.
// Record 0 is all zeroes, i.e. "no sound". (E-0014-11)
//
// FUN_0016E038 also holds a retrigger guard: replaying the same voice within 34.0 units of the
// previous play is dropped. The unit of that 34.0 is NOT recovered.
const char* menu_sound_name_002FB0A8(int sound_id);
inline constexpr int kMenuSoundNone = 0;
inline constexpr int kMenuSoundMenuLeftRight_1 = 1;  // "MenuLeftRight", popup previous
inline constexpr int kMenuSoundMenuLeftRight_2 = 2;  // "MenuLeftRight", popup next
inline constexpr int kMenuSoundMenuUpDown_3 = 3;     // "MenuUpDown", list previous
inline constexpr int kMenuSoundMenuUpDown_4 = 4;     // "MenuUpDown", list next
inline constexpr int kMenuSoundMenuSelect_5 = 5;     // "MenuSelect", item activated
inline constexpr int kMenuSoundMenuCancel_6 = 6;     // "MenuCancel"
inline constexpr int kMenuSoundMenuStart_7 = 7;      // "MenuStart"
inline constexpr int kMenuSoundMenuWrong_8 = 8;      // "MenuWrong"

// ------------------------------------------------------- menu sound output (platform contract)
// FUN_0016E038 selects which bSound event to trigger, and that selection logic IS recovered (see
// menu_sound_name_002FB0A8). Actually producing the sound is PS2 audio hardware work behind
// FUN_00273618/FUN_00274080 and the game's bSound banks, which are not reversed; per AGENTS.md
// rule 7 that stays behind a platform contract rather than being faked in recovered code.
//
// PlayMenuSound_0016E038 therefore resolves the recovered id/name and hands it to the installed
// sink. The default sink records the request so the port can show which original menu sound the
// game would have played, without inventing audio.
struct MenuSoundRequest {
    int sound_id = 0;
    const char* name = nullptr; // verbatim original bSound event name, or nullptr for id 0
};
using MenuSoundSink = std::function<void(const MenuSoundRequest&)>;
void install_menu_sound_sink(MenuSoundSink sink);
void PlayMenuSound_0016E038(int sound_id);
// Last request handed to the sink, for tests and for the port-side on-screen read-out.
const MenuSoundRequest& last_menu_sound_request();

// -------------------------------------------------------------- widget lookup / colour mutation
// FUN_00143D50(screen, name_hash, type): walks the screen's intrusive widget list, matching
// +0x08 against name_hash and, when type is non-zero, +0x28 against type. Returns the record or
// null. The two fields it keys on are exactly the fields this file's parser reads at those
// offsets, which is what ties the recovered code to the recovered file format. (E-0014-05)
FeWidget* FindWidget_00143D50(FeScreen& screen, std::uint32_t name_hash, int type);

// TASK-0021 (E-0021-27): the three widget mutators the event tree drives at runtime. All three
// are tiny and were read in full, so they are translated verbatim rather than approximated.
//
// FUN_001449A8(screen, name_hash, string_id) -> FUN_00143D50(screen, hash, 1) then
// FUN_00140EA8(widget, id), which writes widget+0x60, sets the dirty byte at +0x2B to 5 and
// **clears the authored placeholder at +0x64**. That last part is why a screen stops showing
// its authored text once the game assigns a real string id -- the event tree's title is
// "This Will Change" until this runs.
void SetWidgetStringId_001449A8(FeScreen& screen, std::uint32_t name_hash,
                                std::uint32_t string_id);

// FUN_00144968(screen, name_hash, visible) -> FUN_00143D50(screen, hash, 0) then
// FUN_00140728(widget, visible), which writes widget+0x29.
void SetWidgetVisible_00144968(FeScreen& screen, std::uint32_t name_hash, bool visible);

// FUN_001454E8(screen, class_hash, visible): the same +0x29 write, for **every** widget whose
// class hash matches, walked with FUN_00143DD8.
void SetClassVisible_001454E8(FeScreen& screen, std::uint32_t class_hash, bool visible);


// FUN_00140CA0(widget, rgb): replaces the low 24 bits of widget +0x5C and leaves the top byte
// (PS2 alpha) alone, then stamps +0x2b = 7 when the value actually changed. (E-0014-05)
void SetWidgetColour_00140CA0(FeWidget& widget, const MenuColour_00169BD0& colour);

// ---------------------------------------------------------------------- menu callback protocol
// The 16-byte structure every menu callback receives (memset to 0, then filled). Field 0 carries
// an item name hash for item-level messages and the owning screen handle for group-level ones;
// the callback writes back through result_0x08 and sound_id_0x0C, and the caller then plays
// sound_id_0x0C through FUN_0016E038. (E-0014-12)
struct MenuEvent_0016A458 {
    std::uint32_t field_0x00 = 0;
    std::int32_t pad_index_0x04 = 0;
    std::int32_t result_0x08 = 0;
    std::int32_t sound_id_0x0C = 0;
};

// Message codes, as the numbers the recovered code actually passes. No original names for these
// were recovered, so the enumerators carry only the number plus a comment naming the call site.
enum MenuMessage : int {
    kMenuMessage0x02 = 0x02, // per-item refresh      (FUN_00169858, from FUN_0016A160)
    kMenuMessage0x04 = 0x04, // item gained highlight (FUN_00169968)
    kMenuMessage0x05 = 0x05, // item lost highlight   (FUN_001699F8)
    kMenuMessage0x08 = 0x08, // list moved previous   (FUN_0016A458, SCROLL_UP / ANALOG_UP)
    kMenuMessage0x09 = 0x09, // list moved next       (FUN_0016A458, SCROLL_DOWN / ANALOG_DOWN)
    kMenuMessage0x0B = 0x0b, // menu created          (FUN_0016A858; result_0x08 != -1 overrides
                             //                        the initial index, as result - 1)
    kMenuMessage0x0D = 0x0d, // JOY_EVENT_START on the group   (FUN_0016AA00; result 1 consumes)
    kMenuMessage0x0E = 0x0e, // JOY_EVENT_SELECT on the group  (FUN_0016AA00)
    kMenuMessage0x0F = 0x0f, // JOY_EVENT_MENU_L1 on the group (FUN_0016AA00)
    kMenuMessage0x10 = 0x10, // JOY_EVENT_MENU_R1 on the group (FUN_0016AA00)
};

using MenuCallback = std::function<void(int message, MenuEvent_0016A458&)>;

class MenuList_00169F78;
class MenuGroup_0016AA00;

// ------------------------------------------------------------------- menu item (FUN_00169740)
// Field offsets are the original ones. +0x00/+0x04 are the intrusive list links the owning
// MenuList_00169F78 writes; they are modelled as real links so the recovered traversal order and
// its sentinel behaviour are reproduced rather than reinterpreted. (E-0014-13)
class MenuThing_00169740 {
public:
    MenuThing_00169740* next_0x00 = nullptr;
    MenuThing_00169740* prev_0x04 = nullptr;
    std::uint32_t state_0x08 = 0;
    std::uint32_t name_hash_0x0C = 0;
    std::int32_t index_0x10 = 0;
    std::uint32_t authored_colour_0x1C = 0;
    std::int32_t pad_index_0x20 = -1;
    FeScreen* screen_0x24 = nullptr;

    // FUN_00169740(pool, owner_list, name_hash, index): sets state to 1, copies the owner list's
    // pad index (+0x18) and screen (+0x14), then calls FUN_00169818.
    void Construct_00169740(MenuList_00169F78* owner, std::uint32_t name_hash, std::int32_t index);
    // FUN_00169818: resolve the type-1 widget by name hash and snapshot its colour from +0x5C.
    void Resolve_00169818();

    bool enabled() const { return (state_0x08 & kMenuThingStateEnabled_bit0) != 0; }
    bool highlighted() const { return (state_0x08 & kMenuThingStateHighlighted_bit1) != 0; }

    // FUN_00169BD0 / FUN_00169CB0: OR or AND-NOT the state bits, then re-apply the colour implied
    // by (state & 3) to the widget and stamp its dirty byte.
    void SetStateBits_00169BD0(std::uint32_t bits);
    void ClearStateBits_00169CB0(std::uint32_t bits);
    // The shared tail of both: re-select the colour from (state & 3) and write it to the widget.
    void ApplyStateColour_00169BD0();

    // FUN_00169968 / FUN_001699F8 / FUN_00169858.
    void Highlight_00169968(const MenuCallback& callback);
    void Unhighlight_001699F8(const MenuCallback& callback);
    void Refresh_00169858(const MenuCallback& callback);
};

// ------------------------------------------------------------------- menu list (FUN_00169F78)
// A circular doubly linked list whose head doubles as the sentinel: FUN_00169F78 initialises
// +0x00 and +0x04 to point at the list object itself, and the traversal helpers compare against
// that address to detect the ends. Modelled with the same links for that reason. (E-0014-13)
class MenuList_00169F78 {
public:
    MenuThing_00169740* next_0x00 = nullptr;  // first item, or sentinel() when empty
    MenuThing_00169740* prev_0x04 = nullptr;  // last item, or sentinel() when empty
    MenuThing_00169740* current_0x08 = nullptr;
    MenuCallback callback_0x0C;
    MenuGroup_0016AA00* owner_0x10 = nullptr;
    FeScreen* screen_0x14 = nullptr;
    std::int32_t pad_index_0x18 = -1;
    std::int32_t count_0x1C = 0;
    FeWidget* highlight_bar_0x20 = nullptr;

    MenuList_00169F78() = default;
    MenuList_00169F78(const MenuList_00169F78&) = delete;
    MenuList_00169F78& operator=(const MenuList_00169F78&) = delete;

    // FUN_00169F78(list, owner_group, callback, initial_index, name_format): for i in 0..15,
    // formats name_format with i, hashes it, and appends an item for every type-1 widget the
    // screen actually declares under that name. Then sets the current index and resolves
    // HIGHLIGHT_BAR as a type-2 widget.
    void Construct_00169F78(MenuGroup_0016AA00* owner, MenuCallback callback,
                            std::int32_t initial_index,
                            const char* name_format = kMenuItemNameFormat_002B2760);

    // FUN_002503F8(list, index): the n-th item, or the sentinel when index is out of range.
    MenuThing_00169740* Nth_002503F8(std::int32_t index);
    // FUN_0016A5B0(list, name_hash): walk the circular list from the head comparing item +0x0C,
    // returning the first match or 0. The head is the sentinel, so an empty list returns 0
    // without a comparison. (E-0025-L05)
    MenuThing_00169740* FindByHash_0016A5B0(std::uint32_t name_hash);
    // FUN_0016A0F0(list, index).
    void SetCurrentIndex_0016A0F0(std::int32_t index);
    // FUN_0016A130(list): position of the current item; 0 when the list is empty or the current
    // item is not reachable from the head (the original returns 0 in both cases).
    std::int32_t CurrentIndex_0016A130() const;
    // FUN_0016A160(list): message 0x02 to every item, then move the current pointer to the first
    // enabled item at or after it and highlight that.
    void RefreshAll_0016A160();
    // FUN_0016A5E0(list) / FUN_0016A6C8(list).
    void Next_0016A5E0();
    void Previous_0016A6C8();
    // FUN_0016A458(list, pad, joy_event, pressed).
    void HandleJoyEvent_0016A458(std::int32_t pad_index, std::int32_t joy_event, bool pressed);

    MenuThing_00169740* sentinel() { return reinterpret_cast<MenuThing_00169740*>(this); }
    const MenuThing_00169740* sentinel() const {
        return reinterpret_cast<const MenuThing_00169740*>(this);
    }
    std::int32_t count() const { return count_0x1C; }
    std::vector<MenuThing_00169740*> items();

private:
    std::vector<std::unique_ptr<MenuThing_00169740>> storage_;
};

// FUN_0016D2A8(class_name, name_hash, screen, mode): the generic "turn one menu item on or
// off" helper. The original resolves the screen group from the class name with FUN_0016C8C8
// and FUN_0016B340 before looking the item up with FUN_0016A5B0; the port is handed the list
// and the screen the caller already owns, so only the item/widget effects are modelled here.
// Mode is the original's fourth argument:
//   0 or 2 -> FUN_00169CB0(item, 1, 0), clearing the enabled bit
//   else   -> FUN_00169BD0(item, 1, 0), setting it
//   2      -> additionally FUN_00144968(screen, hash, 0), hiding the widget
//   1      -> additionally FUN_00144968(screen, hash, 1), showing it
// A missing list or item returns without touching the widget, exactly as the original's two
// null checks do. The original translation unit for this address is not pinned
// (docs/ORIGINAL_SOURCE_TREE.md); it lives here because it only operates on this file's menu
// framework. (E-0025-L04)
void Sub_0016D2A8(MenuList_00169F78* list, FeScreen& screen, std::uint32_t name_hash, int mode);

// --------------------------------------------------------------- screen menu group (FUN_0016AA00)
// One node of the per-screen list FUN_0016B140 walks (screen +0x08). Holds the pad filter the
// original compares against (-1 means any pad), the screen handle the items resolve against, the
// callback, and the lazily created menu list. (E-0014-14)
class MenuGroup_0016AA00 {
public:
    MenuList_00169F78* child_0x08 = nullptr;
    std::int32_t initial_index_0x0C = 0;
    std::int32_t pad_index_0x10 = -1;
    FeScreen* screen_0x14 = nullptr;
    MenuCallback callback_0x18;

    // FUN_0016A858(group): creates the menu list on first use (message 0x0B may override the
    // initial index through result_0x08), sets the current index and refreshes.
    void EnsureMenu_0016A858();
    // FUN_0016AA00(group, pad, joy_event, pressed).
    void HandleJoyEvent_0016AA00(std::int32_t pad_index, std::int32_t joy_event, bool pressed);

private:
    std::unique_ptr<MenuList_00169F78> owned_list_;
};

// FUN_00169D98(item, pad, joy_event, pressed, list_callback): the per-item handler the menu list
// forwards to. Translated in full: JOY_EVENT_SCROLL_LEFT/_MENU_ANALOG_LEFT raise message 6,
// SCROLL_RIGHT/MENU_ANALOG_RIGHT raise message 7, START and MENU_SELECT raise message 1 with
// sound 5 ("MenuSelect") -- that is the original item-activation path for list menus -- and
// MENU_SQUARE raises message 10. Everything else is ignored. Field +0x08 of the event carries the
// joy event id. (E-0014-15)
void Sub_00169D98(MenuThing_00169740& item, std::int32_t pad_index, std::int32_t joy_event,
                  bool pressed, const MenuCallback& list_callback);
inline constexpr int kMenuMessage0x01 = 0x01; // item activated  (FUN_00169D98, START/MENU_SELECT)
inline constexpr int kMenuMessage0x06 = 0x06; // item adjust left  (FUN_00169D98)
inline constexpr int kMenuMessage0x07 = 0x07; // item adjust right (FUN_00169D98)
inline constexpr int kMenuMessage0x0A = 0x0a; // MENU_SQUARE on the item (FUN_00169D98)

// =====================================================================================
// Recovered but not translated. AGENT.md R3: these are reachable code paths of the original
// front-end whose behaviour is known to exist but is not implemented here, so they raise
// hp2::NotImplemented instead of degrading into something plausible.
// =====================================================================================

// FUN_0016BAC0: the pop-up / help menu joypad handler the manager (FUN_0016CAB0) offers events to
// before the screen's own menus. Decompiled: it remaps JOY_EVENT_SCROLL_LEFT/_RIGHT onto
// previous/next for horizontal pop-ups (flag bit 0), moves a plain integer selection index at
// +0x5C with wrap-around, dispatches one of three handlers at +0x24/+0x28/+0x2C on
// START/MENU_SELECT and a fourth at +0x30 on MENU_BACK, and plays sounds 1 and 2
// ("MenuLeftRight"). Not translated: the pop-up object's own construction (FUN_0016B478 binds
// POPUP_1/POPUP_2/POPUP_3) and the meaning of its remaining flag bits are not recovered, so the
// handler table cannot be populated without inventing it.
[[noreturn]] void Sub_0016BAC0(std::int32_t pad_index, std::int32_t joy_event, bool pressed);

// FUN_0016FBB0: global FMenus animation/loading widget constructor. Its full global binding
// lifecycle remains unimplemented; the recovered main-menu pulse is separately implemented
// by MenuHighlightAnim_00248870 (E-0015-03..09). This probe tests the missing global constructor,
// not whether the main-menu animation exists.
[[noreturn]] void Sub_0016FBB0();

// Runtime localised strings. The shipped text lives in LANGUAGES/FE_*.BIN, whose format has not
// been parsed; what the layout records carry at +0x64 is the author-time placeholder. Screens
// whose placeholder happens to equal the shipped text (the mode-select menu, Options) look
// correct by coincidence, which is exactly why this must not silently fall back.
[[noreturn]] std::string localised_text(std::uint32_t string_id);

// ------------------------------------------------------------------- screen (document) names
// The layout file stores no screen-name string, only the hash at 0x10203 +0x08, so the names
// below were recovered by hashing candidate strings with StringHash_002534A8 and keeping only
// exact 32-bit matches against the stored hashes. Each entry is therefore a confirmed name, not
// a label: "NEW_MAIN_MENU" for screen 0 independently confirms the content-based guess TASK-0013
// had to leave as an inference, and TITLE_SCREEN / OPTIONS / JOYSTICK_CONFIG land on exactly the
// three screens that task had visually identified as the title, options and controller pages.
// 23 of the 41 screens are still unnamed; their stored hashes are listed in
// docs/FRONTEND_MENU_INTERACTION.md so the search can be continued. (E-0014-17)
struct RecoveredScreenName { int screen_index; const char* name; };
const RecoveredScreenName* recovered_screen_names(std::size_t& count);

// Index of the screen whose stored name hash matches, or -1. This is what the original does: it
// hashes a name and uses the hash as the document handle (FUN_0016FBB0 -> FUN_00143D50).
int screen_index_for_name_hash(const std::vector<FeScreen>& screens, std::uint32_t name_hash);
int screen_index_for_screen_name(const std::vector<FeScreen>& screens, const char* screen_name);

// =====================================================================================
// Recovered front-end widget geometry + the highlight animation (TASK-0015)
// =====================================================================================
//
// The original drives its menu visuals by mutating the same widget records the renderer walks.
// These helpers are the recovered mutators, named after the ELF functions they translate.

// FUN_00140588 / FUN_001405E0 need the rendered size of a widget, which for a text widget is a
// font measurement (FUN_00140D30 sums the glyph advances of the widget's string and multiplies
// by +0x38; FUN_00140CD8 multiplies the font's line height by +0x3C) and for a quad is the
// texture size. Fonts and textures live on the port's asset side, so the measurement is supplied
// through this seam rather than duplicated into recovered code. This struct is port-side; the
// arithmetic that consumes it is not.
struct WidgetMetrics {
    float width = 0.0f;
    float height = 0.0f;
};
using WidgetMeasure = std::function<WidgetMetrics(const FeWidget&)>;

// FUN_00144280(doc, hash, &x0, &y0, &x1, &y1): the widget's bounding box, where x1/y1 are
// x + FUN_00140588(widget) and y + FUN_001405E0(widget). (E-0015-06)
struct WidgetRect_00144280 {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    bool valid = false;
};
// ELF 00145F30 /00143470: text clipping requires +58 bit0x10 and has
// left edge mask.x+4; quads use the mask bounds as their temporary scissor.
// A missing clip widget disables clipping, as in the original.
WidgetRect_00144280 GetWidgetClip_00145F30(FeScreen& screen, const FeWidget& widget,
                                        const WidgetMeasure& measure);

// 00249DE0 with Audio ctor001EF3C8's exact 26-element widget list.
// Per-widget 00249C78 paths retain velocity and use duration0.5, no group guess.
class Type_00249DE0 {
public:
    void Construct_00249DE0(FeScreen& screen,std::uint32_t top_hash,std::uint32_t bottom_hash,
        std::uint32_t up_hash,std::uint32_t down_hash,std::uint32_t clip_hash);
    void Add_00249FF8(const std::vector<std::uint32_t>& widget_hashes);
    void Sub_001EF3C8(FeScreen& screen);
    void Sub_001F33B8(FeScreen& screen);
    void Handle_0024A480(int message, std::uint32_t item_hash);
    void Update_0024A1A8(float delta_seconds);
    std::uint32_t Sub_0024A368() const; // fixed viewport mask for selection bars
    int field_0x34 = 0; // original total-row count, assigned by each screen constructor
private:
    struct Type_00249C78 {
        std::size_t field_0x3c = 0;
        float field_0x0c = 0, field_0x10 = 0, field_0x14 = 0;
        float field_0x1c = 0, field_0x20 = 0, field_0x24 = 0, field_0x28 = 0;
        float field_0x2c = 0;
        int field_0x38 = 0;
    };
    FeScreen* field_0x00 = nullptr;
    std::vector<Type_00249C78> field_0x04;
    float field_0x0c = 0, field_0x10 = 0, field_0x24 = 0;
    int field_0x14 = -1;
    std::uint32_t field_0x28=0,field_0x2c=0,field_0x30=0;
};
// E-0017-S04: selector arrows follow the bound VALUE widget's integer
// bounding-box vertical centre. Zero target hides both arrows immediately.
class Type_0024A690 {
public:
    void Construct_0024A690(FeScreen& screen, std::uint32_t left_hash, std::uint32_t right_hash);
    void Bind_0024A860(std::uint32_t target_hash);
    void Update_0024A730(float delta_seconds, const WidgetMeasure& measure);
private:
    FeScreen* field_0x00=nullptr;
    std::uint32_t field_0x04=0,field_0x08=0,field_0x0c=0;
    float field_0x1c=0;
};
WidgetRect_00144280 GetWidgetRect_00144280(FeScreen& screen, std::uint32_t name_hash,
                                            const WidgetMeasure& measure);

// FUN_00144458(doc, hash, y) -> FUN_00140A48: set the widget's Y, truncated to an int by the
// caller (FUN_001B9AE8 passes `(int)y`).
void SetWidgetY_00144458(FeScreen& screen, std::uint32_t name_hash, int y);

// FUN_00140ED0(widget, alpha): replace the top byte of +0x5C, stamp +0x2b = 6. (E-0015-05)
void SetWidgetAlpha_00140ED0(FeWidget& widget, int alpha);

// FUN_001410A0(dst, src): copy 0x40 bytes from src+0x28 over dst+0x28, preserving dst+0x2C, then
// stamp dst+0x2b = 8. That range covers type, visible, layer, x, y, scale, the texture hash, the
// colour and the string id, so the destination becomes a visual duplicate of the source.
// (E-0015-04)
void CopyWidgetTransform_001410A0(FeWidget& dst, const FeWidget& src);

// FUN_00140FA8(scale, widget): set both scale fields, moving x and y first so the widget grows
// about the anchor its +0x58 bits select. Bit 1 (value 2) keeps the centre on both axes, bit 2
// (value 4) keeps the vertical centre, bit 0 (value 1) keeps the right/bottom edge, and no bit
// set is a plain top-left anchor that only writes +0x3C before +0x38. (E-0015-09)
void SetWidgetScale_00140FA8(FeWidget& widget, float scale, const WidgetMetrics& metrics);

// ------------------------------------------------------------- highlight animation constants
// All read out of the ELF's initialised data with gp = 0x003358F0. (E-0015-03)
inline constexpr float kAnimFrameToSeconds_gp_0x6790 = 0.00025f; // frame counter -> seconds
inline constexpr float kAnimDuration_gp_0x3ADC = 0.5f;           // one pulse lasts 0.5 s
inline constexpr float kAnimQuarterTurn_gp_0x678C = 1.57075f;    // pi/2, the sine argument scale
inline constexpr float kAnimScaleStart_gp_0x3AD8 = 1.0f;         // x1 the target's own scale
inline constexpr float kAnimScaleEnd_gp_0x3AD4 = 2.0f;           // x2 the target's own scale
inline constexpr float kAnimAlphaStart_gp_0x3AD0 = 128.0f;       // 0x80, PS2 opaque
inline constexpr float kAnimAlphaEnd_gp_0x3ACC = 0.0f;
inline constexpr float kAnimDefaultPeriod_gp_0x3AC8 = 4.0f;      // FUN_00248870's default
inline constexpr float kAnimFMenusPeriod_0016FBB0 = 2.0f;        // FUN_0016FBB0 overrides it
inline constexpr std::uint32_t kAnimGhostColour_gp_0x3F08 = 0x00000080u; // written by FUN_00140CA0
// FUN_00216880: MI_UNDERSCORE lands 3 below the mask's bottom edge and MI_OVERSCORE 6 above its
// top edge (gp-0x3EF0 = -3, gp-0x3EF4 = 6, used as `y1 + (-3)` and `y0 - 6`). (E-0015-07)
inline constexpr float kUnderscoreOffset_gp_0x3EF0 = -3.0f;
inline constexpr float kOverscoreOffset_gp_0x3EF4 = 6.0f;

// --------------------------------------------------- "FMenus Highlight Animation" (FUN_00248870)
// The object the ELF names "FMenus Highlight Animation" (pool-name string 0x002B3000). It makes
// the highlighted menu entry emit a ghost: a template widget (ANIMATING_MENUITEM on the screens
// that declare one, GLOBAL_ANIMATING_MENUITEM for the manager-owned instance) is copied onto the
// highlighted item, then scaled from the item's own scale to twice that while its alpha falls
// from 0x80 to 0, over 0.5 s, repeating every 2 s.
//
// The easing is the original's: FUN_0025A5E0 converts radians to a 16-bit angle with the factor
// 10430.4 == 65536 / (2*pi) and indexes a sine table, so `sin(elapsed * (pi/2) / 0.5)` runs 0 -> 1
// across the pulse. (E-0015-02)
class MenuHighlightAnim_00248870 {
public:
    FeScreen* screen_0x00 = nullptr;
    std::uint32_t target_hash_0x04 = 0;   // the highlighted item, set by FUN_002488D0
    float start_time_0x08 = 0.0f;         // seconds; the ELF stores a 4000 Hz frame counter
    std::uint32_t template_hash_0x0C = 0; // ANIMATING_MENUITEM
    std::int32_t colour_0x10 = -1;        // -1 means "do not touch the ghost's RGB"
    float scale_start_0x14 = kAnimScaleStart_gp_0x3AD8;
    float scale_end_0x18 = kAnimScaleEnd_gp_0x3AD4;
    float period_0x1C = kAnimDefaultPeriod_gp_0x3AC8;

    // FUN_00248870(obj, doc, template_hash, colour).
    void Construct_00248870(FeScreen* screen, std::uint32_t template_hash, std::int32_t colour,
                            float now_seconds);
    // FUN_002488D0(obj, target_hash): retarget, restart the clock, and snapshot the target's
    // scale into the start/end scales (the ELF reads +0x38 twice through FUN_001444E8 and
    // multiplies by 1.0 and 2.0).
    void Bind_002488D0(std::uint32_t target_hash, float now_seconds);
    // FUN_00248970(obj): one frame.
    void Update_00248970(float now_seconds, const WidgetMeasure& measure);

    bool active() const { return active_; }
    float elapsed(float now_seconds) const { return now_seconds - start_time_0x08; }

private:
    bool active_ = false;
};

// FUN_00216880(screen_instance, mask_hash, string_id): place MI_UNDERSCORE and MI_OVERSCORE
// against the highlighted item's MI_<n>_MASK quad. The original also hands `string_id` to the
// scrolling-text object; that part is not implemented here (see Sub_00249348).
void PlaceSelectionBars_00216880(FeScreen& screen, std::uint32_t mask_hash,
                                 const WidgetMeasure& measure);
// E-0017-S03 /001F4E68 common Options subclass selection: unlike main,
// underline is exactly mask bottom, overscore is mask top-6.
void Sub_001F4E68(FeScreen& screen, std::uint32_t mask_hash,
                                 const WidgetMeasure& measure);
inline constexpr const char* kUnderscoreName = "MI_UNDERSCORE";  // hash 0x32C91DAE
inline constexpr const char* kOverscoreName = "MI_OVERSCORE";    // hash 0xC671680C
inline constexpr const char* kMaskNameFormat_002BEFA0 = "MI_%d_MASK";       // ELF 0x002BEFA0
inline constexpr const char* kItemGroupNameFormat_002BEF88 = "MENUITEM_%d_GROUP"; // 0x002BEF88

// FUN_00249348: the scrolling-text / white-bar cross-fade state machine driven from each screen's
// per-frame message 0x12. Decompiled: states 0..3, cross-fading SCROLLING_TEXT against
// WHITEOUT_BAR over 0.6 s and holding for 0.5 s. The former claim '650 px/s'
// is disproved by E-0016-S02: 650 is initial X, duration is 0.25 s.
// Legacy unbound boundary remains a throw. The bound state is Type_002491B0 below.
[[noreturn]] void Sub_00249348();

// E-0016-S02: FUN_002491B0/00249518/00249348. Host delta replaces PS2
// clock production. The path is zero-velocity Hermite from X=650 to authored X
// in 0.25 seconds; 650 is a position, not a scrolling speed.
class Type_002491B0 {
public:
    void Construct_002491B0(FeScreen& screen);
    // ELF 0x002491B0; E-0022-R01. Screen-owned hashes are constructor operands,
    // including EVENT_TITLE_BAR for the event tree (0x001F96B8).
    void Construct_002491B0(FeScreen& screen, std::uint32_t text_hash,
                           std::uint32_t alpha_hash, std::uint32_t bar_hash);
    void Bind_00249518(const std::string& text);
    void Update_00249348(float delta_seconds);
private:
    FeScreen* field_0x00 = nullptr;
    std::uint32_t field_0x04 = 0x75980cf0, port_alpha_hash = 0x75980cf0,
                  port_bar_hash = 0x70a404ec;
    float field_0x08 = 0, field_0x198 = 0;
    int field_0x0c = 0, field_0x19c = 0;
    float field_0x194 = 0, field_0x28 = 0;
    std::uint8_t field_0x5c_alpha = 0, field_0x78_alpha = 0;
    std::string field_0x94;
};

// E-0016-S03. Original global RNG seed's initialized word is 0x12345678
// at 00332068; other original callers advance it. Host owns the shared state.
std::int32_t Sub_0025A3D0(std::int32_t bound, std::uint32_t& state);
class Type_002495F0 {
public:
    using Random_0025A488 = std::function<std::int32_t(std::int32_t)>;
    void Construct_002495F0();
    void Bind_00249A40(FeScreen* screen, const Random_0025A488& random,
                       const WidgetMeasure& measure);
    void Add_00249730(std::uint32_t texture_hash);
    void Remove_00249790(std::uint32_t texture_hash);
    void Reset_00249888(const Random_0025A488& random);
    void Update_00249AE8(float delta_seconds, const Random_0025A488& random);
private:
    FeScreen* field_0x00 = nullptr;
    std::uint32_t field_0x7c[8] = {};
    float field_0x70 = 0, field_0x78 = 0;
    float field_0x10 = 320, field_0x40 = 240, field_0x18 = 320;
    float field_0x48 = 240, field_0x30 = 0;
    WidgetMeasure port_measure_; // host texture/font dimension seam
};

// FUN_0016A330(name_hash): brute-force reverse lookup of a "MENUITEM_%d" index over 0..15,
// returning 0 when nothing matches (the original's own fallback, not a port-side choice).
int MenuItemIndexFromNameHash_0016A330(std::uint32_t name_hash);


// TitleScreen spelling confirmed by the named allocation in 0016F440 followed by
// constructor 0016F000 (label 002B2F68); E-0028-C02. Original method names unknown.
// Existing host projection of title instance fields. Actual hardware callbacks,
// texture-widget binding, attract-mode/reset and input filtering stay host seams.
// ELF SLUS_203.62 SHA256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
struct TitleScreen {
    std::int32_t Field_0x08{}; // movie index
    std::int32_t Field_0x0c{}; // observed values 1 and 2
    std::uint32_t Field_0x10{}, Field_0x14{};
    bool port_demo{};
    int port_language{};
    // 0016F000 / E-0016-V01. Parameter skip_movies projects gp-0x59FC.
    ::port::media::StartupResult Sub_0016F000(bool skip_movies, bool demo, int language, std::uint32_t ticks);
    // 0016F1F0 / E-0016-V01. movie_done comes from recovered 0016EE18's host playback seam.
    ::port::media::StartupResult Sub_0016F1F0(bool movie_done, std::uint32_t ticks);
    // 0016EEF8 message 1 / E-0016-V01. caller must supply menu-select event only.
    ::port::media::StartupResult Sub_0016EEF8(std::uint32_t ticks);
    // 0016F4F8 / E-0016-V01. Strict >4 seconds and the two original timestamp sentinels.
    bool Sub_0016F4F8(std::uint32_t ticks) const;

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0016F190; E-0016-V01; E-0028-C02; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0016f190.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0016F190();
};
// 0016EE90/0016EEE0 and table 002FB750/002FB7D0, E-0016-V01.
const char* Sub_0016EE90(int language, int movie_index, bool demo);

} // namespace hp2::frontend

namespace port::frontend {
// Port projection of confirmed per-class scrolling attachments and message-4
// numeric IDs. E-0016-S02/S05. Unknown classes retain an explicit unknown result.
struct HintBinding {
    bool attachment_known = false;
    bool attached = false;
    std::uint32_t string_id = 0;
};
inline HintBinding hint_binding(std::string_view class_name, std::uint32_t item_hash) {
    if (class_name == "Language Options" || class_name == "Language Options Pre FE" ||
        class_name == "Credits") return {true, false, 0};
    const auto item = item_hash - 0x52e039f2u;
    if (class_name == "New Main Menu") {
        constexpr std::uint32_t ids[]{0x2720,0x271f,0x2719};
        return {true, true, item >= 1 && item <= 3 ? ids[item-1] : 0};
    }
    if (class_name == "Cops On Mode Select") {
        constexpr std::uint32_t ids[]{0x2721,0x271e,0x271c,0x271d,0x2719};
        return {true, true, item >= 1 && item <= 5 ? ids[item-1] : 0};
    }
    if (class_name == "Cops Off Mode Select") {
        constexpr std::uint32_t ids[]{0x2716,0x2717,0x2718,0x2719};
        return {true, true, item >= 1 && item <= 4 ? ids[item-1] : 0};
    }
    if (class_name == "Options")
        return {true, true, item >= 1 && item <= 7 ? 0x186b3 + item : 0};
    if (class_name == "Display Options") {
        constexpr std::uint32_t ids[]{0x24cac,0x24cad,0x24cae,0x24caf,0x24cb1,0x24cb2,0x24cb3,0x24cb4,0x24cb5};
        return {true,true,item >= 1 && item <= 9 ? ids[item-1] : 0};
    }
    if (class_name == "Audio Options") {
        if (item_hash == 0xaee77883u) return {true, true, 0x24bed};
        if (item_hash == 0xaee77884u) return {true, true, 0x24bee};
        return {true, true, item >= 1 && item <= 9 ? 0x24be3 + item : 0};
    }
    if (class_name == "Joystick PreConfig")
        return {true, true, item >= 1 && item <= 6 ? 0x24c47 + item : 0};
    if (class_name == "NameEntry")
        return {true, true, item >= 1 && item <= 4 ? 0x24d0f + item : 0};
    return {};
}
}

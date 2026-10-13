// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cstdint>

// Recovered joypad event enum. Every identifier in `JoyEvent` below is the original developers'
// own ASCII name, read verbatim out of the authorized ELF (`SLUS_203.62`), not a port-side label.
//
// Evidence (E-0014-01): the ELF holds an explicit `{ uint32 value, const char* name }` table at
// 0x002FA2D0, 8 bytes per record, ascending, whose name pointers walk the contiguous string block
// at 0x002B0218.. ("JOY_EVENT_NULL", "JOY_EVENT_TYPE_CHANGED", ...). Reading the table gives the
// value/name pairing directly; no ordering had to be assumed.
//
// Cross-check (E-0014-02): the values below are used as raw switch cases by the recovered
// front-end menu code, and the groupings match the names exactly. `FUN_00169D98` groups
// 5 with 0x12 and 6 with 0x13 -- i.e. SCROLL_LEFT with MENU_ANALOG_LEFT and SCROLL_RIGHT with
// MENU_ANALOG_RIGHT. `FUN_0016A458` groups 3 with 0x14 and 4 with 0x15 -- SCROLL_UP with
// MENU_ANALOG_UP and SCROLL_DOWN with MENU_ANALOG_DOWN. Both pairings are only coherent under
// this exact table, which is independent confirmation that the table is the dispatch enum.
//
// Unresolved: the original translation unit that declares this enum has not been identified. The
// ELF does carry the profiler label "GenerateJoyEvents()" (0x002B01C8) next to "Joystick",
// "EventNode" and "EventNodeSlotPool" pool names, so a joypad-event source file exists, but no
// `__FILE__` string has been tied to it. Header placement here is therefore port-side; the
// identifiers are not.
namespace hp2::input {

enum JoyEvent : std::int32_t {
    JOY_EVENT_NULL = 0,
    JOY_EVENT_TYPE_CHANGED = 1,
    JOY_EVENT_EXIT_DEMO_DISC = 2,
    JOY_EVENT_SCROLL_UP = 3,
    JOY_EVENT_SCROLL_DOWN = 4,
    JOY_EVENT_SCROLL_LEFT = 5,
    JOY_EVENT_SCROLL_RIGHT = 6,
    JOY_EVENT_START = 7,
    JOY_EVENT_SELECT = 8,
    JOY_EVENT_MENU_SELECT = 9,
    JOY_EVENT_MENU_CANCEL = 10,
    JOY_EVENT_MENU_BACK = 11,
    JOY_EVENT_MENU_HELP = 12,
    JOY_EVENT_MENU_L1 = 13,
    JOY_EVENT_MENU_L2 = 14,
    JOY_EVENT_MENU_R1 = 15,
    JOY_EVENT_MENU_R2 = 16,
    JOY_EVENT_MENU_SQUARE = 17,
    JOY_EVENT_MENU_ANALOG_LEFT = 18,
    JOY_EVENT_MENU_ANALOG_RIGHT = 19,
    JOY_EVENT_MENU_ANALOG_UP = 20,
    JOY_EVENT_MENU_ANALOG_DOWN = 21,
    JOY_EVENT_SKIP_DEMO_MODE = 22,
    JOY_EVENT_MAIN_MENU_SELECT_TRANSMISSION = 23,
    JOY_EVENT_ANY = 24,
    JOY_EVENT_EXIT_EDIT_TEXT = 25,
    JOY_EVENT_CLEAR_TEXT = 26,
    JOY_EVENT_CANCEL_EDIT_TEXT = 27,
    JOY_EVENT_DELETE_CHAR = 28,
    JOY_EVENT_ROTATE_LETTER_UP = 29,
    JOY_EVENT_ROTATE_LETTER_DOWN = 30,
    JOY_EVENT_PREV_CHAR = 31,
    JOY_EVENT_NEXT_CHAR = 32,
    JOY_EVENT_CARSEL_CHANGE_MODE = 33,
    JOY_EVENT_CARSEL_UNLOCK = 34,
    JOY_EVENT_CARSEL_ORBIT_LEFT = 35,
    JOY_EVENT_CARSEL_ORBIT_RIGHT = 36,
    JOY_EVENT_CARSEL_ORBIT_UP = 37,
    JOY_EVENT_CARSEL_ORBIT_DOWN = 38,
    // The ELF table skips 0x27/0x28 and 0x2c between these records; the gaps are real gaps in the
    // table, not omissions here, and the enumerators they belong to are unrecovered.
    JOY_EVENT_CARSEL_ORBIT_LEFTRIGHT = 41,
    JOY_EVENT_CARSEL_ORBIT_UPDOWN = 42,
    JOY_EVENT_CARSEL_ORBIT_INOUT = 43,
    JOY_EVENT_CHANGE_DEBUG_CAMERA = 45,
};

// Name as it appears in the 0x002FA2D0 table, or nullptr for a value that table does not list.
const char* joy_event_name(std::int32_t event);

} // namespace hp2::input

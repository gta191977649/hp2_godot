// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\frontendscreens\options.cpp`
// (string at 0x002B9E00, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "options.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdio>

bool ::port::frontend::advance_zero_velocity_path(hp2::frontend::Type_00248568& path, float delta_seconds) {
        if (path.field_0x48 == 0) return true;
        if (path.field_0x48 == 2) {
            path.field_0x28 = 0;
            path.field_0x48 = 1;
            // FUN_001DA780: both endpoint velocities were initialized to zero by 00248568.
            const float difference = path.field_0x24 - path.field_0x1c;
            path.field_0x38 = path.field_0x1c;
            path.field_0x34 = 0;
            path.field_0x2c = -(difference + difference);
            path.field_0x30 = difference * 3.0f;
        }
        path.field_0x28 += delta_seconds / path.field_0x40;
        if (1.0f <= path.field_0x28) {
            path.field_0x1c = path.field_0x24;
            path.field_0x48 = 0;
        } else {
            const float t = path.field_0x28;
            path.field_0x1c = ((path.field_0x2c * t + path.field_0x30) * t +
                             path.field_0x34) * t + path.field_0x38;
        }
    return path.field_0x48 == 0;
}

namespace hp2::frontend {
void Type_002490D8::Sub_002486E8(Type_00248568& path) {
    FeWidget* mask = FindWidget_00143D50(*field_0x00, path.field_0x10, 0);
    if (!mask) return; // FUN_00144398 / FUN_00145308 return for absent anchors
    const float x = static_cast<float>(static_cast<int>(path.field_0x1c));
    const float y = path.field_0x18;
    auto write = [](FeWidget& widget, float px, float py) {
        if (widget.x != px) { widget.x = px; widget.dirty_0x2b = 4; }
        if (widget.y != py) { widget.y = py; widget.dirty_0x2b = 4; }
    };
    if (path.field_0x0c == 0) { write(*mask, x, y); return; }
    // FUN_00145308 snapshots the mask position before traversing group members.
    const float dx = x - mask->x, dy = y - mask->y;
    for (FeWidget& widget : field_0x00->widgets)
        if (widget.class_hash == path.field_0x0c) write(widget, widget.x + dx, widget.y + dy);
}

void Type_002490D8::Sub_00248C60(float x, float duration, std::uint32_t group,
                               std::uint32_t mask_hash) {
    Type_00248568 path;
    path.field_0x0c = group; path.field_0x10 = mask_hash;
    path.field_0x1c = x; path.field_0x40 = duration;
    if (FeWidget* mask = FindWidget_00143D50(*field_0x00, mask_hash, 0))
        path.field_0x18 = static_cast<float>(static_cast<int>(mask->y)); // FUN_001440D8
    path.field_0x48 = x == path.field_0x24 ? 0 : 2;
    Sub_002486E8(path);
    field_0x04.push_back(path);
}

void Type_002490D8::Construct_002490D8(FeScreen& screen) {
    field_0x00 = &screen; field_0x04.clear();
    float maximum = 0;
    for (int i = 0; i < 16; ++i) {
        char text[32]; std::snprintf(text, sizeof(text), "MENUITEM_%d_GROUP", i);
        const auto group = StringHash_002534A8(text);
        bool present = false;
        for (const FeWidget& widget : screen.widgets) if (widget.class_hash == group) present = true;
        if (!present) continue;
        std::snprintf(text, sizeof(text), "MI_%d_MASK", i);
        const auto mask = StringHash_002534A8(text);
        const FeWidget* widget = FindWidget_00143D50(screen, mask, 0);
        const float duration = 0.05f + static_cast<float>(i) * 0.05f; // gp-0x3AB4/-0x3AB0
        if (maximum < duration) maximum = duration;
        Sub_00248C60(widget ? widget->x : 0.0f, duration, group, mask);
    }
    const auto mask = std::uint32_t{0x32C91DAE}; // MI_UNDERSCORE
    const FeWidget* widget = FindWidget_00143D50(screen, mask, 0);
    Sub_00248C60(widget ? widget->x : 0.0f, maximum, 0x97BF4B25, mask);
}

bool Type_002490D8::Update_00249128(float delta_seconds) {
    for (Type_00248568& path : field_0x04) {
        if (path.field_0x48 == 0) continue;
        ::port::frontend::advance_zero_velocity_path(path, delta_seconds);
        Sub_002486E8(path);
    }
    for (const Type_00248568& path : field_0x04) if (path.field_0x48 != 0) return false;
    return true;
}

void Sub_001EECA8(int message, std::uint32_t item_hash,
                  const ::port::frontend::ScreenActionServices& services) {
    if (message != 1) return;
    switch (item_hash) {
    case 0x52E039F3: services.exit_animation("Audio Options"); break;
    case 0x52E039F4: services.exit_animation("Joystick PreConfig"); break;
    case 0x52E039F5:
        services.display_prepare(); // FUN_001EE6C0 precedes construction of the exit object
        services.exit_animation("Display Options"); break;
    case 0x52E039F6: services.request("Phils Load Save", true); break;
    case 0x52E039F7: services.exit_animation("NameEntry"); break;
    case 0x52E039F8: services.request("Credits", true); break;
    case 0x52E039F9: services.request("Language Options", true); break;
    default:
        hp2::not_implemented("0x001EECA8", "unmatched item uses unresolved default target at 0x00331660");
    }
}

void Construct_001F1A78(Type_001F1A78& instance, FeScreen& screen, MenuList_00169F78* list,
                        std::int32_t dat_0031C1C8, std::int32_t field_0x08) {
    // FUN_001F1A78: instance+4 is the screen from the message-0x0B payload, +8 the flag, and
    // +0 the current language clamped to 0 when it is still the -1 "unset" value. The vtable
    // written at +0xC (002BA520, or 002BA508 when 001F1D70 overwrites it) has no port analogue.
    instance.field_0x08 = field_0x08;
    instance.field_0x00 = dat_0031C1C8;
    if (dat_0031C1C8 == -1) instance.field_0x00 = 0;
    // FUN_0016FDB0 returns 0 unconditionally in this build, so `if (FUN_0016fdb0() == 0)` is
    // always taken and MENUITEM_7 is always disabled and hidden. What the original condition
    // was meant to express is not recovered; only the constant return is. (E-0025-L02)
    Sub_0016D2A8(list, screen, kWidgetMenuItem7_52E039F9, 2);
}

void Sub_001F1B08(int message, std::uint32_t item_hash, Type_001F1A78& instance,
                  std::int32_t& dat_0031C1C8, std::int32_t& dat_00331A60,
                  const ::port::frontend::ScreenActionServices& services) {
    if (message != 1) return; // the 0x11/0x12 arms are elsewhere; every other message returns
    if (!services.invoke) hp2::not_implemented("0x001F1B08", "activation side-effect service missing");
    // FUN_001F1B08 case 1: the jump table on (hash - MENUITEM_1) stores the index first, and
    // only then do the unsigned range guards decide what to do with it. An out-of-range hash
    // leaves +0x00 alone and returns, so the store and the guards are kept in this order.
    if (item_hash >= kWidgetMenuItem1_52E039F3 && item_hash <= kWidgetMenuItem6_52E039F8)
        instance.field_0x00 = static_cast<std::int32_t>(item_hash - kWidgetMenuItem1_52E039F3);
    if (item_hash < kWidgetMenuItem1_52E039F3) return; // sltu against MENUITEM_1
    if (item_hash > kWidgetMenuItem6_52E039F8) {
        // The always-hidden extra item. DAT_00331A60's consumer is unresolved, so the write is
        // reproduced where the original makes it rather than being reinterpreted; 001740C0 is
        // the only other writer. The item is unreachable from input because the constructor
        // disables it, so this arm is recovered but not exercised. (E-0025-L06)
        if (item_hash == kWidgetMenuItem7_52E039F9) {
            dat_00331A60 = 1;
            services.invoke(0x00170320, 1);
        }
        return;
    }
    // thunk_FUN_0021E0D0(&DAT_0031C1C8, instance+0x00): the loader takes the address of the
    // global and writes it itself, so the handler must not write it here. That matters: the
    // check below is a re-read of what the loader left behind, and a handler-side copy of
    // instance+0x00 would make the -1 arm unreachable.
    services.invoke(0x0021E0D0, static_cast<std::uint32_t>(instance.field_0x00));
    // The original tests DAT_0031C1C8 after the loader ran and returns without popping when it
    // is still -1.
    if (dat_0031C1C8 == -1) return;
    if (instance.field_0x08 == 0) services.invoke(0x00183830, 0x002FC050);
    services.invoke(0x00170320, 1);
}
}

namespace port::frontend {
namespace {
using Context=::port::frontend::BackContext;
using Event=::port::frontend::BackEvent;
using Services=::port::frontend::BackServices;
bool changed(const Context& c,const char* address) {
    if(!c.settings_changed) ::hp2::not_implemented(address,"Back settings comparison state missing");
    return *c.settings_changed;
}
void invoke(const Services& s,std::uint32_t address,std::uint32_t argument) {
    if(!s.invoke) hp2::not_implemented("0x0016C670","Back side-effect service missing");
    s.invoke(address,argument);
}
void autosave(const Context& c,const Services& s) {
    if(!c.autosave)return;
    if(!s.push_history) hp2::not_implemented("0x001F5C28","Autosave history push service missing");
    s.push_history("Autosave");
}
}
} // namespace port::frontend
namespace hp2::frontend {
using Context=::port::frontend::BackContext;
using Event=::port::frontend::BackEvent;
using Services=::port::frontend::BackServices;
using ::port::frontend::changed;
using ::port::frontend::invoke;
using ::port::frontend::autosave;
// 001EFB80 case0x11 / E-0017-B01: copy0xB0 audio snapshot then apply seven gains,
// jukebox state5 and output settings through001EF120; conditionally push Autosave.
void Sub_001EFB80(Context& c,Event&,const Services& s) {
    if(changed(c,"0x001EFB80")){invoke(s,0x001EFB80,0x11);autosave(c,s);}
}
// 001F3A28 case0x11 / E-0017-B02: save display64bits before aggregate comparison.
void Sub_001F3A28(Context& c,Event&,const Services& s) {
    c.dat_002FC19C=c.display_field_0x64;
    if(changed(c,"0x001F3A28"))autosave(c,s);
}
// 0024D870 / E-0017-B04: keyboard cancellation blocks the outer manager pop.
void Sub_0024D870(Context& c,Event& e,const Services& s) {
    if(!c.keyboard_active) hp2::not_implemented("0x0024D870","NameEntry keyboard state missing");
    if(*c.keyboard_active) {
        invoke(s,0x0024D088,0x11); // closure updates profile/name and editor via0024C5D0
        e.permission=0;e.sound=0x1d;*c.keyboard_active=false;
        invoke(s,0x0016D448,0);
    }else if(changed(c,"0x0024D370"))autosave(c,s);
}
// 001F1B08 / E-0017-B05: no generic Back permission; handler performs its own pop.
void Sub_001F1B08(Context& c,Event& e,const Services& s) {
    e.permission=0;
    if(!c.language)hp2::not_implemented("0x001F1B08","language state missing");
    if(*c.language==-1)return;
    if(c.selected_hash>=0x52E039F3&&c.selected_hash<=0x52E039F8) {
        const auto selected=c.selected_hash-0x52E039F3;
        invoke(s,0x0021E0D0,selected); // locale loader before field/global projection
        *c.language=static_cast<std::int32_t>(selected);
        if(*c.language!=-1&&c.language_instance_0x08==0)invoke(s,0x00183830,0x002FC050);
    }
    invoke(s,0x00170320,1);
}
// 001F0D10 listing / E-0017-B06: conditional Autosave precedes unconditional
// snapshot copy,001AF050(5), and selection-dependent jukebox resume/preview.
void Sub_001F0D10(Context& c,Event&,const Services& s) {
    if(changed(c,"0x001F0D10"))autosave(c,s);
    invoke(s,0x001F0D10,0x11); // required0xB0 snapshot copy; native data unavailable
    invoke(s,0x001AF050,5);
    if(c.jukebox_selected_enabled&&c.jukebox_selected_loaded)
        hp2::not_implemented("0x0027D6A8","Jukebox preview category/song operands missing");
    invoke(s,0x0027D640,1);
}
// 001F1568 case0x11 / E-0017-B07: two0x30 profiles/name commit only if changed.
void Sub_001F1568(Context& c,Event&,const Services& s) {
    if(changed(c,"0x001F1568")){invoke(s,0x001F1568,0x11);autosave(c,s);}
}
}
namespace port::frontend {
BackEvent query_back(std::uint32_t handler,BackContext& c,const BackServices& s) {
    BackEvent event;
    // 001F4F88/002174A0/00217AF8 ->00249128 exit callback gate, E-0017-B08.
    switch(handler) {
    case 0x001F0278:case 0x001F3EC0:case 0x00240060:case 0x001F1220:
    case 0x002174A0:case 0x00217AF8:
        if(c.exit_active){event.permission=0;event.sound=0;return event;}
        break;
    default:break;
    }
    switch(handler) {
    case 0x001F0278:hp2::frontend::Sub_001EFB80(c,event,s);break;
    case 0x001F3EC0:hp2::frontend::Sub_001F3A28(c,event,s);break;
    case 0x00240060:hp2::frontend::Sub_002402C0(c,event,s);break;
    case 0x0024DBD8:hp2::frontend::Sub_0024D870(c,event,s);break;
    case 0x001F1DD8:hp2::frontend::Sub_001F1B08(c,event,s);break;
    case 0x001F1220:hp2::frontend::Sub_001F0D10(c,event,s);break;
    case 0x001F19E8:hp2::frontend::Sub_001F1568(c,event,s);break;
    case 0x001F21B0:break; // 001F2070 calls00248AE0, which ignoresmessage0x11
    case 0x002174A0:case 0x00217AF8:break; // active-state handler has no0x11 write
    // 0x001FB218 "View Event Tree" / TASK-0021, E-0021-26. FUN_001FB218 forwards message 0x11
    // to FUN_001FAF60, whose state-0 arm calls FUN_001F5D28(0x2FC1B4, tree->type_0x08,
    // record+0x28) and then returns without writing a permission or a sound -- so the manager's
    // own history pop decides what happens, exactly as the two mode selects do. The
    // FUN_001F5D28 call is a profile write and is performed by the caller, which owns the
    // profile; see FrontEndRuntime::menu_joy_event. State 2 never reaches here, because the
    // view consumes Back itself through FUN_001FAD20.
    case 0x001FB218:break;
    case 0x00247830:
        if(c.loadsave_state_0x54!=0)hp2::not_implemented("0x00247530","load/save operation Back polling dependencies missing");
        break; // common pulse dispatcher00248AE0 ignoresmessage0x11
    default:hp2::not_implemented("0x0016C670","screen Back handler not recovered");
    }
    return event;
}
}

namespace hp2::frontend {
// ELF001EFB80 messages4/8/9; E-0017-S04, read-only handler listing/decompile.
void Sub_001EFB80(int message,MenuEvent_0016A458& event,Type_00249DE0& viewport,
                  Type_0024A690& selector,FeScreen& screen,const WidgetMeasure& measure) {
    viewport.Handle_0024A480(message,event.field_0x00);
    if (message == 4) {
        selector.Bind_0024A860(event.field_0x00 == 0x52E039FA ? 0x2932DFCA : 0);
        PlaceSelectionBars_001F4E68(screen,viewport.Sub_0024A368(),measure);
    }
    if ((message == 8 && event.field_0x00 == 0x52E039F3) ||
        (message == 9 && event.field_0x00 == 0xAEE77884)) event.result_0x08 = 1;
}
}

namespace hp2::frontend {
// ELF001F3A28 case4/8/9; E-0017-S05. No settings writes are implied by this slice.
void Sub_001F3A28(int message,MenuEvent_0016A458& event,Type_00249DE0& viewport,
                  Type_0024A690& selector,FeScreen& screen,const WidgetMeasure& measure) {
    viewport.Handle_0024A480(message,event.field_0x00);
    if (message == 4) {
        constexpr std::uint32_t targets[]{0x55CB09F2,0x2E931F1B,0x03D90172,
            0x5610F2F4,0x422313CB,0x8FE4086B,0,0,0};
        const auto index = event.field_0x00 - 0x52E039F3u;
        selector.Bind_0024A860(index < 9 ? targets[index] : 0);
        PlaceSelectionBars_001F4E68(screen,viewport.Sub_0024A368(),measure);
    }
    if ((message == 8 && event.field_0x00 == 0x52E039F3) ||
        (message == 9 && event.field_0x00 == 0x52E039FB)) event.result_0x08 = 1;
}
}

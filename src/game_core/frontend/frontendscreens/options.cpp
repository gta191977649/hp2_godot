#include "options.hpp"
#include "game_core/not_implemented.hpp"
#include <cstdio>

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
        Sub_002486E8(path);
    }
    for (const Type_00248568& path : field_0x04) if (path.field_0x48 != 0) return false;
    return true;
}

void Sub_001EECA8(int message, std::uint32_t item_hash,
                  const port::frontend::ScreenActionServices& services) {
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
        HP2_NOT_IMPLEMENTED("0x001EECA8", "unmatched item uses unresolved default target at 0x00331660");
    }
}
}

namespace port::frontend {
namespace {
using Context=::port::frontend::BackContext;
using Event=::port::frontend::BackEvent;
using Services=::port::frontend::BackServices;
bool changed(const Context& c,const char* address) {
    if(!c.settings_changed) ::hp2::throw_not_implemented(address,"Back settings comparison state missing");
    return *c.settings_changed;
}
void invoke(const Services& s,std::uint32_t address,std::uint32_t argument) {
    if(!s.invoke) HP2_NOT_IMPLEMENTED("0x0016C670","Back side-effect service missing");
    s.invoke(address,argument);
}
void autosave(const Context& c,const Services& s) {
    if(!c.autosave)return;
    if(!s.push_history) HP2_NOT_IMPLEMENTED("0x001F5C28","Autosave history push service missing");
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
    if(!c.keyboard_active) HP2_NOT_IMPLEMENTED("0x0024D870","NameEntry keyboard state missing");
    if(*c.keyboard_active) {
        invoke(s,0x0024D088,0x11); // closure updates profile/name and editor via0024C5D0
        e.permission=0;e.sound=0x1d;*c.keyboard_active=false;
        invoke(s,0x0016D448,0);
    }else if(changed(c,"0x0024D370"))autosave(c,s);
}
// 001F1B08 / E-0017-B05: no generic Back permission; handler performs its own pop.
void Sub_001F1B08(Context& c,Event& e,const Services& s) {
    e.permission=0;
    if(!c.language)HP2_NOT_IMPLEMENTED("0x001F1B08","language state missing");
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
        HP2_NOT_IMPLEMENTED("0x0027D6A8","Jukebox preview category/song operands missing");
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
    case 0x00247830:
        if(c.loadsave_state_0x54!=0)HP2_NOT_IMPLEMENTED("0x00247530","load/save operation Back polling dependencies missing");
        break; // common pulse dispatcher00248AE0 ignoresmessage0x11
    default:HP2_NOT_IMPLEMENTED("0x0016C670","screen Back handler not recovered");
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

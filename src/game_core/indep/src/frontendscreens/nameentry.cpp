// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\frontendscreens\nameentry.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/NameEntry.hpp"

#include "game_core/indep/src/frontendscreens/nameentry.hpp"

#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace hp2::frontend {
// Retail0024C708, E-0095-N01. Row/column survive repeated editor opens.
void Sub_0024C708(::port::frontend::NameEntryState& s,int player) {
    if(player<0 || player>1)hp2::not_implemented("0x0024C708","unknown NameEntry player");
    s.active=true;s.player=player;s.uppercase=false;s.cursor=0;s.buffer.fill(0);
    std::copy_n(s.names[player].begin(),7,s.buffer.begin());
}

// Retail0024D088/0024CC00/0024CCB0/0024CBB0, E-0095-N01.
bool Sub_0024D088(::port::frontend::NameEntryState& s,int message,MenuEvent_0016A458& e) {
    // Back has no PlayerRight gate in the original dispatch.
    if(message!=17 && e.pad_index_0x04!=s.player){e.sound_id_0x0C=0;return true;}
    const auto commit=[&] {
        s.names[s.player].fill(0);
        std::copy(s.buffer.begin(),s.buffer.end(),s.names[s.player].begin());
        s.active=false;e.result_0x08=0;e.sound_id_0x0C=29;return false;
    };
    switch(message) {
    case 1: {
        constexpr char lower[][15]={"abcdefghijklmn","opqrstuvwxyz!.","0123456789# ()"};
        constexpr char upper[][15]={"ABCDEFGHIJKLMN","OPQRSTUVWXYZ!.","0123456789# ()"};
        const char ch=(s.uppercase?upper:lower)[s.row][s.column];
        if(ch==')')return commit();
        if(ch=='(') {
            for(int i=s.cursor;i<6;++i)s.buffer[i]=s.buffer[i+1];
            s.buffer[6]=' ';s.buffer[7]=0;s.cursor=std::max(0,s.cursor-1);e.sound_id_0x0C=26;
        } else {s.buffer[s.cursor]=ch;s.cursor=std::min(6,s.cursor+1);e.sound_id_0x0C=25;}
        break;
    }
    case 6:s.column=(s.column+13)%14;e.result_0x08=1;e.sound_id_0x0C=28;break;
    case 7:s.column=(s.column+1)%14;e.result_0x08=1;e.sound_id_0x0C=28;break;
    case 8:s.row=(s.row+2)%3;e.result_0x08=1;e.sound_id_0x0C=28;break;
    case 9:s.row=(s.row+1)%3;e.result_0x08=1;e.sound_id_0x0C=28;break;
    case 10:s.uppercase=!s.uppercase;e.sound_id_0x0C=27;break;
    case 15:s.cursor=std::max(0,s.cursor-1);e.sound_id_0x0C=1;break;
    case 16:s.cursor=std::min(6,s.cursor+1);e.sound_id_0x0C=2;break;
    case 17:return commit();
    default:break;
    }
    return true;
}

// Retail0024D660, E-0095-N01. Each player's row accepts only that player's pad.
void Sub_0024D660(::port::frontend::NameEntryState& s,int message,MenuEvent_0016A458& e) {
    const int player=e.field_0x00==0x52e039f6 ? 1:0;
    if(e.pad_index_0x04!=player){e.sound_id_0x0C=0;return;}
    s.flags[player]=s.flags[player]==0;e.sound_id_0x0C=message==6?1:2;
}

// Retail0024D370, E-0095-N01. Original uses strcmp, not all12-byte memcmp.
bool Sub_0024D370(const ::port::frontend::NameEntryState& s) {
    for(int p=0;p<2;++p)if(s.flags[p]!=s.initial_flags[p] ||
        std::strncmp(s.names[p].data(),s.initial_names[p].data(),12)!=0)return true;
    return false;
}
}

namespace hp2::recovered {
// ELF 0x0024D048; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024d048.md.
void tKeyboard::PlayerRight() {
    hp2::not_implemented("0x0024D048", "tKeyboard::PlayerRight (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::frontend::name_entry_projection {
using namespace hp2::frontend;
void text(FeScreen& screen,std::uint32_t hash,const std::string& value) {
    if(auto* w=FindWidget_00143D50(screen,hash,1)) {
        w->text=value.substr(0,w->text_capacity ? w->text_capacity-1:0);
        w->string_id_0x60=0;w->dirty_0x2b=3;
    }
}
void translate(FeScreen& screen,float x,float y) {
    auto* reference=FindWidget_00143D50(screen,0x35b2f256,0);
    if(!reference)return;
    const float dx=x-reference->x,dy=y-reference->y;
    for(auto& w:screen.widgets)if(w.class_hash==0x670b6fdd) {
        w.x+=dx;w.y+=dy;w.dirty_0x2b=4;
    }
}
}
namespace hp2::frontend {
// Retail0024D408/0024C5D0/0024CF50/0024CD40, E-0095-N02.
void Sub_0024D408(::port::frontend::NameEntryState& s,FeScreen& screen,
    const std::function<std::string(std::uint32_t)>& localize) {
    using ::port::frontend::name_entry_projection::text;
    SetClassVisible_001454E8(screen,0x670b6fdd,s.active);
    for(int p=0;p<2;++p) {
        const bool editing=s.active && s.player==p;
        SetClassVisible_001454E8(screen,p==0?0x7b95e55c:0x7deb0dfd,editing);
        if(auto* w=FindWidget_00143D50(screen,p==0?0x71450a81:0x71572302,1))w->visible=!editing;
        text(screen,p==0?0x71450a81:0x71572302,s.names[p].data());
        const auto hash=p==0?0xd6f9d954u:0xc33bae35u;
        const auto id=s.flags[p]==0?0x27104u:s.flags[p]==1?0x27105u:0u;
        if(auto* w=FindWidget_00143D50(screen,hash,1)) {
            w->string_id_0x60=id;w->text=localize(id);w->dirty_0x2b=3;
        }
    }
    if(auto* w=FindWidget_00143D50(screen,0xc01086b5,0))w->visible=s.active;
    if(!s.active) {
        ::port::frontend::name_entry_projection::translate(screen,s.panel_authored_x,s.panel_authored_y);
        return;
    }
    constexpr char lower[][15]={"abcdefghijklmn","opqrstuvwxyz!.","0123456789# ()"};
    constexpr char upper[][15]={"ABCDEFGHIJKLMN","OPQRSTUVWXYZ!.","0123456789# ()"};
    for(int r=0;r<3;++r)for(int c=0;c<14;++c) {
        char name[32];std::snprintf(name,sizeof(name),"CHAR_%X%X",r,c);
        text(screen,bStringHash(name),std::string(1,(s.uppercase?upper:lower)[r][c]));
    }
    for(int i=0;i<7;++i) {
        char name[32];std::snprintf(name,sizeof(name),"P%dC%d",s.player+1,i+1);
        if(auto* w=FindWidget_00143D50(screen,bStringHash(name),1)) {
            text(screen,w->name_hash,s.buffer[i]?std::string(1,s.buffer[i]):std::string{});
            w->r=128;w->g=i==s.cursor?64:128;w->b=i==s.cursor?0:128;w->a=128;
        }
    }
}

// Retail0024C948 +001DAAA0, E-0095-N03. Independent scalar paths retain velocity.
void Sub_0024C948(::port::frontend::NameEntryState& s,FeScreen& screen,const WidgetMeasure& measure,bool snap) {
    char name[32];std::snprintf(name,sizeof(name),"CHAR_0%X",s.column);
    const auto x=GetWidgetRect_00144280(screen,bStringHash(name),measure);
    std::snprintf(name,sizeof(name),"CHAR_%X0",s.row);
    const auto y=GetWidgetRect_00144280(screen,bStringHash(name),measure);
    if(!x.valid || !y.valid)hp2::not_implemented("0x0024C948","NameEntry authored grid anchors missing");
    const auto retarget=[snap](::port::frontend::ScalarPath& path,float target) {
        path.duration=0.1f;path.target=target;
        if(path.current!=target)path.phase=2;
        if(snap){path.current=target;path.velocity=path.target_velocity;path.phase=0;}
    };
    retarget(s.grid_x,static_cast<float>(static_cast<int>(x.x0)+(static_cast<int>(x.x1)-static_cast<int>(x.x0))/2));
    retarget(s.grid_y,static_cast<float>(static_cast<int>(y.y0)+(static_cast<int>(y.y1)-static_cast<int>(y.y0))/2));
}

// Retail0024CA60/0024A5D8/0024A610/001E4638, E-0095-N03.
void Sub_0024CA60(::port::frontend::NameEntryState& s,FeScreen& screen,float delta,
    const WidgetMeasure& measure,const std::function<std::string(std::uint32_t)>& localize) {
    Sub_0024D408(s,screen,localize);
    if(!s.active)return;
    // While the panel is entering, the original forces grid coordinates to targets.
    if(s.panel_x.phase!=0)Sub_0024C948(s,screen,measure,true);
    Sub_001DAEC8(s.grid_x,delta);Sub_001DAEC8(s.grid_y,delta);
    if(auto* box=FindWidget_00143D50(screen,0x249db7b7,0)) {
        const int half=static_cast<int>(measure(*box).height)/2;
        box->x=static_cast<float>(static_cast<int>(s.grid_x.current)-half);
        box->y=static_cast<float>(static_cast<int>(s.grid_y.current)-half);box->dirty_0x2b=4;
    }
    Sub_001DAEC8(s.panel_x,delta);
    ::port::frontend::name_entry_projection::translate(screen,static_cast<float>(static_cast<int>(s.panel_x.current)),s.panel_authored_y);
    char name[32];std::snprintf(name,sizeof(name),"P%dC%d",s.player+1,s.cursor+1);
    const auto* letter=FindWidget_00143D50(screen,bStringHash(name),1);
    if(auto* caret=FindWidget_00143D50(screen,0xc01086b5,0)) {
        if(letter){caret->x=static_cast<float>(static_cast<int>(letter->x));
            caret->y=static_cast<float>(static_cast<int>(letter->y+measure(*letter).height+2.0f));}
        s.caret_phase+=10.0f*delta;
        const std::uint32_t bits=0x40c90e56;float limit;std::memcpy(&limit,&bits,4);
        if(s.caret_phase>limit)s.caret_phase-=limit;
        SetAlpha(*caret,static_cast<int>(128.0f*(bSin(s.caret_phase)+1.0f)*0.5f));caret->dirty_0x2b=4;
    }
}
}

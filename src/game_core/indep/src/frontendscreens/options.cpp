// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\frontendscreens\options.cpp`
// (string at 0x002B9E00, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "options.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

bool ::port::frontend::advance_zero_velocity_path(hp2::frontend::AnimationGroup& path, float delta_seconds) {
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

namespace port::frontend::jukebox_projection {
// Host bounds checks surround the original array accesses; malformed/unsupported
// inputs fail explicitly instead of manufacturing songs or clipping the list.
std::uint32_t word(const JukeboxScreenState& state,std::size_t offset) {
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(state.snapshot.at(offset+i))<<(i*8);
    return value;
}
std::size_t group_index(const JukeboxScreenState& state) {
    const auto index=state.snapshot[0xae];
    if(index>=state.groups.size()) hp2::not_implemented("0x001F0478","Jukebox category outside recovered groups");
    return index;
}
void preview(const JukeboxScreenState& state,const JukeboxScreenServices& services,bool resume) {
    const auto& group=state.groups.at(group_index(state));
    if(state.cursor<0 || static_cast<std::size_t>(state.cursor)>=group.songs.size())
        hp2::not_implemented("0x001F0CC8","Jukebox song cursor outside recovered group");
    const auto song=group.songs[static_cast<std::size_t>(state.cursor)];
    if(!services.preview) hp2::not_implemented("0x0027D6A8","Jukebox preview service missing");
    services.preview(state.snapshot[0x1c+song*4],state.snapshot[0x1d+song*4],resume);
}
// ELF00143E70 ->00140E20, E-0092-J04; the owning string storage is host-side.
void set_text(hp2::frontend::FeScreen& screen,std::uint32_t hash,const std::string& value) {
    auto* widget=hp2::frontend::FindWidget_00143D50(screen,hash,1);
    if(!widget || widget->text==value)return;
    if(widget->text_capacity==0)
        hp2::not_implemented("0x00140E20","Jukebox dynamic text has zero original capacity");
    widget->text=value.substr(0,widget->text_capacity-1);
    widget->string_id_0x60=0;
    widget->dirty_0x2b=5;
}
void set_colour(hp2::frontend::FeScreen& screen,std::uint32_t hash,
    const hp2::frontend::MenuColour_00169BD0& colour) {
    if(auto* widget=hp2::frontend::FindWidget_00143D50(screen,hash,1))hp2::frontend::SetColor(*widget,colour);
}
}

namespace port::frontend::display_projection {
template<std::size_t N>std::uint32_t word(const std::array<std::uint8_t,N>& bytes,std::size_t offset) {
    std::uint32_t value=0;for(unsigned b=0;b<4;++b)value|=std::uint32_t(bytes.at(offset+b))<<(b*8);return value;
}
template<std::size_t N>void store(std::array<std::uint8_t,N>& bytes,std::size_t offset,std::uint32_t value) {
    for(unsigned b=0;b<4;++b)bytes.at(offset+b)=static_cast<std::uint8_t>(value>>(b*8));
}
void label(hp2::frontend::FeScreen& screen,std::uint32_t hash,std::uint32_t id,
    const std::function<std::string(std::uint32_t)>& localize) {
    hp2::frontend::SetWidgetStringId_001449A8(screen,hash,id);
    if(auto* widget=hp2::frontend::FindWidget_00143D50(screen,hash,1))widget->text=id ? localize(id) : "";
}
template<std::size_t N>std::uint8_t cycle(std::uint8_t current,int direction,const std::uint8_t (&table)[N]) {
    for(std::size_t i=0;i<N;++i)if(table[i]==current)return table[(static_cast<int>(i)+direction+static_cast<int>(N))%N];
    return table[0]; //001BD8B0 unknown-value path ignores direction.
}
}

namespace hp2::frontend {
// E-0094-D01:001F3A28/001F3730/001EE148 and six original change helpers.
const char* Sub_001F3A28(::port::frontend::DisplayOptionsState& state,int message,MenuEvent_0016A458& event,
    const ::port::frontend::DisplayContext& context) {
    using namespace ::port::frontend::display_projection;
    const auto item=event.field_0x00-0x52e039f3u;
    if(message==1) {
        if(item==6)return "Customize P1 HUD";
        if(item==7)return "Customize P2 HUD";
        if(item==8)return "Calibrate display offset";
        event.sound_id_0x0C=0;
    } else if((message==6 || message==7) && item<6) {
        const int direction=message==6 ? -1 : 1;
        auto& bytes=state.snapshot;
        if(item==0) {
            constexpr std::uint8_t field_003140A0[]{0,1,2,3,4,6};
            constexpr std::uint8_t field_00331090[]{7,6},field_00331098[]{8,6};
            if(context.gameflow==3 || (context.dat_0031ab0c==0 && context.gp_ae48==1))bytes[2]=cycle(bytes[2],direction,field_003140A0);
            else if(context.dat_0031ab0c!=0)bytes[4]=cycle(bytes[4],direction,field_00331098);
            else bytes[3]=cycle(bytes[3],direction,field_00331090);
            bytes[3]=bytes[2]==6 ? 6 : 7;bytes[4]=bytes[2]==6 ? 6 : 8;
        } else if(item==1)bytes[0]=bytes[0]==0;
        else if(item==2)bytes[1]=bytes[1]==0;
        else if(item==3)bytes[5]=bytes[5]==2 ? 0 : 2;
        else if(item==4)bytes[6]=bytes[6]==0;
        else if(item==5)bytes[7]=bytes[7]==0;
        event.sound_id_0x0C=message==6 ? 1 : 2;
    } else if(message==13)event.sound_id_0x0C=0;
    return nullptr;
}
// E-0094-D01:001F37D0/001F38D0/001F3950/001F36D8/001F3630/001F3650.
void Sub_001F3A28(const ::port::frontend::DisplayOptionsState& state,FeScreen& screen,
    const std::function<std::string(std::uint32_t)>& localize) {
    using namespace ::port::frontend::display_projection;
    const auto& b=state.snapshot;
    constexpr std::uint32_t field_001F37D0[]{0x22345,0x22346,0x22347,0x223a8,0x22348,0x22349,0x22353,0,0x223a9,0x223aa};
    if(b[2]>=10 || b[5]>=3)hp2::not_implemented("0x001F37D0","Display value outside original string tables");
    label(screen,0x55cb09f2,field_001F37D0[b[2]],localize);
    label(screen,0x2e931f1b,b[0] ? 0x22352 : 0x22353,localize);
    label(screen,0x03d90172,b[1] ? 0x2234b : 0x2234a,localize);
    constexpr std::uint32_t field_002BA1B8[]{0x22353,0x18e71,0x22352};
    label(screen,0x5610f2f4,field_002BA1B8[b[5]],localize);
    label(screen,0x422313cb,b[6] ? 0x22352 : 0x22353,localize);
    label(screen,0x8fe4086b,b[7] ? 0x22352 : 0x22353,localize);
}
// E-0094-D02,001F46D8. Address-preserving bounded changes to player settings.
void Sub_001F46D8(::port::frontend::HudOptionsState& state,int message,MenuEvent_0016A458& event) {
    using namespace ::port::frontend::display_projection;
    const auto item=event.field_0x00-0x52e039f3u;
    if((message==6 || message==7) && item<7) {
        auto& b=state.snapshot;
        if(item==0)store(b,0xc,word(b,0xc)==0);
        else if(item==1)store(b,8,word(b,8)==0);
        else if(item==2)store(b,0x10,word(b,0x10)^0x40u);
        else if(item==3)store(b,0x10,word(b,0x10)^0x10u);
        else if(item==4)store(b,0x10,word(b,0x10)^0x100u);
        else if(item==5) {
            const int next=static_cast<int>(b[6])+(message==6 ? -1 : 1);
            b[6]=static_cast<std::uint8_t>(next<0 ? 4 : next>4 ? 0 : next);
            store(b,0x10,b[6]==4 ? word(b,0x10)&~4u : word(b,0x10)|4u);
        } else store(b,0x2c,word(b,0x2c)==0);
        event.sound_id_0x0C=message==6 ? 1 : 2;
    } else if(message==1 || message==13)event.sound_id_0x0C=0;
}
// E-0094-D02: six value helpers001F4360..001F46B0 retain original conditions.
void Sub_001F46D8(const ::port::frontend::HudOptionsState& state,FeScreen& screen,
    const std::function<std::string(std::uint32_t)>& localize) {
    using namespace ::port::frontend::display_projection;
    const auto& b=state.snapshot;const auto flags=word(b,0x10),lookback=word(b,0xc);
    label(screen,0x2e018745,lookback==0 ? 0x22356 : lookback==1 ? 0x22351 : 0,localize);
    label(screen,0xffc3dcd2,word(b,8)==1 ? 0x22353 : 0x22352,localize);
    label(screen,0x4ceb3ed2,(flags&0x40) ? 0x22352 : 0x22353,localize);
    label(screen,0x1054482b,(flags&0x10) ? 0x22352 : 0x22353,localize);
    label(screen,0xc7e2117e,(flags&0x100) ? 0x22352 : 0x22353,localize);
    constexpr std::uint32_t field_001F45D8[]{0x2234c,0x2234d,0x2234e,0x2234f,0x22353};
    label(screen,0x6caf6d10,b[6]<5 ? field_001F45D8[b[6]] : 0,localize);
    label(screen,0x6bd89c11,word(b,0x2c) ? 0x22352 : 0x22353,localize);
}
// E-0094-D03:001F4B20 jump table002BA3D0 and complete interior listing.
bool Sub_001F4B20(::port::frontend::DisplayOffsetState& state,int message,int& sound) {
    sound=0;
    if(message==6) {const auto next=static_cast<std::int32_t>(static_cast<std::uint32_t>(state.x)-4u);state.x=next< -650 ? -650 : next;if(next>= -650)sound=1;}
    else if(message==7){state.x=static_cast<std::int32_t>(static_cast<std::uint32_t>(state.x)+4u);sound=1;}
    else if(message==8){const auto next=static_cast<std::int32_t>(static_cast<std::uint32_t>(state.y)-1u);state.y=next< -50 ? -50 : next;if(next>= -50)sound=1;}
    else if(message==9){state.y=static_cast<std::int32_t>(static_cast<std::uint32_t>(state.y)+1u);sound=1;}
    else if(message==14){state.x=state.cached_x;state.y=state.cached_y;sound=5;}
    else if(message==17)sound=6;
    else if(message==1){sound=5;return true;}
    return false;
}

// ELF001DAEC8 bounded scalar path, E-0093-A01. All operation grouping follows
// 001DA780,001DA7D0,001DA800; no Godot tween or zero-derivative substitution.
void Sub_001DAEC8(::port::frontend::ScalarPath& path,float delta) {
    if(path.phase!=1 && path.phase!=2)return;
    if(path.phase==2) {
        path.time=0;path.phase=1;
        const float difference=path.target-path.current;
        path.a=(path.velocity+path.target_velocity)-(difference+difference);
        path.b=(difference*3.0f-path.target_velocity)-(path.velocity+path.velocity);
        path.c=path.velocity;path.d=path.current;
    }
    path.time+=delta/path.duration;
    if(path.limit<=path.time) {
        if(path.limit>=1.0f){path.current=path.target;path.velocity=path.target_velocity;}
        path.phase=0;
    } else {
        path.current=((path.a*path.time+path.b)*path.time+path.c)*path.time+path.d;
        path.velocity=(path.a*path.time*3.0f+path.b+path.b)*path.time+path.c;
    }
}
}

namespace port::frontend::audio_projection {
constexpr std::uint32_t defaults[]{0x3ee66666,0x3f59999a,0x3ec28f5c,0x3f0ccccd,
    0x3ea8f5c3,0x3e9eb852,0x3f400000};
float read_float(const std::array<std::uint8_t,0xb0>& bytes,std::size_t index) {
    std::uint32_t bits=0;
    for(unsigned b=0;b<4;++b)bits|=std::uint32_t(bytes.at(index*4+b))<<(b*8);
    float result;std::memcpy(&result,&bits,4);return result;
}
void write_float(std::array<std::uint8_t,0xb0>& bytes,std::size_t index,float value) {
    std::uint32_t bits;std::memcpy(&bits,&value,4);
    for(unsigned b=0;b<4;++b)bytes.at(index*4+b)=static_cast<std::uint8_t>(bits>>(b*8));
}
float factor(std::size_t index) {
    if(index>=7)hp2::not_implemented("0x0016DC20","Audio slider index outside seven original categories");
    float value;std::memcpy(&value,&defaults[index],4);return 0.7f/value;
}
}

namespace hp2::frontend {
// ELF001EF3C8/0016DC20, E-0093-A01. Constructor snaps the initial path to
// normalized raw amplitude (no entrance animation) and preserves the full snapshot.
void Sub_001EF3C8(::port::frontend::AudioOptionsState& state,const std::array<std::uint8_t,0xb0>& snapshot) {
    state={};state.snapshot=snapshot;
    for(std::size_t i=0;i<7;++i) {
        auto& path=state.sliders[i];
        path.current=::port::frontend::audio_projection::read_float(snapshot,i)*::port::frontend::audio_projection::factor(i);
        path.target=path.current;
    }
}
// ELF0016DD38/0016DD18/0016DD28, E-0093-A01.
std::optional<float> Sub_0016DD38(::port::frontend::AudioOptionsState& state,std::size_t index,float delta) {
    using namespace ::port::frontend::audio_projection;
    const float scale=factor(index),old=read_float(state.snapshot,index);
    float normalized=(old+delta)*scale;
    if(normalized>1.0f)normalized=1.0f;
    if(normalized<0.0f)normalized=0.0f;
    const float raw=normalized/scale;
    write_float(state.snapshot,index,raw);
    auto& path=state.sliders.at(index);path.target=normalized;
    if(normalized!=path.current)path.phase=2;
    if(raw!=old)return normalized;
    return std::nullopt;
}
// ELF0016DE30 +001EFB38, E-0093-A02. Rectangle integer endpoints precede
// normalized interpolation and final truncation. Marker scale/Y remain authored.
void Sub_0016DE30(::port::frontend::AudioOptionsState& state,FeScreen& screen,float delta,
    const WidgetMeasure& measure,const std::function<std::string(std::uint32_t)>& localize) {
    constexpr std::uint32_t field_001EF3C8[]{0xc866642b,0xc866686c,0xc8666cad,0xc86670ee,0xc866752f,0xc8667970,0xc8667db1};
    for(std::size_t i=0;i<7;++i) {
        Sub_001DAEC8(state.sliders[i],delta);
        const auto rect=GetWidgetRect_00144280(screen,0xa29f7a16+static_cast<std::uint32_t>(i),measure);
        if(!rect.valid)continue;
        const int left=static_cast<int>(rect.x0),right=static_cast<int>(rect.x1);
        const int x=static_cast<int>(static_cast<float>(left)+state.sliders[i].current*static_cast<float>(right-left));
        if(auto* marker=FindWidget_00143D50(screen,field_001EF3C8[i],0)) {
            if(marker->x!=x){marker->x=static_cast<float>(x);marker->dirty_0x2b=4;}
        }
    }
    const unsigned output=state.snapshot[0xac]|(unsigned(state.snapshot[0xad])<<8);
    constexpr std::uint32_t field_001EFB38[]{0x1adb9,0x1adb8,0x1adbb};
    if(output>2)hp2::not_implemented("0x001EFB38","Audio output mode outside original selector");
    SetWidgetStringId_001449A8(screen,0x2932dfca,field_001EFB38[output]);
    if(auto* widget=FindWidget_00143D50(screen,0x2932dfca,1))widget->text=localize(field_001EFB38[output]);
}
}

namespace hp2::frontend {
// ELF001F02D8 +001EF240, E-0092-J01. Portable data/grouping slice only.
void Sub_001F02D8(::port::frontend::JukeboxScreenState& state,
    const std::array<std::uint8_t,0xb0>& snapshot,
    std::vector<::port::frontend::JukeboxSongMetadata> metadata) {
    using namespace ::port::frontend;
    JukeboxScreenState next;
    next.snapshot=snapshot;
    next.metadata=std::move(metadata);
    const auto count=jukebox_projection::word(next,0x9c);
    if(count>32 || count!=next.metadata.size())
        hp2::not_implemented("0x001EF240","Jukebox metadata/count exceeds original 32-song block");
    for(std::size_t i=0;i<count;++i) {
        const auto& label=next.metadata[i].field_3;
        if(label.empty())continue; // Original ignores null/empty metadata field3.
        auto group=std::find_if(next.groups.begin(),next.groups.end(),[&](const auto& g){return g.label==label;});
        if(group==next.groups.end()) {
            if(next.groups.size()==6 || label.size()>=20)
                hp2::not_implemented("0x001EF240","Jukebox group exceeds original storage");
            next.groups.push_back({label,{}});
            group=next.groups.end()-1;
        }
        if(group->songs.size()==20)
            hp2::not_implemented("0x001EF240","Jukebox group exceeds original index storage");
        next.snapshot[0x1f+i*4]=static_cast<std::uint8_t>(group-next.groups.begin());
        group->songs.push_back(i);
    }
    // 001F0478 immediately reads the selected group. Preserve unsupported empty
    // input as a gap, rather than substituting a fabricated category.
    (void)Sub_001F0478(next);
    state=std::move(next);
}

// ELF001F0478, E-0092-J02. Widget transport/colour binding is deliberately separate.
::port::frontend::JukeboxPresentation Sub_001F0478(::port::frontend::JukeboxScreenState& state) {
    using namespace ::port::frontend;
    const auto genre=jukebox_projection::group_index(state);
    const auto& songs=state.groups[genre].songs;
    const auto count=static_cast<int>(songs.size());
    if(state.cursor<0 || state.cursor>=count || state.first_row<0)
        hp2::not_implemented("0x001F0478","Jukebox row state outside recovered bounds");
    while(state.first_row+5<=state.cursor && state.first_row+5<count)++state.first_row;
    while(state.cursor<state.first_row && state.first_row>=0)--state.first_row;
    JukeboxPresentation result;
    result.category_enabled=(state.snapshot[0xaf]&(1u<<genre))!=0;
    result.category_arrows=!state.list_active;
    const auto visible=std::min(5,count);
    for(int row=0;row<visible;++row) {
        const auto song=songs.at(static_cast<std::size_t>(state.first_row+row));
        const auto mode=result.category_enabled ? state.snapshot.at(0x1e + song*4) : 0;
        if(mode>5)hp2::not_implemented("0x001F0478","Jukebox mode outside six original string IDs");
        const bool selected=state.list_active && state.cursor==state.first_row+row;
        result.rows.push_back({song,selected,selected,mode});
    }
    result.up=state.list_active && state.first_row!=0;
    result.down=state.list_active && count>4 && state.first_row!=count-5;
    return result;
}

// ELF001F0478; E-0092-J04, task0092-jukebox.md. Hash arrays are the exact
// five used entries of each six-word static block at0031AE00..0031AE77.
void Sub_001F0478(::port::frontend::JukeboxScreenState& state,FeScreen& screen,
    const std::function<std::string(std::uint32_t)>& localize) {
    using namespace ::port::frontend;
    using namespace ::port::frontend::jukebox_projection;
    const auto view=Sub_001F0478(state);
    constexpr std::uint32_t field_0031AE00[]{0x64a819c7,0x64a819c8,0x64a819c9,0x64a819ca,0x64a819cb};
    constexpr std::uint32_t field_0031AE18[]{0x03b57047,0x03b57048,0x03b57049,0x03b5704a,0x03b5704b};
    constexpr std::uint32_t field_0031AE30[]{0x89fb635f,0x8c508c00,0x8ea5b4a1,0x90fadD42,0x935005e3};
    constexpr std::uint32_t field_0031AE48[]{0x0b825845,0x0b825846,0x0b825847,0x0b825848,0x0b825849};
    constexpr std::uint32_t field_0031AE60[]{0xfc0279b8,0xfc0279b9,0xfc0279ba,0xfc0279bb,0xfc0279bc};
    constexpr std::uint32_t field_002B9D30[]{0x1add9,0x1adda,0x1addd,0x1addb,0x1ade4,0x1addc};
    for(std::size_t i=0;i<5;++i) {
        const bool visible=i<view.rows.size();
        for(auto hash:{field_0031AE00[i],field_0031AE18[i],field_0031AE30[i]})
            SetWidgetVisible_00144968(screen,hash,visible);
        const bool arrows=visible && view.rows[i].arrows;
        SetWidgetVisible_00144968(screen,field_0031AE48[i],arrows);
        SetWidgetVisible_00144968(screen,field_0031AE60[i],arrows);
        if(!visible)continue;
        const auto& row=view.rows[i];
        const auto& metadata=state.metadata.at(row.song);
        set_text(screen,field_0031AE00[i],metadata.field_1);
        set_text(screen,field_0031AE18[i],metadata.field_0);
        const auto id=field_002B9D30[row.mode];
        SetWidgetStringId_001449A8(screen,field_0031AE30[i],id);
        if(auto* widget=FindWidget_00143D50(screen,field_0031AE30[i],1))widget->text=localize(id);
        const auto colour=row.highlighted ? kHighlightColour_gp_0x5330 :
            (view.category_enabled ? MenuColour_00169BD0{128,128,128} : kDisabledColour_gp_0x5324);
        for(auto hash:{field_0031AE00[i],field_0031AE18[i],field_0031AE30[i]})set_colour(screen,hash,colour);
    }
    SetWidgetVisible_00144968(screen,0xedf2e4ee,view.up);
    SetWidgetVisible_00144968(screen,0x20d73d01,view.down);
    const auto playing=word(state,0xa4);
    if(playing>=state.metadata.size())hp2::not_implemented("0x001F0478","Jukebox now-playing index outside snapshot");
    const auto& metadata=state.metadata[playing];
    set_text(screen,0x0b41999a,metadata.field_1+" ("+metadata.field_0+")"); //00331698
    set_text(screen,0x52e039f3,state.groups[group_index(state)].label);
    set_colour(screen,0x52e039f3,view.category_arrows ? kHighlightColour_gp_0x5330 : MenuColour_00169BD0{128,128,128});
    SetWidgetVisible_00144968(screen,0xd1cda594,view.category_arrows);
    SetWidgetVisible_00144968(screen,0x9345e4a7,view.category_arrows);
}

// ELF001F0D10 jump table002B9D50 and listing001F0D5C..001F121C,
// E-0092-J03. Message1/10/13 clear sound; other table default arms do nothing.
void Sub_001F0D10(::port::frontend::JukeboxScreenState& state,int message,int& sound,
    std::uint32_t tick,float elapsed_seconds,bool playback_active,
    const ::port::frontend::JukeboxScreenServices& services) {
    using namespace ::port::frontend;
    const auto genre=jukebox_projection::group_index(state);
    const auto count=static_cast<int>(state.groups[genre].songs.size());
    if(message==1 || message==10 || message==13) {sound=0;return;}
    if(message==15) {
        state.snapshot[0xaf]^=static_cast<std::uint8_t>(1u<<genre);
        if((state.snapshot[0xaf]&(1u<<genre))==0) {
            state.list_active=false;state.first_row=0;state.cursor=0;
        }
        sound=5;
    } else if(message==6 || message==7) {
        const int direction=message==6 ? -1 : 1;
        if(state.list_active) {
            const auto song=state.groups[genre].songs.at(static_cast<std::size_t>(state.cursor));
            auto& mode=state.snapshot.at(0x1e + song*4);
            int next=static_cast<int>(mode)+direction;
            if(next<0)next=5;
            if(next>=6)next=0;
            mode=static_cast<std::uint8_t>(next);
        } else {
            int next=static_cast<int>(genre)+direction;
            if(next<0)next=static_cast<int>(state.groups.size())-1;
            if(next>=static_cast<int>(state.groups.size()))next=0;
            state.snapshot[0xae]=static_cast<std::uint8_t>(next);
            state.cursor=0;
        }
        sound=message==6 ? 1 : 2;
    } else if(message==8 || message==9) {
        if(state.list_active) {
            if(message==8 && state.cursor==0 && state.first_row==0) {
                if(!services.stop)hp2::not_implemented("0x0027D868","Jukebox stop service missing");
                state.list_active=false;
                services.stop(); // Original leaves this message's sound untouched.
            } else {
                const int next=state.cursor+(message==8 ? -1 : 1);
                if(next<0 || next>=count) {sound=0;return;}
                state.cursor=next;sound=message==8 ? 3 : 4;
                state.selection_tick=tick;state.preview_pending=true;
            }
        } else if(message==9) {
            if((state.snapshot[0xaf]&(1u<<genre))!=0) {
                state.list_active=true;state.selection_tick=tick;state.preview_pending=true;
            } else sound=6;
        } else sound=0;
    } else if(message==17) {
        if(!services.commit)hp2::not_implemented("0x001F0D10","Jukebox Back dependency chain missing");
        services.commit(state);
    } else if(message==18) {
        if(elapsed_seconds>0.5f && state.preview_pending) {
            if(!services.preview)hp2::not_implemented("0x0027D6A8","Jukebox preview service missing");
            state.preview_pending=false;
            jukebox_projection::preview(state,services,false);
        }
        if(elapsed_seconds>30.0f && !playback_active)
            jukebox_projection::preview(state,services,false);
    }
    (void)Sub_001F0478(state);
}
}

namespace hp2::frontend {
void tExitScreenAnimation::SetupToAnimate(AnimationGroup& path) {
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

void tExitScreenAnimation::Sub_00248C60(float x, float duration, std::uint32_t group,
                               std::uint32_t mask_hash) {
    AnimationGroup path;
    path.field_0x0c = group; path.field_0x10 = mask_hash;
    path.field_0x1c = x; path.field_0x40 = duration;
    if (FeWidget* mask = FindWidget_00143D50(*field_0x00, mask_hash, 0))
        path.field_0x18 = static_cast<float>(static_cast<int>(mask->y)); // FUN_001440D8
    path.field_0x48 = x == path.field_0x24 ? 0 : 2;
    SetupToAnimate(path);
    field_0x04.push_back(path);
}

void tExitScreenAnimation::Construct_002490D8(FeScreen& screen) {
    field_0x00 = &screen; field_0x04.clear();
    float maximum = 0;
    for (int i = 0; i < 16; ++i) {
        char text[32]; std::snprintf(text, sizeof(text), "MENUITEM_%d_GROUP", i);
        const auto group = bStringHash(text);
        bool present = false;
        for (const FeWidget& widget : screen.widgets) if (widget.class_hash == group) present = true;
        if (!present) continue;
        std::snprintf(text, sizeof(text), "MI_%d_MASK", i);
        const auto mask = bStringHash(text);
        const FeWidget* widget = FindWidget_00143D50(screen, mask, 0);
        const float duration = 0.05f + static_cast<float>(i) * 0.05f; // gp-0x3AB4/-0x3AB0
        if (maximum < duration) maximum = duration;
        Sub_00248C60(widget ? widget->x : 0.0f, duration, group, mask);
    }
    const auto mask = std::uint32_t{0x32C91DAE}; // MI_UNDERSCORE
    const FeWidget* widget = FindWidget_00143D50(screen, mask, 0);
    Sub_00248C60(widget ? widget->x : 0.0f, maximum, 0x97BF4B25, mask);
}

bool tExitScreenAnimation::Update_00249128(float delta_seconds) {
    for (AnimationGroup& path : field_0x04) {
        if (path.field_0x48 == 0) continue;
        ::port::frontend::advance_zero_velocity_path(path, delta_seconds);
        SetupToAnimate(path);
    }
    for (const AnimationGroup& path : field_0x04) if (path.field_0x48 != 0) return false;
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

void Construct_001F1A78(LanguageOptionsScreenBase& instance, FeScreen& screen, MenuList_00169F78* list,
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

void Sub_001F1B08(int message, std::uint32_t item_hash, LanguageOptionsScreenBase& instance,
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
void Sub_001EFB80(int message,MenuEvent_0016A458& event,tScrollingMenuList& viewport,
                  tLeftRightArrowScroll& selector,FeScreen& screen,const WidgetMeasure& measure) {
    viewport.Handle_0024A480(message,event.field_0x00);
    if (message == 4) {
        selector.Bind_0024A860(event.field_0x00 == 0x52E039FA ? 0x2932DFCA : 0);
        HandleHighlight(screen,viewport.Sub_0024A368(),measure);
    }
    if ((message == 8 && event.field_0x00 == 0x52E039F3) ||
        (message == 9 && event.field_0x00 == 0xAEE77884)) event.result_0x08 = 1;
}
}

namespace hp2::frontend {
// ELF001F3A28 case4/8/9; E-0017-S05. No settings writes are implied by this slice.
void Sub_001F3A28(int message,MenuEvent_0016A458& event,tScrollingMenuList& viewport,
                  tLeftRightArrowScroll& selector,FeScreen& screen,const WidgetMeasure& measure) {
    viewport.Handle_0024A480(message,event.field_0x00);
    if (message == 4) {
        constexpr std::uint32_t targets[]{0x55CB09F2,0x2E931F1B,0x03D90172,
            0x5610F2F4,0x422313CB,0x8FE4086B,0,0,0};
        const auto index = event.field_0x00 - 0x52E039F3u;
        selector.Bind_0024A860(index < 9 ? targets[index] : 0);
        HandleHighlight(screen,viewport.Sub_0024A368(),measure);
    }
    if ((message == 8 && event.field_0x00 == 0x52E039F3) ||
        (message == 9 && event.field_0x00 == 0x52E039FB)) event.result_0x08 = 1;
}
}


namespace hp2::frontend {
// ELF 0x001EF3C8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001ef3c8.md.
void AudioOptionsScreen::Sub_001EF3C8() {
    hp2::not_implemented("0x001EF3C8", "AudioOptionsScreen::Sub_001EF3C8 (signature and full body unresolved)");
}

// ELF 0x001EFA00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001efa00.md.
void AudioOptionsScreen::Sub_001EFA00() {
    hp2::not_implemented("0x001EFA00", "AudioOptionsScreen::Sub_001EFA00 (signature and full body unresolved)");
}

// ELF 0x001EFB78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001efb78.md.
void AudioOptionsScreen::Sub_001EFB78() {
    hp2::not_implemented("0x001EFB78", "AudioOptionsScreen::Sub_001EFB78 (signature and full body unresolved)");
}

// ELF 0x001EFB80; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001efb80.md.
void AudioOptionsScreen::Sub_001EFB80() {
    hp2::not_implemented("0x001EFB80", "AudioOptionsScreen::Sub_001EFB80 (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001F02D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f02d8.md.
void JukeBoxMusicScreen::Sub_001F02D8() {
    hp2::not_implemented("0x001F02D8", "JukeBoxMusicScreen::Sub_001F02D8 (signature and full body unresolved)");
}

// ELF 0x001F0460; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f0460.md.
void JukeBoxMusicScreen::Sub_001F0460() {
    hp2::not_implemented("0x001F0460", "JukeBoxMusicScreen::Sub_001F0460 (signature and full body unresolved)");
}

// ELF 0x001F0D10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f0d10.md.
void JukeBoxMusicScreen::Sub_001F0D10() {
    hp2::not_implemented("0x001F0D10", "JukeBoxMusicScreen::Sub_001F0D10 (signature and full body unresolved)");
}


} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001F33B8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f33b8.md.
void tDisplayOptionsScreen::Sub_001F33B8() {
    hp2::not_implemented("0x001F33B8", "tDisplayOptionsScreen::Sub_001F33B8 (signature and full body unresolved)");
}

// ELF 0x001F3588; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f3588.md.
void tDisplayOptionsScreen::Sub_001F3588() {
    hp2::not_implemented("0x001F3588", "tDisplayOptionsScreen::Sub_001F3588 (signature and full body unresolved)");
}

// ELF 0x001F35E0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f35e0.md.
void tDisplayOptionsScreen::Sub_001F35E0() {
    hp2::not_implemented("0x001F35E0", "tDisplayOptionsScreen::Sub_001F35E0 (signature and full body unresolved)");
}

// ELF 0x001F3A28; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f3a28.md.
void tDisplayOptionsScreen::Sub_001F3A28() {
    hp2::not_implemented("0x001F3A28", "tDisplayOptionsScreen::Sub_001F3A28 (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001EEBB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001eebb8.md.
void tOptionsScreen::Sub_001EEBB8() {
    hp2::not_implemented("0x001EEBB8", "tOptionsScreen::Sub_001EEBB8 (signature and full body unresolved)");
}

// ELF 0x001EEC50; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001eec50.md.
void tOptionsScreen::Sub_001EEC50() {
    hp2::not_implemented("0x001EEC50", "tOptionsScreen::Sub_001EEC50 (signature and full body unresolved)");
}

// ELF 0x001EECA8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001eeca8.md.
void tOptionsScreen::Sub_001EECA8() {
    hp2::not_implemented("0x001EECA8", "tOptionsScreen::Sub_001EECA8 (signature and full body unresolved)");
}


} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001EFA68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001efa68.md.
void AudioOptionsScreen::FindSlider() {
    hp2::not_implemented("0x001EFA68", "AudioOptionsScreen::FindSlider (signature and full body unresolved)");
}
} // namespace hp2::frontend

namespace hp2::recovered {
// ELF 0x001F1A78; E-0041-R20; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f1a78.md.
void LanguageOptionsScreenBase::Sub_001F1A78() {
    hp2::not_implemented("0x001F1A78", "LanguageOptionsScreenBase::Sub_001F1A78 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F4DC8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f4dc8.md.
void OptionsScreenBase::Sub_001F4DC8() {
    hp2::not_implemented("0x001F4DC8", "OptionsScreenBase::Sub_001F4DC8 (signature and full body unresolved)");
}

// ELF 0x001F4E68; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001f4e68.md.
void OptionsScreenBase::HandleHighlight() {
    hp2::not_implemented("0x001F4E68", "OptionsScreenBase::HandleHighlight (signature and full body unresolved)");
}

// ELF 0x001F5B80; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f5b80.md.
void OptionsScreenBase::HandleEvent() {
    hp2::not_implemented("0x001F5B80", "OptionsScreenBase::HandleEvent (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001F1528; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f1528.md.
void UserNameOptionsScreen::DisplayGender() {
    hp2::not_implemented("0x001F1528", "UserNameOptionsScreen::DisplayGender (signature and full body unresolved)");
}
} // namespace hp2::recovered

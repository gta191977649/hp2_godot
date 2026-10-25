#include "selection_locked.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cstring>

namespace port::selection {
namespace {
// Original hash operands00192770..001929D0; no inferred authored names.
constexpr std::uint32_t group=0x6a096871,master=0xa8af730b,overlay=0xbea3c79c;
constexpr std::uint32_t text=0x2a67e2d5,rectangle=0x2a65daf9;
void visibility(hp2::frontend::FeScreen& screen,std::uint32_t hash,bool visible){
    hp2::frontend::SetWidgetVisible_00144968(screen,hash,visible);
}
int signed_word(std::uint32_t word){
    return word<0x80000000u?int(word):int(std::int64_t(word)-0x100000000ll);
}
void copy_blurb(std::array<char,256>& out,const std::string& value){
    if(value.size()>=out.size())hp2::not_implemented("0x00244A68","formatted unlock text exceeds original256-byte buffer");
    std::memcpy(out.data(),value.c_str(),value.size()+1);
}
}
// Debug00314510/00314740, E-0098-L02. Caller supplies the SHA-checked Debug
// memory image; no Retail table layout or baked proprietary table is used.
UnlockTables decode_unlock_tables(const port::driving::Memory& memory){
    UnlockTables out;
    for(std::uint32_t i=0;i<out.tasks.size();++i){
        const auto address=0x314510u+i*56;
        out.tasks[i]={signed_word(memory.word(address+36)),signed_word(memory.word(address+40)),signed_word(memory.word(address+44))};
    }
    for(std::uint32_t i=0;i<out.points.size();++i){
        const auto address=0x314740u+i*12;
        out.points[i]={signed_word(memory.word(address)),signed_word(memory.word(address+4))};
    }
    return out;
}
// Debug00244A68,00244490/4E8,002446C0/700,001FA990/AA58/AE10/AED0,
// E-0098-L02. Preserve first-match task/tree/event/reward ordering and priority.
void FormatUnlockBlurbForCar(std::array<char,256>& out,int car_type,int game_type,
    int player_number,const UnlockBlurbServices& svc){
    const auto localized=[&](int id,std::optional<int> argument){
        if(!svc.translated_format)hp2::not_implemented("0x00244A68","original localized unlock text formatter missing");
        copy_blurb(out,svc.translated_format(id,argument));
    };
    for(const auto& task:svc.tables.tasks){
        if((task.UnlockedInGameType==-1 || task.UnlockedInGameType==game_type) && task.Reward==car_type){
            localized(task.CarSelectUnlockBlurb,std::nullopt);return;
        }
    }
    if(game_type==7 && player_number==0){
        copy_blurb(out,"Be A cop Player 1 -- should not see this message");return;
    }
    for(const auto& tree:svc.trees){
        for(const auto& event:tree.pEvents){
            for(int i=0;i<event.nRewards;++i){
                const auto* reward=event.GetReward(tree,i);
                if(!reward)hp2::not_implemented("0x001FAA58","original event reward pointer missing");
                if(reward->Field_0x00[0]!=0 || int(reward->Field_0x00[1])!=car_type)continue;
                if(tree.nType==0)localized(0xc030,int(event.nNumber));
                else if(tree.nType==1)localized(0xc031,int(event.nNumber));
                else copy_blurb(out,"Unknown event tree type");
                return;
            }
        }
    }
    for(const auto& points:svc.tables.points){
        if(points.Reward==car_type){localized(0xbfb8,points.RequiredPoints);return;}
    }
    copy_blurb(out,"This car should be unlocked");
}
// Debug00192770, E-0098-L02. Group AnimationGroup00241C38 saves authored Y,
// starts at640, and retains EndPosition as float before integer publication.
LockedDisplay::LockedDisplay(hp2::frontend::FeScreen& screen):screen_(screen){
    if(auto* w=hp2::frontend::FindWidget_00143D50(screen_,master,0)){
        EndPosition=w->x;saved_y_=port::driving::cvt_w_s(w->y);
    }
    if(auto* w=hp2::frontend::FindWidget_00143D50(screen_,overlay,0))
        MaxAlpha=w->a<128?int(w->a):int(w->a)-256;
    Spline.duration=.5f;Spline.flags=1;
    setup_slide(640.f,EndPosition);
}
// Debug00241DB8, E-0098-L02; grouped branch only, using original saved Y.
void LockedDisplay::setup_slide(float start,float end){
    Spline.flags=0;Spline.state=0;Spline.Val=start;Spline.ValDesired=end;
    Spline.dVal=Spline.dValDesired;
    if(start!=end)Spline.state=2;
    publish_slide();
}
// Debug001450F8 via00244110, E-0098-L02. Capture master position before
// traversing backwards; offsets are relative to the current master, not authored X.
void LockedDisplay::publish_slide(){
    const auto* anchor=hp2::frontend::FindWidget_00143D50(screen_,master,0);
    if(!anchor)return;
    const float dx=float(port::driving::cvt_w_s(Spline.Val))-anchor->x;
    const float dy=float(saved_y_)-anchor->y;
    for(auto i=screen_.widgets.rbegin();i!=screen_.widgets.rend();++i){
        if(i->class_hash!=group)continue;
        const float x=i->x+dx,y=i->y+dy;
        if(i->x!=x){i->x=x;i->dirty_0x2b=4;}
        if(i->y!=y){i->y=y;i->dirty_0x2b=4;}
    }
}
// Debug00192800, E-0098-L02. Identity gates all formatting/reset work; unlock
// flag changes on the same VisibleCar do not reset or refresh this displayer.
void LockedDisplay::SetCurrentCar(hp2::debug::VisibleCar* car,int unlock_all,
    int player_number,int game_type,std::uint32_t real_timer,const LockedDisplayServices& svc){
    if(CurrentCar==car)return;
    if(!car)hp2::not_implemented("0x00192800","original locked display requires a VisibleCar");
    CurrentCar=car;
    const bool locked=!car->CarUnlocked && !unlock_all;
    // Debug001452D8 group visibility writes only Visible, without dirty mutation.
    for(auto& w:screen_.widgets)if(w.class_hash==group)w.visible=locked?1:0;
    visibility(screen_,overlay,locked);visibility(screen_,text,false);
    if(!svc.format_unlock_blurb)
        hp2::not_implemented("0x00244A68","original unlock description formatter binding missing");
    svc.format_unlock_blurb(UnlockBlurb,car->eCarType,game_type,player_number);
    if(std::find(UnlockBlurb.begin(),UnlockBlurb.end(),'\0')==UnlockBlurb.end())
        hp2::not_implemented("0x00244A68","unlock description exceeds original256-byte buffer");
    setup_slide(650.f,EndPosition);ChangedTime=real_timer;
}
// Debug001928F8 and00241EB8, E-0098-L02. SUBU wraps before signed CVT.S.W;
// strict delay gate, real elapsed slide step, independent absolute-time alpha.
void LockedDisplay::Update(std::uint32_t real_timer,float real_time_elapsed){
    const auto bits=real_timer-ChangedTime;
    const std::int64_t ticks=bits<0x80000000u?std::int64_t(bits):std::int64_t(bits)-0x100000000ll;
    const float elapsed=float(ticks)*0x1.0624dep-12f; //003255A0 bits3983126F
    float factor=(elapsed-.4f)/.5f;
    if(.4f<elapsed){hp2::debug::cPoint::SplineSeek(Spline,real_time_elapsed,0,0);publish_slide();}
    else factor=0;
    const int alpha=factor<1.f?port::driving::cvt_w_s(float(MaxAlpha)*factor):MaxAlpha;
    if(auto* w=hp2::frontend::FindWidget_00143D50(screen_,overlay,0))hp2::frontend::SetAlpha(*w,alpha);
}
// Debug001929D0, E-0098-L02. Authored text object remains hidden; original
// rectangle renderer must be bound at the owning frame stage.
void LockedDisplay::Render(int unlock_all,const LockedDisplayServices& svc){
    if(!CurrentCar)hp2::not_implemented("0x001929D0","original locked display has no current car");
    if(CurrentCar->CarUnlocked || unlock_all)return;
    visibility(screen_,text,false);
    if(!svc.render_text)hp2::not_implemented("0x001929D0","original rectangle text renderer binding missing");
    const auto end=std::find(UnlockBlurb.begin(),UnlockBlurb.end(),'\0');
    if(end==UnlockBlurb.end())hp2::not_implemented("0x001929D0","unlock description exceeds original256-byte buffer");
    svc.render_text(screen_.name_hash_0x08,text,rectangle,
        std::string_view(UnlockBlurb.data(),std::size_t(end-UnlockBlurb.begin())),1.f);
}
}

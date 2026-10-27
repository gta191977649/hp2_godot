#include "game_core/port/frontend/selection_locked.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/memory.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(float actual,float expected,const char* message){require(std::abs(actual-expected)<.0001f,message);}
hp2::frontend::FeWidget widget(std::uint32_t hash,std::uint32_t group,float x,float y,int alpha=128){
    hp2::frontend::FeWidget w;w.name_hash=hash;w.class_hash=group;w.type=2;
    w.x=x;w.y=y;w.a=std::uint8_t(alpha);w.visible=1;return w;
}
}
int main(){
    {
        port::driving::Memory memory;memory.map(0x314510,560);memory.map(0x314740,228);
        for(unsigned i=0;i<10;++i){memory.word(0x314510+i*56+36,0xffffffffu);memory.word(0x314510+i*56+40,100+i);memory.word(0x314510+i*56+44,1000+i);}
        for(unsigned i=0;i<19;++i){memory.word(0x314740+i*12,5000+i);memory.word(0x314740+i*12+4,200+i);}
        auto tables=port::selection::decode_unlock_tables(memory);
        require(tables.tasks[9].Reward==109 && tables.tasks[9].UnlockedInGameType==-1
            && tables.tasks[9].CarSelectUnlockBlurb==1009 && tables.points[18].Reward==218
            && tables.points[18].RequiredPoints==5018,"original table address/stride/signed decode differs");
        std::vector<hp2::debug::EventTree> trees(2);
        for(auto& tree:trees){
            hp2::debug::EventReward reward;reward.Field_0x00[0]=0;reward.Field_0x00[1]=42;
            tree.pRewards.push_back(reward);hp2::debug::EventInfo event;
            event.nRewards=1;event.aRewards[0]=0;event.nNumber=-7;tree.pEvents.push_back(event);
        }
        trees[0].nType=1;trees[1].nType=0;
        int translated=0,last_id=0;std::optional<int> last_arg;
        port::selection::UnlockBlurbServices svc{tables,trees,[&](int id,std::optional<int> arg){
            ++translated;last_id=id;last_arg=arg;return std::string("formatted fixture");
        }};
        std::array<char,256> out{};
        tables.tasks[0]={9,42,888};tables.tasks[1]={-1,42,999};
        port::selection::FormatUnlockBlurbForCar(out,42,9,0,svc);
        require(last_id==888 && !last_arg,"task must precede event/points and retain first row");
        port::selection::FormatUnlockBlurbForCar(out,42,7,0,svc);
        require(last_id==999 && translated==2,"wildcard task must precede BAC special message");
        tables.tasks[1].Reward=-1;
        port::selection::FormatUnlockBlurbForCar(out,42,7,0,svc);
        require(std::string(out.data())=="Be A cop Player 1 -- should not see this message"
            && translated==2,"BAC player zero must precede event lookup");
        port::selection::FormatUnlockBlurbForCar(out,42,7,1,svc);
        require(last_id==0xc031 && last_arg==-7,"event tree order/signed event number differs");
        tables.tasks[0].Reward=-1;trees[0].nType=4;
        port::selection::FormatUnlockBlurbForCar(out,42,9,1,svc);
        require(std::string(out.data())=="Unknown event tree type","unknown first matching tree incorrectly fell through");
        trees[0].pRewards[0].Field_0x00[0]=1;
        port::selection::FormatUnlockBlurbForCar(out,42,9,1,svc);
        require(last_id==0xc030 && last_arg==-7,"noncar reward matched or later tree lost");
        tables.points[0]={1234,42};trees[1].pRewards[0].Field_0x00[1]=43;
        port::selection::FormatUnlockBlurbForCar(out,42,9,1,svc);
        require(last_id==0xbfb8 && last_arg==1234,"point reward description/value differs");
        port::selection::FormatUnlockBlurbForCar(out,99,9,1,svc);
        require(std::string(out.data())=="This car should be unlocked","nonreward fallback differs");
        svc.translated_format={};bool failed=false;
        try{port::selection::FormatUnlockBlurbForCar(out,42,9,1,svc);}catch(const hp2::NotImplemented&){failed=true;}
        require(failed,"missing original localization must fail");
        svc.translated_format=[](int,std::optional<int>){return std::string(256,'x');};failed=false;
        try{port::selection::FormatUnlockBlurbForCar(out,42,9,1,svc);}catch(const hp2::NotImplemented&){failed=true;}
        require(failed,"unbounded localized formatter overwrote original buffer");
    }
    {
        hp2::frontend::FeScreen restored;
        restored.widgets={widget(0xa8af730b,0x6a096871,80.5f,15.75f),widget(9,0x6a096871,100.5f,35.75f),widget(10,0,4,5)};
        {port::selection::LockedDisplay temporary(restored);restored.widgets[0].visible=0;restored.widgets[1].a=23;}
        require(restored.widgets[0].x==80 && restored.widgets[0].y==15
            && restored.widgets[1].x==100 && restored.widgets[1].y==35,
            "locked group destructor failed saved integer coordinates");
        require(!restored.widgets[0].visible && restored.widgets[1].a==23 && restored.widgets[2].x==4,
            "locked group destructor invented visibility/alpha/unrelated restoration");
    }
    hp2::frontend::FeScreen screen;screen.name_hash_0x08=7;
    screen.widgets={widget(0xa8af730b,0x6a096871,80.5f,15.75f),
        widget(9,0x6a096871,100.5f,35.75f),widget(0xbea3c79c,0,0,0,200),
        widget(0x2a67e2d5,0,0,0),widget(10,0,4,5)};
    port::selection::LockedDisplay display(screen);
    require(display.MaxAlpha==-56,"original signed cached overlay alpha lost");
    near(display.EndPosition,80.5f,"constructor discarded fractional endpoint");
    near(screen.widgets[0].x,640,"constructor initial slide start differs");
    near(screen.widgets[1].x,660,"constructor lost relative group offset");
    near(screen.widgets[0].y,15,"constructor did not publish saved integer Y");
    near(screen.widgets[4].x,4,"group slide moved unrelated object");
    hp2::debug::CarTypeInfo info;hp2::debug::CarStatus status;
    hp2::debug::VisibleCar locked(3,status,0,info),unlocked(4,status,0,info);
    locked.CarUnlocked=false;unlocked.CarUnlocked=true;
    port::selection::LockedDisplayServices svc;int formats=0,renders=0;
    svc.format_unlock_blurb=[&](auto& out,int car,int game,int player){
        require((car==3 || car==4) && game==9 && player==1,"formatter semantic arguments changed");
        std::strcpy(out.data(),"original localized fixture");++formats;
    };
    svc.render_text=[&](auto scr,auto text,auto rect,std::string_view value,float scale){
        require(scr==7 && text==0x2a67e2d5 && rect==0x2a65daf9 &&
            value=="original localized fixture" && scale==1,"original rectangle submission differs");++renders;
    };
    display.SetCurrentCar(&locked,0,1,9,0xfffffff0u,svc);
    require(formats==1 && screen.widgets[0].visible && !screen.widgets[3].visible,"locked presentation visibility differs");
    near(screen.widgets[0].x,650,"change reset must start650, distinct from constructor640");
    display.Update(0xfffffff0u+1600u,.25f); // exactly original .4 second delay
    near(screen.widgets[0].x,650,"slide advanced at delay equality");
    require(screen.widgets[2].a==0,"overlay must be zero through delay equality");
    display.Update(0xfffffff0u+2600u,.25f);
    near(screen.widgets[0].x,365,"first quarter-second step lost original cubic/integer projection");
    require(screen.widgets[2].a==228,"half fade lost original negative alpha wrapping");
    const auto position=screen.widgets[0].x;
    display.SetCurrentCar(&locked,1,1,9,7000,svc);
    require(formats==1 && display.ChangedTime==0xfffffff0u && screen.widgets[0].x==position
        && screen.widgets[0].visible,"same identity unexpectedly refreshed unlock override/timer");
    display.Update(0xfffffff0u+3600u,.25f);
    near(screen.widgets[0].x,80,"arrival did not publish integer endpoint");
    near(screen.widgets[1].x,100,"repeated slide lost group offsets");
    require(screen.widgets[2].a==200,"completed fade did not restore cached signed alpha bits");
    display.Render(0,svc);display.Render(1,svc);require(renders==1,"unlock override did not suppress rectangle draw");
    locked.CarUnlocked=true;display.Render(0,svc);require(renders==1,"live car unlock did not suppress draw");
    display.SetCurrentCar(&unlocked,0,1,9,9000,svc);
    require(formats==2 && !screen.widgets[0].visible && !screen.widgets[2].visible,
        "unlocked new identity must still format/reset while hiding lock display");
    display.Update(8999,.25f); // signed elapsed negative, never enormous unsigned duration
    require(screen.widgets[2].a==0 && screen.widgets[0].x==650,"packed signed timer subtraction differs");
    locked.CarUnlocked=false;
    display.SetCurrentCar(&locked,0,1,9,10000,svc);
    port::selection::LockedDisplayServices absent;
    bool failed=false;try{display.Render(0,absent);}catch(const hp2::NotImplemented&){failed=true;}
    require(failed,"missing original rectangle renderer silently ignored");
    failed=false;try{display.SetCurrentCar(&unlocked,0,1,9,11000,absent);}catch(const hp2::NotImplemented&){failed=true;}
    require(failed,"missing original unlock formatter silently ignored");
    hp2::frontend::FeScreen empty;port::selection::LockedDisplay no_widgets(empty);
    require(no_widgets.MaxAlpha==0 && no_widgets.EndPosition==0,"original missing widget getters must yield zero");
    std::puts("PASS original locked display timing, identity, group movement, fade and required services");
}

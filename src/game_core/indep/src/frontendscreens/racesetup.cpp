#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "Fred.hpp"
#include "Language.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <cstring>
#include <vector>

namespace hp2 {
namespace {
constexpr std::uint32_t FirstMenu=0x52e039f3u;
constexpr std::uint32_t PlayersValue=0x164eb99au;
constexpr std::uint32_t TypeValue=0x15dd325eu;
constexpr std::uint32_t SkillValue=0x53a607f5u;
constexpr std::uint32_t Groups[]={0x6e34e350u,0x708a0bf1u,0x72df3492u,0x75345d33u,
    0x778985d4u,0x79deae75u,0x7c33d716u,0x7e88ffb7u};
constexpr std::uint32_t Items[]={FirstMenu,FirstMenu+1,FirstMenu+2,FirstMenu+3,
    FirstMenu+4,FirstMenu+5,FirstMenu+6,FirstMenu+7};
constexpr std::uint32_t Values[]={0x69a2c757u,0x69a2c758u,0x69a2c759u,0x69a2c75au,
    0x69a2c75bu,0x69a2c75cu,0x69a2c75du,0x69a2c75eu};
constexpr std::int32_t Skills[]={0x753f,0x7540,0x7541};
constexpr std::int32_t ChallengeHelp[]={800003,800004,800005,800006,0,0};
constexpr std::int32_t HotPursuitHelp[]={805000,805001,805002,805003,0,0};
constexpr std::int32_t SingleHelp[]={801002,800007,800008,800009,0,0};
constexpr std::int32_t KnockoutHelp[]={800010,800011,800012,800013,0,0};
constexpr std::int32_t FreeHelp[]={800014,800015,800016,800017,0,0};
constexpr std::int32_t CopHelp[]={800600,800601,800602,800603,0,0};

MainMenuScreenServices* services{};

// PAL1B5C78. These are the source's endpoint wraps, not a modulo policy.
std::int32_t ValueChanger(std::int32_t value,std::int32_t delta,
    std::int32_t minimum,std::int32_t maximum)
{
    value+=delta;
    if(value>maximum)value=minimum;
    if(value<minimum)value=maximum;
    return value;
}
bool BothControllers(const MainMenuScreenServices& s)
{
    return joystickIsPluggedIn(s.screens->input->router,0) &&
        joystickIsPluggedIn(s.screens->input->router,1);
}
bool MaybeChangeNumPlayers(const MainMenuScreenServices& s,FrontendChallengeSetup& data,
    std::int32_t desired)
{
    // PAL1B5C90 checks port1; MakeJoystickArrowsRight separately tests both.
    if(desired<=0)desired=2;
    else if(desired>=3)desired=1;
    if(!joystickIsPluggedIn(s.screens->input->router,1))desired=1;
    const bool changed=desired!=data.playerCount;data.playerCount=desired;return changed;
}
float FirstItemX(MainMenuScreenServices& s,std::uint32_t screen)
{
    auto* object=s.ui->resources->FindObject(screen,Groups[0]);
    Require(object,"Race setup has no authored first item anchor");
    float x,y;s.ui->resources->GetTopLeft(*object,x,y,s.ui->language);return x;
}
void SetupHighlight(MainMenuScreenServices& s,std::uint32_t screen,std::uint32_t key)
{
    // PAL1B63F0/1B7D50; source globals331A00=-3,3319FC=6.
    std::int32_t left,top,right,bottom;
    Require(s.ui->resources->GetBoundingBox(screen,key,left,top,right,bottom,s.ui->language),
        "Race setup highlight has no authored bounding rectangle");
    if(auto* lower=s.ui->resources->FindObject(screen,0x32c91daeu))
        lower->SetTopLeftY(static_cast<float>(bottom)-3.0f);
    if(auto* upper=s.ui->resources->FindObject(screen,0xc671680cu))
        upper->SetTopLeftY(static_cast<float>(top)-6.0f);
}
void SetItemState(MainMenuScreenServices& s,const char* name,std::uint32_t screen,
    std::uint32_t key,std::int32_t state)
{
    // Original ScreenList16D2A8 state0=disabled,1=enabled,2=hidden.
    auto* node=s.screens->FindScreen(name);
    Require(node,"Race setup configuration is not registered");
    auto* child=node->FindFlailerScreen(static_cast<std::int32_t>(screen));
    Require(child && child->menu,"Race setup has no actual menu owner");
    auto* item=child->menu->FindItem(key);
    if(!item)return; // The source permits an absent authored menu item.
    if(state==1)item->SetMenuItemState(1);else item->ClearMenuItemState(1);
    if(state!=0)s.ui->resources->SetObjectVisibility(screen,key,state==1);
}
void StartTheRightMovie(MainMenuScreenServices& s)
{
    // PAL1B5DE0 includes the authored space in worldracing_ movie.AVI.
    s.screens->StartMovie(s.frontend->copsEnabled ? "MOVIES\\hotpursuit_movie.AVI" :
        "MOVIES\\worldracing_ movie.AVI");
}
void MakeCarsAvailable(MainMenuScreenServices& s)
{
    const auto& setup=*s.raceSetup;
    s.frontend->MakeChallengeCarsAvailable(*setup.cars,*setup.events,*setup.demoDisc);
}
std::unique_ptr<FlailerExitScreenAnimation> MakeExit(MainMenuScreenServices& s,
    std::uint32_t screen,const char* target,bool push)
{
    auto exit=std::make_unique<FlailerExitScreenAnimation>(*s.ui,*s.screens,screen,target,push);
    auto* anchor=s.ui->resources->FindObject(screen,0x32c91daeu);
    Require(anchor,"Race setup exit lacks its authored lower rail anchor");
    float x,y;s.ui->resources->GetTopLeft(*anchor,x,y,s.ui->language);
    exit->AddGroup(0x97bf4b25u,0x32c91daeu,x,650.0f,exit->MaximumDuration());
    return exit;
}

// Concrete PAL Challenge/BeACopPreSetup owners share only the native storage
// arrangement below. The boolean selects the two individually reviewed
// source callback policies, including their different difficulty cells.
class ChallengeScreen final {
public:
    ChallengeScreen(MainMenuScreenServices& s,std::uint32_t screen,bool cop)
        : s_(s),screen_(screen),cop_(cop),data_(s.frontend->challengeSetup),
          blurb_(*s.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu),
          slide_(*s.ui,screen,FirstItemX(s,screen)),
          highlight_(*s.ui,screen,0xab671c9au,s.ui->animationSettings.highlightColor),
          valueHighlight_(*s.ui,screen,0x1042eac3u,-1),
          arrows_(*s.ui,screen,0xd1cda594u,0x9345e4a7u)
    {
        selection_.showOtherItems=1;
        selection_.AddItems(*s.ui->resources,screen,Groups,cop?3:5);
        if(!cop && !s.frontend->recordDisplaySuppressed && data_.gameType>=3 && data_.gameType<=6)
            typeIndex_=data_.gameType-3;
        MakeJoystickArrowsRight();ResyncChangeables();StartTheRightMovie(s_);
        s_.screens->SetHelpOverride((cop_ || s_.frontend->copsEnabled)?HotPursuitHelp:nullptr);
        s_.frontend->playerCount=data_.playerCount;
        s_.frontend->gameType=cop_?7:data_.gameType;
        MakeCarsAvailable(s_);
        // PAL183EC0 is JR RA/NOP, retired without a native placeholder.
    }
    ~ChallengeScreen()
    {
        exit_.reset();
        if(!cop_)MakeCarsAvailable(s_);
        s_.frontend->playerCount=data_.playerCount;
        if(cop_)s_.frontend->gameType=7;
        s_.frontend->challengeSetup=data_;
        s_.screens->SetHelpOverride(nullptr);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& event)
    {
        arrows_.HandleEvent(message,event);
        if(message==ScreenMessage::Tick) {
            slide_.Animate();highlight_.Animate();valueHighlight_.Animate();blurb_.Animate();
            if(current_==FirstMenu) {
                arrows_.SetCurrentItem(BothControllers(s_)?PlayersValue:0);
                if(!BothControllers(s_) && MaybeChangeNumPlayers(s_,data_,1)) {
                    ResyncChangeables();ResyncBlurb(current_);
                }
            }
            if(!BothControllers(s_) && MaybeChangeNumPlayers(s_,data_,1))ResyncChangeables();
        }
        if(exit_){exit_->HandleMessages(message,event);return;}
        if(message==ScreenMessage::Highlight)OnHighlight(event.key);
        else if(message==ScreenMessage::Left || message==ScreenMessage::Right)OnScroll(message,event);
        else if(message==ScreenMessage::Select)OnSelect(event);
    }
private:
    void MakeJoystickArrowsRight()
    {
        if(!BothControllers(s_)){arrows_.SetCurrentItem(0);MaybeChangeNumPlayers(s_,data_,1);}
    }
    const char* Name() const {return cop_?"ChallengeBeACop":"Challenge";}
    void ResyncChangeables()
    {
        auto& r=*s_.ui->resources;
        if(!cop_ && data_.gameType>=3 && data_.gameType<=6) {
            constexpr std::int32_t names[]={0x4e25,0x4e27,0x4e28,0x4e26};
            r.SetLanguageID(screen_,TypeValue,names[data_.gameType-3]);
        }
        if(data_.playerCount==1 || data_.playerCount==2)
            r.SetLanguageID(screen_,PlayersValue,data_.playerCount==1?0x4e23:0x4e24);
        const auto skill=cop_?data_.copSkill:data_.opponentSkill;
        Require(skill>=0 && skill<3,"Race setup difficulty is outside its three authored labels");
        r.SetLanguageID(screen_,SkillValue,Skills[skill]);
        const bool enabled=cop_?data_.playerCount!=2:(data_.gameType!=6 || s_.frontend->copsEnabled);
        SetItemState(s_,Name(),screen_,FirstMenu+(cop_?1:2),enabled?1:0);
        r.SetObjectVisibility(screen_,SkillValue,enabled);
    }
    void ResyncBlurb(std::uint32_t key)
    {
        std::int32_t language=0;
        if(key==FirstMenu)language=data_.playerCount==1?0x4a38:data_.playerCount==2?0x4a39:0;
        else if(!cop_ && key==FirstMenu+1) {
            constexpr std::int32_t texts[]={0x4e2a,0x4e2d,0x4e2c,0x4e2b};
            if(data_.gameType>=3 && data_.gameType<=6)language=texts[data_.gameType-3];
        } else if(key==FirstMenu+(cop_?1:2)) {
            const auto skill=cop_?data_.copSkill:data_.opponentSkill;
            if(skill>=0 && skill<3)language=(!cop_ && s_.frontend->copsEnabled?0x4a3e:0x4a3a)+skill;
        } else if(key==FirstMenu+(cop_?2:3))language=0x4a3d;
        blurb_.NewText(language);
    }
    void OnHighlight(std::uint32_t key)
    {
        SetupHighlight(s_,screen_,key);highlight_.SetCurrentMenuItem(key);
        current_=key;ResyncBlurb(key);
        if(key<FirstMenu || key>FirstMenu+(cop_?2u:3u))return;
        const auto index=key-FirstMenu;
        const auto value=index==0?PlayersValue:index==(cop_?1u:2u)?SkillValue:
            !cop_ && index==1?TypeValue:0;
        arrows_.SetCurrentItem(value);if(index==0)MakeJoystickArrowsRight();
        selection_.SetCurrentItem(Groups[index]);valueHighlight_.SetCurrentMenuItem(value);
    }
    void OnScroll(ScreenMessage message,ScreenCallbackData& event)
    {
        const auto delta=message==ScreenMessage::Left?-1:1;
        bool changed=false;std::uint32_t value=0;
        if(event.key==FirstMenu) {
            changed=MaybeChangeNumPlayers(s_,data_,data_.playerCount+delta);value=PlayersValue;
        } else if(!cop_ && event.key==FirstMenu+1 && !s_.frontend->recordDisplaySuppressed) {
            const auto index=ValueChanger(typeIndex_,delta,0,3);
            changed=data_.gameType!=index+3;
            if(changed){typeIndex_=index;data_.gameType=index+3;ResyncChangeables();}
            value=TypeValue;
        } else if(event.key==FirstMenu+(cop_?1:2)) {
            auto& skill=cop_?data_.copSkill:data_.opponentSkill;
            const auto next=ValueChanger(skill,delta,0,2);changed=next!=skill;skill=next;value=SkillValue;
        }
        event.sound=changed?(message==ScreenMessage::Left?1:2):0;
        if(changed){valueHighlight_.SetCurrentMenuItem(value);ResyncChangeables();ResyncBlurb(event.key);}
    }
    void OnSelect(ScreenCallbackData& event)
    {
        if(!((data_.playerCount==1 && event.joystickPort==0)||data_.playerCount==2))return;
        auto& f=*s_.frontend;f.challengeSetup=data_;f.playerCount=data_.playerCount;
        f.gameType=(cop_ || f.recordDisplaySuppressed)?7:data_.gameType;
        auto& race=*s_.raceSetup->parameters;race.mode=data_.playerCount;
        race.authoredRaceOption254=data_.raceOption254;
        const char* target=nullptr;
        if(event.key==FirstMenu+(cop_?2:3)) {
            switch(f.gameType) {
            case 3:target="Single Race Setup";break;case 4:target="Knockout Setup";break;
            case 5:target="Lap Knockout Setup";break;case 6:target="Free Run Setup";break;
            case 7:target="Be The Cop Setup";break;
            default:Require(false,"Challenge selects no original race setup mode");
            }
        } else {
            if(cop_ && event.key!=FirstMenu && event.key!=FirstMenu+1)target="";
            else {
                target=data_.playerCount==2?"Select Car2P":"Select Car";
                if(cop_)MakeCarsAvailable(s_);
                f.SetupRaceParametersAfterRaceSetup(*s_.raceSetup);
            }
        }
        exit_=MakeExit(s_,screen_,target,true);
    }
    MainMenuScreenServices& s_;
    std::uint32_t screen_{};
    bool cop_{};
    FrontendChallengeSetup data_;
    std::int32_t typeIndex_{-1};
    std::uint32_t current_{};
    FlailerSelectionSet selection_;
    FlailerAnimatedBlurb blurb_;
    FlailerSlideInAnimation slide_;
    FlailerHighlightAnimation highlight_,valueHighlight_;
    std::unique_ptr<FlailerExitScreenAnimation> exit_;
    FlailerLeftRightArrowScroll arrows_;
};

class RaceSetupScreen final {
    enum class Change {Grid,Count,OpponentType,Catchup,Traffic,Track,Damage};
    struct Changeable {
        Change operation;
        std::int32_t language,blurb;
        std::int32_t* value;
    };
public:
    RaceSetupScreen(MainMenuScreenServices& s,std::uint32_t screen,std::int32_t type)
        : s_(s),screen_(screen),type_(type),
          slide_(*s.ui,screen,FirstItemX(s,screen)),
          highlight_(*s.ui,screen,0xab671c9au,s.ui->animationSettings.highlightColor),
          valueHighlight_(*s.ui,screen,0x1042eac3u,-1),
          scroll_(*s.ui,screen,FirstMenu,FirstMenu+6,0xedf2e4eeu,0x20d73d01u,0xc67cfb4au),
          arrows_(*s.ui,screen,0xd1cda594u,0x9345e4a7u),
          blurb_(*s.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        // PAL1B8390 plus the five concrete constructor bodies. Value storage
        // copies the source screen's saved options; the live profile stays
        // canonical and receives them at the original destructor boundary.
        StartTheRightMovie(s_);
        for(auto key:{0xd1cda594u,0x9345e4a7u})
            if(auto* b=static_cast<FlailerBitmapChunk*>(s.ui->resources->FindObject(screen,key,2)))
                b->clipObjectKey=0xc67cfb4au;
        LoadOptions();AttachOptions();BuildMenu();ResyncChangeables();
    }
    ~RaceSetupScreen()
    {
        SaveOptions();
        s_.frontend->SetupRaceParametersAfterRaceSetup(*s_.raceSetup);
        // PAL1B93B0 refreshes the actual profiles after the cop options commit.
        if(type_==7)MakeCarsAvailable(s_);
        s_.screens->SetHelpOverride(nullptr);exit_.reset();
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& event)
    {
        if(message==ScreenMessage::Tick){highlight_.Animate();valueHighlight_.Animate();}
        arrows_.HandleEvent(message,event);
        if(exit_){exit_->HandleMessages(message,event);return;}
        scroll_.HandleEvent(message,event);
        switch(message) {
        case ScreenMessage::Previous:if(event.key==FirstMenu)event.value=1;break;
        case ScreenMessage::Next:
            if(Index(event.key)>=rows_.size()-1)event.value=1;
            break;
        case ScreenMessage::Left:case ScreenMessage::Right: {
            const auto index=Index(event.key);if(index>=rows_.size())break;
            auto& row=rows_[index];
            const bool changed=MaybeChange(row,message==ScreenMessage::Left?-1:1);
            event.sound=changed?(message==ScreenMessage::Left?1:2):0;
            if(changed){Refresh(row,Values[index]);valueHighlight_.ForceStart();}
            break;
        }
        case ScreenMessage::Highlight: {
            const auto index=Index(event.key);Require(index<rows_.size(),"Race setup highlights an unconfigured source row");
            blurb_.NewText(rows_[index].blurb);current_=event.key;
            arrows_.SetCurrentItem(Values[index]);
            const auto mask=scroll_.GetCurrentMenuMask();
            selection_.SetCurrentItem(mask);SetupHighlight(s_,screen_,mask);
            highlight_.SetCurrentMenuItem(Items[index]);valueHighlight_.SetCurrentMenuItem(Values[index]);
            break;
        }
        case ScreenMessage::Tick:
            blurb_.Animate();if(!slide_.IsFinished())slide_.Animate();
            break;
        case ScreenMessage::Select:
            // The source advanced menu does not push another stack frame.
            exit_=MakeExit(s_,screen_,s_.frontend->playerCount==2?"Select Car2P":"Select Car",false);
            arrows_.SetCurrentItem(0);break;
        default:break;
        }
    }
private:
    const char* Name() const
    {
        switch(type_) {
        case 3:return "Single Race Setup";case 4:return "Knockout Setup";
        case 5:return "Lap Knockout Setup";case 6:return "Free Run Setup";
        case 7:return "Be The Cop Setup";
        default:throw std::runtime_error("Unknown original setup screen type");
        }
    }
    std::size_t Index(std::uint32_t key) const
    {
        if(key>=FirstMenu && key<FirstMenu+8)return key-FirstMenu;
        return 0; // PAL1B8358 returns the first index when no map key matches.
    }
    template<std::size_t N> void Copy(const std::array<std::int32_t,N>& source)
    {std::copy(source.begin(),source.end(),options_.begin());optionCount_=N;}
    void LoadOptions()
    {
        const auto& f=*s_.frontend;const auto& modes=f.challengeModeSetup;
        switch(type_) {
        case 3:Copy(modes.single);break;
        case 4:Copy(modes.knockout);options_[optionCount_++]=f.knockoutTrackPolicy;break;
        case 5:Copy(modes.lapKnockout);break;
        case 6:Copy(modes.freeRun);break;
        case 7:Copy(modes.beTheCop);break;
        default:Require(false,"Race setup has no source option bank");
        }
        options_[optionCount_++]=f.savedChallengeOptions[static_cast<std::size_t>(type_-3)];
    }
    template<std::size_t N> void Save(std::array<std::int32_t,N>& destination)
    {std::copy_n(options_.begin(),N,destination.begin());}
    void SaveOptions()
    {
        auto& f=*s_.frontend;auto& modes=f.challengeModeSetup;
        switch(type_) {
        case 3:Save(modes.single);break;
        case 4:Save(modes.knockout);f.knockoutTrackPolicy=options_[5];break;
        case 5:Save(modes.lapKnockout);break;
        case 6:Save(modes.freeRun);break;
        case 7:Save(modes.beTheCop);break;
        default:break;
        }
        f.savedChallengeOptions[static_cast<std::size_t>(type_-3)]=options_[optionCount_-1];
    }
    void Attach(Change kind,std::int32_t language,std::int32_t blurb,std::size_t lane)
    {
        Require(lane<optionCount_ && rows_.size()<8,"Race setup changeable exceeds its original option bank");
        rows_.push_back({kind,language,blurb,&options_[lane]});
    }
    void AttachOptions()
    {
        const auto players=s_.frontend->playerCount;const bool cops=s_.frontend->copsEnabled!=0;
        if(type_==3) {
            if(players==2 && !cops) {
                if(options_[0]==1)options_[0]=0;
                Attach(Change::Grid,0x7532,0x7724,0);Attach(Change::OpponentType,0x7533,0x7725,2);
            } else if(players==1) {
                if(!cops && options_[0]==0)options_[0]=1;
                Attach(Change::Grid,0x7532,0x7724,cops?1:0);
                Attach(Change::OpponentType,0x7533,0x7725,2);
            }
            Attach(Change::Catchup,0x7535,0x7726,3);
            Attach(Change::Traffic,0x7536,0x7727,cops?5:4);
            Attach(Change::Damage,0x7595,0x7728,6);
        } else if(type_==4 || type_==5) {
            if(players==1 && !cops)Attach(Change::Count,0x7532,0x7724,0);
            if(!cops)Attach(Change::OpponentType,0x7533,0x7725,1);
            Attach(Change::Catchup,0x7535,0x7726,2);
            Attach(Change::Traffic,0x7536,0x7727,cops?4:3);
            if(type_==4)Attach(Change::Track,0x7537,0x7729,5);
            Attach(Change::Damage,0x7595,0x7728,type_==4?6:5);
        } else if(type_==6) {
            Attach(Change::Traffic,0x7536,0x7727,cops?1:0);
            Attach(Change::Damage,0x7595,0x7728,2);
        } else if(type_==7) {
            Attach(Change::Catchup,0x7535,0x7726,2);
            Attach(Change::Traffic,0x7536,0x7727,3);
            Attach(Change::Damage,0x7595,0x7728,4);
        }
    }
    void BuildMenu()
    {
        // PAL1B85B8 assigns authored labels and line numbers in list order.
        selection_.showOtherItems=1;selection_.AddItems(*s_.ui->resources,screen_,Groups,8);
        for(std::size_t i=0;i<8;++i) {
            SetItemState(s_,Name(),screen_,Items[i],i<rows_.size()?1:2);
            s_.ui->resources->SetObjectVisibility(screen_,Values[i],i<rows_.size());
            if(i<rows_.size())s_.ui->resources->SetLanguageID(screen_,Items[i],rows_[i].language);
        }
        scroll_.SetItemCount(static_cast<std::int32_t>(rows_.size()));
        scroll_.AddObjects(Values,rows_.size());scroll_.AddObjects(Items,rows_.size());
    }
    bool MaybeChange(Changeable& row,std::int32_t delta)
    {
        const auto before=*row.value;auto after=before;const auto players=s_.frontend->playerCount;
        switch(row.operation) {
        case Change::Grid:
            if(players==1){if(before==1)after=2;else if(before==2)after=1;}
            else if(players==2){if(before==0)after=2;else if(before==2)after=0;}
            break;
        case Change::Count:after=ValueChanger(before,delta,players==2?2:3,players==2?6:7);break;
        case Change::OpponentType:case Change::Track:after=ValueChanger(before,delta,0,1);break;
        case Change::Catchup:case Change::Damage:after=before==0;break;
        case Change::Traffic:after=ValueChanger(before,delta,0,3);break;
        }
        *row.value=after;return after!=before;
    }
    void Refresh(const Changeable& row,std::uint32_t key)
    {
        auto& r=*s_.ui->resources;const auto value=*row.value;std::int32_t language=0;
        constexpr std::int32_t grid[]={0x753b,0x753a,0x753c};
        constexpr std::int32_t binary[]={0x753d,0x753e};
        constexpr std::int32_t onoff[]={0x7538,0x7539};
        constexpr std::int32_t traffic[]={0x7542,0x7543,0x7544,0x7538};
        switch(row.operation) {
        case Change::Grid:Require(value>=0 && value<3,"Opponent grid label index is invalid");language=grid[value];break;
        case Change::Count:
            if(value==(s_.frontend->playerCount==2?6:7))language=0x753c;
            else {r.SetLanguageID(screen_,key,0);r.Printf(screen_,key,"%d",value);return;}
            break;
        case Change::OpponentType:case Change::Track:
            Require(value>=0 && value<2,"Race setup binary label index is invalid");language=binary[value];break;
        case Change::Catchup:case Change::Damage:
            Require(value>=0 && value<2,"Race setup switch label index is invalid");language=onoff[value];break;
        case Change::Traffic:
            Require(value>=0 && value<4,"Race setup traffic label index is invalid");language=traffic[value];break;
        }
        r.SetLanguageID(screen_,key,language);
    }
    void ResyncChangeables()
    {for(std::size_t i=0;i<rows_.size();++i)Refresh(rows_[i],Values[i]);}
    MainMenuScreenServices& s_;
    std::uint32_t screen_{};
    std::int32_t type_{};
    std::array<std::int32_t,7> options_{};
    std::size_t optionCount_{};
    std::vector<Changeable> rows_;
    std::uint32_t current_{};
    FlailerSelectionSet selection_;
    FlailerSlideInAnimation slide_;
    FlailerHighlightAnimation highlight_,valueHighlight_;
    std::unique_ptr<FlailerExitScreenAnimation> exit_;
    FlailerScrollingMenuList scroll_;
    FlailerLeftRightArrowScroll arrows_;
    FlailerAnimatedBlurb blurb_;
};

std::unique_ptr<ChallengeScreen> challengeOwner,copOwner;
std::unique_ptr<RaceSetupScreen> setupOwner;

void ChallengeCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(services && data,"Challenge callback lacks live application services");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!challengeOwner,"Challenge received duplicate activation");
        challengeOwner=std::make_unique<ChallengeScreen>(*services,data->key,false);
    } else if(message==ScreenMessage::ScreenDeactivate)challengeOwner.reset();
    else {Require(challengeOwner!=nullptr,"Challenge message is outside its owner lifetime");challengeOwner->HandleEvent(message,*data);}
}
void CopCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(services && data,"BeACop callback lacks live application services");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!copOwner,"BeACop received duplicate activation");
        copOwner=std::make_unique<ChallengeScreen>(*services,data->key,true);
    } else if(message==ScreenMessage::ScreenDeactivate)copOwner.reset();
    else {Require(copOwner!=nullptr,"BeACop message is outside its owner lifetime");copOwner->HandleEvent(message,*data);}
}
void SetupCallback(ScreenMessage message,ScreenCallbackData* data,std::int32_t type)
{
    Require(services && data,"Race setup callback lacks live application services");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!setupOwner,"Race setup received duplicate activation");
        setupOwner=std::make_unique<RaceSetupScreen>(*services,data->key,type);
    } else if(message==ScreenMessage::ScreenDeactivate)setupOwner.reset();
    else {Require(setupOwner!=nullptr,"Race setup message is outside its owner lifetime");setupOwner->HandleEvent(message,*data);}
}
void SingleCallback(ScreenMessage m,ScreenCallbackData* d){SetupCallback(m,d,3);}
void KnockoutCallback(ScreenMessage m,ScreenCallbackData* d){SetupCallback(m,d,4);}
void LapCallback(ScreenMessage m,ScreenCallbackData* d){SetupCallback(m,d,5);}
void FreeCallback(ScreenMessage m,ScreenCallbackData* d){SetupCallback(m,d,6);}
void CopSetupCallback(ScreenMessage m,ScreenCallbackData* d){SetupCallback(m,d,7);}
}

// PAL1B6478..1B64E0, uniquely matched beta ArrowAligner1B38B0.
// GetCenterY144860 uses the integer rectangle from144280 and signed
// truncation of its height/2; SetCenterY1446E0 uses integer object height.
void ArrowAligner(FlailerUiState& ui,std::uint32_t screen,std::uint32_t left,
    std::uint32_t right,std::uint32_t item)
{
    Require(ui.resources,"ArrowAligner has no actual Flailer resources");
    const auto signedBits=[](std::uint32_t bits) {
        std::int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
    };
    std::int32_t x0,y0,x1,y1;
    Require(ui.resources->GetBoundingBox(screen,item,x0,y0,x1,y1,ui.language),
        "ArrowAligner has no authored item rectangle");
    const auto height=signedBits(static_cast<std::uint32_t>(y1)-static_cast<std::uint32_t>(y0));
    const auto center=signedBits(static_cast<std::uint32_t>(y0)+static_cast<std::uint32_t>(height/2));
    for(const auto key:{left,right}) {
        auto* arrow=ui.resources->FindObject(screen,key);
        if(!arrow)continue; // SetCenterY's original absent-object return.
        Require(ui.resources->GetBoundingBox(screen,key,x0,y0,x1,y1,ui.language),
            "ArrowAligner arrow lost its authored rectangle");
        const auto arrowHeight=signedBits(static_cast<std::uint32_t>(y1)-static_cast<std::uint32_t>(y0));
        arrow->SetTopLeftY(static_cast<float>(center)-static_cast<float>(arrowHeight)*0.5f);
    }
}

void BindRaceSetupScreens(MainMenuScreenServices& s)
{
    Require(!challengeOwner && !copOwner && !setupOwner,"Cannot rebind an active RaceSetup owner");
    Require(s.ui && s.ui->resources && s.ui->clock && s.ui->language && s.screens &&
        s.screens->ui==s.ui && s.screens->input && s.screens->input->router && s.frontend &&
        s.raceSetup && s.raceSetup->parameters && s.raceSetup->cars && s.raceSetup->events &&
        s.raceSetup->demoDisc && s.raceSetup->language==s.ui->language,
        "RaceSetup needs the actual shared profile, race, input, UI and catalogue owners");
    services=&s;
}
void UnbindRaceSetupScreens()
{
    setupOwner.reset();copOwner.reset();challengeOwner.reset();services=nullptr;
}
std::array<FlailerScreenConfig,7>& RaceSetupScreenConfigurations()
{
    // Final global constructor1B98E8 supplies these exact authored identities.
    static std::array<FlailerScreenConfig,7> configurations{{
        {"Challenge",ChallengeCallback,{{0x2d62cfe2,0,0,0}},{{-1,-1,-1,-1}},1,ChallengeHelp},
        {"ChallengeBeACop",CopCallback,{{static_cast<std::int32_t>(0xe55f1297u),0,0,0}},{{-1,-1,-1,-1}},1,ChallengeHelp},
        {"Single Race Setup",SingleCallback,{{static_cast<std::int32_t>(0xd1eadbcau),0,0,0}},{{-1,-1,-1,-1}},1,SingleHelp},
        {"Free Run Setup",FreeCallback,{{static_cast<std::int32_t>(0xd1eadbcau),0,0,0}},{{-1,-1,-1,-1}},1,FreeHelp},
        {"Lap Knockout Setup",LapCallback,{{static_cast<std::int32_t>(0xd1eadbcau),0,0,0}},{{-1,-1,-1,-1}},1,KnockoutHelp},
        {"Knockout Setup",KnockoutCallback,{{static_cast<std::int32_t>(0xd1eadbcau),0,0,0}},{{-1,-1,-1,-1}},1,KnockoutHelp},
        {"Be The Cop Setup",CopSetupCallback,{{static_cast<std::int32_t>(0xd1eadbcau),0,0,0}},{{-1,-1,-1,-1}},1,CopHelp}
    }};
    return configurations;
}
} // namespace hp2

#include "EndGameScreens.hpp"
#include "Fred.hpp"
#include "EventTree.hpp"
#include "Language.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cstdio>

namespace hp2 {
// Exact final mutable color owners initialized by EndGameScreens static I.
std::array<std::array<float,4>,2> endGamePlayerColors{{
    {{134.0f,96.0f,48.0f,128.0f}},{{0.0f,128.0f,0.0f,128.0f}}
}};
std::array<float,4> endGameOpponentColor{{128.0f,128.0f,128.0f,128.0f}};

std::uint32_t SprintfAndHash(const char* format,std::int32_t value)
{
    char text[32];
    const auto size=std::snprintf(text,sizeof(text),format,value);
    Require(size>=0 && static_cast<std::size_t>(size)<sizeof(text),
        "Race stat key exceeds the original scratch extent");
    return hashString(text);
}
std::int32_t GetTheRightTrackLanguageID(const TrackScenarioDefinition* track,std::int32_t direction)
{
    Require(track,"Race caption has no actual current track descriptor");
    if (direction==0)return track->nameLanguageIds[0];
    if (direction==1)return track->nameLanguageIds[1];
    return 0;
}
std::int32_t GetTheRightTrackLanguageID(const TrackInfoResources& tracks,
    std::int32_t track,std::int32_t direction)
{
    return GetTheRightTrackLanguageID(tracks.Find(track),direction);
}
void SetRaceTrackCaption(RaceScreenServices& bindings,std::uint32_t screen,
    std::uint32_t trackLabel,std::uint32_t roundLabel,std::int32_t increment)
{
    // Complete final-only direct dependency1DC5D8. This descriptive native
    // helper does not claim a beta name or add an accepted original body.
    Require(bindings.frontend && bindings.frontend->raceSetup && bindings.frontend->ui,
        "Race caption owners are not bound");
    auto& services=*bindings.frontend;
    auto& parameters=*services.raceSetup->parameters;
    auto& resources=*services.ui->resources;
    resources.SetLanguageID(screen,trackLabel,static_cast<std::uint32_t>(
        GetTheRightTrackLanguageID(bindings.currentTrack,parameters.reverse)));
    if (services.frontend->UseTheRaceEvent() &&
        (parameters.raceKind==4 || parameters.raceKind==5)) {
        Require(services.raceSetup->currentEvent && services.ui->language,
            "Multi-race caption has no actual current event/language");
        const auto& event=*services.raceSetup->currentEvent;
        const auto count=event.GetRaceCount();
        const auto number=std::min(event.GetRaceIndex()+increment,count);
        const auto text=FormatLocalizedText(services.ui->language->GetString(0x3A983),{number,count});
        resources.SetLocalizedText(screen,roundLabel,text.c_str());
        resources.SetObjectVisibility(screen,roundLabel,1);
    } else resources.SetObjectVisibility(screen,roundLabel,0);
}
}

#include "Race.hpp"
#include "Player.hpp"
#include "Replay.hpp"
#include <cstring>
#include <iterator>
#include "FEAnim.hpp"
#include "CarInfo.hpp"
#include "Timer.hpp"
#include "RaceCoordinator.hpp"
#include "indep/src/speech/mgr.hpp"
#include "bware/inc/bMath.hpp"
#include <climits>
#include <vector>

namespace hp2 {
EndGameScreens* pEndGameScreens{};
namespace {
void CloseEndGameViews() noexcept;
const RaceParameters& EndGameParameters(const RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->frontend && services.frontend->raceSetup &&
        services.frontend->raceSetup->parameters,"EndGameScreens has no actual FRED/race parameters");
    return *services.frontend->raceSetup->parameters;
}
bool RewardWasEarnedBefore(const RaceEvent& event,std::int32_t index)
{
    if(event.kind==0 || event.kind==2) {
        EventRankState rank{static_cast<std::uint8_t>(event.priorState.ranks)};
        return event.IsRewardUnlocked(index,&rank);
    }
    if(event.kind==1 || event.kind==5 || event.kind==6) {
        EventTimeState time;std::memcpy(&time.ticks,&event.priorState.ranks,sizeof(time.ticks));
        return event.IsRewardUnlocked(index,&time);
    }
    if(event.kind==3) {
        auto snapshot=event.priorState;return event.IsRewardUnlocked(index,&snapshot);
    }
    if(event.kind==4) {
        EventTournamentState old;
        for(std::size_t i=0;i<8;++i)old.points[i]=static_cast<std::uint8_t>(
            (i<4?event.priorState.ranks:event.priorState.lastFinishPositions)>>((i%4)*8));
        return event.IsRewardUnlocked(index,&old);
    }
    return false;
}
bool SeeEventReward(const RaceScreenServices& services)
{
    auto& frontend=*services.frontend->frontend;
    if(!frontend.IsEventGame())return false;
    const auto* event=services.frontend->raceSetup->currentEvent;
    Require(event,"Event reward screen has no actual selected event");
    if(event->number==32 || (event->number==31 && !event->willUnlockBonus))return true;
    if(event->GetStatus()<3)return false;
    for(std::size_t index=0;index<event->rewards.size();++index)
        if(!RewardWasEarnedBefore(*event,static_cast<std::int32_t>(index)))return true;
    return event->rewards.empty() && event->DidBetter(frontend);
}
}

EndGameScreens::EndGameScreens(RaceScreenServices& services,std::int32_t continueType)
    :services_(services),continueType_(continueType),autosaveOwner_(std::make_shared<EndGameScreens*>(this))
{
    Require(!pEndGameScreens,"Another EndGameScreens owner is still alive");
    BuildScreenList();current_=screens_.begin();pEndGameScreens=this;
    // ResetAllScreenCounters1DCD20 is an empty released body. The real
    // constructor stores initialize the eleven distinct counters above.
}
std::exception_ptr EndGameScreens::Close() noexcept
{
    if(closed_)return closeFailure_;
    closed_=true;
    if(autosaveOwner_)*autosaveOwner_=nullptr;
    autosaveOwner_.reset();
    const auto attempt=[&](auto&& operation) noexcept {
        try {operation();}catch(...) {if(!closeFailure_)closeFailure_=std::current_exception();}
    };
    if(active_ || suspendPending_) {
        active_=false;
        attempt([&]{services_.frontend->screens->ChangeToScreen(nullptr,false);});
        attempt([&] {
            auto& ui=*services_.frontend->ui;
            ui.render->SetScreenVisibility(*ui.resources,0x49c15518u,1);
        });
        suspendPending_=false;
    }
    if(pEndGameScreens==this) {
        CloseEndGameViews();pEndGameScreens=nullptr;
    }
    return closeFailure_;
}
EndGameScreens::~EndGameScreens() noexcept {(void)Close();}
void EndGameScreens::BuildScreenList()
{
    const auto& parameters=EndGameParameters(services_);
    auto& frontend=*services_.frontend->frontend;
    Require(screens_.empty(),"EndGameScreens list is already constructed");
    if(frontend.transientStats.pointsReward!=-1) {
        const auto model=GetCarForPointsReward(frontend.transientStats.pointsReward);
        if(IsCarUnlocked(frontend,services_.frontend->raceSetup->events,model,3))screens_.push_back("Points Rewards");
    }
    if(frontend.transientStats.taskReward!=-1)screens_.push_back("Task Rewards");
    if(frontend.IsEventGame()) {
        const auto* event=services_.frontend->raceSetup->currentEvent;
        Require(event,"Event results have no selected RaceEvent");
        if(event->GetStatus()==4 && event->DidBetter(frontend) && event->unlocks.GetNumEventsUnlocked(*event))
            screens_.push_back("Events Unlocked");
    }
    if(SeeEventReward(services_))screens_.push_back("Reward menu");
    if(parameters.specialPursuit && frontend.playerCount==1)screens_.push_back("Be The Cop Results 1P");
    if(parameters.specialPursuit && frontend.playerCount==2)screens_.push_back("Be The Cop Results 2P");
    bool normal=!parameters.specialPursuit && frontend.gameType!=6;
    if(normal && frontend.IsEventGame()) {
        Require(services_.frontend->raceSetup->currentEvent,"Race results have no selected event");
        normal=services_.frontend->raceSetup->currentEvent->kind!=1;
    }
    if(normal)screens_.push_back("Race Results");
    if(!parameters.specialPursuit && frontend.IsEventGame() &&
        services_.frontend->raceSetup->currentEvent->kind==4)screens_.push_back("Tourney Standings");
    if(!parameters.specialPursuit && frontend.gameType!=6 && !parameters.pointToPoint && parameters.totalLaps!=1)
        screens_.push_back("Best Lap Times");
    if(normal)screens_.push_back("Best Total Times");
    screens_.push_back("End Game Menu");
}
const char* EndGameScreens::GetNextScreen() const
{
    if(current_==screens_.end())return nullptr;
    const auto next=std::next(current_);return next==screens_.end()?nullptr:*next;
}
const char* EndGameScreens::GetCurrentScreen() const
{
    return current_==screens_.end()?nullptr:*current_;
}
void EndGameScreens::RemoveScreen(const char* name)
{
    Require(name,"EndGameScreens remove has no actual name");
    for(auto i=screens_.begin();i!=screens_.end();++i)
        if(!std::strcmp(*i,name)) {
            Require(i!=current_,"Cannot erase the currently selected EndGameScreens node");
            screens_.erase(i);return;
        }
}
void EndGameScreens::AdvanceScreen()
{
    if(current_!=screens_.end() && std::next(current_)!=screens_.end())++current_;
}
bool EndGameScreens::AtRoot() const {return current_==screens_.begin();}
void EndGameScreens::MoveBackScreen()
{
    if(current_!=screens_.begin())--current_;
}
void EndGameScreens::ResumeScreens()
{
    Require(!closed_,"Cannot resume retired EndGameScreens");
    if(active_)return;
    Require(services_.frontend && services_.frontend->screens && services_.frontend->ui &&
        services_.frontend->ui->render && services_.frontend->ui->resources,
        "EndGameScreens has no actual Flailer screen/render owners");
    auto& ui=*services_.frontend->ui;
    const auto* name=GetCurrentScreen();
    Require(name,"EndGameScreens has no current source screen");
    suspendPending_=true;
    if(!std::strcmp(name,"Autosave")) {
        active_=true;
        Require(RequestAutosave(),"EndGameScreens could not schedule its actual profile save");
    } else {services_.frontend->screens->ChangeToScreen(name,true);active_=true;}
    ui.render->SetScreenVisibility(*ui.resources,0x49c15518u,0);
}
// PAL 1DD038..1DD06C: advance once, query the selected screen, then
// ChangeToScreen(name,0). This is the original deferred-save callback.
void EndGameScreens::AutosaveCompletedCallback()
{
    AdvanceScreen();
    services_.frontend->screens->ChangeToScreen(GetCurrentScreen(),false);
}

void EndGameScreens::SuspendScreens()
{
    if(!active_ && !suspendPending_)return;
    active_=false;services_.frontend->screens->ChangeToScreen(nullptr,false);
    auto& ui=*services_.frontend->ui;
    ui.render->SetScreenVisibility(*ui.resources,0x49c15518u,1);
    suspendPending_=false;
}
bool EndGameScreens::RequestAutosave()
{
    std::weak_ptr<EndGameScreens*> owner=autosaveOwner_;
    return RequestProfileAutosave([owner] {
        const auto retained=owner.lock();
        if(!retained || !*retained)return;
        (*retained)->AutosaveCompletedCallback();
    },[owner] {
        const auto retained=owner.lock();return retained && *retained;
    });
}
bool EndGameScreens::RequestCoordinatorActionAfterAutosave(std::int32_t kind,std::int32_t argument)
{
    std::weak_ptr<EndGameScreens*> owner=autosaveOwner_;
    return RequestProfileAutosave([owner,kind,argument] {
        const auto retained=owner.lock();
        if(!retained || !*retained)return;
        auto* session=(*retained)->Services().coordinator;
        Require(session && raceActionsEnqueue(&session->actions,kind,argument),
            "Completed autosave could not enqueue the original coordinator action");
    },[owner] {
        const auto retained=owner.lock();return retained && *retained;
    });
}
void EndGameScreens::HandleBaseEvent(ScreenMessage message,ScreenCallbackData& data)
{
    auto& screens=*services_.frontend->screens;
    if(message==ScreenMessage::Select) {
        const auto* next=GetNextScreen();
        AdvanceScreen();
        if(next && !std::strcmp(next,"Autosave")) {
            Require(RequestAutosave(),"EndGameScreens could not schedule its actual profile save");
        } else screens.ChangeToScreen(next,false);
    } else if(message==ScreenMessage::LeaveScreen) {
        data.sound=0;const auto* old=GetCurrentScreen();MoveBackScreen();
        if(GetCurrentScreen()!=old){data.sound=6;screens.ChangeToScreen(GetCurrentScreen(),false);}
        data.value=0;
    } else if(message==ScreenMessage::Previous || message==ScreenMessage::Next)data.sound=0;
    // Tick/Render/Scroll are dispatched by the concrete active original
    // view. The original empty base virtual bodies are not runtime stubs.
}
}

namespace hp2 {
namespace {
FlailerUiState& ResultsUi()
{
    Require(pEndGameScreens && pEndGameScreens->Services().frontend &&
        pEndGameScreens->Services().frontend->ui,"Results view has no live EndGameScreens/UI");
    auto& ui=*pEndGameScreens->Services().frontend->ui;
    Require(ui.resources && ui.language && ui.clock,"Results view has no actual resource/language/clock");
    return ui;
}
FrontendPlayerManager& ResultsFrontend() {return *pEndGameScreens->Services().frontend->frontend;}
const FrontendRaceSetupBindings& ResultsSetup() {return *pEndGameScreens->Services().frontend->raceSetup;}
const RaceParameters& ResultsParameters() {return *ResultsSetup().parameters;}
RaceEvent& ResultsEvent()
{
    Require(ResultsSetup().currentEvent,"Results view has no actual current event");
    return *ResultsSetup().currentEvent;
}
std::int32_t ResultsNow() {return ResultsUi().clock->realTimer;}
float ResultsElapsed(std::int32_t then)
{
    const auto bits=static_cast<std::uint32_t>(ResultsNow())-static_cast<std::uint32_t>(then);
    std::int32_t ticks;std::memcpy(&ticks,&bits,sizeof(ticks));
    return static_cast<float>(ticks)*0.0002500000118743628f;
}
void ResultColor(std::uint32_t screen,std::uint32_t key,const std::array<float,4>& color)
{
    auto* object=ResultsUi().resources->FindObject(screen,key);
    if(!object)return;
    const auto rgb=static_cast<std::uint32_t>(convertFloatToInt32(color[0])) |
        static_cast<std::uint32_t>(convertFloatToInt32(color[1]))<<8 |
        static_cast<std::uint32_t>(convertFloatToInt32(color[2]))<<16;
    if(object->kind==1)static_cast<FlailerTextChunk*>(object)->SetColor(rgb);
    else if(object->kind==2)static_cast<FlailerBitmapChunk*>(object)->SetColor(rgb);
    object->SetAlpha(convertFloatToInt32(color[3]));
}
void ResultFormat(std::uint32_t screen,std::uint32_t key,bool numeric)
{
    if(auto* object=ResultsUi().resources->FindObject(screen,key,1)) {
        auto* text=static_cast<FlailerTextChunk*>(object);
        if(numeric)text->SetFormatOptions(0x40);else text->ClearFormatOptions(0x40);
    }
}
bool ValidResultTime(std::int32_t ticks) {return ticks && ticks!=INT32_MAX;}
std::string PrintTimeToString(std::int32_t ticks,bool difference)
{
    // PAL1DC500 rounds a valid time to hundredths before formatting; zero
    // and the infinite sentinel retain the timer's own released handling.
    if(ValidResultTime(ticks))gameTimerRoundTime(&ticks,0.009999999776482582f);
    char text[GAME_TIMER_TEXT_CAPACITY];
    gameTimerPrintToString(&ticks,text,difference?1:0,&ResultsUi().language->currentLanguage);
    return text;
}
const RaceDriverInfo& ResultDriver(std::int32_t number)
{
    auto* info=raceParametersGetDriverInfoByDriverNumber(ResultsSetup().parameters,number);
    Require(info,"Results row has no authored driver in the actual race parameters");
    return *info;
}
const CarFramePlayerState* ResultPlayer(std::int32_t number)
{
    const auto* players=pEndGameScreens->Services().players;
    Require(players,"Results view has no actual Player registry");
    return number>=0 && number<2?players->playersByNumber[static_cast<std::size_t>(number)]:nullptr;
}
WorldAudioMessages* ResultSpeech()
{
    const auto* players=pEndGameScreens->Services().players;
    Require(players && players->services && players->services->streamedAudio &&
        players->services->streamedAudio->messages,"Results speech has no actual streamed audio owner");
    return players->services->streamedAudio->messages;
}
void CheckResultSpeech(WorldAudioMessagesResult result)
{
    Require(result==WORLD_AUDIO_MESSAGES_OK,"Results speech request failed on the actual audio queue");
}
void ResultTrackCaption(std::uint32_t screen)
{
    ResultsUi().resources->SetLanguageID(screen,0xf601f2d4u,
        static_cast<std::uint32_t>(GetTheRightTrackLanguageID(
            pEndGameScreens->Services().currentTrack,ResultsParameters().reverse)));
}
void ResultDriverColumns(std::uint32_t screen,std::int32_t row,std::int32_t driverNumber,
    std::int32_t position,const char* fourthFormat)
{
    auto& ui=ResultsUi();auto& resources=*ui.resources;
    const auto& driver=ResultDriver(driverNumber);
    const auto pos=SprintfAndHash("POS_%d",row),name=SprintfAndHash("NAME_%d",row),
        type=SprintfAndHash("TYPE_%d",row),fourth=SprintfAndHash(fourthFormat,row);
    const auto* player=driver.driverClass==1?ResultPlayer(driver.driverNumber):nullptr;
    const auto& color=player?endGamePlayerColors.at(static_cast<std::size_t>(player->movement.seat)):endGameOpponentColor;
    resources.Printf(screen,pos,"%2d",position);
    resources.Printf(screen,name,"%s",player?player->settings.playerName:RaceDriverName(driver));
    Require(ResultsSetup().cars,"Results row has no canonical car catalogue");
    const auto& car=ResultsSetup().cars->Car(static_cast<std::size_t>(driver.appearance.modelCode));
    resources.Printf(screen,type,"%s",GetTranslatedString(*ui.language,car.GetFullName(*ui.language)));
    for(const auto key:{pos,name,type,fourth}){resources.SetObjectVisibility(screen,key,1);ResultColor(screen,key,color);}
}
void ResultHideRow(std::uint32_t screen,std::int32_t row,const std::initializer_list<const char*>& columns)
{
    for(const auto* column:columns)ResultsUi().resources->SetObjectVisibility(screen,SprintfAndHash(column,row),0);
}
void ResultPoints(std::uint32_t screen,std::uint32_t key,std::int32_t points)
{
    const auto text=FormatRewardPoints(points,*ResultsUi().language);
    ResultsUi().resources->Printf(screen,key,"%s",text.c_str());
}
}

class EndGameMenusBase {
public:
    explicit EndGameMenusBase(std::uint32_t screen):screen_(screen)
    {ResultsUi().resources->SetObjectVisibility(screen_,0xaf6f1cdcu,!pEndGameScreens->AtRoot());}
    virtual ~EndGameMenusBase()=default;
    virtual void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {pEndGameScreens->HandleBaseEvent(message,data);}
protected:
    std::uint32_t screen_;
};

class tSlideshowStats final {
public:
    explicit tSlideshowStats(std::uint32_t screen):screen_(screen),started_(ResultsNow())
    {
        const auto& parameters=ResultsParameters();const auto& stats=ResultsFrontend().transientStats;
        Require(stats.count>0 && stats.count<=8,"Results slideshow needs one to eight retained race results");
        Add(0x4bcebu,[&](const auto& row,std::size_t index) {
            if(parameters.raceKind==3 && index)return Value{3,0x3ad69,0};
            if(row.completionReason==2)return Value{3,0x49447,0};
            return Value{index?4:2,index?row.totalTicks-stats.rows[0].totalTicks:row.totalTicks,0};
        });
        if(!parameters.specialPursuit && ResultsFrontend().IsEventGame() && ResultsEvent().kind==4)
            Add(0x4bce4u,[](const auto& row,std::size_t){
                Require(row.position>=1 && row.position<=8,"Tournament result position is outside the actual point table");
                return Value{0,eventTournamentPoints[static_cast<std::size_t>(row.position-1)],0};});
        if(parameters.policeDriverCount>0)
            Add(0x4bce5u,[](const auto& row,std::size_t){return Value{0,row.tickets,0};});
        if(!parameters.pointToPoint && parameters.totalLaps!=1)
            Add(0x4bce6u,[](const auto& row,std::size_t){return Value{2,row.bestLapTicks,0};});
        const float factor=ResultsFrontend().displaySettings.languageDependentOption?3.5999999046325684f:2.2369389533996582f;
        Add(0x4bce7u,[factor](const auto& row,std::size_t){return Value{1,0,row.averageSpeed*factor};});
        Add(0x4bce8u,[factor](const auto& row,std::size_t){return Value{1,0,row.maximumSpeed*factor};});
        SetupDataset(0);
    }
    void SetAlphaScale(float scale)
    {
        for(std::int32_t i=1;i<=8;++i)if(auto* object=ResultsUi().resources->FindObject(screen_,SprintfAndHash("BEHIND_%d",i)))
            object->SetAlpha(convertFloatToInt32(scale*128.0f));
    }
    void SetupDataset(std::size_t index)
    {
        Require(index<datasets_.size(),"Slideshow selected a non-existent authored dataset");current_=index;
        auto& resources=*ResultsUi().resources;const auto& dataset=datasets_[index];
        for(std::int32_t i=0;i<8;++i) {
            const auto key=SprintfAndHash("BEHIND_%d",i+1);const auto& value=dataset.values[static_cast<std::size_t>(i)];
            switch(value.kind) {
            case 0:resources.Printf(screen_,key,"%d",value.integer);ResultFormat(screen_,key,true);break;
            case 1:resources.Printf(screen_,key,"%3.f",static_cast<double>(value.scalar));ResultFormat(screen_,key,true);break;
            case 2:case 4:
                if(ValidResultTime(value.integer) || (value.kind==4 && value.integer==0)) {
                    const auto text=PrintTimeToString(value.integer,value.kind==4);
                    resources.Printf(screen_,key,"%s",text.c_str());ResultFormat(screen_,key,true);
                } else resources.Printf(screen_,key,"%s","");
                break;
            case 3:
                if(value.integer)resources.SetLanguageID(screen_,key,static_cast<std::uint32_t>(value.integer));
                else resources.Printf(screen_,key,"%s","");
                ResultFormat(screen_,key,false);break;
            default:break;
            }
        }
        resources.SetLanguageID(screen_,0xee0b838au,dataset.caption);
    }
    void Tick()
    {
        const auto elapsed=ResultsElapsed(started_);
        if(elapsed>5.0f) {
            if(elapsed<5.5f)SetAlphaScale(1.0f-(elapsed-5.0f)*2.0f);
            else if(elapsed<6.0f) {
                if(!changed_){SetupDataset((current_+1)%datasets_.size());changed_=true;}
                SetAlphaScale((elapsed-5.5f)*2.0f);
            } else started_=ResultsNow();
        } else {changed_=false;SetAlphaScale(1.0f);}
    }
    void ChangeStats(std::int32_t direction)
    {
        auto next=current_;
        if(direction==1)next=(next+1)%datasets_.size();
        else if(direction==-1)next=next?next-1:datasets_.size()-1;
        SetupDataset(next);changed_=false;started_=ResultsNow();
    }
private:
    struct Value {std::int32_t kind{},integer{};float scalar{};};
    struct Dataset {std::uint32_t caption{};std::array<Value,8> values{};};
    template<class F> void Add(std::uint32_t caption,F build)
    {
        Dataset dataset;dataset.caption=caption;
        const auto& stats=ResultsFrontend().transientStats;
        for(std::int32_t i=0;i<stats.count;++i)dataset.values[static_cast<std::size_t>(i)]=build(stats.rows[static_cast<std::size_t>(i)],static_cast<std::size_t>(i));
        datasets_.push_back(dataset);
    }
    std::uint32_t screen_;std::int32_t started_;bool changed_{};
    std::size_t current_{};std::vector<Dataset> datasets_;
};

class tRaceResultsPointsAnimation final {
public:
    explicit tRaceResultsPointsAnimation(std::uint32_t screen):screen_(screen),started_(ResultsNow())
    {
        if(const auto* object=ResultsUi().resources->FindObject(screen,0x68a2d5d1u)) {
            ResultsUi().resources->GetTopLeft(*object,anchorX_,anchorY_,ResultsUi().language);
        }
    }
    void Add(std::uint32_t caption,std::int32_t value) {items_.push_back({caption,value});}
    void StartAnimation() {current_=0;Redraw();}
    void Redraw()
    {
        Require(current_<items_.size(),"Points animation has no actual reward/result items");
        ResultsUi().resources->SetLanguageID(screen_,0x68a2d5d1u,items_[current_].first);
        ResultPoints(screen_,0x123d12feu,items_[current_].second);started_=ResultsNow();
    }
    void Tick()
    {
        const auto elapsed=ResultsElapsed(started_);
        if(elapsed<=5.0f || items_.size()<2)return;
        if(elapsed>5.5f) {
            current_=(current_+1)%items_.size();Redraw();
            ResultsUi().resources->SetGroupAlpha(screen_,0xab0f6086u,128);
        } else ResultsUi().resources->SetGroupAlpha(screen_,0xab0f6086u,
            convertFloatToInt32((1.0f-(elapsed-5.0f)*2.0f)*128.0f));
    }
private:
    std::uint32_t screen_;std::int32_t started_;std::size_t current_{};
    float anchorX_{},anchorY_{};std::vector<std::pair<std::uint32_t,std::int32_t>> items_;
};

class RaceResultsScreen final:public EndGameMenusBase {
public:
    explicit RaceResultsScreen(std::uint32_t screen):EndGameMenusBase(screen),slideshow_(screen),points_(screen)
    {
        auto& ui=ResultsUi();auto& resources=*ui.resources;const auto& stats=ResultsFrontend().transientStats;
        SetRaceTrackCaption(pEndGameScreens->Services(),screen,0xf601f2d4u,0xd0ced2e2u,0);
        if(ResultsFrontend().IsEventGame()) {
            const auto title=FormatLocalizedText(ui.language->GetString(0x4bcea),{static_cast<std::int32_t>(ResultsEvent().number)});
            resources.SetLocalizedText(screen,0x3288a48cu,title.c_str());
        }
        else resources.SetLanguageID(screen,0x3288a48cu,0x497ca);
        for(std::int32_t i=0;i<stats.count;++i) {
            const auto& row=stats.rows[static_cast<std::size_t>(i)];
            ResultDriverColumns(screen,i+1,row.driverNumber,row.position,"TIME_%d");
            const auto& driver=ResultDriver(row.driverNumber);const auto* player=driver.driverClass==1?ResultPlayer(driver.driverNumber):nullptr;
            ResultColor(screen,SprintfAndHash("BEHIND_%d",i+1),player?endGamePlayerColors.at(static_cast<std::size_t>(player->movement.seat)):endGameOpponentColor);
            ResultFormat(screen,SprintfAndHash("TIME_%d",i+1),true);
        }
        for(auto i=stats.count+1;i<=8;++i)ResultHideRow(screen,i,{"POS_%d","NAME_%d","TYPE_%d","TIME_%d","BEHIND_%d"});
        BuildPointsResults();
        resources.SetObjectVisibility(screen,0xaf6f1cdcu,!pEndGameScreens->AtRoot());
    }
    void BuildPointsResults()
    {
        auto& resources=*ResultsUi().resources;const auto& frontend=ResultsFrontend();
        const bool show=frontend.gameType!=6;
        resources.SetGroupVisibility(screen_,0x26c0a2e8u,show);
        resources.SetGroupVisibility(screen_,0xab0f6086u,show);
        if(!show)return;
        // Released1E04A8 sets this counter before its later speech gate.
        pEndGameScreens->ScreenCounter(0)=1;
        if(frontend.playerCount==1) {
            const auto* row=frontend.transientStats.GetDriver(0);
            if(row) {
                points_.Add(0xef420,row->racePoints);ResultPoints(screen_,0x27aa4effu,row->profilePoints);
                if(row->bonusPoints) {
                    const std::uint32_t caption=row->position==1?0x495eeu:row->position==2?0x495efu:row->position==3?0x495f0u:0x495f5u;
                    points_.Add(caption,row->bonusPoints);
                }
                points_.Add(0xef422,row->racePoints+row->bonusPoints);
            }
            points_.StartAnimation();
        } else if(frontend.playerCount==2) {
            for(std::int32_t player=0;player<2;++player) {
                const auto* row=frontend.transientStats.GetDriver(player);
                const auto label=player?0x896f342eu:0x68a2d5d1u,value=player?0x27aa4effu:0x123d12feu;
                if(row) {resources.SetLanguageID(screen_,label,player?0xef424u:0xef423u);ResultPoints(screen_,value,row->racePoints);}
                resources.SetObjectVisibility(screen_,label,row!=nullptr);resources.SetObjectVisibility(screen_,value,row!=nullptr);
            }
        }
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        if(message==ScreenMessage::Left)slideshow_.ChangeStats(-1);
        else if(message==ScreenMessage::Right)slideshow_.ChangeStats(1);
        else if(message==ScreenMessage::Tick){slideshow_.Tick();points_.Tick();}
        else EndGameMenusBase::HandleEvent(message,data);
    }
private:
    tSlideshowStats slideshow_;tRaceResultsPointsAnimation points_;
};

class TourneyStandingsScreen final:public EndGameMenusBase {
public:
    explicit TourneyStandingsScreen(std::uint32_t screen):EndGameMenusBase(screen)
    {
        SetRaceTrackCaption(pEndGameScreens->Services(),screen,0xf601f2d4u,0xd0ced2e2u,0);
        if(!ResultsFrontend().UseTheRaceEvent() || ResultsEvent().kind!=4)return;
        auto& event=ResultsEvent();const auto* standings=event.GetTournamentState();
        Require(standings && pEndGameScreens->Services().race,"Tournament view has no actual standings/race");
        const auto& race=*pEndGameScreens->Services().race;
        struct Row {std::int32_t driver,points,lastPosition;};std::vector<Row> rows;
        for(std::size_t i=0;i<race.entryCount;++i) {
            Require(race.entries && race.entries[i] && race.entries[i]->car && race.entries[i]->car->driverInfo,
                "Tournament view has no real participant driver");
            const auto driver=race.entries[i]->car->driverInfo->driverNumber;
            const auto racer=event.RacerIndexFromDriverNumber(driver);
            Require(racer>=0 && racer<8,"Tournament racer is outside the source standings table");
            rows.push_back({driver,standings->points[static_cast<std::size_t>(racer)],event.results[static_cast<std::size_t>(racer)].resultCode});
        }
        // TourneyPointsSorter1DD5C0 compares only descending accumulated points.
        std::stable_sort(rows.begin(),rows.end(),[](const Row& a,const Row& b){return a.points>b.points;});
        auto& resources=*ResultsUi().resources;
        for(std::size_t i=0;i<rows.size();++i) {
            const auto number=static_cast<std::int32_t>(i+1);const auto& row=rows[i];
            ResultDriverColumns(screen,number,row.driver,number,"RACE_%d");
            const auto points=SprintfAndHash("RACE_%d",number),total=SprintfAndHash("TOTAL_%d",number);
            if(row.lastPosition) {
                Require(row.lastPosition>=1 && row.lastPosition<=8,"Tournament last position exceeds its source point table");
                resources.Printf(screen,points,"%d",eventTournamentPoints[static_cast<std::size_t>(row.lastPosition-1)]);
            } else resources.Printf(screen,points,"%s","DNF");
            resources.Printf(screen,total,"%d",row.points);resources.SetObjectVisibility(screen,total,1);
            const auto& driver=ResultDriver(row.driver);const auto* player=driver.driverClass==1?ResultPlayer(driver.driverNumber):nullptr;
            const auto& color=player?endGamePlayerColors.at(static_cast<std::size_t>(player->movement.seat)):endGameOpponentColor;
            ResultColor(screen,total,color);
            if(player) {
                if(!pEndGameScreens->ScreenCounter(6)) {
                    const auto status=event.GetStatus();
                    CheckResultSpeech(worldAudioTournamentRaceSpeech(ResultSpeech(),status==3 || status==4,static_cast<std::int32_t>(i)));
                }
                pEndGameScreens->ScreenCounter(6)=1;
            }
        }
        for(auto i=static_cast<std::int32_t>(rows.size())+1;i<=8;++i)
            ResultHideRow(screen,i,{"POS_%d","NAME_%d","TYPE_%d","RACE_%d","TOTAL_%d"});
    }
};

namespace {
std::int32_t WhichPlayerSetRecord(const RaceProfileBestTime* record,bool total)
{
    const auto* players=pEndGameScreens->Services().players;Require(players,"Record display has no current Player registry");
    for(std::int32_t i=0;i<players->count;++i) {
        const auto* player=players->playersByNumber[static_cast<std::size_t>(i)];
        Require(player,"Record display found an absent live Player");
        if((total?player->recordTotalTime:player->recordLapTime)==record)return player->movement.seat;
    }
    return -1;
}
void BuildBestTimes(std::uint32_t screen,bool total)
{
    auto& frontend=ResultsFrontend();auto& ui=ResultsUi();auto& resources=*ui.resources;
    const auto& parameters=ResultsParameters();const auto* race=pEndGameScreens->Services().race;
    Require(race && race->entryCount && race->entries && race->entries[0] && race->entries[0]->car &&
        race->entries[0]->car->driverInfo && ResultsSetup().cars && frontend.highScores,
        "Best-time display has no actual race/model/profile table owners");
    const auto model=race->entries[0]->car->driverInfo->appearance.modelCode;
    const auto carClass=ResultsSetup().cars->Car(static_cast<std::size_t>(model)).routeVehicleClass;
    auto* records=total?frontend.GetBestTotalTimesTable(parameters.selectedScenario,carClass,parameters.reverse,parameters.totalLaps):
        raceProfileFindBestTime(&frontend.highScores->LapView(),parameters.selectedScenario,carClass,parameters.reverse);
    Require(records,"Best-time display has no actual five-record table");ResultTrackCaption(screen);
    std::int32_t firstPlayer=-1,firstIndex=0,bestPosition=9;const TransientStatsElement* other{};
    for(std::int32_t i=0;i<5;++i) {
        const auto row=i+1;const auto& record=records[i];
        const auto pos=SprintfAndHash("POS_%d",row),name=SprintfAndHash("NAME_%d",row),
            type=SprintfAndHash("TYPE_%d",row),time=SprintfAndHash("TIME_%d",row);
        const bool valid=ValidResultTime(record.ticks);
        for(const auto key:{pos,name,type,time})resources.SetObjectVisibility(screen,key,valid);
        if(!valid)continue;
        resources.Printf(screen,pos,"%2d",row);
        resources.Printf(screen,name,"%s",frontend.RecordPlayerName(record.playerNameIndex));
        const auto& car=ResultsSetup().cars->Car(static_cast<std::size_t>(record.modelCode));
        resources.Printf(screen,type,"%s",GetTranslatedString(*ui.language,car.GetFullName(*ui.language)));
        const auto text=PrintTimeToString(record.ticks,false);resources.Printf(screen,time,"%s",text.c_str());
        const auto player=WhichPlayerSetRecord(&record,total);
        const auto& color=player==-1?endGameOpponentColor:endGamePlayerColors.at(static_cast<std::size_t>(player));
        if(player!=-1 && firstPlayer==-1) {
            firstPlayer=player;firstIndex=i;other=frontend.transientStats.GetDriver(player==1?0:1);
            const auto* own=frontend.transientStats.GetDriver(player);
            Require(own,"New profile record has no retained race result");bestPosition=std::min(bestPosition,own->position);
        }
        for(const auto key:{pos,name,type,time})ResultColor(screen,key,color);
        ResultFormat(screen,time,true);
    }
    auto& seen=pEndGameScreens->ScreenCounter(total?8u:7u);
    if(!seen && firstPlayer!=-1 && bestPosition<4 && other) {
        if(total)CheckResultSpeech(worldAudioPostGameTimeRecord(ResultSpeech(),parameters,
            ResultsSetup().currentEvent,firstPlayer,firstIndex,*other));
        else CheckResultSpeech(worldAudioPostGameLapRecord(ResultSpeech(),parameters,
            ResultsSetup().currentEvent,firstPlayer,firstIndex,*other));
    }
    seen=1;
}
}
class BestLapTimes final:public EndGameMenusBase {
public:explicit BestLapTimes(std::uint32_t screen):EndGameMenusBase(screen){BuildBestTimes(screen,false);}
};
class BestTotalTimes final:public EndGameMenusBase {
public:explicit BestTotalTimes(std::uint32_t screen):EndGameMenusBase(screen){BuildBestTimes(screen,true);}
};

class BeACopResults1P final:public EndGameMenusBase {
public:
    explicit BeACopResults1P(std::uint32_t screen):EndGameMenusBase(screen)
    {
        ResultTrackCaption(screen);const auto* row=ResultsFrontend().transientStats.GetDriver(0);
        Require(row,"Single-player pursuit results have no retained player result");
        auto& resources=*ResultsUi().resources;std::int32_t bonus=0;
        resources.SetGroupVisibility(screen,0x8564c282u,row->completionReason==4);
        if(row->completionReason==4) {
            const auto seconds=convertFloatToInt32(row->pursuitSeconds);bonus=seconds*100;
            resources.Printf(screen,0x761c6807u,"%d",seconds);resources.Printf(screen,0xc8f86ae5u,"%d",bonus);
        }
        resources.Printf(screen,0xd49e65d2u,"%d",row->racePoints);
        resources.Printf(screen,0x98fcdd99u,"%d",row->racePoints+bonus);
        resources.Printf(screen,0xb8b4361bu,"%d",row->profilePoints);
    }
};
class BeACopResults2P final:public EndGameMenusBase {
public:
    explicit BeACopResults2P(std::uint32_t screen):EndGameMenusBase(screen)
    {
        ResultTrackCaption(screen);auto& ui=ResultsUi();auto& resources=*ui.resources;
        const auto* first=ResultsFrontend().transientStats.GetDriver(0);
        const auto* second=ResultsFrontend().transientStats.GetDriver(1);
        Require(first && second,"Two-player pursuit results have no two retained player results");
        std::uint32_t left=0,right=0;
        if(first->completionReason==4 && second->completionReason==5){left=0xecd1c;right=0xecd1f;}
        else if(first->completionReason==5 && (second->completionReason==1 || second->completionReason==4)) {left=0xecd1d;right=0xecd1e;}
        else if(first->completionReason==5 && second->completionReason==5){left=0xecd25;right=0xecd26;}
        if(left) {
            resources.RenderTextIntoGroup(screen,0xfe6b3c76u,ui.language->GetString(static_cast<std::int32_t>(left)));
            resources.RenderTextIntoGroup(screen,0x4b657937u,ui.language->GetString(static_cast<std::int32_t>(right)));
        }
        resources.Printf(screen,0x89e0e2dbu,"%d",first->racePoints);
        resources.Printf(screen,0x89e0e2dcu,"%d",second->racePoints);
    }
};

class LapTimesDisplay final {
public:
    explicit LapTimesDisplay(std::uint32_t screen):screen_(screen)
    {
        ResultTrackCaption(screen);auto& resources=*ResultsUi().resources;
        const auto* first=ResultPlayer(0);if(!first)first=ResultPlayer(1);if(!first)return;
        resources.Printf(screen,0x6dbbc830u,"%s",first->settings.playerName);
        if(ResultsParameters().mode==2) {
            const auto* second=ResultPlayer(1);if(!second)return;
            resources.Printf(screen,0x6dbbc831u,"%s",second->settings.playerName);
        }
        const auto& stats=ResultsFrontend().transientStats;
        const auto* left=stats.GetDriver(0);const auto* right=stats.GetDriver(1);
        if(!left){left=right;right=nullptr;}
        const auto laps=std::max(left?left->laps:0,right?right->laps:0);
        std::array<std::int32_t,2> best{{-1,-1}},ticks{{99999999,99999999}};
        const std::array<const TransientStatsElement*,2> columns{{left,right}};
        for(std::size_t column=0;column<2;++column)if(columns[column])
            for(std::int32_t lap=0;lap<9;++lap) {
                const auto time=columns[column]->lapTicks[static_cast<std::size_t>(lap+1)];
                if(time && time<ticks[column]){ticks[column]=time;best[column]=lap;}
            }
        const std::array<float,4> bestColor{{134.0f,96.0f,48.0f,128.0f}};
        for(std::int32_t lap=0;lap<9;++lap) {
            const auto number=lap+1;
            const auto label=SprintfAndHash("LAP_%d",number),name=SprintfAndHash("LAPNAME_%d",number);
            resources.Printf(screen,label,"%d",number);
            for(std::size_t column=0;column<2;++column) {
                const auto key=SprintfAndHash(column?"P2_LAP_%d":"P1_LAP_%d",number);
                const auto* row=columns[column];const auto time=row?row->lapTicks[static_cast<std::size_t>(number)]:0;
                const bool visible=row && ValidResultTime(time) && lap<laps;
                resources.SetObjectVisibility(screen,key,visible);
                if(row)ResultColor(screen,key,best[column]==lap?bestColor:endGameOpponentColor);
                if(visible) {const auto text=PrintTimeToString(time,false);resources.Printf(screen,key,"%s",text.c_str());ResultFormat(screen,key,true);}
            }
            resources.SetObjectVisibility(screen,name,lap<laps);
            if(lap>=laps)resources.SetObjectVisibility(screen,label,0);
        }
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {if(message!=ScreenMessage::LeaveScreen)data.sound=0;}
private:std::uint32_t screen_;
};

namespace {
void HideNumCarsUnlocked(std::uint32_t screen)
{ResultsUi().resources->RenderTextIntoGroup(screen,0x1a190ea0u,"");}
void ShowNumCarsUnlocked(std::uint32_t screen,bool cops)
{
    const auto& setup=ResultsSetup();Require(setup.cars,"Unlocked-car display has no actual car catalogue");
    const auto& frontend=ResultsFrontend();auto& ui=ResultsUi();std::int32_t unlocked,total;
    if(cops) {
        unlocked=GetNumCopCarsUnlocked(frontend,*setup.cars,setup.events);total=GetNumCopCarsAvailable(*setup.cars);
    } else {
        unlocked=GetNumCarsUnlocked(frontend,*setup.cars,setup.events,0)+GetNumCarsUnlocked(frontend,*setup.cars,setup.events,1)+ResultsParameters().unlockedRewardCarCount;
        if(frontend.UseTheRaceEvent()) {
            const auto& event=ResultsEvent();
            for(std::size_t i=0;i<event.rewards.size();++i)
                if((event.rewards[i].selection&255u)==0 && SeeEventReward(pEndGameScreens->Services()) && event.IsRewardUnlocked(static_cast<std::int32_t>(i)))++unlocked;
        }
        total=GetNumCarsAvailable(*setup.cars,0)+GetNumCarsAvailable(*setup.cars,1);
    }
    const auto text=FormatLocalizedText(ui.language->GetString(cops?0xea7f5:0xea7f4),{unlocked,total});
    ui.resources->RenderTextIntoGroup(screen,0x1a190ea0u,text.c_str());
}
std::int32_t RewardPlacement()
{
    const auto& event=ResultsEvent();
    if(event.kind==4) {const auto* state=event.GetTournamentState();Require(state,"Reward tournament has no actual standings");return state->RacerRank(0);}
    const auto* result=ResultsFrontend().transientStats.GetDriver(0);Require(result,"Reward screen has no retained first driver result");return result->position;
}
std::int32_t CalculateRewardLanguageID()
{
    const auto& event=ResultsEvent();const bool reward=event.AnyRewardUnlocked(),won=event.GetStatus()==4;
    if(won) {
        if(event.number==30)return ResultsFrontend().gameType==8?0xe30d5:ResultsFrontend().gameType==9?0xe30d6:0;
        if(event.number==32)return 0xe32c4;
        if(event.number==31)return ResultsFrontend().gameType==8?0xe32c5:ResultsFrontend().gameType==9?0xe32c6:0;
    }
    const auto& p=ResultsParameters();
    if(p.raceKind==2 && ValidResultTime(p.timeLimitTicks) && !p.specialPursuit)return won?(reward?0xe09c1:0xe09c6):0xe09c0;
    if(p.pointToPoint && ValidResultTime(p.timeLimitTicks) && !p.specialPursuit && p.raceKind!=2)return won?(reward?0xe09c3:0xe09c7):0xe09c2;
    if(p.specialPursuit)return won?0xe09c4:0xe09c5;
    if(!won)return 0xe30d3;
    const auto rank=RewardPlacement();
    if(reward)return rank==1?0xe30d0:rank==2?0xe30d1:rank==3?0xe30d2:0xe30d4;
    return rank==1?0xe30da:rank==2?0xe30db:rank==3?0xe30dc:0xe30dd;
}
std::string FormatRewardDescription(std::int32_t id)
{
    auto& ui=ResultsUi();const auto& event=ResultsEvent();
    const auto* row=ResultsFrontend().transientStats.GetDriver(0);Require(row,"Reward description has no retained first driver result");
    const auto best=PrintTimeToString(row->bestLapTicks,false),time=PrintTimeToString(row->totalTicks,false);
    const auto goal=event.objective.kind==2?PrintTimeToString(static_cast<std::int32_t>(event.objective.value),false):std::string{};
    const auto place=RewardPlacement();Require(place>=0 && place<=8,"Reward rank has no released ordinal-language entry");
    const char* ordinal=ui.language->GetString(0xee48+place);
    const auto target=event.objective.kind==1?event.objective.value:0;
    Require(target<=8,"Reward objective rank has no released ordinal-language entry");
    const char* targetOrdinal=ui.language->GetString(static_cast<std::int32_t>(0xee48+target));
    std::string reward;
    if(!event.rewards.empty()) {
        const auto selection=event.rewards[0].selection;const auto object=static_cast<std::int32_t>((selection>>8)&255u);
        if((selection&255u)==0) {
            Require(ResultsSetup().cars,"Reward description has no actual car catalogue");
            reward=GetTranslatedString(*ui.language,ResultsSetup().cars->Car(static_cast<std::size_t>(object)).GetFullName(*ui.language));
        } else if((selection&255u)==1) {
            Require(ResultsSetup().tracks,"Reward description has no actual track catalogue");
            const auto track=GetTheRightTrackLanguageID(*ResultsSetup().tracks,object,static_cast<std::int32_t>((selection>>16)&255u));
            reward=std::string(ui.language->GetString(track))+" "+ui.language->GetString(0xe09bf);
        }
    }
    const auto* format=ui.language->GetString(id);
    switch(id) {
    case 0xe09c1:return FormatLocalizedText(format,{best.c_str(),reward.c_str()});
    case 0xe09c0:return FormatLocalizedText(format,{best.c_str(),goal.c_str()});
    case 0xe09c6:return FormatLocalizedText(format,{best.c_str()});
    case 0xe09c3:return FormatLocalizedText(format,{time.c_str(),reward.c_str()});
    case 0xe09c2:return FormatLocalizedText(format,{time.c_str(),goal.c_str()});
    case 0xe09c7:return FormatLocalizedText(format,{time.c_str()});
    case 0xe09c4:case 0xe30d0:case 0xe30d1:case 0xe30d2:case 0xe30d4:
        return FormatLocalizedText(format,{reward.c_str()});
    case 0xe30d3:return FormatLocalizedText(format,{ordinal,targetOrdinal});
    default:Require(format,"Reward description has no language string");return format;
    }
}
CarFramePlayerState* FirstResultSpeechPlayer()
{
    const auto* players=pEndGameScreens->Services().players;Require(players,"Results speech has no actual Player registry");
    return players->playersByNumber[0];
}
}

class PointsRewardScreen final:public EndGameMenusBase {
public:
    explicit PointsRewardScreen(std::uint32_t screen):EndGameMenusBase(screen)
    {
        auto& frontend=ResultsFrontend();auto& ui=ResultsUi();
        const auto model=GetCarForPointsReward(frontend.transientStats.pointsReward);
        Require(model>=0 && ResultsSetup().cars,"Points reward has no actual awarded car");
        const auto& car=ResultsSetup().cars->Car(static_cast<std::size_t>(model));
        // Actual language images already contain templates longer than128
        // bytes. Keep their complete text and the released one-car argument;
        // a PS2 stack scratch extent is not a native message-size contract.
        const auto text=FormatLocalizedText(
            ui.language->GetString(GetNumPointsRewardsEarned(frontend)==18?0xe7ef1:0xe7ef0),
            {GetTranslatedString(*ui.language,car.GetFullName(*ui.language))});
        if(!pEndGameScreens->ScreenCounter(0))CheckResultSpeech(worldAudioUnlockCarSpeech(ResultSpeech(),*ResultsSetup().cars,model));
        pEndGameScreens->ScreenCounter(0)=1;
        ui.resources->RenderTextIntoGroup(screen,0x461d42d0u,text.c_str());
        ui.resources->SetLanguageID(screen,0x4703a605u,0xde4a7);ShowNumCarsUnlocked(screen,false);
    }
};
class TaskRewardScreen final:public EndGameMenusBase {
public:
    explicit TaskRewardScreen(std::uint32_t screen):EndGameMenusBase(screen)
    {
        const auto task=ResultsFrontend().transientStats.taskReward;auto& ui=ResultsUi();
        ui.resources->RenderTextIntoGroup(screen,0x461d42d0u,ui.language->GetString(GetLanguageIDForTaskReward(task)));
        if(!pEndGameScreens->ScreenCounter(1)) {
            Require(ResultsSetup().cars,"Task reward has no actual car catalogue");
            CheckResultSpeech(worldAudioUnlockCarSpeech(ResultSpeech(),*ResultsSetup().cars,GetCarForTaskReward(task)));
        }
        pEndGameScreens->ScreenCounter(1)=1;
        ui.resources->SetLanguageID(screen,0x4703a605u,0xde4a8);ShowNumCarsUnlocked(screen,GetGameTypeForTaskReward(task)==7);
    }
};
class EventsUnlockedScreen final:public EndGameMenusBase {
public:
    explicit EventsUnlockedScreen(std::uint32_t screen):EndGameMenusBase(screen)
    {
        auto& ui=ResultsUi();const auto& event=ResultsEvent();std::array<std::int32_t,8> events{};
        const auto count=std::min(event.unlocks.GetEventsUnlocked(events.data(),events.size(),event),3);
        const std::array<std::int32_t,4> championship{{0,0xde2b0,0xde2b4,0xde2b1}},pursuit{{0,0xde2b2,0xde2b5,0xde2b3}};
        const auto& ids=ResultsFrontend().gameType==8?championship:pursuit;
        Require(count>=1,"EventsUnlockedScreen has no actual newly unlocked event");
        const auto text=FormatLocalizedText(ui.language->GetString(ids[static_cast<std::size_t>(count)]),
            {events[0],events[1],events[2]});
        ui.resources->RenderTextIntoGroup(screen,0x461d42d0u,text.c_str());ui.resources->SetLanguageID(screen,0x4703a605u,0xde2b6);
        if(!pEndGameScreens->ScreenCounter(2))CheckResultSpeech(worldAudioRaceEventSpeech(ResultSpeech(),FirstResultSpeechPlayer(),
            ResultsFrontend().transientStats.GetDriver(0),event,ResultsFrontend()));
        pEndGameScreens->ScreenCounter(2)=1;HideNumCarsUnlocked(screen);
    }
};
class EventRewardScreen final:public EndGameMenusBase {
public:
    explicit EventRewardScreen(std::uint32_t screen):EndGameMenusBase(screen)
    {
        auto& ui=ResultsUi();const auto& event=ResultsEvent();const bool won=event.GetStatus()==4;
        if(!pEndGameScreens->ScreenCounter(3)) {
            if(!won || event.number==32 || (event.number==31 && !event.willUnlockBonus))
                CheckResultSpeech(worldAudioRaceEventSpeech(ResultSpeech(),FirstResultSpeechPlayer(),
                    ResultsFrontend().transientStats.GetDriver(0),event,ResultsFrontend()));
            else {
                Require(ResultsSetup().cars,"Event reward has no actual car catalogue");
                CheckResultSpeech(worldAudioRewardScreenSpeech(ResultSpeech(),*ResultsSetup().cars,
                    event.AnyRewardUnlocked(),event.rewards.empty()?nullptr:&event.rewards[0]));
            }
        }
        pEndGameScreens->ScreenCounter(3)=1;
        const auto text=FormatRewardDescription(CalculateRewardLanguageID());
        ui.resources->RenderTextIntoGroup(screen,0x461d42d0u,text.c_str());
        const auto title=FormatLocalizedText(ui.language->GetString(won?0xde4a4:0xde4a5),{static_cast<std::int32_t>(event.number)});
        ui.resources->SetLocalizedText(screen,0x4703a605u,title.c_str());
        if(!event.rewards.empty() && (event.rewards[0].selection&255u)==0 && event.DidBetter(ResultsFrontend()) && event.IsRewardUnlocked(0))
            ShowNumCarsUnlocked(screen,false);
        else HideNumCarsUnlocked(screen);
    }
};

class EndGameMenu final:public EndGameMenusBase {
public:
    EndGameMenu(std::uint32_t screen,ScreenCallbackData& data):EndGameMenusBase(screen),
        highlight_(ResultsUi(),screen,0xab671c9au,ResultsUi().animationSettings.highlightColor)
    {
        FigureOutPhrases();data.value=0;
        if(SeeNextRace())items_.push_back({phrases_[10],0,0});
        if(SeeRestartRace())items_.push_back({phrases_[1],0,1});
        if(SeeRestartEvent())items_.push_back({phrases_[4],0,2});
        Require(pEndGameScreens->Services().replay,"End-game menu has no actual replay owner");
        if(!pEndGameScreens->Services().replay->desynchronized)items_.push_back({phrases_[13],0,3});
        if(!ResultsParameters().specialPursuit)items_.push_back({phrases_[14],0,4});
        items_.push_back({phrases_[7],0,5});
        auto& resources=*ResultsUi().resources;
        auto* node=pEndGameScreens->Services().frontend->screens->FindScreen(static_cast<std::int32_t>(screen));
        Require(node && node->screen && node->screen->menu,"End-game menu has no authored Flailer menu items");
        for(std::size_t i=0;i<6;++i) {
            const auto key=0x52e039f3u+static_cast<std::uint32_t>(i);
            if(i<items_.size()) {
                items_[i].key=key;resources.SetLanguageID(screen,key,items_[i].language);
                if(auto* item=node->screen->menu->FindItem(key))item->SetMenuItemState(1);
                resources.SetObjectVisibility(screen,key,1);
            } else {
                if(auto* item=node->screen->menu->FindItem(key))item->ClearMenuItemState(1);
                resources.SetObjectVisibility(screen,key,0);
            }
        }
        resources.SetLanguageID(screen,0x5964eab1u,phrases_[0]);
    }
    bool SeeNextRace() const
    {
        const auto& p=ResultsParameters();return (p.raceKind==4 || p.raceKind==5) &&
            ResultsFrontend().UseTheRaceEvent() && ResultsEvent().CanContinue(ResultsFrontend(),p);
    }
    bool SeeRestartRace() const
    {return !ResultsFrontend().IsEventGame() || ResultsParameters().raceKind==4 || ResultsParameters().raceKind==5;}
    bool SeeRestartEvent() const
    {return ResultsParameters().raceKind==4 || ResultsParameters().raceKind==5 || ResultsFrontend().UseTheRaceEvent();}
    void FigureOutPhrases()
    {
        phrases_.fill(UINT32_MAX);
        phrases_[10]=0x4baf2;phrases_[11]=0x382d5;phrases_[12]=0x382d4;
        phrases_[13]=0x30d43;phrases_[14]=0x4bd49;
        phrases_[4]=0x30d4d;phrases_[5]=0x38280;phrases_[6]=0x38286;
        const auto& p=ResultsParameters();
        if(p.eventGame) {
            phrases_[1]=0x30d45;phrases_[2]=0x38271;phrases_[3]=0x3827c;
            const bool multi=p.raceKind==4 || p.raceKind==5;
            phrases_[0]=multi && ResultsFrontend().UseTheRaceEvent() && ResultsEvent().CanContinue(ResultsFrontend(),p)?0x30da4:0x30da6;
            phrases_[7]=ResultsFrontend().gameType==9?0x38338:0x3833a;
            phrases_[8]=ResultsFrontend().gameType==9?0x38339:0x3833b;phrases_[9]=0x38287;
        } else if(p.specialPursuit) {
            phrases_[0]=0x30da5;phrases_[1]=0x30d51;phrases_[2]=0x38281;phrases_[3]=0x38285;
            phrases_[7]=0x30d52;phrases_[8]=0x38282;phrases_[9]=0x38283;
        } else {
            phrases_[0]=0x30da4;phrases_[1]=0x30d45;phrases_[2]=0x38271;phrases_[3]=0x3827c;
            phrases_[7]=0x30d46;phrases_[8]=0x38272;phrases_[9]=0x3827b;
        }
    }
    void Popup(std::uint32_t footer,std::uint32_t message,std::int32_t state)
    {
        auto& ui=ResultsUi();auto& screens=*pEndGameScreens->Services().frontend->screens;
        FlailerPopupConfig config;config.buttonCount=2;config.flags=3;
        config.sourceNode=screens.active;config.screenKey=0x037f3133;
        config.message=ui.language->GetString(static_cast<std::int32_t>(message));
        // PAL1E18D8..1E18E0 stores 1 at config+20 (OK), then 0 at
        // config+24 (Cancel). HandleEvent consumes that result as a boolean.
        config.buttonLanguageIds={{0x38277,0x38278,0}};config.buttonResults={{1,0,0}};
        config.initialButton=1;config.joystickPort=-1;config.footerLanguageId=footer;
        Require(screens.SummonPopup(config)>=0,"End-game confirmation has no available real popup owner");
        state_=state;Require(ui.render,"End-game confirmation has no actual renderer");
        ui.render->SetScreenVisibility(*ui.resources,screen_,1);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        highlight_.HandleEvents(message,data);
        if(message==ScreenMessage::Select) {
            const auto item=std::find_if(items_.begin(),items_.end(),[&](const Item& x){return x.key==data.key;});
            if(item==items_.end())return;
            switch(item->action) {
            case 0:Popup(phrases_[12],phrases_[11],3);break;
            case 1:Popup(phrases_[3],phrases_[2],0);break;
            case 2:Popup(phrases_[6],phrases_[5],1);break;
            case 3:Queue(12,0);break;
            case 4:pEndGameScreens->Services().frontend->screens->ChangeToScreen(
                ResultsParameters().mode==1?"Lap Times 1P":"Lap Times 2P",true);break;
            case 5:Popup(phrases_[9],phrases_[8],2);break;
            default:break;
            }
        } else if(message==ScreenMessage::LeaveScreen) {
            const auto* old=pEndGameScreens->GetCurrentScreen();pEndGameScreens->MoveBackScreen();
            data.value=0;
            if(old!=pEndGameScreens->GetCurrentScreen())pEndGameScreens->Services().frontend->screens->ChangeToScreen(pEndGameScreens->GetCurrentScreen(),false);
        } else if(message==ScreenMessage::PopupDismiss) {
            if(!data.value) {
                auto& ui=ResultsUi();ui.render->SetScreenVisibility(*ui.resources,screen_,0);return;
            }
            if(state_==2 || state_==0) {
                if(ResultsFrontend().UseTheRaceEvent())ResultsEvent().MaybeDiscardResult(ResultsFrontend());
                if(state_==2) QueueAfterAutosave(10,0);else Queue(8,0);
            } else if(state_==1) {
                ResultsEvent().RestartEvent(ResultsFrontend(),ResultsSetup());Queue(9,0);
            } else if(state_==3) {
                if(!ResultsEvent().ResultShouldBeDiscarded(ResultsFrontend()))QueueAfterAutosave(9,1);
                else Queue(9,1);
            }
        }
    }
private:
    struct Item {std::uint32_t language,key;std::int32_t action;};
    void Queue(std::int32_t kind,std::int32_t argument)
    {
        auto* session=pEndGameScreens->Services().coordinator;
        Require(session && raceActionsEnqueue(&session->actions,kind,argument),
            "End-game menu could not enqueue its original coordinator action");
    }
    void QueueAfterAutosave(std::int32_t kind,std::int32_t argument)
    {
        if(!ResultsFrontend().autoSave){Queue(kind,argument);return;}
        // Changing to Autosave may retire this concrete menu. The retained
        // save owner guards both the screen stack and coordinator callback.
        Require(pEndGameScreens->RequestCoordinatorActionAfterAutosave(kind,argument),
            "An earlier actual profile transfer is still active");
    }
    FlailerHighlightAnimation highlight_;std::int32_t state_{};
    std::array<std::uint32_t,15> phrases_{};std::vector<Item> items_;
};

namespace {
std::unique_ptr<EndGameMenusBase> currentEndGameView;
std::unique_ptr<LapTimesDisplay> currentLapTimes;
void CloseEndGameViews() noexcept
{
    currentEndGameView.reset();currentLapTimes.reset();
}
}
void EndGameMenusCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(pEndGameScreens && data,"EndGameMenus callback has no live original owner/payload");
    // The menu exists before ScreenActivate constructs its concrete view.
    // If that constructor throws, UnActivate still sends item retirement
    // notifications before ScreenDeactivate. A back-navigation consultation
    // may also arrive; an absent view must not intercept that transition.
    if(!currentEndGameView && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if(message==ScreenMessage::ScreenActivate) {
        Require(!currentEndGameView,"A concrete end-game view is already active");
        const auto* node=pEndGameScreens->Services().frontend->screens->active;
        Require(node && node->configuration && node->configuration->name,"End-game activation has no actual configured screen");
        const auto* name=node->configuration->name;const auto screen=data->key;
        if(!std::strcmp(name,"Points Rewards"))currentEndGameView=std::make_unique<PointsRewardScreen>(screen);
        else if(!std::strcmp(name,"Task Rewards"))currentEndGameView=std::make_unique<TaskRewardScreen>(screen);
        else if(!std::strcmp(name,"Events Unlocked"))currentEndGameView=std::make_unique<EventsUnlockedScreen>(screen);
        else if(!std::strcmp(name,"Reward menu"))currentEndGameView=std::make_unique<EventRewardScreen>(screen);
        else if(!std::strcmp(name,"Race Results"))currentEndGameView=std::make_unique<RaceResultsScreen>(screen);
        else if(!std::strcmp(name,"Tourney Standings"))currentEndGameView=std::make_unique<TourneyStandingsScreen>(screen);
        else if(!std::strcmp(name,"Best Lap Times"))currentEndGameView=std::make_unique<BestLapTimes>(screen);
        else if(!std::strcmp(name,"Best Total Times"))currentEndGameView=std::make_unique<BestTotalTimes>(screen);
        else if(!std::strcmp(name,"End Game Menu"))currentEndGameView=std::make_unique<EndGameMenu>(screen,*data);
        else if(!std::strcmp(name,"Be The Cop Results 1P"))currentEndGameView=std::make_unique<BeACopResults1P>(screen);
        else if(!std::strcmp(name,"Be The Cop Results 2P"))currentEndGameView=std::make_unique<BeACopResults2P>(screen);
        else Require(false,"EndGameMenus callback received an unrelated configuration");
    } else if(message==ScreenMessage::ScreenDeactivate)currentEndGameView.reset();
    else {Require(currentEndGameView!=nullptr,"EndGameMenus callback has no active concrete view");currentEndGameView->HandleEvent(message,*data);}
}
void LapTimesDisplayCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(pEndGameScreens && data,"LapTimes callback has no live original owner/payload");
    if(!currentLapTimes && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if(message==ScreenMessage::ScreenActivate) {
        Require(!currentLapTimes,"A lap-times view is already active");currentLapTimes=std::make_unique<LapTimesDisplay>(data->key);
    } else if(message==ScreenMessage::ScreenDeactivate)currentLapTimes.reset();
    else {Require(currentLapTimes!=nullptr,"LapTimes callback has no actual view");currentLapTimes->HandleEvent(message,*data);}
}
std::array<FlailerScreenConfig,13>& EndGameScreenConfigurations()
{
    // PAL1E1EA0 initializer, all thirteen complete constructor operands.
    static std::array<FlailerScreenConfig,13> configurations{{
        {"Lap Times 2P",LapTimesDisplayCallback,{{static_cast<std::int32_t>(0xc4881cdeu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Lap Times 1P",LapTimesDisplayCallback,{{static_cast<std::int32_t>(0xc4881cbdu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Tourney Standings",EndGameMenusCallback,{{0x0532b6ff,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Best Lap Times",EndGameMenusCallback,{{static_cast<std::int32_t>(0xacb2a4cau),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Best Total Times",EndGameMenusCallback,{{static_cast<std::int32_t>(0xf5a3cf51u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Events Unlocked",EndGameMenusCallback,{{0x7e420558,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Reward menu",EndGameMenusCallback,{{0x7e420558,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Race Results",EndGameMenusCallback,{{static_cast<std::int32_t>(0xab049b2bu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"End Game Menu",EndGameMenusCallback,{{0x1a64e903,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Be The Cop Results 1P",EndGameMenusCallback,{{0x67eb8d12,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Be The Cop Results 2P",EndGameMenusCallback,{{0x67eb8d33,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Points Rewards",EndGameMenusCallback,{{0x7e420558,0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {"Task Rewards",EndGameMenusCallback,{{0x7e420558,0,0,0}},{{-1,-1,-1,-1}},1,nullptr}
    }};
    return configurations;
}
}

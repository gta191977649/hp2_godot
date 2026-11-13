#include "indep/inc/EventTree.hpp"
#include "indep/inc/Timer.hpp"
#include "Language.hpp"
#include "indep/inc/Fred.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdio>
#include <cstring>

namespace hp2 {

tUnlockMap::tUnlockMap() {Reset();}
void tUnlockMap::Reset() {count_=0;}
void tUnlockMap::AddGoal(const EventGoal& goal,std::int32_t destination)
{
    // PAL1F72B0 retains at most four source prerequisite records.
    if(count_<4)entries_[static_cast<std::size_t>(count_++)]={goal,destination};
}
std::int32_t tUnlockMap::GetEventsUnlocked(std::int32_t* output,std::size_t capacity,
    const RaceEvent& event) const
{
    std::int32_t written=0;
    for(std::int32_t index=0;index<count_;++index) {
        const auto& entry=entries_[static_cast<std::size_t>(index)];
        if(event.GetStatus(entry.goal)!=4)continue;
        Require(output && static_cast<std::size_t>(written)<capacity,
            "UnlockMap output does not hold the actual unlocked event records");
        output[written++]=entry.destination;
    }
    return written;
}
std::int32_t tUnlockMap::GetNumEventsUnlocked(const RaceEvent& event) const
{
    std::array<std::int32_t,8> unlocked{};
    return GetEventsUnlocked(unlocked.data(),unlocked.size(),event);
}
void tUnlockMap::BuildForEvent(const EventInfo& event)
{
    // Native ownership projection of tEventLeaf::BuildUnlockList1F7480.
    // The leaf's outgoing edges are the authored goals naming this event.
    // Scan that same retained tree order; do not store a second leaf graph.
    Require(event.tree,"UnlockMap event has no authored event tree");
    Reset();
    // PAL1F9894..1F99D4 appends one outgoing leaf per prerequisite, in
    // authored event/goal order. BuildUnlockList then scans all that leaf's
    // goals for every occurrence. Preserve even repeated prerequisites.
    for(const auto& destination:event.tree->Events()) {
        for(std::size_t edge=0;edge<destination.goalIndices.size();++edge) {
            if(destination.GetGoal(static_cast<std::int32_t>(edge))->eventNumber!=
                static_cast<std::uint16_t>(event.number))continue;
            for(std::size_t i=0;i<destination.goalIndices.size();++i) {
                const auto* goal=destination.GetGoal(static_cast<std::int32_t>(i));
                if(goal->eventNumber==static_cast<std::uint16_t>(event.number) && destination.number!=32)
                    AddGoal(*goal,destination.number);
            }
        }
    }
    if(event.WillSuperBonusEventBeUnlockedIfThisEventIsWon())
        AddGoal(*event.GetObjective(),32);
}

std::int32_t DataEventSelection::GetSelectedEvent(std::int32_t type) const
{
    if (type==0)return selectedChampionshipEvent;
    if (type==1)return selectedHotPursuitEvent;
    return 1;
}

void DataEventSelection::SetSelectedEvent(std::int32_t ignoredType,std::int32_t event)
{
    (void)ignoredType;
    if (treeType==0)selectedChampionshipEvent=event;
    else if (treeType==1)selectedHotPursuitEvent=event;
}

std::array<EventDescriptionToken,8> eventDescriptionTokens{{
    {0,"$TARGET_TIME"},{1,"$NUM_LAPS"},{2,"$NUM_OPPONENTS"},{3,"$NUM_RACES"},
    {4,"$EVENTS_UNLOCKED"},{5,"$COMPLETION_TIME"},{6,"$EVENT_UNLOCKED"},{7,"$TRACK_NAME"}
}};

const EventDescriptionToken* GetEventDescriptionToken(const char* text)
{
    for (const auto& token:eventDescriptionTokens) {
        if (!std::strncmp(token.name.c_str(),text,token.name.size()))return &token;
    }
    return nullptr;
}

void FormatEventDescriptionToken(const EventTree& tree,const EventInfo& event,
    const EventDescriptionToken& token,char* output,const LanguageManager& language)
{
    const auto copy=[output](const char* text) {
        std::memcpy(output,text,std::strlen(text)+1);
    };
    switch (token.kind) {
    case 0: {
        Require(event.objectiveIndex>=0 && static_cast<std::size_t>(event.objectiveIndex)<tree.goals_.size(),
            "Description objective is outside its retained event tree");
        const auto& goal=tree.goals_[static_cast<std::size_t>(event.objectiveIndex)];
        if (goal.kind==2) {
            std::int32_t ticks;std::memcpy(&ticks,&goal.value,sizeof(ticks));
            char formatted[GAME_TIMER_TEXT_CAPACITY];
            gameTimerPrintToString(&ticks,formatted,0,&language.currentLanguage);
            const char* text=formatted;while (*text==' ')++text;
            copy(text);
        } else copy("zz:zz");
        break;
    }
    case 1: {
        Require(!event.trackIndices.empty(),"Description needs the first authored event track");
        const auto index=event.trackIndices.front();
        Require(index>=0 && static_cast<std::size_t>(index)<tree.tracks_.size(),
            "Description track is outside its retained event tree");
        std::snprintf(output,64,"%d",static_cast<int>((tree.tracks_[static_cast<std::size_t>(index)].configuration>>14)&15u));
        break;
    }
    case 2:std::snprintf(output,64,"%d",static_cast<int>(event.opponentIndices.size()));break;
    case 3:std::snprintf(output,64,"%d",event.GetRaceCount());break;
    // These are the actual final PAL texts; this method does not implement
    // unlock bookkeeping or guess names where the original prints a token.
    case 4:copy("events x,y,z are Unlocked!");break;
    case 5:copy("ff:ggg");break;
    case 6:copy("event x is unlocked");break;
    case 7:copy("TRACK_NAME_HERE");break;
    default:break;
    }
}

void FormatEventDescription(const EventTree& tree,const EventInfo& event,
    const char* input,char* output,std::int32_t limit,const LanguageManager& language)
{
    char* destination=output;
    while (*input && destination-output<limit) {
        if (*input=='$') {
            const auto* token=GetEventDescriptionToken(input);
            if (token) {
                Require(token->kind>=0 && token->kind<8,"Description token has no defined formatter");
                input+=token->name.size();
                char formatted[64];
                FormatEventDescriptionToken(tree,event,*token,formatted,language);
                for (const char* source=formatted;*source && destination-output<limit;++source)
                    *destination++=*source;
                continue;
            }
        }
        *destination++=*input++;
    }
    *destination=0;
}

} // namespace hp2

#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "FlailerDB.hpp"
#include "CarLoader.hpp"
#include "CarSounds.hpp"
#include "GameFlow.hpp"
#include "bware/inc/bMath.hpp"
#include "eMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace hp2 {
namespace {
constexpr std::uint32_t CampaignScreen=0x147f8a59u;
constexpr std::uint32_t LeafTemplateGroup=0xa01bcdc6u;
constexpr std::uint32_t DetailsScreen=0xa744ba7bu;
constexpr std::uint32_t DetailsGroup=0x2d3f3165u;
constexpr std::uint32_t RewardScreen=0xf1eed4cdu;
constexpr std::uint32_t RewardGroup=0x461d42d0u;
constexpr std::uint32_t BonusScreen=0xfd297fd4u;
constexpr std::int32_t ChampionshipTreeHelp[]{800033,800034,800035,0};
constexpr std::int32_t PursuitTreeHelp[]{800045,800046,800047,0};
constexpr std::int32_t ChampionshipDetailsHelp[]{800037,800038,801000,0};
constexpr std::int32_t PursuitDetailsHelp[]{800049,800050,801001,0};
constexpr std::int32_t ContinueDetailsHelp[]{800038,801000,800041,0};
constexpr std::int32_t ChampionshipRewardHelp[]{800044,0};
constexpr std::int32_t PursuitRewardHelp[]{800052,0};
using Point=std::array<float,2>;
using Color=std::array<float,4>;
struct CampaignPresentation {
    float verticalSpacing{350.0f},horizontalSpacing{130.0f},masterScale{1.0f};
    float blurbFade{0.15f},blurbDelay{0.15f},blurbFinish{0.35f},blurbX{382.0f};
    float trackLogoTravel{0.27f},indicatorPhaseStep{0.2f},indicatorInflation{1.05f},indicatorCorner{0.15f};
    float unselectedAlpha{0.2f};
    std::array<float,2> zoom{{0.4f,1.0f}},carLogoX{{140.0f,280.0f}};
    std::array<float,4> carLogoY{{70.0f,150.0f,210.0f,260.0f}};
    Color green{{0.0f,80.0f,0.0f,80.0f}},red{{60.0f,0.0f,0.0f,80.0f}};
    Color white{{128.0f,128.0f,128.0f,128.0f}},fog{{128.0f,128.0f,128.0f,128.0f}};
};
// Source writable presentation cells, including final global ctor1FB790 colors.
CampaignPresentation campaignPresentation;
SelectCarScreenServices* campaignServices{};

float CampaignSeconds(std::int32_t now,std::int32_t then)
{
    const auto bits=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(then);
    std::int32_t difference;std::memcpy(&difference,&bits,sizeof(difference));
    return static_cast<float>(difference)*0.0002500000118743628f;
}
std::int32_t CampaignAddTicks(std::int32_t value,std::int32_t addition)
{
    const auto bits=static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(addition);
    std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
}
void Desired(tCubic1D& curve,float value)
{
    curve.desiredValue=value;if (curve.value!=value)curve.mode=2;
}
void Snap(tCubic1D& curve) {curve.value=curve.desiredValue;curve.derivative=curve.desiredDerivative;curve.mode=0;}
void Help(SelectCarScreenServices& services,const std::int32_t* items)
{
    services.menus->screens->SetHelpOverride(items);
}
Point Transform(const Point& point,const std::array<float,16>& matrix)
{
    float input[4]{point[0],point[1],0.0f,1.0f},output[4]{};
    eMulVector4(output,matrix.data(),input);return {output[0],output[1]};
}
struct BitmapCopy final : FlailerBitmapChunk {
    explicit BitmapCopy(const FlailerBitmapChunk& source) : FlailerBitmapChunk(source)
    {strip=nullptr;next=previous=nullptr;updateKind=8;}
    ~BitmapCopy() {DeleteFlailerStrip();}
};
struct TextCopy final : FlailerTextChunk {
    explicit TextCopy(const FlailerTextChunk& source) : FlailerTextChunk(source)
    {
        strip=nullptr;next=previous=nullptr;updateKind=8;
        if (!source.ownedText.empty() && source.text==source.ownedText.data())text=ownedText.data();
    }
    ~TextCopy() {DeleteFlailerStrip();}
};
float TextureTime(const FlailerUiState& ui) {return static_cast<float>(ui.clock->realTimer)*0.0002500000118743628f;}
void RenderBitmap(FlailerUiState& ui,FlailerBitmapChunk& bitmap,float alpha)
{ui.render->RenderFlailerBitmap(*ui.resources,bitmap,alpha,TextureTime(ui));}
void RenderText(FlailerUiState& ui,FlailerTextChunk& text,float alpha)
{ui.render->RenderFlailerText(*ui.resources,text,ui.language,nullptr,nullptr,alpha);}
void RenderGroup(FlailerUiState& ui,std::uint32_t screen,std::uint32_t group,float alpha,bool bonus=false)
{
    FlailerObjectChunk* object{};
    while ((object=ui.resources->FindObjectByGroup(screen,group,0,object)) != nullptr) {
        if (bonus && (object->key==0x03c8f634u || object->key==0xf601f2d4u))continue;
        if (object->kind==2)RenderBitmap(ui,*static_cast<FlailerBitmapChunk*>(object),alpha);
        else if (object->kind==1)RenderText(ui,*static_cast<FlailerTextChunk*>(object),alpha);
    }
}
FlailerBitmapChunk& Bitmap(FlailerUiState& ui,std::uint32_t screen,std::uint32_t key)
{
    auto* object=ui.resources->FindObject(screen,key,2);
    Require(object,"Campaign screen is missing an authored bitmap");return *static_cast<FlailerBitmapChunk*>(object);
}
void SetObjectColor(FlailerBitmapChunk& bitmap,const Color& color)
{
    const auto r=static_cast<std::uint32_t>(convertFloatToInt32(color[0]));
    const auto g=static_cast<std::uint32_t>(convertFloatToInt32(color[1]));
    const auto b=static_cast<std::uint32_t>(convertFloatToInt32(color[2]));
    bitmap.SetColor(r|(g<<8)|(b<<16));bitmap.SetAlpha(convertFloatToInt32(color[3]));
}
void RenderLine(FlailerUiState& ui,bool horizontal,float first,float last,float cross,const Color& color)
{
    // PAL1F6CF0/1F6DF8 copy the actual line bitmap and scale its authored extent.
    const auto key=horizontal?0x02eca94fu:0x03ea005du;
    auto* source=ui.resources->FindObject(CampaignScreen,key,2);if (!source)return;
    BitmapCopy copy(*static_cast<FlailerBitmapChunk*>(source));
    const float width=ui.resources->ScaledBitmapWidth(copy),height=ui.resources->ScaledBitmapHeight(copy);
    Require(width!=0.0f && height!=0.0f,"Campaign line bitmap has an empty authored extent");
    if (last<first)std::swap(first,last);
    if (horizontal) {copy.topLeftX=first;copy.topLeftY=cross-height*0.5f;copy.scaleX=(last-first)/width;}
    else {copy.topLeftX=cross-width*0.5f;copy.topLeftY=first;copy.scaleY=(last-first)/height;}
    SetObjectColor(copy,color);RenderBitmap(ui,copy,1.0f);
}

struct CampaignLeaf {
    const EventInfo* event{};
    std::size_t index{};
    std::int32_t generation{};
    Point position{},masterCenter{};
    float selectionScale{1.0f};
    FlailerBitmapChunk* master{};
    const EditorMarkerTexture* texture{};
    std::vector<CampaignLeaf*> parents,children;
    std::unique_ptr<tTrackModelViewer> track;
    std::unique_ptr<tCarModelViewer> car;
    std::string rewardString;

    CampaignLeaf(SelectCarScreenServices& services,const EventInfo& supplied,std::size_t ordinal)
        :event(&supplied),index(ordinal)
    {
        auto& ui=*services.menus->ui;master=&Bitmap(ui,CampaignScreen,0xa3b23510u);
        const auto status=event->GetStatus();
        const char* suffix=status==0?"LOCKED":status==4?"WON":status>=1 && status<=3?"UNLOCKED":nullptr;
        if (suffix) {
            const std::string name=std::string(event->tree->TreeType()==0?"SPEED_EVENT_PANEL_":"PURSUIT_EVENT_PANEL_")+suffix;
            texture=ui.resources->FindTexture(hashString(name.c_str()));
            Require(texture,"Campaign event panel has no authored texture");master->ChangeTexture(texture);
        }
        masterCenter={master->topLeftX+ui.resources->ScaledBitmapWidth(*master)*0.5f,
            master->topLeftY+ui.resources->ScaledBitmapHeight(*master)*0.5f};
        if (const auto* reward=event->GetReward(0)) {
            const auto kind=reward->selection&255u;
            if (kind==0) {
                car=std::make_unique<tCarModelViewer>(services,*reward);
                const auto model=static_cast<std::size_t>((reward->selection>>8)&255u);
                rewardString=GetTranslatedString(*ui.language,services.menus->raceSetup->cars->Car(model).GetFullName(*ui.language));
            }
            else if (kind==1) {
                track=std::make_unique<tTrackModelViewer>(services,*reward);
                const auto number=static_cast<std::int32_t>((reward->selection>>8)&255u);
                const auto direction=static_cast<std::int32_t>((reward->selection>>16)&255u);
                rewardString=ui.language->GetString(GetTheRightTrackLanguageID(*services.menus->raceSetup->tracks,number,direction));
            }
        }
    }
    std::array<Point,2> Bounds(FlailerUiState& ui) const
    {
        const float halfWidth=ui.resources->ScaledBitmapWidth(*master)*0.5f;
        const float halfHeight=ui.resources->ScaledBitmapHeight(*master)*0.5f;
        return {{{position[0]-halfWidth,position[1]-halfHeight},{position[0]+halfWidth,position[1]+halfHeight}}};
    }
    bool Visible(const EventTreeResources& trees) const
    {
        if (event->number==31)return event->GetStatus()!=0;
        if (event->number!=32 || event->GetStatus()!=0)return true;
        for (std::int32_t type=0;type<2;++type) {
            auto* tree=trees.PrimaryTree(type);Require(tree,"Campaign visibility has no actual primary tree");
            const auto* bonus=tree->GetEventNumber(31);Require(bonus,"Campaign visibility has no authored bonus event");
            if (bonus->GetStatus()==4)return true;
        }
        return false;
    }
    bool Fogged() const
    {
        std::int32_t maximum=0;for (const auto* parent:parents)maximum=std::max(maximum,parent->event->GetStatus());
        return event->GetStatus()==0 && maximum<=0;
    }
    void Render(SelectCarScreenServices& services,const std::array<float,16>& view,float alpha)
    {
        auto& ui=*services.menus->ui;if (!Visible(*services.menus->raceSetup->events))return;
        auto matrix=view;for (auto& value:matrix)value*=campaignPresentation.masterScale;
        FlailerObjectChunk* object{};
        while ((object=ui.resources->FindObjectByGroup(CampaignScreen,LeafTemplateGroup,0,object)) != nullptr) {
            if (object->kind==2) {
                BitmapCopy copy(*static_cast<FlailerBitmapChunk*>(object));
                if (copy.key==master->key && texture)copy.ChangeTexture(texture);
                const auto placed=Transform({copy.topLeftX+position[0]-masterCenter[0],
                    copy.topLeftY+position[1]-masterCenter[1]},matrix);
                copy.topLeftX=placed[0];copy.topLeftY=placed[1];copy.scaleX*=matrix[0];copy.scaleY*=matrix[5];
                RenderBitmap(ui,copy,alpha);
            } else if (object->kind==1) {
                TextCopy copy(*static_cast<FlailerTextChunk*>(object));
                const bool alignNumber=ui.externalPresentation && copy.key==0x16b7d629u;
                auto anchor=position;
                // EVENT_NUMBER is centered. Keep its authored vertical
                // relation to the panel instead of using the panel center
                // as the top of the glyphs.
                if (alignNumber)anchor[1]+=copy.topLeftY-masterCenter[1];
                const auto placed=Transform(anchor,matrix);
                copy.topLeftX=placed[0];copy.SetTopLeftY(placed[1]);copy.scaleX*=matrix[0];copy.scaleY*=matrix[5];
                copy.SetFormatOptions(16);
                if (copy.key==0x16b7d629u) {
                    char number[32];
                    std::snprintf(number,sizeof(number),alignNumber?"%d":"%2d",event->number);
                    copy.SetString(number);
                }
                copy.SetAlpha(convertFloatToInt32((Fogged()?campaignPresentation.fog:campaignPresentation.white)[3]));
                RenderText(ui,copy,alpha);
            }
        }
        const auto bounds=Bounds(ui);
        if (!parents.empty()) {
            const auto other=parents.front()->Bounds(ui);
            const auto first=Transform({position[0],bounds[0][1]},matrix);
            const auto last=Transform({position[0],(bounds[0][1]+other[1][1])*0.5f},matrix);
            RenderLine(ui,false,first[1]+5.0f,last[1],first[0],event->GetStatus()>0?campaignPresentation.green:campaignPresentation.red);
        }
        if (!children.empty() && children.front()->Visible(*services.menus->raceSetup->events)) {
            const auto other=children.front()->Bounds(ui);
            const auto first=Transform({position[0],bounds[1][1]},matrix);
            const auto last=Transform({position[0],(bounds[1][1]+other[0][1])*0.5f},matrix);
            RenderLine(ui,false,first[1]-(event->tree->TreeType()==1?5.0f:16.0f),last[1],first[0],
                event->GetStatus()>=4?campaignPresentation.green:campaignPresentation.red);
        }
        // Final RenderDot is jr ra/nop; no marker is synthesized.
    }
};
struct CampaignGroup {
    struct Segment {Point first{},last{};Color color{};};
    std::vector<CampaignLeaf*> members,top,bottom;
    std::vector<Segment> segments;
    bool Contains(const CampaignLeaf* leaf)const {return std::find(members.begin(),members.end(),leaf)!=members.end();}
    void Add(CampaignLeaf* leaf) {if (!Contains(leaf))members.push_back(leaf);}
    void Build()
    {
        Require(!members.empty(),"Campaign group has no actual leaves");
        auto minimum=members.front()->generation,maximum=minimum;
        for (const auto* member:members) {minimum=std::min(minimum,member->generation);maximum=std::max(maximum,member->generation);}
        for (auto* member:members) {if (member->generation==minimum)top.push_back(member);if (member->generation==maximum)bottom.push_back(member);}
        std::stable_sort(members.begin(),members.end(),[](const auto* a,const auto* b){return a->position[0]<b->position[0];});
        const float y=(top.front()->position[1]+bottom.front()->position[1])*0.5f;
        for (std::size_t i=1;i<members.size();++i)
            segments.push_back({{members[i-1]->position[0],y},{members[i]->position[0],y},campaignPresentation.red});
        for (const auto* child:bottom) {
            if (child->event->GetStatus()<=0)continue;
            for (std::size_t i=0;i<child->event->goalIndices.size();++i) {
                const auto* goal=child->event->GetGoal(static_cast<std::int32_t>(i));
                const auto found=std::find_if(members.begin(),members.end(),[&](const auto* member){return member->event->number==goal->eventNumber;});
                if (found==members.end() || (*found)->event->GetStatus(goal)<2)continue;
                const float first=std::min((*found)->position[0],child->position[0]);
                const float last=std::max((*found)->position[0],child->position[0]);
                for (auto& segment:segments) {const auto x=(segment.first[0]+segment.last[0])*0.5f;if (x>=first && x<=last)segment.color=campaignPresentation.green;}
            }
        }
    }
    void Render(FlailerUiState& ui,const std::array<float,16>& matrix)const
    {
        for (const auto& segment:segments) {
            const auto first=Transform(segment.first,matrix),last=Transform(segment.last,matrix);
            RenderLine(ui,true,first[0],last[0],first[1],segment.color);
        }
    }
};

std::string RewardName(SelectCarScreenServices& services,const EventInfo& event)
{
    auto& ui=*services.menus->ui;const auto* reward=event.GetReward(0);
    if (!reward)return ui.language->GetString(66008);
    const auto number=static_cast<std::int32_t>((reward->selection>>8)&255u);
    if ((reward->selection&255u)==0)
        return GetTranslatedString(*ui.language,services.menus->raceSetup->cars->Car(static_cast<std::size_t>(number)).GetFullName(*ui.language));
    if ((reward->selection&255u)==1)
        return ui.language->GetString(GetTheRightTrackLanguageID(*services.menus->raceSetup->tracks,number,
            static_cast<std::int32_t>((reward->selection>>16)&255u)));
    return {};
}
std::string StatusName(const EventInfo& event,const LanguageManager& language)
{
    const auto status=event.GetStatus();if (status<0 || status>4)return {};
    const auto* format=language.GetString(62000+status);
    if (status<=1)return format;
    return status==2?FormatLocalizedText(format,{event.GetRaceIndex(),event.GetRaceCount()},256):
        FormatLocalizedText(format,{event.GetFinishString(language).c_str()},256);
}
class CampaignBlurb {
public:
    explicit CampaignBlurb(SelectCarScreenServices& services):services_(services)
    {
        master_=services_.menus->ui->resources->FindObject(CampaignScreen,0xcea3b7c2u,1);
        Require(master_,"Campaign blurb has no authored text anchor");
    }
    void SetEvent(const EventInfo& event)
    {
        if (event_==&event)return;
        event_=&event;started_=Ui().clock->realTimer;
        savedAlpha_=static_cast<FlailerTextChunk*>(master_)->GetAlpha();
        Ui().resources->SetGroupAlpha(CampaignScreen,0xa2a16ea1u,0);
    }
    void StartRewound() {savedAlpha_=0;started_=CampaignAddTicks(started_,convertFloatToInt32(campaignPresentation.blurbDelay*4000.0f));}
    void Animate()
    {
        if (!event_)return;
        const auto elapsed=CampaignSeconds(Ui().clock->realTimer,started_);
        float x=master_->topLeftX,alpha=0.0f;
        if (elapsed>campaignPresentation.blurbDelay) {
            x=campaignPresentation.blurbX;
            if (elapsed<campaignPresentation.blurbFinish)
                x+=(640.0f-x)*(campaignPresentation.blurbFinish-elapsed)/(campaignPresentation.blurbFinish-campaignPresentation.blurbDelay);
            Redraw();
        }
        if (elapsed<campaignPresentation.blurbFade)alpha=static_cast<float>(savedAlpha_)*(1.0f-elapsed/campaignPresentation.blurbFade);
        else if (elapsed>campaignPresentation.blurbDelay)alpha=128.0f;
        Ui().resources->MoveGroupTo(CampaignScreen,0xa2a16ea1u,master_->key,x,master_->topLeftY);
        Ui().resources->SetGroupAlpha(CampaignScreen,0xa2a16ea1u,convertFloatToInt32(alpha));
    }
private:
    FlailerUiState& Ui()const {return *services_.menus->ui;}
    void Redraw()
    {
        auto& ui=Ui();const auto status=StatusName(*event_,*ui.language),reward=RewardName(services_,*event_);
        ui.resources->SetLocalizedText(CampaignScreen,0x87cfd6c6u,status.c_str());
        ui.resources->Printf(CampaignScreen,0x35b77247u,"%s",reward.c_str());
        const auto title=FormatLocalizedText(ui.language->GetString(70063),{static_cast<std::int32_t>(event_->number)});
        ui.resources->SetLocalizedText(CampaignScreen,master_->key,title.c_str());
    }
    SelectCarScreenServices& services_;FlailerObjectChunk* master_{};const EventInfo* event_{};
    std::int32_t started_{},savedAlpha_{};
};
class CampaignCarLogos {
public:
    explicit CampaignCarLogos(SelectCarScreenServices& services):services_(services),bitmap_(Bitmap(Ui(),CampaignScreen,0xc6398c48u))
    {savedAlpha_=bitmap_.GetAlpha(0);started_=Ui().clock->realTimer;}
    ~CampaignCarLogos() {services_.menus->screens->background->DefaultTextureList();bitmap_.SetAlpha(savedAlpha_);}
    void SetEvent(const EventInfo& event)
    {
        auto& background=*services_.menus->screens->background;background.DefaultTextureList();count_=0;
        for (std::size_t i=0;i<event.carIndices.size() && i<keys_.size();++i) {
            const auto model=(static_cast<std::uint32_t>(*event.GetCar(static_cast<std::int32_t>(i)))>>16)&255u;
            const auto& car=services_.menus->raceSetup->cars->Car(model);
            const auto key=hashStringContinue(car.name.c_str(),hashString("CARLOGO_"));
            keys_[count_++]=key;background.AddTextureHash(key);
        }
        current_=x_=y_=0;phase_=1;StartTexture(false);
        started_=CampaignAddTicks(Ui().clock->realTimer,-22000);Show(true);
    }
    void Show(bool enabled) {Ui().resources->SetObjectVisibility(CampaignScreen,bitmap_.key,enabled?1:0);}
    void Animate()
    {
        const float elapsed=CampaignSeconds(Ui().clock->realTimer,started_);
        if (elapsed<=5.0f)return;
        if (elapsed<5.5f) {bitmap_.SetAlpha(convertFloatToInt32(static_cast<float>(savedAlpha_)*(1.0f-(elapsed-5.0f)*2.0f)));phase_=0;}
        else if (elapsed<6.0f) {
            if (!phase_) {if (++current_>=count_)current_=0;StartTexture(true);phase_=1;}
            bitmap_.SetAlpha(convertFloatToInt32(static_cast<float>(savedAlpha_)*(elapsed-5.5f)*2.0f));
        } else started_=Ui().clock->realTimer;
    }
private:
    FlailerUiState& Ui()const {return *services_.menus->ui;}
    void StartTexture(bool random)
    {
        if (current_<count_)bitmap_.ChangeTexture(keys_[current_]);
        if (random) {y_=(y_+vectorDefaultRandomBounded(3)+1)%4;x_=(x_+vectorDefaultRandomBounded(2))&1u;}
        bitmap_.SetTopLeftX(campaignPresentation.carLogoX[x_]);bitmap_.SetTopLeftY(campaignPresentation.carLogoY[y_]);
    }
    SelectCarScreenServices& services_;FlailerBitmapChunk& bitmap_;
    std::array<std::uint32_t,8> keys_{};std::size_t count_{},current_{},x_{},y_{};
    std::int32_t savedAlpha_{},started_{},phase_{};
};
class CampaignTrackLogos {
public:
    explicit CampaignTrackLogos(SelectCarScreenServices& services):services_(services),bitmap_(Bitmap(Ui(),CampaignScreen,0xf60114e4u))
    {savedX_=bitmap_.topLeftX;savedAlpha_=bitmap_.GetAlpha(0);Show(true);}
    ~CampaignTrackLogos() {bitmap_.SetTopLeftX(savedX_);bitmap_.SetAlpha(savedAlpha_);}
    void SetEvent(const EventInfo& event)
    {
        count_=0;
        for (std::size_t i=0;i<event.trackIndices.size() && i<keys_.size();++i) {
            const auto configuration=event.GetTrack(static_cast<std::int32_t>(i))->configuration;
            const auto* track=services_.menus->raceSetup->tracks->Find(static_cast<std::int32_t>(configuration&255u));
            Require(track,"Campaign logo names an absent authored track");
            keys_[count_++]=FigureOutTrackNameImageHash(track->baseNumber,static_cast<std::int32_t>((configuration>>9)&1u),*Ui().language);
        }
        if (CampaignSeconds(Ui().clock->realTimer,started_)<0.35f)bitmap_.SetTopLeftX(640.0f);
        current_=0;started_=Ui().clock->realTimer;Show(true);
    }
    void Show(bool enabled) {Ui().resources->SetObjectVisibility(CampaignScreen,bitmap_.key,enabled?1:0);}
    void Animate()
    {
        const float elapsed=CampaignSeconds(Ui().clock->realTimer,started_);
        if (elapsed<0.35f) {bitmap_.SetAlpha(convertFloatToInt32(static_cast<float>(savedAlpha_)*(1.0f-elapsed/0.35f)));phase_=0;}
        else if (elapsed>0.35f && elapsed<=0.35f+campaignPresentation.trackLogoTravel) {
            if (!phase_) {if (current_<count_)bitmap_.ChangeTexture(keys_[current_]);phase_=1;}
            bitmap_.SetTopLeftX(640.0f+(elapsed-0.35f)/campaignPresentation.trackLogoTravel*(savedX_-640.0f));bitmap_.SetAlpha(savedAlpha_);
        } else if (elapsed>0.35f+campaignPresentation.trackLogoTravel)bitmap_.SetTopLeftX(savedX_);
    }
private:
    FlailerUiState& Ui()const {return *services_.menus->ui;}
    SelectCarScreenServices& services_;FlailerBitmapChunk& bitmap_;std::array<std::uint32_t,12> keys_{};
    std::size_t count_{},current_{};float savedX_{};std::int32_t savedAlpha_{},started_{},phase_{};
};

class tEventTreeView final {
public:
    explicit tEventTreeView(SelectCarScreenServices& services,EventTree& tree)
        :services_(services),tree_(tree),blurb_(services),
          animated_(*services.menus->ui,CampaignScreen,0xfecebef6u,0xfecebef6u,0x70a404ecu),
          carLogos_(services),trackLogos_(services)
    {
        auto& ui=Ui();const auto& setup=*services_.menus->raceSetup;
        Require(soundJukeboxStart(*services_.menus->sounds,Frontend(),*setup.parameters,*setup.flow,true)==WORLD_AUDIO_OK,
            "Campaign jukebox could not start on the actual audio owner");
        for (const auto& event:tree_.Events())leaves_.push_back(std::make_unique<CampaignLeaf>(services_,event,leaves_.size()));
        Require(!leaves_.empty(),"Campaign has no authored events");
        for (auto& leaf:leaves_)for (std::size_t i=0;i<leaf->event->goalIndices.size();++i) {
            const auto* goal=leaf->event->GetGoal(static_cast<std::int32_t>(i));auto* parent=FindLeaf(goal->eventNumber);
            if (parent) {leaf->parents.push_back(parent);parent->children.push_back(leaf.get());}
        }
        for (auto& leaf:leaves_) {
            auto* ancestor=leaf.get();
            while (!ancestor->parents.empty()) {
                Require(static_cast<std::size_t>(leaf->generation)<leaves_.size(),"Campaign prerequisite chain contains a cycle");
                ++leaf->generation;ancestor=ancestor->parents.front();
            }
        }
        for (auto& leaf:leaves_) {
            if (leaf->children.empty())continue;
            auto found=std::find_if(groups_.begin(),groups_.end(),[&](const auto& group){return group.Contains(leaf->children.front());});
            if (found!=groups_.end())found->Add(leaf.get());
            else {groups_.emplace_back();auto& group=groups_.back();group.Add(leaf.get());for (auto* child:leaf->children)group.Add(child);}
        }
        InstallLayout();
        current_=FindLeaf(Frontend().eventSelection.GetSelectedEvent(tree_.TreeType()));
        if (!current_)current_=leaves_.front().get();
        if ((current_->event->number==31 || current_->event->number==32) && current_->event->GetStatus()==0)current_=leaves_.front().get();
        blurb_.SetEvent(*current_->event);blurb_.StartRewound();
        services_.menus->screens->StopMovie(); // exact PAL1F9C54 -> StopMovie16D0A0.
        ui.resources->SetGroupVisibility(CampaignScreen,LeafTemplateGroup,0);
        for (auto* curve:{&zoom_,&pan_.axes[0],&pan_.axes[1],&alpha_}) {
            tCubic1DInitialize(curve);curve->duration=0.5f;curve->continuous=0;
        }
        for (auto& curve:parallax_.axes) {tCubic1DInitialize(&curve);curve.duration=0.5f;}
        parallax_.axes[0].value=320.0f;parallax_.axes[1].value=240.0f;
        for (auto& curve:parallax_.axes)if (curve.value!=curve.desiredValue)curve.mode=2;
        Desired(alpha_,1.0f);Snap(alpha_);MaybeChangeZoom(0);Snap(zoom_);for (auto& curve:pan_.axes)Snap(curve);Recenter();
        ui.resources->SetLanguageID(CampaignScreen,0xa01f24f0u,Frontend().gameType==8?10026u:Frontend().gameType==9?10029u:0u);
        const auto* superBonus=tree_.GetEventNumber(32);
        Require(superBonus,"Campaign tree has no authored super bonus event");
        if (superBonus->GetStatus()>0 && superBonus->GetStatus()!=4) {state_=7;animated_.Hide();}
        else {
            state_=Frontend().eventSelection.lastViewState;
            Require(state_==0 || state_==2,"Saved campaign view is outside its source stable states");
            if (state_==2)TreeToDetails();
            animated_.NewText(current_->event->summaryLanguageId);
        }
        SyncHelp();
        std::int32_t left{},top{},right{},bottom{};
        Require(ui.resources->GetBoundingBox(CampaignScreen,0xaa5d7d3fu,left,top,right,bottom,ui.language),
            "Campaign tree has no authored clipping rectangle");
        clipMin_={static_cast<float>(left),static_cast<float>(top)};clipMax_={static_cast<float>(right),static_cast<float>(bottom)};
        camera_=std::make_unique<FrontendStillCamera>(*services_.cameras,1);
        carLogos_.SetEvent(*current_->event);
        for (auto& group:groups_)group.Build();
        const auto* message=ui.language->GetString(0x000e32c9);
        const auto length=std::strlen(message);Require(length<bonusText_.size(),"Campaign bonus description exceeds source text extent");
        std::memcpy(bonusText_.data(),message,length+1);
    }
    ~tEventTreeView() {Help(services_,nullptr);}
    void OnTick()
    {
        const auto elapsed=Ui().clock->realTimeElapsed;
        tCubic1DUpdate(&zoom_,elapsed,0.0f,0.0f);tCubic2DUpdate(&pan_,elapsed);tCubic1DUpdate(&alpha_,elapsed,0.0f,0.0f);
        blurb_.Animate();
        if (!alpha_.mode) {
            if (state_==1 || state_==6) {if (state_==6)UnActivateReward();state_=2;Frontend().eventSelection.lastViewState=2;}
            else if (state_==3) {state_=0;Frontend().eventSelection.lastViewState=0;}
            else if (state_==4) {state_=5;ActivateReward();}
        }
        Ui().resources->SetGroupVisibility(CampaignScreen,0xa2a16ea1u,state_==0?1:0);
        tCubic2DUpdate(&parallax_,elapsed);
        animated_.Animate();
        if (current_->track)current_->track->TickTrack(*camera_);
        if (current_->car)current_->car->Tick(*camera_);
        carLogos_.Animate();trackLogos_.Animate();
    }
    void OnRender()
    {
        if (state_==0 || state_==1 || state_==3) {
            std::array<float,16> matrix{};eIdentity(matrix.data());matrix[0]=matrix[5]=zoom_.value;
            matrix[12]=pan_.axes[0].value;matrix[13]=pan_.axes[1].value;
            struct RestoreClip {~RestoreClip(){ResetFlailerTextClipWindow();}} restore;
            SetFlailerTextClipWindow(clipMin_.data(),clipMax_.data());
            for (const auto& group:groups_)group.Render(Ui(),matrix);
            for (auto& leaf:leaves_) {
                float alpha=leaf.get()==current_?alpha_.value:campaignPresentation.unselectedAlpha;
                if (leaf->event->number==32 && leaf->event->GetStatus()==0)alpha=campaignPresentation.unselectedAlpha;
                if (leaf.get()==current_ && state_!=0)ResetFlailerTextClipWindow();
                else SetFlailerTextClipWindow(clipMin_.data(),clipMax_.data());
                leaf->Render(services_,matrix,alpha);
            }
            RenderIndicator(matrix);
        }
        if (state_==1 || state_==2 || state_==3 || state_==4 || state_==6)RenderDetails(1.0f-alpha_.value);
        if (state_==4 || state_==5 || state_==6)RenderReward(alpha_.value);else UnActivateReward();
        if (state_==7) {
            RenderGroup(Ui(),BonusScreen,DetailsGroup,alpha_.value,true);
            Ui().render->RenderTextIntoRectangle(*Ui().resources,BonusScreen,0x7a31036bu,0x7ad2801eu,bonusText_.data(),alpha_.value);
        }
    }
    void HandleMessage(ScreenMessage message,ScreenCallbackData& data)
    {
        if (state_==1 || state_==3 || state_==4 || state_==6) {data.value=0;data.sound=0;return;}
        if (state_==2) {
            if (message==ScreenMessage::LeaveScreen) {DetailsToTree();data.value=0;data.sound=21;return;}
            if (message==ScreenMessage::ItemEvent) {
                if (!current_->event->rewardIndices.empty()) {DetailsToReward();data.sound=24;}else data.sound=8;
                return;
            }
            if (message==ScreenMessage::Select) {
                if (current_->event->GetStatus()>0) {data.sound=22;StartEvent(data);return;}
                data.sound=23;
            } else if (IsDirection(message))data.sound=0;
            return;
        }
        if (state_==5) {
            if (message==ScreenMessage::ItemEvent || message==ScreenMessage::LeaveScreen) {
                if (message==ScreenMessage::LeaveScreen)data.value=0;
                RewardToDetails();data.sound=24;
            } else if (IsDirection(message))data.sound=0;
            return;
        }
        if (state_==7) {
            if (message==ScreenMessage::Select) {data.sound=5;ExitBonus();}
            else if (IsDirection(message))data.sound=0;
            return;
        }
        switch (message) {
        case ScreenMessage::Select:SaveSelected();TreeToDetails();data.sound=21;break;
        case ScreenMessage::Left:data.sound=MaybeChangeLeaf(-1)?20:0;break;
        case ScreenMessage::Right:data.sound=MaybeChangeLeaf(1)?20:0;break;
        case ScreenMessage::Previous:data.sound=MaybeChangeLeaf(2)?19:0;break;
        case ScreenMessage::Next:data.sound=MaybeChangeLeaf(-2)?19:0;break;
        case ScreenMessage::LeaveScreen:SaveSelected();break;
        // Final ScreenEvent13 asks CanCheat16FDB0, exactly return false.
        // Preserve its disabled branch; no unlock or debug state is fabricated.
        default:break;
        }
    }
private:
    FlailerUiState& Ui()const {return *services_.menus->ui;}
    FrontendPlayerManager& Frontend()const {return *services_.menus->frontend;}
    static bool IsDirection(ScreenMessage message)
    {return message>=ScreenMessage::Left && message<=ScreenMessage::Next;}
    CampaignLeaf* FindLeaf(std::int32_t number)const
    {for (const auto& leaf:leaves_)if (leaf->event->number==number)return leaf.get();return nullptr;}
    void SaveSelected() {Frontend().eventSelection.SetSelectedEvent(tree_.TreeType(),current_->event->number);}
    void Recenter()
    {
        const float desired[2]{320.0f-current_->position[0]*zoom_.desiredValue,240.0f-current_->position[1]*zoom_.desiredValue};
        tCubic2DSetValDesired(&pan_,desired);
    }
    bool MaybeChangeZoom(std::int32_t step)
    {
        const auto next=std::clamp(zoomIndex_+step,0,1);if (next==zoomIndex_ && step)return false;
        zoomIndex_=next;Desired(zoom_,campaignPresentation.zoom[static_cast<std::size_t>(next)]);Recenter();return true;
    }
    bool MaybeChangeLeaf(std::int32_t direction)
    {
        CampaignLeaf* candidate{};float distance=std::numeric_limits<float>::max();
        if (direction==1 || direction==-1) {
            for (const auto& leaf:leaves_) {
                if (leaf.get()==current_ || leaf->generation!=current_->generation)continue;
                const auto difference=leaf->position[0]-current_->position[0];
                if ((direction==1 && difference<=0.0f) || (direction==-1 && difference>=0.0f))continue;
                if (std::fabs(difference)<distance) {distance=std::fabs(difference);candidate=leaf.get();}
            }
        } else {
            const auto& choices=direction==2?current_->parents:current_->children;
            for (auto* leaf:choices) {const auto difference=std::fabs(leaf->position[0]-current_->position[0]);if (difference<distance) {distance=difference;candidate=leaf;}}
        }
        if (!candidate || candidate==current_ || !candidate->Visible(*services_.menus->raceSetup->events))return false;
        current_->selectionScale=1.0f;current_=candidate;current_->selectionScale=2.0f;
        blurb_.SetEvent(*current_->event);animated_.NewText(current_->event->summaryLanguageId);
        carLogos_.SetEvent(*current_->event);trackLogos_.SetEvent(*current_->event);Recenter();
        const float desired[2]{FlailerAnimationRandomRange(0.0f,320.0f),FlailerAnimationRandomRange(0.0f,240.0f)};
        tCubic2DSetValDesired(&parallax_,desired);return true;
    }
    void SyncHelp()
    {
        if (state_==1)DetailsHelp();else Help(services_,tree_.TreeType()==0?ChampionshipTreeHelp:PursuitTreeHelp);
        if (state_==7)Ui().helpEnabled=0;
        for (auto key:{0x0880ac4cu,0x027f7503u})Ui().resources->SetObjectVisibility(CampaignScreen,key,state_==7?0:1);
    }
    void DetailsHelp()
    {
        if (current_->event->IsMultiTrack() && current_->event->GetStatus()==2)Help(services_,ContinueDetailsHelp);
        else Help(services_,tree_.TreeType()==0?ChampionshipDetailsHelp:PursuitDetailsHelp);
    }
    void ActivateReward() {Help(services_,tree_.TreeType()==0?ChampionshipRewardHelp:PursuitRewardHelp);}
    void UnActivateReward()
    {
        if (current_->car)current_->car->MakeInvisible();
        Ui().resources->Printf(RewardScreen,0x7e428084u,"%s",current_->rewardString.c_str());
    }
    void TreeToDetails()
    {state_=1;Desired(alpha_,0.0f);Desired(zoom_,3.0f);Recenter();if (current_->car)current_->car->BeginLoading();DetailsHelp();}
    void DetailsToTree()
    {if (current_->car)current_->car->Unload();state_=3;Desired(alpha_,1.0f);Desired(zoom_,campaignPresentation.zoom[static_cast<std::size_t>(zoomIndex_)]);Recenter();SyncHelp();}
    void DetailsToReward()
    {state_=4;Desired(alpha_,1.0f);trackLogos_.Show(false);carLogos_.Show(false);ActivateReward();}
    void RewardToDetails()
    {state_=6;Desired(alpha_,0.0f);carLogos_.Show(true);trackLogos_.Show(true);DetailsHelp();}
    void ExitBonus()
    {
        state_=Frontend().eventSelection.lastViewState;SyncHelp();if (state_==2)TreeToDetails();
        animated_.NewText(current_->event->summaryLanguageId);
        for (auto key:{0x0880ac4cu,0x027f7503u})Ui().resources->SetObjectVisibility(CampaignScreen,key,1);
    }
    void RenderDetails(float alpha)
    {
        const auto& event=*current_->event;auto& setup=*services_.menus->raceSetup;
        if (Ui().advancePresentation && detailsEvent_!=&event) {
            FormatEventDescription(tree_,event,Ui().language->GetString(event.descriptionLanguageId),description_.data(),512,*Ui().language);
            auto* track=event.GetTrack(event.GetRaceIndex());if (!track)track=event.GetTrack(0);
            Require(track,"Campaign details have no authored event track");
            const auto id=GetTheRightTrackLanguageID(*setup.tracks,static_cast<std::int32_t>(track->configuration&255u),
                static_cast<std::int32_t>((track->configuration>>9)&1u));
            Ui().resources->Printf(DetailsScreen,0xf601f2d4u,"%s",Ui().language->GetString(id));detailsEvent_=&event;
            if (!(event.IsMultiTrack() && event.GetStatus()==2)) {
                Frontend().gameType=tree_.TreeType()==0?8:9;Frontend().MakeEventCarsAvailable(tree_,event);PreloadCurrentSelectedCars(Frontend());
            }
        }
        RenderGroup(Ui(),DetailsScreen,DetailsGroup,alpha);
        Ui().render->RenderTextIntoRectangle(*Ui().resources,DetailsScreen,0x7a31036bu,0x7ad2801eu,description_.data(),alpha);
    }
    void RenderReward(float alpha)
    {
        if (current_->track)current_->track->RenderTrack();
        if (current_->car)current_->car->MakeVisible();
        RenderGroup(Ui(),RewardScreen,RewardGroup,alpha);
    }
    void StartEvent(ScreenCallbackData& data)
    {
        auto& setup=*services_.menus->raceSetup;auto& frontend=Frontend();auto* screens=services_.menus->screens;
        auto* selected=tree_.GetEventNumber(current_->event->number);
        Require(selected==current_->event,"Campaign race selection lost its canonical event owner");
        auto& event=*selected;
        // PAL1F6A60/1F6A70 write FRED+10 and RaceParameters+254.
        // These are the authored race option, not either player-count cell.
        frontend.challengeSetup.raceOption254=1;
        setup.parameters->authoredRaceOption254=1;
        frontend.selectedEventNumber=event.number;
        tree_.SetRace(*setup.parameters,event,{&frontend,setup.currentEvent,setup.cars,setup.tracks,setup.language});
        setup.currentEvent->unlocks.BuildForEvent(event);
        if (event.IsMultiTrack() && event.GetStatus()==2) {
            if (data.joystickPort==0) {
                // PAL1F6AD4 ->1F5C28 ->16C760 ->ScreenStack::Push.
                // The tree is the active screen, so it is not yet on the
                // back stack. PopUntil would discard every return target
                // and leave LaunchFrontendMenus with an empty name.
                screens->stack.Push("View Event Tree");
                frontend.StartRace(setup);
            }
            return;
        }
        frontend.gameType=tree_.TreeType()==0?8:9;frontend.MakeEventCarsAvailable(tree_,event);
        // ChangeToScreen synchronously destroys this callback owner. Nothing
        // below that boundary reads members or a string owned by this object.
        screens->ChangeToScreen("Select Car",true);
    }
    void RenderIndicator(const std::array<float,16>& matrix)
    {
        auto bounds=current_->Bounds(Ui());
        const Point center{{(bounds[0][0]+bounds[1][0])*0.5f,(bounds[0][1]+bounds[1][1])*0.5f}};
        for (std::size_t axis=0;axis<2;++axis) {
            const auto half=(bounds[1][axis]-bounds[0][axis])*0.5f*campaignPresentation.indicatorInflation;
            bounds[0][axis]=center[axis]-half;bounds[1][axis]=center[axis]+half;
        }
        // One source step publishes its pre-increment phase. Additional
        // display frames reuse that exact phase instead of consuming a step
        // or exposing the next logical update's colour early.
        if(Ui().advancePresentation) {
            displayedIndicatorPhase_=indicatorPhase_;
            indicatorPhase_+=campaignPresentation.indicatorPhaseStep;
        }
        Color color{{128.0f,128.0f,128.0f,(rotationSineRadians(displayedIndicatorPhase_)+1.0f)*64.0f}};
        const auto first=Transform(bounds[0],matrix),last=Transform(bounds[1],matrix);
        const auto dx=(last[0]-first[0])*campaignPresentation.indicatorCorner;
        const auto dy=(last[1]-first[1])*campaignPresentation.indicatorCorner;
        RenderLine(Ui(),true,first[0],first[0]+dx,first[1],color);RenderLine(Ui(),true,last[0]-dx,last[0],first[1],color);
        RenderLine(Ui(),true,first[0],first[0]+dx,last[1],color);RenderLine(Ui(),true,last[0]-dx,last[0]+2.0f,last[1],color);
        RenderLine(Ui(),false,first[1],first[1]+dy,first[0],color);RenderLine(Ui(),false,last[1]-dy,last[1],first[0],color);
        RenderLine(Ui(),false,first[1],first[1]+dy,last[0],color);RenderLine(Ui(),false,last[1]-dy,last[1],last[0],color);
    }
    void InstallLayout();
    SelectCarScreenServices& services_;EventTree& tree_;
    std::vector<std::unique_ptr<CampaignLeaf>> leaves_;std::vector<CampaignGroup> groups_;CampaignLeaf* current_{};
    CampaignBlurb blurb_;FlailerAnimatedBlurb animated_;CampaignCarLogos carLogos_;CampaignTrackLogos trackLogos_;
    tCubic1D zoom_{},alpha_{};tCubic2D pan_{},parallax_{};std::int32_t state_{},zoomIndex_{};
    float indicatorPhase_{},displayedIndicatorPhase_{};Point clipMin_{},clipMax_{};std::unique_ptr<FrontendStillCamera> camera_;
    const EventInfo* detailsEvent_{};std::array<char,513> description_{},bonusText_{};
};

// Layout text is copied verbatim from the final PAL's two authored source
// tables31B3C0/31B490; no geometry or screen template is generated here.
constexpr const char ChampionshipLayout[]="        x               x       x         x   x  x  x  x     x x   x x   x   x  x  x  x x x    x   xx  x   x x x     x       x       x              x                   x                   x           ";
constexpr const char PursuitLayout[]="        x               x       x         x   x x x   x      x x    x  x  x  x   x    x   x x x x  xx x      x   x x x       x          x           x                   x                   x           ";
void tEventTreeView::InstallLayout()
{
    std::array<float,96> positions{};
    const auto* layout=tree_.TreeType()==0?ChampionshipLayout:PursuitLayout;
    const auto size=tree_.TreeType()==0?sizeof(ChampionshipLayout):sizeof(PursuitLayout);
    Require(size==201 && leaves_.size()<=positions.size(),"Campaign source layout extent is invalid");
    for (const auto& leaf:leaves_) {
        auto remaining=static_cast<std::int32_t>(leaf->event->number);
        Require(remaining>0 && static_cast<std::size_t>(remaining)<=positions.size(),"Campaign event number exceeds source layout");
        const auto destination=static_cast<std::size_t>(remaining-1);
        for (std::size_t index=0;index<201;++index) {
            if (layout[index]!=' ')--remaining;
            if (!remaining) {positions[destination]=static_cast<float>(index%20)*campaignPresentation.horizontalSpacing;break;}
        }
    }
    for (auto& leaf:leaves_)leaf->position={positions[leaf->index],static_cast<float>(leaf->generation)*campaignPresentation.verticalSpacing};
}
std::unique_ptr<tEventTreeView> campaignView;
void CampaignCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(data,"Campaign callback has no actual callback record");
    if (message==ScreenMessage::ScreenActivate) {
        Require(campaignServices && !campaignView,"Campaign activation has no unique service owner");
        const auto& setup=*campaignServices->menus->raceSetup;
        auto* tree=campaignServices->menus->frontend->GetEventTree(*setup.events,*setup.demoDisc);
        Require(tree,"Campaign activation has no canonical event tree");
        campaignView=std::make_unique<tEventTreeView>(*campaignServices,*tree);
    } else if (message==ScreenMessage::ScreenDeactivate)campaignView.reset();
    else if (campaignView) {
        if (message==ScreenMessage::Tick)campaignView->OnTick();
        else if (message==ScreenMessage::RenderModels)campaignView->OnRender();
        else campaignView->HandleMessage(message,*data);
    }
}
}

void BindCampaignScreen(SelectCarScreenServices& services)
{
    Require(!campaignView && (!campaignServices || campaignServices==&services),"Campaign services are already owned by another application");
    Require(services.menus && services.menus->ui && services.menus->ui->resources && services.menus->ui->render &&
        services.menus->ui->clock && services.menus->ui->language && services.menus->screens && services.menus->screens->background &&
        services.menus->frontend && services.menus->raceSetup && services.menus->sounds && services.cameras &&
        services.resources && services.naming && services.markers && services.frame,
        "Campaign services require the actual frontend/UI/camera/render owners");
    // Event resources are published later into this same raceSetup aggregate,
    // just as Select Car borrows it. Validate those owners at activation.
    campaignServices=&services;
}
void UnbindCampaignScreen() {campaignView.reset();campaignServices=nullptr;}
FlailerScreenConfig& CampaignScreenConfiguration()
{
    static FlailerScreenConfig configuration{"View Event Tree",&CampaignCallback,
        {static_cast<std::int32_t>(CampaignScreen),0,0,0},{-1,-1,-1,-1},1,nullptr};
    return configuration;
}
} // namespace hp2

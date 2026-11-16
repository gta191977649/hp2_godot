#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "subtitle_menu_row.hpp"
#include "Fred.hpp"
#include "Language.hpp"
#include "NativeAudioProfileValidation.hpp"
#include "CarLoader.hpp"
#include "Timer.hpp"
#include "Flags.hpp"
#include <tuple>
#include <optional>
#include <cmath>
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

namespace hp2 {

FlailerSlideOnScreen::FlailerSlideOnScreen(FlailerUiState& state,std::uint32_t screen,
    std::uint32_t anchor,std::uint32_t group,std::uint32_t visibilityScreen)
    : ui_(state),screen_(screen),anchor_(anchor),group_(group),visibilityScreen_(visibilityScreen)
{
    Require(ui_.resources && ui_.render && ui_.clock,"Options slide has no actual UI resource/render/clock owners");
    position.duration=position.threshold=1.0f;
    position.continuous=1;
    if (const auto* object=ui_.resources->FindObject(screen_,anchor_))
        ui_.resources->GetTopLeft(*object,savedX_,savedY_,ui_.language);
    Appear();
}

FlailerSlideOnScreen::~FlailerSlideOnScreen()
{
    Disappear();
}

void FlailerSlideOnScreen::Tick()
{
    tCubic1DUpdate(&position,ui_.clock->realTimeElapsed,0.0f,0.0f);
    if (ui_.resources->FindObject(screen_,anchor_))
        ui_.resources->MoveGroupTo(screen_,group_,anchor_,
            static_cast<float>(convertFloatToInt32(position.value)),
            static_cast<float>(convertFloatToInt32(savedY_)));
}

void FlailerSlideOnScreen::Disappear()
{
    ui_.resources->MoveGroupTo(screen_,group_,anchor_,
        static_cast<float>(convertFloatToInt32(savedX_)),
        static_cast<float>(convertFloatToInt32(savedY_)));
    ui_.render->SetScreenVisibility(*ui_.resources,visibilityScreen_,1);
}

void FlailerSlideOnScreen::Appear()
{
    position.desiredValue=ui_.animationSettings.screenSlideStartPosition;
    position.duration=0.5f;position.continuous=0;
    if (position.value!=position.desiredValue)position.mode=2;
    position.mode=0;position.value=position.desiredValue;
    position.derivative=position.desiredDerivative;
    position.desiredValue=savedX_;
    if (position.value!=position.desiredValue)position.mode=2;
    ui_.render->SetScreenVisibility(*ui_.resources,visibilityScreen_,0);
    Tick();
}


namespace {
constexpr std::uint32_t OptionsGroups[]={0x6e34e350u,0x708a0bf1u,0x72df3492u,0x75345d33u,
    0x778985d4u,0x79deae75u,0x7c33d716u,0x7e88ffb7u};
MainMenuScreenServices* optionsServices{};
std::unique_ptr<OptionsScreenBase> optionsOwner;
void ClearLanguageOptionsOwner();

float OptionsLeft(FlailerUiState& ui,std::uint32_t screen)
{
    auto* object=ui.resources->FindObject(screen,OptionsGroups[0]);
    Require(object,"Options slide has no authored first menu group");
    float x,y;ui.resources->GetTopLeft(*object,x,y,ui.language);return x;
}

// Full original DifferentData1EE688. This is comparison of actual values,
// never a native object's padding, console pointer image or second owner.
bool DifferentData(const void* first,const void* second,std::int32_t count)
{
    auto* a=static_cast<const std::uint8_t*>(first);
    auto* b=static_cast<const std::uint8_t*>(second);
    for (;count>0;--count,++a,++b)if (*a!=*b)return true;
    return false;
}

bool DifferentPlayerSettings(const PlayerSettings& a,const PlayerSettings& b)
{
    // The original snapshot compares its complete 48-byte authored settings
    // record. Native field arrangement differs: compare every logical field
    // and all twelve name bytes, deliberately excluding native padding.
    return std::tie(a.inputProfile,a.authoredOption,a.initialVehicleMode,a.gearInputMode,
        a.cameraView,a.alternateCamera,a.splitTimeMode,a.authoredTimeDisplayOption,
        a.cameraSpring,a.lookback,a.hudFlags,a.bestTotalTimeTicks,a.bestLapTimeTicks,
        a.hudHotkey,a.scoring)!=
        std::tie(b.inputProfile,b.authoredOption,b.initialVehicleMode,b.gearInputMode,
        b.cameraView,b.alternateCamera,b.splitTimeMode,b.authoredTimeDisplayOption,
        b.cameraSpring,b.lookback,b.hudFlags,b.bestTotalTimeTicks,b.bestLapTimeTicks,
        b.hudHotkey,b.scoring) || DifferentData(a.playerName,b.playerName,12);
}
}

struct DisplayOptionsChangeManager::Storage {
    std::array<std::int32_t,2> calibration{};
    GlobalDisplaySettings display{};
    std::array<PlayerSettings,2> players{};
};
DisplayOptionsChangeManager::DisplayOptionsChangeManager():storage_(std::make_unique<Storage>()) {}
DisplayOptionsChangeManager::~DisplayOptionsChangeManager()=default;
DisplayOptionsChangeManager gDisplayOptionsChangeManager;

void DisplayOptionsChangeManager::Start(const FrontendPlayerManager& frontend)
{
    // PAL1EE6C0 snapshots FRED+144 calibration, +14C display and both real
    // profile settings. This source-owned snapshot is intentionally separate
    // from the live settings; it neither applies nor fabricates a change.
    Require(frontend.players[0] && frontend.players[1],"Display change snapshot requires both actual profiles");
    storage_->calibration=frontend.displayCalibration;
    storage_->display=frontend.displaySettings;
    storage_->players[0]=frontend.players[0]->settings;
    storage_->players[1]=frontend.players[1]->settings;
}

bool DisplayOptionsChangeManager::Different(const FrontendPlayerManager& frontend) const
{
    Require(frontend.players[0] && frontend.players[1],"Display change comparison requires both actual profiles");
    if (DifferentData(storage_->calibration.data(),frontend.displayCalibration.data(),8))return true;
    static_assert(sizeof(GlobalDisplaySettings)==8,"All eight original display choices must remain present");
    if (DifferentData(&storage_->display,&frontend.displaySettings,8))return true;
    if (DifferentPlayerSettings(storage_->players[0],frontend.players[0]->settings))return true;
    return DifferentPlayerSettings(storage_->players[1],frontend.players[1]->settings);
}

OptionsMenuState::OptionsMenuState(MainMenuScreenServices& bindings,std::uint32_t key)
    : services(bindings),screen(key),slide(*bindings.ui,key,OptionsLeft(*bindings.ui,key)),
      highlight(*bindings.ui,key,0xab671c9au,bindings.ui->animationSettings.highlightColor),
      selectionChange(*bindings.ui,key,0x1042eac3u,-1),selection(std::make_unique<FlailerSelectionSet>()) {}

MainMenuScreenServices& OptionsScreenBase::BoundServices()
{
    Require(optionsServices,"Options screen has no canonical service bindings");
    return *optionsServices;
}
void OptionsScreenBase::Install(std::unique_ptr<OptionsScreenBase> instance)
{
    Require(instance && !optionsOwner,"Options screen duplicates the original live instance");
    optionsOwner=std::move(instance);
}

OptionsScreenBase::OptionsScreenBase(MainMenuScreenServices& services,std::uint32_t screen)
    : state_(std::make_unique<OptionsMenuState>(services,screen))
{
    // PAL1F4C98; the eight groups are read from final2BA430.
    state_->selection->showOtherItems=1;
    state_->selection->AddItems(*services.ui->resources,screen,OptionsGroups,8);
    services.screens->StartMovie("MOVIES\\options_movie_new.AVI");
}
OptionsScreenBase::~OptionsScreenBase()=default;

void OptionsScreenBase::HandleHighlight(ScreenMessage,ScreenCallbackData& data,std::uint32_t group)
{
    SetMenuHighlight(data,group);
}

void OptionsScreenBase::SetMenuHighlight(ScreenCallbackData& data,std::uint32_t group,float rowOffset)
{
    if (!group && data.key>=0x52e039f3u && data.key<=0x52e039f9u)
        group=OptionsGroups[data.key-0x52e039f3u];
    state_->selection->SetCurrentItem(group);
    PositionHighlightBars(group,rowOffset);
    state_->highlight.SetCurrentMenuItem(data.key);
}

void OptionsScreenBase::PositionHighlightBars(std::uint32_t group,float rowOffset)
{
    PositionHighlightBars(*state_->services.ui,state_->screen,group,rowOffset);
}

void OptionsScreenBase::PositionHighlightBars(FlailerUiState& ui,std::uint32_t screen,
    std::uint32_t group,float rowOffset)
{
    std::int32_t left,top,right,bottom;
    Require(ui.resources->GetBoundingBox(screen,group,left,top,right,bottom,ui.language),
        "Options highlight has no authored bounding rectangle");
    // The Options lower rail is at bottom, unlike the MainMenu/Cops -3.
    if (auto* lower=ui.resources->FindObject(screen,0x32c91daeu))
        lower->SetTopLeftY(static_cast<float>(bottom)+rowOffset);
    if (auto* upper=ui.resources->FindObject(screen,0xc671680cu))
        upper->SetTopLeftY(static_cast<float>(top-6)+rowOffset);
}

void OptionsScreenBase::FirstCrackAtMessage(ScreenMessage message,ScreenCallbackData& data)
{
    const bool publishAfterLayout=message==ScreenMessage::Tick &&
        state_->services.ui->externalPresentation && !state_->exit;
    if (message==ScreenMessage::Highlight)HandleHighlight(message,data,0);
    else if (message==ScreenMessage::Tick) {
        state_->slide.Animate();
        if(!publishAfterLayout){state_->highlight.Animate();state_->selectionChange.Animate();}
    }
    if (state_->exit) {state_->exit->HandleMessages(message,data);return;}
    HandleEvent(message,data);
    if(publishAfterLayout){state_->highlight.Animate();state_->selectionChange.Animate();}
}

void OptionsScreenBase::BaseCallback(ScreenMessage message,ScreenCallbackData* data)
{
    // PAL1F5060's one actual OptionsScreenBase::pInstance owner, shared by
    // Options family implementations when they are implemented and bound.
    if (message==ScreenMessage::ScreenDeactivate) {optionsOwner.reset();return;}
    Require(optionsOwner && data,"Options callback arrived without its actual owner/data");
    optionsOwner->FirstCrackAtMessage(message,*data);
}

// Port-side entry in the original Options ring, immediately after Display.
class OptionsScreen::PortRow final {
public:
    static constexpr std::uint32_t Key=0x52e039fau;
    static constexpr std::uint32_t Mask=OptionsGroups[7];

    PortRow(MainMenuScreenServices& services,std::uint32_t screen)
        : art_(*services.ui->resources->FindScreen(screen)),
          page_(*services.screens->active->FindFlailerScreen(static_cast<std::int32_t>(screen))),
          menu_(*page_.menu),item_(&menu_,Key,8)
    {
        auto& resources=*services.ui->resources;
        auto* display=menu_.FindItem(0x52e039f5u);
        auto* displayMask=resources.FindObject(screen,OptionsGroups[2],2);
        Require(display && display->object && displayMask && !menu_.FindItem(Key),
            "Options extension needs the authored Display row");
        text_=static_cast<const FlailerTextChunk&>(*display->object);
        text_.key=Key;
        text_.strip=nullptr;
        text_.parentKey=hashString("MENUITEM_8_GROUP");
        text_.name={};
        const char name[]="MENUITEM_8";
        std::copy_n(name,sizeof(name),text_.name.begin());
        text_.textCapacity=64;
        text_.ownedText.assign(65,0);
        text_.text=text_.ownedText.data();
        text_.SetString(services.optionsExtension->label);
        auto* mask=resources.FindObject(screen,Mask,2);
        if (!mask) {
            mask_.emplace(static_cast<const FlailerBitmapChunk&>(*displayMask));
            mask_->key=Mask;
            mask_->strip=nullptr;
            mask_->parentKey=text_.parentKey;
            mask_->name={};
            Append(*mask_);
            ++art_.bitmapCount;
            mask=&*mask_;
        } else {
            Save(*mask);
        }
        if(text_.clipObjectKey==displayMask->key)text_.clipObjectKey=Mask;
        text_.SetTopLeftY(mask->topLeftY + display->object->topLeftY - displayMask->topLeftY);
        Append(text_);
        ++art_.textCount;
        item_.object=&text_;
        item_.ResetSavedParameters();
        item_.previous=display;
        item_.next=display->next;
        display->next->previous=&item_;
        display->next=&item_;
        Layout(resources,screen);
    }

    ~PortRow()
    {
        if(page_.menu==&menu_) {
            if(menu_.current==&item_)menu_.current=item_.previous;
            item_.previous->next=item_.next;
            item_.next->previous=item_.previous;
        }
        for(const auto& [object,y]:positions_)object->SetTopLeftY(y);
        for(const auto& [mask,scale]:maskScales_)mask->SetScaleY(scale);
        for(const auto& [object,visible]:visibility_)object->visible=visible;
        Remove(text_);
        --art_.textCount;
        if(mask_) {Remove(*mask_);--art_.bitmapCount;}
    }
private:
    void Layout(FlailerResources& resources,std::uint32_t screen)
    {
        std::vector<std::pair<FlailerObjectChunk*,FlailerBitmapChunk*>> rows;
        for(auto* item=menu_.root.next;item!=&menu_.root;item=item->next) {
            if (!item->object || item->objectKey<0x52e039f3u || item->objectKey>Key) continue;
            const auto key=item->objectKey==Key ? Mask : OptionsGroups[item->objectKey-0x52e039f3u];
            auto* mask=static_cast<FlailerBitmapChunk*>(resources.FindObject(screen,key,2));
            Require(mask,"Options row has no authored mask");
            if (item->flags&1u) rows.emplace_back(item->object,mask);
            else {
                visibility_.emplace_back(mask,mask->visible);
                mask->visible=0;
                mask->DeleteFlailerStrip();
            }
        }
        Require(rows.size()>1,"Options needs consecutive authored rows");
        const float start=rows.front().second->topLeftY;
        const float height=resources.ScaledBitmapHeight(*rows.front().second);
        const float authoredPitch=rows[1].second->topLeftY-start;
        // The movie ends at y=341 in the authored Options canvas.
        const float pitch=std::min(authoredPitch,std::floor((341.0f-start-height)/(rows.size()-1)));
        for(std::size_t i=0;i<rows.size();++i) {
            const auto& [label,mask]=rows[i];
            const float y=start+pitch*i;
            Save(*label);
            Save(*mask);
            maskScales_.emplace_back(mask,mask->scaleY);
            label->SetTopLeftY(label->topLeftY+y-mask->topLeftY);
            mask->SetTopLeftY(y);
            mask->SetHeight(resources,height);
        }
        for(auto* object=art_.objects.next;object!=&art_.objects;object=object->next) {
            if(object->key!=0xaa5d7d3fu)continue; // Authored fixed gaps precede the new row layout.
            visibility_.emplace_back(object,object->visible);
            object->visible=0;
            object->DeleteFlailerStrip();
        }
    }
    void Save(FlailerObjectChunk& object) {positions_.emplace_back(&object,object.topLeftY);}
    void Append(FlailerObjectChunk& object)
    {
        object.previous=art_.objects.previous;
        object.next=&art_.objects;
        object.previous->next=&object;
        art_.objects.previous=&object;
    }
    static void Remove(FlailerObjectChunk& object)
    {
        object.DeleteFlailerStrip();
        object.previous->next=object.next;
        object.next->previous=object.previous;
    }

    FlailerScreenResource& art_;
    FlailerScreen& page_;
    FlailerMenu& menu_;
    FlailerMenuItem item_;
    FlailerTextChunk text_;
    std::optional<FlailerBitmapChunk> mask_;
    std::vector<std::pair<FlailerObjectChunk*,float>> positions_;
    std::vector<std::pair<FlailerBitmapChunk*,float>> maskScales_;
    std::vector<std::pair<FlailerObjectChunk*,std::uint8_t>> visibility_;
};

OptionsScreen::OptionsScreen(MainMenuScreenServices& services,std::uint32_t screen)
    : OptionsScreenBase(services,screen),
      blurb_(std::make_unique<FlailerAnimatedBlurb>(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu))
{
    if (services.controlsExtension) {
        // The host's combined page replaces the authored Controller entry.
        auto* label=static_cast<FlailerTextChunk*>(services.ui->resources->FindObject(screen,0x52e039f4u,1));
        Require(label && label->text,"Options Controls row needs its authored label");
        originalControlsText_=label->text;
        originalControlsLanguage_=label->languageId;
        label->SetString(services.controlsExtension->label);
        portControlsLabel_=label;
    }
    // PAL1EEB78 initializes only the embedded RumbleTimer word. Its Tick
    // and TriggerSomething callees are final JR RA/NOP and are not emitted.
    rumbleTimer_=0;
    bool hideLanguages=gameTimeGetVideoMode(services.ui->clock)==1;
    if(services.ui->externalPresentation) {
        int available=0;
        for(int language=0;language<6;++language)
            if(services.ui->language->CanSelect(language))++available;
        hideLanguages=available<2;
    }
    if (hideLanguages) {
        // Complete relevant constant-state2 path of actual16D198: find the
        // registered screen/menu/item, clear enabled bit1 and hide its object.
        // No replacement menu or generic callback is created for this call.
        if (auto* node=services.screens->FindScreen(static_cast<std::int32_t>(screen)))
            if (auto* sourceScreen=node->FindFlailerScreen(static_cast<std::int32_t>(screen)))
                if (sourceScreen->menu)
                    if (auto* item=sourceScreen->menu->FindItem(0x52e039f9u)) {
                        item->ClearMenuItemState(1);
                        services.ui->resources->SetObjectVisibility(screen,0x52e039f9u,0);
                    }
    }
    if(services.optionsExtension) {
        portRow_=std::make_unique<PortRow>(services,screen);
        if(!state_->selection->items[7])
            state_->selection->AddItem(services.ui->resources->FindObject(screen,PortRow::Mask));
    }
}
OptionsScreen::~OptionsScreen()
{
    if (!portControlsLabel_) return;
    portControlsLabel_->SetString(originalControlsText_.c_str());
    portControlsLabel_->SetLanguageID(originalControlsLanguage_);
}

void OptionsScreen::HandleHighlight(ScreenMessage message,ScreenCallbackData& data,std::uint32_t group)
{
    OptionsScreenBase::HandleHighlight(message,data,portRow_ && data.key==PortRow::Key?PortRow::Mask:group);
}

void OptionsScreen::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    auto& state=*state_;
    if (message==ScreenMessage::Select) {
        const char* destination=""; // actual PAL331660 default for unknown key
        switch (data.key) {
        case 0x52e039f3u:destination="Audio Options";break;
        case 0x52e039f4u:destination=state.services.controlsExtension
            ?state.services.controlsExtension->screen->name:"Joystick PreConfig";break;
        case 0x52e039f5u:
            destination="Display Options";
            gDisplayOptionsChangeManager.Start(*state.services.frontend);
            break;
        case PortRow::Key:
            if(portRow_)destination=state.services.optionsExtension->screen->name;
            break;
        case 0x52e039f6u:state.services.screens->ChangeToScreen("Phils Load Save",true);return;
        case 0x52e039f7u:destination="NameEntry";break;
        // Final jump table2B9BD0 selects Credits for F8, Language for F9;
        // physical ordering of their string/basic blocks is different.
        case 0x52e039f8u:state.services.screens->ChangeToScreen("Credits",true);return;
        case 0x52e039f9u:state.services.screens->ChangeToScreen("Language Options",true);return;
        default:break;
        }
        Require(!state.exit,"Options already owns an exit animation");
        auto& ui=*state.services.ui;
        state.exit=std::make_unique<FlailerExitScreenAnimation>(ui,*state.services.screens,
            state.screen,destination,true);
        auto* anchor=ui.resources->FindObject(state.screen,0x32c91daeu);
        Require(anchor,"Options exit has no authored lower rail anchor");
        float x,y;ui.resources->GetTopLeft(*anchor,x,y,ui.language);
        state.exit->AddGroup(0x97bf4b25u,0x32c91daeu,x,650.0f,state.exit->MaximumDuration());
        return;
    }
    if (message==ScreenMessage::Highlight) {
        std::int32_t language=0;
        if (data.key>=0x52e039f3u && data.key<=0x52e039f9u)
            language=100020+static_cast<std::int32_t>(data.key-0x52e039f3u);
        if(portRow_ && data.key==PortRow::Key)blurb_->NewText(state.services.optionsExtension->description);
        else blurb_->NewText(language);
        // Original Highlight falls through to the same Animate operation.
    } else if (message!=ScreenMessage::Tick)return;
    blurb_->Animate();
    // Rumbleator.Tick PAL1EEBA8 is a proven no-op, not a sound/input stub.
}

void OptionsScreen::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    if (message==ScreenMessage::ScreenActivate) {
        Require(optionsServices && data && !optionsOwner,
            "Options activation lacks actual services/data or duplicates a live owner");
        optionsOwner=std::make_unique<OptionsScreen>(*optionsServices,data->key);
    } else OptionsScreenBase::BaseCallback(message,data);
}

void BindOptionsScreen(MainMenuScreenServices& services)
{
    Require(!optionsOwner && services.ui && services.screens && services.frontend,
        "Options services cannot replace a live screen or omit original owners");
    optionsServices=&services;
}
void UnbindOptionsScreen()
{
    ClearLanguageOptionsOwner();optionsOwner.reset();optionsServices=nullptr;
}
namespace {
constexpr std::int32_t OptionsHelpItems[]={800057,800058,800059,0};
}
FlailerScreenConfig fOptions={
    "Options",&OptionsScreen::Callback,
    {{static_cast<std::int32_t>(0x95ad096bu),0,0,0}},{{-1,-1,-1,-1}},1,OptionsHelpItems};

} // namespace hp2

#include "bsound/include/bJukebox.h"
#include "bsound/include/bSound.h"
#include "CarSounds.hpp"
#include <cstring>
namespace hp2 {
namespace {
// Full original GenreMgr::Populate1EF240. Genre rows are the screen's
// bounded derived index; the actual playlist remains in FRED and cJukebox.
class GenreMgr {
public:
    struct Genre {
        std::array<char,20> name{};
        std::int32_t count{};
        std::array<std::uint8_t,20> rows{};
    };
    std::array<Genre,6> genres{};
    std::int32_t count{};
    void Populate(const CarSoundBindings& sound,FrontendMusicSettings& settings)
    {
        genres={};count=0;
        Require(settings.count>=0 && static_cast<std::size_t>(settings.count)<=settings.rows.size(),
            "Music row count exceeds the original settings table");
        for(std::int32_t index=0;index<settings.count;++index) {
            auto& row=settings.rows[static_cast<std::size_t>(index)];
            const auto* name=soundJukeboxGetTrackGenre(sound,row.group,row.track);
            if(!name || !*name)continue;
            std::int32_t genre=0;
            for(;genre<count;++genre)
                if(std::strcmp(name,genres[static_cast<std::size_t>(genre)].name.data())==0)break;
            if(genre==count) {
                Require(static_cast<std::size_t>(count)<genres.size(),
                    "Final jukebox exceeds the original six genres");
                auto& destination=genres[static_cast<std::size_t>(genre)].name;
                const auto bytes=std::strlen(name);
                Require(bytes<destination.size(),"Final jukebox genre exceeds its original name slot");
                std::memcpy(destination.data(),name,bytes+1);++count;
            }
            auto& group=genres[static_cast<std::size_t>(genre)];
            Require(static_cast<std::size_t>(group.count)<group.rows.size(),
                "Final jukebox genre exceeds its original track-index slots");
            row.genreBit=static_cast<std::int8_t>(genre);
            group.rows[static_cast<std::size_t>(group.count++)]=static_cast<std::uint8_t>(index);
        }
    }
};
}

void FrontendPlayerManager::CaptureAudioOptions(const CarSoundBindings& sound,bool rebuildMusicList)
{
    Require(sound.audio && sound.audio->library,"Audio options have no actual native mixer");
    constexpr std::array<std::uint32_t,7> categories{{5,4,0,1,2,3,6}};
    for(std::size_t i=0;i<categories.size();++i)
        musicSettings.savedVolumes[i]=sound.audio->library->GetCategoryVolume(categories[i]);
    musicSettings.savedAudioMode=worldAudioGetMode();
    if(!rebuildMusicList)return;
    // PAL1EEF60 iterates the actual loaded cartridge's groups/tracks and
    // retains at most32 rows. PAL1EF240 assigns genre bits in first-seen
    // string order; strings come from the same final jukebox metadata cache.
    musicSettings.count=0;
    for(std::size_t group=0;group<soundJukeboxPlayer.groups.size();++group) {
        const auto& row=soundJukeboxPlayer.groups[group];
        for(std::size_t track=0;track<row.tunes.size();++track) {
            if(musicSettings.count>=static_cast<std::int32_t>(musicSettings.rows.size()))continue;
            Require(group<=127 && track<=127,"Jukebox indices exceed original signed row lanes");
            auto& output=musicSettings.rows[static_cast<std::size_t>(musicSettings.count++)];
            output.group=static_cast<std::int8_t>(group);output.track=static_cast<std::int8_t>(track);
            output.status=static_cast<std::int8_t>(soundJukeboxGetTrackStatus(sound,static_cast<std::int32_t>(group),static_cast<std::int32_t>(track)));

        }
    }
    // PAL DataAudioOptionsScreen::InitWithDefaults1EF1C8 calls the
    // actual capture, initializes A0/A4/A8/AE/AF, then populates GenreMgr.
    musicSettings.menuState={{0,5,0}};musicSettings.menuMode=0;musicSettings.enabledGenres=255;
    GenreMgr genres;genres.Populate(sound,musicSettings);
}

void FrontendPlayerManager::ApplyAudioOptions(const CarSoundBindings& bindings,
    const RaceParameters& parameters,FlailerSoundState& uiSound) const
{
    Require(bindings.audio && bindings.audio->library && uiSound.audio==bindings.audio &&
        uiSound.manager==&bindings.audio->library->Mixer() && uiSound.volumes &&
        bindings.volumes==uiSound.volumes,"Saved audio options have no shared canonical mixer and speech-volume owner");
    Require(nativeAudioProfileValid(musicSettings,applicationLaunchOptions.musicEnabled?&soundJukeboxPlayer:nullptr),
        "Saved audio options must pass validation before changing the live mixer");
    Require(!applicationLaunchOptions.musicEnabled || soundJukeboxPlayer.audio==bindings.audio,
        "Saved playlist has no shared canonical jukebox owner");
    // Complete final SetSoundFromAudioOptions1EF120. Category order is
    // source5,4,0,6,2,3,1, followed by speech-volume save, playlist and mode.
    constexpr std::array<std::uint32_t,7> categories{{5,4,0,6,2,3,1}};
    constexpr std::array<std::size_t,7> savedIndices{{0,1,2,6,4,5,3}};
    for(std::size_t i=0;i<categories.size();++i)
        Require(bindings.audio->library->SetCategoryVolume(categories[i],musicSettings.savedVolumes[savedIndices[i]])==WORLD_AUDIO_OK,
            "Physical mixer rejected a saved category volume");
    SaveVolumesForSpeech(uiSound);
    Require(soundJukeboxSetSelection(bindings,*this,parameters,5)==WORLD_AUDIO_OK,
        "Actual jukebox rejected the saved track and genre choices");
    Require(worldAudioSetMode(bindings.audio,musicSettings.savedAudioMode)==WORLD_AUDIO_OK,
        "Physical mixer rejected the saved audio mode");
}

} // namespace hp2

#include "Language.hpp"
namespace hp2 {
namespace {
// The unshipped bulk-language debug command is intentionally not exposed
// by the native PC frontend; ordinary language selection stays unchanged.
class LanguageOptionsScreenBase final {
public:
    LanguageOptionsScreenBase(MainMenuScreenServices& services,std::uint32_t screen,bool preFrontend)
        :services_(services),screen_(screen),preFrontend_(preFrontend)
    {
        Require(services.ui && services.ui->language && services.screens && services.frontend && services.raceSetup,
            "Language options lack their canonical runtime owners");
        selected_=std::max(services.ui->language->currentLanguage,0);
        if(!preFrontend)slide_=std::make_unique<FlailerSlideOnScreen>(*services.ui,screen,
            0x03c790bbu,0x718cd8c3u,0x95ad096bu);
        // FRONTB's seventh choice is "Load All Languages", an authored
        // debug-only operation. There is no normal PAL gameplay consumer for
        // bulk loading; leaving it selectable causes an invalid screen exit.
        // Keep ordinary six language choices independent of this debug item.
        // This authored bulk-load action is not a released gameplay option.
        // Clearing the enabled bit (rather than setting the highlight bit)
        // actually prevents navigation to the invalid debug-only callback.
        if(auto* node=services.screens->active) {
            for(auto* current=node->screenRoot.next;current!=&node->screenRoot;current=current->next)
                if(current->menu) {
                    if(auto* item=current->menu->FindItem(0x52e039f9u))
                        item->ClearMenuItemState(1);
                    if(services.ui->externalPresentation) {
                        for(int language=0;language<6;++language) {
                            if(services.ui->language->CanSelect(language))continue;
                            const auto key=0x52e039f3u+language;
                            if(auto* item=current->menu->FindItem(key))item->ClearMenuItemState(1);
                            services.ui->resources->SetObjectVisibility(screen,key,0);
                        }
                    }
                }
        }
        services.ui->resources->SetObjectVisibility(screen,0x52e039f9u,0);
    }
    void HandleScreenCallback(ScreenMessage message,ScreenCallbackData& data) {
        if(message==ScreenMessage::Tick){if(slide_)slide_->Tick();return;}
        if(message==ScreenMessage::LeaveScreen) {
            data.value=0;
            if(services_.ui->language->currentLanguage==-1)return;
            auto* active=services_.screens->active;
            const auto choice=active?active->GetActiveMenuItem():0;
            Close(choice,data.joystickPort);
            return;
        }
        if(message!=ScreenMessage::Select)return;
        if(data.key>=0x52e039f3u && data.key<=0x52e039f8u) {
            if(services_.ui->externalPresentation && !services_.ui->language->CanSelect(
                static_cast<std::int32_t>(data.key-0x52e039f3u)))return;
            Close(data.key,data.joystickPort);
            return;
        }
        if(data.key==0x52e039f9u) {
            // A synthetic selection event must not reactivate the debug-only
            // action even when native developer diagnostics were enabled.
            data.sound=0;
            return;
        }
    }
private:
    void Close(std::uint32_t key,std::int32_t port) {
        const auto previous=services_.ui->language->currentLanguage;
        if(key>=0x52e039f3u && key<=0x52e039f8u) {
            selected_=static_cast<std::int32_t>(key-0x52e039f3u);
            if(!services_.ui->externalPresentation || services_.ui->language->CanSelect(selected_))CommitLanguage();
        }
        const bool autosave=!preFrontend_ && services_.ui->externalPresentation && services_.frontend->autoSave &&
            services_.ui->language->currentLanguage!=previous;
        auto* screens=services_.screens;
        Require(screens->MoveToPreviousScreen(1,false,port),"Language options have no preceding original screen");
        if(autosave)(void)RequestProfileAutosave([] {});
    }

    void CommitLanguage() {
        services_.ui->language->SetLanguage(selected_);
        if(preFrontend_ && services_.ui->language->currentLanguage!=-1) {
            services_.frontend->InitWithDefaults(*services_.raceSetup);
            if(services_.sounds)services_.frontend->CaptureAudioOptions(*services_.sounds,true);
        }
    }
    MainMenuScreenServices& services_;
    std::uint32_t screen_{};
    bool preFrontend_{};
    std::int32_t selected_{};
    std::unique_ptr<FlailerSlideOnScreen> slide_;
};
std::unique_ptr<LanguageOptionsScreenBase> languageOptionsOwner;
void ClearLanguageOptionsOwner(){languageOptionsOwner.reset();}
constexpr std::array<std::int32_t,4> LanguageHelp{{0xc354c,0xc354d,0xc354e,0}};
}
void LanguageOptionsScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(optionsServices && data,"Language callback lacks the actual options binding/payload");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!languageOptionsOwner,"Language options received duplicate activation");
        auto* node=optionsServices->screens->active;
        Require(node && node->configuration && node->configuration->name,"Language screen lacks its original configuration");
        const bool pre=!std::strcmp(node->configuration->name,"Language Options Pre FE");
        languageOptionsOwner=std::make_unique<LanguageOptionsScreenBase>(*optionsServices,data->key,pre);
        if(optionsServices->ui->externalPresentation)
            data->value=std::max(optionsServices->ui->language->currentLanguage,0)+1;
    } else if(message==ScreenMessage::ScreenDeactivate)languageOptionsOwner.reset();
    else {
        Require(languageOptionsOwner!=nullptr,"Language message arrived outside its actual screen lifetime");
        languageOptionsOwner->HandleScreenCallback(message,*data);
    }
}
std::array<FlailerScreenConfig,2>& LanguageScreenConfigurations()
{
    // Exact final1F5738..1F578C constructor arguments, source names and shared
    // callback. Both configurations borrow the one final authored screen.
    static std::array<FlailerScreenConfig,2> configurations{{
        {"Language Options",LanguageOptionsScreenCallback,{{static_cast<std::int32_t>(0x632b068eu),0,0,0}},{{-1,-1,-1,-1}},1,LanguageHelp.data()},
        {"Language Options Pre FE",LanguageOptionsScreenCallback,{{static_cast<std::int32_t>(0x632b068eu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr}
    }};
    return configurations;
}
} // namespace hp2

#include "DemoDisc.hpp"
#include "GameFlow.hpp"
namespace hp2 {
namespace {
constexpr std::uint32_t AudioChannels63[7]={5,4,0,1,2,3,6};
constexpr std::uint32_t MenuFirst63=0x52e039f3u;

bool SameAudioOptions63(const FrontendMusicSettings& first,const FrontendMusicSettings& second)
{
    if(first.savedVolumes!=second.savedVolumes || first.savedAudioMode!=second.savedAudioMode ||
       first.count!=second.count || first.enabledGenres!=second.enabledGenres ||
       first.menuState!=second.menuState || first.menuMode!=second.menuMode)return false;
    for(std::size_t index=0;index<first.rows.size();++index) {
        const auto& a=first.rows[index];const auto& b=second.rows[index];
        if(std::tie(a.group,a.track,a.status,a.genreBit)!=std::tie(b.group,b.track,b.status,b.genreBit))return false;
    }
    return true;
}

class AudioOptionsScreen final:public OptionsScreenBase {
    FrontendMusicSettings editing_;
    std::array<std::unique_ptr<FlailerSlider>,7> sliders_;
    FlailerScrollingMenuList scrolling_;
    FlailerLeftRightArrowScroll arrows_;
    FlailerAnimatedBlurb blurb_;
public:
    AudioOptionsScreen(MainMenuScreenServices& services,std::uint32_t screen)
        :OptionsScreenBase(services,screen),editing_(services.frontend->musicSettings),
         scrolling_(*services.ui,screen,MenuFirst63,MenuFirst63+6,0xedf2e4eeu,0x20d73d01u,0xc67cfb4au),
         arrows_(*services.ui,screen,0xd1cda594u,0x9345e4a7u),
         blurb_(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        Require(services.sounds && services.sounds->audio && services.sounds->audio->library,
            "Audio options require the original live mixer");
        auto& mixer=*services.sounds->audio->library;
        // PAL1EF3C8 copies FRED data, then reads the seven actual categories.
        for(std::size_t index=0;index<7;++index) {
            editing_.savedVolumes[index]=mixer.GetCategoryVolume(AudioChannels63[index]);
            sliders_[index]=std::make_unique<FlailerSlider>(*services.ui,screen,
                MenuFirst63+static_cast<std::uint32_t>(index),0xa29f7a16u+static_cast<std::uint32_t>(index),
                0xc866642bu+0x441u*static_cast<std::uint32_t>(index),
                static_cast<std::int32_t>(AudioChannels63[index]),&editing_.savedVolumes[index],0.0f,1.0f);
        }
        editing_.savedAudioMode=worldAudioGetMode();RedrawMode();
        constexpr std::uint32_t objects[]={0x52e039f3u,0x52e039f4u,0x52e039f5u,0x52e039f6u,
            0x52e039f7u,0x52e039f8u,0x52e039f9u,0x52e039fau,0x52e039fbu,0xaee77883u,0xaee77884u,
            0xa29f7a16u,0xc866642bu,0xa29f7a17u,0xc866686cu,0xa29f7a18u,0xc8666cadu,
            0xa29f7a19u,0xc86670eeu,0xa29f7a1au,0xc866752fu,0xa29f7a1bu,0xc8667970u,
            0xa29f7a1cu,0xc8667db1u,0x2932dfcau};
        scrolling_.AddObjects(objects,std::size(objects));scrolling_.SetItemCount(11);
    }
    void HandleHighlight(ScreenMessage,ScreenCallbackData&,std::uint32_t) override {}
    void RedrawMode()
    {
        constexpr std::uint32_t labels[]={0x1adb9u,0x1adb8u,0x1adbbu};
        Require(editing_.savedAudioMode>=0 && editing_.savedAudioMode<3,"Audio mode is outside its three source choices");
        state_->services.ui->resources->SetLanguageID(state_->screen,0x2932dfcau,labels[editing_.savedAudioMode]);
    }
    void Commit()
    {
        state_->services.frontend->musicSettings=editing_;
        auto& services=state_->services;
        Require(services.raceSetup && services.raceSetup->parameters && services.ui->sound,
            "Audio commit has no canonical race or speech-volume settings");
        services.frontend->ApplyAudioOptions(*services.sounds,*services.raceSetup->parameters,*services.ui->sound);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        auto& state=*state_;auto& services=state.services;auto& ui=*services.ui;
        scrolling_.HandleEvent(message,data);arrows_.HandleEvent(message,data);
        if(message==ScreenMessage::LeaveScreen) {
            if(!SameAudioOptions63(editing_,services.frontend->musicSettings)) {
                Commit();if(services.frontend->autoSave)services.screens->stack.Push("Autosave");
            }
        } else if(message==ScreenMessage::ScreenBack) data.sound=0;
        else if(message==ScreenMessage::Previous && data.key==MenuFirst63)data.value=1;
        else if(message==ScreenMessage::Next && data.key==0xaee77884u)data.value=1;
        else if(message==ScreenMessage::Highlight) {
            std::int32_t label=0;
            if(data.key>=MenuFirst63 && data.key<=MenuFirst63+8)label=0x24be4+static_cast<std::int32_t>(data.key-MenuFirst63);
            else if(data.key==0xaee77883u)label=0x24bed;
            else if(data.key==0xaee77884u)label=0x24bee;
            blurb_.NewText(label);
            SetMenuHighlight(data,scrolling_.GetCurrentMenuMask(),scrolling_.CurrentMenuOffsetY());
            arrows_.SetCurrentItem(data.key==0x52e039fau?0x2932dfcau:0);
        } else if(message==ScreenMessage::Left || message==ScreenMessage::Right) {
            if(data.key==0x52e039fau) {
                const auto current=worldAudioGetMode();
                Require(current>=0 && current<3,"Current source audio mode is outside its three choices");
                const auto requested=(current+(message==ScreenMessage::Left?2:1))%3;
                Require(worldAudioSetMode(services.sounds->audio,requested)==WORLD_AUDIO_OK,
                    "Audio mode change could not reach its canonical mixer");
                editing_.savedAudioMode=worldAudioGetMode();
                RedrawMode();data.sound=message==ScreenMessage::Left?1:2;
            } else if(data.key>=MenuFirst63 && data.key<=MenuFirst63+6)
                sliders_[data.key-MenuFirst63]->StepValue(message==ScreenMessage::Left?-0.05000000074505806f:0.05000000074505806f,true);
        } else if(message==ScreenMessage::Select) {
            if(data.key==0x52e039fbu) {
                Commit();state.exit=std::make_unique<FlailerExitScreenAnimation>(ui,*services.screens,state.screen,"JukeBox Music",true);
                float x{},y{};const auto* anchor=ui.resources->FindObject(state.screen,0x32c91daeu);
                Require(anchor,"Audio exit is missing its authored rail");ui.resources->GetTopLeft(*anchor,x,y,ui.language);
                state.exit->AddGroup(0x97bf4b25u,0x32c91daeu,x,650.0f,state.exit->MaximumDuration());
            } else if(data.key==0xaee77883u) {
                // PAL ResetVolumes1AFAB8 uses actual demo policy, then the
                // canonical defaults and saves the same live speech values.
                Require(ui.sound && ui.sound->volumes,"Volume reset has no actual default settings");
                const auto* demo=services.demoMode?services.demoMode->demoDisc:nullptr;
                const float scale=demo && demo->ActivationWord()?static_cast<float>(demo->Parameters().masterVolumeScale)*0.10000000149011612f:1.0f;
                auto& mixer=*services.sounds->audio->library;
                for(std::int32_t channel=0;channel<7;++channel)
                    Require(mixer.SetCategoryVolume(static_cast<std::uint32_t>(channel),
                        GetVolumeDefault(*ui.sound->volumes,channel)*scale)==WORLD_AUDIO_OK,"Volume reset rejected by mixer");
                SaveVolumesForSpeech(*ui.sound);
                for(std::size_t index=0;index<7;++index) {
                    editing_.savedVolumes[index]=mixer.GetCategoryVolume(AudioChannels63[index]);sliders_[index]->StepValue(0.0f,false);
                }
            } else if(data.key==0xaee77884u) {
                Require(editing_.count>=0 && static_cast<std::size_t>(editing_.count)<=editing_.rows.size(),"Music settings count is invalid");
                for(std::int32_t index=0;index<editing_.count;++index) {
                    auto& row=editing_.rows[static_cast<std::size_t>(index)];
                    row.status=static_cast<std::int8_t>(soundJukeboxGetTrackStatus(*services.sounds,row.group,row.track));
                }
                editing_.menuState={};editing_.menuMode=0;editing_.enabledGenres=255;
            } else data.sound=0;
        } else if(message==ScreenMessage::Tick) {
            if(ui.externalPresentation && state.selection->current)
                PositionHighlightBars(scrolling_.GetCurrentMenuMask(),scrolling_.CurrentMenuOffsetY());
            for(auto& slider:sliders_)slider->Animate();
            blurb_.Animate();
        }
    }
    static void Callback(ScreenMessage message,ScreenCallbackData* data)
    {
        if(message==ScreenMessage::ScreenActivate) {
            Require(data,"Audio activation has no source screen data");
            Install(std::make_unique<AudioOptionsScreen>(BoundServices(),data->key));
        } else BaseCallback(message,data);
    }
};
} // namespace
} // namespace hp2

namespace hp2 {
namespace {
class DisplayOptionsScreen final:public OptionsScreenBase {
    static constexpr std::uint32_t SubtitleRow=0xaee77883u;
    static constexpr auto SubtitleValue=SubtitleMenuRow::ValueKey;
    GlobalDisplaySettings editing_;
    std::unique_ptr<SubtitleMenuRow> subtitles_;
    FlailerScrollingMenuList scrolling_;
    FlailerLeftRightArrowScroll arrows_;
    FlailerAnimatedBlurb blurb_;
    static constexpr std::array<std::uint32_t,6> Values{{0x55cb09f2u,0x2e931f1bu,
        0x03d90172u,0x5610f2f4u,0x422313cbu,0x8fe4086bu}};
public:
    DisplayOptionsScreen(MainMenuScreenServices& services,std::uint32_t screen)
        :OptionsScreenBase(services,screen),editing_(services.frontend->displaySettings),
         scrolling_(*services.ui,screen,MenuFirst63,MenuFirst63+6,0xedf2e4eeu,0x20d73d01u,0xc67cfb4au),
         arrows_(*services.ui,screen,0xd1cda594u,0x9345e4a7u),
         blurb_(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        for(std::size_t index=0;index<6;++index)Redraw(index);
        constexpr std::uint32_t objects[]={0x52e039f3u,0x52e039f4u,0x52e039f5u,
            0x52e039f6u,0x52e039f7u,0x52e039f8u,0x52e039f9u,0x52e039fau,0x52e039fbu,
            0x55cb09f2u,0x2e931f1bu,0x03d90172u,0x5610f2f4u,0x8fe4086bu,0x422313cbu};
        scrolling_.SetItemCount(9);
        if(services.subtitleSetting) {
            auto& resources=*services.ui->resources;
            auto* zone=resources.FindObject(screen,MenuFirst63+5,1);
            auto* firstHud=resources.FindObject(screen,MenuFirst63+6,1);
            const float spacing=firstHud->topLeftY-zone->topLeftY;
            subtitles_=std::make_unique<SubtitleMenuRow>(
                services,screen,SubtitleRow,MenuFirst63+5,Values.back(),spacing);
            for(std::uint32_t row=6;row<9;++row) {
                auto* object=resources.FindObject(screen,MenuFirst63+row,1);
                subtitles_->ShiftRow(*object,spacing);
            }
            scrolling_.SetVisibleArea(MenuFirst63,SubtitleRow);
            scrolling_.SetLastMaskRow(SubtitleRow);
            scrolling_.AddObject(SubtitleRow);scrolling_.AddObject(SubtitleValue);
            scrolling_.SetItemCount(10);
        }
        scrolling_.AddObjects(objects,std::size(objects));
        // Final1F354C/1F3564 set the same real clipping polygon on both arrows.
        for(const auto key:{0xd1cda594u,0x9345e4a7u}) {
            auto* object=services.ui->resources->FindObject(screen,key);
            if(!object)continue;
            object->SetClipToPoly(0xc67cfb4au);
        }
    }
    void HandleHighlight(ScreenMessage,ScreenCallbackData&,std::uint32_t) override {}
    void Redraw(std::size_t index)
    {
        std::uint32_t language{};
        switch(index) {
        case 0: {
            // Actual final2BA1D0 dispatch, including the empty mode7 label.
            constexpr std::uint32_t labels[]={0x22345u,0x22346u,0x22347u,0x223a8u,0x22348u,
                0x22349u,0x22353u,0,0x223a9u,0x223aau};
            if(static_cast<std::uint32_t>(editing_.singleViewTrackMapMode)<std::size(labels))
                language=labels[editing_.singleViewTrackMapMode];
            break;
        }
        case 1:language=editing_.authoredOption0?0x22352u:0x22353u;break;
        case 2:language=editing_.languageDependentOption==0?0x2234au:
            editing_.languageDependentOption==1?0x2234bu:0;break;
        case 3: {
            constexpr std::uint32_t labels[]={0x22353u,0x18e71u,0x22352u};
            Require(editing_.leaderboardMode>=0 && editing_.leaderboardMode<3,"Leaderboard display mode is invalid");
            language=labels[editing_.leaderboardMode];break;
        }
        case 4:language=editing_.authoredOption6?0x22352u:0x22353u;break;
        case 5:language=editing_.authoredOption7?0x22352u:0x22353u;break;
        default:throw std::runtime_error("Invalid original Display option index");
        }
        state_->services.ui->resources->SetLanguageID(state_->screen,Values[index],language);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        auto& state=*state_;auto& services=state.services;auto& ui=*services.ui;
        scrolling_.HandleEvent(message,data);arrows_.HandleEvent(message,data);
        if(message==ScreenMessage::Left || message==ScreenMessage::Right ||
           (message==ScreenMessage::Select && subtitles_ && data.key==SubtitleRow)) {
            if(subtitles_ && data.key==SubtitleRow) {
                subtitles_->Toggle();
                data.sound=1;
            } else if(data.key>=MenuFirst63 && data.key<MenuFirst63+6) {
                const auto index=data.key-MenuFirst63;
                const auto direction=message==ScreenMessage::Left?-1:1;
                if(index==0) {
                    Require(services.raceSetup && services.raceSetup->parameters && ui.flow,
                        "Track-map option has no canonical race and GameFlow context");
                    globalDisplaySettingsCycleTrackMapMode(&editing_,direction,services.raceSetup->parameters,
                        &ui.flow->currentState,&services.frontend->playerCount);
                    if(editing_.singleViewTrackMapMode==6)editing_.splitViewTrackMapMode=editing_.pursuitTrackMapMode=6;
                    else {
                        editing_.splitViewTrackMapMode=static_cast<std::int8_t>(splitPlayerTrackMapModes[0]);
                        editing_.pursuitTrackMapMode=static_cast<std::int8_t>(pursuitTrackMapModes[0]);
                    }
                } else if(index==1)editing_.authoredOption0=editing_.authoredOption0==0;
                else if(index==2)editing_.languageDependentOption=editing_.languageDependentOption==0;
                else if(index==3)globalDisplaySettingsCycleLeaderboardMode(&editing_,direction);
                else if(index==4)editing_.authoredOption6=editing_.authoredOption6==0;
                else editing_.authoredOption7=editing_.authoredOption7==0;
                Redraw(index);data.sound=index>=4?1:(direction<0?1:2);
            }
            state.selectionChange.ForceStart();
        } else if(message==ScreenMessage::Highlight) {
            constexpr std::int32_t blurbs[]={0x24cac,0x24cad,0x24cae,0x24caf,0x24cb1,0x24cb2,0x24cb3,0x24cb4,0x24cb5};
            const auto index=data.key-MenuFirst63;
            const auto value=data.key==SubtitleRow?SubtitleValue:index<Values.size()?Values[index]:0;
            if(index>=6)state.selectionChange.SetCurrentMenuItem(0);
            if(data.key==SubtitleRow)blurb_.NewText("Show police radio subtitles during races.");
            else blurb_.NewText(index<std::size(blurbs)?blurbs[index]:0);
            state.selectionChange.SetCurrentMenuItem(value);arrows_.SetCurrentItem(value);
            SetMenuHighlight(data,scrolling_.GetCurrentMenuMask(),scrolling_.CurrentMenuOffsetY());
        } else if((message==ScreenMessage::Previous && data.key==MenuFirst63) ||
                  (message==ScreenMessage::Next && data.key==MenuFirst63+8))data.value=1;
        else if(message==ScreenMessage::Select) {
            const char* destination=nullptr;
            if(data.key==MenuFirst63+6)destination="Customize P1 HUD";
            else if(data.key==MenuFirst63+7)destination="Customize P2 HUD";
            else if(data.key==MenuFirst63+8)destination="Calibrate display offset";
            else data.sound=0;
            if(destination) {
                services.frontend->displaySettings=editing_;
                state.exit=std::make_unique<FlailerExitScreenAnimation>(ui,*services.screens,state.screen,destination,true);
                float x{},y{};const auto* anchor=ui.resources->FindObject(state.screen,0x32c91daeu);
                Require(anchor,"Display exit is missing its authored rail");ui.resources->GetTopLeft(*anchor,x,y,ui.language);
                state.exit->AddGroup(0x97bf4b25u,0x32c91daeu,x,650.0f,state.exit->MaximumDuration());arrows_.SetCurrentItem(0);
            }
        } else if(message==ScreenMessage::LeaveScreen) {
            services.frontend->displaySettings=editing_;
            if(gDisplayOptionsChangeManager.Different(*services.frontend) && services.frontend->autoSave)
                services.screens->stack.Push("Autosave");
        } else if(message==ScreenMessage::Tick) {
            if(ui.externalPresentation && state.selection->current)
                PositionHighlightBars(scrolling_.GetCurrentMenuMask(),scrolling_.CurrentMenuOffsetY());
            blurb_.Animate();
        }
    }
    static void Callback(ScreenMessage message,ScreenCallbackData* data)
    {
        if(message==ScreenMessage::ScreenActivate) {
            Require(data,"Display activation has no source screen data");
            Install(std::make_unique<DisplayOptionsScreen>(BoundServices(),data->key));
        } else BaseCallback(message,data);
    }
};
// JukeBoxMusicScreen1F02D8..1F1280 uses the same Options base, live FRED,
// authored text objects and actual cJukebox player as the other screens.
class JukeBoxMusicScreen final:public OptionsScreenBase {
    FrontendMusicSettings editing_;
    GenreMgr genres_;
    std::int32_t previewTicks_{};
    bool previewPending_{},listFocus_{};
    std::int32_t topRow_{};
    std::unique_ptr<FlailerListViewport> playlist_;
    std::vector<std::array<FlailerTextChunk*,3>> playlistRows_;
    std::array<float,3> playlistY_{};
    struct PlaylistMarker {FlailerObjectChunk* object;float y;std::uint32_t clip;};
    std::array<PlaylistMarker,2> playlistMarkers_{};
    int playlistGenre_=-1;
    static constexpr std::array<std::uint32_t,5> Titles{{0x64a819c7u,0x64a819c8u,0x64a819c9u,0x64a819cau,0x64a819cbu}};
    static constexpr std::array<std::uint32_t,5> Artists{{0x03b57047u,0x03b57048u,0x03b57049u,0x03b5704au,0x03b5704bu}};
    static constexpr std::array<std::uint32_t,5> Statuses{{0x89fb635fu,0x8c508c00u,0x8ea5b4a1u,0x90fadd42u,0x935005e3u}};
    static constexpr std::array<std::uint32_t,5> FirstMarkers{{0x0b825845u,0x0b825846u,0x0b825847u,0x0b825848u,0x0b825849u}};
    static constexpr std::array<std::uint32_t,5> SecondMarkers{{0xfc0279b8u,0xfc0279b9u,0xfc0279bau,0xfc0279bbu,0xfc0279bcu}};
    static constexpr std::array<std::uint32_t,6> StatusLabels{{0x1add9u,0x1addau,0x1adddu,0x1adbbu,0x1ade4u,0x1addcu}};
    static std::uint32_t PackedColor(const std::array<std::uint32_t,3>& rgb)
    {
        return 0x80000000u | static_cast<std::uint32_t>(rgb[0]) |
            (static_cast<std::uint32_t>(rgb[1])<<8) | (static_cast<std::uint32_t>(rgb[2])<<16);
    }
    const GenreMgr::Genre& CurrentGenre() const
    {
        Require(editing_.menuMode<genres_.count,"Saved music genre is absent from the actual cartridge");
        return genres_.genres[editing_.menuMode];
    }
    FrontendMusicSettings::Row& SelectedRow()
    {
        const auto& genre=CurrentGenre();const auto selected=editing_.menuState[0];
        Require(selected>=0 && selected<genre.count,"Music selection exceeds its actual genre rows");
        return editing_.rows[genre.rows[static_cast<std::size_t>(selected)]];
    }
    const FrontendMusicSettings::Row& FavouriteRow() const
    {
        const auto row=editing_.menuState[1];
        Require(row>=0 && static_cast<std::size_t>(row)<editing_.rows.size(),
            "Saved music footer row exceeds the original settings table");
        return editing_.rows[static_cast<std::size_t>(row)];
    }
    bool GenreEnabled(std::int32_t genre) const
    {
        Require(genre>=0 && genre<6,"Saved music genre bit is outside the original genre bank");
        return (editing_.enabledGenres&(std::uint32_t{1}<<genre))!=0;
    }
    std::pair<std::string,std::string> TrackText(const FrontendMusicSettings::Row& row) const
    {
        const auto& sound=*state_->services.sounds;
        const auto* name=soundJukeboxGetTrackName(sound,row.group,row.track);
        Require(name,"Music title is absent from the actual cartridge metadata");
        std::string title(name);
        const auto* artist=soundJukeboxGetTrackArtist(sound,row.group,row.track);
        Require(artist,"Music artist is absent from the actual cartridge metadata");
        return {std::move(title),artist};
    }
    float PreviewElapsed() const
    {
        const auto difference=static_cast<std::uint32_t>(state_->services.ui->clock->realTimer)-
            static_cast<std::uint32_t>(previewTicks_);
        std::int32_t signedDifference;std::memcpy(&signedDifference,&difference,sizeof(signedDifference));
        return static_cast<float>(signedDifference)*0.0002500000118743628f;
    }
    void PreparePlaylist()
    {
        auto& resources=*state_->services.ui->resources;
        const auto screen=state_->screen;
        auto* marker=static_cast<FlailerBitmapChunk*>(resources.FindObject(screen,FirstMarkers[0],2));
        auto* second=resources.FindObject(screen,SecondMarkers[0]);
        const auto top=resources.FindObject(screen,Titles[0],1)->topLeftY;
        const auto pitch=resources.FindObject(screen,Titles[1],1)->topLeftY-top;
        Require(marker && second && pitch>0.0f,"Jukebox scrolling requires the authored playlist rows");
        playlist_=std::make_unique<FlailerListViewport>(resources,*resources.FindScreen(screen),*marker,
            -4.0f,top-3.0f,648.0f,5.0f*pitch,pitch,5);
        playlistMarkers_={{{marker,marker->topLeftY,marker->clipObjectKey},{second,second->topLeftY,second->clipObjectKey}}};
        for(const auto& entry:playlistMarkers_)playlist_->Clip(*entry.object);
        const std::array keys{Titles[0],Artists[0],Statuses[0]};
        for(std::size_t column=0;column<keys.size();++column)
            playlistY_[column]=resources.FindObject(screen,keys[column],1)->topLeftY;
        int count=0;
        for(int i=0;i<genres_.count;++i)count=std::max(count,static_cast<int>(genres_.genres[i].count));
        playlistRows_.resize(static_cast<std::size_t>(count));
        for(std::size_t row=0;row<playlistRows_.size();++row)for(std::size_t column=0;column<keys.size();++column) {
            const auto& model=*static_cast<const FlailerTextChunk*>(resources.FindObject(screen,keys[column],1));
            const auto key=hashString(("HP2_PLAYLIST_"+std::to_string(row)+"_"+std::to_string(column)).c_str());
            playlistRows_[row][column]=&playlist_->CloneText(model,key);
        }
        for(std::size_t i=0;i<Titles.size();++i) {
            for(const auto key:{Titles[i],Artists[i],Statuses[i]})resources.SetObjectVisibility(screen,key,0);
            if(i)for(const auto key:{FirstMarkers[i],SecondMarkers[i]})resources.SetObjectVisibility(screen,key,0);
        }
    }
    void PositionPlaylist()
    {
        for(std::size_t row=0;row<static_cast<std::size_t>(CurrentGenre().count);++row)
            for(std::size_t column=0;column<playlistY_.size();++column)
                playlistRows_[row][column]->SetTopLeftY(playlistY_[column]+playlist_->Displacement(row));
        for(const auto& entry:playlistMarkers_)
            entry.object->SetTopLeftY(entry.y+playlist_->Displacement(editing_.menuState[0]));
    }
    void StylePlaylistRow(const GenreMgr::Genre& genre,int number,bool enabled,
        std::uint32_t title,std::uint32_t artist,std::uint32_t state)
    {
        auto& ui=*state_->services.ui;auto& resources=*ui.resources;const auto screen=state_->screen;
        const auto& row=editing_.rows[genre.rows[static_cast<std::size_t>(number)]];
        const auto text=TrackText(row);
        resources.Printf(screen,artist,"%s",text.second.c_str());
        resources.Printf(screen,title,"%s",text.first.c_str());
        const auto status=enabled?row.status:0;
        Require(status>=0 && static_cast<std::size_t>(status)<StatusLabels.size(),"Music status is outside its six source choices");
        resources.SetLanguageID(screen,state,StatusLabels[static_cast<std::size_t>(status)]);
        const bool highlighted=listFocus_ && editing_.menuState[0]==number;
        const auto color=highlighted?PackedColor(ui.highlightedRgb):(!enabled?PackedColor(ui.inactiveRgb):0x00808080u);
        for(const auto key:{title,artist,state}) {
            resources.SetTextColor(screen,key,color);resources.SetObjectVisibility(screen,key,1);
        }
    }
public:
    JukeBoxMusicScreen(MainMenuScreenServices& services,std::uint32_t screen)
        :OptionsScreenBase(services,screen),editing_(services.frontend->musicSettings)
    {
        Require(services.sounds && services.sounds->audio && services.sounds->audio->library &&
            services.raceSetup && services.raceSetup->parameters && services.ui->clock &&
            soundJukeboxPlayer.audio==services.sounds->audio && soundJukeboxPlayer.IsCartridgeLoaded(),
            "Music screen has no actual cartridge, mixer, race settings or source clock");
        editing_.menuState[0]=0;
        genres_.Populate(*services.sounds,editing_);
        if(services.ui->externalPresentation)PreparePlaylist();
        ShowPlaylist();
    }
    ~JukeBoxMusicScreen() override
    {
        if(!playlist_)return;
        for(const auto& entry:playlistMarkers_) {
            entry.object->SetTopLeftY(entry.y);entry.object->SetClipToPoly(entry.clip);
        }
    }
    void ShowPlaylist()
    {
        auto& ui=*state_->services.ui;auto& resources=*ui.resources;const auto screen=state_->screen;
        const auto& genre=CurrentGenre();const auto selected=editing_.menuState[0];
        Require(selected>=0 && selected<genre.count,"Music selection is outside the displayed genre");
        while(selected>=topRow_+5 && topRow_+5<genre.count)++topRow_;
        while(selected<topRow_ && topRow_>=0)--topRow_;
        const bool enabled=GenreEnabled(editing_.menuMode);
        const auto visible=std::min<std::int32_t>(5,genre.count);
        if(playlist_) {
            if(playlistGenre_!=editing_.menuMode || !listFocus_)playlist_->Reset();
            playlistGenre_=editing_.menuMode;
            playlist_->Reveal(selected);
            for(std::size_t i=0;i<playlistRows_.size();++i) {
                const auto& row=playlistRows_[i];
                if(i<static_cast<std::size_t>(genre.count))StylePlaylistRow(genre,static_cast<int>(i),enabled,row[0]->key,row[1]->key,row[2]->key);
                else for(const auto* text:row)resources.SetObjectVisibility(screen,text->key,0);
            }
            for(const auto& entry:playlistMarkers_)resources.SetObjectVisibility(screen,entry.object->key,listFocus_);
            PositionPlaylist();
        } else for(std::size_t index=0;index<Titles.size();++index) {
            if(static_cast<std::int32_t>(index)>=visible) {
                for(const auto key:{Titles[index],Artists[index],Statuses[index],FirstMarkers[index],SecondMarkers[index]})
                    resources.SetObjectVisibility(screen,key,0);
                continue;
            }
            const auto number=topRow_+static_cast<std::int32_t>(index);
            Require(number>=0 && number<genre.count,"Music scroller exceeded its genre index bank");
            const bool highlighted=listFocus_ && selected==number;
            resources.SetObjectVisibility(screen,FirstMarkers[index],highlighted);
            resources.SetObjectVisibility(screen,SecondMarkers[index],highlighted);
            StylePlaylistRow(genre,number,enabled,Titles[index],Artists[index],Statuses[index]);
        }
        resources.SetObjectVisibility(screen,0xedf2e4eeu,listFocus_ && topRow_!=0);
        resources.SetObjectVisibility(screen,0x20d73d01u,listFocus_ && genre.count>=5 && topRow_!=genre.count-5);
        const auto footer=TrackText(FavouriteRow());
        resources.Printf(screen,0x0b41999au,"%s (%s)",footer.first.c_str(),footer.second.c_str());
        resources.Printf(screen,0x52e039f3u,"%s",genre.name.data());
        resources.SetTextColor(screen,0x52e039f3u,listFocus_?0x00808080u:PackedColor(ui.highlightedRgb));
        resources.SetObjectVisibility(screen,0xd1cda594u,!listFocus_);
        resources.SetObjectVisibility(screen,0x9345e4a7u,!listFocus_);
    }
    void PlayCurrentSelectedSong()
    {
        const auto& row=SelectedRow();
        Require(soundJukeboxPlayer.Start(static_cast<std::uint32_t>(row.group),
            static_cast<std::uint32_t>(row.track),0)==WORLD_AUDIO_OK,
            "Music preview could not start its actual selected stream");
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        auto& services=state_->services;
        switch(message) {
        case ScreenMessage::LeaveScreen: {
            if(!SameAudioOptions63(editing_,services.frontend->musicSettings) && services.frontend->autoSave)
                services.screens->stack.Push("Autosave");
            services.frontend->musicSettings=editing_;
            Require(soundJukeboxSetSelection(*services.sounds,*services.frontend,
                *services.raceSetup->parameters,5)==WORLD_AUDIO_OK,"Music selection could not apply its actual track choices");
            const auto& row=FavouriteRow();
            const auto result=GenreEnabled(row.genreBit) && row.status!=0 ?
                soundJukeboxPlayer.Start(static_cast<std::uint32_t>(row.group),static_cast<std::uint32_t>(row.track),1):
                soundJukeboxPlayer.Start(1);
            Require(result==WORLD_AUDIO_OK,"Music exit could not resume the actual playlist");
            break;
        }
        case ScreenMessage::ScreenEvent13:
            editing_.enabledGenres^=static_cast<std::uint8_t>(std::uint32_t{1}<<editing_.menuMode);
            if(!GenreEnabled(editing_.menuMode)){listFocus_=false;topRow_=0;editing_.menuState[0]=0;}
            data.sound=5;ShowPlaylist();break;
        case ScreenMessage::Left:case ScreenMessage::Right: {
            const std::int32_t delta=message==ScreenMessage::Left?-1:1;
            if(listFocus_) {
                auto& row=SelectedRow();const auto value=static_cast<std::int32_t>(row.status)+delta;
                row.status=static_cast<std::int8_t>(value<0?5:value>5?0:value);
            } else {
                const auto value=static_cast<std::int32_t>(editing_.menuMode)+delta;
                editing_.menuMode=static_cast<std::uint8_t>(value<0?genres_.count-1:value>=genres_.count?0:value);
                editing_.menuState[0]=0;
            }
            data.sound=message==ScreenMessage::Left?1:2;ShowPlaylist();break;
        }
        case ScreenMessage::Previous:case ScreenMessage::Next:
            if(listFocus_) {
                auto& selected=editing_.menuState[0];
                if(message==ScreenMessage::Previous && selected==0 && topRow_==0) {
                    listFocus_=false;
                    Require(soundJukeboxPlayer.Stop()==WORLD_AUDIO_OK,"Music genre return could not stop its preview");
                    ShowPlaylist();break;
                }
                const auto candidate=selected+(message==ScreenMessage::Previous?-1:1);
                if(candidate<0 || candidate>=CurrentGenre().count){data.sound=0;break;}
                selected=candidate;data.sound=message==ScreenMessage::Previous?3:4;
                previewTicks_=services.ui->clock->realTimer;previewPending_=true;ShowPlaylist();
            } else if(message==ScreenMessage::Previous)data.sound=0;
            else if(GenreEnabled(editing_.menuMode)) {
                listFocus_=true;ShowPlaylist();previewTicks_=services.ui->clock->realTimer;previewPending_=true;
            } else data.sound=6;
            break;
        case ScreenMessage::Tick:
            if(playlist_ && playlist_->Tick(services.ui->clock->realTimeElapsed))PositionPlaylist();
            if(PreviewElapsed()>0.5f && previewPending_){previewPending_=false;PlayCurrentSelectedSong();}
            if(PreviewElapsed()>30.0f) {
                Require(soundJukeboxPlayer.Update()==WORLD_AUDIO_OK,"Music preview update failed");
                if(soundJukeboxPlayer.state!=2 && !soundJukeboxPlayer.IsFading())PlayCurrentSelectedSong();
            }
            break;
        case ScreenMessage::Select:case ScreenMessage::ItemEvent:case ScreenMessage::ScreenBack:data.sound=0;break;
        default:break;
        }
    }
    static void Callback(ScreenMessage message,ScreenCallbackData* data)
    {
        if(message==ScreenMessage::ScreenActivate) {
            Require(data,"Music activation has no original screen payload");
            Install(std::make_unique<JukeBoxMusicScreen>(BoundServices(),data->key));
        } else BaseCallback(message,data);
    }
};

constexpr std::int32_t AudioHelp63[]={800060,800061,800500,800063,0};
constexpr std::int32_t DisplayHelp63[]={800067,800068,800311,800069,0};
}
std::array<FlailerScreenConfig,3>& AudioDisplayScreenConfigurations()
{
    static constexpr std::int32_t musicHelp[]={802220,802221,802223,802224,0};
    static std::array<FlailerScreenConfig,3> configurations{{
        {"Audio Options",&AudioOptionsScreen::Callback,{{static_cast<std::int32_t>(0xe116761cu),0,0,0}},{{-1,-1,-1,-1}},1,AudioHelp63},
        {"Display Options",&DisplayOptionsScreen::Callback,{{static_cast<std::int32_t>(0x8bc68340u),0,0,0}},{{-1,-1,-1,-1}},1,DisplayHelp63},
        {"JukeBox Music",&JukeBoxMusicScreen::Callback,{{static_cast<std::int32_t>(0x95b2b627u),0,0,0}},{{-1,-1,-1,-1}},1,musicHelp}}};
    return configurations;
}
} // namespace hp2

namespace hp2 {
namespace {
// PAL1F3F20..1F4A2C; the switch tables at2BA370/2BA390 put Lookback
// and CameraSpring before the gauges, independent of basic-block order.
class CustomizeHUDScreen final:public OptionsScreenBase {
    std::int32_t port_;
    PlayerSettings editing_;
    FlailerAnimatedBlurb blurb_;
    FlailerBitmapChunk* left_{};
    FlailerBitmapChunk* right_{};
    static constexpr std::array<std::uint32_t,7> Values{{0x2e018745u,0xffc3dcd2u,
        0x4ceb3ed2u,0x1054482bu,0xc7e2117eu,0x6caf6d10u,0x6bd89c11u}};
    static PlayerSettings& Settings(MainMenuScreenServices& services,std::int32_t port)
    {
        Require(services.frontend && port>=0 && port<2 && services.frontend->players[port],
            "HUD options have no actual selected player profile");
        return services.frontend->players[port]->settings;
    }
    void Redraw(std::size_t index)
    {
        std::uint32_t language{};
        switch(index) {
        case 0:language=editing_.lookback==0?0x22356u:editing_.lookback==1?0x22351u:0;break;
        case 1:language=editing_.cameraSpring==1?0x22353u:0x22352u;break;
        case 2:language=(editing_.hudFlags&PLAYER_HUD_GAUGES)?0x22352u:0x22353u;break;
        case 3:language=(editing_.hudFlags&PLAYER_HUD_RACE_POSITION)?0x22352u:0x22353u;break;
        case 4:language=(editing_.hudFlags&PLAYER_HUD_LAP_INFO)?0x22352u:0x22353u;break;
        case 5: {
            constexpr std::uint32_t labels[]={0x2234cu,0x2234du,0x2234eu,0x2234fu,0x22353u};
            if(static_cast<std::uint32_t>(editing_.splitTimeMode)<std::size(labels))
                language=labels[editing_.splitTimeMode];
            break;
        }
        case 6:language=editing_.scoring?0x22352u:0x22353u;break;
        default:throw std::runtime_error("Invalid original HUD option index");
        }
        state_->services.ui->resources->SetLanguageID(state_->screen,Values[index],language);
    }
    void DoHighlight(std::uint32_t key)
    {
        if(key)ArrowAligner(*state_->services.ui,state_->screen,0xd1cda594u,0x9345e4a7u,key);
        if(left_)left_->visible=key?1:0;
        if(right_)right_->visible=key?1:0;
    }
public:
    CustomizeHUDScreen(MainMenuScreenServices& services,std::uint32_t screen,std::int32_t port)
        :OptionsScreenBase(services,screen),port_(port),editing_(Settings(services,port)),
         blurb_(*services.ui,screen,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        for(std::size_t index=0;index<Values.size();++index)Redraw(index);
        left_=static_cast<FlailerBitmapChunk*>(services.ui->resources->FindObject(screen,0xd1cda594u,2));
        right_=static_cast<FlailerBitmapChunk*>(services.ui->resources->FindObject(screen,0x9345e4a7u,2));
    }
    ~CustomizeHUDScreen() override
    {
        // Source dtor commits the complete48-byte logical settings record;
        // the parent Display change manager controls the later autosave.
        Settings(state_->services,port_)=editing_;
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override
    {
        const auto index=data.key-MenuFirst63;
        if(message==ScreenMessage::Left || message==ScreenMessage::Right) {
            if(index<Values.size()) {
                switch(index) {
                case 0:playerSettingsToggleLookback(&editing_);break;
                case 1:playerSettingsToggleCameraSpring(&editing_);break;
                case 2:playerSettingsToggleGauges(&editing_);break;
                case 3:playerSettingsToggleRacePosition(&editing_);break;
                case 4:playerSettingsToggleLapInfo(&editing_);break;
                case 5:playerSettingsCycleSplitTime(&editing_,message==ScreenMessage::Left?-1:1);break;
                case 6:playerSettingsToggleScoring(&editing_);break;
                }
                Redraw(index);data.sound=message==ScreenMessage::Left?1:2;
            }
            state_->selectionChange.ForceStart();
        } else if(message==ScreenMessage::Highlight) {
            constexpr std::int32_t blurbs[]={0x24d74,0x24d75,0x24d76,0x24d77,0x24d78,0x24d79,0x24cb0};
            if(index<Values.size()) {
                DoHighlight(index<2?data.key:Values[index]);blurb_.NewText(blurbs[index]);
                state_->selectionChange.SetCurrentMenuItem(Values[index]);
            } else {
                blurb_.NewText(0);state_->selectionChange.SetCurrentMenuItem(0);
            }
        } else if(message==ScreenMessage::Tick) {
            blurb_.Animate();AnimateCursor(*state_->services.ui,left_,right_,std::int32_t(0));
        } else if(message==ScreenMessage::Select || message==ScreenMessage::ScreenBack)data.sound=0;
    }
    static void Callback(ScreenMessage message,ScreenCallbackData* data,std::int32_t port)
    {
        if(message==ScreenMessage::ScreenActivate) {
            Require(data,"HUD activation has no authored screen data");
            Install(std::make_unique<CustomizeHUDScreen>(BoundServices(),data->key,port));
        } else BaseCallback(message,data);
    }
    static void CallbackP1(ScreenMessage message,ScreenCallbackData* data){Callback(message,data,0);}
    static void CallbackP2(ScreenMessage message,ScreenCallbackData* data){Callback(message,data,1);}
};
constexpr std::int32_t CustomizeHUDHelp63[]={800067,800068,800312,0};
}
std::array<FlailerScreenConfig,2>& CustomizeHUDScreenConfigurations()
{
    static std::array<FlailerScreenConfig,2> configurations{{
        {"Customize P1 HUD",&CustomizeHUDScreen::CallbackP1,{{static_cast<std::int32_t>(0x28891a22u),0,0,0}},{{-1,-1,-1,-1}},1,CustomizeHUDHelp63},
        {"Customize P2 HUD",&CustomizeHUDScreen::CallbackP2,{{static_cast<std::int32_t>(0x28891a22u),0,0,0}},{{-1,-1,-1,-1}},1,CustomizeHUDHelp63}}};
    return configurations;
}
} // namespace hp2

#include "Ecstasy.hpp"
namespace hp2 {
namespace {
std::array<std::int32_t,2> displayCalibrationDefaults{};
bool displayCalibrationDefaultsCaptured{};
std::uint32_t displayCalibrationScreen{};
std::uint32_t displayCalibrationOffsetScreen{};

MainMenuScreenServices& CalibrationServices()
{
    Require(optionsServices && optionsServices->frontend && optionsServices->screens &&
        optionsServices->ui && optionsServices->ui->clock && optionsServices->ui->resources &&
        optionsServices->ui->render && optionsServices->ui->language,
        "Display calibration lacks its actual frontend/render/language owners");
    return *optionsServices;
}

std::int32_t CalibrationAdd(std::int32_t value,std::int32_t increment)
{
    // Original ADDIU wraps a 32-bit field. Avoid native signed overflow.
    const auto bits=static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(increment);
    std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
}

void DrawCalibrationHelp(MainMenuScreenServices& services,std::uint32_t screen,std::uint32_t textId)
{
    auto& ui=*services.ui;
    const auto* text=ui.language->GetString(static_cast<std::int32_t>(textId));
    Require(text,"Display calibration has no authored localized instruction");
    ui.render->RenderTextIntoRectangle(*ui.resources,screen,0x6586d35fu,0xaf18eb98u,text,1.0f);
}

void DisplayCalibrateCallback(ScreenMessage message,ScreenCallbackData* data)
{
    // Complete PAL1F4A30..1F4AD8; this screen has no OptionsScreenBase owner.
    Require(data,"Display calibration callback has no original message payload");
    auto& services=CalibrationServices();data->sound=0;
    if(message==ScreenMessage::LeaveScreen)data->sound=6;
    else if(message==ScreenMessage::ScreenActivate)displayCalibrationScreen=data->key;
    else if(message==ScreenMessage::RenderModels)
        DrawCalibrationHelp(services,displayCalibrationScreen,0x222f4u);
}

void DisplayCalibrateOffsetCallback(ScreenMessage message,ScreenCallbackData* data)
{
    // Complete PAL1F4B20..1F4C98, including the final Commit on every message.
    Require(data,"Display offset callback has no original message payload");
    auto& services=CalibrationServices();data->sound=0;
    auto& setting=services.frontend->displayCalibration;
    switch(message) {
    case ScreenMessage::Left:
        setting[0]=CalibrationAdd(setting[0],-4);
        if(setting[0]<-650)setting[0]=-650;else data->sound=1;
        break;
    case ScreenMessage::Right:
        setting[0]=CalibrationAdd(setting[0],4);data->sound=1;break;
    case ScreenMessage::Previous:
        setting[1]=CalibrationAdd(setting[1],-1);
        if(setting[1]<-50)setting[1]=-50;else data->sound=1;
        break;
    case ScreenMessage::Next:
        setting[1]=CalibrationAdd(setting[1],1);data->sound=1;break;
    case ScreenMessage::LeaveScreen:data->sound=6;break;
    case ScreenMessage::ScreenActivate:displayCalibrationOffsetScreen=data->key;break;
    case ScreenMessage::Select:
        data->sound=5;
        Require(services.screens->MoveToPreviousScreen(1,false,0),
            "Display offset calibration has no preceding original screen");
        break;
    case ScreenMessage::ScreenEvent8:
        Require(displayCalibrationDefaultsCaptured,"Display calibration factory offsets were not captured");
        setting=displayCalibrationDefaults;data->sound=5;break;
    case ScreenMessage::RenderModels:
        DrawCalibrationHelp(services,displayCalibrationOffsetScreen,0x222f3u);break;
    default:break;
    }
    ApplyDisplayCalibration(*services.frontend,*services.ui->clock);
}
constexpr std::int32_t CalibrationHelp63[]={800313,0};
}

void InitializeDisplayCalibration(FrontendPlayerManager& frontend,const GameTime& clock)
{
    // Original tDisplayCalibrationData constructor1F4AD8 captures the process
    // defaults and reads the actual current display slot into the FRED record.
    if(!displayCalibrationDefaultsCaptured) {
        eGetScreenDisplayOffsets(clock,displayCalibrationDefaults[0],displayCalibrationDefaults[1]);
        displayCalibrationDefaultsCaptured=true;
    }
    eGetScreenDisplayOffsets(clock,frontend.displayCalibration[0],frontend.displayCalibration[1]);
}

void ApplyDisplayCalibration(const FrontendPlayerManager& frontend,const GameTime& clock)
{
    // Original tDisplayCalibrationData::Commit1F4B10 tail-calls eSet.
    eSetScreenDisplayOffsets(clock,frontend.displayCalibration[0],frontend.displayCalibration[1]);
}

std::array<FlailerScreenConfig,2>& DisplayCalibrationScreenConfigurations()
{
    static std::array<FlailerScreenConfig,2> configurations{{
        {"Calibrate Display",&DisplayCalibrateCallback,{{static_cast<std::int32_t>(0xddf09124u),0,0,0}},{{-1,-1,-1,-1}},1,CalibrationHelp63},
        {"Calibrate display offset",&DisplayCalibrateOffsetCallback,{{static_cast<std::int32_t>(0x7ab428c1u),0,0,0}},{{-1,-1,-1,-1}},1,CalibrationHelp63}}};
    return configurations;
}
} // namespace hp2

// Original Options.obj / indep/src/frontendscreens/options.cpp:
// CreditsScreen, CreditsMoviePlayer, CreditsLine, tCreditsSlideshow and
// CreditsScroller. Native services borrow the existing application owners.
#include "CarSounds.hpp"
#include "EventTree.hpp"
#include "Language.hpp"
#include "GameFlow.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace hp2 {
namespace {
constexpr std::int32_t CreditsHelp[]={800080,800081,800079,0};
constexpr std::int32_t CreditsScrollHelp[]={800082,800083,800084,0};
constexpr const char* CreditsNames[]={"Credits","Credits FE Movie","Credits Reward movie",
    "BBG Creation movie","BBG Credits movie","EACredits","BBGCredits"};
FrontendStartupServices* creditsServices{};
constexpr char PortCreditsScreenName[]="Port Credits";
constexpr std::uint32_t PortCreditsItemKey=0x52e039f8u; // MENUITEM_6, port-side.

MainMenuScreenServices& CreditsMenus() {return *creditsServices->menus;}
void CreditsMusicStart(MainMenuScreenServices& menus)
{
    const auto& setup=*menus.raceSetup;
    Require(soundJukeboxStart(*menus.sounds,*menus.frontend,*setup.parameters,*setup.flow,true)==WORLD_AUDIO_OK,
        "Credits jukebox could not restart on its actual audio owner");
}
void CreditsGrayItem(MainMenuScreenServices& menus,std::uint32_t screen,std::uint32_t key,bool available)
{
    // The released GrayMenuItem(...,1/2,screen) enables/shows or disables/hides.
    auto* node=menus.screens->FindScreen(static_cast<std::int32_t>(screen));
    if (!node || !node->screen || !node->screen->menu)return;
    auto* item=node->screen->menu->FindItem(key);
    if (!item)return;
    if (available)item->SetMenuItemState(1);else item->ClearMenuItemState(1);
    menus.ui->resources->SetObjectVisibility(screen,key,available?1:0);
}

// Port-side row. It shares the panel motion and uses the original menu ring.
class PortCreditsRow final {
public:
    PortCreditsRow(MainMenuScreenServices& menus,std::uint32_t screen,bool reward)
        : resources_(*menus.ui->resources),screen_(*menus.screens->active->screen)
    {
        auto& art=*resources_.FindScreen(screen);
        auto& menu=*screen_.menu;
        auto* making=menu.FindItem(0x52e039f6u);
        Require(making && making->object && !resources_.FindObject(screen,PortCreditsItemKey),
            "Port Credits requires the authored Making of HP2 row");
        text_=static_cast<const FlailerTextChunk&>(*making->object);
        text_.strip=nullptr;
        text_.key=PortCreditsItemKey;
        text_.name.fill(0);
        std::memcpy(text_.name.data(),"MENUITEM_6",10);
        text_.textCapacity=static_cast<std::uint32_t>(std::strlen(menus.credits->label)+1);
        text_.ownedText.assign(text_.textCapacity+1,0);
        text_.text=text_.ownedText.data();
        text_.SetString(menus.credits->label);
        item_=std::make_unique<FlailerMenuItem>(&menu,text_.key,6);
        item_->object=&text_;
        if (reward) {
            constexpr std::array<std::uint32_t,5> keys{0x52e039f3u,0x52e039f4u,0x52e039f5u,0x52e039f6u,0x52e039f7u};
            constexpr std::array<float,5> positions{173,201,229,257,313};
            for (std::size_t i=0;i<keys.size();++i) {
                auto* object=resources_.FindObject(screen,keys[i],1);
                Require(object,"Port Credits requires all five authored rows");
                positions_[i]={object,object->topLeftY};
            }
            for (std::size_t i=0;i<keys.size();++i)positions_[i].first->SetTopLeftY(positions[i]);
        }
        text_.SetTopLeftY(reward?285.0f:295.0f);
        text_.previous=art.objects.previous;text_.next=&art.objects;
        text_.previous->next=&text_;art.objects.previous=&text_;
        ++art.textCount;
        art_=&art;
        item_->ResetSavedParameters();
        item_->previous=making;item_->next=making->next;
        making->next->previous=item_.get();making->next=item_.get();
    }
    ~PortCreditsRow()
    {
        if (auto* menu=screen_.menu) {
            if(menu->current==item_.get())menu->current=item_->previous;
            item_->previous->next=item_->next;item_->next->previous=item_->previous;
        }
        item_.reset();
        text_.DeleteFlailerStrip();
        text_.previous->next=text_.next;text_.next->previous=text_.previous;
        --art_->textCount;
        for (const auto& [object,y]:positions_)if(object)object->SetTopLeftY(y);
    }
private:
    FlailerResources& resources_;
    FlailerScreen& screen_;
    FlailerScreenResource* art_{};
    FlailerTextChunk text_{};
    std::unique_ptr<FlailerMenuItem> item_;
    std::array<std::pair<FlailerObjectChunk*,float>,5> positions_{};
};

class CreditsScreen final {
public:
    CreditsScreen(MainMenuScreenServices& menus,std::uint32_t screen)
        : menus_(menus),screen_(screen),slide_(*menus.ui,screen,0xaed1a723u,0x718cd8c3u,0x95ad096bu),
          highlight_(*menus.ui,screen,0xab671c9au,menus.ui->animationSettings.highlightColor)
    {
        menus_.screens->StartMovie("MOVIES\\options_movie_new.AVI");
        const bool reward=ShouldShowRewardMovieOption();
        CreditsGrayItem(menus_,screen_,0x52e039f7u,reward);
        if(menus_.credits)portRow_=std::make_unique<PortCreditsRow>(menus_,screen_,reward);
    }
    bool ShouldShowRewardMovieOption() const
    {
        // Final2026F8: live event32 in BOTH canonical progress trees must be
        // status4. The original GP cells map to PrimaryTree1 and PrimaryTree0.
        const auto* events=menus_.raceSetup->events;
        Require(events,"Credits reward query has no actual EventTree resource owner");
        auto* first=events->PrimaryTree(1);
        auto* second=events->PrimaryTree(0);
        Require(first && second,"Credits reward query lacks one original event tree");
        const auto* a=first->GetEventNumber(32);const auto* b=second->GetEventNumber(32);
        Require(a && b,"Credits reward query lacks authored event32");
        return a->GetStatus()==4 && b->GetStatus()==4;
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        highlight_.HandleEvents(message,data);
        if (message==ScreenMessage::Tick)slide_.Tick();
        else if (message==ScreenMessage::Select && data.key==PortCreditsItemKey && portRow_) {
            menus_.screens->ChangeToScreen(PortCreditsScreenName,true);
        } else if (message==ScreenMessage::Select && data.key>=0x52e039f3u && data.key<=0x52e039f7u) {
            static constexpr const char* destinations[]={"EACredits","BBG Credits movie",
                "Credits FE Movie","BBG Creation movie","Credits Reward movie"};
            menus_.screens->ChangeToScreen(destinations[data.key-0x52e039f3u],true);
        }
    }
private:
    MainMenuScreenServices& menus_;
    std::uint32_t screen_;
    FlailerSlideOnScreen slide_;
    FlailerHighlightAnimation highlight_;
    std::unique_ptr<PortCreditsRow> portRow_;
};

class CreditsMoviePlayer final {
public:
    CreditsMoviePlayer(FrontendStartupServices& services,std::uint32_t screen,
        const char* movie,const char* following)
        : services_(services),following_(following)
    {
        auto& menus=*services_.menus;auto& ui=*menus.ui;
        Require(movie && ui.movie && services_.movieLog,"Credits movie has no actual file/player/replay owners");
        const bool creation=std::strcmp(movie,"MOVIES\\INTRO\\MAKEOF_EN.AVI")==0;
        const auto key=creation?0x3acf2d73u:0xbfaebf79u;
        bitmap_=static_cast<FlailerBitmapChunk*>(ui.resources->FindObject(screen,key,2));
        Require(bitmap_,"Credits movie lacks its authored movie bitmap");
        ui.resources->SetObjectVisibility(screen,0x3acf2d73u,creation?1:0);
        ui.resources->SetObjectVisibility(screen,0xbfaebf79u,creation?0:1);
        Require(soundJukeboxStop(*menus.sounds)==WORLD_AUDIO_OK,
            "Credits movie could not stop the actual jukebox");
        menus.screens->StopMovie();
        try {
            Require(ui.movie->InitFile(movie)==1,"Credits movie could not open its original authored AVI");
            ui.resources->SetObjectVisibility(screen,0xb070dbc9u,0);
            ui.resources->SetGroupVisibility(screen,0xb73c8d23u,0);
        } catch (...) {
            const auto primary=std::current_exception();
            if (const auto cleanup=ui.movie->Shutdown()) {
                try {std::rethrow_exception(cleanup);}
                catch (const std::exception& error) {RecordSessionEvent("movie.credits.cleanup_failed",error.what());}
                catch (...) {RecordSessionEvent("movie.credits.cleanup_failed","Unknown movie cleanup failure");}
            }
            try {CreditsMusicStart(menus);}
            catch (const std::exception& error) {RecordSessionEvent("movie.credits.music_restore_failed",error.what());}
            catch (...) {RecordSessionEvent("movie.credits.music_restore_failed","Unknown credits music restore failure");}
            std::rethrow_exception(primary);
        }
    }
    void Retire()
    {
        // PAL1F2358 destructor actions. Explicit retirement lets a failed
        // real audio submission preserve this owner for the caller's retry;
        // unique_ptr destruction is never a throwing acknowledgement path.
        if (retired_)return;
        services_.menus->ui->movie->Close();
        // Ordinary transitions restart music; terminal retirement has closed
        // admission and must not create another physical audio borrower.
        if (!carLoader.ShuttingDown())CreditsMusicStart(*services_.menus);
        retired_=true;
    }
    void MovieFinished()
    {
        auto& screens=*services_.menus->screens;
        if (following_)screens.ChangeToScreen(following_,false);
        else screens.MoveToPreviousScreen(1,false,0);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        if (message==ScreenMessage::Tick) {
            if (DriveMovie(*services_.menus->ui,*bitmap_,*services_.movieLog)==1) {
                services_.menus->ui->movie->Close();MovieFinished();
            }
        } else if (message==ScreenMessage::Select)MovieFinished();
        else if (message==ScreenMessage::Left || message==ScreenMessage::Right ||
            message==ScreenMessage::Previous || message==ScreenMessage::Next || message==ScreenMessage::ScreenBack)
            data.sound=0;
    }
private:
    FrontendStartupServices& services_;
    const char* following_{};
    FlailerBitmapChunk* bitmap_{};
    bool retired_{};
};

bool CreditsDigit(char c) {return c>='0' && c<='9';}
bool CreditsLetter(char c) {return (c>='a' && c<='z') || (c>='A' && c<='Z');}
const char* CreditsSkipToDigit(const char* text) {while (*text && !CreditsDigit(*text))++text;return text;}
const char* CreditsSkipToLetter(const char* text) {while (*text && !CreditsLetter(*text))++text;return text;}
std::uint32_t CreditsDecimal(const char* text)
{
    std::uint32_t value=0;
    while (CreditsDigit(*text)) {value=value*10u+static_cast<std::uint32_t>(*text-'0');++text;}
    return value;
}
std::uint32_t ParseColor(const char* text)
{
    std::uint32_t channels[]={128,128,128};
    const char* cursor=CreditsSkipToDigit(text);
    for (std::size_t i=0;i<3 && *cursor;++i) {
        channels[i]=CreditsDecimal(cursor);
        const char delimiter=i==2?')':',';
        const auto* end=std::strchr(cursor,delimiter);
        Require(end,"Credits RGB directive has no authored closing delimiter");
        cursor=CreditsSkipToDigit(end);
    }
    return 0x80000000u|channels[0]|(channels[1]<<8)|(channels[2]<<16);
}
std::uint32_t ParseFont(const char* text)
{
    const auto* first=CreditsSkipToLetter(text);
    if (!*first)return 0;
    const auto* end=std::strchr(first,')');
    Require(end,"Credits FONT directive has no authored closing delimiter");
    const std::string font(first,static_cast<std::size_t>(end-first));
    return hashString(font.c_str());
}
void ParseCreditsColors(std::uint32_t& color,std::uint32_t& font,char* output,
    std::size_t capacity,const char* source)
{
    Require(output && capacity && source,"Credits line parser has no actual text storage");
    std::array<char,32> fontText{},colorText{};
    std::fill_n(output,capacity,char{});
    bool command=false,parenthesis=false,fontCommand=false,colorCommand=false;
    char* token=nullptr;std::size_t tokenSize=0,outputSize=0;
    for (const char* current=source;*current;++current) {
        if (*current=='$') {
            if (current[1]=='$')++current;
            else {
                if (std::strncmp(current+1,"FONT",4)==0)fontCommand=true;
                if (std::strncmp(current+1,"RGB",3)==0)colorCommand=true;
                if (fontCommand || colorCommand)command=true;
            }
        }
        if (*current=='(' && command)parenthesis=true;
        if (!command && !parenthesis) {
            Require(outputSize+1<capacity,"Credits text exceeds the original bounded line buffer");
            output[outputSize++]=*current;
        }
        if (parenthesis && !token) {
            if (fontCommand)token=fontText.data();
            else if (colorCommand)token=colorText.data();
            tokenSize=0;
        }
        if (token) {
            Require(tokenSize+1<fontText.size(),"Credits directive exceeds its original bounded token buffer");
            token[tokenSize++]=*current;token[tokenSize]=0;
        }
        if (*current==')') {
            parenthesis=false;command=false;fontCommand=false;colorCommand=false;token=nullptr;
        }
    }
    Require(!command && !parenthesis,"Credits directive is truncated");
    if (colorText[0])color=ParseColor(colorText.data());
    if (fontText[0])font=ParseFont(fontText.data());
}

class CreditsLine final {
public:
    CreditsLine(FlailerUiState& ui,FlailerTextChunk& text):ui_(ui),text_(text) {}
    float SetLanguageID(std::int32_t id,float y)
    {
        const auto* localized=ui_.language->GetString(id);
        Require(localized,"Credits line has no authored translation");
        return SetText(localized,y,id);
    }
    float SetText(const char* source,float y,std::int32_t id=0,float scale=1.0f)
    {
        std::array<char,256> display{};std::uint32_t color=0,font=0;
        ParseCreditsColors(color,font,display.data(),display.size(),source);
        text_.SetColor(color?color:0x80808080u);
        if (font==hashString("FONT_IMPACTBIG") || font==hashString("FONT_IMPACTNORMAL")) {
            scale*=0.6000000238418579f;text_.fontKey=font;
        } else {
            text_.fontKey=hashString("FONT_STATS");
        }
        text_.SetScaleX(scale);text_.SetScaleY(scale);
        text_.updateKind=1;
        if (color || font || !id) {
            if (!id) {
                text_.SetLanguageID(0);
                const auto* atlas=ui_.resources->FindFont(text_.fontKey);
                text_.portAtlasUnderscore=atlas && !atlas->characters['_'] && atlas->characters['-'];
                bool missing=false;
                if (atlas)for(const auto* c=reinterpret_cast<const unsigned char*>(display.data());*c;++c)
                    if(!atlas->characters[*c] && !(*c=='_' && text_.portAtlasUnderscore)) {missing=true;break;}
                text_.visible=missing?0:1;
                const auto length=std::strlen(display.data());
                if(length>=text_.textCapacity) {
                    text_.textCapacity=static_cast<std::uint32_t>(length+1);
                    text_.ownedText.assign(length+2,0);
                    text_.text=text_.ownedText.data();
                }
            }
            text_.SetString(display.data());
        } else text_.SetLanguageID(static_cast<std::uint32_t>(id));
        text_.SetTopLeftY(y);text_.SetClipToPoly(0xa94bd818u);
        return ui_.resources->ScaledTextHeight(text_);
    }
    void PublishFallback(FlailerBitmapChunk& clip,std::vector<FrontendCreditsText>& result) const
    {
        if(text_.visible || !*text_.text)return;
        auto& resources=*ui_.resources;
        const auto width=resources.ScaledBitmapWidth(clip),height=resources.ScaledBitmapHeight(clip);
        const auto lineHeight=resources.ScaledTextHeight(text_);
        if(text_.topLeftY+lineHeight<=clip.topLeftY || text_.topLeftY>=clip.topLeftY+height)return;
        result.push_back({text_.text,text_.color,text_.topLeftX,text_.topLeftY,lineHeight,
            clip.topLeftX+4,clip.topLeftY,width-4,height});
    }
    void Animate(float elapsed)
    {
        float x,y;ui_.resources->GetTopLeft(text_,x,y,ui_.language);text_.SetTopLeftY(y-elapsed*30.0f);
    }
    bool PleaseResetMe() const
    {
        float x,y;ui_.resources->GetTopLeft(text_,x,y,ui_.language);return y<0.0f;
    }
    float Bottom() const
    {
        float x,y;ui_.resources->GetTopLeft(text_,x,y,ui_.language);return y+ui_.resources->ScaledTextHeight(text_);
    }
private:
    FlailerUiState& ui_;
    FlailerTextChunk& text_;
};
class tCreditsSlideshow final {
public:
    tCreditsSlideshow(FlailerUiState& ui,std::uint32_t screen,std::uint32_t first,std::uint32_t second)
        : ui_(ui),screen_(screen),first_(first),second_(second),started_(ui.clock->realTimer),
          current_(hashString("CREDITS_BG1")),secondAlpha_(ui,screen),firstAlpha_(ui,screen)
    {
        secondAlpha_.AddObject(second);firstAlpha_.AddObject(first);
        if (auto* object=ui_.resources->FindObject(screen_,second_))object->SetAlpha(0);
    }
    std::uint32_t PickNewTexture(std::uint32_t previous)
    {
        for (std::int32_t attempt=0;attempt<10;++attempt) {
            char name[32];std::snprintf(name,sizeof(name),"CREDITS_BG%u",vectorDefaultRandomBounded(8)+1);
            const auto key=hashString(name);if (key!=previous)return key;
        }
        return previous;
    }
    void Animate()
    {
        const auto raw=static_cast<std::uint32_t>(ui_.clock->realTimer)-static_cast<std::uint32_t>(started_);
        std::int32_t delta;std::memcpy(&delta,&raw,sizeof(delta));
        float elapsed=static_cast<float>(delta)*0.0002500000118743628f;
        if (choose_) {current_=PickNewTexture(current_);choose_=false;}
        if (!(elapsed>4.0f))return;
        if (elapsed>5.0f) {
            ChangeTexture(first_,current_);started_=ui_.clock->realTimer;choose_=true;
        } else {
            elapsed-=4.0f;ChangeTexture(second_,current_);
            secondAlpha_.SetAlphaScale(elapsed);firstAlpha_.SetAlphaScale(1.0f-elapsed);
        }
    }
private:
    void ChangeTexture(std::uint32_t object,std::uint32_t texture)
    {
        if (auto* target=ui_.resources->FindObject(screen_,object,2))
            static_cast<FlailerBitmapChunk*>(target)->ChangeTexture(texture);
    }
    FlailerUiState& ui_;
    std::uint32_t screen_,first_,second_;
    std::int32_t started_;
    std::uint32_t current_;
    bool choose_{true};
    FlailerAlphaAnimation secondAlpha_,firstAlpha_;
};
class CreditsScroller final {
    // Port Credits borrows the same art as EA and BBG. Restore its values
    // before the original scrollers can borrow it again.
    struct BorrowedArt {
        std::vector<std::pair<FlailerTextChunk*,FlailerTextChunk>> text;
        std::vector<std::pair<FlailerBitmapChunk*,FlailerBitmapChunk>> images;
        BorrowedArt(MainMenuScreenServices& menus,std::uint32_t screen,bool save)
        {
            if(!save)return;
            auto& art=*menus.ui->resources;
            auto* object=art.FindObjectByGroup(screen,0xd2638b19u,1);
            while(object) {
                auto* line=static_cast<FlailerTextChunk*>(object);
                text.emplace_back(line,*line);
                object=art.FindObjectByGroup(screen,0xd2638b19u,0,object);
            }
            for(const auto key:{0xf947e681u,0x4ef4d164u}) {
                if(auto* image=static_cast<FlailerBitmapChunk*>(art.FindObject(screen,key,2)))
                    images.emplace_back(image,*image);
            }
        }
        ~BorrowedArt()
        {
            for(auto& [target,saved]:text) {
                target->DeleteFlailerStrip();
                saved.next=target->next;saved.previous=target->previous;
                saved.strip=nullptr;saved.updateKind=10;
                *target=std::move(saved);
                target->text=target->ownedText.data();
            }
            for(auto& [target,saved]:images) {
                target->DeleteFlailerStrip();
                saved.next=target->next;saved.previous=target->previous;
                saved.strip=nullptr;saved.updateKind=10;
                *target=std::move(saved);
            }
        }
    };
public:
    CreditsScroller(MainMenuScreenServices& menus,std::uint32_t screen)
        : menus_(menus),screen_(screen),
          port_(std::strcmp(menus.screens->active->configuration->name,PortCreditsScreenName)==0?menus.credits:nullptr),
          borrowed_(menus,screen,port_!=nullptr),slideshow_(*menus.ui,screen,0xf947e681u,0x4ef4d164u)
    {
        Require(menus_.screens->active && menus_.screens->active->configuration,
            "Credits scroll has no actual active configuration");
        const auto* name=menus_.screens->active->configuration->name;
        BuildCredits(std::strcmp(name,"EACredits")==0?0:1);
        menus_.screens->StopMovie();
    }
    void BuildCredits(std::int32_t kind)
    {
        Require(kind==0 || kind==1,"Credits scroll has no authored credit range");
        first_=kind==0?501000:601000;last_=kind==0?501614:601226;next_=first_;
        auto& ui=*menus_.ui;
        auto* object=ui.resources->FindObjectByGroup(screen_,0xd2638b19u,1);
        while (object) {
            Require(object->kind==1,"Credits text group contains a non-text object");
            lines_.emplace_back(ui,static_cast<FlailerTextChunk&>(*object));
            object=ui.resources->FindObjectByGroup(screen_,0xd2638b19u,0,object);
        }
        Require(!lines_.empty(),"Credits screen has no authored line pool");
        if(port_)Require(port_->lines && port_->count,"Port Credits requires at least one line");
        float y=240.0f;
        for (auto& line:lines_)y+=NextLine(line,y);
    }
    float NextLine(CreditsLine& line,float y)
    {
        if(!port_) {
            const auto id=next_++;
            if (next_>last_)next_=first_;
            return line.SetLanguageID(id,y);
        }
        constexpr std::size_t gap=16;
        const auto index=portNext_++;
        if(portNext_==port_->count+gap)portNext_=0;
        const auto text=index<port_->count?port_->lines[index]:FrontendCreditsLine{""};
        return line.SetText(text.text,y,0,text.scale);
    }
    float GetMaxY() const
    {
        float result=-10.0f;for (const auto& line:lines_)if (result<line.Bottom())result=line.Bottom();return result;
    }
    void Tock(float elapsed)
    {
        for (auto& line:lines_) {
            if (line.PleaseResetMe()) {
                NextLine(line,GetMaxY());
            } else line.Animate(elapsed);
        }
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        if (message==ScreenMessage::Tick) {if (!paused_)Tock(menus_.ui->clock->realTimeElapsed);slideshow_.Animate();}
        else if (message==ScreenMessage::Left || message==ScreenMessage::Right ||
            message==ScreenMessage::Previous || message==ScreenMessage::Next) {data.sound=28;Tock(1.0f);}
        else if (message==ScreenMessage::ItemEvent) {data.sound=5;paused_=!paused_;}
        else if (message==ScreenMessage::Select)data.sound=0;
    }
    void PublishFallback(std::vector<FrontendCreditsText>& text) const
    {
        if(!port_)return;
        auto* clip=static_cast<FlailerBitmapChunk*>(menus_.ui->resources->FindObject(screen_,0xa94bd818u,2));
        if(clip)for(const auto& line:lines_)line.PublishFallback(*clip,text);
    }
private:
    MainMenuScreenServices& menus_;
    std::uint32_t screen_;
    std::vector<CreditsLine> lines_;
    std::int32_t first_{},last_{},next_{};
    bool paused_{};
    const FrontendCreditsContent* port_{};
    std::size_t portNext_{};
    BorrowedArt borrowed_;
    tCreditsSlideshow slideshow_;
};
std::unique_ptr<CreditsScreen> creditsOwner;
std::unique_ptr<CreditsMoviePlayer> creditsMovieOwner;
std::unique_ptr<CreditsScroller> creditsScrollOwner;
}

void CreditsScreenCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(creditsServices && data,"Credits callback has no actual bindings/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!creditsOwner,"Credits received duplicate activation");
        creditsOwner=std::make_unique<CreditsScreen>(CreditsMenus(),data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)creditsOwner.reset();
    else {Require(creditsOwner!=nullptr,"Credits callback precedes activation");creditsOwner->HandleEvent(message,*data);}
}
void CreditsMoviePlayerCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(creditsServices && data,"Credits movie callback has no actual bindings/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!creditsMovieOwner,"Credits movie received duplicate activation");
        const auto& menus=CreditsMenus();
        Require(menus.screens->active && menus.screens->active->configuration,
            "Credits movie has no actual active configuration");
        const auto* name=menus.screens->active->configuration->name;
        const bool french=menus.ui->language->currentLanguage==1;
        const char* movie=nullptr;const char* following=nullptr;
        if (std::strcmp(name,"Credits FE Movie")==0)
            movie=french?"MOVIES\\INTRO\\NFSINTRO_FR.AVI":"MOVIES\\INTRO\\NFSINTRO_EN.AVI";
        else if (std::strcmp(name,"Credits Reward movie")==0)
            movie=french?"MOVIES\\INTRO\\REWARD_FR.AVI":"MOVIES\\INTRO\\REWARD_EN.AVI";
        else if (std::strcmp(name,"BBG Creation movie")==0)movie="MOVIES\\INTRO\\MAKEOF_EN.AVI";
        else if (std::strcmp(name,"BBG Credits movie")==0) {
            movie="MOVIES\\INTRO\\BBGLOGO_EN.AVI";following="BBGCredits";
        }
        Require(movie,"Credits movie has no released configuration mapping");
        creditsMovieOwner=std::make_unique<CreditsMoviePlayer>(*creditsServices,data->key,movie,following);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        if (creditsMovieOwner)creditsMovieOwner->Retire();
        creditsMovieOwner.reset();
    } else {Require(creditsMovieOwner!=nullptr,"Credits movie callback precedes activation");creditsMovieOwner->HandleEvent(message,*data);}
}
void CreditsScrollerCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(creditsServices && data,"Credits scroll callback has no actual bindings/data");
    if (message==ScreenMessage::ScreenActivate) {
        Require(!creditsScrollOwner,"Credits scroll received duplicate activation");
        creditsScrollOwner=std::make_unique<CreditsScroller>(CreditsMenus(),data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)creditsScrollOwner.reset();
    else {Require(creditsScrollOwner!=nullptr,"Credits scroll callback precedes activation");creditsScrollOwner->HandleEvent(message,*data);}
}
void BindCreditsScreens(FrontendStartupServices& services)
{
    Require(!creditsOwner && !creditsMovieOwner && !creditsScrollOwner && services.menus && services.movieLog,
        "Credits binding lacks the actual frontend/movie replay owner");
    auto& menus=*services.menus;
    // Menu registration precedes the asynchronous FRONTB event publication.
    // Keep the live setup borrow here; ShouldShowRewardMovieOption validates
    // its actual event owner, both trees and event32 when Credits activates.
    Require(menus.ui && menus.ui->resources && menus.ui->language && menus.ui->clock && menus.ui->movie &&
        menus.ui->render && menus.screens && menus.screens->ui==menus.ui && menus.frontend && menus.sounds &&
        menus.raceSetup && menus.raceSetup->parameters && menus.raceSetup->flow,
        "Credits bindings do not share the actual application owners");
    creditsServices=&services;
}
void UnbindCreditsScreens()
{
    std::exception_ptr failure;
    try {if (creditsMovieOwner)creditsMovieOwner->Retire();}
    catch (...) {failure=std::current_exception();}
    // Do not retain static Flailer borrowers after one audio failure. Movie
    // retains its first error and pending voice for application shutdown.
    creditsMovieOwner.reset();creditsScrollOwner.reset();creditsOwner.reset();creditsServices=nullptr;
    if (failure)std::rethrow_exception(failure);
}
std::array<FlailerScreenConfig,7>& CreditsScreenConfigurations()
{
    static std::array<FlailerScreenConfig,7> configurations{{
        {CreditsNames[0],CreditsScreenCallback,{{static_cast<std::int32_t>(0xfd8a3b6du),0,0,0}},{{-1,-1,-1,-1}},1,CreditsHelp},
        {CreditsNames[1],CreditsMoviePlayerCallback,{{static_cast<std::int32_t>(0xb5761d00u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {CreditsNames[2],CreditsMoviePlayerCallback,{{static_cast<std::int32_t>(0xb5761d00u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {CreditsNames[3],CreditsMoviePlayerCallback,{{static_cast<std::int32_t>(0xb5761d00u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {CreditsNames[4],CreditsMoviePlayerCallback,{{static_cast<std::int32_t>(0xb5761d00u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr},
        {CreditsNames[5],CreditsScrollerCallback,{{static_cast<std::int32_t>(0x39c4c62cu),0,0,0}},{{-1,-1,-1,-1}},1,CreditsScrollHelp},
        {CreditsNames[6],CreditsScrollerCallback,{{static_cast<std::int32_t>(0x39c4c62cu),0,0,0}},{{-1,-1,-1,-1}},1,CreditsScrollHelp}
    }};
    return configurations;
}
FlailerScreenConfig& PortCreditsScreenConfiguration()
{
    static FlailerScreenConfig configuration{
        PortCreditsScreenName,CreditsScrollerCallback,{{static_cast<std::int32_t>(0x39c4c62cu),0,0,0}},
        {{-1,-1,-1,-1}},1,CreditsScrollHelp};
    return configuration;
}
void GetPortCreditsText(std::vector<FrontendCreditsText>& text)
{
    if(creditsScrollOwner)creditsScrollOwner->PublishFallback(text);
}
}

#include "FMenus.hpp"
#include "bsound/include/bSound.h"
#include "bsound/include/bRegion.h"
#include "FMenus.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "Flags.hpp"
#include "FEAnim.hpp"
#include "GameFlow.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"
#include <cstdio>
#include <cstring>
#include <utility>
#include <algorithm>
#include "FMenus.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"

namespace hp2 {

FlailerNewSongMonitor::FlailerNewSongMonitor()
    : screenKey(hashString("CHYRON"))
{
}

FlailerNewSongMonitor::~FlailerNewSongMonitor()
{
    alphaAnimation.reset();
    numberOfChanges=0;
}

void FlailerNewSongMonitor::Reset()
{
    alphaAnimation.reset();
}

// Independent released process cells PAL3305E0/3305F4.
std::int32_t newSongDisplayEnabled=1;
std::int32_t newSongTitleWidth=10;

void FlailerNewSongMonitor::Update(FlailerUiState& ui,const CarSoundBindings& sound)
{
    Require(ui.clock && ui.flow && ui.resources && ui.render,
        "Song monitor has no actual clock/flow/resource/render owners");
    if(soundJukeboxIsPlaying(sound,*ui.flow)) {
        const auto track=soundJukeboxGetCurrentTrack();
        if(track!=currentJukeboxIndex || !numberOfChanges) {
            currentJukeboxIndex=track;songChangedTimer=ui.clock->realTimer;
            ++numberOfChanges;songChangeStarted=0;
        }
    } else numberOfChanges=0;
    if(!songChangedTimer || songChangedTimer==INT32_MAX)return;
    const auto delta=static_cast<std::uint32_t>(ui.clock->realTimer)-static_cast<std::uint32_t>(songChangedTimer);
    std::int32_t ticks;std::memcpy(&ticks,&delta,sizeof(ticks));
    const float elapsed=static_cast<float>(ticks)*0.0002500000118743628f;
    if(elapsed>0.0f && elapsed<5.0f) {
        if(newSongDisplayEnabled)ui.render->SetScreenVisibility(*ui.resources,screenKey,0);
        if(!songChangeStarted) {
            const auto* title=soundJukeboxGetTrackName(sound);
            const auto* artist=soundJukeboxGetTrackArtist(sound);
            const auto* label=soundJukeboxGetTrackLabel(sound);
            Require(title && artist && label,"Active jukebox track lacks its authored title/artist/label metadata");
            auto& resources=*ui.resources;
            resources.Printf(screenKey,hashString("RECORD_LABEL"),"%s",label);
            resources.Printf(screenKey,hashString("ARTIST_NAME"),"%s",artist);
            resources.Printf(screenKey,hashString("SONG_TITLE"),"\"%s\"",title);
            songChangeStarted=1;
            // Exact direct GetWidth140588 dependency, borrowing the same
            // text/bitmap nodes and their existing width implementation.
            const auto width=[&](std::uint32_t key) {
                auto* object=resources.FindObject(screenKey,key);
                if(!object)return 0;
                if(object->kind==1)return convertFloatToInt32(resources.ScaledTextWidth(
                    *static_cast<FlailerTextChunk*>(object),ui.language));
                if(object->kind==2)return convertFloatToInt32(resources.ScaledBitmapWidth(
                    *static_cast<FlailerBitmapChunk*>(object)));
                return 0;
            };
            newSongTitleWidth=width(hashString("RECORD_TITLE"));
            auto maximum=std::max(width(hashString("RECORD_LABEL")),width(hashString("ARTIST_NAME")));
            maximum=std::max(maximum,width(hashString("SONG_TITLE")));
            maximum=std::max(maximum,newSongTitleWidth);
            for(std::int32_t index=1;index<=5;++index) {
                char name[32];std::snprintf(name,sizeof(name),"CHYRON_BG_%d",index);
                const auto key=hashString(name);const auto scaled=static_cast<float>(width(key));
                if(auto* object=resources.FindObject(screenKey,key)) {
                    // PAL divides the already-rounded current width by
                    // the actual current X scale before setting new scale.
                    const auto unscaled=scaled/object->scaleX;
                    object->SetScaleX((static_cast<float>(maximum)+20.0f)/unscaled);
                }
            }
        }
    } else ui.render->SetScreenVisibility(*ui.resources,screenKey,1);
    // PAL16C15C..16C1D8 retains comparisons for old fade intervals but no
    // writes or calls. Do not invent alpha-animation effects for that tail.
}

void FlailerScreenList::Tick()
{
    // PAL16CCA8: tick the live screen/help, both popup objects, movie,
    // original song monitor, frontend background, then loading animation.
    if(active)active->Tick();
    if(help)help->Tick();
    for(auto* popup:popups)if(popup)popup->Tick();
    UpdateMovie();
    Require(ui && ui->flow && soundBindings,"Menu tick has no actual UI/flow/sound bindings");
    newSongMonitor.Update(*ui,*soundBindings);
    if(ui->flow->currentState==3) {
        Require(background,"Frontend menu tick has no actual background animation");
        background->Tick();
    }
    Require(loadingAnimation,"Menu tick has no actual loading-screen animation");
    loadingAnimation->Tick();
}
void MenuSystemTick(FlailerScreenList* menuSystem)
{
    if(menuSystem)menuSystem->Tick();
}

void FlailerScreenList::StartMovie(const char* filename)
{
    // Native launcher option skips only the two startup title movies.
    // This is the shared menu/credits movie owner: suppressing it here also
    // suppresses Hot Pursuit, World Racing, option and credit films.
    // The title-screen owner handles the intro policy before opening them.
    Require(ui && ui->resources && ui->movie,"Movie screen has no actual UI/resources/player");
    Require(filename,"Movie screen filename is null");
    if (movieTextures[0].Data()) {
        if (movieFilename==filename)return;
        StopMovie();
    }
    static constexpr const char* textureNames[]={"PANELMOVIE","MOVIE_256_384_Y",
        "MOVIE_256_384_UA","MOVIE_256_384_VA","MOVIE_256_384_US","MOVIE_256_384_VS"};
    for (std::size_t index=0;index<movieTextures.size();++index) {
        auto* descriptor=ui->resources->FindTexture(hashString(textureNames[index]));
        auto* info=textureInfoForDescriptor(descriptor);
        if (index==0)Require(info,"Movie screen has no authored PANELMOVIE texture");
        movieTextures[index]=info ? info->pixels : TextureByteView{};
    }
    ui->movie->Close();
    movieFilename=filename;
    ui->movie->InitFile(movieFilename.c_str());
}

void FlailerScreenList::UpdateMovie()
{
    // PAL16D028 loops the source file for every return other than1.
    if (!movieTextures[0].Data())return;
    Require(ui && ui->movie && ui->flow && ui->clock,
        "Movie update has no actual player/flow/clock owners");
    // PAL16D044 reads independent InGameFlag330540, not desired flow330548.
    // Frontend desired/current remain3 while this owner keeps decoding.
    if (ui->flow->inGameFlag) {StopMovie();return;}
    auto& pixels=movieTextures[0];
    if (ui->movie->Update(ui->clock->realTimeElapsed,pixels.MutableData(),pixels.size)!=1)
        ui->movie->InitFile(movieFilename.c_str());
}

void FlailerScreenList::StopMovie()
{
    // PAL16D0A0 retains the filename but releases all six pixel references.
    if (!movieTextures[0].Data())return;
    Require(ui && ui->movie,"Movie stop has no actual player owner");
    ui->movie->Close();
    for (auto& pixels:movieTextures)pixels={};
}

const char* FlailerScreenList::GetDefaultMovieFilename() const noexcept
{
    return "MOVIES\\options_movie.AVI";
}

void FlailerScreenList::StartDefaultMovie()
{
    StartMovie(GetDefaultMovieFilename());
}

FlailerSelectionSet::FlailerSelectionSet()
    : items{},count(0),current(nullptr),mode(1),showOtherItems(0)
{
}

void FlailerSelectionSet::AddItem(FlailerObjectChunk* object)
{
    Require(count>=0 && static_cast<std::size_t>(count)<items.size(),
        "SelectionSet exceeds its actual 32-object extent");
    if (object) {
        object->visible=static_cast<std::uint8_t>(showOtherItems!=0);
        if (!showOtherItems)object->DeleteFlailerStrip();
    }
    items[static_cast<std::size_t>(count++)]=object;
}

void FlailerSelectionSet::AddItems(FlailerResources& resources,std::uint32_t screen,
    const std::uint32_t* keys,std::int32_t number)
{
    Require(number<=0 || keys,"SelectionSet has no authored item-key array");
    for (std::int32_t index=0;index<number;++index)AddItem(resources.FindObject(screen,keys[index]));
}

void FlailerSelectionSet::SetCurrentItem(std::uint32_t key)
{
    if (mode!=1)return;
    if (current) {
        current->visible=static_cast<std::uint8_t>(showOtherItems!=0);
        if (!showOtherItems)current->DeleteFlailerStrip();
    }
    current=nullptr;
    for (std::int32_t index=0;index<count;++index) {
        auto* item=items[static_cast<std::size_t>(index)];
        if (item && item->key==key) {current=item;break;}
    }
    if (current) {
        current->visible=static_cast<std::uint8_t>(showOtherItems==0);
        if (showOtherItems)current->DeleteFlailerStrip();
    }
}

std::int32_t FlailerMenu::GetCurrentItemNumber() const
{
    std::int32_t number=0;
    for (auto* item=root.next;item!=&root;item=item->next,++number)
        if (item==current) return number;
    return 0;
}

std::uint32_t FlailerMenu::GetActiveMenuItem() const
{
    return current ? current->objectKey : 0;
}

FlailerMenuItem* FlailerMenu::FindItem(std::uint32_t objectKey)
{
    for (auto* item=root.next;item!=&root;item=item->next)
        if (item->objectKey==objectKey) return item;
    return nullptr;
}

FlailerScreen::FlailerScreen(std::int32_t id,ScreenCallback handler,std::int32_t port)
    : menu(nullptr),currentItemNumber(0),joystickPort(port),screenId(id),callback(handler)
{
}

FlailerScreen* FlailerScreenNode::FindFlailerScreen(std::int32_t id)
{
    for (auto* candidate=screenRoot.next;candidate!=&screenRoot;candidate=candidate->next)
        if (candidate->screenId==id) return candidate;
    return nullptr;
}

bool FlailerScreenNode::IsPortFiltered(std::int32_t port) const
{
    for (auto* candidate=screenRoot.next;candidate!=&screenRoot;candidate=candidate->next)
        if (candidate->joystickPort==port || candidate->joystickPort==-1) return false;
    return true;
}

char* ScreenStack::Pop()
{
    if (!count) return nullptr;
    --count;
    return names[count].data();
}

void FlailerScreenList::SetMainScreen(const char* name)
{
    mainScreen=name;
}

FlailerScreenNode* FlailerScreenList::FindScreen(std::int32_t screenId)
{
    for (auto* node=root.next;node!=&root;node=node->next)
        if (node->configuration->screenIds[0]==screenId) return node;
    return nullptr;
}

bool FlailerScreenList::IsAnyPopupActive() const
{
    for (const auto* popup:popups) if (popup->activeState) return true;
    return false;
}

std::int32_t FlailerScreenList::GetPopupActiveState(std::int32_t index) const
{
    return popups[static_cast<std::size_t>(index)]->activeState;
}

FlailerPopupConfig* FlailerScreenList::GetPopupConfig(std::int32_t index) const
{
    return popups[static_cast<std::size_t>(index)]->configuration;
}

float FlailerSlider::ValueToDisplay(float value) const
{
    return value*displayScale;
}

float FlailerSlider::DisplayToValue(float value) const
{
    return finiteRatio(value,displayScale);
}

FlailerSlider::FlailerSlider(FlailerUiState& state,std::uint32_t screen,std::uint32_t menuItem,
    std::uint32_t bounds,std::uint32_t thumb,std::int32_t audioChannel,
    float* value,float minimum,float maximum)
    : ui(&state),screenKey(screen),menuItemKey(menuItem),boundsObjectKey(bounds),
      thumbObjectKey(thumb),channel(audioChannel),setting(value),
      minimumDisplay(minimum),maximumDisplay(maximum)
{
    Require(state.sound && state.sound->volumes && value,"Slider has no live sound settings/value owner");
    const auto& volumes=*state.sound->volumes;
    const float balance=GetVolumeBalance(volumes);
    const float maximumValue=GetVolumeDefault(volumes,channel);
    displayScale=finiteRatio(balance,maximumValue);
    tCubic1DInitialize(&position);
    position.duration=0.25f;position.continuous=0;
    position.desiredValue=ValueToDisplay(*setting);
    position.value=position.desiredValue;
    position.mode=0;position.derivative=position.desiredDerivative;
}

void FlailerSlider::StepValue(float step,bool updateMixer)
{
    Require(ui && ui->sound && ui->sound->volumes && ui->sound->manager && setting,
        "Slider change has no actual mixer/sound-settings/value owner");
    auto& sound=*ui->sound;
    const auto& manager=*sound.manager;
    float displayed=ValueToDisplay(*setting+step);
    if (maximumDisplay<displayed) displayed=maximumDisplay;
    if (displayed<minimumDisplay) displayed=minimumDisplay;
    const float value=DisplayToValue(displayed);
    const bool changed=!(value==*setting);
    *setting=value;
    if (updateMixer)
        Require(worldAudioSetChannelVolume(sound.audio,&manager,channel,value)==WORLD_AUDIO_OK,
            "Slider mixer update has no matching writable owner or backend command capacity");
    SaveVolumesForSpeech(sound);
    if (changed) PlayCommonRegionSample(sound,"MenuAdjustVolume",displayed);
    position.desiredValue=displayed;
    if (!(displayed==position.value)) position.mode=2;
}

void FlailerSlider::Animate()
{
    Require(ui && ui->clock && ui->resources,"Slider animation has no live clock/resource owner");
    tCubic1DUpdate(&position,ui->clock->realTimeElapsed,0.0f,0.0f);
    std::int32_t left=0,top=0,right=0,bottom=0;
    Require(ui->resources->GetBoundingBox(screenKey,boundsObjectKey,left,top,right,bottom,ui->language),
        "Slider bounds object is absent from its authored screen");
    const auto rawWidth=static_cast<std::uint32_t>(right)-static_cast<std::uint32_t>(left);
    std::int32_t width;std::memcpy(&width,&rawWidth,sizeof(width));
    const float x=static_cast<float>(left)+position.value*static_cast<float>(width);
    ui->resources->SetTopLeftX(screenKey,thumbObjectKey,convertFloatToInt32(x));
}

void FlailerUiState::SetIgnoreJoystick(std::int32_t ignored)
{
    ignoreJoystick=ignored;
}

void FlailerUiState::SetTwoPlayerScreenMode(std::int32_t enabled)
{
    twoPlayerScreenMode=enabled;
}

FlailerMenuItem::FlailerMenuItem(FlailerMenu* menu,std::uint32_t key,std::uint32_t number)
    : objectKey(key),ordinal(number)
{
    if (menu) { joystickPort=menu->joystickPort;screenKey=menu->screenKey;ui=menu->ui; }
    ResetSavedParameters();
}

FlailerMenuItem::~FlailerMenuItem()
{
    if (ui) {
        if (auto* text=ui->resources->FindObject(screenKey,objectKey,1))
            static_cast<FlailerTextChunk*>(text)->color=savedColor;
    }
}

void FlailerMenuItem::ResetSavedParameters()
{
    if (!ui) return;
    if (auto* text=ui->resources->FindObject(screenKey,objectKey,1))
        savedColor=static_cast<FlailerTextChunk*>(text)->color;
}

void FlailerMenuItem::Activate(ScreenCallback handler)
{
    if (!handler) return;
    ScreenCallbackData data{objectKey,joystickPort,0,0};
    handler(ScreenMessage::ItemActivate,&data);
    ui->sound->Play(data.sound);
}

void FlailerMenuItem::UnActivate(ScreenCallback handler)
{
    if (!handler) return;
    ScreenCallbackData data{objectKey,joystickPort,0,0};
    handler(ScreenMessage::ItemDeactivate,&data);
    ui->sound->Play(data.sound);
}

void FlailerMenuItem::Highlight(ScreenCallback handler,FlailerBitmapChunk*)
{
    SetMenuItemState(2);
    if (!handler) return;
    ScreenCallbackData data{objectKey,joystickPort,0,0};
    handler(ScreenMessage::Highlight,&data);
    ui->sound->Play(data.sound);
}

void FlailerMenuItem::UnHighlight(ScreenCallback handler,FlailerBitmapChunk*)
{
    ClearMenuItemState(2);
    if (!handler) return;
    ScreenCallbackData data{objectKey,joystickPort,0,0};
    handler(ScreenMessage::Unhighlight,&data);
    ui->sound->Play(data.sound);
}

void FlailerMenuItem::SetMenuItemState(std::uint32_t bits)
{
    flags|=bits;
    std::uint32_t color=savedColor;
    if ((flags&3u)!=1u) {
        const auto& rgb=(flags&3u)==3u ? ui->highlightedRgb : ui->inactiveRgb;
        color=rgb[0]|(rgb[1]<<8)|(rgb[2]<<16);
        if ((flags&3u)==3u) color|=0x80000000u;
    }
    ui->resources->SetTextColor(screenKey,objectKey,color);
    if (auto* text=ui->resources->FindObject(screenKey,objectKey,1)) text->updateKind=1;
}

void FlailerMenuItem::ClearMenuItemState(std::uint32_t bits)
{
    flags&=~bits;
    std::uint32_t color=savedColor;
    if ((flags&3u)!=1u) {
        const auto& rgb=(flags&3u)==3u ? ui->highlightedRgb : ui->inactiveRgb;
        color=rgb[0]|(rgb[1]<<8)|(rgb[2]<<16);
        if ((flags&3u)==3u) color|=0x80000000u;
    }
    ui->resources->SetTextColor(screenKey,objectKey,color);
    if (auto* text=ui->resources->FindObject(screenKey,objectKey,1)) text->updateKind=1;
}

void FlailerMenuItem::JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value,ScreenCallback handler)
{
    if (ui->ignoreJoystick || !value) return;
    ScreenMessage message;
    std::int32_t sound=0;
    switch (event) {
    case 5:case 18:message=ScreenMessage::Left;break;
    case 6:case 19:message=ScreenMessage::Right;break;
    case 7:case 9:message=ScreenMessage::Select;sound=5;break;
    case 17:message=ScreenMessage::ItemEvent;break;
    default:return;
    }
    ScreenCallbackData data{objectKey,port,event,sound};
    // PAL169EE4/169F44 may delete this item during the callback.
    // Its following MakeFrontendSound call uses the live application
    // sound owner, not the retired menu/item. Preserve that borrow.
    auto* const soundOwner=ui->sound;
    if (handler) handler(message,&data);
    soundOwner->Play(data.sound);
}

void FlailerMenu::SetCurrentItemNumber(std::int32_t number)
{
    auto* selected=root.next;
    std::int32_t index=0;
    while (selected!=&root && index!=number) { selected=selected->next;++index; }
    current=selected;
    // PAL16A104..16A118: an out-of-range index selects the first actual
    // item; only an empty ring produces nullptr. Never highlight its sentinel.
    if (selected==&root) {
        current=root.next;
        if (current==&root)current=nullptr;
    }
}

void FlailerMenu::Activate()
{
    for (auto* item=root.next;item!=&root;item=item->next) item->Activate(callback);
    if (!current) return;
    auto* chosen=current;
    while (!(chosen->flags&1u)) {
        auto* next=chosen->next;
        if (next==&root) next=root.next;
        if (next==current) break;
        chosen=next;
    }
    current=chosen;
    chosen->Highlight(callback,highlightBar);
}

void FlailerMenu::UnActivate()
{
    if (current) current->UnHighlight(callback,highlightBar);
    for (auto* item=root.next;item!=&root;item=item->next) item->UnActivate(callback);
}

void FlailerMenu::ResetCurrentMenuItem()
{
    if (!current) return;
    auto* original=current;
    auto* chosen=root.next;
    current=chosen;
    while (!(chosen->flags&1u)) {
        auto* next=chosen->next;
        if (next==&root) break;
        chosen=next;current=chosen;
    }
    if (chosen!=original) {
        original->UnHighlight(callback,highlightBar);
        current->Highlight(callback,highlightBar);
    }
}

void FlailerMenu::SetActiveMenuItem(std::uint32_t key)
{
    auto* item=FindItem(key);
    if (!item || item==current || !(item->flags&1u)) return;
    if (current) current->UnHighlight(callback,highlightBar);
    current=item;
    item->Highlight(callback,highlightBar);
}

void FlailerMenu::MoveToNextItem()
{
    if (!current) return;
    auto* original=current;
    auto* chosen=original->next;
    if (ui->wrapNavigation) {
        if (chosen==&root) chosen=root.next;
        while (chosen!=original && !(chosen->flags&1u)) {
            chosen=chosen->next;if (chosen==&root) chosen=root.next;
        }
    } else if (chosen==&root) chosen=original;
    if (chosen==original) return;
    original->UnHighlight(callback,highlightBar);
    current=chosen;
    chosen->Highlight(callback,highlightBar);
}

void FlailerMenu::MoveToPrevItem()
{
    if (!current) return;
    auto* original=current;
    auto* chosen=original->previous;
    if (ui->wrapNavigation) {
        if (chosen==&root) chosen=root.previous;
        while (chosen!=original && !(chosen->flags&1u)) {
            chosen=chosen->previous;if (chosen==&root) chosen=root.previous;
        }
    } else if (chosen==&root) chosen=original;
    if (chosen==original) return;
    original->UnHighlight(callback,highlightBar);
    current=chosen;
    chosen->Highlight(callback,highlightBar);
}

void FlailerMenu::JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value)
{
    if (ui->ignoreJoystick || !value) return;
    ScreenCallbackData data{current?current->objectKey:0,port,0,0};
    ScreenMessage message{};
    if (event==3 || event==20) { message=ScreenMessage::Previous;data.sound=3; }
    else if (event==4 || event==21) { message=ScreenMessage::Next;data.sound=4; }
    if (callback && static_cast<std::int32_t>(message)) callback(message,&data);
    if (!data.value) {
        if (message==ScreenMessage::Previous) { MoveToPrevItem();ui->sound->Play(data.sound); }
        else if (message==ScreenMessage::Next) { MoveToNextItem();ui->sound->Play(data.sound); }
    }
    if (current) current->JoyHandler(port,event,value,callback);
}

void FlailerMenu::Tick()
{
    // PAL's current-item tick target is JR RA/NOP. The call has no CPU effect.
}

void FlailerScreen::Activate()
{
    if (!menu) {
        ownedMenu=std::make_shared<FlailerMenuStorage>(*ui,*this,currentItemNumber);
        menu=&ownedMenu->Menu();
    }
    ScreenCallbackData data{static_cast<std::uint32_t>(screenId),joystickPort,-1,0};
    if (callback) callback(ScreenMessage::ScreenActivate,&data);
    if (data.value!=-1) currentItemNumber=data.value-1;
    ui->sound->Play(data.sound);
    menu->SetCurrentItemNumber(currentItemNumber);
    menu->Activate();
}

void FlailerScreen::UnActivate()
{
    ScreenCallbackData data{static_cast<std::uint32_t>(screenId),joystickPort,-1,0};
    if (menu) {
        currentItemNumber=menu->GetCurrentItemNumber();
        menu->UnActivate();
        ownedMenu.reset();menu=nullptr;
    }
    if (callback) callback(ScreenMessage::ScreenDeactivate,&data);
    ui->sound->Play(data.sound);
}

void FlailerScreen::JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value)
{
    if (ui->ignoreJoystick || (joystickPort!=-1 && joystickPort!=port) || !value) return;
    ScreenMessage message{};
    switch (event) {
    case 7:message=ScreenMessage::ScreenBack;break;
    case 8:message=ScreenMessage::ScreenEvent8;break;
    case 13:message=ScreenMessage::ScreenEvent13;break;
    case 15:message=ScreenMessage::ScreenEvent15;break;
    default:break;
    }
    if (static_cast<std::int32_t>(message)) {
        ScreenCallbackData data{static_cast<std::uint32_t>(screenId),port,0,0};
        if (callback) callback(message,&data);
        ui->sound->Play(data.sound);
        if (event==7 && data.value==1) return;
    }
    if (menu) menu->JoyHandler(port,event,value);
}

void FlailerScreen::Tick()
{
    if (menu) menu->Tick();
    if (callback) {
        ScreenCallbackData data{static_cast<std::uint32_t>(screenId),joystickPort,0,0};
        callback(ScreenMessage::Tick,&data);
        ui->sound->Play(data.sound);
    }
}

void FlailerScreen::SetActiveMenuItem(std::uint32_t key)
{
    if (menu) menu->SetActiveMenuItem(key);
    // PAL's MapMenuHashToIndex callee is folded here. Its individual original
    // definition path remains unknown; it adds no separately mapped body.
    currentItemNumber=0;
    for (std::int32_t index=0;index<16;++index) {
        char name[32];
        std::snprintf(name,sizeof(name),"MENUITEM_%d",index);
        if (hashString(name)==key) { currentItemNumber=index;break; }
    }
}

FlailerMenuStorage::FlailerMenuStorage(FlailerUiState& ui,FlailerScreen& screen,std::int32_t selected,
    const char* itemFormat)
{
    menu_.root.next=menu_.root.previous=&menu_.root;
    menu_.callback=screen.callback;menu_.ui=&ui;
    menu_.screenKey=static_cast<std::uint32_t>(screen.screenId);menu_.joystickPort=screen.joystickPort;
    for (std::uint32_t index=0;index<16;++index) {
        char name[128];
        const int written=std::snprintf(name,sizeof(name),itemFormat,index);
        Require(written>=0 && static_cast<std::size_t>(written)<sizeof(name),"Menu item name exceeds its native buffer");
        const auto key=hashString(name);
        auto* object=ui.resources->FindObject(menu_.screenKey,key,1);
        if (!object) continue;
        auto item=std::make_unique<FlailerMenuItem>(&menu_,key,index);
        item->object=object;
        item->previous=menu_.root.previous;item->next=&menu_.root;
        item->previous->next=item.get();menu_.root.previous=item.get();
        items_.push_back(std::move(item));
    }
    menu_.SetCurrentItemNumber(selected);
    menu_.highlightBar=static_cast<FlailerBitmapChunk*>(ui.resources->FindObject(menu_.screenKey,0x27c5f4abu,2));
}

FlailerMenuStorage::~FlailerMenuStorage()
{
    // Deleting the source list restores each item's original authored color
    // in forward order, before freeing its node. Native ownership is RAII.
    for (auto& item:items_) item.reset();
}

void FlailerSoundState::Play(std::int32_t event)
{
    if (!enabled || !audio || !clock || !(clock->ticksPerSecond>0.0f)) {
        result=WORLD_AUDIO_INVALID_BINDING;return;
    }
    if (!*enabled) return;
    if (event<0 || static_cast<std::size_t>(event)>=events.size()) {
        result=WORLD_AUDIO_INVALID_BINDING;return;
    }
    const auto voice=events[static_cast<std::size_t>(event)].voice;
    if (!voice.instance) return;
    if (lastVoice.instance==voice.instance) {
        const auto elapsed=static_cast<std::uint32_t>(clock->ticks-lastTicks);
        const float milliseconds=(static_cast<float>(elapsed)/clock->ticksPerSecond)*1000.0f;
        if (milliseconds<34.0f) return;
    }
    lastTicks=clock->ticks;
    result=worldAudioValidateVoice(audio,voice);
    if (result!=WORLD_AUDIO_OK) return;
    // PAL16E0B0 assigns the UI category before PLAY. StopFrontendSound
    // is a separate operation; it is not called on this path.
    result=worldAudioSetVoiceCategory(audio,voice,3);
    if (result!=WORLD_AUDIO_OK) return;
    result=worldAudioPlayVoice(audio,voice,1.0f,0.0f,0.0f);
    if (result==WORLD_AUDIO_OK) lastVoice=voice;
}

FlailerSoundState::FlailerSoundState()
{
    // PAL's authored event table. Entries sharing a bank cue share the same
    // live voice, as the original bank's case-insensitive lookup does.
    static constexpr const char* names[EventCount]={nullptr,
        "MenuLeftRight","MenuLeftRight","MenuUpDown","MenuUpDown","MenuSelect",
        "MenuCancel","MenuStart","MenuWrong","MenuWrong","MenuTrackRightLeft",
        "MenuTrackRightLeft","MenuCarRight","MenuCarLeft","MenuCarColour",
        "MenuCarDisplayOn","MenuCarDisplayActive","MenuCarDisplayOff","MenuCarSelect",
        "TreeUpDown","TreeLeftRight","TreeZoomInOut","TreeSelectEvent","TreeEventLocked",
        "RewardView","NameSelectText","NameDeleteText","NameCaseChange","NameKeyboard",
        "NameFinalSelect","EasterEgg"};
    for (std::size_t index=0;index<events.size();++index) {
        events[index].bankKind=index<10 ? 0 : 1;
        events[index].name=names[index];
    }
}

// PAL16DED8..16DF5C, called by InitFlailerMenuSystem1696C0 before
// constructing the menu screen list. Common owns these borrowed samples.
void InitGlobalMenuSound(FlailerSoundState& sounds,const CarSoundBindings& bindings)
{
    Require(bindings.globals && sounds.enabled==&bindings.globals->policy.audioSystemEnabled,
        "Global menu sound initialization has no canonical audio policy");
    if (!*sounds.enabled)return;
    Require(bindings.audio && sounds.audio==bindings.audio && bindings.audio->library,
        "Global menu sound initialization has no actual bSound owner");
    const auto* region=bindings.audio->library->CommonRegion();
    Require(region,"Global menu sounds require the completed Common region");
    Require(sounds.BindBank(0,&region->Bank())==WORLD_AUDIO_OK,
        "Global menu sound sample binding failed");
}

void InitFrontendSounds(FlailerSoundState& sounds,const CarSoundBindings& bindings)
{
    Require(bindings.globals && sounds.enabled==&bindings.globals->policy.audioSystemEnabled,
        "Frontend sound initialization has no canonical audio policy");
    if (!*sounds.enabled)return;
    Require(bindings.audio && sounds.audio==bindings.audio && bindings.audio->library,
        "Frontend sound initialization has no actual bSound owner");
    const auto* region=bindings.audio->library->Region("FrontEnd");
    Require(region,"Frontend sounds require the completed FrontEnd region");
    Require(sounds.BindBank(1,&region->Bank())==WORLD_AUDIO_OK,
        "Frontend sound sample binding failed");
}

void UnInitFrontendSounds(FlailerSoundState& sounds)
{
    // PAL16DFF8 only clears the frontend event references. The owning
    // bSound region performs voice retirement later in UnLoadSoundRegions.
    for (auto& event:sounds.events)if (event.bankKind==1)event.voice={};
}

WorldAudioResult FlailerSoundState::BindBank(std::int32_t kind,const WorldAudioBank* bank)
{
    if (!audio || !enabled || (kind!=0 && kind!=1)) return result=WORLD_AUDIO_INVALID_BINDING;
    if (!*enabled) return result=WORLD_AUDIO_OK;
    if (!bank || !audio->library || !audio->library->OwnsLoadedBank(bank) || !bank->name)
        return result=WORLD_AUDIO_INVALID_BINDING;
    const auto* region=audio->library->Region(bank->name);
    if (!region || &region->Bank()!=bank)return result=WORLD_AUDIO_INVALID_BINDING;
    // PAL16DFC0 borrows bRegion::Sample2719B8. These event-table entries
    // never own private copies: UnInitFrontendSounds only clears references,
    // and the same region retires its master samples before its bank UNLOAD.
    for (auto& event:events) {
        if (event.bankKind!=kind || !event.name) continue;
        event.voice=region->Sample(event.name);
        if (kind==0) {
            // PAL16DF34: the Common menu samples start in UI category3.
            result=worldAudioSetVoiceCategory(audio,event.voice,3);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    return result=WORLD_AUDIO_OK;
}

WorldAudioResult FlailerSoundState::ReleaseBank(std::int32_t kind)
{
    if (!audio || (kind!=0 && kind!=1)) return result=WORLD_AUDIO_INVALID_BINDING;
    for (auto& event:events) {
        if (event.bankKind!=kind || !event.voice.instance) continue;
        // Event-table entries borrow the region master; detaching this
        // table must not retire another caller's shared bank sample.
        if (lastVoice.instance==event.voice.instance && lastVoice.generation==event.voice.generation)
            lastVoice={};
        event.voice={};
    }
    return result=WORLD_AUDIO_OK;
}

void FlailerScreenNode::Activate()
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->Activate();
}

void FlailerScreenNode::UnActivate()
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->UnActivate();
}

void FlailerScreenNode::JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value)
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->JoyHandler(port,event,value);
}

void FlailerScreenNode::SetCallback(ScreenCallback handler)
{
    configuration->callback=handler;
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->callback=handler;
}

void FlailerScreenNode::SetFilterPort(std::int32_t port)
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->joystickPort=port;
}

void FlailerScreenNode::SetActiveMenuItem(std::uint32_t key)
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->SetActiveMenuItem(key);
}

void ScreenStack::Push(const char* name)
{
    Require(name,"Screen stack name has no owner");
    const auto length=std::strlen(name);
    Require(count<names.size() && length<NameCapacity,"Screen stack capacity exceeded");
    auto& target=names[count++];
    std::memcpy(target.data(),name,length+1);
}

FlailerScreenNode* FlailerScreenList::FindScreen(const char* name)
{
    for (auto* node=root.next;node!=&root;node=node->next)
        if (std::strcmp(node->configuration->name,name)==0) return node;
    return nullptr;
}

void FlailerScreenList::PopScreens(std::int32_t number)
{
    while (number>0) { stack.Pop();--number; }
}

const char* FlailerScreenList::GetCurrentScreenNameFromStack() const
{
    Require(stack.count<=stack.names.size(),"Screen stack exceeds its actual source extent");
    return stack.count ? stack.names[stack.count-1].data() : "";
}

void FlailerScreenList::PopUntil(const char* name)
{
    Require(name,"Screen stack target name is null");
    for (;;) {
        const auto* current=GetCurrentScreenNameFromStack();
        if (!*current || std::strcmp(current,name)==0)return;
        stack.Pop();
    }
}

FlailerScreenStorage::FlailerScreenStorage(FlailerUiState& ui,FlailerScreenConfig& configuration)
{
    Require(configuration.screenCount>=0 && configuration.screenCount<=4,
        "Screen configuration exceeds its authored screen slots");
    node_.configuration=&configuration;
    node_.screenRoot.next=node_.screenRoot.previous=&node_.screenRoot;
    for (std::int32_t index=0;index<configuration.screenCount;++index) {
        const auto slot=static_cast<std::size_t>(index);
        auto screen=std::make_unique<FlailerScreen>(configuration.screenIds[slot],
            configuration.callback,configuration.joystickPorts[slot]);
        screen->ui=&ui;
        screen->previous=node_.screenRoot.previous;screen->next=&node_.screenRoot;
        screen->previous->next=screen.get();node_.screenRoot.previous=screen.get();
        if (!node_.screen) node_.screen=screen.get();
        screens_.push_back(std::move(screen));
    }
}

FlailerScreenStorage::~FlailerScreenStorage()
{
    // Source node disposal deletes children in forward registration order.
    // A child menu, if present, restores its original colors before release.
    for (auto& screen:screens_) screen.reset();
}

void FlailerScreenList::SetHelpOverride(const std::int32_t* items)
{
    // Retail PAL16D448 stores the list then calls EnableHelp1E3DC0.
    // The beta wrapper lacks the latter side effect; use the retail contract.
    Require(ui,"Help override has no live frontend UI owner");
    overrideHelpItems=items;
    ui->helpEnabled=1;
}

bool FlailerScreenList::MakeHelpGo(const std::int32_t* items,std::int32_t state)
{
    if (state==1) {
        if (!help->IsActive()) help->Summon(items);
        else help->Dismiss();
    }
    return help->IsActive();
}

FlailerPopup::FlailerPopup()=default;

FlailerPopup::FlailerPopup(FlailerUiState& state):ui(&state)
{
    Require(state.clock,"Popup has no live UI clock");
    highlight=std::make_unique<FlailerHighlightAnimation>(state,0,0xab671c9au,128);
    for (std::uint32_t index=0;index<buttons.size();++index) {
        buttons[index]=std::make_unique<FlailerMenuItem>(nullptr,0xe0105663u+index,index);
        buttons[index]->ui=&state;
    }
}

FlailerPopup::~FlailerPopup()=default;

FlailerMenuItem* FlailerPopup::GetCurrentMenuItem()
{
    const auto count=static_cast<std::uint32_t>(configuration->buttonCount);
    if (!count) return buttons[0].get();
    Require(count<=buttons.size(),"Popup button count exceeds its actual three owners");
    const auto selected=static_cast<std::uint32_t>(selectedIndex);
    const auto index=((selected%count)+count)%count;
    return buttons[index].get();
}

void FlailerPopup::UnHighlightCurrentMenuItem()
{
    GetCurrentMenuItem()->UnHighlight(nullptr,nullptr);
}

void FlailerPopup::HighlightCurrentMenuItem()
{
    auto* item=GetCurrentMenuItem();
    if (configuration->buttonCount) item->Highlight(nullptr,nullptr);
}

void FlailerPopup::Summon(const FlailerPopupConfig& parameters)
{
    Require(ui && highlight && parameters.sourceNode && parameters.sourceNode->configuration &&
        parameters.sourceNode->configuration->callback && parameters.buttonCount>=0 && parameters.buttonCount<=3,
        "Popup configuration has no concrete screen callback or button owners");
    activeState=1;ownedConfiguration=parameters;configuration=&ownedConfiguration;
    callback=configuration->sourceNode->configuration->callback;
    for (std::size_t index=buttons.size();index--;) buttons[index]->ClearMenuItemState(2);
    ui->render->SetScreenVisibility(*ui->resources,configuration->screenKey,0);
    ScreenCallbackData data{0,-1,0,0};
    callback(ScreenMessage::PopupActivate,&data);ui->sound->Play(data.sound);
    selectedIndex=configuration->initialButton;
    for (auto& item:buttons) {
        item->screenKey=configuration->screenKey;item->joystickPort=configuration->joystickPort;
        item->ResetSavedParameters();item->ClearMenuItemState(2);
    }
    HighlightCurrentMenuItem();
    const auto bottom=ui->resources->RenderTextIntoGroup(configuration->screenKey,0x5d6dd123u,configuration->message)+10;
    if (configuration->flags&4u)
        for (const auto& item:buttons)
            if (auto* object=ui->resources->FindObject(configuration->screenKey,item->objectKey))
                object->SetTopLeftY(static_cast<float>(bottom));
    for (const auto& item:buttons) ui->resources->Printf(configuration->screenKey,item->objectKey,"");
    ui->resources->Printf(configuration->screenKey,0x85eda094u,"");
    for (std::size_t index=0;index<buttons.size();++index)
        ui->resources->SetLanguageID(configuration->screenKey,buttons[index]->objectKey,
            configuration->buttonCount ? configuration->buttonLanguageIds[index] : 0);
    ui->resources->SetLanguageID(configuration->screenKey,0x85eda094u,configuration->footerLanguageId);
    // AlignArrows16BAB8 is an actual JR RA/NOP method in final PAL.
    highlight->screenKey=configuration->screenKey;
    highlight->SetCurrentMenuItem(GetCurrentMenuItem()->objectKey);
}

void FlailerPopup::Dismiss(std::int32_t port,std::int32_t result)
{
    if (!(configuration->flags&16u))
        ui->render->SetScreenVisibility(*ui->resources,configuration->screenKey,1);
    activeState=0;
    ScreenCallbackData data{0,port,result,5};
    callback(ScreenMessage::PopupDismiss,&data);ui->sound->Play(data.sound);
}

void FlailerPopup::Tick()
{
    phase+=ui->clock->realTimeElapsed*9.0f;
    if (phase>6.283185005187988f) phase-=6.283185005187988f;
    // Source computes this argument even though Animate never reads it.
    (void)rotationSineRadians(phase);
    highlight->Animate();
}

bool FlailerPopup::JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value)
{
    if (value!=1) return true;
    const auto flags=configuration->flags;
    if (flags&1u) {
        if (event==3 || event==4) event=0;
        if (event==5 || event==18) event=3;
        if (event==6 || event==19) event=4;
    } else if (event==5 || event==6) event=0;
    if (event==3 || event==4) {
        if (configuration->buttonCount>=2) {
            UnHighlightCurrentMenuItem();
            if (event==3) {
                --selectedIndex;if (selectedIndex<0) selectedIndex=configuration->buttonCount-1;
            } else {
                ++selectedIndex;if (selectedIndex>=configuration->buttonCount) selectedIndex=0;
            }
            HighlightCurrentMenuItem();
            if (callback) {
                ScreenCallbackData data{GetCurrentMenuItem()->objectKey,port,0,event==3?1:2};
                callback(event==3?ScreenMessage::PopupPrevious:ScreenMessage::PopupNext,&data);
                ui->sound->Play(data.sound);
            }
            highlight->SetCurrentMenuItem(GetCurrentMenuItem()->objectKey);
        }
    } else if (event==7 || event==9) {
        auto result=configuration->buttonResults[0];
        if (configuration->buttonCount) {
            const auto index=static_cast<std::uint32_t>(selectedIndex)%static_cast<std::uint32_t>(configuration->buttonCount);
            result=configuration->buttonResults[index==0?0:index==1?1:2];
        }
        if (!(flags&8u)) Dismiss(port,result);
        else if (flags&2u) Dismiss(port,configuration->cancelResult);
    } else if (event==11 && (flags&2u)) Dismiss(port,configuration->cancelResult);
    return true;
}

std::int32_t FlailerScreenList::SummonPopup(const FlailerPopupConfig& parameters)
{
    for (std::size_t index=0;index<popups.size();++index) {
        Require(popups[index],"Screen list has no popup owner");
        if (!popups[index]->activeState) {
            popups[index]->Summon(parameters);return static_cast<std::int32_t>(index);
        }
    }
    return -1;
}

void FlailerScreenList::DismissPopup(std::int32_t index)
{
    if (static_cast<std::uint32_t>(index)>=popups.size()) return;
    auto* popup=popups[static_cast<std::size_t>(index)];
    Require(popup,"Screen list has no popup owner");
    if (popup->activeState) popup->Dismiss(popup->configuration->joystickPort,popup->configuration->dismissResult);
}

void FlailerScreenNode::SetVisibility(std::int32_t visibility)
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next)
        child->ui->render->SetScreenVisibility(*child->ui->resources,static_cast<std::uint32_t>(child->screenId),visibility);
}

void FlailerScreenList::ChangeToScreen(const char* name,bool push)
{
    Require(ui && background && input && input->router && input->screens==this,
        "Screen list transition has no live UI/background/input owner");
    // Native exit animations own their destination string. UnActivate can
    // destroy that animation and its string before FindScreen below. Retain
    // the requested name through both lookup and the main-screen stack check.
    // PAL names are stable authored strings; keep the same lookup/order.
    const std::string destination=name ? name : "";
    if (name)name=destination.c_str();
    auto* old=active;
    if (old) {
        old->SetVisibility(1);
        if (push) stack.Push(active->configuration->name);
        active->UnActivate();active=nullptr;
    }
    if (name) {
        if (auto* found=FindScreen(name)) {
            active=found;found->SetVisibility(0);active->Activate();
            background->ChangeScreenHash(static_cast<std::uint32_t>(found->configuration->screenIds[0]));
        }
    } else background->ChangeScreenHash(0);
    // A missing non-null name deliberately leaves the previous background
    // selection intact, as the source branch does.
    if (active) { if (!old) AddNormalMenuJoyHandlers(); }
    else if (old) RemoveNormalMenuJoyHandlers();
    if (name && mainScreen && std::strcmp(name,mainScreen)==0) stack.count=0;
}

bool FlailerScreenList::MoveToPreviousScreen(std::int32_t count,bool consultCallback,std::int32_t port)
{
    ScreenCallbackData data{0,port,1,6};
    if (consultCallback) {
        Require(active && active->configuration,"Previous-screen transition has no active configuration");
        if (active->configuration->callback) active->configuration->callback(ScreenMessage::LeaveScreen,&data);
        ui->sound->Play(data.sound);
    }
    if (data.value!=1) return false;
    char* name=nullptr;
    while (count>0) { name=stack.Pop();--count; }
    Require(name,"Previous-screen transition has no valid stack entry");
    if (!*name) return false;
    ChangeToScreen(name,false);return true;
}

void FlailerScreenList::AddNormalMenuJoyHandlers()
{
    static constexpr std::int32_t events[16]={7,8,3,4,5,6,9,11,12,13,15,17,18,19,21,20};
    Require(input && input->router && input->screens==this,"Frontend input has no concrete list owner");
    GameInputTarget target{};target.kind=GAME_INPUT_FRONTEND;target.owner.frontend=input;
    for (auto event:events) for (std::int32_t port=0;port<2;++port)
        Require(gameInputRegister(input->router,port,event,target)==GAME_INPUT_OK,
            "Frontend input registrations exceed their live router capacity");
}

void FlailerScreenList::RemoveNormalMenuJoyHandlers()
{
    static constexpr std::int32_t events[16]={7,8,3,4,5,6,9,11,12,13,15,17,18,19,21,20};
    Require(input && input->router,"Frontend input has no router owner");
    for (auto event:events) for (std::int32_t port=0;port<2;++port) gameInputUnregister(input->router,port,event);
}

void FlailerScreenList::JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value)
{
    Require(ui && ui->clock && help,"Frontend input has no live clock/help owner");
    if (value) {
        const auto index=ui->twoPlayerScreenMode ? port : 0;
        Require(index>=0 && index<2,"Frontend input port exceeds its actual two seats");
        auto& previous=ui->lastInputLoop[static_cast<std::size_t>(index)];
        if (previous==ui->clock->realLoopCounter) return;
        previous=ui->clock->realLoopCounter;
    }
    if (event==12 && !ui->ignoreJoystick && !IsAnyPopupActive()) {
        Require(active,"Help input has no active screen");
        if (!active->IsPortFiltered(port))
            MakeHelpGo(overrideHelpItems ? overrideHelpItems : active->configuration->helpItems,value);
    }
    if (help->IsActive()) return;
    bool popupHandled=false;
    for (auto* popup:popups) {
        Require(popup,"Frontend input has no popup owner");
        if (popup->activeState && (popup->configuration->joystickPort==port || popup->configuration->joystickPort==-1)) {
            popup->JoyHandler(port,event,value);popupHandled=true;
        }
    }
    if (popupHandled) return;
    Require(active && active->configuration,"Frontend input has no active screen configuration");
    if (active->configuration->callback) {
        ScreenCallbackData data{0,port,0,0};active->configuration->callback(ScreenMessage::Input,&data);
    }
    if (event==11 && !active->IsPortFiltered(port)) MoveToPreviousScreen(1,true,port);
    else active->JoyHandler(port,event,value);
}

void FlailerScreen::RenderModels(bool advanceState)
{
    if (callback) {
        Require(ui && ui->sound,"Frontend model callback has no UI/sound owner");
        struct RestoreRenderContext {
            FlailerUiState& state;bool previous;
            ~RestoreRenderContext(){state.advancePresentation=previous;}
        } restore{*ui,ui->advancePresentation};
        const bool advance=advanceState && ui->advancePresentation;
        ui->advancePresentation=advance;
        ScreenCallbackData data{static_cast<std::uint32_t>(screenId),joystickPort,0,0};
        callback(ScreenMessage::RenderModels,&data);
        if(advance)ui->sound->Play(data.sound);
    }
}

void FlailerScreenNode::Tick()
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->Tick();
}

void FlailerScreenNode::RenderModels(bool advanceState)
{
    for (auto* child=screenRoot.next;child!=&screenRoot;child=child->next) child->RenderModels(advanceState);
}

std::uint32_t FlailerScreenNode::GetActiveMenuItem() const
{
    const auto* first=screenRoot.next;
    if (first==&screenRoot || !first->menu) return 0;
    return first->menu->GetActiveMenuItem();
}

void FlailerScreenList::RenderModels(bool advanceState)
{
    if (active) active->RenderModels(advanceState);
}

} // namespace hp2

extern "C" GameInputResult flailerFrontendDispatchInput(FlailerFrontendInput* input,
    std::int32_t port,std::int32_t event,std::uint8_t value)
{
    if (!input || !input->screens || input->screens->input!=input) return GAME_INPUT_INVALID_BINDING;
    try { input->screens->JoyHandler(port,event,value);return GAME_INPUT_OK; }
    catch (const std::exception&) { return GAME_INPUT_OWNER_FAILURE; }
}

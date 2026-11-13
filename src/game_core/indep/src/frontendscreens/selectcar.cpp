#include "Fred.hpp"
#include "FMenus.hpp"
#include "CarLoader.hpp"
#include "EasterEggs.hpp"
#include "Flags.hpp"
#include "Language.hpp"
#include "eLight.hpp"
#include "AcidFX.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <list>
#include <cstring>

namespace hp2 {

namespace {
// PAL GP-4E88/-4E84 are independent preview model IDs, initialized-1.
// FRED selected rides and the actual CarLoader remain their own owners.
std::array<std::int32_t,2> currentSelectedCarPreloads{{-1,-1}};

// The five independent PAL GP-4E78..-4E68 cells. These buffers are the
// source spotlight's saved channels, not another texture resource owner.
TextureInfo* spotlightTexture{};
std::vector<std::uint8_t> spotlightAlpha;
std::vector<std::uint8_t> spotlightColour;
std::int32_t spotlightPixelCount{};
bool spotlightsInitialized{};

void selectSkin(VisibleCar& car,std::int32_t number)
{
    const auto& available=*car.availability;
    Require(available.skinCount>=0 && static_cast<std::size_t>(available.skinCount)<=available.skinNumbers.size(),
        "Visible car has an invalid authored skin-choice extent");
    for (std::int32_t index=0;index<available.skinCount;++index)
        if (available.skinNumbers[static_cast<std::size_t>(index)]==number) {car.skinIndex=index;return;}
}

// Complete1944E8 ctor/194728 ordering dependency for the actual preload
// producer. Nodes borrow the SAME profile availability and64-car resource.
struct ProfileVisibleCars final {
    VisibleCarSet set{};
    std::list<VisibleCar> nodes;
    ProfileVisibleCars(FrontendPlayerInfo& profile,const FrontendPlayerManager& frontend,bool pursuit)
    {
        Require(profile.cars && profile.language,"Visible car selection has no actual car/language resources");
        set.root.next=set.root.previous=&set.root;
        const auto* selected=profile.GetSelectedRideInfo(frontend.gameType);
        Require(selected!=nullptr,"Visible car selection has no selected profile ride");
        for (std::int32_t model=0;model<64;++model) {
            const auto& availability=profile.availability[static_cast<std::size_t>(model)];
            // Both authored rewards and the explicit PC traffic option use
            // this same profile availability owner. An unrelated beta
            // all-cars flag cannot expose unavailable resource entries.
            if (availability.status==3)continue;
            nodes.emplace_back();auto& node=nodes.back();
            node.definition=&profile.cars->Car(static_cast<std::size_t>(model));node.availability=&availability;
            node.selectionOrder=static_cast<std::int32_t>(nodes.size()-1);
            node.thumbnailKey=hashString(("THUMBNAIL_"+node.definition->name).c_str());
            selectSkin(node,node.definition->defaultSkinNumber);
            node.selectionFlag=!availability.status || easterEggs.IsCarUnlocked(static_cast<std::uint32_t>(model),pursuit)!=0;
            if (selected->appearance.modelCode==model) {set.current=&node;selectSkin(node,selected->appearance.skinNumber);}
        }
        const auto score=[](const VisibleCar& car) {
            const auto& info=*car.definition;
            std::uint32_t value=car.selectionFlag ? 0u : 0x40000000u;
            if (info.bodyCategory==1)value+=0x01000000u;
            value+=static_cast<std::uint32_t>(info.selectionSortOrder)<<16;
            value+=static_cast<std::uint32_t>(5-static_cast<std::int32_t>(info.routeVehicleClass))<<8;
            return value;
        };
        nodes.sort([&](const VisibleCar& a,const VisibleCar& b) {
            const auto first=score(a),second=score(b);
            if (first!=second)return first<second;
            return std::strcmp(a.definition->GetFullName(*profile.language),b.definition->GetFullName(*profile.language))<0;
        });
        std::int32_t order=0;
        for (auto& node:nodes) {
            node.selectionOrder=order++;node.previous=set.root.previous;node.next=&set.root;
            set.root.previous->next=&node;set.root.previous=&node;
        }
        if (!set.current)set.current=set.root.next;
    }
};
}

void CarSelector::InitSpotlights(ResourceRegistry& resources)
{
    spotlightTexture=textureInfoForDescriptor(resources.ResolveTexture(hashString("BRUSHEDMETAL"),false));
    if (spotlightsInitialized || !spotlightTexture)return;
    Require(spotlightTexture->pixelBytes>=0,"Spotlight texture has a negative pixel extent");
    spotlightPixelCount=spotlightTexture->pixelBytes/4;
    spotlightAlpha.resize(static_cast<std::size_t>(spotlightPixelCount));
    spotlightTexture->GetAlphaChannel(spotlightAlpha.data(),spotlightAlpha.size());
    spotlightColour.resize(static_cast<std::size_t>(spotlightPixelCount)*3);
    spotlightTexture->GetColourChannel(spotlightColour.data(),spotlightColour.size());
    spotlightsInitialized=true;
}

void CarSelector::CloseSpotlights()
{
    std::vector<std::uint8_t>().swap(spotlightAlpha);
    if (!spotlightColour.empty()) {
        std::vector<std::uint8_t>().swap(spotlightColour);
        spotlightPixelCount=0;
    }
    spotlightTexture=nullptr;
    // PAL195088 does not clear GP-4E68. Preserve that source lifetime cell.
}

void CloseSelectCarScreen()
{
    UnloadCurrentSelectedCars();
    CarSelector::CloseSpotlights();
}

void UnloadCurrentSelectedCars()
{
    for (auto& model:currentSelectedCarPreloads)if (model!=-1) {
        CarAppearanceSelection ride{};InitializeRideInfo(ride,model,-1,0,0);
        carLoader.Unload(ride);model=-1;
    }
}

void PreloadCurrentSelectedCars(FrontendPlayerManager& frontend)
{
    // Terminal teardown cannot turn the selector's release into a new load.
    if (carLoader.ShuttingDown())return;
    if (frontend.playerCount<=0)return;
    Require(frontend.playerCount<=2,"Preview preload exceeds the source's two player seats");
    for (std::int32_t index=0;index<frontend.playerCount;++index) {
        auto* profile=frontend.players[static_cast<std::size_t>(index)];
        Require(profile!=nullptr,"Preview preload has no actual frontend player profile");
        ProfileVisibleCars visible(*profile,frontend,index==0 && frontend.gameType==7);
        Require(visible.set.current && visible.set.current!=&visible.set.root,
            "Preview preload has no actual visible car");
        const auto selected=visible.set.current->definition->modelCode;
        auto& previous=currentSelectedCarPreloads[static_cast<std::size_t>(index)];
        // A prior selector can retire its preview while a selected-car
        // preload still remembers its model number. Reentering Select Car
        // must not trust a cached model ID unless its actual resource owner
        // still retains a skin and geometry request.
        if (previous==selected) {
            CarAppearanceSelection cached{};
            InitializeRideInfo(cached,selected,-1,0,0);
            if (carLoader.IsLoaded(cached,true))continue;
            previous=-1;
        }
        CarAppearanceSelection ride{};
        if (previous!=-1) {InitializeRideInfo(ride,previous,-1,0,0);carLoader.Unload(ride);}
        previous=selected;carLoader.frontendTextures=1;carLoader.singleSkinBatch=true;
        InitializeRideInfo(ride,selected,-1,0,0);(void)carLoader.Load(ride);carLoader.BeginLoading();
    }
}

std::int32_t VisibleCar::NextSkin()
{
    ++skinIndex;
    if (skinIndex>=availability->skinCount) skinIndex=0;
    return skinIndex;
}

std::int32_t VisibleCar::PrevSkin()
{
    --skinIndex;
    if (skinIndex<0) skinIndex=availability->skinCount-1;
    return skinIndex;
}

VisibleCar* VisibleCarSet::GetNextCar(VisibleCar* selected) const
{
    return selected->next==&root ? selected : selected->next;
}

VisibleCar* VisibleCarSet::GetPrevCar(VisibleCar* selected) const
{
    return selected->previous==&root ? selected : selected->previous;
}

void SelectCarScreen::SetSelectedCars()
{
    Require(frontend && frontend->playerCount>=0 && frontend->playerCount<=2,
        "Car selection has no live frontend player manager");
    for (std::int32_t index=0;index<frontend->playerCount;++index) {
        auto* selector=selectors[static_cast<std::size_t>(index)];
        if (!selector) continue;
        auto* profile=frontend->players[static_cast<std::size_t>(index)];
        auto* visible=selector->visibleCars.current;
        Require(profile && visible && visible->definition && visible->availability && visible->skinIndex>=0 &&
            visible->skinIndex<visible->availability->skinCount &&
            static_cast<std::size_t>(visible->skinIndex)<visible->availability->skinNumbers.size(),
            "Car selector has no valid current vehicle/profile skin");
        profile->AssignCarOfType(frontend->gameType,visible->definition->modelCode,
            visible->availability->skinNumbers[static_cast<std::size_t>(visible->skinIndex)],true);
        if (selector->firstChoice>=0 && selector->firstChoice<=2)
            profile->settings.gearInputMode=static_cast<std::int8_t>(selector->firstChoice);
        if (selector->secondChoice==0 || selector->secondChoice==1)
            profile->settings.initialVehicleMode=static_cast<std::int8_t>(selector->secondChoice);
    }
}

void SelectCarScreen::DismissAllPopups()
{
    Require(selectorCount>=0 && selectorCount<=2,"Car selector count exceeds its actual two seats");
    for (std::int32_t index=0;index<selectorCount;++index) {
        auto* selector=selectors[static_cast<std::size_t>(index)];
        Require(selector && selector->screens,"Car selector has no popup screen-list owner");
        // The complete196D48 callee is an index load plus a tail call to the
        // same global FlailerScreenList. No extra wrapper is introduced.
        selector->screens->DismissPopup(selector->popupIndex);
    }
}

bool SelectCarScreen::AreAllSelectorsReady() const
{
    Require(selectorCount>=0 && selectorCount<=2,"Car selector count exceeds its actual two seats");
    for (std::int32_t index=0;index<selectorCount;++index) {
        const auto* selector=selectors[static_cast<std::size_t>(index)];
        Require(selector,"Car selector readiness has no concrete owner");
        if (selector->selectionState!=(selector->splitScreen?3:2)) return false;
    }
    return true;
}

} // namespace hp2

#include "FEAnim.hpp"
#include "FlailerDB.hpp"
#include "CameraMover.hpp"
#include "CarSounds.hpp"
#include "GameFlow.hpp"
#include "FMenus.hpp"
#include "bware/inc/bMath.hpp"
#include "eMath.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include <algorithm>
#include <cstdio>
#include <utility>

namespace hp2 {
namespace {
SelectCarScreenServices* selectCarServices{};
std::unique_ptr<SelectCarScreen> selectCarScreen;
constexpr std::int32_t carSelectHelp[]{800018,800019,800020,800021,800022,0};
constexpr std::int32_t carSelectTwoPlayerHelp[]{802000,802001,802002,802003,0};
constexpr std::int32_t copSelectHelp[]{800701,800702,800703,800704,0};
constexpr std::int32_t copSelectTwoPlayerHelp[]{800705,800706,800707,800708,0};
constexpr std::int32_t carOrbitHelp[]{800023,800024,800027,0};
constexpr std::int32_t eventEightHelp[]{800039,800040,800401,800041,800042,0};
constexpr std::int32_t eventNineHelp[]{800053,800054,800400,800055,800056,0};
constexpr std::int32_t eventEightOrbitHelp[]{802004,802005,802008,0};
constexpr std::int32_t eventNineOrbitHelp[]{802010,802011,802014,0};

std::int32_t SelectCarTickDifference(std::int32_t now,std::int32_t then)
{
    const auto delta=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(then);
    std::int32_t result;std::memcpy(&result,&delta,sizeof(result));return result;
}
std::uint32_t SelectCarIndexedKey(const char* format,std::int32_t index)
{
    char name[64];const auto written=std::snprintf(name,sizeof(name),format,index);
    Require(written>=0 && static_cast<std::size_t>(written)<sizeof(name),"Car selector key exceeds source scratch capacity");
    return hashString(name);
}
void FadeCarPreview(float duration)
{
    if(!carSoundGlobals.policy.audioSystemEnabled)return;
    auto* manager=worldSpeechGetInstance();Require(manager,"Car preview has no actual speech manager");
    const auto result=worldSpeechFade(manager,duration);
    Require(result==WORLD_SPEECH_OK || result==WORLD_SPEECH_SUPPRESSED,"Car preview speech fade failed");
}

// Full final CarLockedDisplayer owner. Extra release hide/restore/input
// branches are written from194DE0..194FD0, not copied from beta's layout.
class CarLockedDisplayer final {
public:
    CarLockedDisplayer(MainMenuScreenServices& menus,std::uint32_t screen)
        : menus_(menus),screen_(screen)
    {
        auto& ui=*menus_.ui;auto& resources=*ui.resources;
        auto* anchor=resources.FindObject(screen_,0xa8af730bu);
        Require(anchor,"Locked car display is missing its authored anchor");
        float y;resources.GetTopLeft(*anchor,x_,y,ui.language);
        if(auto* object=resources.FindObject(screen_,0xbea3c79cu)) {
            if(object->kind==1)alpha_=static_cast<FlailerTextChunk*>(object)->GetAlpha();
            else if(object->kind==2)alpha_=static_cast<FlailerBitmapChunk*>(object)->GetAlpha(0);
        }
        slide_=std::make_unique<FlailerAnimationGroup>(ui,screen_,0x6a096871u,0xa8af730bu,640.0f,x_,0.5f);
    }
    void SetCurrentCar(VisibleCar& current,bool cheat,std::int32_t secondPlayer)
    {
        if(current_==&current)return;
        current_=&current;auto& ui=*menus_.ui;auto& resources=*ui.resources;
        const bool locked=!current.selectionFlag && !cheat;
        resources.SetGroupVisibility(screen_,0x6a096871u,locked?1:0);
        resources.SetObjectVisibility(screen_,0xbea3c79cu,locked?1:0);phase_=locked?2:0;
        resources.SetObjectVisibility(screen_,0x2a67e2d5u,0);
        Require(menus_.raceSetup && menus_.raceSetup->events,"Locked car display has no actual event/reward catalogue");
        FormatUnlockBlurbForCar(message_.data(),message_.size(),current.definition->modelCode,
            menus_.frontend->gameType,secondPlayer,*menus_.raceSetup->events,*ui.language);
        slide_->SetupToAnimate(650.0f,x_);started_=ui.clock->realTimer;
    }
    void Update()
    {
        auto& ui=*menus_.ui;
        const float elapsed=static_cast<float>(SelectCarTickDifference(ui.clock->realTimer,started_))*0.0002500000118743628f;
        float progress=(elapsed-0.4000000059604645f)/0.5f;
        if(elapsed>0.4000000059604645f)slide_->Animate();else progress=0.0f;
        if(auto* object=ui.resources->FindObject(screen_,0xbea3c79cu))
            object->SetAlpha(progress<1.0f?convertFloatToInt32(static_cast<float>(alpha_)*progress):alpha_);
        // Deliberately retain final PAL's hideTime-now order, including wrap.
        if(phase_==1 && static_cast<float>(SelectCarTickDifference(hidden_,ui.clock->realTimer))*0.0002500000118743628f>40.0f)
            Restore();
    }
    void Render(bool cheat)
    {
        if(!current_ || current_->selectionFlag || cheat || phase_!=2)return;
        auto& ui=*menus_.ui;Require(ui.render,"Locked car text has no actual Flailer renderer");
        ui.resources->SetObjectVisibility(screen_,0x2a67e2d5u,0);
        ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0x2a67e2d5u,0x2a65daf9u,message_.data(),1.0f);
    }
    void JoyHandler(std::int32_t event,std::uint8_t value,bool cheat)
    {
        if(!value)return;
        if(phase_==1) {Restore();menus_.ui->sound->Play(5);}
        else if(phase_==2 && event==33 && current_ && !current_->selectionFlag && !cheat) {
            menus_.ui->resources->SetGroupVisibility(screen_,0x6a096871u,0);
            menus_.ui->resources->SetObjectVisibility(screen_,0xbea3c79cu,0);
            phase_=1;hidden_=menus_.ui->clock->realTimer;menus_.ui->sound->Play(5);
        }
    }
    bool FirstCrack(ScreenMessage message)
    {
        if(phase_!=1)return false;
        if(message==ScreenMessage::Select || message==ScreenMessage::Left || message==ScreenMessage::Right ||
            message==ScreenMessage::Previous || message==ScreenMessage::Next || message==ScreenMessage::LeaveScreen) {
            Restore();menus_.ui->sound->Play(5);return true;
        }
        return false;
    }
private:
    void Restore()
    {
        auto& ui=*menus_.ui;ui.resources->SetGroupVisibility(screen_,0x6a096871u,1);
        ui.resources->SetObjectVisibility(screen_,0xbea3c79cu,1);phase_=2;
        slide_->SetupToAnimate(650.0f,x_);started_=ui.clock->realTimer;
    }
    MainMenuScreenServices& menus_;std::uint32_t screen_{};VisibleCar* current_{};
    float x_{};std::int32_t alpha_{},phase_{2},started_{},hidden_{};
    std::array<char,256> message_{};std::unique_ptr<FlailerAnimationGroup> slide_;
};
}

struct CarSelector::Storage {
    SelectCarScreenServices* services{};FrontendPlayerInfo* profile{};
    std::list<VisibleCar> nodes;std::int32_t port{},view{};std::uint32_t screen{};
    std::unique_ptr<FrontEndRenderingCar> normal,reflection;
    EditorMarkerModel cover{},platform{};
    std::unique_ptr<SelectCarCameraMover> camera;
    std::array<std::unique_ptr<FlailerAlphaAnimation>,3> alpha;
    std::unique_ptr<CarLockedDisplayer> locked;
    VisibleCar* loaded{};VisibleCar* pending{};
    std::int32_t fade{130};bool cheat{},orbitInputSeen{};
    float transition{},desiredTransition{};std::int32_t lastOrbitInput{};
    float radius{7.94950008392334f};std::array<float,4> offset{{-0.015769999474287033f,0.0f,-0.015769999474287033f,0.0f}};
    std::uint16_t vertical{0x3d60},fov{0x1c44},yaw{0x3efb};
    std::array<std::uint32_t,10> swatches{},swatchTextures{};std::int32_t swatchCount{};
    struct NativeBitmapLayout {
        FlailerBitmapChunk* bitmap{};
        std::uint32_t textureKey{};
        float scaleX{},scaleY{},width{},height{};
    };
    std::vector<NativeBitmapLayout> nativeLayouts;
    std::list<FlailerBitmapChunk> nativeThumbnails;
    tCubic1D thumbnails{};bool inputRegistered{};
    std::array<ChopperEffectInstance*,20> nebula{};
    std::int32_t nebulaCount{};
    float nebulaBlend{};
    bool ownsFrameEffects{};
    void ReleaseNebula() {
        if(!ownsFrameEffects)return;
        for(auto*& emitter:nebula)if(emitter) {
            services->effects->ReleaseFrameEmitter(*emitter,this);emitter=nullptr;
        }
        worldParticlesKillAllActiveEffects(&services->effects->FrameParticles());
        ownsFrameEffects=false;
    }
    ~Storage() {
        ReleaseNebula();
        // These objects borrow the same screen and texture catalogue as the
        // authored selector. Retire their strips and unlink them before the
        // owning FRONTB package or selected-car texture load is released.
        for(auto& bitmap:nativeThumbnails) {
            bitmap.DeleteFlailerStrip();
            if(bitmap.next)bitmap.next->previous=bitmap.previous;
            if(bitmap.previous)bitmap.previous->next=bitmap.next;
        }
        for(const auto& layout:nativeLayouts) {
            layout.bitmap->DeleteFlailerStrip();
            layout.bitmap->ChangeTexture(layout.textureKey);
            layout.bitmap->SetScaleX(layout.scaleX);layout.bitmap->SetScaleY(layout.scaleY);
        }
        if(normal)normal->overrideModel=nullptr;
        if(reflection)reflection->overrideModel=nullptr;
        editorMarkerModelDispose(&cover);editorMarkerModelDispose(&platform);
    }
    FlailerUiState& Ui()const {return *services->menus->ui;}
    FrontendPlayerManager& Frontend()const {return *services->menus->frontend;}
    bool Cloaked(const VisibleCar& car)const {return !car.selectionFlag && !cheat && car.availability->status==4;}
    static std::uint32_t TrafficIcon() {
        // All playable traffic uses the requested Generic presentation. The
        // released frontend supplies this neutral icon, not a Generic logo.
        return hashString("THUMBNAIL_UNKNOWN");
    }
    const NativeBitmapLayout& RememberBitmap(FlailerBitmapChunk& bitmap) {
        for(const auto& layout:nativeLayouts)if(layout.bitmap==&bitmap)return layout;
        auto& resources=*Ui().resources;
        nativeLayouts.push_back({&bitmap,bitmap.textureKey,bitmap.scaleX,bitmap.scaleY,
            resources.ScaledBitmapWidth(bitmap),resources.ScaledBitmapHeight(bitmap)});
        return nativeLayouts.back();
    }
    void NativeTexture(FlailerBitmapChunk& bitmap,std::uint32_t key,bool square) {
        const auto& layout=RememberBitmap(bitmap);
        const float width=square?std::min(layout.width,layout.height):layout.width;
        const float height=square?width:layout.height;
        bitmap.ChangeTexture(key);
        bitmap.SetWidth(*Ui().resources,width);bitmap.SetHeight(*Ui().resources,height);
    }
    void AuthoredTexture(FlailerBitmapChunk& bitmap,std::uint32_t key) {
        for(const auto& layout:nativeLayouts)if(layout.bitmap==&bitmap) {
            bitmap.SetScaleX(layout.scaleX);bitmap.SetScaleY(layout.scaleY);break;
        }
        bitmap.ChangeTexture(key);
    }
    void CreateTrafficThumbnails() {
        if(std::none_of(nodes.begin(),nodes.end(),[](const VisibleCar& car){return car.definition->bodyCategory==2;}))return;
        auto& resources=*Ui().resources;auto* target=resources.FindScreen(screen);
        Require(target,"Traffic selector has no actual screen");
        const auto* object=resources.FindObject(screen,SelectCarIndexedKey("THUMBNAIL_CLOAK_%d",0),2);
        Require(object,"Traffic selector has no authored thumbnail layout");
        const auto& prototype=*static_cast<const FlailerBitmapChunk*>(object);
        for(const auto& car:nodes)if(car.definition->bodyCategory==2 && !resources.FindObject(screen,car.thumbnailKey,2)) {
            nativeThumbnails.push_back(prototype);auto& bitmap=nativeThumbnails.back();
            bitmap.strip=nullptr;bitmap.next=bitmap.previous=nullptr;
            bitmap.key=car.thumbnailKey;bitmap.visible=0;bitmap.name.fill(0);
            const auto name="THUMBNAIL_"+car.definition->name;
            std::copy_n(name.c_str(),std::min(name.size(),bitmap.name.size()-1),bitmap.name.begin());
            bitmap.ChangeTexture(TrafficIcon());
            bitmap.next=&target->objects;bitmap.previous=target->objects.previous;
            bitmap.previous->next=&bitmap;target->objects.previous=&bitmap;
        }
    }
    float SeparatorWidth()const {
        auto* object=Ui().resources->FindObject(screen,0x2561a343u,2);
        Require(object,"Car selector is missing its authored thumbnail separator");
        return Ui().resources->ScaledBitmapWidth(*static_cast<FlailerBitmapChunk*>(object));
    }
    void RenderParameters(FrontEndRenderingCar& car,bool reflected)
    {
        if(!loaded)return;
        const auto& type=*loaded->definition;
        const float z=type.wheels[0].radius-type.wheels[0].position[2]+0.009999999776482582f;
        car.position[0]=0.0f;car.position[1]=0.0f;car.position[2]=reflected?-z:z;car.position[3]=0.0f;
        car.headlights=0;car.policeLights=0;
        eIdentity(car.orientation);
        if(reflected){car.orientation[1]=-car.orientation[1];car.orientation[10]=-car.orientation[10];}
        motionMatrixRotateYaw(car.orientation,car.orientation,yaw);
        const auto steering=static_cast<std::uint16_t>(convertFloatToInt32((1.0f-camera->GetModeParam())*4240.0f));
        for(std::size_t index=0;index<4;++index) {
            eIdentity(car.wheels[index]);eIdentity(car.brakes[index]);
            motionMatrixRotateY(car.wheels[index],car.wheels[index],0);
            if(index<2){motionMatrixRotateYaw(car.wheels[index],car.wheels[index],steering);motionMatrixRotateYaw(car.brakes[index],car.brakes[index],steering);}
            for(std::size_t lane=0;lane<3;++lane)car.wheels[index][12+lane]=car.brakes[index][12+lane]=type.wheels[index].position[lane];
            car.wheels[index][15]=car.brakes[index][15]=1.0f;
        }
    }
};

CarSelector::CarSelector(SelectCarScreenServices& services,FrontendPlayerInfo& profile,
    std::int32_t port,bool split,std::int32_t view,std::uint32_t screen)
    : splitScreen(split),screens(services.menus?services.menus->screens:nullptr),storage_(std::make_unique<Storage>())
{
    Require(services.menus && services.menus->ui && services.menus->ui->resources &&
        services.menus->ui->clock && services.menus->ui->language && services.menus->ui->sound &&
        services.menus->frontend && services.menus->raceSetup && services.menus->raceSetup->flow &&
        services.menus->raceSetup->cars && screens && screens->input && screens->input->router &&
        services.resources && services.naming && services.markers && services.cameras && services.frame && services.effects,
        "Car selector requires the actual application/profile/input/resources/camera owners");
    auto& own=*storage_;own.services=&services;own.profile=&profile;own.port=port;own.view=view;own.screen=screen;
    ProfileVisibleCars available(profile,*services.menus->frontend,view==1 && services.menus->frontend->gameType==7);
    own.nodes=std::move(available.nodes);visibleCars.root.next=visibleCars.root.previous=&visibleCars.root;
    visibleCars.current=available.set.current;
    for(auto& car:own.nodes){car.previous=visibleCars.root.previous;car.next=&visibleCars.root;visibleCars.root.previous->next=&car;visibleCars.root.previous=&car;}
    Require(!own.nodes.empty() && visibleCars.current && visibleCars.current!=&available.set.root,"Car selector has no actual visible car");
    CarRenderBindings render{};render.cars=services.menus->raceSetup->cars;render.resources=services.resources;
    render.naming=services.naming;render.markers=services.markers;render.applicationState=&services.menus->raceSetup->flow->currentState;
    own.normal=std::make_unique<FrontEndRenderingCar>(render,nullptr,view);
    own.reflection=std::make_unique<FrontEndRenderingCar>(render,nullptr,view);own.reflection->reflected=true;
    editorMarkerModelInitialize(&own.cover,&own.Ui().resources->Models(),hashString("CARCOVER"));
    editorMarkerModelInitialize(&own.platform,&own.Ui().resources->Models(),hashString("PLATFORM"));
    Require(own.cover.solid && own.platform.solid,"Select Car is missing its original CARCOVER/PLATFORM models");
    // PAL195198/1951AC/1951C0 use the group constructor248360.
    // These are group hashes, not individual object keys.
    constexpr std::uint32_t fadeGroups[]{0x0015678fu,0x036e80bfu,0x0961a600u};
    for(std::size_t i=0;i<own.alpha.size();++i)
        own.alpha[i]=std::make_unique<FlailerAlphaAnimation>(own.Ui(),screen,fadeGroups[i]);
    own.locked=std::make_unique<CarLockedDisplayer>(*services.menus,screen);
    own.camera=std::make_unique<SelectCarCameraMover>(*services.cameras,view,services.menus->ui->sound->audio);
    SetCameraParameters(visibleCars.current->definition->modelCode);
    own.pending=visibleCars.current;
    CarAppearanceSelection ride{};InitializeRideInfo(ride,own.pending->definition->modelCode,-1,0,0);
    (void)carLoader.Load(ride);carLoader.BeginLoading();
    own.locked->SetCurrentCar(*visibleCars.current,false,view==2?1:0);
    SetupCarOverrideModel();SetLogo();UpdateInfoBarAtTop();
    SetSelectCarLighting(services.resources->Lights(),view,0.0f,0);
    AdjustHelpMessage();SetupSwatches();own.CreateTrafficThumbnails();SetupThumbnails();UpdateFadeGroups();
    // PAL195694..1956BC: the localized label is an argument, not a format.
    own.Ui().resources->Printf(screen,0x736163fcu,"%s %d",own.Ui().language->GetString(0xbfb0),profile.totalPoints);
    // PAL1956C0..195784: single-player uses20 real FrameEffects emitters,
    // split-screen uses none. Both borrow the global afx emitter pool.
    own.ownsFrameEffects=true;own.nebulaCount=split?0:20;
    for(std::int32_t i=0;i<own.nebulaCount;++i)
        own.nebula[static_cast<std::size_t>(i)]=services.effects->CreateFrameEmitter(hashString("XE_SelectCarNebula"),&own);
    auto& input=*screens->input->router;GameInputTarget target{};target.kind=GAME_INPUT_CAR_SELECTOR;target.owner.carSelector=this;
    const auto firstPort=split?port:0,lastPort=split?port:1;
    Require(firstPort>=0 && lastPort<2,"Car selector input port exceeds the actual two ports");
    for(std::int32_t device=firstPort;device<=lastPort;++device)for(std::int32_t event=33;event<=43;++event) {
        if(gameInputRegister(&input,device,event,target)!=GAME_INPUT_OK) {
            for(std::int32_t oldPort=firstPort;oldPort<=device;++oldPort)
                for(std::int32_t oldEvent=33;oldEvent<=(oldPort==device?event-1:43);++oldEvent)gameInputUnregister(&input,oldPort,oldEvent);
            throw std::runtime_error("Car selector input registration failed");
        }
    }
    own.inputRegistered=true;Tick();
}
CarSelector::~CarSelector()
{
    if(!storage_)return;
    auto& own=*storage_;
    own.ReleaseNebula(); // PAL195800..1958C0, before preview resource release.
    if(own.inputRegistered)for(std::int32_t port=splitScreen?own.port:0;port<=(splitScreen?own.port:1);++port)
        for(std::int32_t event=33;event<=43;++event)gameInputUnregister(screens->input->router,port,event);
    if(popupIndex>=0)screens->DismissPopup(popupIndex);
    // PAL195964 explicitly clears both borrowed pointers before deletion.
    if(own.normal)own.normal->overrideModel=nullptr;
    if(own.reflection)own.reflection->overrideModel=nullptr;
    own.camera.reset();own.normal.reset();own.reflection.reset();
    for(auto* visible:{own.loaded,own.pending})if(visible){CarAppearanceSelection ride{};InitializeRideInfo(ride,visible->definition->modelCode,-1,0,0);carLoader.Unload(ride);}
    for(std::int32_t i=0;i<own.swatchCount;++i)own.Ui().resources->SetObjectVisibility(own.screen,own.swatches[static_cast<std::size_t>(i)],0);
    if(own.services->frame)own.services->frame->Clear();
    // The actual selected-car preload owns its own references and is not
    // conflated with this selector's loaded/pending ride above.
    PreloadCurrentSelectedCars(own.Frontend());PreloadCurrentSelectedCars(own.Frontend());
}

void CarSelector::SetCameraParameters(std::int32_t)
{
    auto& own=*storage_;own.offset={-0.015769999474287033f,0.0f,-0.015769999474287033f,0.0f};
    own.radius=7.94950008392334f;own.vertical=0x3d60;own.fov=0x1c44;own.yaw=0x3efb;
}

// BETA193E60..194108 -> PAL1963B8..196660, same complete body.
// Colour controls are those initialized by PAL198BC8, not a replacement fog.
void CarSelector::UpdateNebula()
{
    auto& own=*storage_;
    if(visibleCars.current->selectionFlag || own.cheat)
        own.nebulaBlend=std::max(own.nebulaBlend-0.009999999776482582f,0.0f);
    else own.nebulaBlend=std::min(own.nebulaBlend+0.009999999776482582f,1.0f);
    constexpr float unlocked[16]{60,100,180,0,60,100,180,38,60,100,180,41,60,100,180,0};
    constexpr float locked[16]{77,35,22,0,34,12,12,125,29,4,1.2999999523162842f,80,21,2.5999999046325684f,0,0};
    const auto step=own.nebulaCount?static_cast<std::uint16_t>(65536/own.nebulaCount):0;
    for(std::int32_t i=0;i<own.nebulaCount;++i) {
        auto* emitter=own.nebula[static_cast<std::size_t>(i)];if(!emitter)continue;
        const auto angle=static_cast<std::uint16_t>(i*step);
        eIdentity(emitter->frame);
        motionMatrixRotateYaw(emitter->frame,emitter->frame,static_cast<std::uint16_t>(angle+0x4000u));
        emitter->frame[12]=vectorCosineAngle(angle)*15.0f;
        emitter->frame[13]=rotationSineAngle(angle)*15.0f;
        emitter->frame[14]=-1.25f;emitter->frame[15]=1.0f;
        if(emitter->particleDefinition) {
            auto* definition=own.services->resources->Effects().EditableDefinition(emitter->particleDefinition->key);
            Require(definition==emitter->particleDefinition,"Nebula definition is not owned by its actual resource catalogue");
            for(std::size_t lane=0;lane<16;++lane)
                definition->controlColors[lane]=(locked[lane]-unlocked[lane])*own.nebulaBlend+unlocked[lane];
        }
    }
}
void CarSelector::SetupCarOverrideModel()
{
    auto& own=*storage_;auto* model=own.Cloaked(*visibleCars.current)?&own.cover:nullptr;
    own.normal->overrideModel=model;own.reflection->overrideModel=model;
}
void CarSelector::ShowLoadedCar()
{
    auto& own=*storage_;Require(own.loaded,"Car selector has no actually loaded car");
    const auto& car=*own.loaded;Require(car.skinIndex>=0 && car.skinIndex<car.availability->skinCount,"Loaded car skin index is invalid");
    const auto skin=car.availability->skinNumbers[static_cast<std::size_t>(car.skinIndex)];
    Require(own.services->menus->raceSetup->cars->FindSkin(car.definition->modelCode,skin,false),"Loaded car has no authored SkinInfo record");
    CarAppearanceSelection ride{};InitializeRideInfo(ride,car.definition->modelCode,skin,0,0);
    own.normal->ReInit(&ride);own.reflection->ReInit(&ride);
}
void CarSelector::SetupSwatches()
{
    auto& own=*storage_;auto& resources=*own.Ui().resources;
    for(std::int32_t i=0;i<own.swatchCount;++i)resources.SetObjectVisibility(own.screen,own.swatches[static_cast<std::size_t>(i)],0);
    const auto& car=*visibleCars.current;own.swatchCount=car.availability->skinCount;
    Require(own.swatchCount>=0 && own.swatchCount<=10,"Car selector authored swatches exceed source extent");
    for(std::int32_t i=0;i<own.swatchCount;++i) {
        const auto index=static_cast<std::size_t>(i);own.swatches[index]=SelectCarIndexedKey("SWATCH_%d",i);
        const auto* skin=own.services->menus->raceSetup->cars->FindSkin(car.definition->modelCode,car.availability->skinNumbers[index],car.definition->bodyCategory!=2);
        Require(skin,"Car selector swatch has no actual skin descriptor");
        // Every released traffic SkinInfo row points at the same placeholder
        // swatch. Its real per-skin livery is already acquired by CarLoader's
        // frontend all-skins load and identifies paint as well as markings.
        own.swatchTextures[index]=car.definition->bodyCategory==2?
            hashString((car.definition->baseName+"_SKIN"+std::to_string(skin->skinNumber)).c_str()):skin->authoredWord30;
    }
}
void CarSelector::UpdateSwatches()
{
    auto& own=*storage_;auto& resources=*own.Ui().resources;
    const auto x=splitScreen?606:587,y=own.screen==0x18dc8e52u?77:own.screen==0x18dc8e53u?321:125;
    const bool traffic=visibleCars.current->definition->bodyCategory==2;
    const bool visible=(own.swatchCount>=2 || (traffic && own.swatchCount==1)) && !own.Cloaked(*visibleCars.current);
    for(auto key:{0xedf2e4eeu,0x20d73d01u,0x7fe46db3u})resources.SetObjectVisibility(own.screen,key,visible?1:0);
    if(!visible)return;
    resources.SetTopLeftPosition(own.screen,0xedf2e4eeu,x,y-22);
    for(std::int32_t i=0;i<own.swatchCount;++i) {
        const auto index=static_cast<std::size_t>(i);auto* object=resources.FindObject(own.screen,own.swatches[index],2);
        Require(object,"Car selector is missing an authored SWATCH bitmap");
        auto& bitmap=*static_cast<FlailerBitmapChunk*>(object);
        if(traffic) {
            (void)own.RememberBitmap(bitmap);
            // Pending asynchronous car loads have no pixel owner yet. Keep
            // the cell hidden until that exact skin arrives; never resolve
            // the renderer's missing/default texture as a colour choice.
            if(!own.services->resources->ResolveTexture(own.swatchTextures[index],false)) {
                bitmap.DeleteFlailerStrip();bitmap.ChangeTexture(own.swatchTextures[index]);
                resources.SetObjectVisibility(own.screen,own.swatches[index],0);continue;
            }
            own.NativeTexture(bitmap,own.swatchTextures[index],false);
        } else own.AuthoredTexture(bitmap,own.swatchTextures[index]);
        resources.SetObjectVisibility(own.screen,own.swatches[index],1);resources.SetTopLeftPosition(own.screen,own.swatches[index],x,y+18*i);
    }
    resources.SetTopLeftPosition(own.screen,0x7fe46db3u,x-4,y+18*visibleCars.current->skinIndex-4);
    resources.SetTopLeftPosition(own.screen,0x20d73d01u,x,y+18*own.swatchCount+2);
}
void CarSelector::SetupThumbnails()
{
    auto& own=*storage_;tCubic1DInitialize(&own.thumbnails);own.thumbnails.continuous=0;own.thumbnails.duration=0.5f;
    own.thumbnails.value=own.thumbnails.desiredValue=static_cast<float>(splitScreen?395:377)-39.0f*static_cast<float>(visibleCars.current->selectionOrder)-
        (visibleCars.current->selectionFlag?0.0f:own.SeparatorWidth());
    own.thumbnails.mode=0;own.thumbnails.derivative=own.thumbnails.desiredDerivative;
    for(const auto& car:own.services->menus->raceSetup->cars->Cars())
        own.Ui().resources->SetObjectVisibility(own.screen,hashString(("THUMBNAIL_"+car.name).c_str()),0);
}
void CarSelector::UpdateThumbnails()
{
    auto& own=*storage_;auto& resources=*own.Ui().resources;tCubic1DUpdate(&own.thumbnails,own.Ui().clock->realTimeElapsed,0.0f,0.0f);
    for(std::int32_t i=0;i<25;++i)resources.SetObjectVisibility(own.screen,SelectCarIndexedKey("THUMBNAIL_CLOAK_%d",i),0);
    resources.SetObjectVisibility(own.screen,0x2561a343u,0);bool separated=false;std::int32_t cloaks=0;
    const auto y=own.screen==0x18dc8e52u?25:own.screen==0x18dc8e53u?265:402;
    float separator=0.0f;
    for(auto& car:own.nodes) {
        const float base=own.thumbnails.value+39.0f*static_cast<float>(car.selectionOrder);
        if(!car.selectionFlag && !separated) {
            resources.SetObjectVisibility(own.screen,0x2561a343u,1);
            resources.SetTopLeftPosition(own.screen,0x2561a343u,convertFloatToInt32(base),y);
            separator=own.SeparatorWidth();separated=true;
        }
        auto key=car.thumbnailKey;
        if(own.Cloaked(car)) {
            resources.SetObjectVisibility(own.screen,key,0);
            Require(cloaks<25,"Car selector authored cloaked thumbnail count exceeds source extent");
            key=SelectCarIndexedKey("THUMBNAIL_CLOAK_%d",cloaks++);
        }
        resources.SetObjectVisibility(own.screen,key,1);
        resources.SetTopLeftPosition(own.screen,key,convertFloatToInt32(base+separator),y);
    }
}
void CarSelector::SetLogo()
{
    auto& own=*storage_;auto& resources=*own.Ui().resources;const auto& car=*visibleCars.current->definition;
    if(own.Cloaked(*visibleCars.current)) {
        resources.SetObjectVisibility(own.screen,0x39e91754u,0);resources.SetObjectVisibility(own.screen,0xb270d4a5u,0);return;
    }
    resources.SetObjectVisibility(own.screen,0x39e91754u,1);resources.SetObjectVisibility(own.screen,0xb270d4a5u,1);
    resources.Printf(own.screen,0x39e91754u,"%s",car.GetFullName(*own.Ui().language));
    if(car.bodyCategory==2) {
        const std::string label="Generic "+car.name;
        resources.SetLocalizedText(own.screen,0x39e91754u,label.c_str());
        if(auto* logo=resources.FindObject(own.screen,0xb270d4a5u,2))
            own.NativeTexture(*static_cast<FlailerBitmapChunk*>(logo),Storage::TrafficIcon(),true);
        resources.SetObjectVisibility(own.screen,0x729a0c4bu,0);return;
    }
    auto name=car.name;
    if(splitScreen) {
        if(car.pursuitConfiguration) {
            if(name=="SPIDERHP")name="SPIDERHP2P";else {Require(name.size()>=2,"Car logo authored name is shorter than its suffix");name.resize(name.size()-2);}
        } else if(car.bodyCategory==1) {
            if(name=="CROWNVIC")name="CROWNVICNOCOP";else if(name=="BMWM5COP")name="BMWM5";
            else if(name=="MUSTANGCOP")name="MUSTANG";else if(name=="CORVETTECOP")name="CORVETTE";
            else if(name=="LAMBOCOP")name="MURCIELAGO";
        }
    }
    if(auto* logo=resources.FindObject(own.screen,0xb270d4a5u,2))own.AuthoredTexture(*static_cast<FlailerBitmapChunk*>(logo),hashString(("CARLOGO_"+name).c_str()));
    std::uint32_t extra=0;if(splitScreen)extra=car.bodyCategory==1?hashString("LOGO_SHERIFF"):car.pursuitConfiguration?hashString("LOGO_NFS"):0;
    resources.SetObjectVisibility(own.screen,0x729a0c4bu,extra?1:0);
    if(extra)if(auto* logo=resources.FindObject(own.screen,0x729a0c4bu,2))static_cast<FlailerBitmapChunk*>(logo)->ChangeTexture(extra);
}
void CarSelector::UpdateInfoBarAtTop()
{
    auto& own=*storage_;auto& resources=*own.Ui().resources;auto& language=*own.Ui().language;const auto& car=*visibleCars.current->definition;
    const bool display=!own.Cloaked(*visibleCars.current) && car.bodyCategory==0;
    for(auto key:{0x0c0384ccu,0xe6882001u,0xe79e7936u,0x02920d75u,0xd8b0bab5u,0xc86209a4u,0xc86209a5u,0xc86209a6u})
        resources.SetObjectVisibility(own.screen,key,display?1:0);
    if(display) {
        Require(car.languageInfo,"Car specification has no original translated descriptor");const auto& ids=*car.languageInfo;
        const auto units=own.Frontend().displaySettings.authoredOption0;const bool modified=car.pursuitConfiguration || car.bodyCategory==1;
        const auto speedId=units==0?ids.topSpeedMphLanguageId:units==1?ids.topSpeedKmhLanguageId:0;
        resources.Printf(own.screen,0x0c0384ccu,"%s%s %s",language.GetString(speedId),modified?"+":"",language.GetString(units?0x2234b:0x2234a));
        resources.Printf(own.screen,0xe6882001u,"%s%s %s",language.GetString(ids.accelerationLanguageId),modified?"-":"",language.GetString(0xa410));
        resources.Printf(own.screen,0xe79e7936u,"%s%s %s",language.GetString(ids.powerLanguageId),modified?"+":"",language.GetString(0xabe0));
        resources.Printf(own.screen,0x02920d75u,"%d",static_cast<std::int32_t>(car.routeVehicleClass)+1);
        resources.Printf(own.screen,0xd8b0bab5u,"%s",language.GetString(0xbfb1));
    }
    const auto unlocked=static_cast<std::int32_t>(std::count_if(own.nodes.begin(),own.nodes.end(),[](const auto& node){return node.selectionFlag;}));
    resources.Printf(own.screen,0x09b562fdu,"%s %d/%d",language.GetString(0xbfaf),unlocked,static_cast<std::int32_t>(own.nodes.size()));
    resources.SetObjectVisibility(own.screen,0x09b562fdu,own.Frontend().UseTheRaceEvent()?0:1);
}
void CarSelector::UpdateFadeGroups()
{
    auto& own=*storage_;own.alpha[0]->SetAlphaScale(1.0f-std::min(own.transition*3.0f,1.0f));
    own.alpha[1]->SetAlphaScale(visibleCars.current->definition->bodyCategory==1?0.0f:own.transition);
    if(own.transition==1.0f)own.alpha[2]->SetAlphaScale(0.0f);
    else if(own.desiredTransition==0.0f && own.transition<0.3333333432674408f)own.alpha[2]->SetAlphaScale(1.0f);
}
void CarSelector::AdjustHelpMessage()
{
    auto& own=*storage_;const auto game=own.Frontend().gameType;
    if(own.desiredTransition==0.0f) {
        screens->SetHelpOverride(game==8?eventEightHelp:game==9?eventNineHelp:
            game==7?(splitScreen?copSelectTwoPlayerHelp:copSelectHelp):(splitScreen?carSelectTwoPlayerHelp:nullptr));
    } else screens->SetHelpOverride(game==8?eventEightOrbitHelp:game==9?eventNineOrbitHelp:carOrbitHelp);
}
bool CarSelector::CanEnterOrbitMode(const VisibleCar* car)const
{
    const auto& own=*storage_;return car && (!screens->help || !screens->help->IsActive()) &&
        own.desiredTransition==0.0f && (car->selectionFlag || own.cheat);
}
void CarSelector::EnterOrbitMode()
{
    auto& own=*storage_;own.lastOrbitInput=own.Ui().clock->realLoopCounter;own.desiredTransition=1.0f;own.orbitInputSeen=false;
    const auto& car=*visibleCars.current->definition;
    // Traffic has no authored manufacturer/specification narration. Do not
    // enqueue the racer-description event with an empty car-description mask.
    if(car.bodyCategory!=2 && carSoundGlobals.policy.audioSystemEnabled && own.Ui().language->currentLanguage==0) {
        auto* manager=worldSpeechGetInstance();Require(manager && manager->messages && manager->messages->engine,
            "Car preview speech has no actual localized SPCH owner");
        auto* engine=manager->messages->engine;
        const auto description=static_cast<std::uint32_t>(worldSpeechConvertCarTypeToDescription(car.modelCode));
        if(car.bodyCategory==1)(void)spchAddEventV(engine,0xd3,&description,1);
        else {const std::uint32_t parameters[]{car.pursuitConfiguration?1u:0u,description};(void)spchAddEventV(engine,0x92,parameters,2);}
    }
    AdjustHelpMessage();own.camera->SetMode(1);
}
void CarSelector::EnterChooseMode()
{
    auto& own=*storage_;own.desiredTransition=0.0f;AdjustHelpMessage();own.camera->SetMode(0);FadeCarPreview(1.0f);
}
void CarSelector::AdvanceState(){++selectionState;SummonPopup();}
void CarSelector::DevanceState(){--selectionState;SummonPopup();}
void CarSelector::DismissPopup(){screens->DismissPopup(popupIndex);}
void CarSelector::SummonPopup()
{
    auto& own=*storage_;FlailerPopupConfig popup{};popup.flags=3;popup.sourceNode=screens->active;
    popup.screenKey=splitScreen?(own.view==1?0x6344da01u:0x6344da02u):0x41c2fdc3u;
    popup.joystickPort=own.port;popup.cancelResult=0;popup.dismissResult=6;
    if(selectionState==1) {
        popup.buttonCount=3;popup.message=own.Ui().language->GetString(0xbf68);
        popup.buttonLanguageIds={0xbf7c,0xbf86,0xbf72};popup.buttonResults={1,2,3};popup.initialButton=firstChoice;
    } else if(selectionState==2) {
        popup.buttonCount=2;popup.message=own.Ui().language->GetString(0xbf90);
        popup.buttonLanguageIds={0xbf9a,0xbfa4,0};popup.buttonResults={4,5,0};popup.initialButton=secondChoice;
    } else if(selectionState==3) {
        popup.message=own.Ui().language->GetString(0xbfae);popup.buttonResults={7,7,7};
    } else return;
    popupIndex=screens->SummonPopup(popup);
}
void CarSelector::Tick()
{
    auto& own=*storage_;CarAppearanceSelection ride{};
    if(own.pending) {
        InitializeRideInfo(ride,own.pending->definition->modelCode,-1,0,0);
        if(carLoader.IsLoaded(ride)) {
            own.loaded=own.pending;own.pending=nullptr;
            if(own.loaded==visibleCars.current){SetupCarOverrideModel();ShowLoadedCar();}
        }
    }
    if(own.loaded && own.loaded!=visibleCars.current && own.fade==130 && own.thumbnails.mode==0) {
        // Native preview models borrow the loaded skin's CPU texture views.
        // At full fade retire both appearances before releasing that skin:
        // another preview preload can retain the same geometry with a
        // different skin. ShowLoadedCar rebuilds them after the new load.
        own.normal->ReInit(nullptr);own.reflection->ReInit(nullptr);
        InitializeRideInfo(ride,own.loaded->definition->modelCode,-1,0,0);carLoader.Unload(ride);
        own.pending=visibleCars.current;own.loaded=nullptr;
        InitializeRideInfo(ride,own.pending->definition->modelCode,-1,0,0);(void)carLoader.Load(ride);carLoader.BeginLoading();
    }
    if(own.loaded==visibleCars.current) {
        if(own.fade==130)ShowLoadedCar();
        SetupCarOverrideModel();own.fade=std::max(own.fade-3,0);
    } else {own.fade+=7;if(own.fade>130){own.fade=130;SetupCarOverrideModel();}}
    if(auto* object=own.Ui().resources->FindObject(own.screen,0x75c8b565u))object->SetAlpha(own.fade);
    if(own.transition==1.0f && SelectCarTickDifference(own.Ui().clock->realLoopCounter,own.lastOrbitInput)>2400)EnterChooseMode();
    if(own.transition==0.0f || !own.orbitInputSeen)own.yaw=static_cast<std::uint16_t>(own.yaw+46u);
    const float statsSingle[]{0.2739799916744232f,0.0f,-0.5299999713897705f,0.0f};
    const float statsSplit[]{-0.6200000047683716f,0.0f,-1.0499999523162842f,0.0f};
    const float orbit[]{0.0f,0.0f,-0.6384000182151794f,0.0f};
    own.camera->SetRadiusStats(splitScreen?16.5f:12.930000305175781f);own.camera->SetOffsetStats(splitScreen?statsSplit:statsSingle);
    own.camera->SetOffsetOrbit(orbit);own.camera->SetOrientationStats(0,splitScreen?15654u:15262u);own.camera->SetFOVStats(own.fov);
    if(own.transition<own.desiredTransition)own.transition=std::min(own.transition+0.029999999329447746f,own.desiredTransition);
    if(own.transition>own.desiredTransition)own.transition=std::max(own.transition-0.029999999329447746f,own.desiredTransition);
    own.RenderParameters(*own.normal,false);own.RenderParameters(*own.reflection,true);UpdateFadeGroups();
    // PAL1970B0..1972A0: the same loaded swatch provides the preview-light
    // compensation. Preserve the authored palette lookup, float order and
    // distinct locked-car preset instead of deriving colour from the car mesh.
    const auto& current=*visibleCars.current;
    const auto swatch=current.availability->skinCount==1?0:current.skinIndex;
    Require(swatch>=0 && static_cast<std::size_t>(swatch)<own.swatchTextures.size(),
        "Select Car lighting swatch exceeds the actual texture table");
    auto lightingSwatch=own.swatchTextures[static_cast<std::size_t>(swatch)];
    if(current.definition->bodyCategory==2) {
        const auto* skin=own.services->menus->raceSetup->cars->FindSkin(current.definition->modelCode,
            current.availability->skinNumbers[static_cast<std::size_t>(swatch)],false);
        Require(skin,"Traffic preview lighting lost its actual skin");
        lightingSwatch=skin->authoredWord30; // UI atlas choice does not change authored preview lighting.
    }
    auto* texture=texturePackRegistry.ResolveLoadedTexture(own.services->resources->Models(),lightingSwatch);
    float amount=0.0f;
    if(!own.Cloaked(current) && texture && texture->pixels.Data() && texture->pixelBytes && texture->palette.Data()) {
        Require(texture->pixels.size>=4,"Select Car lighting swatch has no authored palette-index byte");
        const auto offset=static_cast<std::size_t>(texture->pixels.Data()[3])*4;
        Require(offset+4<=texture->palette.size,"Select Car lighting swatch index exceeds its actual palette");
        const auto color=ReadU32(texture->palette.Data()+offset);
        const float red=static_cast<float>(color&255u)*0.29899999499320984f;
        const float green=static_cast<float>((color>>8)&255u)*0.5870000123977661f;
        const float blue=static_cast<float>((color>>16)&255u)*0.11400000005960464f;
        amount=1.0f-((red+green)+blue)*0.003921568859368563f;
        amount=amount*amount;amount=amount*amount;
    }
    const bool alternate=!own.cheat && own.loaded &&
        !own.loaded->selectionFlag && own.loaded->availability->status==2;
    SetSelectCarLighting(own.services->resources->Lights(),own.view,amount,alternate?1:0);
    own.locked->Update();
    own.Ui().resources->SetObjectVisibility(own.screen,0xabb370a6u,visibleCars.GetNextCar(visibleCars.current)!=visibleCars.current || own.thumbnails.mode!=0);
    own.Ui().resources->SetObjectVisibility(own.screen,0x84f544f3u,visibleCars.GetPrevCar(visibleCars.current)!=visibleCars.current || own.thumbnails.mode!=0);
    UpdateSwatches();UpdateThumbnails();
    UpdateNebula();
    // The final UpdateSpotlight body is JR RA/NOP.
}
void CarSelector::JoyHandlerBridge(std::int32_t port,std::int32_t event,std::uint8_t value,void* owner)
{
    Require(owner,"Car selector input has no actual owner");static_cast<CarSelector*>(owner)->JoyHandler(port,event,value);
}
void CarSelector::JoyHandler(std::int32_t,std::int32_t event,std::uint8_t value)
{
    auto& own=*storage_;own.locked->JoyHandler(event,value,own.cheat);if(selectionState!=0)return;
    if(event==33) {
        if(!value || splitScreen || own.fade!=0)return;
        if(CanEnterOrbitMode(visibleCars.current))EnterOrbitMode();else if(own.desiredTransition==1.0f)EnterChooseMode();
        return;
    }
    // Final IsCheatingAllowed16FDB0 returns zero; source event34's body is
    // unreachable in PAL. No native cheat-success wrapper is introduced.
    if(event==34 || event==39 || event==40 || event==43)return;
    if(event>=35 && event<=42) {
        own.lastOrbitInput=own.Ui().clock->realLoopCounter;own.orbitInputSeen=true;
        if(event==35)own.camera->desiredHorizontalSpeed=value?-9000.0f:0.0f;
        else if(event==36)own.camera->desiredHorizontalSpeed=value?9000.0f:0.0f;
        else if(event==37)own.camera->desiredVerticalSpeed=value?10000.0f:0.0f;
        else if(event==38)own.camera->desiredVerticalSpeed=value?-10000.0f:0.0f;
        else if(event==41)own.camera->desiredHorizontalSpeed=(static_cast<float>(value)-128.0f)*70.3125f;
        else if(event==42)own.camera->desiredVerticalSpeed=(static_cast<float>(value)-128.0f)*-78.125f;
    }
}
void CarSelector::HandleMessage(ScreenMessage message,ScreenCallbackData& data)
{
    auto& own=*storage_;if(own.locked->FirstCrack(message))return;
    if(message==ScreenMessage::Tick){Tick();return;}
    if(message==ScreenMessage::LeaveScreen) {
        if(own.desiredTransition==1.0f){data.value=0;EnterChooseMode();data.sound=0;}return;
    }
    if(message==ScreenMessage::Previous || message==ScreenMessage::Next) {
        if(own.transition==0.0f && own.swatchCount>=2 && (visibleCars.current->selectionFlag || own.cheat)) {
            if(message==ScreenMessage::Previous)visibleCars.current->PrevSkin();else visibleCars.current->NextSkin();
            own.Ui().sound->Play(14);if(own.loaded==visibleCars.current)ShowLoadedCar();
        }
        data.value=1;return;
    }
    if(message==ScreenMessage::Left || message==ScreenMessage::Right) {
        if(own.transition!=0.0f)return;
        auto* current=message==ScreenMessage::Right?visibleCars.GetNextCar(visibleCars.current):visibleCars.GetPrevCar(visibleCars.current);
        own.locked->SetCurrentCar(*current,own.cheat,own.view==2?1:0);
        if(current==visibleCars.current)return;
        if(own.fade!=130)own.Ui().sound->Play(message==ScreenMessage::Right?12:13);
        visibleCars.current=current;
        own.thumbnails.desiredValue=static_cast<float>(splitScreen?395:377)-39.0f*static_cast<float>(current->selectionOrder)-
            (current->selectionFlag?0.0f:own.SeparatorWidth());
        if(own.thumbnails.value!=own.thumbnails.desiredValue)own.thumbnails.mode=2;
        SetLogo();UpdateInfoBarAtTop();SetupSwatches();return;
    }
    if(message==ScreenMessage::Select) {
        if(own.desiredTransition==1.0f){data.sound=0;return;}
        if(data.key==0x52e039f3u && selectionState==0 && (visibleCars.current->selectionFlag || own.cheat))AdvanceState();
        return;
    }
    if(message==ScreenMessage::RenderModels) {
        FrontendModelRequest request{};request.model=&own.platform;request.viewNumber=own.view;eIdentity(request.transform.data());
        own.services->frame->models.push_back(request);own.locked->Render(own.cheat);return;
    }
    if(message==ScreenMessage::PopupDismiss) {
        popupIndex=-1;if(data.value==0){DevanceState();return;}
        if(data.value==1)firstChoice=0;else if(data.value==2)firstChoice=1;else if(data.value==3)firstChoice=2;
        else if(data.value==4)secondChoice=0;else if(data.value==5)secondChoice=1;else if(data.value!=7)return;
        Require(selectCarScreen!=nullptr,"Popup completion has no actual SelectCarScreen owner");
        if(selectCarScreen->MaybeContinueToNextScreen())data.sound=18;else AdvanceState();
    }
}

SelectCarScreen::SelectCarScreen(SelectCarScreenServices& services)
    : frontend(services.menus?services.menus->frontend:nullptr),services_(&services)
{
    Require(frontend && services.menus->screens,"Select Car screen has no actual frontend/screen list");
    carLoader.frontendTextures=1;carLoader.singleSkinBatch=true;services.menus->screens->StopMovie();
}
SelectCarScreen::~SelectCarScreen()=default;
bool IsCarSelectScreenActive() noexcept
{
    // PAL198418 reads the same screen pointer published/retired by Callback.
    return selectCarScreen!=nullptr;
}
void SelectCarScreen::ActivateCarSelector(ScreenCallbackData& data)
{
    Require(services_ && frontend,"Select Car activation has no actual screen owner");
    const bool split=frontend->playerCount==2;if(split && data.joystickPort==-1)return;
    const std::size_t index=split && data.joystickPort!=0?1u:0u;
    Require(!ownedSelectors_[index] && frontend->players[index],"Select Car selector is already active or has no profile");
    ownedSelectors_[index]=std::make_unique<CarSelector>(*services_,*frontend->players[index],data.joystickPort,split,
        static_cast<std::int32_t>(index)+1,data.key);selectors[index]=ownedSelectors_[index].get();++selectorCount;
}
void SelectCarScreen::DeactivateCarSelector(ScreenCallbackData& data)
{
    const bool split=frontend->playerCount==2;if(split && data.joystickPort==-1)return;
    const std::size_t index=split && data.joystickPort!=0?1u:0u;
    if(ownedSelectors_[index]){ownedSelectors_[index].reset();selectors[index]=nullptr;--selectorCount;}
}
void SelectCarScreen::HandleMessage(ScreenMessage message,ScreenCallbackData& data)
{
    const std::size_t index=frontend->playerCount==2 && data.joystickPort!=0?1u:0u;
    if(selectors[index])selectors[index]->HandleMessage(message,data);
}
bool SelectCarScreen::MaybeContinueToNextScreen()
{
    if(!AreAllSelectorsReady())return false;
    SetSelectedCars();DismissAllPopups();
    if(frontend->gameType==8 || frontend->gameType==9)frontend->StartRace(*services_->menus->raceSetup);
    else services_->menus->screens->ChangeToScreen("Select Track",true);
    return true;
}
void SelectCarScreen::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(data && selectCarServices,"Select Car callback has no actual bound services/payload");
    if(selectCarScreen)selectCarScreen->HandleMessage(message,*data);
    auto& services=*selectCarServices;
    if(message==ScreenMessage::ScreenActivate) {
        if(!selectCarScreen)selectCarScreen=std::make_unique<SelectCarScreen>(services);
        selectCarScreen->ActivateCarSelector(*data);
        if(services.menus->frontend->playerCount==2)services.menus->ui->SetTwoPlayerScreenMode(1);
    } else if(message==ScreenMessage::ScreenDeactivate) {
        services.menus->screens->SetHelpOverride(nullptr);
        if(selectCarScreen){selectCarScreen->DeactivateCarSelector(*data);if(selectCarScreen->selectorCount==0)selectCarScreen.reset();}
        FadeCarPreview(0.20000000298023224f);
        if(services.menus->frontend->playerCount==2)services.menus->ui->SetTwoPlayerScreenMode(0);
    } else if(message==ScreenMessage::LeaveScreen && selectCarScreen)selectCarScreen->SetSelectedCars();
    else if(message==ScreenMessage::Tick && selectCarScreen) {
        for(auto* selector:selectCarScreen->selectors)if(selector && selector->storage_) {
            auto& owner=*selector->storage_;
            if(owner.normal && owner.loaded)owner.normal->drawFlags=1;
        }
    }
    // Final FrontEndRenderingCar::RestoreAlpha is JR RA/NOP and is folded.
}
void BindSelectCarScreen(SelectCarScreenServices& services)
{
    Require(!selectCarServices && !selectCarScreen && services.menus,"Select Car services are already bound or absent");selectCarServices=&services;
}
void UnbindSelectCarScreen(){selectCarScreen.reset();selectCarServices=nullptr;}
std::array<FlailerScreenConfig,2>& SelectCarScreenConfigurations()
{
    static std::array<FlailerScreenConfig,2> configurations{{
        {"Select Car",&SelectCarScreen::Callback,{{static_cast<std::int32_t>(0xb5074ab4u),0,0,0}},{{0,-1,-1,-1}},1,carSelectHelp},
        {"Select Car2P",&SelectCarScreen::Callback,{{static_cast<std::int32_t>(0x18dc8e52u),static_cast<std::int32_t>(0x18dc8e53u),0,0}},{{0,1,-1,-1}},2,carSelectHelp}}};
    return configurations;
}
} // namespace hp2

#include "Fred.hpp"
#include "FMenus.hpp"
#include "FlailerDB.hpp"
#include "CarLoader.hpp"
#include "GameFlow.hpp"
#include "bware/inc/bMath.hpp"
#include "eMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdio>

namespace hp2 {
namespace {
// RewardView.obj's retained final source cells, initialized by PAL23CC88.
float trackRewardAngle{};
float carRewardAngle{};
constexpr float RewardAngleStep=0.009999999776482582f;
constexpr float RadiansToAngle=10430.3779296875f;
}

struct tTrackModelViewer::Storage {
    SelectCarScreenServices* services{};
    EditorMarkerModel model{};
    ~Storage() {editorMarkerModelDispose(&model);}
};

tTrackModelViewer::tTrackModelViewer(SelectCarScreenServices& services,const EventReward& reward)
    :storage_(std::make_unique<Storage>())
{
    // PAL23C2E0 forwards the actual reward bytes to SetModel23C248.
    Require(services.menus && services.menus->ui && services.menus->ui->resources &&
        services.menus->raceSetup && services.menus->raceSetup->tracks && services.frame,
        "Track reward preview has no actual track/model/frame owners");
    auto& own=*storage_;own.services=&services;
    const auto number=static_cast<std::int32_t>((reward.selection>>8)&255u);
    const auto* track=services.menus->raceSetup->tracks->Find(number);
    Require(track,"Track reward preview names an absent authored track");
    char name[32];const auto count=std::snprintf(name,sizeof(name),"TRACK%d_MINI",track->baseNumber);
    Require(count>=0 && static_cast<std::size_t>(count)<sizeof(name),"Track reward model name exceeds its source scratch extent");
    editorMarkerModelInitialize(&own.model,&services.menus->ui->resources->Models(),hashString(name));
    // The original RenderTrack tests model.solid; it does not invent a mesh.
}

tTrackModelViewer::~tTrackModelViewer()=default;

void tTrackModelViewer::RenderTrack()
{
    // Complete PAL23C108..23C1F4. The original view is one, scale is one.
    auto& own=*storage_;if (!own.model.solid)return;
    FrontendModelRequest request{};request.model=&own.model;request.viewNumber=1;
    rotationMatrixIdentity(request.transform.data());
    const auto angle=static_cast<std::uint16_t>(convertFloatToInt32(trackRewardAngle*RadiansToAngle));
    motionMatrixRotateYaw(request.transform.data(),request.transform.data(),angle);
    own.services->frame->models.push_back(request);
    own.services->frame->modelDrawCpuClosureRequired=true;
}

void tTrackModelViewer::TickTrack(FrontendStillCamera& camera)
{
    // Complete PAL23C1F8..23C244 and final constructor vector writes.
    camera.eye={30.0f,7.690000057220459f,20.0f,0.0f};
    camera.target={3.9100000858306885f,-1.059000015258789f,0.875f,0.0f};
    trackRewardAngle+=RewardAngleStep;
    if (trackRewardAngle>6.283185005187988f)trackRewardAngle-=6.283185005187988f;
}

struct tCarModelViewer::Storage {
    SelectCarScreenServices* services{};
    CarAppearanceSelection ride{};
    std::unique_ptr<FrontEndRenderingCar> render;
    bool loaded{};
    ~Storage() {
        // PAL23C410 destroys the render owner before releasing its loaded ride.
        render.reset();
        if (loaded) {loaded=false;carLoader.Unload(ride);}
    }
};

tCarModelViewer::tCarModelViewer(SelectCarScreenServices& services,const EventReward& reward)
    :storage_(std::make_unique<Storage>())
{
    // Complete PAL23C368..23C410, native lifetime projection of the same ride.
    Require(services.menus && services.menus->raceSetup && services.menus->raceSetup->cars &&
        services.menus->raceSetup->flow && services.resources && services.naming && services.markers,
        "Car reward preview has no canonical car/resource/application owners");
    auto& own=*storage_;own.services=&services;
    const auto model=static_cast<std::int32_t>((reward.selection>>8)&255u);
    const auto skin=static_cast<std::int32_t>((reward.selection>>16)&255u);
    InitializeRideInfo(own.ride,model,skin,0,0);
    CarRenderBindings bindings{};bindings.cars=services.menus->raceSetup->cars;
    bindings.resources=services.resources;bindings.naming=services.naming;bindings.markers=services.markers;
    bindings.applicationState=&services.menus->raceSetup->flow->currentState;
    own.render=std::make_unique<FrontEndRenderingCar>(bindings,&own.ride,1);
    own.render->enabled=false;
}

tCarModelViewer::~tCarModelViewer()=default;

void tCarModelViewer::BeginLoading()
{
    // PAL23C488..23C4F4. Mark the acquired native reference before the
    // asynchronous BeginLoading boundary so an exception releases it once.
    auto& own=*storage_;if (own.loaded)return;
    carLoader.frontendTextures=1;carLoader.singleSkinBatch=true;
    (void)carLoader.Load(own.ride);own.loaded=true;carLoader.BeginLoading();
}
void tCarModelViewer::Unload()
{
    // PAL23C4F8..23C520 clears the loaded cell before Unload.
    auto& own=*storage_;if (!own.loaded)return;
    own.loaded=false;carLoader.Unload(own.ride);
}
void tCarModelViewer::MakeVisible()
{
    auto& own=*storage_;if (own.render->enabled)return;
    own.render->ReInit(&own.ride);own.render->enabled=true; // PAL23C7C8.
}
void tCarModelViewer::MakeInvisible() {storage_->render->enabled=false;} // PAL23C810.

void tCarModelViewer::Tick(FrontendStillCamera& camera)
{
    // Complete PAL23C520..23C7C8. RenderCar23C480 is exactly jr ra/nop;
    // this owner is consumed by the real global RenderFrontEndCars instead.
    auto& own=*storage_;auto& render=*own.render;
    for (auto& value:render.position)value=0.0f;
    carRewardAngle+=RewardAngleStep;
    if (carRewardAngle>6.2829999923706055f)carRewardAngle-=6.2829999923706055f;
    rotationMatrixIdentity(render.orientation);
    motionMatrixRotateYaw(render.orientation,render.orientation,
        static_cast<std::uint16_t>(convertFloatToInt32(carRewardAngle*RadiansToAngle)));
    const auto& car=own.services->menus->raceSetup->cars->Car(static_cast<std::size_t>(own.ride.modelCode));
    for (std::size_t index=0;index<4;++index) {
        rotationMatrixIdentity(render.wheels[index]);rotationMatrixIdentity(render.brakes[index]);
        motionMatrixRotateY(render.wheels[index],render.wheels[index],0);
        if (index<2) {
            motionMatrixRotateYaw(render.wheels[index],render.wheels[index],0x1090);
            motionMatrixRotateYaw(render.brakes[index],render.brakes[index],0x1090);
        }
        for (std::size_t lane=0;lane<3;++lane)
            render.wheels[index][12+lane]=render.brakes[index][12+lane]=car.wheels[index].position[lane];
        render.wheels[index][15]=render.brakes[index][15]=1.0f;
    }
    camera.eye={-6.866000175476074f,-10.0f,3.819999933242798f,0.0f};
    camera.target={-0.1379999965429306f,1.24399995803833f,-0.414000004529953f,0.0f};
}

} // namespace hp2

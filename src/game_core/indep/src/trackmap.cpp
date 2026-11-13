#include "indep/inc/TrackMap.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Race.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace hp2 {
namespace {
// Complete final1BD1F8 constant-write trace, including all11 records and9
// colours. No beta assets or serialized native layouts participate.
const std::array<TrackMapModeInfo,11> modes{{
    {54746,200.0f,92,228,190.0f,190.0f,92,240,139.0f,91.2863998413086f,0.44178101420402527f,0.20205500721931458f,0,0.09589000046253204f,0.024999000132083893f,0.1349949985742569f,1,1,1,1.0f,1,1},
    {54746,200.0f,92,228,190.0f,190.0f,92,240,139.0f,91.2863998413086f,0.44178101420402527f,0.20205500721931458f,0,0.09589000046253204f,0.03499000146985054f,0.16998699307441711f,0,1,1,1.0f,1,1},
    {65535,200.0f,72,243,130.0f,130.0f,72,243,100.0f,100.0f,0.33000001311302185f,0.33000001311302185f,0,0.10000000149011612f,0.03998199850320816f,0.18498000502586365f,1,1,1,1.0f,1,1},
    {65535,200.0f,72,243,130.0f,130.0f,72,243,100.0f,100.0f,0.33000001311302185f,0.33000001311302185f,0,0.10000000149011612f,0.039980001747608185f,0.1949779987335205f,0,1,1,1.0f,1,1},
    {65535,200.0f,79,256,130.0f,130.0f,79,218,1000.0f,1000.0f,1.0f,1.0f,0,0.07000000029802322f,0.04497700184583664f,0.14997200667858124f,0,0,0,1.0f,1,0},
    {65535,200.0f,320,243,262.0f,262.0f,320,243,1000.0f,1000.0f,1.0f,1.0f,0,0.05999999865889549f,0.029999999329447746f,0.0949999988079071f,0,0,1,0.28999999165534973f,1,0},
    {0,0.0f,0,0,0.0f,0.0f,0,0,0.0f,0.0f,0.0f,0.0f,0,0.0f,0.0f,0.0f,0,0,0,0.0f,0,0},
    {65535,200.0f,83,245,130.0f,130.0f,83,245,140.0f,140.0f,1.0f,1.0f,0,0.07000000029802322f,0.04497700184583664f,0.14997200667858124f,0,0,0,1.0f,1,0},
    {65535,200.0f,79,256,130.0f,130.0f,79,218,1000.0f,1000.0f,1.0f,1.0f,0,0.07000000029802322f,0.04497700184583664f,0.14997200667858124f,0,0,0,1.0f,1,0},
    {65535,200.0f,320,243,262.0f,262.0f,320,243,1000.0f,1000.0f,1.0f,1.0f,0,0.05999999865889549f,0.029999999329447746f,0.0949999988079071f,0,0,1,0.28999999165534973f,1,0},
    {65535,200.0f,72,243,130.0f,130.0f,72,243,100.0f,100.0f,0.33000001311302185f,0.33000001311302185f,1,0.10000000149011612f,0.03998199850320816f,0.18498000502586365f,1,1,1,1.0f,1,1},
}};
const std::array<std::array<std::uint8_t,3>,9> markerColors{{
    {172,53,0},
    {75,85,255},
    {0,0,255},
    {0,100,0},
    {0,0,128},
    {128,128,0},
    {172,53,0},
    {0,64,122},
    {0,0,192},
}};
FlailerPoly MakePolygon()
{
    FlailerPoly polygon;
    // Final ePoly constructor10FA38..10FA58: ALPHA=(Cs-Cd)*As+Cd,
    // FIX=128 and ScaleCoordinates=1. Value-initialized transport words
    // alone lose both the authored alpha equation and the640x480 UI space.
    polygon.rasterState[0]=0x01000100u;
    polygon.rasterState[1]=0x01000080u;
    const float x[4]{0,64,64,0},y[4]{0,0,64,64};
    for (std::size_t i=0;i<4;++i) {
        polygon.vertices[i].position={x[i],y[i],0,0};
        polygon.vertices[i].textureCoordinate={x[i]/64.0f,y[i]/64.0f,1.0f};
        polygon.vertices[i].color=0x80808080u;
    }
    return polygon;
}
std::uint8_t Byte(float value)
{return static_cast<std::uint8_t>(convertFloatToInt32(value));}
bool MarkerVisible(const CarMovement* car)
{
    Require(car && car->bindings.ownerMotion && car->bindings.state,"TrackMap has no actual car state");
    return car->bindings.ownerMotion->actionEnabled && car->bindings.ownerMotion->transitionCounter;
}
}

const std::array<TrackMapModeInfo,11>& TrackMap::Modes(){return modes;}
const TrackMapModeInfo& TrackMap::CurrentMode() const{return modes[static_cast<std::size_t>(activeMode_)];}
void TrackMap::SetMode(std::int32_t mode)
{
    Require(mode>=0 && static_cast<std::size_t>(mode)<modes.size(),"TrackMap mode is outside its original table");
    configuredMode_=mode;
}
TrackMap::TrackMap(const TrackResourceInfo* track,CarMovement* car,CarFramePlayerState* player,
    FlailerResources& resources,FlailerRenderManager& renderer,
    CarCinematicServices& cameras,std::int32_t overlayViewNumber,
    const RaceParameters& parameters,bool playerHeading)
    :track_(track),car_(car),player_(player),resources_(resources),renderer_(renderer),
    cameras_(cameras),parameters_(parameters),overlayViewNumber_(overlayViewNumber),
    playerHeading_(playerHeading)
{
    valid_=track_ && car_ && player_;
    if (!valid_)return;
    Require(player_->registry && player_->registry->clock,"TrackMap constructor has no live GameTime");
    timestamp_=player_->registry->clock->worldTimer;
    char name[32];std::snprintf(name,sizeof(name),"TRACK%d_MAP",static_cast<int>(track_->number));
    mapTexture_=resources_.FindTexture(hashString(name));
    marker_=resources_.FindTexture(hashString("CAR_MARKER"));
    orientedMarker_=resources_.FindTexture(hashString("CAR_MARKERB"));
    frame_=resources_.FindTexture(hashString("TRACKMAPFRAME"));
    background_=resources_.FindTexture(hashString("MAPBACK"));
    startLine_=resources_.FindTexture(hashString("START_LINE"));
    roadBlock_=resources_.FindTexture(hashString("BKGBMP"));
    valid_=mapTexture_ && marker_ && orientedMarker_ && frame_ && background_ && (parameters_.reverse || startLine_);
    eIdentity(mapTransform_);motionMatrixRotateYaw(mapTransform_,mapTransform_,track_->mapRotation);
}
TrackMapPoint TrackMap::GetCarUVPosition(const float position[3]) const
{
    Require(track_ && track_->mapScale!=0.0f,"TrackMap final track transform has zero scale");
    float point[4]{(position[0]-track_->mapOriginX)/track_->mapScale,
        (-position[1]+track_->mapOriginY)/track_->mapScale,1.0f,0.0f};
    eMulVector3(point,mapTransform_,point);return {point[0],point[1]};
}
TrackMapQuad TrackMap::GetBoxOnMap(const TrackMapPoint& center,std::uint16_t angle,
    float upper,float lower,float left,float right)
{
    float rotate[16],transform[16];eIdentity(rotate);motionMatrixRotateYaw(rotate,rotate,angle);
    const float translate[3]{center[0],center[1],0};motionMatrixTranslation(transform,translate);
    eMulMatrix(transform,rotate,transform);
    std::array<std::array<float,4>,4> points{{{right,upper,0,0},{-left,upper,0,0},
        {-left,-lower,0,0},{right,-lower,0,0}}};
    TrackMapQuad result{};
    for (std::size_t i=0;i<4;++i) {eMulVector3(points[i].data(),transform,points[i].data());result[i]={points[i][0],points[i][1]};}
    return result;
}
TrackMapPoint TrackMap::CalculateCentroidOfQuad(const TrackMapQuad& quad)
{
    TrackMapPoint result{};
    for (std::size_t axis=0;axis<2;++axis) {
        const float first=((quad[0][axis]+quad[1][axis])+quad[2][axis])*0.3333333432674408f;
        const float second=((quad[0][axis]+quad[2][axis])+quad[3][axis])*0.3333333432674408f;
        result[axis]=(first+second)*0.5f;
    }
    return result;
}
void TrackMap::EvalCubic(float& position,float& velocity,float target,float elapsed)
{
    const float difference=target-position;
    float second=(difference+difference)+difference;
    float third=velocity-difference;third-=difference;second-=velocity;second-=velocity;
    float next=(third*elapsed+second)*elapsed;next=(next+velocity)*elapsed+position;
    float rate=((third+third)+third)*elapsed;rate=(rate+(second+second))*elapsed+velocity;
    position=next;velocity=rate;
}
bool TrackMap::GetCameraDirection(std::array<float,4>& direction)
{
    auto* screen=ecstasyViewAt(&cameras_,overlayViewNumber_);
    Require(screen,"TrackMap camera screen is absent");
    auto* camera=screen->first;
    if (!camera || camera->kind!=1) {
        const auto* race=player_->registry->services->race;
        Require(race && car_->bindings.state,"TrackMap direction has no actual race/car");
        if (race->state==4) {direction[0]=direction_[0];direction[1]=direction_[1];}
        else {direction[0]=car_->bindings.state->orientation[0];direction[1]=car_->bindings.state->orientation[1];}
        return true;
    }
    Require(camera->view && camera->ownedObject,"TrackMap chase direction has no actual camera owner");
    const auto* chase=static_cast<const CarChaseCamera*>(camera->ownedObject);
    std::copy_n(camera->view->current.forward,3,direction.data());
    float matrix[16];eIdentity(matrix);motionMatrixRotateYaw(matrix,matrix,chase->yawOffset);
    eMulVector3(direction.data(),matrix,direction.data());
    if (std::fabs(direction_[0])<=0.0010000000474974513f && std::fabs(direction_[1])<=0.0010000000474974513f) {
        direction_={direction[0],direction[1]};timestamp_=player_->registry->clock->worldTimer;
    }
    return true;
}
bool TrackMap::GetAngle(std::uint16_t& angle)
{
    if (!CurrentMode().followCamera) {angle=32768;return true;}
    if (playerHeading_) {
        Require(car_->bindings.state,"TrackMap direction has no actual car");
        // World Y is reflected in GetCarUVPosition. The map's positive
        // local Y axis projects to screen up.
        angle=static_cast<std::uint16_t>(track_->mapRotation-car_->bindings.state->heading-0x4000);
        return true;
    }
    std::array<float,4> target{};if (!GetCameraDirection(target))return false;
    const auto now=player_->registry->clock->worldTimer;
    const float elapsed=(static_cast<float>(static_cast<std::int32_t>(static_cast<std::uint32_t>(now)-
        static_cast<std::uint32_t>(timestamp_)))*0.0002500000118743628f)*0.8999999761581421f;
    EvalCubic(direction_[0],velocity_[0],target[0],elapsed);
    EvalCubic(direction_[1],velocity_[1],target[1],elapsed);timestamp_=now;
    // GetCarUVPosition reflects world Y and rotates the map by mapRotation.
    // Rotate the follow direction into the same map frame before building
    // the texture crop and positioning its markers.
    // DrawMarker negates the local map axes when projecting to screen.
    // The forward vector must map to local +Y, hence screen up.
    angle=static_cast<std::uint16_t>(track_->mapRotation+
        vectorAtan2Angle(direction_[1],direction_[0]));return true;
}
void TrackMap::MapToScreen(FlailerPoly& polygon,
    std::array<std::array<float,4>,4>& coordinates) const
{
    const auto& mode=CurrentMode();float matrix[16];eIdentity(matrix);motionMatrixRotateX(matrix,matrix,mode.tilt);
    for (std::size_t i=0;i<4;++i) {
        auto& point=coordinates[i];eMulVector3(point.data(),matrix,point.data());
        auto& vertex=polygon.vertices[i];const float denominator=point[2]+mode.perspectiveDepth;
        vertex.position[0]=(point[0]*mode.perspectiveDepth)/denominator+mode.screenX;
        vertex.position[1]=(point[1]*mode.perspectiveDepth)/denominator+mode.screenY;
        const auto alpha=point[2]>0.0f?Byte((1.0f-point[2]/(mode.mapHeight*0.6666666865348816f))*128.0f):128u;
        vertex.color=(vertex.color&0x00ffffffu)|(static_cast<std::uint32_t>(alpha)<<24);
    }
}
void TrackMap::DrawOverOrUnderLay(const EditorMarkerTexture* texture,std::int32_t depth)
{
    const auto& mode=CurrentMode();if (!mode.borderEnabled)return;
    auto polygon=MakePolygon();const float halfWidth=mode.clipWidth*0.5f,halfHeight=mode.clipHeight*0.5f;
    polygon.vertices[0].position={mode.clipX-halfWidth,mode.clipY-halfHeight,static_cast<float>(depth),0};
    polygon.vertices[1].position={mode.clipX+halfWidth,mode.clipY-halfHeight,static_cast<float>(depth),0};
    polygon.vertices[2].position={mode.clipX+halfWidth,mode.clipY+halfHeight,static_cast<float>(depth),0};
    polygon.vertices[3].position={mode.clipX-halfWidth,mode.clipY+halfHeight,static_cast<float>(depth),0};
    if (!mode.mapEnabled)return;
    polygon.rasterState[2]=1;
    renderer_.SubmitPolygon(resources_.StripPool(),texture,polygon,depth);
}
void TrackMap::DrawMap(TrackMapPoint& center,TrackMapQuad& bounds,std::uint16_t angle)
{
    const auto& mode=CurrentMode();if (!mode.mapEnabled)return;
    const auto uv=GetCarUVPosition(car_->bindings.state->position);TrackMapQuad coordinates{};
    if (mode.localMap) {
        float upper=mode.upperExtent,lower=mode.lowerExtent,horizontal=(upper+lower)*0.5f;
        if (mode.targetZoom && target_) {
            const auto* first=car_->bindings.state->position;const auto* second=target_->bindings.state->position;
            const float dx=first[0]-second[0],dy=first[1]-second[1];
            upper=((std::sqrt(dx*dx+dy*dy)/track_->mapScale)*(mode.mapWidth/mode.clipWidth))+0.10000000149011612f;
            lower=upper;horizontal=upper;
        }
        upperExtent_=upper;lowerExtent_=lower;
        coordinates=GetBoxOnMap(uv,angle,upper,lower,horizontal,horizontal);
        bounds=GetBoxOnMap(uv,angle,upper*1.2000000476837158f,lower*1.2000000476837158f,
            horizontal*1.2000000476837158f,horizontal*1.2000000476837158f);
    } else {coordinates={{{0,0},{1,0},{1,1},{0,1}}};bounds=coordinates;}
    center=CalculateCentroidOfQuad(coordinates);auto polygon=MakePolygon();
    std::array<std::array<float,4>,4> points{{{-mode.mapWidth*0.5f,-mode.mapHeight*0.5f,0,0},
        {mode.mapWidth*0.5f,-mode.mapHeight*0.5f,0,0},{mode.mapWidth*0.5f,mode.mapHeight*0.5f,0,0},
        {-mode.mapWidth*0.5f,mode.mapHeight*0.5f,0,0}}};
    for (std::size_t i=0;i<4;++i)polygon.vertices[i].textureCoordinate={coordinates[i][0],coordinates[i][1],1};
    MapToScreen(polygon,points);
    for (std::size_t i=0;i<4;++i) {
        auto& vertex=polygon.vertices[i];const auto alpha=Byte(static_cast<float>(vertex.color>>24)*mode.opacity);
        vertex.color=(vertex.color&0x00ffffffu)|(static_cast<std::uint32_t>(alpha)<<24);
        const float q=1.0f/(points[i][2]+mode.perspectiveDepth);
        for (auto& component:vertex.textureCoordinate)component*=q;
    }
    ClipPolygon(polygon,mapTexture_,14);
}
void TrackMap::DrawMarker(const float position[3],const TrackMapPoint& center,std::uint16_t angle,
    const TrackMapQuad& bounds,const std::array<std::uint8_t,3>& color,std::int32_t depth,
    const EditorMarkerTexture* texture,float width,float height,std::uint16_t rotation)
{
    const auto& mode=CurrentMode();if (!mode.mapEnabled)return;
    const auto uv=GetCarUVPosition(position);float rawBounds[4][2];
    for (std::size_t i=0;i<4;++i) {rawBounds[i][0]=bounds[i][0];rawBounds[i][1]=bounds[i][1];}
    if (!pursuitPointInPolygon(uv.data(),rawBounds,4))return;
    float relative[4]{uv[0]-center[0],uv[1]-center[1],0,0};float rotate[16];eIdentity(rotate);
    motionMatrixRotateYaw(rotate,rotate,static_cast<std::uint16_t>(-angle));eMulVector3(relative,rotate,relative);
    if (mode.localMap) {relative[0]/=upperExtent_+lowerExtent_;relative[1]/=upperExtent_+lowerExtent_;}
    const auto box=GetBoxOnMap({relative[0],relative[1]},rotation,height*0.5f,height*0.5f,width*0.5f,width*0.5f);
    auto polygon=MakePolygon();std::array<std::array<float,4>,4> points{};
    for (std::size_t i=0;i<4;++i)points[i]={-box[i][0]*mode.mapWidth,-box[i][1]*mode.mapHeight,0,0};
    MapToScreen(polygon,points);
    const std::uint32_t rgb=color[0]|(static_cast<std::uint32_t>(color[1])<<8)|(static_cast<std::uint32_t>(color[2])<<16);
    for (auto& vertex:polygon.vertices)vertex.color=(vertex.color&0xff000000u)|rgb;
    ClipPolygon(polygon,texture,depth);
}
void TrackMap::ClipPolygon(const FlailerPoly& polygon,const EditorMarkerTexture* texture,std::int32_t depth)
{
    const auto& mode=CurrentMode();const float halfX=mode.clipWidth*0.5f,halfY=mode.clipHeight*0.5f;
    const TrackMapQuad clip{{{mode.clipX-halfX,mode.clipY-halfY},{mode.clipX+halfX,mode.clipY-halfY},
        {mode.clipX+halfX,mode.clipY+halfY},{mode.clipX-halfX,mode.clipY+halfY}}};
    std::vector<FlailerPolyVertex> input(polygon.vertices.begin(),polygon.vertices.end()),output;
    input.reserve(8);output.reserve(8);
    // PAL clips in edge order left/top/right/bottom, emits each previous
    // inside vertex, then the current->previous intersection. UVQ and byte
    // alpha interpolate; RGB is copied from the original polygon corners.
    for (std::size_t edge=0;edge<4;++edge) {
        output.clear();if (input.empty())break;
        const auto& a=clip[edge];const auto& b=clip[(edge+3)%4];
        const float edgeX=b[0]-a[0],edgeY=b[1]-a[1];
        const auto inside=[&](const FlailerPolyVertex& vertex) {
            const float dx=vertex.position[0]-a[0],dy=vertex.position[1]-a[1];
            return dx*edgeY+dy*(-edgeX)>=0.0f;
        };
        std::size_t previous=input.size()-1;
        for (std::size_t current=0;current<input.size();++current) {
            const auto& first=input[current];const auto& last=input[previous];
            const bool currentInside=inside(first),previousInside=inside(last);
            if (previousInside)output.push_back(last);
            if (currentInside!=previousInside) {
                const float dx=last.position[0]-first.position[0],dy=last.position[1]-first.position[1];
                const float ax=first.position[0]-a[0],ay=first.position[1]-a[1];
                const float fraction=(ay*edgeX-ax*edgeY)/(dx*edgeY-dy*edgeX);
                FlailerPolyVertex point{};
                point.position[0]=first.position[0]+fraction*dx;
                point.position[1]=first.position[1]+fraction*dy;
                for (std::size_t i=0;i<3;++i)point.textureCoordinate[i]=first.textureCoordinate[i]+
                    fraction*(last.textureCoordinate[i]-first.textureCoordinate[i]);
                const float alpha=static_cast<float>(first.color>>24)+fraction*
                    static_cast<float>(static_cast<int32_t>(last.color>>24)-static_cast<int32_t>(first.color>>24));
                point.color=static_cast<std::uint32_t>(Byte(alpha))<<24;output.push_back(point);
            }
            previous=current;
        }
        Require(output.size()<=8,"TrackMap convex clip exceeded its source eight-vertex buffer");
        input.swap(output);
    }
    if (input.size()<3)return;
    std::size_t anchor=0;
    for (std::size_t i=1;i<input.size();++i) {
        const float y=input[i].position[1],best=input[anchor].position[1];
        if (y<best || (std::fabs(y-best)<=0.0010000000474974513f &&
            input[i].position[0]<input[anchor].position[0]))anchor=i;
    }
    for (std::size_t first=1;first<input.size()-1;) {
        auto rendered=MakePolygon();rendered.rasterState[2]=1;
        std::size_t last=first;
        for (std::size_t corner=0;corner<4;++corner) {
            const auto sequence=corner?std::min(first+corner-1,input.size()-1):0;
            last=corner?sequence:last;
            rendered.vertices[corner]=input[(anchor+sequence)%input.size()];
            rendered.vertices[corner].position[2]=static_cast<float>(depth);
            rendered.vertices[corner].color=(rendered.vertices[corner].color&0xff000000u)|
                (polygon.vertices[corner].color&0x00ffffffu);
        }
        renderer_.SubmitPolygon(resources_.StripPool(),texture,rendered,depth);first=last;
    }
}
void TrackMap::Render()
{
    auto* registry=player_?player_->registry:nullptr;
    auto* world=registry?registry->world:nullptr;
    auto* services=registry?registry->services:nullptr;
    auto* race=services?services->race:nullptr;
    if (!valid_)return;
    Require(world && race,"TrackMap render has no actual World/Race owners");
    target_=nullptr;activeMode_=configuredMode_;
    // MVP uses the authored rotating local view for the fixed on/off presets.
    if (playerHeading_ && (configuredMode_==7 || configuredMode_==8))activeMode_=2;
    auto* pursuit=services->pursuit;
    if (configuredMode_!=6 && pursuit) {
        auto* selected=pursuitManagerSelectedEntry(pursuit);
        if (selected && selected->group && selected->group->target) {
            target_=selected->group->target;activeMode_=10;
        }
    }
    std::uint16_t angle{};if (!GetAngle(angle))return;
    DrawOverOrUnderLay(background_,15);
    TrackMapPoint center{};TrackMapQuad bounds{};DrawMap(center,bounds,angle);
    const auto& mode=CurrentMode();
    float start[3]{race->startPosition[0],race->startPosition[1],0};
    DrawMarker(start,center,angle,bounds,markerColors[6],13,startLine_,mode.markerWidth,mode.markerHeight,
        static_cast<std::uint16_t>(track_->mapRotation-angle+track_->startLineRotation));
    if (parameters_.reverse) {
        float finish[3]{race->finishPosition[0],race->finishPosition[1],0};
        DrawMarker(finish,center,angle,bounds,markerColors[6],13,startLine_,mode.markerWidth,mode.markerHeight,
            static_cast<std::uint16_t>(track_->mapRotation-angle+track_->finishLineRotation));
    }
    const auto* texture=mode.orientedMarker?orientedMarker_:marker_;
    const auto heading=[&](const CarMovement& car) {
        return mode.orientedMarker?static_cast<std::uint16_t>(track_->mapRotation-
            car.bindings.state->heading-angle+0x4000u):std::uint16_t{0};
    };
    const auto drawCar=[&](CarMovement* movement,std::size_t color,int32_t depth,bool targetColor) {
        if (!MarkerVisible(movement))return;
        if (targetColor && movement==target_) {color=7;depth=3;}
        DrawMarker(movement->bindings.state->position,center,angle,bounds,markerColors[color],depth,
            texture,mode.markerSize,mode.markerSize,heading(*movement));
    };
    Require(world->firstGroupEnd>=0 && world->firstGroupEnd<=world->secondGroupEnd &&
        world->secondGroupEnd<=world->thirdGroupEnd && world->thirdGroupEnd<=world->totalCars &&
        world->totalCars<=WORLD_MAX_CARS,"TrackMap actual car table is invalid");
    for (int32_t i=world->firstGroupEnd;i<world->secondGroupEnd;++i) {
        Require(world->carTable[i],"TrackMap racer table contains no car");
        drawCar(world->carTable[i]->Movement(),3,5,true);
    }
    for (int32_t i=world->secondGroupEnd;i<world->thirdGroupEnd;++i) {
        Require(world->carTable[i],"TrackMap police table contains no car");
        drawCar(world->carTable[i]->Movement(),4,6,false);
    }
    if (pursuit)for (int32_t i=0;i<pursuitManagerRoadBlockCount(pursuit);++i) {
        const auto* block=pursuitManagerRoadBlockAt(pursuit,i);
        Require(block,"TrackMap roadblock count has no matching actual record");
        const auto rotation=static_cast<std::uint16_t>(track_->mapRotation-block->carHeadings[0]-angle+0x4000u);
        DrawMarker(block->groundPoint,center,angle,bounds,markerColors[8],7,roadBlock_,
            mode.markerWidth,mode.markerHeight,rotation);
    }
    if (race->ghostModel) {
        const auto* frame=race->ghostModel->frame;
        const auto rotation=mode.orientedMarker?static_cast<std::uint16_t>(track_->mapRotation-
            vectorAtan2Angle(frame[0],frame[1])-angle+0x4000u):std::uint16_t{0};
        DrawMarker(frame+12,center,angle,bounds,markerColors[5],4,texture,mode.markerSize,mode.markerSize,rotation);
    }
    const auto playerColor=[&](const CarMovement& car,std::size_t color) {
        Require(car.bindings.info,"TrackMap player has no actual CarInfo view");
        return car.bindings.info->bodyCategory==1?std::size_t{2}:color;
    };
    if (registry->count==1) {
        auto* player=registry->players[0];
        if (player && player->car && MarkerVisible(player->car))drawCar(player->car,playerColor(*player->car,0),1,true);
    } else if (registry->count==2) {
        auto* first=registry->playersByNumber[0];auto* second=registry->playersByNumber[1];
        Require(first && second && first->car && second->car,"Split TrackMap has no actual Players");
        const auto* firstEntry=raceFrameEntryForCar(first->car->bindings.owner);
        const auto* secondEntry=raceFrameEntryForCar(second->car->bindings.owner);
        Require(firstEntry && secondEntry,"Split TrackMap has no actual racing entries");
        int32_t firstDepth=1,secondDepth=2;if (secondEntry->position<firstEntry->position)std::swap(firstDepth,secondDepth);
        if (MarkerVisible(first->car))drawCar(first->car,playerColor(*first->car,0),firstDepth,true);
        if (MarkerVisible(second->car))drawCar(second->car,playerColor(*second->car,1),secondDepth,true);
    }
    DrawOverOrUnderLay(frame_,0);
}
}

// Source identities and raw PAL boundaries: ENVIRONMENT69_SOURCE_PROOF.json.
#include "indep/inc/eEnvMap.hpp"
#include "indep/inc/NativeRenderSettings.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace hp2 {
namespace {
// Released HackDirArray2D6E50 and HackDirArrayUp2D6F10, two actual banks.
constexpr float directions[2][6][4]={
    {{-1,0,0,0},{0,1,0,0},{1,0,0,0},{0,-1,0,0},{0,0,1,0},{0,0,-1,0}},
    {{0,1,0,0},{0,-1,0,0},{0,0,-1,0},{0,0,1,0},{-1,0,0,0},{1,0,0,0}}};
constexpr float upDirections[2][6][4]={
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{1,0,0,0},{-1,0,0,0}},
    {{-1,0,0,0},{-1,0,0,0},{-1,0,0,0},{-1,0,0,0},{0,0,-1,0},{0,0,1,0}}};
constexpr bool primarySchedule[4][6]={
    {1,1,0,0,0,1},{1,0,1,0,0,1},{1,0,0,1,0,1},{1,0,0,0,1,1}};

void rotationOnly(float matrix[16])
{
    matrix[3]=matrix[7]=matrix[11]=0.0f;
    matrix[12]=matrix[13]=matrix[14]=0.0f;matrix[15]=1.0f;
}

void environmentOrientation(float towardOrigin[16],float orientation[16],
    const float eye[4],const float origin[4])
{
    const float zero[4]{},reference[4]{1,0,0,0},up[4]{0,0,1,0};
    float baseline[16];
    carCinematicLookAt(baseline,zero,reference,up);rotationOnly(baseline);
    carCinematicLookAt(towardOrigin,eye,origin,up);rotationOnly(towardOrigin);
    eTransposeMatrix(orientation,baseline);eMulMatrix(orientation,towardOrigin,orientation);
    rotationOnly(orientation);eTransposeMatrix(orientation,orientation);
}

void environmentFaceMatrix(float output[16],const float origin[4],const float orientation[16],
    std::size_t bank,std::size_t face)
{
    float target[4],direction[4],cameraUp[4];
    eMulVector3(direction,orientation,directions[bank][face]);
    eMulVector3(cameraUp,orientation,upDirections[bank][face]);
    for(std::size_t axis=0;axis<4;++axis)target[axis]=origin[axis]+direction[axis];
    carCinematicLookAt(output,origin,target,cameraUp);
}

std::array<float,2> computeReflectionST(std::array<float,3> direction)
{
    // PAL1179B0..117A38. The source normalizes its temporary vector first.
    float square=direction[0]*direction[0]+direction[1]*direction[1];
    square+=direction[2]*direction[2];
    const float inverse=1.0f/std::sqrt(square);
    for(auto& axis:direction)axis*=inverse;
    const float z=direction[2]+1.0f;
    square=direction[0]*direction[0]+direction[1]*direction[1];
    square+=z*z;
    float denominator=std::sqrt(square);denominator+=denominator;
    return {{direction[0]/denominator+0.5f,direction[1]/denominator+0.5f}};
}
}

void BuildFrontendEnvironmentMatrix(float output[16],const float viewEye[3],const float origin[4])
{
    // EnvironmentMap::eyeOffset is zero. No face rendering is needed for the
    // frontend material's static CarSelectEnvMap texture.
    const float eye[4]{viewEye[0],viewEye[1],viewEye[2],0};
    float towardOrigin[16],orientation[16];
    environmentOrientation(towardOrigin,orientation,eye,origin);
    environmentFaceMatrix(output,origin,orientation,0,2);
}

EnvironmentMap::EnvironmentMap(CarCinematicServices& views,EcstasyRenderTarget* targets,std::size_t count)
    :views_(views)
{
    Require(targets && count>=15,"Environment map lacks the original fifteen render targets");
    mappedTarget_=&targets[14];
    Require(mappedTarget_->id==14 && mappedTarget_->width==256 && mappedTarget_->height==256 &&
        mappedTarget_->format==0x32 && !mappedTarget_->opaqueOnly,
        "Environment map target differs from released SetScreenBuffers");
    CreateReflectionSamples();
    for(std::size_t index=0;index<6;++index) {
        auto& view=views_.screens[index+4];auto& target=targets[index+8];
        Require(target.id==static_cast<std::int32_t>(index+8) && target.width==64 && target.height==64 &&
            target.format==0x32 && target.opaqueOnly,
            "Environment face differs from released SetScreenBuffers");
        carCinematicViewInitialize(&views_,&cameras_[index]);
        // eInitEnvMap117508..1175BC binds the existing view records.
        view.enabled=1;
        view.id=static_cast<std::int32_t>(index+4);
        view.view=&cameras_[index];view.target=&target;target.enabled=1;
    }
}

EnvironmentMap::~EnvironmentMap()
{
    // Native RAII revokes only our six borrowed cameras after GPU retirement.
    for(std::size_t index=0;index<6;++index) {
        auto& view=views_.screens[index+4];
        if(view.view==&cameras_[index]) {view.view=nullptr;view.enabled=0;}
    }
}

void EnvironmentMap::CreateReflectionSamples()
{
    // PAL117A38..11814C: the original 7x7 storage has six initialized rows
    // and columns. Keep that stride; the emitted strips use only 6x6.
    for(std::size_t face=0;face<6;++face)for(std::size_t row=0;row<6;++row)
        for(std::size_t column=0;column<6;++column) {
            const float x=0.4000000059604645f*static_cast<float>(row);
            const float y=0.4000000059604645f*static_cast<float>(column);
            auto& sample=samples_[face][row*7+column];
            switch(face) {
            case 0:sample.direction={{x-1.0f,y-1.0f,1.0f}};break;
            case 1:sample.direction={{1.0f,y-1.0f,1.0f-x}};break;
            case 2:sample.direction={{1.0f-x,y-1.0f,-1.0f}};break;
            case 3:sample.direction={{-1.0f,y-1.0f,x-1.0f}};break;
            case 4:sample.direction={{x-1.0f,-1.0f,y-1.0f}};break;
            default:sample.direction={{x-1.0f,1.0f,1.0f-y}};break;
            }
            sample.mappedST=computeReflectionST(sample.direction);
            sample.faceST={{x*0.5f,y*0.5f}};
        }
    for(std::size_t patch=0;patch<4;++patch)for(std::size_t row=0;row<6;++row) {
        const auto& edge=patch==0 ? samples_[4][row*7]:patch==1 ? samples_[1][5*7+row]:
            patch==2 ? samples_[5][row*7+5]:samples_[3][row];
        std::array<float,2> radial{{edge.mappedST[0]-0.5f,edge.mappedST[1]-0.5f}};
        vectorNormalize2(radial.data(),radial.data());
        radial[0]*=0.5f;radial[1]*=0.5f;
        const float t=static_cast<float>(row)/5.0f;
        const std::array<float,2> start=patch==0 ? std::array<float,2>{{1.0f-t,0.0f}}:
            patch==1 ? std::array<float,2>{{0.0f,t}}:
            patch==2 ? std::array<float,2>{{1.0f-t,1.0f}}:std::array<float,2>{{1.0f,t}};
        for(std::size_t column=0;column<6;++column) {
            auto& sample=backSamples_[patch][row*7+column];
            const float factor=static_cast<float>(column)/5.0f;
            for(std::size_t axis=0;axis<2;++axis) {
                float delta=radial[axis]*factor;delta*=0.1599999964237213f;
                // Source preserves the subtract/add0.5 rounding on the edge.
                sample.mappedST[axis]=(edge.mappedST[axis]-0.5f)+0.5f+delta;
                sample.faceST[axis]=start[axis]+(0.5f-start[axis])*factor;
            }
        }
    }
}

const EnvironmentReflectionSample& EnvironmentMap::Sample(std::size_t face,std::size_t row,std::size_t column) const
{
    Require(face<6 && row<6 && column<6,"Reflection sample lies outside its original grid");
    return samples_[face][row*7+column];
}
const EnvironmentReflectionSample& EnvironmentMap::BackSample(std::size_t patch,std::size_t row,std::size_t column) const
{
    Require(patch<4 && row<6 && column<6,"Back reflection sample lies outside its original grid");
    return backSamples_[patch][row*7+column];
}

void EnvironmentMap::UpdateCameras(const float viewEye[3],const float reflectionOrigin[3],
    std::int32_t applicationState,float realTimeElapsed)
{
    // PAL1175C8..1178E8 defines the six-camera order at the source display
    // cadence. Advance that visual schedule by elapsed time on PC; repeated
    // high-FPS presentation must not multiply the reflection workload.
    Require(views_.clock && std::isfinite(realTimeElapsed) && realTimeElapsed>=0.0f,
        "Environment schedule has no finite real clock interval");
    float eye[4]{viewEye[0],viewEye[1],viewEye[2],0};
    float origin[4]{reflectionOrigin[0],reflectionOrigin[1],reflectionOrigin[2],0};
    float towardOrigin[16],orientation[16];
    environmentOrientation(towardOrigin,orientation,eye,origin);
    std::copy(towardOrigin,towardOrigin+16,bodyMatrix_.begin());
    const std::size_t bank=applicationState==3 ? 0:1;
    const bool primary=views_.presentationLayout==1 || views_.presentationLayout==2;
    std::array<bool,6> due{};
    const auto select=[&](std::uint32_t phase) {
        for(std::size_t face=0;face<6;++face)
            due[face]=due[face] || (primary ? primarySchedule[phase%4u][face]:phase%6u==face);
    };
    if(!scheduleInitialized_) {
        scheduleInitialized_=true;select(schedulePhase_);
    } else {
        const double frequency=gameTimeGetVideoMode(views_.clock)==HP2_VIDEO_PAL ? 50.0:60.0;
        scheduleRemainder_+=static_cast<double>(realTimeElapsed)*frequency;
        const double elapsedPhases=std::floor(scheduleRemainder_+1.0e-7);
        scheduleRemainder_=std::max(0.0,scheduleRemainder_-elapsedPhases);
        // A low-FPS frame renders the union of due faces, with each image
        // using the latest pose. Six phases already cover the whole bank.
        const auto count=static_cast<std::uint32_t>(std::min(elapsedPhases,6.0));
        for(std::uint32_t index=1;index<=count;++index)select(schedulePhase_+index);
        schedulePhase_=(schedulePhase_+static_cast<std::uint32_t>(std::fmod(elapsedPhases,12.0)))%12u;
        if(primary!=previousPrimary_)select(schedulePhase_);
    }
    previousPrimary_=primary;
    for(std::size_t face=0;face<6;++face) {
        cameraElapsed_[face]+=realTimeElapsed;
        auto& view=views_.screens[face+4];view.enabled=static_cast<std::uint8_t>(due[face]);
        if(!view.view)continue;
        // Face2 additionally supplies the body's current environment matrix;
        // it must follow the car even between texture refresh intervals.
        if(!view.enabled && face!=2)continue;
        float matrix[16];
        environmentFaceMatrix(matrix,origin,orientation,bank,face);
        if(face==2)std::copy(matrix,matrix+16,bodyMatrix_.begin());
        if(!view.enabled)continue;
        carCinematicViewPublish(&views_,view.view,matrix,cameraElapsed_[face]);
        cameraElapsed_[face]=0.0f;
        if(!views_.suppressPublication) {
            view.view->resetHistory=0;view.view->current.minimumVisibility=0.0f;
        }
        view.view->current.nearClip=0.5f;view.view->current.farClip=10000.0f;
        if(!views_.suppressPublication)view.view->current.fieldOfView=0x75e4;
    }
}

void EnvironmentMap::CreateFacePrimitive(EcstasyFrame& frame,const CarCinematicScreen& view,
    const EnvironmentReflectionSample* samples) const
{
    // PAL118150/118498 quantize to whole pixels before emitting five
    // 12-vertex strips. The GPU samples the face attachment after its fence
    // dependency; no CPU image or substitute texture is created.
    Require(view.target,"Environment face has no actual render target");
    const float width=static_cast<float>(mappedTarget_->width),height=static_cast<float>(mappedTarget_->height);
    for(std::size_t row=0;row<5;++row) {
        std::vector<FlailerPolyVertex> vertices;vertices.reserve(12);
        for(std::size_t column=0;column<6;++column)for(std::size_t pair=0;pair<2;++pair) {
            const auto& sample=samples[(row+pair)*7+column];
            FlailerPolyVertex vertex;
            // All authored samples fit the target. Remove the former
            // GS2048/12.4 address round trip while retaining its pixel grid.
            vertex.position={{std::floor(sample.mappedST[0]*width),
                std::floor(sample.mappedST[1]*height),0,1}};
            vertex.textureCoordinate={{sample.faceST[0],sample.faceST[1],1}};
            vertex.color=0x40606060u;vertices.push_back(vertex);
        }
        frame.RenderTargetStrip(vertices,*view.target,*mappedTarget_);
    }
}

void EnvironmentMap::CreatePrimitives(EcstasyFrame& frame) const
{
    // PAL118848..118880 excludes face2 from the ordinary loop. The four
    // back patches follow ALL ordinary faces (including5), never split it.
    for(std::size_t face=0;face<6;++face) {
        if(face==2)continue;
        const auto& view=views_.screens[face+4];
        if(!view.enabled || !view.target || !view.target->enabled)continue;
        CreateFacePrimitive(frame,view,samples_[face].data());
    }
    const auto& back=views_.screens[6];
    if(back.enabled && back.target && back.target->enabled)
        for(std::size_t patch=0;patch<4;++patch)CreateFacePrimitive(frame,back,backSamples_[patch].data());
}
}

#include "game_core/port/frontend/selection_camera.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
namespace {
void require(bool result,const char* message){if(!result)throw std::runtime_error(message);}
void near(float a,float b,float tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
}
int main(){
    using namespace hp2::debug;
    std::array<float,16> matrix{};
    bQuaternionToMatrix(matrix,{0,1,0,0});
    require(matrix[0]==-1 && matrix[5]==1 && matrix[10]==-1 && matrix[15]==1,"original quaternion matrix axis signs changed");
    bQuaternion from;bMatrixToQuaternion(from,matrix);
    require(from.y==1 && from.x==0 && from.z==0 && from.w==0,"negative-trace quaternion branch differs");
    bMatrixToQuaternion(from,hp2::recovered::bIdentity());require(from.w==1,"positive-trace quaternion branch differs");
    bQuaternion product{0,0,0,1},half_turn{0,1,0,0};
    MyQuaternionMultiply(product,half_turn,product);
    require(product.y==1 && product.w==0,"in-place quaternion multiply corrupted source components");
    product={0,0,0,2};NormalizeQuaternion(product);require(product.w==1,"original normalization differs");
    auto view=hp2::recovered::bIdentity();view[12]=2;view[13]=-3;view[14]=7;
    const auto plain=port::selection::camera_frame_from_view(view,4500,5);
    require(plain.position==std::array<float,3>{-2,3,-7} && plain.target==std::array<float,3>{-2,3,-2}
        && plain.up==std::array<float,3>{0,-1,0},"view matrix was mistaken for world camera transform");
    const auto image=port::driving::debug_image_path(".");
    if(image.empty() || !std::filesystem::exists(image)){std::puts("PASS selection quaternion/projection; SKIP original Debug image");return 0;}
    auto memory=port::driving::read_debug_image(image);
    const auto defaults=port::selection::load_lighting_defaults(memory);
    std::array<SelectCarLights,2> players{};
    SetSelectCarLighting(players,1,.25f,0,defaults);
    require(players[0].Light0.Colour[0]==87.5f && players[0].LightA.Colour[0]==32,
        "selection light colour was integer-truncated or ambient was intensity-shifted");
    SetSelectCarLighting(players,2,100,0,defaults);
    require(players[1].Light2.Colour[2]==255 && players[0].Light0.Colour[0]==87.5f,
        "player lighting separation or upper clamp differs");
    const auto identity=hp2::recovered::bIdentity();
    const auto light_packet=hp2::debug::elSetupLightContext(players[0],identity,identity);
    require(light_packet[0]==87.5f && light_packet[12]==32 && light_packet[15]==128 && light_packet[19]==0,
        "selection VU light packet colour/alpha/padding differs");
    auto translated_view=identity,translated_part=identity;translated_view[12]=99;translated_part[14]=-123;
    require(light_packet==hp2::debug::elSetupLightContext(players[0],translated_view,translated_part),
        "selection light directions incorrectly subtract world/camera position");
    const auto d=defaults.normal.Light0.Position;
    const float length=std::sqrt((d[0]*d[0]+d[1]*d[1])+d[2]*d[2]);
    near(light_packet[16],d[0]/length,.000001f,"selection light first axis was transposed incorrectly");
    auto rotated_view=identity;rotated_view[0]=0;rotated_view[1]=1;rotated_view[4]=-1;rotated_view[5]=0;
    const auto rotated_packet=hp2::debug::elSetupLightContext(players[0],rotated_view,identity);
    near(rotated_packet[16],d[1]/length,.000001f,"selection lighting omitted inverse-view rotation");
    near(rotated_packet[20],-d[0]/length,.000001f,"selection inverse-view rotation uses host handedness");
    SetSelectCarLighting(players,1,0,1,defaults);
    require(players[0].Light0.Colour==std::array<float,3>{} && players[0].LightA.Colour==std::array<float,3>{},
        "silhouette leaked normal/ambient colour");
    std::array<CarTypeInfo,64> cars{};auto& test_car=cars[0];std::snprintf(test_car.CarTypeName.data(),32,"TEST");
    test_car.NumAvailableSkinNumbers=2;test_car.AvailableSkinNumbers={2,4};test_car.DefaultSkinNumber=4;
    for(int i=0;i<4;++i){test_car.Wheels[i].Position={{float(i),float(i+1),-.1f}};test_car.Wheels[i].Radius=.3f;}
    FrontendPlayerInfo player;player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,0,-1,false,cars,0);
    const port::selection::ListServices list_services{cars,{},[](int){return std::string("fixture");},false};
    VisibleCarSet list(player,9,false,list_services);CarSelector selector{list};
    FrontEndRenderingCar main_car,reflection;SelectCarCameraMover pose_camera;
    selector.pFrontEndRenderingCar=&main_car;selector.pFrontEndRenderingCarReflection=&reflection;
    selector.LoadedCar=list.pCurrentVisibleCar;selector.pSelectCarCameraMover=&pose_camera;
    selector.LightsOn=1;selector.CopLightsOn=1;
    selector.UpdateRenderingCarParameters(memory,main_car,0,cars);
    selector.UpdateRenderingCarParameters(memory,reflection,1,cars);
    near(main_car.Position[2],(.3f-(-.1f))+memory.scalar(0x326c90),0,"original showcase height differs");
    require(main_car.Position[0]==0 && main_car.Position[1]==0 && reflection.Position[2]==-main_car.Position[2]
        && reflection.BodyMatrix[10]==-1 && main_car.BodyMatrix[10]==1,"selection reflection used wrong axis/height");
    require(main_car.LightsOn && main_car.CopLightsOn,"original showroom light flags were dropped");
    for(int i=0;i<4;++i)require(main_car.TireMatrices[i][12]==float(i) && main_car.BrakeMatrices[i][13]==float(i+1)
        && main_car.TireMatrices[i][15]==1,"original wheel translation/homogeneous lane differs");
    const auto steered=main_car.TireMatrices[0];pose_camera.ModeParam=1;
    selector.UpdateRenderingCarParameters(memory,main_car,0,cars);
    require(steered!=main_car.TireMatrices[0] && main_car.BrakeMatrices[2]==reflection.BrakeMatrices[2],
        "front steering did not fade with camera mode or rear wheels were steered");
    std::vector<int> reinit_order;
    const port::selection::RenderServices render_services{
        [&](int type,int skin,int fallback){require(type==0 && skin==4 && fallback==0,"ShowLoadedCar resource skin lookup differs");reinit_order.push_back(0);},
        [&](auto& target,const auto& ride){require(ride.Type==0 && ride.SkinNumber==4 && !ride.HasDash && !ride.CanBeVertexDamaged,
            "ShowLoadedCar fabricated ride/dashboard state");reinit_order.push_back(&target==&main_car?1:2);}};
    selector.ShowLoadedCar(render_services);require(reinit_order==std::vector<int>{0,1,2},"main/reflection ReInit call order differs");
    list.pCurrentVisibleCar->CarUnlocked=false;list.pCurrentVisibleCar->pCarStatus->nStatus=eCAR_LOCKED_CLOAKED;
    selector.SetupCarOverrideModel();require(main_car.OverrideModel && main_car.OverrideModel==reflection.OverrideModel
        && main_car.OverrideModel->NameHash==hp2::frontend::bStringHash("CARCOVER"),"cloaked override missing from main/reflection");
    selector.AllCarsUnlocked=1;selector.SetupCarOverrideModel();require(!main_car.OverrideModel && !reflection.OverrideModel,"unlock did not remove original cover override");
    auto scaled=identity;scaled[0]=2;scaled[5]=3;scaled[10]=4;
    main_car.BodyMatrix=identity;main_car.BodyMatrix[0]=0;main_car.BodyMatrix[1]=1;
    main_car.BodyMatrix[4]=-1;main_car.BodyMatrix[5]=0;main_car.Position={{10,20,30}};
    const auto normal_root=port::selection::frontend_render_matrix(main_car,scaled,{{2,-1,-1}},{{6,1,1}});
    require(normal_root[12]==6 && normal_root[13]==20 && normal_root[14]==30
        && normal_root[1]==2 && normal_root[4]==-3,"frontend bbox correction was rotated or scale multiplication order changed");
    main_car.OverrideModel=&selector.CloakedCarModel;
    require(port::selection::frontend_render_matrix(main_car,scaled,{{2,-1,-1}},{{6,1,1}})==main_car.BodyMatrix,
        "cover override incorrectly applied showcase height, car scale or bounds centering");
    // Exact original Slerp sign mutation; do not silently replace with host Quaternion.slerp.
    bQuaternion a{0,0,0,-1},b{0,0,0,1},out;
    Slerp(memory,out,a,b,.4f);require(a.w==1 && out.w==1,"original Slerp must mutate opposite first quaternion");
    SelectCarCameraMover camera;camera.RadiusStats=12.930000305175781f;
    camera.OffsetStats={{.2739799916744232f,0,-.5299999713897705f}};
    camera.OffsetOrbit={{0,0,-.6384000182151794f}};
    camera.SetOrientationStats(memory,0,0x3b9e);
    require(camera.VAngleStats==static_cast<std::uint16_t>(0u-0x3b9e),"negative unsigned stats angle was lost");
    std::vector<port::selection::CameraAudioRequest> sounds;sounds.reserve(1000);
    port::selection::CameraFrame frame;int published=0;
    const port::selection::CameraServices services{true,
        [&](const auto& request){sounds.push_back(request);},
        [&](const auto& m,float,int fov,float distance){frame=port::selection::camera_frame_from_view(m,fov,distance);++published;}};
    camera.Update(memory,1.f/60,services);
    require(frame.fov_bangle==4500 && published==1 && camera.ModeParam==0 && camera.VAngleOrbit==0xc350,
        "stats reset/FOV publication differs");
    near(frame.target_distance,camera.RadiusStats,0,"stats output used initial orbit radius");
    // Camera eye must transform to view origin using the authored matrix.
    for(int lane=0;lane<3;++lane){const auto& m=frame.view;const auto& eye=frame.position;
        const float at_eye=((m[lane]*eye[0]+m[4+lane]*eye[1])+m[8+lane]*eye[2])+m[12+lane];
        near(at_eye,0,.0001f,"inverse camera pose does not map to authored view origin");}
    camera.SetMode(SelectCarCameraMover::SCC_ORBIT,services);
    require(sounds.size()==2 && sounds[0].operation==port::selection::CameraAudioOp::play
        && std::string(sounds[0].sample)=="MenuCarDisplayActive" && sounds[0].value==0
        && std::string(sounds[1].sample)=="MenuCarDisplayOn","orbit entry sample call order differs");
    const auto counter=memory.word(0x3274b8);
    camera.Update(memory,1.f/60,services);
    // Matrix uses old ModeParam0; only after publication math does state become .03.
    near(frame.target_distance,camera.RadiusStats,0,"mode parameter advanced before camera matrix generation");
    near(camera.ModeParam,memory.scalar(0x325074),0,"mode increment is not the original fixed step");
    require(memory.word(0x3274b8)==counter+1,"shared original quaternion count was not advanced");
    const auto pitch=sounds.back();require(pitch.operation==port::selection::CameraAudioOp::pitch,"orbit pitch update missing");
    near(pitch.value,(((camera.Radius-memory.scalar(0x325060))*memory.scalar(0x325064))*memory.scalar(0x325068)
        +memory.scalar(0x32506c))-1.f,0,"pitch used decompiler pointer cast instead of original float load");
    // Deliberately different elapsed values: transition is per Update, not an invented delta tween.
    camera.Update(memory,.001f,services);near(camera.ModeParam,2*memory.scalar(0x325074),0,"mode used elapsed tween");
    camera.ModeParam=1;camera.VAngleOrbit=0xebab;camera.VOrbitSpeed=1000;camera.DesiredVOrbitSpeed=1000;
    camera.Radius=15.5f;camera.RadiusSpeed=4;camera.DesiredRadiusSpeed=4;
    camera.Update(memory,.01f,services);
    require(camera.VAngleOrbit==0xebab && camera.VOrbitSpeed<0 && camera.Radius==15.5f && camera.RadiusSpeed<0,
        "upper orbit clamps/reflection differ");
    camera.VAngleOrbit=0xc350;camera.VOrbitSpeed=-1000;camera.DesiredVOrbitSpeed=-1000;
    camera.Radius=memory.scalar(0x325044);camera.RadiusSpeed=-4;camera.DesiredRadiusSpeed=-4;
    camera.Update(memory,.01f,services);
    require(camera.VAngleOrbit==0xc350 && camera.VOrbitSpeed>0 && camera.Radius==memory.scalar(0x325044)
        && camera.RadiusSpeed>0,"lower orbit clamps/reflection differ");
    SelectCarCameraMover second;second.SetOrientationStats(memory,0,0x3b9e);
    second.Mode=SelectCarCameraMover::SCC_ORBIT;
    const auto shared_count=memory.word(0x3274b8);second.Update(memory,.01f,services);
    require(memory.word(0x3274b8)==shared_count+1,"second selector incorrectly reset global normalization cadence");
    camera.SetMode(SelectCarCameraMover::SCC_STATS,services);
    require(std::string(sounds[sounds.size()-2].sample)=="MenuCarDisplayOff"
        && sounds.back().operation==port::selection::CameraAudioOp::stop,"stats entry audio order differs");
    for(int i=0;i<40;++i)camera.Update(memory,.001f,services);
    require(camera.ModeParam==0 && frame.fov_bangle==4500,"return to stats did not settle at original mode boundary");
    require(!hp2::guard([&]{camera.Update(memory,.01f,{});}).empty(),"missing publication service became a silent camera no-op");
    std::puts("PASS original Debug selection camera clocks, clamps, inverse view, Slerp mutation, sample order and shared cadence");
}

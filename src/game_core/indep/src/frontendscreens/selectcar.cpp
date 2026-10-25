// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/frontendscreens/selectcar.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/input/joyevents.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include "game_core/port/driving/projections.hpp"
#include "game_core/src/bMath.hpp"

namespace port::selection::widget_projection {
// Port cache of immutable original THUMBNAIL_CLOAK_%d hashes. No per-frame strings.
const std::array<std::uint32_t,25>& cloak_hashes(){
    static const auto values=[](){std::array<std::uint32_t,25> out{};char name[32];
        for(int i=0;i<25;++i){std::snprintf(name,sizeof(name),"THUMBNAIL_CLOAK_%d",i);out[i]=hp2::frontend::bStringHash(name);}
        return out;}();
    return values;
}
// Debug00144188 ->00140810/38, E-0098-U02. Missing widgets are original no-ops.
void position(WidgetContext& ctx,std::uint32_t hash,int x,int y){
    if(auto* w=hp2::frontend::FindWidget_00143D50(ctx.screen,hash,0)){
        if(w->x!=float(x)){w->x=float(x);w->dirty_0x2b=4;}
        if(w->y!=float(y)){w->y=float(y);w->dirty_0x2b=4;}
    }
}
// Debug00144818 ->00141140, E-0098-U02; cached texture ownership is the port catalog.
void texture(WidgetContext& ctx,std::uint32_t hash,std::uint32_t value){
    if(auto* w=hp2::frontend::FindWidget_00143D50(ctx.screen,hash,2)){w->texture_hash=value;w->dirty_0x2b=10;}
}
// Debug00144758, E-0098-U02; reviewed semantic match to existing Retail visibility.
void visible(WidgetContext& ctx,std::uint32_t hash,bool value){hp2::frontend::SetWidgetVisible_00144968(ctx.screen,hash,value);}
// Debug00143F80, E-0098-U02. Width service supplies original scaled texture width.
int width(WidgetContext& ctx,std::uint32_t hash){
    const auto* w=hp2::frontend::FindWidget_00143D50(ctx.screen,hash,0);if(!w)return 0;
    if(!ctx.measure)hp2::not_implemented("0x00143F80","original scaled widget measurement service missing");
    return port::driving::cvt_w_s(ctx.measure(*w).width);
}
std::string car_code(const hp2::debug::CarTypeInfo& car,const char* address){
    const auto end=std::find(car.CarTypeName.begin(),car.CarTypeName.end(),'\0');
    if(end==car.CarTypeName.end())hp2::not_implemented(address,"unterminated original car code");
    return {car.CarTypeName.begin(),end};
}
hp2::debug::VisibleCar& current(const hp2::debug::CarSelector& selector,const char* address){
    auto* car=selector.VisibleCars.pCurrentVisibleCar;
    if(!car || !car->pCarStatus)hp2::not_implemented(address,"missing original current car/status");
    return *car;
}
bool cloaked(const hp2::debug::CarSelector& selector,const hp2::debug::VisibleCar& car){
    return !car.CarUnlocked && !selector.AllCarsUnlocked && car.pCarStatus->nStatus==hp2::debug::eCAR_LOCKED_CLOAKED;
}
}

namespace port::selection {
// Debug00196688, E-0098-T01. No screen registration or other initializer claim.
void initialize_camera_offsets(::port::driving::Memory& m){
    m.scalar(0x307970,m.scalar(0x325608));m.scalar(0x307974,0);m.scalar(0x307978,m.scalar(0x32560c));
    m.scalar(0x32b140,m.scalar(0x325610));m.scalar(0x32b144,0);m.scalar(0x32b148,m.scalar(0x325614));
    m.scalar(0x32b130,0);m.scalar(0x32b134,0);m.scalar(0x32b138,m.scalar(0x325618));
}
// E-0098-R01. Port projection of the reviewed frontend/render matrix handoff.
std::array<float,16> frontend_render_matrix(const hp2::debug::FrontEndRenderingCar& car,
    const std::array<float,16>& scale,const std::array<float,3>& low,const std::array<float,3>& high){
    if(car.OverrideModel)return car.BodyMatrix;
    std::array<float,16> result;hp2::recovered::eMulMatrix(result,scale,car.BodyMatrix);
    result[12]=car.Position[0]-(low[0]+(high[0]-low[0])*.5f);
    result[13]=car.Position[1];result[14]=car.Position[2];result[15]=1;
    return result;
}
}

namespace hp2::debug {
// Debug001947D0, E-0098-T01; full instruction order in lifecycle/0.json.
// Dependency bodies remain explicit services; no synthetic loader completions.
void CarSelector::Tick(const ::port::selection::TickServices& svc){
    using namespace ::port::selection::widget_projection;
    auto& m=svc.memory;
    current(*this,"0x001947D0");
    if(!pSelectCarCameraMover || !pFrontEndRenderingCar || !pFrontEndRenderingCarReflection)
        hp2::not_implemented("0x001947D0","original selector camera/main/reflection binding missing");
    if(LoadingCar){
        if(!svc.is_loaded)hp2::not_implemented("0x001947D0","original CarLoader::IsLoaded binding missing");
        if(svc.is_loaded(LoadingCar->eCarType)){
            LoadedCar=LoadingCar;LoadingCar=nullptr;
            if(LoadedCar==VisibleCars.pCurrentVisibleCar){SetupCarOverrideModel();ShowLoadedCar(svc.rendering);}
        }
    }
    if(LoadedCar && LoadedCar!=VisibleCars.pCurrentVisibleCar
        && FadeCarOverlayAlpha==CAR_FADED_ALPHA && ThumbnailHorizCubic.state==0){
        if(!svc.unload || !svc.load || !svc.begin_loading)
            hp2::not_implemented("0x001947D0","original CarLoader unload/load/begin binding missing");
        LoadingCar=VisibleCars.pCurrentVisibleCar;
        svc.unload(LoadedCar->eCarType);LoadedCar=nullptr;
        svc.load(LoadingCar->eCarType);svc.begin_loading();
    }
    if(LoadedCar==VisibleCars.pCurrentVisibleCar){
        if(FadeCarOverlayAlpha==CAR_FADED_ALPHA)ShowLoadedCar(svc.rendering);
        SetupCarOverrideModel();FadeCarOverlayAlpha=std::max(0,FadeCarOverlayAlpha-3);
    }else{
        FadeCarOverlayAlpha+=7;
        // Equality does not run SetupCarOverrideModel: original signed SLT is strict.
        if(FadeCarOverlayAlpha>CAR_FADED_ALPHA){FadeCarOverlayAlpha=CAR_FADED_ALPHA;SetupCarOverrideModel();}
    }
    if(auto* w=hp2::frontend::FindWidget_00143D50(svc.widgets.screen,0x75c8b565,0))
        hp2::frontend::SetAlpha(*w,FadeCarOverlayAlpha);
    const auto elapsed_frames=static_cast<std::int32_t>(std::uint32_t(svc.orbit.real_time_frames)-std::uint32_t(OrbitActiveTime));
    if(SelectorMode==SELECTOR_MODE_ORBIT && elapsed_frames>m.signed_word(0x327b20))EnterChooseMode(svc.orbit);
    if(SelectorMode==SELECTOR_MODE_CHOOSE || !ManualOrbitEngaged)
        CarRotationZ=static_cast<std::uint16_t>(CarRotationZ+m.half(0x327ad2));
    auto& camera=*pSelectCarCameraMover;
    camera.RadiusStats=m.scalar(IsTwoPlayerScreen?0x327b1c:0x327b18);
    const auto offset=IsTwoPlayerScreen?0x32b140u:0x307970u;
    for(int i=0;i<3;++i){camera.OffsetStats[i]=m.scalar(offset+i*4);camera.OffsetOrbit[i]=m.scalar(0x32b130+i*4);}
    camera.SetOrientationStats(m,m.half(0x327ad4),IsTwoPlayerScreen?0x3d26:m.half(0x327b14));
    camera.FOVStats=StatCamFOV;
    if(SelectorMode<DesiredSelectorMode){SelectorMode+=m.scalar(0x3255d4);if(SelectorMode>DesiredSelectorMode)SelectorMode=DesiredSelectorMode;}
    if(SelectorMode>DesiredSelectorMode){SelectorMode-=m.scalar(0x3255d8);if(SelectorMode<DesiredSelectorMode)SelectorMode=DesiredSelectorMode;}
    UpdateRenderingCarParameters(m,*pFrontEndRenderingCar,0,svc.widgets.cars);
    UpdateRenderingCarParameters(m,*pFrontEndRenderingCarReflection,1,svc.widgets.cars);
    UpdateFadeGroups(svc.fade,svc.orbit_fade,svc.snap_fade,svc.widgets.cars);
    const auto& selected=current(*this,"0x001947D0");
    const int skin=selected.pCarStatus->nSkins==1?0:selected.SkinIndex;
    if(skin<0 || skin>=10)hp2::not_implemented("0x001947D0","swatch index outside original array");
    if(!svc.swatch_rgb)hp2::not_implemented("0x001947D0","original swatch texture palette binding missing");
    const auto rgb=svc.swatch_rgb(SwatchTextureHashes[skin]); // Original lookup precedes cloak check.
    float intensity=0;
    if(!cloaked(*this,selected) && rgb){
        intensity=1.f-((float((*rgb)[0])*m.scalar(0x3255dc)+float((*rgb)[1])*m.scalar(0x3255e0))
            +float((*rgb)[2])*m.scalar(0x3255e4))*m.scalar(0x3255e8);
        intensity*=intensity;intensity*=intensity;
    }
    int silhouette=0;
    if(!svc.all_cars_available && !AllCarsUnlocked && LoadedCar && !LoadedCar->CarUnlocked)
        silhouette=LoadedCar->pCarStatus->nStatus==eCAR_LOCKED_PARTLY_VISIBLE;
    if(!svc.set_lighting)hp2::not_implemented("0x001947D0","original SetSelectCarLighting binding missing");
    svc.set_lighting(ViewID,intensity,silhouette);
    if(!svc.update_locked)hp2::not_implemented("0x001947D0","original CarLockedDisplayer::Update binding missing");
    svc.update_locked();
    visible(svc.widgets,0xabb370a6,VisibleCars.GetNextCar(VisibleCars.pCurrentVisibleCar)!=VisibleCars.pCurrentVisibleCar || ThumbnailHorizCubic.state!=0);
    visible(svc.widgets,0x84f544f3,VisibleCars.GetPrevCar(VisibleCars.pCurrentVisibleCar)!=VisibleCars.pCurrentVisibleCar || ThumbnailHorizCubic.state!=0);
    UpdateSwatches(svc.widgets);
    // Original UpdateSpotlight00193AE0 is JR RA/NOP, confirmed by fresh listing.
    UpdateThumbnails(svc.widgets,svc.elapsed);
    if(!svc.update_nebula)hp2::not_implemented("0x001947D0","original UpdateNebula binding missing");
    svc.update_nebula();
}
// Debug001936E8, E-0098-R01. Uses selected car even while another is displayed.
void CarSelector::SetupCarOverrideModel(){
    const auto& car=::port::selection::widget_projection::current(*this,"0x001936E8");
    if(!pFrontEndRenderingCar || !pFrontEndRenderingCarReflection)
        hp2::not_implemented("0x001936E8","original main/reflection car binding missing");
    const eModel* model=::port::selection::widget_projection::cloaked(*this,car)?&CloakedCarModel:nullptr;
    pFrontEndRenderingCar->OverrideModel=model;pFrontEndRenderingCarReflection->OverrideModel=model;
}
// Debug00195AF0, E-0098-R01. Validate skin then reinit main/reflection in that order.
void CarSelector::ShowLoadedCar(const ::port::selection::RenderServices& svc){
    if(!LoadedCar || !LoadedCar->pCarStatus || LoadedCar->SkinIndex<0 || LoadedCar->SkinIndex>=10
        || !pFrontEndRenderingCar || !pFrontEndRenderingCarReflection || !svc.get_skin_info || !svc.reinit)
        hp2::not_implemented("0x00195AF0","original loaded car/skin/reinit binding missing");
    RideInfo ride;ride.Type=LoadedCar->eCarType;ride.SkinNumber=LoadedCar->pCarStatus->aSkins[LoadedCar->SkinIndex];
    svc.get_skin_info(ride.Type,ride.SkinNumber,0);
    svc.reinit(*pFrontEndRenderingCar,ride);svc.reinit(*pFrontEndRenderingCarReflection,ride);
}
// Debug001957D8 full listing, E-0098-R01. Position is original Z-up, not host Y-up.
void CarSelector::UpdateRenderingCarParameters(port::driving::Memory& m,FrontEndRenderingCar& car,
    int mirror_image,const std::array<CarTypeInfo,64>& cars){
    if(!LoadedCar)return;
    if(LoadedCar->eCarType<0 || LoadedCar->eCarType>=64 || !pSelectCarCameraMover)
        hp2::not_implemented("0x001957D8","original selected car/camera binding missing");
    const auto& info=cars[LoadedCar->eCarType];
    float height=(info.Wheels[0].Radius-info.Wheels[0].Position[2])+m.scalar(0x326c90);
    if(mirror_image)height=-height;
    car.LightsOn=LightsOn;car.CopLightsOn=CopLightsOn;car.Position={{0,0,height}};
    auto body=hp2::recovered::bIdentity();
    if(mirror_image){body[1]=-body[1];body[10]=-body[10];}
    hp2::recovered::eRotateZ(m,body,body,CarRotationZ);car.BodyMatrix=body;
    const auto angle=m.half(0x327ad6);const float mode=pSelectCarCameraMover->ModeParam;
    const float steer=angle<0x8001?(1.f-mode)*float(angle):float(angle)+mode*float(0x10000u-angle);
    for(int i=0;i<4;++i){
        auto tire=hp2::recovered::bIdentity(),brake=tire;
        hp2::recovered::eRotateY(m,tire,tire,0);
        if(i<2){const auto converted=static_cast<std::uint16_t>(port::driving::cvt_w_s(steer));
            hp2::recovered::eRotateZ(m,brake,brake,converted);hp2::recovered::eRotateZ(m,tire,tire,converted);}
        for(int axis=0;axis<3;++axis)tire[12+axis]=brake[12+axis]=info.Wheels[i].Position[axis];
        tire[15]=brake[15]=1;car.TireMatrices[i]=tire;car.BrakeMatrices[i]=brake;
    }
}
// Debug00193A70, E-0098-U02.
void CarSelector::CloseSwatches(::port::selection::WidgetContext& ctx){
    if(NumSwatches>10)hp2::not_implemented("0x00193A70","swatch count exceeds original array");
    for(int i=0;i<NumSwatches;++i)::port::selection::widget_projection::visible(ctx,SwatchFlailerHashes[i],false);
}
// Debug00193740, E-0098-U02. SkinIndex is NOT a resource skin ID.
void CarSelector::SetupSwatches(::port::selection::WidgetContext& ctx){
    CloseSwatches(ctx);const auto& car=::port::selection::widget_projection::current(*this,"0x00193740");
    NumSwatches=car.pCarStatus->nSkins;
    if(NumSwatches<0 || NumSwatches>10)hp2::not_implemented("0x00193740","skin/swatch count outside original storage");
    if(NumSwatches && !ctx.skin_swatch)hp2::not_implemented("0x00193740","original SkinInfo swatch lookup missing");
    for(int i=0;i<NumSwatches;++i){
        SwatchFlailerHashes[i]=hp2::frontend::bStringHash(("SWATCH_"+std::to_string(i)).c_str());
        SwatchTextureHashes[i]=ctx.skin_swatch(car.eCarType,car.pCarStatus->aSkins[i]);
    }
}
// Debug00193820, E-0098-U02.
void CarSelector::UpdateSwatches(::port::selection::WidgetContext& ctx){
    using namespace ::port::selection::widget_projection;
    const auto& car=current(*this,"0x00193820");
    int x=587,y=125;
    if(ScreenHash==0x18dc8e52){x=606;y=77;}else if(ScreenHash==0x18dc8e53){x=606;y=321;}
    if(NumSwatches>=2 && !cloaked(*this,car)){
        if(NumSwatches>10)hp2::not_implemented("0x00193820","swatch count exceeds original array");
        visible(ctx,0xedf2e4ee,true);position(ctx,0xedf2e4ee,x,y-22);
        for(int i=0;i<NumSwatches;++i){
            texture(ctx,SwatchFlailerHashes[i],SwatchTextureHashes[i]);visible(ctx,SwatchFlailerHashes[i],true);
            position(ctx,SwatchFlailerHashes[i],x,y+i*18);visible(ctx,0x7fe46db3,true);
            if(i==car.SkinIndex)position(ctx,0x7fe46db3,x-4,y+i*18-4);
        }
        visible(ctx,0x20d73d01,true);position(ctx,0x20d73d01,x,y+NumSwatches*18+2);
    }else{visible(ctx,0xedf2e4ee,false);visible(ctx,0x20d73d01,false);visible(ctx,0x7fe46db3,false);}
}
// Debug00193AE8, E-0098-U02. Original setup snaps cubic after configuring desired.
void CarSelector::SetupThumbnails(::port::selection::WidgetContext& ctx){
    using namespace ::port::selection::widget_projection;
    const auto& car=current(*this,"0x00193AE8");
    ThumbnailHorizCubic.duration=.5f;ThumbnailHorizCubic.flags=0;
    float desired=(IsTwoPlayerScreen?395.f:377.f)-float(car.CarIndex)*39.f;
    if(!car.CarUnlocked)desired=desired-float(width(ctx,0x2561a343));
    ThumbnailHorizCubic.ValDesired=desired;ThumbnailHorizCubic.state=0;
    ThumbnailHorizCubic.Val=desired;ThumbnailHorizCubic.dVal=ThumbnailHorizCubic.dValDesired;
    for(const auto& info:ctx.cars)visible(ctx,hp2::frontend::bStringHash(("THUMBNAIL_"+car_code(info,"0x00193AE8")).c_str()),false);
}
// Debug00193C18, E-0098-U02. Original cPoint spline is used, never a host Tween.
void CarSelector::UpdateThumbnails(::port::selection::WidgetContext& ctx,float elapsed){
    using namespace ::port::selection::widget_projection;
    const int y=ScreenHash==0x18dc8e53?265:ScreenHash==0x18dc8e52?25:402;
    cPoint::SplineSeek(ThumbnailHorizCubic,elapsed,0,0);
    for(const auto hash:cloak_hashes())visible(ctx,hash,false);
    visible(ctx,0x2561a343,false);const int divider_width=width(ctx,0x2561a343);
    bool divider=false;int cloak=0,index=0;const VisibleCar* previous=nullptr;
    for(const auto& node:VisibleCars.VisibleCarList){
        const auto& car=*node;int x=port::driving::cvt_w_s(ThumbnailHorizCubic.Val)+index*39;
        if(!car.CarUnlocked && previous && previous->CarUnlocked){
            visible(ctx,0x2561a343,true);divider=true;position(ctx,0x2561a343,x,y);
        }
        if(divider)x+=divider_width;
        std::uint32_t hash=car.ThumbnailHash;
        if(cloaked(*this,car) && cloak<25){
            visible(ctx,hash,false);hash=cloak_hashes()[cloak++];
        }
        visible(ctx,hash,true);position(ctx,hash,x,y);previous=&car;++index;
    }
}
// Debug00195B90, E-0098-U02. Explicit original2P aliases, not a general naming heuristic.
std::uint32_t CarSelector::CalculateCarLogoTexture(const CarTypeInfo& info)const{
    auto code=::port::selection::widget_projection::car_code(info,"0x00195B90");
    if(IsTwoPlayerScreen){
        if(info.HighPerformance){
            if(code.size()<2)hp2::not_implemented("0x00195B90","original high-performance code too short");
            code.resize(code.size()-2);
        }else if(info.UsageType==CAR_USAGE_TYPE_COP){
            if(code=="CROWNVIC")code="CROWNVICNOCOP";
            else if(code=="BMWM5COP")code="BMWM5";
            else if(code=="MUSTANGCOP")code="MUSTANG";
            else if(code=="CORVETTECOP")code="CORVETTE";
            else if(code=="LAMBOCOP")code="MURCIELAGO";
        }
    }
    return hp2::frontend::bStringHash(("CARLOGO_"+code).c_str());
}
// Debug00195CF8, E-0098-U02.
std::uint32_t CarSelector::CalculateNFSLogoTexture(const CarTypeInfo& info)const{
    if(IsTwoPlayerScreen){if(info.UsageType==CAR_USAGE_TYPE_COP)return hp2::frontend::bStringHash("LOGO_SHERIFF");
        if(info.HighPerformance)return hp2::frontend::bStringHash("LOGO_NFS");}
    return 0;
}
// Debug00195D60, E-0098-U02. Numeric name lookup then original string-key translation.
void CarSelector::SetLogo(::port::selection::WidgetContext& ctx){
    using namespace ::port::selection::widget_projection;
    const auto& car=current(*this,"0x00195D60");
    if(cloaked(*this,car)){visible(ctx,0x39e91754,false);visible(ctx,0xb270d4a5,false);return;}
    visible(ctx,0x39e91754,true);visible(ctx,0xb270d4a5,true);
    if(car.eCarType<0 || car.eCarType>=64)hp2::not_implemented("0x00195D60","invalid original car identity");
    const auto& info=ctx.cars[car.eCarType];
    if(!ctx.translated_key)hp2::not_implemented("0x00195D60","original string-key translation service missing");
    const auto name=ctx.translated_key(info.GetFullName(ctx.translated_id));
    if(auto* w=hp2::frontend::FindWidget_00143D50(ctx.screen,0x39e91754,1)){
        if(!w->text_capacity)hp2::not_implemented("0x00195D60","original selected car name buffer has zero capacity");
        w->text=name.substr(0,w->text_capacity-1);w->string_id_0x60=0;w->dirty_0x2b=5;
    }
    texture(ctx,0xb270d4a5,CalculateCarLogoTexture(info));
    const auto nfs=CalculateNFSLogoTexture(info);
    if(nfs){texture(ctx,0x729a0c4b,nfs);visible(ctx,0x729a0c4b,true);}else visible(ctx,0x729a0c4b,false);
}
// Debug00191F38, E-0098-S01. Debug0024C8B0 hashing matches reviewed frontend hash.
VisibleCar::VisibleCar(int type,CarStatus& status,int car_index,const CarTypeInfo& info)
    :pCarStatus(&status),eCarType(type),CarIndex(car_index){
    const auto zero=std::find(info.CarTypeName.begin(),info.CarTypeName.end(),'\0');
    if(zero==info.CarTypeName.end())hp2::not_implemented("0x00191F38","unterminated original car code");
    const std::string name="THUMBNAIL_"+std::string(info.CarTypeName.begin(),zero);
    ThumbnailHash=hp2::frontend::bStringHash(name.c_str());
    SetCurrentSkinNumber(info.DefaultSkinNumber);
}
// Debug00191FB8, E-0098-S01. Missing requested skin preserves existing SkinIndex.
void VisibleCar::SetCurrentSkinNumber(int skin_number){
    if(!pCarStatus || pCarStatus->nSkins>10)
        hp2::not_implemented("0x00191FB8","skin status outside decoded storage");
    for(int i=0;i<pCarStatus->nSkins;++i)if(pCarStatus->aSkins[i]==skin_number){SkinIndex=i;return;}
}
// Debug00192038, E-0098-S01. Do not normalize original zero-count semantics.
int VisibleCar::NextSkin(){
    if(!pCarStatus)hp2::not_implemented("0x00192038","missing original car status");
    if(++SkinIndex>=pCarStatus->nSkins)SkinIndex=0;
    return SkinIndex;
}
// Debug00192068, E-0098-S01. A zero-count previous request produces -1 originally.
int VisibleCar::PrevSkin(){
    if(!pCarStatus)hp2::not_implemented("0x00192068","missing original car status");
    if(--SkinIndex<0)SkinIndex=pCarStatus->nSkins-1;
    return SkinIndex;
}
// Debug00192410, E-0098-S01. Original comparator means 'already ordered', including ties.
int CarListSort(const VisibleCar& before,const VisibleCar& after,const ::port::selection::ListServices& svc){
    if(!svc.full_name)hp2::not_implemented("0x00192410","original translated full-name service missing");
    const auto key=[&](const VisibleCar& car){
        if(car.eCarType<0 || car.eCarType>=64)hp2::not_implemented("0x00192410","invalid original car identity");
        const auto& info=svc.cars[car.eCarType];
        return std::uint32_t(car.CarUnlocked?0:0x40000000)+(info.UsageType==CAR_USAGE_TYPE_COP?0x1000000u:0u)
            +std::uint32_t(int(info.HighPerformance)*0x10000)+std::uint32_t((5-int(info.CarClass))*0x100);
    };
    const auto a=key(before),b=key(after);
    const auto first=svc.full_name(before.eCarType),second=svc.full_name(after.eCarType);
    return !(b<a+std::uint32_t(std::strcmp(first.c_str(),second.c_str())>0));
}
// Debug00192518 -> typed00196960 -> bList::Sort00249958, E-0098-S01.
void VisibleCarSet::SortVisibleCarList(const ::port::selection::ListServices& svc){
    // Same adjacent pair passes; ownership moves, original node identity stays stable.
    bool changed;
    do{changed=false;for(std::size_t i=0;i+1<VisibleCarList.size();++i){
        if(!CarListSort(*VisibleCarList[i],*VisibleCarList[i+1],svc)){
            std::swap(VisibleCarList[i],VisibleCarList[i+1]);changed=true;
        }
    }}while(changed);
    for(std::size_t i=0;i<VisibleCarList.size();++i)VisibleCarList[i]->CarIndex=static_cast<int>(i);
}
// Debug001921D0, E-0098-S01. Bind globals explicitly; never synthesize unlocks or full names.
VisibleCarSet::VisibleCarSet(FrontendPlayerInfo& player,int game_type,bool be_a_cop,
    const ::port::selection::ListServices& svc){
    VisibleCarList.reserve(64);
    auto& ride=player.GetSelectedRideInfo(game_type);
    for(int type=0;type<64;++type){
        auto& status=player.aCarStatus[type];
        if(status.nStatus==eCAR_LOCKED_INVISIBLE && !svc.all_cars_available)continue;
        auto car=std::make_unique<VisibleCar>(type,status,TotalNumCars++,svc.cars[type]);
        if(status.nStatus==eCAR_UNLOCKED_FULLY_VISIBLE)car->CarUnlocked=true;
        else{
            if(!svc.is_car_unlocked)hp2::not_implemented("0x001921D0","original EasterEggs unlock service missing");
            car->CarUnlocked=svc.is_car_unlocked(type,be_a_cop);
        }
        if(car->CarUnlocked)++NumberUnlocked;
        if(ride.Type==type){pCurrentVisibleCar=car.get();car->SetCurrentSkinNumber(ride.SkinNumber);}
        VisibleCarList.push_back(std::move(car));
    }
    SortVisibleCarList(svc);
    if(!pCurrentVisibleCar && !VisibleCarList.empty())pCurrentVisibleCar=VisibleCarList.front().get();
    // Original empty list returns sentinel. Host uses nullptr; dereferencing it is a gap.
}
// Debug00192090, E-0098-S01. No wrap at last car.
VisibleCar* VisibleCarSet::GetNextCar(VisibleCar* car){
    if(!car || car->CarIndex<0 || static_cast<std::size_t>(car->CarIndex)>=VisibleCarList.size()
        || VisibleCarList[car->CarIndex].get()!=car)
        hp2::not_implemented("0x00192090","car is not in original visible list");
    const auto i=static_cast<std::size_t>(car->CarIndex);
    return i+1<VisibleCarList.size()?VisibleCarList[i+1].get():car;
}
// Debug001920A8, E-0098-S01. No wrap at first car.
VisibleCar* VisibleCarSet::GetPrevCar(VisibleCar* car){
    if(!car || car->CarIndex<0 || static_cast<std::size_t>(car->CarIndex)>=VisibleCarList.size()
        || VisibleCarList[car->CarIndex].get()!=car)
        hp2::not_implemented("0x001920A8","car is not in original visible list");
    const auto i=static_cast<std::size_t>(car->CarIndex);
    return i?VisibleCarList[i-1].get():car;
}
// Debug00194628, E-0098-S02. Popup submission is a port service, not a PS2 pointer.
void CarSelector::SummonPopup(const ::port::selection::SelectorServices& svc){
    if(SelectorState!=SS_TRANSMISSION && SelectorState!=SS_HANDLING && SelectorState!=SS_READY)return;
    if(!svc.summon_popup)hp2::not_implemented("0x00194628","original popup submission service missing");
    ::port::selection::Popup pop;
    pop.screen_hash=ScreenHash==0xb5074ab4?0x41c2fdc3:ScreenHash==0x18dc8e52?0x6344da01:0x6344da02;
    pop.port=Port;
    if(SelectorState==SS_TRANSMISSION){
        pop.n_buttons=3;pop.button_params={1,2,3};pop.button_ids={0xbf7c,0xbf86,0xbf72};
        pop.message_id=49000;pop.start_button=TransmissionType;
    }else if(SelectorState==SS_HANDLING){
        pop.n_buttons=2;pop.button_params={4,5,0};pop.button_ids={0xbf9a,0xbfa4,0};
        pop.message_id=0xbf90;pop.start_button=HandlingMode;
    }else{
        pop.button_params={7,7,7};pop.message_id=0xbfae;
    }
    PopupID=svc.summon_popup(pop);
}
// Debug001945F0, E-0098-S02.
void CarSelector::AdvanceState(const ::port::selection::SelectorServices& svc){
    if(SelectorState!=SS_READY)SelectorState=static_cast<eSelectorState>(int(SelectorState)+1);
    SummonPopup(svc);
}
// Debug00194610, E-0098-S02. Cancellation retains transmission/handling choices.
void CarSelector::DevanceState(const ::port::selection::SelectorServices& svc){
    if(SelectorState!=SS_SELECT)SelectorState=static_cast<eSelectorState>(int(SelectorState)-1);
    SummonPopup(svc);
}
// Debug00194E08, E-0098-S02, scoped MENUITEM_SELECT/POPUP_DISMISS bodies only.
void CarSelector::ScreenCallback(int msg,int param,std::uint32_t item_hash,
    const ::port::selection::SelectorServices& svc){
    if(msg==1){
        if(DesiredSelectorMode==SELECTOR_MODE_ORBIT || item_hash!=0x52e039f3 || SelectorState!=SS_SELECT)return;
        const auto* car=VisibleCars.pCurrentVisibleCar;
        if(!car)hp2::not_implemented("0x00194E08","empty original visible-car list");
        if(!car->CarUnlocked && !AllCarsUnlocked)return;
        AdvanceState(svc);return;
    }
    if(msg!=21)hp2::not_implemented("0x00194E08","selector message presentation/input path not translated");
    PopupID=-1;
    switch(param){
        case 0:DevanceState(svc);return;
        case 1:TransmissionType=TT_AUTO;break;
        case 2:TransmissionType=TT_MANUAL;break;
        case 3:TransmissionType=TT_SPORT;break;
        case 4:HandlingMode=HM_CLASSIC;break;
        case 5:HandlingMode=HM_EXTREME;break;
        case 7:break;
        default:return;
    }
    if(!svc.maybe_continue)hp2::not_implemented("0x00194E08","original MaybeContinueToNextScreen service missing");
    // Original checks readiness BEFORE advancing (single-player ready at HANDLING).
    if(!svc.maybe_continue())AdvanceState(svc);
}
// Debug00196348, E-0098-S02. Original zero selectors returns true.
int SelectCarScreen::AreAllSelectorsReady() const{
    if(NumCarSelectors<0 || NumCarSelectors>2)hp2::not_implemented("0x00196348","invalid selector count");
    for(int i=0;i<NumCarSelectors;++i){
        const auto* selector=CarSelectors[i];
        if(!selector)hp2::not_implemented("0x00196348","missing original selector");
        if(selector->SelectorState!=(selector->IsTwoPlayerScreen?SS_READY:SS_HANDLING))return 0;
    }
    return 1;
}
// Debug00196198, E-0098-S02. Outer Back also invokes this; do not implement rollback.
void SelectCarScreen::SetSelectedCars(FrontendDatabase& fred,const std::array<CarTypeInfo,64>& cars){
    if(fred.NumPlayerCars<0 || fred.NumPlayerCars>2)hp2::not_implemented("0x00196198","invalid player count");
    for(int i=0;i<fred.NumPlayerCars;++i){
        const auto* selector=CarSelectors[i];if(!selector)continue;
        const auto* car=selector->VisibleCars.pCurrentVisibleCar;
        if(!car || !car->pCarStatus || car->SkinIndex<0 || car->SkinIndex>=10)
            hp2::not_implemented("0x00196198","selected skin outside decoded original storage");
        auto& player=fred.FrontendPlayers[i];
        player.AssignCarOfType(fred.GameType,car->eCarType,car->pCarStatus->aSkins[car->SkinIndex],1,cars);
        switch(selector->TransmissionType){case TT_AUTO:player.mPlayerSettings.TransmissionType=0;break;
            case TT_MANUAL:player.mPlayerSettings.TransmissionType=1;break;case TT_SPORT:player.mPlayerSettings.TransmissionType=2;break;}
        switch(selector->HandlingMode){case HM_CLASSIC:player.mPlayerSettings.DrivingStyle=0;break;
            case HM_EXTREME:player.mPlayerSettings.DrivingStyle=1;break;}
    }
}
}

namespace hp2::recovered {
// ELF 0x00195C40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00195c40.md.
void CarSelector::SetupCarOverrideModel() {
    hp2::not_implemented("0x00195C40", "CarSelector::SetupCarOverrideModel (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00198878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00198878.md.
void SelectCarScreen::AreAllSelectorsReady() {
    hp2::not_implemented("0x00198878", "SelectCarScreen::AreAllSelectorsReady (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001942D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001942d0.md.
void VisibleCar::SetCurrentSkinNumber() {
    hp2::not_implemented("0x001942D0", "VisibleCar::SetCurrentSkinNumber (signature and full body unresolved)");
}

// ELF 0x00194350; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00194350.md.
void VisibleCar::NextSkin() {
    hp2::not_implemented("0x00194350", "VisibleCar::NextSkin (signature and full body unresolved)");
}

// ELF 0x00194380; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00194380.md.
void VisibleCar::PrevSkin() {
    hp2::not_implemented("0x00194380", "VisibleCar::PrevSkin (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug00195290, E-0098-U03. Snap group intentionally retains its previous alpha mid-transition.
void CarSelector::UpdateFadeGroups(feAlphaAnimation& fade,feAlphaAnimation& orbit,
    feAlphaAnimation& snap,const std::array<CarTypeInfo,64>& cars){
    const auto& selected=::port::selection::widget_projection::current(*this,"0x00195290");
    fade.SetAlphaScale(1-std::min(1.f,SelectorMode*3.f));
    orbit.SetAlphaScale(SelectorMode);
    if(cars.at(selected.eCarType).UsageType==CAR_USAGE_TYPE_COP)orbit.SetAlphaScale(0);
    if(SelectorMode==SELECTOR_MODE_ORBIT)snap.SetAlphaScale(0);
    else if(DesiredSelectorMode==SELECTOR_MODE_CHOOSE && SelectorMode<.333333343f)snap.SetAlphaScale(1);
}
}

namespace hp2::debug {
// Debug00194378, E-0098-I01. No help-active or locked-car orbit entry.
bool CarSelector::CanEnterOrbitMode(const VisibleCar* car,bool help_active) const{
    if(help_active || DesiredSelectorMode!=SELECTOR_MODE_CHOOSE)return false;
    if(!car)hp2::not_implemented("0x00194378","original current VisibleCar missing");
    return car->CarUnlocked || AllCarsUnlocked!=0;
}
// Debug00194248, E-0098-I01. Speech, help and camera calls retain original order.
void CarSelector::EnterOrbitMode(const ::port::selection::OrbitServices& services){
    OrbitActiveTime=services.real_time_frames;DesiredSelectorMode=SELECTOR_MODE_ORBIT;ManualOrbitEngaged=0;
    if(services.camera.sound_enabled){
        const auto& selected=::port::selection::widget_projection::current(*this,"0x00194248");
        if(!services.car_description || !services.speech_event)hp2::not_implemented("0x00194248","original speech services missing");
        const auto& info=services.cars.at(selected.eCarType);const int description=services.car_description(selected.eCarType);
        if(info.UsageType==CAR_USAGE_TYPE_COP)services.speech_event(0xd3,{description});
        else services.speech_event(0x92,{info.HighPerformance!=0,description});
    }
    if(!services.adjust_help)hp2::not_implemented("0x00194248","original AdjustHelpMessage binding missing");
    services.adjust_help();
    if(!pSelectCarCameraMover)hp2::not_implemented("0x00194248","original selector camera missing");
    pSelectCarCameraMover->SetMode(SelectCarCameraMover::SCC_ORBIT,services.camera);
}
// Debug00194310, E-0098-I01.
void CarSelector::EnterChooseMode(const ::port::selection::OrbitServices& services){
    DesiredSelectorMode=SELECTOR_MODE_CHOOSE;
    if(!services.adjust_help)hp2::not_implemented("0x00194310","original AdjustHelpMessage binding missing");
    services.adjust_help();
    if(!pSelectCarCameraMover)hp2::not_implemented("0x00194310","original selector camera missing");
    pSelectCarCameraMover->SetMode(SelectCarCameraMover::SCC_STATS,services.camera);
    if(services.camera.sound_enabled){
        if(!services.speech_fade)hp2::not_implemented("0x00194310","original speech fade binding missing");
        services.speech_fade(1);
    }
}
// Debug001943F0, E-0098-I01. JoyEvent values freshly checked against Debug enum.
// Original port argument is unused. IN/OUT/INOUT events have no switch branch.
void CarSelector::JoyHandler(int port,int event,std::uint8_t param,const ::port::selection::OrbitServices& services){
    using namespace hp2::input;
    (void)port;if(SelectorState!=SS_SELECT)return;
    if(event==JOY_EVENT_CARSEL_CHANGE_MODE){
        if(!param || IsTwoPlayerScreen || FadeCarOverlayAlpha)return;
        if(CanEnterOrbitMode(VisibleCars.pCurrentVisibleCar,services.help_active))EnterOrbitMode(services);
        else if(DesiredSelectorMode==SELECTOR_MODE_ORBIT)EnterChooseMode(services);
        return;
    }
    if(event==JOY_EVENT_CARSEL_UNLOCK){if(!services.version_e3 && !AllCarsUnlocked && param)AllCarsUnlocked=1;return;}
    if((event>=JOY_EVENT_CARSEL_ORBIT_LEFT && event<=JOY_EVENT_CARSEL_ORBIT_DOWN) || event==JOY_EVENT_CARSEL_ORBIT_LEFTRIGHT || event==JOY_EVENT_CARSEL_ORBIT_UPDOWN){
        ManualOrbitEngaged=1;OrbitActiveTime=services.real_time_frames;
        if(!pSelectCarCameraMover)hp2::not_implemented("0x001943F0","original selector camera missing");
        auto& camera=*pSelectCarCameraMover;
        switch(event){
        case JOY_EVENT_CARSEL_ORBIT_LEFT:camera.DesiredHOrbitSpeed=param?-9000.f:0;break;
        case JOY_EVENT_CARSEL_ORBIT_RIGHT:camera.DesiredHOrbitSpeed=param?9000.f:0;break;
        case JOY_EVENT_CARSEL_ORBIT_UP:camera.DesiredVOrbitSpeed=param?10000.f:0;break;
        case JOY_EVENT_CARSEL_ORBIT_DOWN:camera.DesiredVOrbitSpeed=param?-10000.f:0;break;
        case JOY_EVENT_CARSEL_ORBIT_LEFTRIGHT:camera.DesiredHOrbitSpeed=(float(param)-128)*70.3125f;break;
        case JOY_EVENT_CARSEL_ORBIT_UPDOWN:camera.DesiredVOrbitSpeed=(float(param)-128)*-78.125f;break;
        }
    }
}
}

namespace hp2::debug {
// Debug00194140, E-0098-I01; table immediates independently checked in listing.
// Address submission is a port service, not a Retail help-table pointer cast.
void CarSelector::AdjustHelpMessage(const FrontendDatabase& fred,
    const std::function<void(std::uint32_t)>& set_help_override){
    std::optional<std::uint32_t> table;
    if(DesiredSelectorMode==SELECTOR_MODE_CHOOSE){
        if(fred.GameType==8)table=0x311cb0; // ChampionshipModeCarSelectHelp
        else if(fred.GameType==9)table=0x311cf8; // HotPursuitCarSelectHelp
        else if(fred.GameType==7){
            if(fred.NumPlayerCars==1)table=0x311c28; // BeACop1PCarSelectHelp
            else if(fred.NumPlayerCars==2)table=0x311c40; // BeACop2PCarSelectHelp
        }else if(fred.NumPlayerCars==1)table=0;
        else if(fred.NumPlayerCars==2)table=0x311bf8; // CarSelectTwoPlayerHelp
    }else if(DesiredSelectorMode==SELECTOR_MODE_ORBIT){
        if(fred.GameType==8)table=0x311e18; // ChampionshipModeOrbitModeHelp
        else if(fred.GameType==9)table=0x311e28; // HotPursuitOrbitModeHelp
        else table=0x311c58; // OrbitModeHelp
    }
    if(table){
        if(!set_help_override)hp2::not_implemented("0x00194140","original SetHelpOverride binding missing");
        set_help_override(*table);
    }
}
}

namespace hp2::debug {
// Debug00194E08, E-0098-I02. Typed callback data and explicit dependency services;
// full Tick/platform/locked renderer bodies remain required external bindings.
void CarSelector::ScreenCallback(ScreenMessage msg,ScreenCallbackData& data,
    const ::port::selection::ScreenServices& services){
    const auto sound=[&](int event){
        if(!services.sound)hp2::not_implemented("0x00194E08","original frontend sound service missing");
        services.sound(event);
    };
    switch(msg){
    case MENUITEM_SELECT:
        if(DesiredSelectorMode==SELECTOR_MODE_ORBIT){data.Noise=0;return;}
        ScreenCallback(int(msg),data.Param,data.ItemHash,services.state);return;
    case MENUITEM_PAD_LEFT:case MENUITEM_PAD_RIGHT:
        if(SelectorMode==SELECTOR_MODE_CHOOSE){
            auto* old=VisibleCars.pCurrentVisibleCar;
            auto* next=msg==MENUITEM_PAD_RIGHT?VisibleCars.GetNextCar(old):VisibleCars.GetPrevCar(old);
            if(!services.set_locked_car)hp2::not_implemented("0x00194E08","original CarLockedDisplayer binding missing");
            services.set_locked_car(next,AllCarsUnlocked,Port==1);
            if(next==old)return;
            if(FadeCarOverlayAlpha!=CAR_FADED_ALPHA)sound(msg==MENUITEM_PAD_LEFT?13:12);
            VisibleCars.pCurrentVisibleCar=next;
            float target=(IsTwoPlayerScreen?395.f:377.f)-float(next->CarIndex)*39.f;
            if(!next->CarUnlocked)target-=float(::port::selection::widget_projection::width(services.widgets,0x2561a343));
            ThumbnailHorizCubic.ValDesired=target;
            if(target!=ThumbnailHorizCubic.Val)ThumbnailHorizCubic.state=2;
            SetLogo(services.widgets);
            if(!services.update_info)hp2::not_implemented("0x00194E08","original UpdateInfoBarAtTop binding missing");
            services.update_info();SetupSwatches(services.widgets);
        }
        return;
    case MENUITEM_PAD_UP:case MENUITEM_PAD_DOWN:
        if(SelectorMode==SELECTOR_MODE_CHOOSE && NumSwatches>1){
            auto& selected=const_cast<VisibleCar&>(::port::selection::widget_projection::current(*this,"0x00194E08"));
            if(selected.CarUnlocked || AllCarsUnlocked){
                if(msg==MENUITEM_PAD_UP)selected.PrevSkin();else selected.NextSkin();
                sound(14);
                if(LoadedCar==&selected)ShowLoadedCar(services.rendering);
            }
        }
        data.Param=1;return;
    case SCREEN_SELECT_BUTTON:sound(3);return;
    case SCREEN_BACK_BUTTON:
        if(DesiredSelectorMode==SELECTOR_MODE_ORBIT){data.Param=0;EnterChooseMode(services.orbit);data.Noise=0;}
        return;
    case SCREEN_TICK:
        if(!services.tick)hp2::not_implemented("0x00194E08","original selector Tick binding missing");
        services.tick();return;
    case SCREEN_RENDER:
        if(!services.render_platform || !services.render_locked)
            hp2::not_implemented("0x00194E08","original platform/frame allocation and locked renderer bindings missing");
        services.render_platform();services.render_locked(AllCarsUnlocked);return;
    case POPUP_DISMISS:{
        bool continued=false;auto state=services.state;
        if(state.maybe_continue){const auto original=state.maybe_continue;
            state.maybe_continue=[&,original]{continued=original();return continued;};}
        ScreenCallback(int(msg),data.Param,data.ItemHash,state);
        if(continued)data.Noise=18;
        return;
    }
    default:return; // These ScreenMessage values have no branch in the original switch.
    }
}
}

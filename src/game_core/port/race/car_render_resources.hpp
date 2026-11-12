#pragma once
// Port-owned Memory views/lifetime for original car rendering resources.
// TASK-0099 E-0099-W25; Debug SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/object_view.hpp"
#include <filesystem>
#include <utility>
#include <string>
#include <vector>
namespace port::race {
using driving::Memory;
// Port view storage, not original C++ ABI. Method spellings are recovered verbatim.
class eModel:public driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Init(std::uint32_t); // Debug0010C950.
 void GetBoundingBox(std::array<float,4>&,std::array<float,4>&)const; //0010CDA0 Vec3 semantics: preserve W.
 int RenameTexture(std::uint32_t,std::uint32_t); //0010CBE8.
 void ReplaceLightMaterial(std::uint32_t,std::uint32_t); //0010CEB8.
 void SetEnvironmentMapping(std::uint32_t,int); //0010CED8, E-0099-W38.
 void UnInit(); //0010C9A8.
 void Sub_00115100(int); //original inline eModel dtor, E-0099-W57.
 std::uint32_t GetRigidAnimation() const; //0010CCE8, E-0099-W65.
 int GetRigidAnimationTime() const; //0010DEB0.
 void SetRigidAnimationFrameNumber(int); //0010CD08.
 void ReconnectSolid(); //0010C9E0.
 void ReconnectTextureTable(); //0010CA78.
 void ApplyReplacementTextureTable(); //0010CC70.
 void AttachReplacementTextureTable(std::uint32_t,int,int); //0010CB90.
};
class eSolid:public driving::ObjectView {public:using ObjectView::ObjectView;void SetEnvironmentMapping(std::uint32_t,int); /*00103F10, E-0099-W38*/};
std::uint32_t eGetView(Memory&,int); // Debug00107AB0, E-0099-W40.
class eView:public driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_00106E68(); // Debug constructor; E-0099-W50.
 void AttachCameraMover(std::uint32_t); //001073D8.
 void UnattachCameraMover(std::uint32_t); //001073F8.
};
std::uint32_t eGetRenderTarget(Memory&,int); //00105650, E-0099-W50.
void eInitViews(Memory&); //001079B8, E-0099-W50.
void eUpdateViewMode(Memory&); //00107B00, E-0099-W50.
 std::uint32_t GetPlayerView(Memory&,int player); //0010F4B0, E-0099-W60.
 void RadialBlurOff(Memory&,std::uint32_t view); //0010F588, original pointer overload.
 void RadialBlurOff(Memory&,int player); //0010F590, original player overload.
 void RadialBlurOn(Memory&,std::uint32_t view); //0010F508, E-0099-W63.
 void RadialBlurOn(Memory&,int player); //0010F558.
 void RadialBlurAlpha(Memory&,int player,float center,float edge); //0010F618.
 // Port selective resource component: exact radial GetTextureInfo/store from
 // eFixUpTables00103D28, not whole solid/model/animation reconnect scheduling.
 void initialize_radial_blur_resource_state(Memory&);
 void release_radial_blur_resource_state(Memory&); //Port lifetime: after all drivers/views stop borrowing, before texture owner closes.
// Port selective startup owner: only the four Camera and ten eView constructor
// calls from original001148A0, subsequent eUnitVector4 static0011C2E8, then
// runtime eMathInit then original eInitViews, matching InitializeEverything's
// engine order; not whole eInitEngine or all process static initialization.
void initialize_camera_view_state(Memory&);
int eGetCurrentViewMode(const Memory&); //00107AF8, E-0099-W28.
std::uint32_t eFindSolid(Memory&,std::uint32_t); //0010C918.
std::uint32_t elGetLightMaterial(Memory&,std::uint32_t); //00116E28.
void eDuplicateSolid(Memory&,const std::string&,const std::string&); // Debug00103AB0.
void eDeleteAllDuplicatedSolids(Memory&); // Debug00103C18.
void FixStripSTs(Memory&,std::uint32_t); //0010C550.
void FixTextureTable(Memory&,std::uint32_t); //0010C1B0.
void FixLightMaterialTable(Memory&,std::uint32_t); //0010C850.
// Actual inventory packet/hash and inactive platform guard use E-0099-W30.
// Active PS2 development-monitor transport remains an explicit device boundary.
int LoaderSolidList(Memory&,std::uint32_t); //00103588.
int UnloaderSolidList(Memory&,std::uint32_t); //00103950.
int LoaderLightMaterialChunk(Memory&,std::uint32_t); //00115348 branch3401E only.
void UnloadLightMaterialChunk(Memory&,std::uint32_t); // Matching original unloader projection.
int LoaderCarInfo(Memory&,std::uint32_t); // Debug00186148.
int UnloaderCarInfo(Memory&,std::uint32_t); // Debug001862D8.
void GetUsedCarTextureInfo(Memory&,std::uint32_t info,std::uint32_t ride,int front_end_only); // Debug00186940.
std::string render_resource_text(Memory&,std::uint32_t); // Port trusted-ELF string adapter.
std::uint32_t render_format_hash(Memory&,std::uint32_t,const std::string&,int=0,int=0); // Port printf adapter.
std::uint32_t GetSkinInfo(Memory&,int,int,int); // Debug00186330.
void eInitSlotPools(Memory&); // Debug00109E18 all7 original components.
void eCloseSlotPools(Memory&); // Debug00109F38 retains ActiveTexture/VertexAnimator exactly.
void initialize_render_resource_state(Memory&); // Port explicit startup of required original list members.
std::pair<std::uint32_t,std::uint32_t> GetElementST0(Memory&,std::uint32_t,int); //0011C408.
std::array<std::uint32_t,3> GetElementXYZ(Memory&,std::uint32_t,int); //0011C310.
std::uint32_t GetElementRGBA(Memory&,std::uint32_t,int); //0011C4F8.
void CallDamageVertexFunction(Memory&,std::uint32_t); //001B1FE0.
void CallDamageVertexFunctionForEveryVertex(Memory&,std::uint32_t model,std::uint32_t body_model); //00103FA0.
bool DamageVertexFunction(Memory&,std::uint32_t,std::array<float,3>&,std::uint32_t rgba,bool); //001B1D60.
// Explicit original0x930 storage view; no host layout or GPU submission claim.
class CarRenderInfo:public driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_0011EA88(std::uint32_t ride,std::uint32_t car);
 void Sub_0011FCE8(); // Original idle rendering queue branch; alpha buffers owned/released.
 void OverRideAlpha(std::uint8_t,bool,bool); // Debug00123B18.
 void RestoreAlpha(); // Debug00123C40.
 void SwitchSkin(std::uint32_t);
 void UpdateTextureInfo(std::uint32_t); // Debug001200F0, E-0110-CE01.
 void UpdateSkinTextures(); // Debug00120128, E-0110-CE01.
 void UpdateCarReplacementTextures();
 void CreateCarLightFlares();
};
class CarRenderResources {
 Memory& m_;
 std::vector<std::uint32_t> allocations_,chunks_;
public:
 static constexpr bool inventory_transport_bound=false;
 explicit CarRenderResources(Memory&m):m_(m){}
 void load(const std::filesystem::path&);
 void load_bundle(const std::vector<std::uint8_t>&); //Port original decompressed bundle input; same recovered loaders.
 // Port resource preparation after original active-driver filtering; exact
 // required duplication requests from GameFlowLoadingTrackCommonPart2 00167DC0.
 void prepare_ride(std::uint32_t ride);
 void close(); // Close all eModels and CarRenderInfo borrowers before releasing resources.
};
}

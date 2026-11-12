#pragma once
// Port-side declarations/ownership of actual serialized Flailer resource bytes.
// No native PS2 C++ layout/ABI projection is claimed. E-0099-W52.
#include "game_core/port/driving/object_view.hpp"
#include <vector>
namespace hp2::debug {
void Sub_001462B0(::port::driving::Memory&);
int LoaderFlailerChunk(::port::driving::Memory&,std::uint32_t); //0013F990.
int UnloaderFlailerChunk(::port::driving::Memory&,std::uint32_t); //0013FB10.
std::uint32_t FindFlailerScreen(::port::driving::Memory&,std::uint32_t); //00140168.
std::uint32_t FindFlailerObject(::port::driving::Memory&,std::uint32_t screen,std::uint32_t name,int type); //00143B40.
std::uint32_t FindFlailerObjectByGroup(::port::driving::Memory&,std::uint32_t screen,std::uint32_t group,int type,std::uint32_t previous); //00143BC8.
void FlailerSetVisibility(::port::driving::Memory&,std::uint32_t screen,std::uint32_t name,int); //00144758.
void FlailerSetGroupVisibility(::port::driving::Memory&,std::uint32_t screen,std::uint32_t group,int); //001452D8.
void FlailerSetLanguageID(::port::driving::Memory&,int screen,int name,int language); //00144798, E-0112-P01.
int RenderTextIntoGroup(::port::driving::Memory&,std::uint32_t screen,std::uint32_t group,std::uint32_t text); //00144990,E-0112-R05.
// E-0099-W73 original whole helpers; missing hashes retain source return.
void FlailerSetAlpha(::port::driving::Memory&,std::uint32_t,std::uint32_t,int); //001446D8.
void FlailerScaleAboutCenter(::port::driving::Memory&,std::uint32_t,std::uint32_t,float); //001443A0.
void Animate321GoElement(::port::driving::Memory&,float,float,float,std::uint32_t,std::uint32_t); //0014AEE8.
class FlailerBitmapChunk:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void SetAlpha(int,int index=-1); //00140FC8, E-0099-W73.
 std::uint32_t GetTextureInfo(); //00141158.
 void ScaleAboutCenter(float); //00141378.
 void SetRotation(float); //00141048; E-0112-H01.
 void SetColor(std::uint32_t); //00141078; E-0112-H01.
 void ChangeTexture(std::uint32_t); //00141140; E-0112-H01.
};
class FlailerObjectChunk:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 std::array<float,2> GetTopLeft()const; //00140768; text-width consumer remains explicit.
 void DeleteFlailerStrip(); //00140968; active immediate strip destruction remains explicit.
 void SetVisibility(int); //00140518; object invisible=0, distinct from screen invisible=1.
 void SetAlpha(int); //001404C8, E-0099-W73.
 void ScaleAboutCenter(float); //00140610.
 void SetTopLeftY(float); //00140838.
 void SetTopLeftX(float); //00140810; E-0112-H01.
};
class FlailerTextChunk:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;void SetAlpha(int); /*00140CC0 E-0099-W73.*/
 void UpdateLanguageText(); //00140C88.
 void SetString(std::uint32_t source); //00140C10; E-0112-H01, exact bounded source-memory copy.
 void SetLanguageID(std::uint32_t); //00140C98; E-0112-H01.
 void SetFormatOptions(int); //00140A58; E-0112-H01.
 void ClearFormatOptions(int); //00140A70; E-0112-H01.
 void SetColor(std::uint32_t); //00140A90; E-0112-H01.
 float GetScaledWidth(); //00140B20; E-0112-H01.
 float GetScaledHeight(); //00140AC8; E-0112-H01.
 void ScaleAboutCenter(float); //00140D98; E-0112-H01.
};
class FlailerScreenChunk:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void UpdateLanguageText(); //00140090.
 void NotifyVisibilityChange(int); //00140020.
};
class FlailerRenderManager:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 bool IsScreenVisible(std::uint32_t)const; //00145AE8, eight actual visible screen slots.
 void Sub_001458E0();void SetScreenVisibility(std::uint32_t,int); //00145A40.
};
}
namespace port::race {
class FlailerResources {
 ::port::driving::Memory& memory_;
 std::vector<std::uint32_t> chunks_;
public:
 explicit FlailerResources(::port::driving::Memory& m):memory_(m){}
 void load_bundle(const std::vector<std::uint8_t>&);
 void close();
 std::size_t project_count()const{std::size_t count=0;for(const auto chunk:chunks_)if(memory_.word(chunk)==0x80010200)++count;return count;}
};
}

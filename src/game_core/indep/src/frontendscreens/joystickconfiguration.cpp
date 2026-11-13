#include "FMenus.hpp"
#include "FEAnim.hpp"
#include "Fred.hpp"
#include "CarSounds.hpp"
#include "Language.hpp"
#include "FMenus.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace hp2 {
namespace {
// Released data31A660/31A698/31A6B0 and331548, not the beta arrays.
constexpr std::int32_t ConfigHelp[]={801601,801602,0};
constexpr std::int32_t PreConfigHelp[]={802200,802201,802202,802203,0};
constexpr std::int32_t CalibrationHelp[]={802210,801602,0};
constexpr std::int32_t FixedConfigHelp[]={0xc3f2b,0};
constexpr std::uint32_t MenuFirst=0x52e039f3u;
MainMenuScreenServices* joystickServices{};

MainMenuScreenServices& Services()
{
    Require(joystickServices && joystickServices->ui && joystickServices->ui->resources &&
        joystickServices->ui->sound && joystickServices->ui->language && joystickServices->screens &&
        joystickServices->screens->input && joystickServices->screens->input->router &&
        joystickServices->frontend,"Joystick screen lacks its canonical frontend/input owners");
    return *joystickServices;
}
GameInputRouter& Input(MainMenuScreenServices& s) {return *s.screens->input->router;}
FlailerBitmapChunk* Bitmap(MainMenuScreenServices& s,std::uint32_t screen,std::uint32_t key)
{
    return static_cast<FlailerBitmapChunk*>(s.ui->resources->FindObject(screen,key,2));
}
void RefreshHelp(MainMenuScreenServices& s,const std::int32_t* help)
{
    // PAL23DDE8: only an already-open panel reaches the original
    // MakeHelpGo toggle with this family's real items. A closed panel stays closed.
    Require(s.screens->help,"Joystick popup has no canonical help owner");
    if(s.screens->help->IsActive())s.screens->MakeHelpGo(help,1);
}
bool SafeIsPopupActive(MainMenuScreenServices& s,std::int32_t handle,std::uint32_t identifier)
{
    // PAL23DEA0 compares PopupConfig+40 only after the two slot bounds and
    // active-state checks. Handles can be reused by unrelated screens.
    if(static_cast<std::uint32_t>(handle)>=2 || !s.screens->GetPopupActiveState(handle))return false;
    const auto* config=s.screens->GetPopupConfig(handle);
    return config && config->clientParameters[1]==identifier;
}
void SafeDismissPopup(MainMenuScreenServices& s,std::int32_t handle,std::uint32_t identifier)
{
    if(SafeIsPopupActive(s,handle,identifier))s.screens->DismissPopup(handle);
}
std::string PortMessage(MainMenuScreenServices& s,std::uint32_t id,std::int32_t port)
{
    const auto* format=s.ui->language->GetString(id);
    Require(format,"Joystick popup lacks its actual localized message");
    return FormatLocalizedIntegerText(format,port+1,256);
}
FlailerPopupConfig Popup(MainMenuScreenServices& s,std::int32_t port,std::uint32_t identifier,
    const char* message,std::uint32_t label,std::int32_t result,std::int32_t dismissed,
    bool oneButton=true,std::int32_t initial=0)
{
    FlailerPopupConfig value{};
    value.buttonCount=oneButton?1:0;value.sourceNode=s.screens->active;
    value.screenKey=oneButton?0x74d2dfa2u:0x037f3133u;value.message=message;
    value.buttonLanguageIds[0]=label;value.buttonResults[0]=result;
    value.dismissResult=dismissed;value.initialButton=initial;value.joystickPort=port;
    value.clientParameters[1]=identifier;return value;
}
void PushAutosave(MainMenuScreenServices& s)
{
    // Final1F5C28 ->16C760 ->ScreenStack::Push; no immediate screen change.
    if(s.frontend->autoSave)s.screens->stack.Push("Autosave");
}
void MoveBack(MainMenuScreenServices& s,std::int32_t port)
{
    Require(s.screens->MoveToPreviousScreen(1,false,port),"Joystick screen has no preceding original screen");
}
void SetMenuAvailability(MainMenuScreenServices& s,std::uint32_t screen,std::uint32_t key,bool enabled)
{
    // The actual16D198 wrapper, restricted to its source0/1 callers. State0
    // disables input/color; it does not hide the authored menu object.
    auto* node=s.screens->FindScreen(static_cast<std::int32_t>(screen));
    if(!node)return;
    auto* child=node->FindFlailerScreen(static_cast<std::int32_t>(screen));
    if(!child || !child->menu)return;
    auto* item=child->menu->FindItem(key);if(!item)return;
    if(enabled) {item->SetMenuItemState(1);s.ui->resources->SetObjectVisibility(screen,key,1);}
    else item->ClearMenuItemState(1);
}
void ResetCurrentMenus(MainMenuScreenServices& s)
{
    // PAL16D3B0 ->16B1B8 ->16EB18, preserving child list order.
    auto* node=s.screens->active;if(!node)return;
    for(auto* child=node->screenRoot.next;child!=&node->screenRoot;child=child->next)
        if(child->menu)child->menu->ResetCurrentMenuItem();
}
void ControllerPicture(MainMenuScreenServices& s,std::uint32_t screen,std::int32_t configuration)
{
    std::array<char,32> name{};std::snprintf(name.data(),name.size(),"CONTROLLER%d",configuration);
    if(auto* image=Bitmap(s,screen,0x0b2ad6dfu))image->ChangeTexture(hashString(name.data()));
}
}

class JoystickConfigScreen final {
public:
    JoystickConfigScreen(MainMenuScreenServices& services,std::int32_t port,std::uint32_t screen)
        :s_(services),port_(port),displayed_(joystickGetUserConfiguration(&Input(s_),port)),
         saved_(displayed_),type_(joystickGetType(&Input(s_),port)),screen_(screen),
         plugged_(joystickIsPluggedIn(&Input(s_),port)!=0),
         left_(Bitmap(s_,screen_,0xd1cda594u)),right_(Bitmap(s_,screen_,0x9345e4a7u))
    {
        Require(port_>=0 && port_<2,"Joystick configuration port is invalid");
        InitCurrentDisplayedConfig();
    }
    ~JoystickConfigScreen()
    {
        joystickSetUserConfiguration(&Input(s_),port_,saved_);s_.screens->SetHelpOverride(nullptr);
    }
    void HandleScreenCallback(ScreenMessage message,ScreenCallbackData& data);
    static void ScreenCallBack(ScreenMessage message,ScreenCallbackData* data);
private:
    std::int32_t Available(std::array<std::int32_t,10>& values) const
    {return joystickDetermineAvailableConfigurations(values.data(),static_cast<std::int32_t>(values.size()),type_);}
    void InitCurrentDisplayedConfig()
    {
        std::array<std::int32_t,10> values{};const auto count=Available(values);
        if(count>0 && std::find(values.begin(),values.begin()+count,displayed_)==values.begin()+count)
            displayed_=values[0];
    }
    void CycleConfiguration(bool left)
    {
        std::array<std::int32_t,10> values{};const auto count=Available(values);if(count<2)return;
        const auto found=std::find(values.begin(),values.begin()+count,displayed_);
        if(found==values.begin()+count)displayed_=values[0];
        else {
            auto index=static_cast<std::int32_t>(found-values.begin())+(left?-1:1);
            if(index<0)index=count-1;else if(index>=count)index=0;
            displayed_=values[static_cast<std::size_t>(index)];
        }
        joystickSetUserConfiguration(&Input(s_),port_,displayed_);
    }
    void SaveConfiguration()
    {
        if(displayed_!=saved_)changed_=true;
        saved_=displayed_;s_.frontend->joystickConfigurations[port_>0?1u:0u]=saved_;
        joystickSetUserConfiguration(&Input(s_),port_,saved_);
    }
    std::uint32_t GetButtonChar(std::int32_t first,std::int32_t second=0,std::int32_t third=0) const
    {
        const auto* input=&Input(s_);auto value=joystickGetButtonChar(input,port_,first);
        if(!value)value=joystickGetButtonChar(input,port_,second);
        if(!value)value=joystickGetButtonChar(input,port_,third);
        return value;
    }
    void DrawScreen();
    MainMenuScreenServices& s_;
    std::int32_t port_,displayed_,saved_;
    std::uint8_t type_;
    std::uint32_t screen_;
    bool plugged_;
    std::int32_t popup_{};
    bool changed_{};
    FlailerBitmapChunk* left_;
    FlailerBitmapChunk* right_;
    std::string popupMessage_;
};
namespace {std::unique_ptr<JoystickConfigScreen> configOwner;}

void JoystickConfigScreen::ScreenCallBack(ScreenMessage message,ScreenCallbackData* data)
{
    if(message==ScreenMessage::ScreenDeactivate){configOwner.reset();return;}
    Require(data,"Joystick configuration callback has no actual payload");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!configOwner,"Joystick configuration received duplicate activation");
        configOwner=std::make_unique<JoystickConfigScreen>(Services(),data->joystickPort,data->key);return;
    }
    Require(configOwner!=nullptr,"Joystick configuration callback arrived outside its screen lifetime");
    configOwner->HandleScreenCallback(message,*data);
}
void JoystickConfigScreen::HandleScreenCallback(ScreenMessage message,ScreenCallbackData& data)
{
    if(message==ScreenMessage::LeaveScreen) {
        if(data.joystickPort!=port_){data.value=0;data.sound=0;}
        else {SaveConfiguration();if(changed_)PushAutosave(s_);}
    }
    if(message==ScreenMessage::Tick) {
        std::array<std::int32_t,10> configs{};
        if(Available(configs)>=2) {
            AnimateCursor(*s_.ui,left_,right_,0);s_.screens->SetHelpOverride(nullptr);
        } else {
            Require(left_ && right_,"Joystick configuration lacks its authored arrow bitmaps");
            left_->SetAlpha(0);right_->SetAlpha(0);s_.screens->SetHelpOverride(FixedConfigHelp);
        }
        auto& input=Input(s_);const bool nowPlugged=joystickIsPluggedIn(&input,port_)!=0;
        if(nowPlugged!=plugged_) {
            plugged_=nowPlugged;
            if(plugged_) {
                const auto type=joystickGetType(&input,port_);
                if(type_!=type){type_=type;InitCurrentDisplayedConfig();}
                s_.screens->DismissPopup(popup_);
            } else {
                popupMessage_=PortMessage(s_,0xdbba5u,port_);RefreshHelp(s_,ConfigHelp);
                popup_=s_.screens->SummonPopup(Popup(s_,port_,0,popupMessage_.c_str(),0,0,0,false));
            }
        }
        if(plugged_) {
            const auto type=joystickGetType(&input,port_);
            if(type_!=type){type_=type;InitCurrentDisplayedConfig();}
        }
        return;
    }
    if(message==ScreenMessage::RenderModels){DrawScreen();return;}
    if(data.joystickPort!=port_)return;
    if(message==ScreenMessage::Select) {
        SaveConfiguration();if(changed_)PushAutosave(s_);MoveBack(s_,port_);return;
    }
    if(message==ScreenMessage::Left || message==ScreenMessage::Right) {
        const bool left=message==ScreenMessage::Left;s_.ui->sound->Play(left?1:2);
        if(data.key==MenuFirst)CycleConfiguration(left);
    }
}
void JoystickConfigScreen::DrawScreen()
{
    auto& resources=*s_.ui->resources;ControllerPicture(s_,screen_,displayed_);
    std::array<std::int32_t,10> values{};const auto count=Available(values);
    const auto found=std::find(values.begin(),values.begin()+count,displayed_);
    if(count<=0)resources.Printf(screen_,MenuFirst,"No configs available");
    else if(found==values.begin()+count)resources.Printf(screen_,MenuFirst,"Configuration not found");
    else resources.Printf(screen_,MenuFirst,"%c",static_cast<int>('A'+(found-values.begin())));
    constexpr std::array<std::array<std::uint32_t,2>,16> labels{{
        {{0x7cc1f1ff,0x1d4cd}},{{0x336aa372,0x1d4ce}},{{0x6bee2b3c,0x1d8ac}},{{0x866729bf,0x1d8ad}},
        {{0xd565eebb,0x1d4ca}},{{0xdc950ebf,0x1d4cb}},{{0x3991ed98,0x1d4ca}},{{0x6184611c,0x1d4cb}},
        {{0x487e6614,0x1d4c1}},{{0xe9c869c3,0x1d4c2}},{{0xfcf4e8c8,0x1d4c3}},{{0x42d77800,0x1d4c8}},
        {{0x1bf89b53,0x1d4c7}},{{0xecd38543,0x1d4c6}},{{0x4c040295,0x1d8a9}},{{0x15524647,0x1d4c5}}}};
    for(const auto& item:labels)resources.SetLanguageID(screen_,item[0],item[1]);
    if(!joystickIsPluggedIn(&Input(s_),port_))return;
    const auto print=[&](std::uint32_t key,std::uint32_t character) {
        resources.Printf(screen_,key,"%c",static_cast<int>(character));
    };
    const auto secondary=[&](std::uint32_t key,std::uint32_t label,std::uint32_t character) {
        resources.SetObjectVisibility(screen_,key,character?1:0);
        resources.SetObjectVisibility(screen_,label,character?1:0);
        if(character)print(key,character);
    };
    print(0xff20a4fbu,GetButtonChar(0x6f));print(0xd3d405d8u,GetButtonChar(0x83));
    print(0x3d6cd4bbu,GetButtonChar(0x82));print(0x6c443937u,GetButtonChar(0x7c));
    print(0x595759bbu,GetButtonChar(0x7b));print(0x426b9bb0u,GetButtonChar(0x72,0x71));
    print(0x0cf6153fu,GetButtonChar(0x75,0x74));print(0x85b274e4u,GetButtonChar(0x7a,0x79));
    print(0x87e6eb1cu,GetButtonChar(0x7b));print(0x852c78cfu,GetButtonChar(0x7c));
    print(0x7164a0bfu,GetButtonChar(0x5e));print(0xb6a4c851u,GetButtonChar(0x7e));
    print(0xa9bb8243u,GetButtonChar(0x5d));
    secondary(0xcdf8d89au,0x7f670764u,GetButtonChar(0x70));
    secondary(0x967b758fu,0x9a62deb9u,GetButtonChar(0x73));
    secondary(0xe5fa6ddeu,0x7eab8828u,GetButtonChar(0x76));
    std::uint32_t button{},shift{};
    joystickGetAndShiftButtonChar(&Input(s_),port_,0x5f,&button,&shift);
    print(0x55f013b4u,button);secondary(0x05a3ec91u,0xe98f9ed5u,shift);
    joystickGetAndShiftButtonChar(&Input(s_),port_,0x60,&button,&shift);
    print(0x7c30f7b8u,button);secondary(0xede8c395u,0xd953a559u,shift);
    joystickGetAndShiftButtonChar(&Input(s_),port_,0x7f,&button,&shift);
    print(0x8adf84ceu,button);secondary(0x380e492bu,0x5f73d02fu,shift);
}

class JoystickCalibrationScreen final {
public:
    JoystickCalibrationScreen(MainMenuScreenServices& services,std::uint32_t screen,std::int32_t port);
    ~JoystickCalibrationScreen();
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
    void JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value)
    {
        Require(port==port_ && event>=0x6f && event<=0x76,"Calibration input lost its typed original target");
        if(event<=0x70)steering_=value;else if(event<=0x73)throttle_=value;else brake_=value;
    }
private:
    using Calibration=std::array<std::uint8_t,8>;
    void StoreCurrentJoystickCalibration(Calibration& value) const
    {for(int axis=1;axis<=3;++axis)joystickGetCalibration(&Input(s_),port_,axis,&value[axis],&value[axis+4]);}
    void SetCurrentJoystickCalibration(const Calibration& value)
    {for(int axis=1;axis<=3;++axis)joystickSetCalibration(&Input(s_),port_,axis,value[axis],value[axis+4]);}
    static void ResetJoystickCalibration(Calibration& value)
    {for(int axis=1;axis<=3;++axis){value[axis]=0;value[axis+4]=255;}}
    std::int32_t SafeSummonPopup(const FlailerPopupConfig& config)
    {
        RefreshHelp(s_,CalibrationHelp);
        for(const auto slot:{2u,0u,1u,3u,4u})SafeDismissPopup(s_,popups_[slot],PopupIds[slot]);
        return s_.screens->SummonPopup(config);
    }
    void InvalidCalibration(bool steering);
    void AdvanceCurrentState();
    void DrawScreen();
    void DrawSteeringWheel(bool visible);
    void DrawPedal(bool brake,bool visible);
    MainMenuScreenServices& s_;
    std::int32_t port_;
    std::uint8_t type_;
    std::uint32_t screen_;
    bool plugged_;
    static constexpr std::array<std::uint32_t,5> PopupIds{{0x66,0x64,0x65,0x67,0x68}};
    std::array<std::int32_t,5> popups_{{-1,-1,-1,-1,-1}};
    std::int32_t state_{1};
    bool changed_{};
    std::uint8_t steering_{128},throttle_{},brake_{};
    std::array<std::int32_t,2> pedalTop_{},pedalBottom_{};
    Calibration saved_{},staged_{};
    std::string popupMessage_;
};
namespace {std::unique_ptr<JoystickCalibrationScreen> calibrationOwner;}

JoystickCalibrationScreen::JoystickCalibrationScreen(MainMenuScreenServices& services,std::uint32_t screen,std::int32_t port)
    :s_(services),port_(port),type_(joystickGetType(&Input(s_),port)),screen_(screen),
     plugged_(joystickIsPluggedIn(&Input(s_),port)!=0)
{
    Require(port_>=0 && port_<2,"Joystick calibration port is invalid");
    StoreCurrentJoystickCalibration(saved_);ResetJoystickCalibration(staged_);
    const std::uint32_t keys[]={0x50bbd54cu,0x231ac1ceu};
    for(std::size_t i=0;i<2;++i) {
        auto* image=Bitmap(s_,screen_,keys[i]);Require(image,"Calibration has no authored pedal reference bitmap");
        float x,y;s_.ui->resources->GetTopLeft(*image,x,y,s_.ui->language);
        pedalTop_[i]=convertFloatToInt32(y);
        pedalBottom_[i]=pedalTop_[i]+convertFloatToInt32(s_.ui->resources->ScaledBitmapHeight(*image));
        if(pedalTop_[i]==pedalBottom_[i])++pedalBottom_[i];
    }
    auto& input=Input(s_);
    Require(input.registrations && input.count<=input.capacity && input.capacity-input.count>=8 &&
        input.nextOrder<=std::numeric_limits<std::uint64_t>::max()-8,
        "Calibration cannot register its original eight analog handlers");
    for(std::size_t i=0;i<input.count;++i)
        Require(input.registrations[i].device!=port_ || input.registrations[i].event<0x6f || input.registrations[i].event>0x76,
            "Calibration analog event already belongs to another live owner");
    GameInputTarget target{};target.kind=GAME_INPUT_JOYSTICK_CALIBRATION;target.owner.joystickCalibration=this;
    for(std::int32_t event=0x6f;event<=0x76;++event)
        Require(gameInputRegister(&input,port_,event,target)==GAME_INPUT_OK,"Calibration analog registration failed");
}
JoystickCalibrationScreen::~JoystickCalibrationScreen()
{
    SetCurrentJoystickCalibration(saved_);
    for(std::int32_t event=0x6f;event<=0x76;++event)gameInputUnregister(&Input(s_),port_,event);
}
void JoystickCalibrationInput(JoystickCalibrationScreen* owner,std::int32_t port,std::int32_t event,std::uint8_t value)
{
    Require(owner,"Calibration input target has no owner");owner->JoyHandler(port,event,value);
}
void JoystickCalibrationScreen::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    if(message==ScreenMessage::ScreenDeactivate){calibrationOwner.reset();return;}
    Require(data,"Calibration screen callback has no original payload");
    if(message==ScreenMessage::ScreenActivate) {
        Require(!calibrationOwner,"Calibration received duplicate activation");
        calibrationOwner=std::make_unique<JoystickCalibrationScreen>(Services(),data->key,data->joystickPort);return;
    }
    Require(calibrationOwner!=nullptr,"Calibration callback arrived outside its actual lifetime");
    calibrationOwner->HandleEvent(message,*data);
}
void JoystickCalibrationScreen::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if(message==ScreenMessage::LeaveScreen) {
        if(data.joystickPort!=port_){data.sound=0;data.value=0;return;}
        if(state_==1)return;
        const auto* text=s_.ui->language->GetString(0xdbba9u);Require(text,"Calibration exit message is missing");
        popupMessage_=text;
        popups_[2]=SafeSummonPopup(Popup(s_,port_,0x65,popupMessage_.c_str(),0x24a55,3,4,true,1));
        data.value=0;return;
    }
    if(message==ScreenMessage::Tick) {
        auto& input=Input(s_);const bool plugged=joystickIsPluggedIn(&input,port_)!=0;
        if(plugged!=plugged_) {
            plugged_=plugged;
            if(plugged_) {type_=joystickGetType(&input,port_);SafeDismissPopup(s_,popups_[0],0x66);}
            else {
                popupMessage_=PortMessage(s_,0xdbba5u,port_);
                popups_[0]=SafeSummonPopup(Popup(s_,port_,0x66,popupMessage_.c_str(),0,5,6,false));
            }
        }
        if(!plugged_)return;
        if(!joystickCanBeCalibrated(joystickGetType(&input,port_))) {
            if(!SafeIsPopupActive(s_,popups_[1],0x64)) {
                popupMessage_=PortMessage(s_,0xdbbaau,port_);
                popups_[1]=SafeSummonPopup(Popup(s_,port_,0x64,popupMessage_.c_str(),0x24a55,1,2));
            }
        } else SafeDismissPopup(s_,popups_[1],0x64);
        return;
    }
    if(message==ScreenMessage::PopupDismiss) {
        if(data.value==1 || data.value==3){MoveBack(s_,port_);return;}
        if(data.value==7 || data.value==8)state_=2;
        else if(data.value==9 || data.value==10)state_=4;
        return;
    }
    if(message==ScreenMessage::RenderModels) {
        for(std::size_t i=0;i<popups_.size();++i)if(SafeIsPopupActive(s_,popups_[i],PopupIds[i]))return;
        DrawScreen();return;
    }
    if(data.joystickPort==port_ && message==ScreenMessage::Select)AdvanceCurrentState();
}
void JoystickCalibrationScreen::InvalidCalibration(bool steering)
{
    const auto* text=s_.ui->language->GetString(0xdbba6u);Require(text,"Calibration validation message is missing");
    popupMessage_=text;const auto slot=steering?3u:4u;
    popups_[slot]=SafeSummonPopup(Popup(s_,port_,PopupIds[slot],popupMessage_.c_str(),0xdbba7u,
        steering?7:9,steering?8:10));
}
void JoystickCalibrationScreen::AdvanceCurrentState()
{
    auto& input=Input(s_);
    switch(state_) {
    case 1:
        ResetJoystickCalibration(staged_);SetCurrentJoystickCalibration(staged_);state_=2;break;
    case 2:
        staged_[1]=steering_;
        if(static_cast<int>(steering_)<123-static_cast<int>(joystickGetDeadzone(&input,port_,1)))state_=3;
        else InvalidCalibration(true);
        break;
    case 3:
        staged_[5]=steering_;
        if(staged_[1]<steering_ && static_cast<int>(steering_)>133+static_cast<int>(joystickGetDeadzone(&input,port_,1)))state_=4;
        else InvalidCalibration(true);
        break;
    case 4:
        staged_[6]=throttle_;staged_[7]=brake_;
        if(static_cast<int>(throttle_)>5+static_cast<int>(joystickGetDeadzone(&input,port_,2)) &&
           static_cast<int>(brake_)>5+static_cast<int>(joystickGetDeadzone(&input,port_,3)))state_=5;
        else InvalidCalibration(false);
        break;
    case 5:
        staged_[2]=throttle_;staged_[3]=brake_;
        if(throttle_<staged_[6] && brake_<staged_[7] &&
           static_cast<int>(throttle_)<250-static_cast<int>(joystickGetDeadzone(&input,port_,2)) &&
           static_cast<int>(brake_)<250-static_cast<int>(joystickGetDeadzone(&input,port_,3))) {
            SetCurrentJoystickCalibration(staged_);state_=6;
        } else InvalidCalibration(false);
        break;
    case 6:
        saved_=staged_;changed_=true;PushAutosave(s_);MoveBack(s_,port_);break;
    default:break;
    }
}
void JoystickCalibrationScreen::DrawSteeringWheel(bool visible)
{
    s_.ui->resources->SetObjectVisibility(screen_,0x0b2ad6dfu,visible?1:0);
    if(visible)if(auto* image=Bitmap(s_,screen_,0x0b2ad6dfu))
        image->SetRotation(static_cast<float>(static_cast<int>(steering_)-128)*0.703125f);
}
void JoystickCalibrationScreen::DrawPedal(bool brake,bool visible)
{
    const std::uint32_t image=brake?0x7c42ddecu:0xac3079aau;
    const std::uint32_t reference=brake?0x231ac1ceu:0x50bbd54cu;
    const std::uint32_t label=brake?0xb43340d6u:0x487e6614u;
    auto& resources=*s_.ui->resources;
    for(auto key:{image,reference,label})resources.SetObjectVisibility(screen_,key,visible?1:0);
    if(!visible)return;
    const auto slot=brake?1u:0u;
    const auto length=static_cast<float>(pedalBottom_[slot]-pedalTop_[slot])*0.003921568859368563f;
    const float height=static_cast<float>(brake?brake_:throttle_)*length;
    if(auto* bitmap=Bitmap(s_,screen_,image)) {
        bitmap->SetTopLeftY(static_cast<float>(pedalBottom_[slot]-convertFloatToInt32(height)));
        // Complete final141528 dependency: use authored signed texture+26
        // height, not the current Y scale (which can legitimately be zero).
        (void)resources.ScaledBitmapHeight(*bitmap);
        const auto raw=ReadU16(bitmap->texture->authoredRecord+38);
        std::int16_t textureHeight;std::memcpy(&textureHeight,&raw,sizeof(textureHeight));
        bitmap->SetScaleY(height/static_cast<float>(textureHeight));
    }
}
void JoystickCalibrationScreen::DrawScreen()
{
    auto& resources=*s_.ui->resources;
    resources.SetLanguageID(screen_,0x03c458c1u,0xc3d3au);
    resources.SetLanguageID(screen_,0x487e6614u,0xc3d3bu);
    resources.SetLanguageID(screen_,0xb43340d6u,0xc3d3cu);
    const auto* title=s_.ui->language->GetString(0xc3d3du);Require(title,"Calibration title is missing");
    const auto caption=FormatLocalizedIntegerText(title,port_+1,128);
    resources.Printf(screen_,0x15380cd3u,"%s",caption.c_str());
    auto& input=Input(s_);ControllerPicture(s_,screen_,
        joystickChooseConfiguration(joystickGetType(&input,port_),joystickGetUserConfiguration(&input,port_)));
    const bool wheel=state_==1 || state_==2 || state_==3 || state_==6;
    const bool pedal=state_==1 || state_==4 || state_==5 || state_==6;
    std::uint32_t language=0;
    if(state_>=1 && state_<=6) {
        DrawSteeringWheel(wheel);DrawPedal(false,pedal);DrawPedal(true,pedal);
        language=0xc3d33u+static_cast<std::uint32_t>(state_);
    }
    const auto* text=s_.ui->language->GetString(language);
    Require(text,"Calibration step has no actual localized instructions");
    s_.ui->render->RenderTextIntoRectangle(resources,screen_,0x4f79f553u,0x31e15ffeu,text,1.0f);
}

class JoystickPreConfig final : public OptionsScreenBase {
public:
    JoystickPreConfig(MainMenuScreenServices& services,std::uint32_t screen)
        :OptionsScreenBase(services,screen),s_(services),screen_(screen),
         highlight_(*s_.ui,screen_,0xab671c9au,s_.ui->animationSettings.highlightColor),
         selectionChange_(*s_.ui,screen_,0x1042eac3u,-1),
         blurb_(*s_.ui,screen_,0x75980cf0u,0x75980cf0u,0x70a404ecu)
    {
        const auto* label=static_cast<FlailerTextChunk*>(s_.ui->resources->FindObject(screen_,0x3fabc377u,1));
        Require(label,"Joystick preconfiguration lacks its original vibration label");savedColor_=label->color;
        constexpr std::uint32_t keys[]={0x7bcd6704u,0x7c51b6d7u,0x7bcd6705u,0x7c51b6d8u};
        for(std::size_t i=0;i<arrows_.size();++i) {
            arrows_[i]=Bitmap(s_,screen_,keys[i]);Require(arrows_[i],"Joystick preconfiguration lacks its authored arrows");
        }
        for(int port=0;port<2;++port){types_[port]=joystickGetType(&Input(s_),port);savedVibration_[port]=GetVibration(port);}
        UpdateMenuItems(0,false);UpdateMenuItems(1,false);
    }
    ~JoystickPreConfig() override
    {
        s_.ui->resources->SetTextColor(screen_,0x3fabc377u,savedColor_);
        s_.ui->resources->SetTextColor(screen_,0xe2f33138u,savedColor_);
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override;
    static void Callback(ScreenMessage message,ScreenCallbackData* data)
    {
        if(message==ScreenMessage::ScreenActivate) {
            Require(data,"Joystick preconfiguration activation has no original payload");
            Install(std::make_unique<JoystickPreConfig>(BoundServices(),data->key));
        } else BaseCallback(message,data);
    }
private:
    bool GetVibration(int port) const
    {
        const auto* profile=s_.frontend->players[port>0?1u:0u];Require(profile,"Joystick vibration has no actual player settings");
        return profile->settings.authoredOption!=0;
    }
    void SetVibration(int port,bool value) {s_.frontend->players[port>0?1u:0u]->settings.authoredOption=value?1:0;}
    bool ToggleVibration(int port) {const bool enabled=!GetVibration(port);SetVibration(port,enabled);return enabled;}
    void DoTestVibration(int port)
    {
        auto& input=Input(s_);
        if(joystickIsTypeWheel(&input,port)) {
            // Source240BD0 calls PortPlayCrash(port,500,0.1f,0.0f). Its
            // first lookup12C744 returns without writes when no registered
            // wheel exists. The current native provider exposes no steering
            // force endpoint; an actual registered wheel remains explicit.
            Require(joystickGetWheelId(&input,port)==2,
                "Registered steering wheel has no native PortPlayCrash endpoint");return;
        }
        Require(s_.ui->flow && s_.sounds && s_.sounds->spatial.players,
            "Joystick rumble test lacks the canonical flow/player owners");
        joystickPortRumbleImpact(input,port,8.0f,*s_.ui->flow,s_.sounds->session,
            *s_.frontend,*s_.sounds->spatial.players);
    }
    void UpdateMenuItems(int port,bool test)
    {
        auto& input=Input(s_);const bool plugged=joystickIsPluggedIn(&input,port)!=0;
        const auto type=joystickGetType(&input,port);const auto base=MenuFirst+(port?3u:0u);
        SetMenuAvailability(s_,screen_,base,plugged);
        SetMenuAvailability(s_,screen_,base+1,plugged && joystickCanBeCalibrated(type));
        // Final11D3B0 and12C138 are the exact source capability predicates.
        const bool vibration=plugged && (type==0x73 || type==0x79 || type==0x15 || type==0x16);
        SetMenuAvailability(s_,screen_,base+2,vibration);
        const auto label=port?0xe2f33138u:0x3fabc377u;
        if(vibration) {
            s_.ui->resources->SetTextColor(screen_,label,savedColor_);
            if(test){SetVibration(port,true);DoTestVibration(port);}
        } else {
            const auto& rgb=s_.ui->inactiveRgb;
            s_.ui->resources->SetTextColor(screen_,label,rgb[0]|(rgb[1]<<8)|(rgb[2]<<16));
            SetVibration(port,false);
        }
    }
    MainMenuScreenServices& s_;
    std::uint32_t screen_;
    std::int32_t popup_{};
    std::uint32_t savedColor_{};
    std::array<std::uint8_t,2> types_{};
    std::array<bool,2> savedVibration_{};
    std::array<FlailerBitmapChunk*,4> arrows_{};
    std::array<float,2> arrowTimers_{};
    FlailerHighlightAnimation highlight_,selectionChange_;
    FlailerAnimatedBlurb blurb_;
    std::string popupMessage_;
};
void JoystickPreConfig::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if(message==ScreenMessage::LeaveScreen) {
        if(savedVibration_[0]!=GetVibration(0) || savedVibration_[1]!=GetVibration(1))PushAutosave(s_);
        return;
    }
    if(message==ScreenMessage::Tick) {
        blurb_.Animate();auto& input=Input(s_);
        if(joystickIsPluggedIn(&input,0) || joystickIsPluggedIn(&input,1)) {
            SafeDismissPopup(s_,popup_,0x66);popup_=0;
            AnimateCursor(*s_.ui,arrows_[0],arrows_[1],&arrowTimers_[0]);
            AnimateCursor(*s_.ui,arrows_[2],arrows_[3],&arrowTimers_[1]);
            highlight_.Animate();selectionChange_.Animate();
            for(int port=0;port<2;++port) {
                const auto type=joystickGetType(&input,port);
                if(types_[port]!=type){types_[port]=type;UpdateMenuItems(port,true);ResetCurrentMenus(s_);}
            }
            s_.ui->resources->SetLanguageID(screen_,0x3fabc377u,GetVibration(0)?0x1d525u:0x1d526u);
            s_.ui->resources->SetLanguageID(screen_,0xe2f33138u,GetVibration(1)?0x1d525u:0x1d526u);
        } else if(!SafeIsPopupActive(s_,popup_,0x66)) {
            const auto* text=s_.ui->language->GetString(0xdbbabu);Require(text,"Missing-controller popup text is absent");
            popupMessage_=text;RefreshHelp(s_,PreConfigHelp);SafeDismissPopup(s_,popup_,0x66);
            popup_=s_.screens->SummonPopup(Popup(s_,-1,0x66,popupMessage_.c_str(),0,5,6,false));
        }
        return;
    }
    if(message==ScreenMessage::Highlight) {
        highlight_.SetCurrentMenuItem(data.key);for(auto* arrow:arrows_)arrow->visible=0;
        selectionChange_.SetCurrentMenuItem(0);
        if(data.key==MenuFirst+2) {
            arrows_[0]->visible=arrows_[1]->visible=1;selectionChange_.SetCurrentMenuItem(0x3fabc377u);
        } else if(data.key==MenuFirst+5) {
            arrows_[2]->visible=arrows_[3]->visible=1;selectionChange_.SetCurrentMenuItem(0xe2f33138u);
        }
        const auto id=data.key-MenuFirst;
        blurb_.NewText(id<6?static_cast<std::int32_t>(0x24c48u+id):0);return;
    }
    if(message==ScreenMessage::Select) {
        const char* destination=nullptr;
        if(data.joystickPort==0) {
            if(data.key==MenuFirst)destination="Joystick Config Port1";
            else if(data.key==MenuFirst+1)destination="Joystick Calibration Port1";
        } else if(data.joystickPort==1) {
            if(data.key==MenuFirst+3)destination="Joystick Config Port2";
            else if(data.key==MenuFirst+4)destination="Joystick Calibration Port2";
        }
        if(destination){s_.screens->ChangeToScreen(destination,true);return;}
        data.sound=0;
    } else if(message!=ScreenMessage::Left && message!=ScreenMessage::Right)return;
    int port=-1;
    if(data.key==MenuFirst+2 && data.joystickPort==0)port=0;
    if(data.key==MenuFirst+5 && data.joystickPort==1)port=1;
    if(port<0)return;
    if(ToggleVibration(port))DoTestVibration(port);
    selectionChange_.ForceStart();s_.ui->sound->Play(message==ScreenMessage::Left?1:2);
}

void BindJoystickScreens(MainMenuScreenServices& services)
{
    Require(!configOwner && !calibrationOwner,"Joystick bindings cannot replace a live concrete screen");
    joystickServices=&services;(void)Services();
}
void UnbindJoystickScreens()
{
    calibrationOwner.reset();configOwner.reset();joystickServices=nullptr;
}
std::array<FlailerScreenConfig,5>& JoystickScreenConfigurations()
{
    // Complete final2410F0 constructor arguments, source strings and help.
    static std::array<FlailerScreenConfig,5> result{{
        {"Joystick Config Port1",JoystickConfigScreen::ScreenCallBack,{{static_cast<std::int32_t>(0xb6657464u),0,0,0}},{{0,-1,-1,-1}},1,ConfigHelp},
        {"Joystick Config Port2",JoystickConfigScreen::ScreenCallBack,{{static_cast<std::int32_t>(0xb6657464u),0,0,0}},{{1,-1,-1,-1}},1,ConfigHelp},
        {"Joystick Calibration Port1",JoystickCalibrationScreen::Callback,{{static_cast<std::int32_t>(0xa4031ef6u),0,0,0}},{{0,-1,-1,-1}},1,CalibrationHelp},
        {"Joystick Calibration Port2",JoystickCalibrationScreen::Callback,{{static_cast<std::int32_t>(0xa4031ef6u),0,0,0}},{{1,-1,-1,-1}},1,CalibrationHelp},
        {"Joystick PreConfig",JoystickPreConfig::Callback,{{static_cast<std::int32_t>(0x8bd5060bu),0,0,0}},{{-1,-1,-1,-1}},1,PreConfigHelp}
    }};return result;
}
} // namespace hp2

#include "indep/inc/CarDriver.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "indep/inc/Flags.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdio>
#include <cstdlib>

#include <string.h>

// Preserve the game sample cadence and ring ordering while storing typed
// host controls directly. No SDK packet/header is synthesized or parsed.
static void capture(JoystickSystem* joystick)
{
    if (!joystick->initialized)
        return;
    const int32_t next = (joystick->top + 1) & (JOYSTICK_SAMPLES - 1);
    if (next == joystick->bottom)
        return;

    for (int32_t port = 0; port < JOYSTICK_PORT_COUNT; ++port) {
        JoystickSample* sample = &joystick->samples[port][joystick->top];
        memset(sample, 0, sizeof(*sample));
        if (joystick->read == NULL ||
            !joystick->read(port, &sample->controls)) {
            // Preserve the former disconnected sample's logical values for
            // source edge/hysteresis semantics; its profile rejects input.
            sample->controls.held=0;
            memset(sample->controls.sticks,255,sizeof sample->controls.sticks);
            memset(sample->controls.pressures,255,sizeof sample->controls.pressures);
            sample->controls.mappingProfile=JOYSTICK_PROFILE_DISCONNECTED;
            sample->controllerType=JOYSTICK_PROFILE_DISCONNECTED;
        }
    }
    joystick->top = next;
}

void joystickInitialize(JoystickSystem* joystick, JoystickReader reader)
{
    memset(joystick, 0, sizeof(*joystick));
    joystick->read = reader;
    for(auto& port:joystick->samples)for(auto& sample:port) {
        // Equivalent initial logical values of the original zeroed ring.
        sample.controls.held=0xffff;
        sample.controls.mappingProfile=JOYSTICK_PROFILE_DISCONNECTED;
    }
    joystick->initialized = 1;
}

void joystickShutdown(JoystickSystem* joystick)
{
    joystick->initialized = 0;
    joystick->read = NULL;
}

void joystickSetReader(JoystickSystem* joystick, JoystickReader reader)
{
    joystick->read = reader;
}

void joystickSetVblankReading(JoystickSystem* joystick, int enabled)
{
    joystick->readInVblank = enabled != 0;
}

void joystickReadVblank(JoystickSystem* joystick)
{
    if (joystick->readInVblank)
        capture(joystick);
}

int32_t joystickPadType(const JoystickSample* sample)
{
    const auto profile=sample->controls.mappingProfile;
    return profile>0 && profile<=255?profile:JOYSTICK_PROFILE_DISCONNECTED;
}

void joystickPrepareFrame(JoystickSystem* joystick)
{
    if (!joystick->readInVblank)
        capture(joystick);

    joystick->preparedTop = joystick->top;
    if (joystick->bottom == joystick->preparedTop)
        return;

    for (int32_t port = 0; port < JOYSTICK_PORT_COUNT; ++port) {
        const int32_t padState = joystick->portState[port];
        int32_t cursor = joystick->bottom;
        do {
            JoystickSample* sample = &joystick->samples[port][cursor];
            sample->controllerType = padState == -1 ? -1 : joystickPadType(sample);
            cursor = (cursor + 1) & (JOYSTICK_SAMPLES - 1);
        } while (cursor != joystick->preparedTop);
    }
}

int32_t joystickHasFrame(const JoystickSystem* joystick)
{
    return joystick->bottom != joystick->preparedTop;
}

void joystickAdvanceFrame(JoystickSystem* joystick)
{
    joystick->bottom = (joystick->bottom + 1) & (JOYSTICK_SAMPLES - 1);
}

const JoystickControls* joystickCurrentData(const JoystickSystem* joystick, int32_t port,
                                    int32_t* controllerType, const JoystickControls** previousData)
{
    if (port < 0 || port >= JOYSTICK_PORT_COUNT ||
        controllerType == NULL || previousData == NULL)
        return NULL;
    const int32_t current = joystick->bottom;
    *controllerType = joystick->samples[port][current].controllerType;
    *previousData = &joystick->samples[port][(current - 1) & (JOYSTICK_SAMPLES - 1)].controls;
    return &joystick->samples[port][current].controls;
}


int32_t joystickSupportsPressureMode(uint8_t type)
{
    return type == 0x73 || type == 0x79;
}


#include "indep/inc/CarDriver.hpp"
int32_t joystickCanBeCalibrated(uint8_t type)
{
    return type==0x15 || type==0x16 || type==0x23 || type==0x53;
}
int32_t joystickHasStiffThrottle(const GameInputRouter* router,int32_t port)
{
    const auto type=joystickGetType(router,port);
    return type==0x53 || type==0x73 || type==0x79;
}
int32_t joystickIsTypeWheel(const GameInputRouter* router,int32_t port)
{
    const auto type=joystickGetType(router,port);
    return type==0x15 || type==0x16 || type==0x23;
}
void joystickSetRumble(GameInputRouter* router,int32_t port,uint8_t smallMotor,uint8_t largeMotor)
{
    if (!router || port<0 || port>=2)return;
    auto& bytes=router->pendingMotorBytes[port];
    bytes[0]=smallMotor;bytes[1]=largeMotor;
    bytes[2]=0;bytes[3]=0;bytes[4]=0;bytes[5]=0;
    router->pendingMotorDirty[port]=1;
}

// Native device configuration and logical control mapping.
#include "indep/inc/CarDriver.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace hp2 {
bool NativeKeyboardEnabled(const NativeInputSettings& settings,std::int32_t port) noexcept
{
    if(port<0 || port>=2)return false;
    const auto mode=settings.deviceMode[static_cast<std::size_t>(port)];
    return mode==NativeDeviceMode::Keyboard || mode==NativeDeviceMode::KeyboardAndGamepad ||
        (mode==NativeDeviceMode::Automatic && port==0);
}
bool NativeGamepadEnabled(const NativeInputSettings& settings,std::int32_t port) noexcept
{
    if(port<0 || port>=2)return false;
    const auto index=static_cast<std::size_t>(port);
    const auto mode=settings.deviceMode[index];
    return mode==NativeDeviceMode::Gamepad || mode==NativeDeviceMode::KeyboardAndGamepad ||
        (mode==NativeDeviceMode::Automatic && (settings.controller[index]>=0 || !settings.deviceKey[index].empty()));
}
const std::array<std::uint16_t,20>& NativeKeyboardBindings(const NativeInputSettings& settings,std::int32_t port)
{
    if(port<0 || port>=2)throw std::out_of_range("Native input player port");
    return port==0?settings.keys:settings.player2Keys;
}
void ValidateNativeInputSettings(const NativeInputSettings& settings)
{
    for(const auto* keys:{&settings.keys,&settings.player2Keys})for(const auto key:*keys)
        if(key>254)throw std::invalid_argument("Keyboard binding is outside the Windows virtual-key range");
    for(const auto controller:settings.controller)if(controller<-1 || controller>255)
        throw std::invalid_argument("Invalid legacy controller index");
    if(settings.leftDeadzone<0 || settings.leftDeadzone>29490 ||
        settings.rightDeadzone<0 || settings.rightDeadzone>29490)
        throw std::invalid_argument("Stick deadzone must be between 0 and 90 percent");
    if(settings.triggerThreshold<0 || settings.triggerThreshold>229)
        throw std::invalid_argument("Trigger deadzone must be between 0 and 90 percent");
    for(std::size_t player=0;player<2;++player) {
        const auto mode=settings.deviceMode[player];
        if(mode<NativeDeviceMode::Automatic || mode>NativeDeviceMode::Disabled)
            throw std::invalid_argument("Unknown player input device mode");
        const auto& key=settings.deviceKey[player];
        if(key.size()>4096 || std::any_of(key.begin(),key.end(),[](unsigned char c){return c<32 || c==127;}))
            throw std::invalid_argument("Invalid persistent controller identity");
        const auto& profile=settings.profiles[player];
        for(const auto& binding:profile.buttons) {
            if(binding.kind<NativeBindingKind::None || binding.kind>NativeBindingKind::AxisFullNegative)
                throw std::invalid_argument("Unknown controller binding kind");
            if(binding.kind==NativeBindingKind::None)continue;
            const int limit=binding.kind==NativeBindingKind::Button?64:
                binding.kind==NativeBindingKind::Hat?4:16;
            if(binding.index<0 || binding.index>=limit)
                throw std::invalid_argument("Controller binding exceeds supported physical inputs");
            if(binding.kind==NativeBindingKind::Hat && binding.direction!=1 && binding.direction!=2 &&
                binding.direction!=4 && binding.direction!=8)
                throw std::invalid_argument("Hat binding must select one direction");
        }
        for(std::size_t axis=0;axis<4;++axis) {
            const int zone=profile.deadzone[axis]<0?(axis<2?settings.rightDeadzone:settings.leftDeadzone):profile.deadzone[axis];
            if(profile.axes[axis]<-1 || profile.axes[axis]>=16 || profile.deadzone[axis]<-1 ||
                profile.deadzone[axis]>29490 || profile.saturation[axis]<1 ||
                profile.saturation[axis]>32767 || zone>=profile.saturation[axis])
                throw std::invalid_argument("Invalid controller axis, deadzone or saturation range");
        }
        if(profile.triggerThreshold<-1 || profile.triggerThreshold>229 ||
            profile.vibrationScale<0 || profile.vibrationScale>100)
            throw std::invalid_argument("Invalid controller trigger deadzone or vibration strength");
    }
    if(NativeGamepadEnabled(settings,0) && NativeGamepadEnabled(settings,1)) {
        const bool sameKey=!settings.deviceKey[0].empty() && settings.deviceKey[0]==settings.deviceKey[1];
        const bool sameIndex=settings.deviceKey[0].empty() && settings.deviceKey[1].empty() &&
            settings.controller[0]>=0 && settings.controller[0]==settings.controller[1];
        if(sameKey || sameIndex)throw std::invalid_argument("The same controller cannot drive both players");
    }
}

void MigrateNativeInputPreferences(NativeInputSettings& settings)
{
    ValidateNativeInputSettings(settings);
    auto migrated=settings;
    for(auto& profile:migrated.profiles) {
        const auto axes=profile.axes;
        const auto inverted=profile.invert;
        for(std::size_t i=0;i<4;++i) {
            const auto source=settings.swapSticks?(i+2)%4:i;
            profile.axes[i]=axes[source];
            profile.invert[i]=inverted[source] ^ (i==2 && settings.invertSteering) ^
                (i==1 && settings.invertLookY);
        }
    }
    migrated.swapSticks=false;migrated.invertSteering=false;migrated.invertLookY=false;
    for(std::size_t player=0;player<2;++player)
        if(migrated.deviceMode[player]==NativeDeviceMode::Automatic && migrated.controller[player]<0 && migrated.deviceKey[player].empty())
            migrated.deviceMode[player]=player==0?NativeDeviceMode::Keyboard:NativeDeviceMode::Disabled;
    ValidateNativeInputSettings(migrated);settings=std::move(migrated);
}

std::int32_t ResolveNativeDevice(const std::vector<NativeInputDeviceInfo>& devices,
    const std::string& key,std::int32_t fallbackIndex) noexcept
{
    int resolved=-1;
    for(std::size_t i=0;i<devices.size();++i) {
        const bool match=key.empty()?devices[i].index==fallbackIndex:
            devices[i].id==key || (!devices[i].pathKey.empty() && devices[i].pathKey==key);
        if(match) {
            if(resolved>=0)return -1; // Ambiguous identities never mean "the first one".
            resolved=static_cast<std::int32_t>(i);
        }
    }
    return resolved; // An unplugged saved device never becomes a different controller.
}

std::int32_t ResolveNativePlayerDevice(const NativeInputSettings& settings,
    const std::vector<NativeInputDeviceInfo>& devices,std::int32_t port) noexcept
{
    if(!NativeGamepadEnabled(settings,port))return -1;
    const auto player=static_cast<std::size_t>(port);
    if(!settings.deviceKey[player].empty())
        return ResolveNativeDevice(devices,settings.deviceKey[player],-1);
    int reserved=-1;
    if(port==1)reserved=ResolveNativePlayerDevice(settings,devices,0);
    else if(NativeGamepadEnabled(settings,1) && !settings.deviceKey[1].empty())
        reserved=ResolveNativeDevice(devices,settings.deviceKey[1],-1);
    const auto usable=[&](int candidate) {
        if(candidate<0 || candidate==reserved)return false;
        const auto& id=devices[static_cast<std::size_t>(candidate)].id;
        return !id.empty() && ResolveNativeDevice(devices,id,-1)==candidate;
    };
    const int preferred=ResolveNativeDevice(devices,"",settings.controller[player]);
    if(usable(preferred))return preferred;
    for(std::size_t i=0;i<devices.size();++i)
        if(usable(static_cast<int>(i)))return static_cast<int>(i);
    return -1;
}

namespace {
NativePadSample CanonicalRawSample(const NativePadSample& pad) noexcept
{
    if(pad.rawValid)return pad;
    auto result=pad;result.rawValid=true;result.standardized=true;
    result.axisCount=6;result.buttonCount=15;result.hatCount=0;
    constexpr std::uint16_t bits[15]={0x1000,0x2000,0x4000,0x8000,0x0020,0,0x0010,0x0040,0x0080,
        0x0100,0x0200,0x0001,0x0002,0x0004,0x0008};
    for(std::size_t i=0;i<15;++i)result.rawButtons[i]=(pad.buttons&bits[i])!=0;
    const auto downward=[](std::int16_t value) noexcept {
        return static_cast<std::int16_t>(std::clamp(-static_cast<int>(value),-32768,32767));
    };
    result.rawAxes[0]=pad.leftX;result.rawAxes[1]=downward(pad.leftY);
    result.rawAxes[2]=pad.rightX;result.rawAxes[3]=downward(pad.rightY);
    result.rawAxes[4]=static_cast<std::int16_t>((static_cast<int>(pad.leftTrigger)*32767+254)/255);
    result.rawAxes[5]=static_cast<std::int16_t>((static_cast<int>(pad.rightTrigger)*32767+254)/255);
    return result;
}
std::uint8_t NativeBindingPressure(const NativeInputBinding& binding,const NativePadSample& raw) noexcept
{
    if(binding.index<0)return 0;
    const auto index=static_cast<std::size_t>(binding.index);
    if(binding.kind==NativeBindingKind::Button)
        return index<raw.rawButtons.size() && binding.index<raw.buttonCount && raw.rawButtons[index]?255:0;
    if(binding.kind==NativeBindingKind::Hat)
        return index<raw.hats.size() && binding.index<raw.hatCount && (raw.hats[index]&binding.direction)!=0?255:0;
    if(index>=raw.rawAxes.size() || binding.index>=raw.axisCount)return 0;
    const int value=raw.rawAxes[index];
    switch(binding.kind) {
        case NativeBindingKind::AxisPositive:return static_cast<std::uint8_t>(std::max(0,value)*255/32767);
        case NativeBindingKind::AxisNegative:return static_cast<std::uint8_t>(std::max(0,-value)*255/32768);
        case NativeBindingKind::AxisFullPositive:return static_cast<std::uint8_t>((value+32768)*255/65535);
        case NativeBindingKind::AxisFullNegative:return static_cast<std::uint8_t>((32767-value)*255/65535);
        default:return 0;
    }
}
}

bool DetectNativeBinding(const NativePadSample& current,const NativePadSample& baseline,
    NativeInputBinding& binding,bool axisOnly) noexcept
{
    if(!current.connected || !baseline.connected)return false;
    if(current.instanceId>=0 && baseline.instanceId>=0 && current.instanceId!=baseline.instanceId)return false;
    const auto raw=CanonicalRawSample(current),before=CanonicalRawSample(baseline);
    if(!axisOnly) {
        for(int i=0;i<std::min(raw.buttonCount,64);++i)if(raw.rawButtons[static_cast<std::size_t>(i)] &&
            !before.rawButtons[static_cast<std::size_t>(i)]) {
            binding={NativeBindingKind::Button,i,0};return true;
        }
        for(int i=0;i<std::min(raw.hatCount,4);++i)for(const int direction:{1,2,4,8})
            if((raw.hats[static_cast<std::size_t>(i)]&direction)!=0 &&
                (before.hats[static_cast<std::size_t>(i)]&direction)==0) {
                binding={NativeBindingKind::Hat,i,direction};return true;
            }
    }
    int selected=-1,largest=16000;
    for(int i=0;i<std::min(raw.axisCount,16);++i) {
        const auto index=static_cast<std::size_t>(i);
        const int movement=std::abs(static_cast<int>(raw.rawAxes[index])-before.rawAxes[index]);
        if(movement>largest) {largest=movement;selected=i;}
    }
    if(selected<0)return false;
    const auto index=static_cast<std::size_t>(selected);
    auto kind=raw.rawAxes[index]>before.rawAxes[index]?NativeBindingKind::AxisPositive:NativeBindingKind::AxisNegative;
    if(!raw.standardized && before.rawAxes[index]<-24576)kind=NativeBindingKind::AxisFullPositive;
    else if(!raw.standardized && before.rawAxes[index]>24576)kind=NativeBindingKind::AxisFullNegative;
    binding={kind,selected,0};return true;
}

std::string NativeBindingName(const NativeInputBinding& binding,bool standardized)
{
    constexpr const char* buttons[]={"A / Cross","B / Circle","X / Square","Y / Triangle","Back / Share",
        "Guide","Start / Options","Left stick","Right stick","Left shoulder","Right shoulder",
        "D-pad up","D-pad down","D-pad left","D-pad right","Miscellaneous","Paddle 1","Paddle 2",
        "Paddle 3","Paddle 4","Touchpad"};
    constexpr const char* axes[]={"Left stick X","Left stick Y","Right stick X","Right stick Y","Left trigger","Right trigger"};
    if(binding.kind==NativeBindingKind::None)return "Unbound";
    if(binding.kind==NativeBindingKind::Button) {
        if(standardized && binding.index>=0 && binding.index<21)return buttons[binding.index];
        return "Button "+std::to_string(binding.index+1);
    }
    if(binding.kind==NativeBindingKind::Hat) {
        const char* direction=binding.direction==1?"Up":binding.direction==2?"Right":binding.direction==4?"Down":"Left";
        return "Hat "+std::to_string(binding.index+1)+" "+direction;
    }
    std::string name=standardized && binding.index>=0 && binding.index<6?axes[binding.index]:
        "Axis "+std::to_string(binding.index+1);
    switch(binding.kind) {
        case NativeBindingKind::AxisNegative:name+=" -";break;
        case NativeBindingKind::AxisFullPositive:name+=" (full +)";break;
        case NativeBindingKind::AxisFullNegative:name+=" (full -)";break;
        default:name+=" +";break;
    }
    return name;
}
const char* NativeInputActionName(std::size_t action) noexcept
{
    constexpr const char* names[]={"Reset car / select","Left stick","Nitro / right stick","Pause",
        "Menu up / HUD","Menu right","Menu down","Menu left","Shift down / special","Shift up / special",
        "Look back","Handbrake","Camera / back","Horn","Accelerate / confirm","Brake",
        "Steer left","Steer right","Confirm","Back"};
    return action<20?names[action]:"Unknown action";
}

void NativeCheatTextInput::Cancel() noexcept
{
    text_.fill(0);length_=0;capturing_=false;
}
NativeCheatTextResult NativeCheatTextInput::Character(std::uint32_t character,
    bool alt,bool control,bool focused,bool enabled,std::array<char,6>& command) noexcept
{
    if(!alt || control || !focused || !enabled) {
        Cancel();return NativeCheatTextResult::Unhandled;
    }
    const bool wasCapturing=capturing_;
    if(character<32 || character>126) {
        Cancel();return wasCapturing ? NativeCheatTextResult::Cancelled:NativeCheatTextResult::Unhandled;
    }
    const char value=character>='a' && character<='z' ?
        static_cast<char>(character-'a'+'A'):static_cast<char>(character);
    constexpr char prefix[]="HP2C";
    const bool matches=length_<4 ? value==prefix[length_]:value>='0' && value<='9';
    if(!matches) {
        Cancel();
        if(value=='H') {
            text_[0]='H';length_=1;capturing_=true;return NativeCheatTextResult::Pending;
        }
        return wasCapturing ? NativeCheatTextResult::Cancelled:NativeCheatTextResult::Unhandled;
    }
    text_[length_++]=value;capturing_=true;
    if(length_!=text_.size())return NativeCheatTextResult::Pending;
    command=text_;text_.fill(0);length_=0;
    // Keep keyboard capture until Alt is released. A still-held H/C key
    // must not become an unrelated menu press after the final digit.
    return NativeCheatTextResult::Complete;
}
bool NativeCheatTextInput::FilterKeyboard(std::array<bool,256>& keys,
    bool alt,bool control,bool focused,bool enabled) noexcept
{
    if(!alt || control || !focused || !enabled) {Cancel();return false;}
    if(!capturing_)return false;
    keys.fill(false);return true;
}

NativePadSample ConvertSdl2Gamepad(const Sdl2GamepadSnapshot& sample) noexcept
{
    NativePadSample output{};
    if(!sample.connected)return output;
    output.connected=true;
    // SDL2 GameController button indices (A,B,X,Y,Back,Guide,Start,L3,R3,
    // LB,RB,Up,Down,Left,Right) -> public XInput bit layout used by the
    // native logical button adapter. Game logic never depends on SDL ABI.
    constexpr std::uint16_t buttonBits[15]={
        0x1000,0x2000,0x4000,0x8000,0x0020,0,0x0010,0x0040,0x0080,
        0x0100,0x0200,0x0001,0x0002,0x0004,0x0008
    };
    for(std::size_t button=0;button<sample.buttons.size();++button)
        if(sample.buttons[button])output.buttons|=buttonBits[button];
    output.leftX=sample.axes[0];
    output.rightX=sample.axes[2];
    // SDL sticks are positive down, XInput and original scanner positive up.
    const auto upward=[](std::int16_t axis) noexcept {
        return static_cast<std::int16_t>(std::clamp(-static_cast<int>(axis),-32768,32767));
    };
    output.leftY=upward(sample.axes[1]);
    output.rightY=upward(sample.axes[3]);
    const auto trigger=[](std::int16_t axis) noexcept {
        return static_cast<std::uint8_t>(std::clamp(static_cast<int>(axis),0,32767)*255/32767);
    };
    output.leftTrigger=trigger(sample.axes[4]);
    output.rightTrigger=trigger(sample.axes[5]);
    output.rawValid=true;output.standardized=true;output.axisCount=6;output.buttonCount=15;
    std::copy(sample.axes.begin(),sample.axes.end(),output.rawAxes.begin());
    std::copy(sample.buttons.begin(),sample.buttons.end(),output.rawButtons.begin());
    return output;
}

void AssignNativeKeyboardAndGamepad(NativeInputSettings& settings,std::int32_t gamepad)
{
    if(gamepad<0 || gamepad>255)throw std::invalid_argument("Invalid controller selection for player two");
    auto assigned=settings;
    const auto key=settings.controller[0]==gamepad?settings.deviceKey[0]:
        settings.controller[1]==gamepad?settings.deviceKey[1]:std::string{};
    assigned.controller={{-1,gamepad}};
    assigned.deviceMode={{NativeDeviceMode::Keyboard,NativeDeviceMode::Gamepad}};
    assigned.deviceKey[0].clear();assigned.deviceKey[1]=key;
    ValidateNativeInputSettings(assigned);settings=assigned;
}

void AssignNativePlayerOneGamepad(NativeInputSettings& settings,std::int32_t gamepad)
{
    if(gamepad<0 || gamepad>255)throw std::invalid_argument("Invalid controller selection for player one");
    auto assigned=settings;
    if(assigned.controller[0]!=gamepad)assigned.deviceKey[0].clear();
    if(assigned.controller[1]==gamepad) {
        assigned.deviceKey[0]=assigned.deviceKey[1];assigned.deviceKey[1].clear();
        assigned.controller[1]=-1;assigned.deviceMode[1]=NativeDeviceMode::Disabled;
    }
    assigned.controller[0]=gamepad;assigned.deviceMode[0]=NativeDeviceMode::Gamepad;
    ValidateNativeInputSettings(assigned);settings=assigned;
}

ControllerState NativeInputState(const NativeInputSettings& settings,std::int32_t port,
    const NativePadSample& pad,const std::array<bool,256>& keys,bool focused)
{
    if(port<0 || port>=2)throw std::out_of_range("Native input player port");
    const auto player=static_cast<std::size_t>(port);
    const bool connected=NativeGamepadEnabled(settings,port) && pad.connected;
    const bool keyboard=NativeKeyboardEnabled(settings,port);
    ControllerState state;state.connected=connected || keyboard;
    if(!focused)return state;
    const auto& profile=settings.profiles[player];
    if(connected) {
        const auto raw=CanonicalRawSample(pad);
        const int threshold=profile.triggerThreshold<0?settings.triggerThreshold:profile.triggerThreshold;
        for(std::size_t i=0;i<profile.buttons.size();++i) {
            const auto& binding=profile.buttons[i];
            int pressure=NativeBindingPressure(binding,raw);
            const bool analog=binding.kind==NativeBindingKind::AxisPositive || binding.kind==NativeBindingKind::AxisNegative ||
                binding.kind==NativeBindingKind::AxisFullPositive || binding.kind==NativeBindingKind::AxisFullNegative;
            if(analog && pressure<=threshold)pressure=0;
            SetButton(state,static_cast<Ps2Button>(i),pressure>0,static_cast<std::uint8_t>(pressure));
        }
        for(std::size_t i=0;i<4;++i) {
            const auto source=settings.swapSticks?(i+2)%4:i;
            const auto axis=profile.axes[source];
            if(axis<0 || axis>=raw.axisCount || axis>=static_cast<int>(raw.rawAxes.size()))continue;
            int value=raw.rawAxes[static_cast<std::size_t>(axis)];
            const bool inverted=profile.invert[source] ^ (i==2 && settings.invertSteering) ^
                (i==1 && settings.invertLookY);
            if(inverted)value=std::clamp(-value,-32768,32767);
            const int zone=profile.deadzone[i]<0?(i<2?settings.rightDeadzone:settings.leftDeadzone):profile.deadzone[i];
            const int maximum=profile.saturation[i];
            // Preserve the configured deadzone response even when the
            // launcher makes an inherited value explicit on Save.
            if(value>-zone && value<zone)value=0;
            if(maximum!=32767) {
                const int magnitude=std::min(std::abs(value),maximum);
                const int scaled=magnitude*(value<0?32768:32767)/maximum;
                value=value<0?-scaled:scaled;
            }
            state.sticks[i]=NormalizeAxis(static_cast<std::int16_t>(value));
        }
    }
    if(keyboard) {
        const auto& bindings=NativeKeyboardBindings(settings,port);
        const auto held=[&](std::size_t index){const auto key=bindings[index];return key<keys.size() && key && keys[key];};
        for(unsigned i=0;i<16;++i)if(held(i))SetButton(state,static_cast<Ps2Button>(i),true);
        if(held(18))SetButton(state,Ps2Button::Cross,true);
        if(held(19))SetButton(state,Ps2Button::Triangle,true);
        const bool left=held(16),right=held(17);
        const bool inverted=settings.invertSteering ^ profile.invert[settings.swapSticks?0:2];
        if(left!=right)state.sticks[2]=(left!=inverted)?0:255;
    }
    return state;
}

namespace {
// Authored pressure-channel order: Right, Left, Up, Down, Triangle, Circle, Cross,
// Square, L1, R1, L2, R2. -1 means the button has no pressure data.
constexpr std::int8_t kPressure[16] = {
    -1,-1,-1,-1,2,0,3,1,10,11,8,9,4,5,6,7
};
}
void SetButton(ControllerState& state, Ps2Button button, bool pressed,
               std::uint8_t pressure) noexcept {
    const auto bit = static_cast<unsigned>(button);
    if (pressed) state.held |= static_cast<std::uint16_t>(1u << bit);
    else state.held &= static_cast<std::uint16_t>(~(1u << bit));
    if (kPressure[bit] >= 0)
        state.pressures[static_cast<size_t>(kPressure[bit])] = pressed ? pressure : 0;
}
std::uint8_t NormalizeAxis(std::int16_t s) noexcept {
    // Truncate with endpoints preserved, centre 128. SDL axes have -32768..32767.
    return s >= 0 ? static_cast<std::uint8_t>(128 + (static_cast<std::int32_t>(s) * 127) / 32767)
                  : static_cast<std::uint8_t>(128 + (static_cast<std::int32_t>(s) * 128) / 32768);
}
JoystickControls NativeScannerControls(const ControllerState& state) noexcept {
    JoystickControls controls{};
    if(!state.connected) {
        controls.mappingProfile=JOYSTICK_PROFILE_DISCONNECTED;
        std::fill_n(controls.sticks,4,std::uint8_t{255});
        std::fill_n(controls.pressures,12,std::uint8_t{255});
        return controls;
    }
    controls.held=state.held;
    std::copy(state.sticks.begin(),state.sticks.end(),controls.sticks);
    std::copy(state.pressures.begin(),state.pressures.end(),controls.pressures);
    controls.mappingProfile=JOYSTICK_PROFILE_GAMEPAD;
    return controls;
}
}

#include "indep/inc/CameraMover.hpp"
#include "indep/inc/eMath.hpp"
#include <cstring>
#include "bware/inc/bMath.hpp"


int32_t carCameraEnvelopeSegment(const KeyedCurve* curve, float coordinate)
{
    int32_t next = 0;
    if (curve->count > 0 && !(coordinate < curve->keys[0].coordinate)) {
        next = 1;
        while (next < curve->count && !(coordinate < curve->keys[next].coordinate)) ++next;
    }
    return next - (next > 0);
}

int32_t carCameraEnvelopeOutside(const KeyedCurve* curve, float coordinate)
{
    return coordinate < curve->keys[0].coordinate ||
        curve->keys[curve->count - 1].coordinate <= coordinate;
}

float carCameraEnvelopeSlope(const KeyedCurve* curve, int32_t segment)
{
    if (segment < 0 || curve->count - 2 < segment) return 0.0f;
    const float value = curve->keys[segment + 1].value - curve->keys[segment].value;
    const float span = curve->keys[segment + 1].coordinate - curve->keys[segment].coordinate;
    return finiteRatio(value, span);
}

float carCameraEnvelopeEvaluate(const KeyedCurve* curve, float coordinate)
{
    if (carCameraEnvelopeOutside(curve, coordinate)) return 0.0f;
    const int32_t segment = carCameraEnvelopeSegment(curve, coordinate);
    const float slope = carCameraEnvelopeSlope(curve, segment);
    const float distance = coordinate - curve->keys[segment].coordinate;
    return curve->keys[segment].value + slope * distance;
}

void carCameraShakeAdvanceChannel(CarCameraShakeChannel* channel, float elapsed)
{
    if (channel->envelope) {
        channel->phase += finiteRatio(elapsed, channel->duration);
        if (carCameraEnvelopeOutside(channel->envelope, channel->phase)) channel->envelope = NULL;
    }
}

void carCameraShakeSample(const CarCameraShakeChannel* channel, float output[4])
{
    float value = 0.0f;
    const int32_t cycle = convertFloatToInt32(channel->phase * channel->frequency);
    if (((cycle ^ 1) & 1) && channel->envelope)
        value = carCameraEnvelopeEvaluate(channel->envelope, channel->phase);
    for (int axis = 0; axis < 4; ++axis) output[axis] = channel->direction[axis] * value;
}

#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/CarDriver.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>

namespace {
// Final data and actual156958 constructor writes; no beta assets.
const CurveKey smallRumblePoints[]={{0.0f,255.0f},{0.05000000074505806f,200.0f},
    {0.07000000029802322f,80.0f},{0.800000011920929f,0.0f}};
const CurveKey binaryRumblePoints[]={{0.0f,255.0f},{0.15000000596046448f,255.0f}};
const KeyedCurve smallRumbleEnvelope{smallRumblePoints,4};
const KeyedCurve binaryRumbleEnvelope{binaryRumblePoints,2};
const float impactCoefficients[]={2.0f,11.0f,0.30000001192092896f,2.0f};
[[maybe_unused]] const char* rumbleDebugName{}; // Actual pRumbleDebugName, observer only.
bool RumbleEnabled(int32_t port,const hp2::ApplicationFrontendFlow& flow,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players)
{
    const auto slot=port>0?1u:0u;
    if (flow.currentState==3) {
        hp2::Require(frontend.players[slot],"Rumble has no actual FRED player profile");
        return frontend.players[slot]->settings.authoredOption!=0;
    }
    const auto* player=players.playersByNumber[slot];
    return player && player->settings.authoredOption!=0;
}
}
void joystickRumblerStart(GameInputRumbler& rumbler,const KeyedCurve& envelope,float amplitude)
{
    rumbler.envelope=&envelope;rumbler.amplitude=amplitude;rumbler.phase=0.0f;
}
bool joystickRumblerUpdate(GameInputRumbler& rumbler,float elapsed)
{
    rumbler.previousValue=rumbler.value;
    if (rumbler.envelope) {
        if (carCameraEnvelopeOutside(rumbler.envelope,rumbler.phase)) {
            rumbler.value=0;rumbler.envelope=nullptr;
        } else {
            const auto value=carCameraEnvelopeEvaluate(rumbler.envelope,rumbler.phase);
            rumbler.value=convertFloatToInt32(rumbler.amplitude*value);
            rumbler.phase+=finiteRatio(elapsed,rumbler.amplitude);
        }
    }
    if (rumbler.value<0)rumbler.value=0;
    if (rumbler.value>=256)rumbler.value=255;
    return rumbler.value!=rumbler.previousValue;
}
void joystickUpdatePortRumblers(GameInputRouter& input,const GameTime& clock)
{
    for (int32_t port=0;port<2;++port) {
        bool changed=false;
        for (auto& rumbler:input.rumblers[port])changed=joystickRumblerUpdate(rumbler,clock.realTimeElapsed)||changed;
        if (changed && joystickSupportsPressureMode(static_cast<uint8_t>(joystickGetType(&input,port))))
            joystickSetRumble(&input,port,static_cast<uint8_t>(input.rumblers[port][1].value>128),
                static_cast<uint8_t>(input.rumblers[port][0].value));
    }
}
void joystickPortRumble(GameInputRouter& input,int32_t port,float magnitude,
    const float* coefficients,const KeyedCurve& envelope,bool special,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players)
{
    hp2::Require(port>=0 && port<2 && coefficients,"Rumble has no actual controller port/coefficients");
    if (flow.inGameFlag) {
        hp2::Require(coordinator,"In-game rumble has no actual RaceCoordinator");
        if (coordinator->state!=6 && coordinator->state!=3)return;
    }
    const float minimum=coefficients[0],maximum=coefficients[1];
    if (magnitude<minimum)return;
    if (maximum<magnitude) {
        magnitude=maximum;
        if (special && RumbleEnabled(port,flow,frontend,players))
            joystickRumblerStart(input.rumblers[port][1],binaryRumbleEnvelope,1.0f);
    }
    const float amount=finiteRatio(magnitude-minimum,maximum-minimum)*(coefficients[3]-coefficients[2])+coefficients[2];
    auto& rumbler=input.rumblers[port][0];
    if (!special && rumbler.envelope && amount<rumbler.amplitude)return;
    if (RumbleEnabled(port,flow,frontend,players))joystickRumblerStart(rumbler,envelope,amount);
}
void joystickPortRumbleImpact(GameInputRouter& input,int32_t port,float magnitude,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players)
{
    rumbleDebugName="Impact";
    joystickPortRumble(input,port,magnitude,impactCoefficients,smallRumbleEnvelope,true,flow,coordinator,frontend,players);
}


namespace {
// PAL156958 publishes one mutable three-key suspension envelope for each
// actual port. ShockSway/RoadNoise change its first amplitude before PortRumble.
CurveKey surfaceRumblePoints[2][3]={
    {{0.0f,50.0f},{0.20000000298023224f,60.0f},{1.2000000476837158f,0.0f}},
    {{0.0f,50.0f},{0.20000000298023224f,60.0f},{1.2000000476837158f,0.0f}}};
const KeyedCurve surfaceRumbleEnvelopes[2]={{surfaceRumblePoints[0],3},{surfaceRumblePoints[1],3}};
const CurveKey blownTirePoints[]={{0.0f,255.0f},{0.15000000596046448f,255.0f},
    {0.20000000298023224f,120.0f},{1.2000000476837158f,0.0f}};
const KeyedCurve blownTireEnvelope{blownTirePoints,4};
const CurveKey singleBumpPoints[]={{0.0f,140.0f},{0.07000000029802322f,0.0f}};
const KeyedCurve singleBumpEnvelope{singleBumpPoints,2};
const CurveKey doubleBumpPoints[]={{0.0f,255.0f},{0.05000000074505806f,0.0f},
    {0.07000000029802322f,255.0f},{0.15000000596046448f,0.0f}};
const KeyedCurve doubleBumpEnvelope{doubleBumpPoints,4};
const float swayCoefficients[]={4.0f,30.0f,1.0f,1.5f,45.0f,200.0f};
const float roadCoefficients[]={0.0010000000474974513f,0.02500000037252903f,
    1.100000023841858f,1.5f,60.0f,200.0f};
const float bottomCoefficients[]={1.0f,3.0f,1.0f,2.0f};
const float bumpCoefficients[]={0.0f,1.0f,0.30000001192092896f,2.0f};
struct SurfaceRumbleContext {
    GameInputRouter& input;int32_t port;float magnitude;
    const hp2::ApplicationFrontendFlow& flow;
    const GameSession* coordinator;
    const hp2::FrontendPlayerManager& frontend;
    const CarFramePlayerRegistry& players;
};
void SurfaceRumble(const SurfaceRumbleContext& b,const float* coefficients,
    const KeyedCurve& envelope,bool special)
{
    joystickPortRumble(b.input,b.port,b.magnitude,coefficients,envelope,special,
        b.flow,b.coordinator,b.frontend,b.players);
}
// Complete original wrappers. Their final data, debug name and distinct
// envelope selections are retained; they share the existing actual rumblers.
void PortRumbleShockSway(const SurfaceRumbleContext& b)
{
    auto& point=surfaceRumblePoints[b.port][0];
    point.value=finiteRatio(b.magnitude-swayCoefficients[0],swayCoefficients[1]-swayCoefficients[0])*
        (swayCoefficients[5]-swayCoefficients[4])+swayCoefficients[4];
    rumbleDebugName="Shock Sway";
    SurfaceRumble(b,swayCoefficients,surfaceRumbleEnvelopes[b.port],false);
}
void PortRumbleRoadNoise(const SurfaceRumbleContext& b)
{
    auto& point=surfaceRumblePoints[b.port][0];
    point.value=finiteRatio(b.magnitude-roadCoefficients[0],roadCoefficients[1]-roadCoefficients[0])*
        (roadCoefficients[5]-roadCoefficients[4])+roadCoefficients[4];
    rumbleDebugName="Road Noise";
    SurfaceRumble(b,roadCoefficients,surfaceRumbleEnvelopes[b.port],false);
}
void PortRumbleShockBottom(const SurfaceRumbleContext& b)
{rumbleDebugName="Shock Bottom";SurfaceRumble(b,bottomCoefficients,smallRumbleEnvelope,true);}
void PortRumbleBlownTires(const SurfaceRumbleContext& b)
{rumbleDebugName="Blown Tires";SurfaceRumble(b,impactCoefficients,blownTireEnvelope,true);}
void PortRumbleSingleRoadBump(const SurfaceRumbleContext& b)
{rumbleDebugName="Single Road Bump";SurfaceRumble(b,bumpCoefficients,singleBumpEnvelope,true);}
void PortRumbleDoubleRoadBump(const SurfaceRumbleContext& b)
{rumbleDebugName="Double Road Bump";SurfaceRumble(b,bumpCoefficients,doubleBumpEnvelope,true);}
// PAL1562B0, tShaker::GetAmplitude. This samples the live envelope, not
// the signed direction vector or a manufactured hardware feedback result.
float CameraShakeAmplitude(const CarCameraShakeChannel& channel)
{return channel.envelope?carCameraEnvelopeEvaluate(channel.envelope,channel.phase):0.0f;}
}

void joystickPortRumbleSurface(GameInputRouter& input,int32_t port,int32_t code,float magnitude,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players)
{
    hp2::Require(port>=0 && port<2,"Surface rumble has no actual controller port");
    const SurfaceRumbleContext context{input,port,magnitude,flow,coordinator,frontend,players};
    switch(code) {
    case 0:PortRumbleShockSway(context);break;
    case 1:PortRumbleRoadNoise(context);break;
    case 2:PortRumbleShockBottom(context);break;
    case 3:PortRumbleBlownTires(context);break;
    case 4:PortRumbleSingleRoadBump(context);break;
    case 5:PortRumbleDoubleRoadBump(context);break;
    default:throw std::runtime_error("Unknown original surface rumble operation");
    }
}

// PAL1566C0: MaybeCameraShake(int,bVector3*). The released constructor
// writes acceleration weights {0.7,0.5,2}; data2B13F0 supplies all thresholds.
void carCameraMaybeShake(CarCameraShake& shake,const CarFramePlayerRegistry& players,
    int32_t port,const float acceleration[4])
{
    hp2::Require(port>=0 && port<2 && acceleration && players.playersByNumber[port],
        "Acceleration camera shake has no actual player/view owner");
    const auto* player=players.playersByNumber[port];
    if(carFramePlayerReplayActive(player))return;
    const float weighted[4]={acceleration[0]*0.699999988079071f,
        acceleration[1]*0.5f,acceleration[2]*2.0f,0.0f};
    float magnitude=std::sqrt((weighted[0]*weighted[0]+weighted[1]*weighted[1])+weighted[2]*weighted[2]);
    if(magnitude<250.0f)return;
    if(600.0f<magnitude)magnitude=600.0f;
    const float amount=finiteRatio(magnitude-250.0f,600.0f-250.0f);
    float amplitude=amount*(0.1599999964237213f-0.07999999821186066f)+0.07999999821186066f;
    auto& channel=shake.channels[port];
    if(!(CameraShakeAmplitude(channel)<amplitude))return;
    float duration=amount*(0.699999988079071f-0.30000001192092896f)+0.30000001192092896f;
    if(player->movement.steeringFeedbackBusy==7) {
        amplitude*=0.800000011920929f;duration*=1.2000000476837158f;
    }
    float direction[4]{};vectorSetLength3(direction,weighted,amplitude);
    carCameraShakeStart(&channel,&shake.envelope,direction,duration,17.0f);
}

/* Camera path support. */
void carCameraShakeReset(CarCameraShake* shake)
{
    for (int index = 0; index < 2; ++index) {
        CarCameraShakeChannel* channel = &shake->channels[index];
        memset(channel->direction, 0, sizeof(channel->direction));
        channel->envelope = NULL;
        channel->phase = 0.0f;
        channel->duration = 1.0f;
        channel->frequency = 17.0f;
    }
}

void carCameraShakeInitialize(CarCameraShake* shake)
{
    /* The authored envelope is initialized together with the camera/input
     * service. A channel keeps a reference to this one curve after Start. */
    static const CurveKey keys[5] = {
        {0.0f, 1.0f}, {0.10000000149011612f, 0.699999988079071f},
        {0.30000001192092896f, 0.5f}, {0.5f, 0.4000000059604645f}, {1.0f, 0.0f}
    };
    memset(shake, 0, sizeof(*shake));
    memcpy(shake->envelopeKeys, keys, sizeof(keys));
    shake->envelope.keys = shake->envelopeKeys;
    shake->envelope.count = 5;
    shake->rotationScale[0] = -1200.0f;
    shake->rotationScale[1] = 2000.0f;
    shake->testDirection[2] = 0.30000001192092896f;
    shake->testDuration = 0.6000000238418579f;
    carCameraShakeReset(shake);
}

void carCameraShakeStart(CarCameraShakeChannel* channel, const KeyedCurve* envelope,
    const float direction[4], float duration, float frequency)
{
    for (int axis = 0; axis < 4; ++axis) channel->direction[axis] = direction[axis] * -1.0f;
    channel->frequency = frequency;
    channel->envelope = envelope;
    channel->duration = duration;
    channel->phase = 0.0f;
}

void carCameraShakeAdvance(CarCameraShake* shake, float elapsed)
{
    if (shake->testRequested) {
        carCameraShakeStart(&shake->channels[0], &shake->envelope,
            shake->testDirection, shake->testDuration, 17.0f);
        shake->testRequested = 0;
    }
    for (int index = 0; index < 2; ++index) carCameraShakeAdvanceChannel(&shake->channels[index], elapsed);
}

void carCameraShakeSampleScreen(const CarCameraShake* shake, int32_t channel, float output[4])
{
    carCameraShakeSample(&shake->channels[channel], output);
}

void carCameraShakeApply(const CarCameraShake* shake, const World* world,
    int32_t viewNumber, float matrix[16])
{
    if (world->timing.timeScale < 1.0f) return;
    int32_t channel;
    if (viewNumber == 1 || viewNumber == 3) channel = 0;
    else if (viewNumber == 2) channel = 1;
    else return;
    float direction[4], local[4], rotation[16];
    carCameraShakeSampleScreen(shake, channel, direction);
    direction[3] = 0.0f;
    vectorMatrixTransform(local, matrix, direction);
    rotation[0] = rotation[5] = rotation[10] = rotation[15] = 1.0f;
    rotation[1] = rotation[2] = rotation[3] = rotation[4] = rotation[6] = rotation[7] =
        rotation[8] = rotation[9] = rotation[11] = 0.0f;
    local[3] = 1.0f;
    for (int axis = 0; axis < 4; ++axis) rotation[12 + axis] = local[axis];
    const uint16_t x = (uint16_t)convertFloatToInt32(shake->rotationScale[0] * (local[2] + local[1]));
    motionMatrixRotateX(rotation, rotation, x);
    const uint16_t y = (uint16_t)convertFloatToInt32(shake->rotationScale[1] * local[0]);
    motionMatrixRotateY(rotation, rotation, y);
    const uint16_t z = (uint16_t)convertFloatToInt32(shake->rotationScale[2] * (local[0] + local[1]));
    motionMatrixRotateYaw(rotation, rotation, z);
    vectorMatrixMultiply(matrix, rotation, matrix);
}

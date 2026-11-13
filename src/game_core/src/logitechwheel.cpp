#include "indep/inc/CarDriver.hpp"

// Final12C160/12C240. The source functions select one of two retained wheel
// records and clear its requested force words. They do not execute the
// platform SDK or acknowledge that a physical wheel has consumed the change.
int32_t joystickGetWheelId(const GameInputRouter* router,int32_t port)
{
    if (!router)return 2;
    for (int32_t index=0;index<2;++index)
        if (router->forceWheels[index].port==port)return index;
    return 2;
}
void joystickPortStopForce(GameInputRouter* router,int32_t port)
{
    const auto wheel=joystickGetWheelId(router,port);
    if (wheel==2)return;
    router->forceWheels[wheel].forceWord6C=0;
    router->forceWheels[wheel].forceWord70=0;
}

#include "indep/inc/Player.hpp"
#include <cmath>

GameInputForceDispatchResult joystickDispatchForceFeedback(const GameInputRouter* router,
    const CarFramePresentationEvent* request)
{
    if (!router || !request || request->kind != CAR_FRAME_FORCE_FEEDBACK ||
        request->port < 0 || request->port >= 2)
        return GAME_INPUT_FORCE_INVALID_REQUEST;
    // These are the four typed calls emitted by Car::UpdateForceFeedback.
    // Validate the request before letting an unrelated presentation event
    // disappear through an absent-hardware branch.
    int32_t count = 0;
    switch (request->code) {
    case CAR_FRAME_FORCE_PAIR_PARAMETERS:
        if (request->argument != 0 && request->argument != 1)
            return GAME_INPUT_FORCE_INVALID_REQUEST;
        count = 6;
        break;
    case CAR_FRAME_FORCE_PERIODIC_PARAMETERS:
        if (request->argument != 3 && request->argument != 4)
            return GAME_INPUT_FORCE_INVALID_REQUEST;
        count = 9;
        break;
    case CAR_FRAME_FORCE_IMPACT:
    case CAR_FRAME_FORCE_GAIN:
        if (request->argument != 0) return GAME_INPUT_FORCE_INVALID_REQUEST;
        count = 1;
        break;
    default:
        return GAME_INPUT_FORCE_INVALID_REQUEST;
    }
    if (request->parameterCount != count) return GAME_INPUT_FORCE_INVALID_REQUEST;
    for (int32_t index = 0; index < count; ++index)
        if (!std::isfinite(request->forceParameters[index]))
            return GAME_INPUT_FORCE_INVALID_REQUEST;

    // Final PAL12C160 searches the two port records and returns2 when no
    // wheel is attached. Condition12C288, periodic12C3E8, shake12C7C0 and
    // gain12C828 all return on2 before changing any force/effect word.
    // The native XInput/keyboard provider registers no steering-force wheel.
    // Its ordinary pad rumble remains the separate joystickSetRumble path.
    if (joystickGetWheelId(router, request->port) == 2)
        return GAME_INPUT_FORCE_NO_DEVICE;

    // There is currently no native steering-force endpoint. A future caller
    // must provide the real device/effect transport before registering one;
    // retaining this request as an error avoids claiming physical delivery.
    return GAME_INPUT_FORCE_DEVICE_UNSUPPORTED;
}


GameInputForceDispatchResult joystickPortCollisionForce(const GameInputRouter* router,
    int32_t port,int32_t duration,float strength,float direction)
{
    if(!router || port<0 || port>=2 || duration<0 ||
       !std::isfinite(strength) || !std::isfinite(direction))
        return GAME_INPUT_FORCE_INVALID_REQUEST;
    // Full PAL12C728..12C7C0 calls GetWheelId then returns on2, before
    // duration+D4, strength+DC, direction+EC or pending+434 is written.
    if(joystickGetWheelId(router,port)==2)return GAME_INPUT_FORCE_NO_DEVICE;
    // Keyboard/XInput registers no steering-force endpoint. Preserve the
    // explicit unsupported-device failure if one is registered without a
    // real transport; pad rumble is the independent existing motor path.
    return GAME_INPUT_FORCE_DEVICE_UNSUPPORTED;
}

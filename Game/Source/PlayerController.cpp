#include "PlayerController.h"

#include "GamepadInput.h"

#include <raylib.h>

#include <cmath>

CarInput PlayerController::Poll(float deltaTime)
{
    (void)deltaTime; // neither device needs timers of its own
    CarInput input;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        input.throttle += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        input.throttle -= 1.0f;

    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        input.steer += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        input.steer -= 1.0f;

    input.boost = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    input.jump = IsKeyDown(KEY_SPACE);

    // Airborne, the same WASD keys become pitch and yaw, with roll on Q/E.
    // Pitch is deliberately inverted against the throttle: W drives forwards on
    // the ground and puts the nose down in the air, as in Rocket League.
    input.airPitch = -input.throttle;
    input.airYaw = input.steer;
    if (IsKeyDown(KEY_E))
        input.airRoll += 1.0f;
    if (IsKeyDown(KEY_Q))
        input.airRoll -= 1.0f;

    input.reset = IsKeyPressed(KEY_R);

    // The pad is merged on top rather than chosen between: whichever source is
    // pushing hardest wins each analogue field and the booleans are simply or-ed,
    // so both stay live at once and neither needs a mode. With no pad connected
    // every value below is zero and this loop changes nothing at all.
    if (gamepad::Available())
    {
        if (fabsf(gamepad::Throttle()) > fabsf(input.throttle))
            input.throttle = gamepad::Throttle();
        if (fabsf(gamepad::Steer()) > fabsf(input.steer))
            input.steer = gamepad::Steer();

        input.boost = input.boost || gamepad::Boost();
        input.jump = input.jump || gamepad::Jump();
        input.reset = input.reset || gamepad::ResetCar();

        // Air control comes off the left stick, not off the throttle: on a pad
        // the throttle is a trigger, and a trigger cannot pitch the nose up.
        if (fabsf(gamepad::AirPitch()) > fabsf(input.airPitch))
            input.airPitch = gamepad::AirPitch();
        if (fabsf(gamepad::AirYaw()) > fabsf(input.airYaw))
            input.airYaw = gamepad::AirYaw();
        if (fabsf(gamepad::AirRoll()) > fabsf(input.airRoll))
            input.airRoll = gamepad::AirRoll();
    }

    return input;
}

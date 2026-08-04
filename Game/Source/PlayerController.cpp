#include "PlayerController.h"

#include <raylib.h>

CarInput PlayerController::Poll(float deltaTime)
{
    (void)deltaTime; // the keyboard needs no timers of its own
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
    return input;
}

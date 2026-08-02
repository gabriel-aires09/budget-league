#include "PlayerController.h"

#include <raylib.h>

CarInput PlayerController::Poll()
{
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
    input.reset = IsKeyPressed(KEY_R);
    return input;
}

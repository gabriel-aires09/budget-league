#include "BotController.h"

#include "GameObjects/BallObject.h"
#include "GameObjects/CarObject.h"

#include <cmath>

#include <raymath.h>

CarInput BotController::Poll(float deltaTime)
{
    CarInput input;
    if (car == nullptr || ball == nullptr)
        return input;

    Vector3 carPosition = car->GetBodyPosition();
    Vector3 ballPosition = ball->GetBodyPosition();
    Vector3 ballVelocity = ball->GetBodyVelocity();

    // Where the ball will be shortly, not where it is: chasing the current
    // position alone leaves the bot permanently trailing a moving ball.
    float aimX = ballPosition.x + ballVelocity.x * leadTime;
    float aimZ = ballPosition.z + ballVelocity.z * leadTime;

    // The approach point sits behind the ball as seen from the goal it is
    // attacking, so simply driving through it is also the shot. This is the whole
    // of the bot's aiming - there is no separate decision about where to hit.
    float goalX = -aimX;
    float goalZ = targetGoalZ - aimZ;
    float goalDistance = sqrtf(goalX * goalX + goalZ * goalZ);
    if (goalDistance > 0.001f)
    {
        goalX /= goalDistance;
        goalZ /= goalDistance;
    }
    // Clamped to the flat floor. Unclamped, a ball rolling towards the bot's own
    // net puts the approach point inside the goal recess, and the bot follows it
    // in and wedges itself in the mouth - measured, and it never got out again.
    float targetX = Clamp(aimX - goalX * approachOffset, -fieldHalfWidth, fieldHalfWidth);
    float targetZ = Clamp(aimZ - goalZ * approachOffset, -fieldHalfLength, fieldHalfLength);

    // The two goal lines are opposite ends of the same axis, so the one it
    // defends is simply the far side of the one it attacks.
    const float defendZ = -targetGoalZ;
    const float towardField = defendZ > 0.0f ? -1.0f : 1.0f;
    const bool ballInOwnHalf = defendZ > 0.0f ? aimZ > 0.0f : aimZ < 0.0f;
    // Goal side: nearer its own line than the ball is. Measured against the aim
    // point rather than the ball, so a ball already rolling that way counts.
    const bool goalSide = fabsf(carPosition.z - defendZ) <= fabsf(aimZ - defendZ);
    const float ballRange = sqrtf((aimX - carPosition.x) * (aimX - carPosition.x) +
                                  (aimZ - carPosition.z) * (aimZ - carPosition.z));
    if (ballInOwnHalf && !goalSide && ballRange > recoverMinRange)
    {
        // Beaten in its own half: get between the ball and the net instead of
        // trailing the ball into it. It picks the attack back up the moment it is
        // goal-side again, so this is a recovery, not a mode it sits in.
        targetX = Clamp(aimX * recoverLineUp, -fieldHalfWidth, fieldHalfWidth);
        targetZ = Clamp(defendZ + towardField * defendStandOff, -fieldHalfLength, fieldHalfLength);
    }

    // Signed heading error in the XZ plane. Positive yaw turns left, and steering
    // negates it, so a positive error is exactly a positive steer.
    Vector3 forward = Vector3Transform(Vector3{ 0.0f, 0.0f, -1.0f }, car->GetBodyRotation());
    float toTargetX = targetX - carPosition.x;
    float toTargetZ = targetZ - carPosition.z;
    float error = atan2f(forward.x * toTargetZ - forward.z * toTargetX,
                         forward.x * toTargetX + forward.z * toTargetZ);
    float distance = sqrtf(toTargetX * toTargetX + toTargetZ * toTargetZ);

    // The jam timer runs through the unstick attempts as well, so it measures how
    // long the bot has genuinely been going nowhere.
    bool slow = fabsf(car->GetForwardSpeed()) < stuckSpeed;
    jamTimer = slow ? jamTimer + deltaTime : 0.0f;
    if (jamTimer >= stuckResetTime)
    {
        // Backing out has not worked for five seconds, so it is wedged rather
        // than merely blocked. Take the reset the car already has.
        jamTimer = 0.0f;
        stuckTimer = 0.0f;
        unstickRemaining = 0.0f;
        input.reset = true;
        return input;
    }

    // A stationary car cannot steer at all - steering authority ramps in with
    // speed - so the way out of a wall is to reverse, which gives it both the
    // speed to turn and somewhere to turn into.
    if (unstickRemaining > 0.0f)
    {
        unstickRemaining -= deltaTime;
        input.throttle = -1.0f;
        input.steer = unstickSteer;
        return input;
    }

    if (slow)
    {
        stuckTimer += deltaTime;
        if (stuckTimer >= stuckTime)
        {
            stuckTimer = 0.0f;
            unstickRemaining = unstickTime;
            // Reversing mirrors the steering, so this is the sign that swings the
            // nose towards the target rather than further into the wall.
            unstickSteer = error >= 0.0f ? -1.0f : 1.0f;
        }
    }
    else
    {
        stuckTimer = 0.0f;
    }

    input.steer = Clamp(error * steerGain, -1.0f, 1.0f);
    if (fabsf(error) > reverseAngle)
    {
        // The target is behind it: back up and swing round, rather than dragging
        // a wide turn all the way through.
        input.throttle = -1.0f;
        input.steer = -input.steer;
    }
    else
    {
        input.throttle = 1.0f;
        input.boost = car->grounded && car->boostAmount > boostReserve &&
                      distance > boostMinDistance && fabsf(error) < boostMaxAngle;
    }

    return input;
}

#ifndef BOTCONTROLLER_H
#define BOTCONTROLLER_H

#include "CarController.h"

class BallObject;
class CarObject;

// The AI opponent. It drives at a point just behind the ball, on the far side
// from the goal it is attacking, so running through the ball sends it roughly
// the right way. It boosts down the long approaches and backs itself out when
// it stops making progress.
//
// It has to be beatable: it never jumps, never goes for an aerial, reads the
// ball only `leadTime` ahead and keeps a reserve of boost. See the notes in
// HANDOFF.md before making it sharper.
class BotController final : public CarController
{
public:
    virtual CarInput Poll(float deltaTime) override;

    CarObject *car = nullptr;
    BallObject *ball = nullptr;
    // Goal line it is attacking. Z is the goal-to-goal axis, so the goal it
    // shoots at is a point on that axis.
    float targetGoalZ = 40.0f;
    // The flat part of the floor. Everything it drives at is clamped into this,
    // so it never chases the ball into a goal recess or up a ramp. Set from
    // ArenaObject::FlatHalfWidth/FlatHalfLength.
    float fieldHalfWidth = 22.5f;
    float fieldHalfLength = 35.0f;

    // --- Aim
    float leadTime = 0.25f;      // how far ahead of a moving ball it aims
    float approachOffset = 3.0f; // how far behind the ball the approach point sits
    float steerGain = 2.2f;      // steering per radian of heading error
    // Past this much heading error it reverses and swings the nose round instead
    // of trying to drag a wide turn through it.
    float reverseAngle = 2.0f;

    // --- Positioning
    // When the ball is in its own half and the ball is already past it, chasing
    // from behind is exactly the over-commitment CLAUDE.md 6.4 names: it cannot
    // catch the ball before the ball reaches the goal, and it leaves the net
    // open while it tries. It recovers goal-side first instead.
    //
    // It is also what breaks the midfield shoving match. Two cars pushing the
    // same ball from opposite sides move it nowhere - measured, the ball was
    // under 3 m/s for 35% of open play and the bot managed three shots on target
    // in over three minutes. The car that is out of position backing off is what
    // gives the other one a clean run at it.
    float defendStandOff = 12.0f; // how far in front of its own line it recovers to
    // How much of the ball's sideways position it lines up with while recovering.
    // Fully tracking the ball would put it on the ball rather than between the
    // ball and the goal.
    float recoverLineUp = 0.5f;
    // Inside this it plays the ball whatever the geometry says: it is close
    // enough to contest, and turning away would be the worse mistake. Measured
    // against a fixed opponent, 8 m beat both 0 (never contest) and 14 m, which
    // gave up on balls it could have reached and produced a 5.11 s stall.
    float recoverMinRange = 8.0f;

    // --- Boost
    float boostMinDistance = 14.0f; // only worth it on a long run
    float boostMaxAngle = 0.25f;    // radians of heading error still worth boosting through
    float boostReserve = 25.0f;     // never spends the tank below this

    // --- Stuck recovery
    float stuckSpeed = 2.0f; // m/s under which it is making no progress
    float stuckTime = 1.0f;  // for this long before it does anything about it
    float unstickTime = 0.8f;
    // Backing out does not free a car wedged in the goal mouth, so a jam this
    // long is escalated to a reset back to its own spawn.
    float stuckResetTime = 5.0f;

    float stuckTimer = 0.0f;
    float jamTimer = 0.0f;
    float unstickRemaining = 0.0f;
    float unstickSteer = 1.0f;
};

#endif

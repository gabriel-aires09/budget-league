#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

// What a CarObject consumes each physics step. Jump, boost and air control are
// added by the milestones that implement them.
struct CarInput
{
    float throttle = 0.0f; // -1 reverse .. 1 forward
    float steer = 0.0f;    // -1 left .. 1 right
    bool boost = false;    // held, not tapped

    // Held, not an edge. CarObject finds the rising edge itself, because Update
    // runs once per fixed step while IsKeyPressed is true for a whole frame —
    // at 120 Hz that would fire a jump twice and eat the double jump instantly.
    bool jump = false;

    // Only used while airborne. Sign conventions: nose up, nose right, roll right.
    float airPitch = 0.0f;
    float airYaw = 0.0f;
    float airRoll = 0.0f;

    bool reset = false;
};

// Input source of a CarObject: keyboard for the player, AI for the bot.
//
// Poll runs once per fixed physics step, and is given that step so a controller
// can hold timers of its own - the bot's stuck detection is one. The player
// controller ignores it.
class CarController
{
public:
    virtual ~CarController() {}
    virtual CarInput Poll(float deltaTime) = 0;
};

#endif

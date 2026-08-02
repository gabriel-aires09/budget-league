#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

// What a CarObject consumes each physics step. Jump, boost and air control are
// added by the milestones that implement them.
struct CarInput
{
    float throttle = 0.0f; // -1 reverse .. 1 forward
    float steer = 0.0f;    // -1 left .. 1 right
    bool boost = false;    // held, not tapped
    bool reset = false;
};

// Input source of a CarObject: keyboard for the player, AI for the bot.
class CarController
{
public:
    virtual ~CarController() {}
    virtual CarInput Poll() = 0;
};

#endif

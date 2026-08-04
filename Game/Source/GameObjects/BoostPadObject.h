#ifndef BOOSTPADOBJECT_H
#define BOOSTPADOBJECT_H

#include "GameObject.h"
#include "CarObject.h"

#include <vector>

// A boost pad: refills a car that drives over it, then goes dark until its
// cooldown expires.
//
// It has no physics body at all. The pad has to be flush with the floor because
// the car is a box with no wheels and would stop dead against any raised lip, so
// a collider would be either useless or harmful. Pickup is a distance check,
// the same approach GoalObject uses for the goal line.
class BoostPadObject final : public GameObject
{
public:
    virtual void Initialize(Scene &owner) override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    bool IsReady() const { return cooldownRemaining <= 0.0f; }

    Vector3 position = {};   // on the floor
    float radius = 1.8f;     // pickup radius, measured in the XZ plane
    float refillAmount = 12.0f;
    float cooldownTime = 4.0f;
    // A full-refill pad, as opposed to one of the small ones. It is what the two
    // kinds are, and it is also how the tuning panel knows which pads a refill
    // slider applies to once the amounts have been edited away from the defaults.
    bool fullRefill = false;
    // A car has to be near the floor to collect: flying over a pad must not take it.
    float pickupHeight = 2.0f;

    float cooldownRemaining = 0.0f;
    std::vector<CarObject *> cars;
};

#endif

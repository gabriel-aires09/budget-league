#include "BoostPadObject.h"

#include "Scene.h"
#include "UserInterface.h"

#include <cmath>

void BoostPadObject::Initialize(Scene &owner)
{
    scene = &owner;
    // No body: bodyID stays invalid, which MatchScene::Shutdown already skips.
}

void BoostPadObject::Update(float deltaTime)
{
    if (cooldownRemaining > 0.0f)
    {
        cooldownRemaining = fmaxf(cooldownRemaining - deltaTime, 0.0f);
        return;
    }

    for (CarObject *car : cars)
    {
        Vector3 carPosition = car->GetBodyPosition();
        float dx = carPosition.x - position.x;
        float dz = carPosition.z - position.z;
        if (dx * dx + dz * dz > radius * radius)
            continue;
        if (carPosition.y - position.y > pickupHeight)
            continue;

        // Taken even when the car is already full, as in Rocket League: wasting a
        // pad is part of managing boost.
        car->boostAmount = fminf(car->boostAmount + refillAmount, car->boostCapacity);
        cooldownRemaining = cooldownTime;
        break;
    }
}

void BoostPadObject::Draw()
{
    // Drawn with the immediate mode path, which is unlit, so a ready pad reads as
    // a light on the floor instead of a painted disc.
    //
    // The light *fills back in* as the pad recharges. Ready and not-ready were
    // already two different colours, but every pad on cooldown looked the same as
    // every other one: a pad taken a moment ago and a pad about to come back were
    // the same flat dark disc, so there was nothing to plan a run around. The
    // grown disc is the whole of that information, and it costs one extra call.
    const bool ready = IsReady();
    const float charge = ready || cooldownTime <= 0.0f
                             ? 1.0f
                             : 1.0f - cooldownRemaining / cooldownTime;

    Vector3 base = { position.x, position.y + 0.02f, position.z };
    const float seat = radius * 0.62f;

    // The dark seat is always drawn, so a pad never vanishes off the floor and
    // the lit part always has something to grow against.
    DrawCylinder(base, seat, seat, 0.03f, 16, Color{ 58, 56, 46, 255 });

    // A ready pad breathes, which is what separates "full" from "nearly full" at
    // a glance without another colour.
    const float pulse = ready ? 0.90f + 0.10f * sinf((float)GetTime() * 3.4f) : 1.0f;
    const float lit = seat * charge * pulse;
    if (lit > 0.01f)
    {
        Vector3 above = { base.x, base.y + 0.005f, base.z };
        DrawCylinder(above, lit, lit, 0.03f, 16, uistyle::Boost);
    }

    const Color ring = ready ? Color{ 255, 236, 170, 255 } : Color{ 82, 78, 64, 255 };
    DrawCylinderWires(base, radius, radius, 0.04f, 16, ring);
}

BoundingBox BoostPadObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ position.x - radius, position.y, position.z - radius },
        Vector3{ position.x + radius, position.y + 0.1f, position.z + radius }
    };
}

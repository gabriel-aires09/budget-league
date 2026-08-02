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
    const bool ready = IsReady();
    const Color fill = ready ? uistyle::Boost : Color{ 58, 56, 46, 255 };
    const Color ring = ready ? Color{ 255, 236, 170, 255 } : Color{ 82, 78, 64, 255 };

    Vector3 base = { position.x, position.y + 0.02f, position.z };
    DrawCylinder(base, radius * 0.62f, radius * 0.62f, 0.03f, 16, fill);
    DrawCylinderWires(base, radius, radius, 0.04f, 16, ring);
}

BoundingBox BoostPadObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ position.x - radius, position.y, position.z - radius },
        Vector3{ position.x + radius, position.y + 0.1f, position.z + radius }
    };
}

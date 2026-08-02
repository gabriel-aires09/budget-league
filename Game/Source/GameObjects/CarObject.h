#ifndef CAROBJECT_H
#define CAROBJECT_H

#include "GameObject.h"
#include "CarController.h"
#include "StaticModelAsset.h"

#include <string>

// Rocket car: a single dynamic box driven by arcade forces plus a ground probe.
// Deliberately not a Jolt VehicleConstraint (see CLAUDE.md 2.5).
//
// Every field below is a handling tunable; Milestone 12 binds them to the ImGui
// panel. Call ApplyTuning() after changing the body ones at runtime.
class CarObject final : public GameObject
{
public:
    virtual ~CarObject();

    virtual void Initialize(Scene &owner) override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    // Pushes the body tunables (damping, friction, restitution) into Jolt.
    void ApplyTuning();
    void ResetTo(Vector3 position, float yawDegrees);
    float GetForwardSpeed() const;

    CarController *controller = nullptr;

    // --- Visual model (cosmetic only, the physics body stays a box)
    // Any cooked car works here: Cop, NormalCar1, NormalCar2, SportsCar,
    // SportsCar2, SUV, Taxi. Set both before Initialize().
    std::string modelName = "SportsCar";
    Color teamColor = { 60, 140, 255, 255 };
    // The pack models face +Z and the car drives towards -Z.
    float modelYawDegrees = 180.0f;

    // Spawn transform, also used by the reset.
    Vector3 spawnPosition = { 0.0f, 0.45f, 15.0f };
    float spawnYawDegrees = 0.0f; // facing the middle of the field (-Z)

    // --- Shape and mass
    Vector3 halfExtents = { 0.85f, 0.35f, 1.6f };
    float mass = 180.0f;
    // Rounds the box edges, which softens glancing contacts with arena geometry.
    // Note it does not let the car climb steps: a box has no wheels, so a lip
    // taller than a few centimetres stops it dead. Keep the arena floor smooth.
    float convexRadius = 0.05f;
    // The centre of mass sits below the box centre, which is what keeps the car
    // from tipping over on small bumps.
    float centerOfMassOffsetY = -0.28f;

    // --- Drive
    float engineForce = 4600.0f;
    float brakeForce = 7000.0f;
    float maxSpeed = 32.0f;
    float maxReverseSpeed = 14.0f;
    float coastDrag = 0.6f;

    // --- Steering
    float steerRate = 3.0f;            // rad/s of yaw at full steering, low speed
    float steerSpeedFloor = 5.0f;      // m/s at which steering reaches its full rate
    float highSpeedSteerScale = 0.45f; // steering authority left at top speed
    float grip = 12.0f;                // sideways speed bled off, per second

    // --- Stability
    float linearDamping = 0.1f;
    float angularDamping = 2.2f;
    float bodyFriction = 0.55f;
    float bodyRestitution = 0.05f;
    float groundProbe = 0.35f;     // how far below the box counts as driving
    float recoveryProbe = 1.30f;   // how far below the box the righting assist still works
    float uprightTorque = 6500.0f; // rolls the car back onto its wheels
    float tumbleDamping = 4.0f;    // bleeds off bump-induced pitch and roll, per second
    float driveUprightMin = 0.35f; // below this the car rights itself instead of driving

    bool grounded = false;
    float uprightness = 1.0f; // 1 on its wheels, -1 on its roof

    StaticModelAsset carModel;
    // Fitted in Initialize() so the model always matches the collision box.
    float modelScale = 1.0f;
    float modelOffsetY = 0.0f;

    // Stand-in drawn only when the cooked model is missing.
    Model colliderModel = {};
    bool colliderModelLoaded = false;
};

#endif

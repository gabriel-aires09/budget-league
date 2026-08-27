#ifndef CAROBJECT_H
#define CAROBJECT_H

#include "GameObject.h"
#include "CarController.h"
#include "StaticModelAsset.h"

#include <cmath>
#include <string>

// Rocket car: a single dynamic box driven by arcade forces plus a ground probe.
// Deliberately not a Jolt VehicleConstraint (see CLAUDE.md 2.5).
//
// Every field below is a handling tunable, and most are on the F1 tuning panel
// (TuningPanel.cpp) in Debug and Development. Call ApplyTuning() after changing
// the body ones at runtime.
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

    // --- Boost
    // Held, and it works in the air as well as on the ground, so it stays useful
    // once Milestone 08 adds aerials.
    float boostCapacity = 100.0f;
    float boostAmount = 100.0f;    // 0 .. boostCapacity
    float boostDrainRate = 33.0f;  // per second, so a full tank lasts about 3 s
    // On top of the engine force. At 9000 the push from cruising to top speed
    // took 0.36 s, which read as a teleport; at 4000 the car never reached the
    // cap before running out of field.
    float boostForce = 6000.0f;
    float boostMaxSpeed = 46.0f;   // boost drives past the normal top speed
    bool boosting = false;         // for the HUD, and the flame in Milestone 13
    // How long boost has been held without a break. The flame and the ember
    // trail ramp in over boostRampTime, so a tap does not look like a sustained
    // burn — which is what CLAUDE.md 6.3 asks for and what the flame did not do:
    // it was drawn at one fixed size the whole time.
    float boostHeldTime = 0.0f;
    float boostRampTime = 0.45f;
    float BoostIntensity() const
    {
        return boostRampTime > 0.0f ? fminf(boostHeldTime / boostRampTime, 1.0f) : 1.0f;
    }

    // --- Jump, flip and air control
    float jumpImpulse = 1000.0f;       // N s along the car's own up axis
    float secondJumpImpulse = 900.0f;
    float flipImpulse = 1500.0f;       // horizontal, towards whatever is held
    float flipSpin = 9.0f;             // rad/s of the flip rotation
    // A flip holds that spin for this long instead of leaving it to air control
    // and the body's angular damping, which together bleed about 3 rad/s and
    // stall the rotation before it completes. flipSpin * flipDuration is the
    // angle swept, so 9.0 * 0.70 is a whole turn.
    float flipDuration = 0.70f;
    // Grounded is ignored for this long after a jump, otherwise the ground probe
    // still hits on the next step and instantly hands the jumps back.
    float jumpLockout = 0.15f;
    // Requested rate at full deflection. The car settles at roughly 80% of it,
    // because the body's angularDamping keeps pulling the spin back down every
    // step: 7.0 measures as about 5.6 rad/s, or 300 degrees per second. Tune
    // this by measuring, not by reading the number.
    float airControlRate = 7.0f;
    float airControlResponse = 9.0f;   // how fast it reaches that rate, per second
    float airDamping = 0.8f;           // spin bleed with no air input, per second

    // Set when a jump or flip fires and cleared by whoever consumes it (the
    // effects). It is a latch rather than a per-step flag because Update runs
    // twice per rendered frame at 120 Hz, so a flag cleared each step would be
    // missed by half the jumps.
    bool jumpPending = false;

    bool jumpUsed = false;
    bool doubleJumpUsed = false;
    bool jumpHeldPrevious = false;
    float jumpLockoutRemaining = 0.0f;
    // While this is running the spin is held about flipAxis (world space) and air
    // control is ignored, which is what makes a flip a committed move.
    float flipTimeRemaining = 0.0f;
    Vector3 flipAxis = { 0.0f, 0.0f, 0.0f };

    // --- Stability
    float linearDamping = 0.1f;
    float angularDamping = 2.2f;
    float bodyFriction = 0.55f;
    float bodyRestitution = 0.05f;
    float groundProbe = 0.35f;     // how far below the box counts as driving
    // Reach of the same probe once already grounded, so bridging a concave curve
    // does not drop the car mid-climb. Only ever extends contact the car already
    // had, which is why it does not make the car grounded in mid air.
    float groundStickyProbe = 0.75f;
    float recoveryProbe = 1.30f;   // how far below the box the righting assist still works
    float uprightTorque = 6500.0f; // rolls the car back onto its wheels
    float tumbleDamping = 4.0f;    // bleeds off bump-induced pitch and roll, per second
    float driveUprightMin = 0.35f; // below this the car rights itself instead of driving
    // Holds the car against a wall or the ceiling, in m/s^2 into the surface. It
    // ramps in with the surface tilt and is exactly zero on level ground, so the
    // handling tuned in Milestone 04 is untouched. Must beat gravity on the
    // ceiling, where the full value applies.
    float surfaceStick = 22.0f;

    bool grounded = false;
    // Alignment with the surface the car is standing on: 1 sitting flat on it,
    // -1 inverted relative to it. On the floor that is the world up, on a wall it
    // is the wall's normal, which is what lets the same test work everywhere.
    float uprightness = 1.0f;
    // The surface that alignment is measured against: the normal under the car
    // when grounded, world up otherwise. ChaseCamera offsets along it, so the
    // view goes out from a wall instead of climbing it.
    Vector3 surfaceNormal = { 0.0f, 1.0f, 0.0f };

    StaticModelAsset carModel;
    // Fitted in Initialize() so the model always matches the collision box.
    float modelScale = 1.0f;
    float modelOffsetY = 0.0f;

    // Stand-in drawn only when the cooked model is missing.
    Model colliderModel = {};
    bool colliderModelLoaded = false;
};

#endif

#include "CarObject.h"

#include "BallObject.h"
#include "Lighting.h"
#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>

#include <cmath>

CarObject::~CarObject()
{
    carModel.Unload();
    if (colliderModelLoaded)
    {
        lighting::Detach(colliderModel);
        UnloadModel(colliderModel);
    }
}

void CarObject::Initialize(Scene &owner)
{
    scene = &owner;

    JPH::BoxShapeSettings boxSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), convexRadius);
    boxSettings.SetEmbedded();
    JPH::OffsetCenterOfMassShapeSettings shapeSettings(JPH::Vec3(0.0f, centerOfMassOffsetY, 0.0f), &boxSettings);
    shapeSettings.SetEmbedded();
    JPH::ShapeRefC shape = shapeSettings.Create().Get();

    JPH::BodyCreationSettings settings(shape,
                                       JPH::RVec3(spawnPosition.x, spawnPosition.y, spawnPosition.z),
                                       JPH::Quat::sRotation(JPH::Vec3::sAxisY(), spawnYawDegrees * DEG2RAD),
                                       JPH::EMotionType::Dynamic, physics::Car);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = mass;
    settings.mAllowSleeping = false;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
    ApplyTuning();

    if (carModel.Load(modelName))
    {
        // Fit the model to the collision box: scaled by length, which is the
        // dimension the eye judges against the ball and the goal, then lifted
        // so the wheels rest on the bottom face of the box.
        float modelLength = carModel.bounds.max.z - carModel.bounds.min.z;
        modelScale = modelLength > 0.0f ? (halfExtents.z * 2.0f) / modelLength : 1.0f;
        modelOffsetY = -halfExtents.y - carModel.bounds.min.y * modelScale;
        carModel.SetPaintColor(teamColor);
    }
    else
    {
        colliderModel = LoadModelFromMesh(GenMeshCube(halfExtents.x * 2.0f, halfExtents.y * 2.0f,
                                                      halfExtents.z * 2.0f));
        lighting::Apply(colliderModel);
        colliderModelLoaded = true;
    }
}

void CarObject::ApplyTuning()
{
    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    bodies.SetFriction(bodyID, bodyFriction);
    bodies.SetRestitution(bodyID, bodyRestitution);

    JPH::BodyLockWrite lock(scene->physicsSystem.GetBodyLockInterface(), bodyID);
    if (lock.Succeeded())
    {
        JPH::MotionProperties *motion = lock.GetBody().GetMotionProperties();
        motion->SetLinearDamping(linearDamping);
        motion->SetAngularDamping(angularDamping);
    }
}

void CarObject::Update(float deltaTime)
{
    CarInput input;
    if (controller != nullptr)
        input = controller->Poll(deltaTime);

    if (input.reset)
        ResetTo(spawnPosition, spawnYawDegrees);

    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    JPH::RVec3 position = bodies.GetPosition(bodyID);
    JPH::Quat rotation = bodies.GetRotation(bodyID);
    JPH::Vec3 velocity = bodies.GetLinearVelocity(bodyID);

    const JPH::Vec3 worldUp(0.0f, 1.0f, 0.0f);
    JPH::Vec3 forward = rotation * JPH::Vec3(0.0f, 0.0f, -1.0f);
    JPH::Vec3 right = rotation * JPH::Vec3(1.0f, 0.0f, 0.0f);
    JPH::Vec3 up = rotation * JPH::Vec3(0.0f, 1.0f, 0.0f);

    const JPH::BroadPhaseLayerFilter broadPhaseFilter;
    const JPH::ObjectLayerFilter objectFilter;
    const JPH::IgnoreSingleBodyFilter selfFilter(bodyID);
    JPH::RayCastResult hit;

    // The drive probe casts along the car's OWN down, not the world's, which is
    // the whole of what lets the car hold a wall or the ceiling: on level ground
    // with an upright car the two are the same ray. The surface it finds is then
    // what everything below measures against, instead of the world up.
    // A 3.2 m box bridges a concave curve: on the 5 m ramp its middle rides
    // 0.26 m higher than its ends, which eats most of a probe sized for flat
    // ground and drops the car exactly where it is trying to climb. So once
    // grounded, keep probing further until it genuinely leaves — `grounded` still
    // holds the previous step's answer here.
    // Not while a jump is in progress: a jump clears only 0.72 m in the 0.15 s of
    // jumpLockout, so a probe reaching 0.75 m would still find the floor as the
    // lockout expired, hand the jumps straight back, and let the car jump forever
    // — the exact bug jumpLockout exists to stop.
    JPH::Vec3 groundNormal = worldUp;
    const bool stickyGround = grounded && jumpLockoutRemaining <= 0.0f;
    JPH::RRayCast groundRay(position, -up * (halfExtents.y + (stickyGround ? groundStickyProbe : groundProbe)));
    grounded = scene->physicsSystem.GetNarrowPhaseQuery().CastRay(groundRay, hit, broadPhaseFilter,
                                                                 objectFilter, selfFilter);
    if (grounded)
    {
        JPH::BodyLockRead surfaceLock(scene->physicsSystem.GetBodyLockInterface(), hit.mBodyID);
        if (surfaceLock.Succeeded())
        {
            groundNormal = surfaceLock.GetBody().GetWorldSpaceSurfaceNormal(
                hit.mSubShapeID2, groundRay.GetPointOnRay(hit.mFraction));
        }
    }

    // The recovery probe stays WORLD down on purpose. It exists for a car resting
    // on its roof or balanced on an edge, and such a car has its own down pointing
    // at the sky — a local ray would never find the floor it needs to be righted
    // onto.
    bool nearGround = grounded;
    if (!nearGround)
    {
        JPH::RRayCast recoveryRay(position, JPH::Vec3(0.0f, -(halfExtents.y + recoveryProbe), 0.0f));
        nearGround = scene->physicsSystem.GetNarrowPhaseQuery().CastRay(recoveryRay, hit, broadPhaseFilter,
                                                                       objectFilter, selfFilter);
    }

    // Standing on a surface, align to that surface; otherwise fall back to the
    // world, so a car being recovered off its roof still comes back level.
    const JPH::Vec3 alignTo = grounded ? groundNormal : worldUp;
    uprightness = up.Dot(alignTo);
    surfaceNormal = Vector3{ alignTo.GetX(), alignTo.GetY(), alignTo.GetZ() };

    // Self-righting torque, so a bad landing or a hard bump never leaves the car stuck.
    // The axis is normalised on purpose: its raw length is sin(tilt), which vanishes
    // exactly when the car is upside down and would leave it stranded on its roof.
    //
    // It is suppressed while the player is rotating the car in the air: the assist
    // reaches 1.3 m above the ground, so without this it would quietly fight every
    // deliberate aerial. Ground input must not suppress it, hence the grounded test
    // — W is throttle on the ground and pitch only in the air.
    bool aerialInput = !grounded && (fabsf(input.airPitch) + fabsf(input.airYaw) +
                                     fabsf(input.airRoll)) > 0.01f;

    if (nearGround && !aerialInput && uprightness < 0.999f)
    {
        JPH::Vec3 uprightAxis = up.Cross(alignTo);
        if (uprightAxis.LengthSq() < 1.0e-4f)
            uprightAxis = forward; // perfectly inverted is a tie, so pick an axis to roll around

        float tilt = acosf(fminf(fmaxf(uprightness, -1.0f), 1.0f));
        // Squared so the assist is barely there when nearly level and full past 45 degrees.
        float strength = fminf(tilt / (PI * 0.25f), 1.0f);
        bodies.AddTorque(bodyID, uprightAxis.Normalized() * (uprightTorque * strength * strength));
    }

    // Boost is applied before the grounded gate on purpose: it is the one control
    // that has to keep working in the air, which is what Milestone 08 builds on.
    boosting = input.boost && boostAmount > 0.0f;
    boostHeldTime = boosting ? boostHeldTime + deltaTime : 0.0f;
    if (boosting)
    {
        boostAmount = fmaxf(boostAmount - boostDrainRate * deltaTime, 0.0f);
        if (velocity.Dot(forward) < boostMaxSpeed)
            bodies.AddForce(bodyID, forward * boostForce);
    }

    // Jump and flip. The rising edge is found here rather than in the controller
    // so it survives running several fixed steps inside one rendered frame.
    if (jumpLockoutRemaining > 0.0f)
        jumpLockoutRemaining = fmaxf(jumpLockoutRemaining - deltaTime, 0.0f);
    else if (grounded)
    {
        jumpUsed = false;
        doubleJumpUsed = false;
    }

    bool jumpEdge = input.jump && !jumpHeldPrevious;
    jumpHeldPrevious = input.jump;

    if (jumpEdge && !jumpUsed && grounded)
    {
        // Along the car's own up, so jumping off a wall pushes away from it.
        bodies.AddImpulse(bodyID, up * jumpImpulse);
        jumpUsed = true;
        jumpPending = true;
        jumpLockoutRemaining = jumpLockout;
    }
    else if (jumpEdge && jumpUsed && !doubleJumpUsed)
    {
        // A direction held turns the second jump into a flip that way; nothing
        // held is a plain second jump.
        JPH::Vec3 flipDirection = forward * input.throttle + right * input.steer;
        if (flipDirection.LengthSq() > 0.01f)
        {
            flipDirection = flipDirection.Normalized();
            bodies.AddImpulse(bodyID, flipDirection * flipImpulse);
            // up x direction is the axis that rolls the car over that way: it
            // gives a nose-down pitch for a forward flip and a roll for a side one.
            JPH::Vec3 axis = up.Cross(flipDirection).Normalized();
            bodies.SetAngularVelocity(bodyID, axis * flipSpin);
            // Held from here for flipDuration, so the flip actually finishes.
            flipAxis = Vector3{ axis.GetX(), axis.GetY(), axis.GetZ() };
            flipTimeRemaining = flipDuration;
            // Read after the impulse, so a flip fired while climbing a wall keeps
            // whatever lift that impulse gave it.
            flipVerticalCap = bodies.GetLinearVelocity(bodyID).GetY();
            // Whether there is anything down there to scrape. The body swings
            // halfExtents.z below its centre of mass as it comes over, and it
            // falls for the rest of the flip on top of that, so a car length of
            // clear air below is the point past which a flip cannot reach the
            // floor at all. nearGround is no use here: it stops 1.65 m down, and
            // a jump only ever lifts the car 1.84 m, so a flip at the top of a
            // jump read as a clean aerial and was launched like every other one.
            JPH::RRayCast flipRay(position, JPH::Vec3(0.0f, -halfExtents.z * 2.0f, 0.0f));
            flipCapped = scene->physicsSystem.GetNarrowPhaseQuery().CastRay(
                flipRay, hit, broadPhaseFilter, objectFilter, selfFilter);
            flipHitUsed = false;
            flipHitWindow = flipDuration + flipHitGrace;
            // The same lockout the first jump takes: the car is on its way off
            // the surface, and the probe still reaching it must not be read as
            // a landing (see the flip hold below).
            jumpLockoutRemaining = jumpLockout;
        }
        else
        {
            bodies.AddImpulse(bodyID, up * secondJumpImpulse);
        }
        doubleJumpUsed = true;
        jumpPending = true;
    }

    // The big hit: a flip that connects with the ball adds to what the collision
    // gave it. Read as a jump in the ball's speed, the same way MatchScene reads a
    // hit for the effects, and applied along the direction the ball has just been
    // sent — so it lands on the step after contact and amplifies a hit that has
    // already happened.
    //
    // It is deliberately not an impulse on approach. That was tried first and is
    // much worse: the ball is shoved out of the car's way before the two ever
    // meet, and the flip that should have been the hardest shot in the game left
    // the ball at 8.5 m/s and 20 m, against 26.2 m/s and 52 m for the same flip
    // with no bonus at all.
    if (ball != nullptr)
    {
        float ballSpeed = ball->GetSpeed();
        if (flipHitWindow > 0.0f)
        {
            flipHitWindow = fmaxf(flipHitWindow - deltaTime, 0.0f);
            // The proximity test is what says the hit was this car's and not the
            // other one's, and it is measured against the box rather than a
            // radius around it: the body is 3.2 m long and 1.7 m wide, so a
            // sphere big enough to cover the nose would claim hits a metre clear
            // of the flank.
            Vector3 ballCenter = ball->GetBodyPosition();
            JPH::Vec3 toBall = JPH::Vec3(ballCenter.x, ballCenter.y, ballCenter.z) -
                               JPH::Vec3(position.GetX(), position.GetY(), position.GetZ());
            JPH::Vec3 local = rotation.Conjugated() * toBall;
            JPH::Vec3 box(halfExtents.x, halfExtents.y, halfExtents.z);
            JPH::Vec3 nearest = JPH::Vec3::sMax(JPH::Vec3::sMin(local, box), -box);
            bool touching = (local - nearest).Length() < ball->radius + flipHitReach;

            if (!flipHitUsed && touching && ballSpeed - ballSpeedPrevious > flipHitThreshold)
            {
                JPH::Vec3 sent = bodies.GetLinearVelocity(ball->bodyID);
                if (sent.LengthSq() > 1.0e-6f)
                {
                    bodies.AddImpulse(ball->bodyID, sent.Normalized() * flipHitImpulse);
                    flipHitUsed = true;
                    ballSpeed = ball->GetSpeed();
                }
            }
        }
        ballSpeedPrevious = ballSpeed;
    }

    // A flip is a committed move: hold its spin rather than letting air control
    // and the body's angular damping bleed it away. Those two together decay
    // about 3 rad/s, which stalled every flip well short of a full turn —
    // measured, no flip ever swept even half of one.
    //
    // Tested before the grounded gate, and not inside it, because a flip fired a
    // few hundredths of a second after the jump still reads as grounded: the
    // probe reaches 0.7 m below the car, which it has not cleared yet. Inside
    // the gate that flip was cancelled by the landing reset on the very step it
    // fired, leaving the car with a raw 12 rad/s spin and no cap on it — the
    // floor then threw it 4.4 m up and it stayed airborne 3.75 s, tumbling.
    // Hence the lockout: while it runs, the probe is not a landing.
    if (flipTimeRemaining > 0.0f && (!grounded || jumpLockoutRemaining > 0.0f))
    {
        flipTimeRemaining = fmaxf(flipTimeRemaining - deltaTime, 0.0f);

        // A flip fired straight after a jump starts about half a metre up, and
        // the body is 3.2 m long: as it comes over, its nose or tail sweeps well
        // below that and Jolt resolves the overlap by throwing the car clear.
        // Measured, a forward flip left the ground at 4.4 m/s and was climbing
        // at 9.9 m/s a tenth of a second later — 6 m up and over two seconds in
        // the air, which is the floating the flip was reported for. So a flip is
        // ballistic: the climb may only fall away with gravity, never be added
        // to by the floor it is rotating through. Speed along the ground is left
        // alone — holding that as well only drove the nose deeper into the
        // floor, and the car was thrown 7 m up the moment the flip released it.
        // flipCapped is why a flip high in the air keeps its climb, and it is
        // read rather than re-measured: a car that leaves probe range mid
        // rotation and drops back into it slips a launch through the gap.
        flipVerticalCap += scene->physicsSystem.GetGravity().GetY() * deltaTime;
        JPH::Vec3 flipVelocity = bodies.GetLinearVelocity(bodyID);
        if (flipCapped && flipVelocity.GetY() > flipVerticalCap)
            bodies.SetLinearVelocity(bodyID, JPH::Vec3(flipVelocity.GetX(), flipVerticalCap,
                                                       flipVelocity.GetZ()));

        // The rotation is over when the timer is, so the spin is cancelled
        // rather than released: handing 12 rad/s back to a car that has just
        // come round to level carried it straight past upright and landed it on
        // its roof every time.
        JPH::Vec3 spin = flipTimeRemaining > 0.0f
                             ? JPH::Vec3(flipAxis.x, flipAxis.y, flipAxis.z) * flipSpin
                             : JPH::Vec3::sZero();
        bodies.SetAngularVelocity(bodyID, spin);
        return;
    }

    if (!grounded)
    {
        // Air control: drive the angular velocity towards the requested rate.
        // With no input the requested rate is zero and the response drops to
        // airDamping, so a flip still completes but the car settles for landing.
        JPH::Vec3 angularVelocity = bodies.GetAngularVelocity(bodyID);
        JPH::Vec3 desired = right * (input.airPitch * airControlRate) +
                            up * (-input.airYaw * airControlRate) +
                            forward * (input.airRoll * airControlRate);

        float response = aerialInput ? airControlResponse : airDamping;
        float blend = 1.0f - expf(-response * deltaTime);
        bodies.SetAngularVelocity(bodyID, angularVelocity + (desired - angularVelocity) * blend);
        return;
    }

    // Landing ends a flip, whatever is left of it.
    flipTimeRemaining = 0.0f;

    JPH::Vec3 angularVelocity = bodies.GetAngularVelocity(bodyID);
    float yawSpin = angularVelocity.Dot(up);
    // Rates are per second and converted with the step, so the feel does not
    // change if the fixed timestep is ever retuned.
    JPH::Vec3 tumble = (angularVelocity - up * yawSpin) * expf(-tumbleDamping * deltaTime);

    if (uprightness < driveUprightMin)
    {
        // On its side or roof: let it right itself instead of driving.
        bodies.SetAngularVelocity(bodyID, tumble + up * yawSpin);
        return;
    }

    // Hold the car against the surface. Nothing else does: on a vertical wall
    // gravity pulls along the wall rather than into it, so contact is lost the
    // moment the car stops being pressed there, and on the ceiling gravity pulls
    // straight off. Scaled by how far the surface has tilted past level, which is
    // 0 on the floor (so ground handling is bit-for-bit what it was), half on a
    // wall, and full upside down — where it has to beat gravity to work at all.
    float stickFraction = (1.0f - groundNormal.Dot(worldUp)) * 0.5f;
    if (stickFraction > 0.0f)
        bodies.AddForce(bodyID, -groundNormal * (surfaceStick * stickFraction * mass));

    float forwardSpeed = velocity.Dot(forward);

    if (input.throttle > 0.0f)
    {
        if (forwardSpeed < -0.5f)
            bodies.AddForce(bodyID, forward * (input.throttle * brakeForce)); // braking out of reverse
        else if (forwardSpeed < maxSpeed)
            bodies.AddForce(bodyID, forward * (input.throttle * engineForce));
    }
    else if (input.throttle < 0.0f)
    {
        if (forwardSpeed > 0.5f)
            bodies.AddForce(bodyID, forward * (input.throttle * brakeForce));
        else if (forwardSpeed > -maxReverseSpeed)
            bodies.AddForce(bodyID, forward * (input.throttle * engineForce));
    }
    else
    {
        bodies.AddForce(bodyID, forward * (-forwardSpeed * coastDrag * mass));
    }

    // Steering drives the yaw rate directly: crisper than torque and it never spins out.
    // Authority ramps in from a standstill and falls off at speed, so the car turns
    // tightly in traffic without pivoting on the spot at 100 km/h.
    float speed = fabsf(forwardSpeed);
    float speedRamp = fminf(speed / steerSpeedFloor, 1.0f);
    float fastBlend = fminf(fmaxf((speed - steerSpeedFloor) / (maxSpeed - steerSpeedFloor), 0.0f), 1.0f);
    float authority = 1.0f + (highSpeedSteerScale - 1.0f) * fastBlend;

    float yawRate = -input.steer * steerRate * speedRamp * authority;
    if (forwardSpeed < 0.0f)
        yawRate = -yawRate; // steering mirrors when reversing

    bodies.SetAngularVelocity(bodyID, tumble + up * yawRate);

    // Grip: bleed off the sideways velocity so the car follows its nose but still slides.
    float lateralSpeed = velocity.Dot(right);
    float gripFraction = 1.0f - expf(-grip * deltaTime);
    bodies.AddImpulse(bodyID, right * (-lateralSpeed * mass * gripFraction));
}

void CarObject::Draw()
{
    Vector3 position = GetBodyPosition();
    Matrix rotation = GetBodyRotation();

    if (!carModel.loaded)
    {
        colliderModel.transform = rotation;
        DrawModel(colliderModel, position, 1.0f, teamColor);
        DrawModelWires(colliderModel, position, 1.0f, Fade(BLACK, 0.35f));
        return;
    }

    carModel.model.transform = MatrixMultiply(
        MatrixMultiply(MatrixScale(modelScale, modelScale, modelScale),
                       MatrixRotateY(modelYawDegrees * DEG2RAD)),
        rotation);

    // The lift is in body space, so it follows the car when it rolls or pitches.
    Vector3 lift = Vector3Transform(Vector3{ 0.0f, modelOffsetY, 0.0f }, rotation);
    DrawModel(carModel.model, Vector3Add(position, lift), 1.0f, WHITE);
}

BoundingBox CarObject::GetWorldBounds() const
{
    JPH::AABox bounds = scene->physicsSystem.GetBodyInterface().GetTransformedShape(bodyID).GetWorldSpaceBounds();
    return BoundingBox{
        Vector3{ bounds.mMin.GetX(), bounds.mMin.GetY(), bounds.mMin.GetZ() },
        Vector3{ bounds.mMax.GetX(), bounds.mMax.GetY(), bounds.mMax.GetZ() }
    };
}

void CarObject::ResetTo(Vector3 position, float yawDegrees)
{
    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    bodies.SetPositionAndRotation(bodyID, JPH::RVec3(position.x, position.y, position.z),
                                  JPH::Quat::sRotation(JPH::Vec3::sAxisY(), yawDegrees * DEG2RAD),
                                  JPH::EActivation::Activate);
    bodies.SetLinearAndAngularVelocity(bodyID, JPH::Vec3::sZero(), JPH::Vec3::sZero());

    // The transform is only half of a reset: a car that spent both jumps before a
    // goal was kicking off unable to jump at all, and a lockout or a half-finished
    // flip carried straight through the countdown. jumpHeldPrevious is deliberately
    // left alone, so a jump key still held across the reset is not read as a fresh
    // press the moment play starts.
    jumpUsed = false;
    doubleJumpUsed = false;
    jumpLockoutRemaining = 0.0f;
    flipTimeRemaining = 0.0f;
    flipHitWindow = 0.0f;
    flipHitUsed = false;
    boosting = false;
    boostHeldTime = 0.0f;
    grounded = false;
    uprightness = 1.0f;
}

float CarObject::GetForwardSpeed() const
{
    const JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    JPH::Vec3 forward = bodies.GetRotation(bodyID) * JPH::Vec3(0.0f, 0.0f, -1.0f);
    return bodies.GetLinearVelocity(bodyID).Dot(forward);
}

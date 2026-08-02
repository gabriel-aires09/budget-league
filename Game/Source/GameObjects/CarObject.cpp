#include "CarObject.h"

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
        input = controller->Poll();

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
    uprightness = up.Dot(worldUp);

    // Two probes straight down from the middle of the box. The short one gates
    // driving; the long one gates the righting assist, because a car resting on
    // its roof or balanced on an edge sits too high for the short probe to hit.
    const JPH::BroadPhaseLayerFilter broadPhaseFilter;
    const JPH::ObjectLayerFilter objectFilter;
    const JPH::IgnoreSingleBodyFilter selfFilter(bodyID);
    JPH::RayCastResult hit;

    JPH::RRayCast groundRay(position, JPH::Vec3(0.0f, -(halfExtents.y + groundProbe), 0.0f));
    grounded = scene->physicsSystem.GetNarrowPhaseQuery().CastRay(groundRay, hit, broadPhaseFilter,
                                                                 objectFilter, selfFilter);
    bool nearGround = grounded;
    if (!nearGround)
    {
        JPH::RRayCast recoveryRay(position, JPH::Vec3(0.0f, -(halfExtents.y + recoveryProbe), 0.0f));
        nearGround = scene->physicsSystem.GetNarrowPhaseQuery().CastRay(recoveryRay, hit, broadPhaseFilter,
                                                                       objectFilter, selfFilter);
    }

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
        JPH::Vec3 uprightAxis = up.Cross(worldUp);
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
            bodies.SetAngularVelocity(bodyID, up.Cross(flipDirection).Normalized() * flipSpin);
        }
        else
        {
            bodies.AddImpulse(bodyID, up * secondJumpImpulse);
        }
        doubleJumpUsed = true;
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
}

float CarObject::GetForwardSpeed() const
{
    const JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    JPH::Vec3 forward = bodies.GetRotation(bodyID) * JPH::Vec3(0.0f, 0.0f, -1.0f);
    return bodies.GetLinearVelocity(bodyID).Dot(forward);
}

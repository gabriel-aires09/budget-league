#include "ChaseCamera.h"

#include "GameObjects/CarObject.h"
#include "PhysicsLayers.h"
#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <cmath>

// Flat (XZ) direction the camera should sit behind: the car's nose blended with
// where it is actually travelling.
static Vector3 FollowDirection(const CarObject &car, float velocityBlend)
{
    Vector3 forward = Vector3Transform(Vector3{ 0.0f, 0.0f, -1.0f }, car.GetBodyRotation());
    forward.y = 0.0f;
    if (Vector3Length(forward) < 0.001f)
        forward = Vector3{ 0.0f, 0.0f, -1.0f };
    forward = Vector3Normalize(forward);

    JPH::Vec3 joltVelocity = car.scene->physicsSystem.GetBodyInterface().GetLinearVelocity(car.bodyID);
    Vector3 velocity = { joltVelocity.GetX(), 0.0f, joltVelocity.GetZ() };
    if (Vector3Length(velocity) > 4.0f && car.GetForwardSpeed() > 0.0f)
        forward = Vector3Normalize(Vector3Lerp(forward, Vector3Normalize(velocity), velocityBlend));

    return forward;
}

// Metres of clear line from the car out towards where the camera wants to be:
// the whole way when nothing is in between, otherwise the distance to what is,
// less the margin. The margin belongs in here rather than at the call sites —
// subtracted unconditionally it would creep the camera in every single frame.
//
// The ray starts at the car, not at the look point, because the car is always in
// free space — a physics body cannot be inside static geometry — while the look
// point sits above the car and is therefore beyond the ceiling once the car is
// driving on it, which would make the very first hit be at zero distance.
static float FreeReach(const CarObject &car, Vector3 from, Vector3 to, float margin)
{
    Vector3 offset = Vector3Subtract(to, from);
    float reach = Vector3Length(offset);
    if (reach < 0.001f)
        return reach;

    JPH::RRayCast ray(JPH::RVec3(from.x, from.y, from.z), JPH::Vec3(offset.x, offset.y, offset.z));
    JPH::RayCastResult hit;
    const JPH::BroadPhaseLayerFilter broadPhaseFilter;
    // Only the arena and the goals block the view. The ball and the cars are on
    // their own layers on purpose: something passing behind the car must never
    // yank the camera in.
    const JPH::SpecifiedObjectLayerFilter objectFilter(physics::Arena);
    const JPH::BodyFilter bodyFilter;
    if (!car.scene->physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit, broadPhaseFilter,
                                                               objectFilter, bodyFilter))
        return reach;

    return reach * hit.mFraction - margin;
}

void ChaseCamera::Initialize(Camera3D &camera, const CarObject &car)
{
    Vector3 carPosition = car.GetBodyPosition();
    Vector3 forward = FollowDirection(car, 0.0f);

    position = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, distance)),
                          Vector3{ 0.0f, height, 0.0f });
    target = Vector3Add(carPosition, Vector3{ 0.0f, lookHeight, 0.0f });

    camera.position = position;
    camera.target = target;
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void ChaseCamera::Update(Camera3D &camera, const CarObject &car, Vector3 ballPosition, float deltaTime)
{
    if (IsKeyPressed(KEY_C))
        ballCam = !ballCam;

    Vector3 carPosition = car.GetBodyPosition();
    Vector3 forward = FollowDirection(car, velocityBlend);
    Vector3 carLook = Vector3Add(carPosition, Vector3{ 0.0f, lookHeight, 0.0f });
    Vector3 desiredTarget = carLook;
    float followDistance = distance;
    float followHeight = height;

    if (ballCam)
    {
        // Sit on the far side of the car from the ball, so the two line up in the
        // frame. That direction spins wildly as the car arrives at the ball —
        // exactly when the player most needs to see where the car is pointing —
        // so over the last few metres it gives way to the car's own heading.
        Vector3 toBall = { ballPosition.x - carPosition.x, 0.0f, ballPosition.z - carPosition.z };
        float ballRange = Vector3Length(toBall);
        if (ballRange > 0.001f)
        {
            float weight = ballRange / ballCamNearRange;
            if (weight > 1.0f)
                weight = 1.0f;
            forward = Vector3Normalize(Vector3Lerp(forward, Vector3Scale(toBall, 1.0f / ballRange), weight));
        }

        // Looking between the car and the ball rather than at either one: the
        // ball keeps its place in the frame as it climbs, and the car stays put.
        desiredTarget = Vector3Lerp(desiredTarget, ballPosition, ballCamLookBlend);
        followDistance = ballCamDistance;
        followHeight = ballCamHeight;
    }

    Vector3 desiredPosition = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, followDistance)),
                                         Vector3{ 0.0f, followHeight, 0.0f });
    // Held inside the arena before anything is measured against it: a mark above
    // the ceiling would make the test below read as blocked every single frame
    // the car spends up there.
    if (desiredPosition.y < minHeight)
        desiredPosition.y = minHeight;
    if (desiredPosition.y > maxHeight)
        desiredPosition.y = maxHeight;

    // A wall, a ramp or a corner behind the car: trade the distance that is not
    // there for height, rather than jamming the camera against the car. Up is
    // the one direction that is always open, because every concave join in this
    // arena curves away from the floor. Doing it to the *desired* position keeps
    // it smooth — the amount varies continuously with how much room is left.
    float reach = Vector3Distance(carPosition, desiredPosition);
    float room = FreeReach(car, carPosition, desiredPosition, wallMargin);
    if (reach > 0.001f && room < reach)
    {
        // The distance lost is whatever the geometry takes, but the height is
        // bought back on a curve: a wall a few metres behind the car barely
        // raises the view, while a ramp right up against it goes overhead.
        float blocked = 1.0f - (room > 0.0f ? room / reach : 0.0f);
        desiredPosition = Vector3Add(Vector3Subtract(carPosition,
                                                     Vector3Scale(forward, followDistance * (1.0f - blocked))),
                                     Vector3{ 0.0f, followHeight + blocked * blocked * blockedLift, 0.0f });
        if (desiredPosition.y > maxHeight)
            desiredPosition.y = maxHeight;

        // Looking back at the car as the view goes overhead. Without this, ball
        // cam keeps aiming a third of the way to the ball while the camera rises,
        // and the car drops out of the bottom of the frame.
        desiredTarget = Vector3Lerp(desiredTarget, carLook, blocked);
    }

    // Frame rate independent exponential smoothing. Both ends are inside the
    // arena, so what it produces is too.
    position = Vector3Lerp(position, desiredPosition, 1.0f - expf(-positionSmoothing * sensitivity * deltaTime));
    target = Vector3Lerp(target, desiredTarget, 1.0f - expf(-targetSmoothing * sensitivity * deltaTime));

    // Last word on clipping: whatever the smoothing produced, stop it short of
    // anything solid. The clamped position is kept, so the camera eases back out
    // once what it is up against is gone instead of snapping when the view clears.
    Vector3 offset = Vector3Subtract(position, carPosition);
    float distanceOut = Vector3Length(offset);
    float clear = FreeReach(car, carPosition, position, wallMargin);
    if (clear < minDistance)
        clear = minDistance;
    if (distanceOut > 0.001f && clear < distanceOut)
        position = Vector3Add(carPosition, Vector3Scale(offset, clear / distanceOut));

    camera.position = position;
    camera.target = target;
}

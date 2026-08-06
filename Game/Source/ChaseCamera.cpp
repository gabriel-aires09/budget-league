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
//
// `previous` is the last direction this returned, and it is what a car whose nose
// is not usefully horizontal falls back to. Driving a wall or the ceiling points
// the car nearly straight up or down, and what survives flattening that is noise:
// normalising a 0.01 m long vector hands back a direction that swings through a
// half circle between frames, which is what used to throw the camera into the
// wall it was climbing.
static Vector3 FollowDirection(const CarObject &car, float velocityBlend, Vector3 previous,
                               float flatMinimum)
{
    Vector3 forward = Vector3Transform(Vector3{ 0.0f, 0.0f, -1.0f }, car.GetBodyRotation());
    forward.y = 0.0f;
    forward = Vector3Length(forward) < flatMinimum ? previous : Vector3Normalize(forward);
    if (Vector3Length(forward) < 0.001f)
        forward = Vector3{ 0.0f, 0.0f, -1.0f };

    JPH::Vec3 joltVelocity = car.scene->physicsSystem.GetBodyInterface().GetLinearVelocity(car.bodyID);
    Vector3 velocity = { joltVelocity.GetX(), 0.0f, joltVelocity.GetZ() };
    if (Vector3Length(velocity) > 4.0f && car.GetForwardSpeed() > 0.0f)
        forward = Vector3Normalize(Vector3Lerp(forward, Vector3Normalize(velocity), velocityBlend));

    return forward;
}

// Metres of line from the car out towards where the camera wants to be before
// something solid is in the way — the whole way when nothing is, with `blocked`
// saying which of the two it was. The margin is applied by the callers, because
// only one of them wants it and subtracting it when nothing was hit would creep
// the camera in every single frame.
//
// The ray starts at the car, not at the look point, because the car is always in
// free space — a physics body cannot be inside static geometry — while the look
// point sits above the car and is therefore beyond the ceiling once the car is
// driving on it, which would make the very first hit be at zero distance.
static float SolidReach(const CarObject &car, Vector3 from, Vector3 to, bool &blocked)
{
    blocked = false;
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

    blocked = true;
    return reach * hit.mFraction;
}

void ChaseCamera::Initialize(Camera3D &camera, const CarObject &car)
{
    Vector3 carPosition = car.GetBodyPosition();
    // Reset rather than carried: a rematch builds a new scene but this widget may
    // outlive one, and a stale surface or ball-cam side would start it swinging.
    surfaceUp = Vector3{ 0.0f, 1.0f, 0.0f };
    ballCamActive = false;
    followDirection = FollowDirection(car, 0.0f, followDirection, flatForwardMinimum);
    Vector3 forward = followDirection;

    position = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, distance)),
                          Vector3Scale(surfaceUp, height));
    target = Vector3Add(carPosition, Vector3Scale(surfaceUp, lookHeight));

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
    // Remembered, because it is the fallback when the car's own nose stops being
    // a usable horizontal direction.
    followDirection = FollowDirection(car, velocityBlend, followDirection, flatForwardMinimum);
    Vector3 forward = followDirection;
    // The camera lifts along the surface the car is standing on, not along the
    // world. On the floor the two are the same vector, so flat-ground framing is
    // untouched; on a wall it takes the view out into the arena instead of
    // climbing the wall with the car, and near the ceiling that is the difference
    // between trailing the car and being crushed into the fillet above it.
    // Smoothed, or the floor-to-wall handover would snap.
    surfaceUp = Vector3Normalize(Vector3Lerp(surfaceUp, car.surfaceNormal,
                                             1.0f - expf(-surfaceUpSmoothing * deltaTime)));
    Vector3 carLook = Vector3Add(carPosition, Vector3Scale(surfaceUp, lookHeight));
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
            // Smoothstep, not the raw ratio. The handover has to be flat at both
            // ends: a linear weight changes the follow direction fastest exactly
            // as the ball crosses the car, which is where the view used to snap.
            weight = weight * weight * (3.0f - 2.0f * weight);
            forward = Vector3Normalize(Vector3Lerp(forward, Vector3Scale(toBall, 1.0f / ballRange), weight));
        }

        // Looking between the car and the ball rather than at either one: the
        // ball keeps its place in the frame as it climbs, and the car stays put.
        desiredTarget = Vector3Lerp(desiredTarget, ballPosition, ballCamLookBlend);
        followDistance = ballCamDistance;
        followHeight = ballCamHeight;

        // Swing the side the camera sits on round the car rather than letting the
        // position smoothing interpolate between the two. Overtaking the ball
        // reverses this direction, and a straight line from one side to the other
        // goes through the car: measured, the view crossed at 3 m a frame and the
        // car was off screen for twenty of them. Rotating keeps the camera at its
        // own distance the whole way round.
        //
        // Seeded rather than smoothed on the first frame in ball cam, so pressing
        // C never starts a swing from a direction the camera was never at.
        ballCamDirection = ballCamActive
                               ? Vector3Normalize(Vector3Lerp(ballCamDirection, forward,
                                                              1.0f - expf(-ballCamTurnRate * sensitivity * deltaTime)))
                               : forward;
        ballCamActive = true;
        forward = ballCamDirection;
    }

    Vector3 desiredPosition = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, followDistance)),
                                         Vector3Scale(surfaceUp, followHeight));
    // Held inside the arena before anything is measured against it: a mark above
    // the ceiling would make the test below read as blocked every single frame
    // the car spends up there.
    if (desiredPosition.y < minHeight)
        desiredPosition.y = minHeight;
    if (desiredPosition.y > maxHeight)
        desiredPosition.y = maxHeight;

    // A wall, a ramp or a corner behind the car: trade the distance that is not
    // there for height, rather than jamming the camera against the car. Off the
    // surface is the one direction that is always open, because every concave
    // join in this arena curves away from whatever the car is standing on — which
    // is why the lift goes along surfaceUp with the rest of the offset rather than
    // along the world. Doing it to the *desired* position keeps it smooth: the
    // amount varies continuously with how much room is left.
    float reach = Vector3Distance(carPosition, desiredPosition);
    bool hitSomething = false;
    float room = SolidReach(car, carPosition, desiredPosition, hitSomething) - (hitSomething ? wallMargin : 0.0f);
    if (reach > 0.001f && room < reach)
    {
        // The distance lost is whatever the geometry takes, but the height is
        // bought back on a curve: a wall a few metres behind the car barely
        // raises the view, while a ramp right up against it goes overhead.
        float blocked = 1.0f - (room > 0.0f ? room / reach : 0.0f);
        // minDistance belongs here, on how far the view may be pulled in, and
        // nowhere else. It used to live on the final clamp instead, where it was
        // not a floor on the pull-in at all but a licence to sit 1.5 m from the
        // car along a line that was already inside a wall.
        float pulled = fmaxf(followDistance * (1.0f - blocked), minDistance);
        desiredPosition = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, pulled)),
                                     Vector3Scale(surfaceUp, followHeight + blocked * blocked * blockedLift));
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
    // anything solid. This test now has no floor of its own — the surface is the
    // only thing that decides, so the eye can never be put on the far side of one.
    // The clamped position is kept, so the camera eases back out once what it is
    // up against is gone instead of snapping when the view clears.
    Vector3 offset = Vector3Subtract(position, carPosition);
    float distanceOut = Vector3Length(offset);
    bool eyeBlocked = false;
    float solid = SolidReach(car, carPosition, position, eyeBlocked);
    if (eyeBlocked && distanceOut > 0.001f)
    {
        // Keep the full margin when the gap can pay for it, and half the gap when
        // it cannot. Both are strictly inside the free space, which is the whole
        // property that was missing: the old floor sat the eye at 1.5 m whether or
        // not there was 1.5 m to sit in.
        float clear = fmaxf(solid - wallMargin, solid * 0.5f);
        if (clear < distanceOut)
            position = Vector3Add(carPosition, Vector3Scale(offset, clear / distanceOut));
    }

    camera.position = position;
    camera.target = target;
}

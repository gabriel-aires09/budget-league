#include "ChaseCamera.h"

#include "GameObjects/CarObject.h"
#include "Scene.h"

#include <raymath.h>

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

void ChaseCamera::Update(Camera3D &camera, const CarObject &car, float deltaTime)
{
    Vector3 carPosition = car.GetBodyPosition();
    Vector3 forward = FollowDirection(car, velocityBlend);

    Vector3 desiredPosition = Vector3Add(Vector3Subtract(carPosition, Vector3Scale(forward, distance)),
                                         Vector3{ 0.0f, height, 0.0f });
    Vector3 desiredTarget = Vector3Add(carPosition, Vector3{ 0.0f, lookHeight, 0.0f });

    // Frame rate independent exponential smoothing.
    position = Vector3Lerp(position, desiredPosition, 1.0f - expf(-positionSmoothing * deltaTime));
    target = Vector3Lerp(target, desiredTarget, 1.0f - expf(-targetSmoothing * deltaTime));

    if (position.y < 0.6f)
        position.y = 0.6f;

    camera.position = position;
    camera.target = target;
}

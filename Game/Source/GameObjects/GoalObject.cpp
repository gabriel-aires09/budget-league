#include "GoalObject.h"

#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <cmath>

void GoalObject::Initialize(Scene &owner)
{
    scene = &owner;

    const float halfWidth = width * 0.5f;
    const float t = frameThickness;
    const float midZ = lineZ + direction * depth * 0.5f;
    // These pieces define collision only. The enclosure is intentionally not
    // rendered, leaving an unobstructed view of cars and the ball in the goal.
    // Side panels, just outside the mouth so they never narrow it.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth + t * 0.5f), height * 0.5f, midZ },
                           { t * 0.5f, height * 0.5f, depth * 0.5f } });
    }

    // Floor of the recess. The arena floor slab ends at the goal line, so without
    // this anything that follows the ball into the net drops out of the world -
    // measured with the bot, which chased a rolling ball in and fell for good.
    pieces.push_back({ { 0.0f, -floorThickness * 0.5f, midZ },
                       { halfWidth + t, floorThickness * 0.5f, depth * 0.5f + t } });

    // Back of the net and its roof.
    pieces.push_back({ { 0.0f, height * 0.5f, lineZ + direction * (depth + t * 0.5f) },
                       { halfWidth + t, height * 0.5f, t * 0.5f } });
    pieces.push_back({ { 0.0f, height + t * 0.5f, midZ },
                       { halfWidth + t, t * 0.5f, depth * 0.5f + t } });

    JPH::StaticCompoundShapeSettings compound;
    compound.SetEmbedded();
    for (const Piece &piece : pieces)
    {
        JPH::BoxShapeSettings boxSettings(JPH::Vec3(piece.halfExtents.x, piece.halfExtents.y,
                                                    piece.halfExtents.z));
        boxSettings.SetEmbedded();
        compound.AddShape(JPH::Vec3(piece.center.x, piece.center.y, piece.center.z),
                          JPH::Quat::sIdentity(), boxSettings.Create().Get());
    }

    JPH::BodyCreationSettings settings(compound.Create().Get(), JPH::RVec3::sZero(),
                                       JPH::Quat::sIdentity(), JPH::EMotionType::Static, physics::Arena);
    settings.mFriction = 0.9f;
    // Deader than the arena walls, so a scored ball stays in the net.
    settings.mRestitution = 0.05f;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::DontActivate);

}

bool GoalObject::IsBallFullyInside(Vector3 ballCenter, float ballRadius) const
{
    // Distance past the goal line, measured along the direction the net faces.
    float past = (ballCenter.z - lineZ) * direction;
    return past >= ballRadius &&
           fabsf(ballCenter.x) <= width * 0.5f &&
           ballCenter.y <= height;
}

void GoalObject::Draw()
{
    // Only the solid-colour mouth frame is rendered. The collision enclosure
    // stays invisible so cars and the ball are always readable inside the net.
    DrawLine3D(Vector3{ -width * 0.5f, 0.03f, lineZ }, Vector3{ width * 0.5f, 0.03f, lineZ }, teamColor);
    DrawCubeWires(Vector3{ 0.0f, height * 0.5f, lineZ }, width, height, 0.06f, teamColor);
}

BoundingBox GoalObject::GetWorldBounds() const
{
    float nearZ = lineZ;
    float farZ = lineZ + direction * (depth + frameThickness);
    return BoundingBox{
        Vector3{ -width * 0.5f - frameThickness, 0.0f, fminf(nearZ, farZ) },
        Vector3{ width * 0.5f + frameThickness, height + frameThickness, fmaxf(nearZ, farZ) }
    };
}

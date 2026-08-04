#include "GoalObject.h"

#include "Lighting.h"
#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <cmath>

GoalObject::~GoalObject()
{
    if (boxModelLoaded)
    {
        lighting::Detach(boxModel);
        UnloadModel(boxModel);
    }
}

void GoalObject::Initialize(Scene &owner)
{
    scene = &owner;

    const float halfWidth = width * 0.5f;
    const float t = frameThickness;
    const float midZ = lineZ + direction * depth * 0.5f;
    const Color netColor = { (unsigned char)(teamColor.r / 3), (unsigned char)(teamColor.g / 3),
                             (unsigned char)(teamColor.b / 3), 255 };

    // Side panels, just outside the mouth so they never narrow it.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth + t * 0.5f), height * 0.5f, midZ },
                           { t * 0.5f, height * 0.5f, depth * 0.5f }, teamColor });
    }

    // Floor of the recess. The arena floor slab ends at the goal line, so without
    // this anything that follows the ball into the net drops out of the world -
    // measured with the bot, which chased a rolling ball in and fell for good.
    pieces.push_back({ { 0.0f, -floorThickness * 0.5f, midZ },
                       { halfWidth + t, floorThickness * 0.5f, depth * 0.5f + t }, netColor });

    // Back of the net and its roof.
    pieces.push_back({ { 0.0f, height * 0.5f, lineZ + direction * (depth + t * 0.5f) },
                       { halfWidth + t, height * 0.5f, t * 0.5f }, netColor });
    pieces.push_back({ { 0.0f, height + t * 0.5f, midZ },
                       { halfWidth + t, t * 0.5f, depth * 0.5f + t }, teamColor });

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

    boxModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    lighting::Apply(boxModel);
    boxModelLoaded = true;
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
    for (const Piece &piece : pieces)
    {
        DrawModelEx(boxModel, piece.center, Vector3{ 0.0f, 1.0f, 0.0f }, 0.0f,
                    Vector3Scale(piece.halfExtents, 2.0f), piece.color);
    }

    // The goal line itself, so it is obvious where the ball has to end up.
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

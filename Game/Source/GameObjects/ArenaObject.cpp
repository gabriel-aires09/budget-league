#include "ArenaObject.h"

#include "Lighting.h"
#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

static const Color FLOOR_COLOR = { 42, 50, 68, 255 };
static const Color WALL_COLOR = { 32, 39, 56, 255 };
static const Color TRIM_COLOR = { 52, 64, 90, 255 };

ArenaObject::~ArenaObject()
{
    if (boxModelLoaded)
    {
        lighting::Detach(boxModel);
        UnloadModel(boxModel);
    }
}

void ArenaObject::Initialize(Scene &owner)
{
    scene = &owner;

    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const float t = wallThickness;
    const float halfGoal = goalWidth * 0.5f;

    // Floor, with its top face at y = 0.
    pieces.push_back({ { 0.0f, -floorThickness * 0.5f, 0.0f },
                       { halfWidth, floorThickness * 0.5f, halfLength }, FLOOR_COLOR, true });

    // Side walls, overlapping the corners so there is no seam to catch on.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth + t * 0.5f), wallHeight * 0.5f, 0.0f },
                           { t * 0.5f, wallHeight * 0.5f, halfLength + t }, WALL_COLOR, true });
    }

    // Back walls, in three pieces so a goal-sized hole is left in the middle.
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        const float wallZ = end * (halfLength + t * 0.5f);
        const float panelHalfWidth = (halfWidth - halfGoal) * 0.5f;

        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            pieces.push_back({ { side * (halfGoal + panelHalfWidth), wallHeight * 0.5f, wallZ },
                               { panelHalfWidth, wallHeight * 0.5f, t * 0.5f }, WALL_COLOR, true });
        }

        pieces.push_back({ { 0.0f, (goalHeight + wallHeight) * 0.5f, wallZ },
                           { halfGoal, (wallHeight - goalHeight) * 0.5f, t * 0.5f }, WALL_COLOR, true });
    }

    // Ceiling: collision only. Drawing it would put a slab between the chase
    // camera and the field every time the car climbs a wall.
    pieces.push_back({ { 0.0f, wallHeight + t * 0.5f, 0.0f },
                       { halfWidth + t, t * 0.5f, halfLength + t }, WALL_COLOR, false });

    // Trim strip along the base of each side wall, purely visual.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth - 0.15f), 0.35f, 0.0f },
                           { 0.15f, 0.35f, halfLength }, TRIM_COLOR, true });
    }

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
    settings.mRestitution = 0.2f;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::DontActivate);

    boxModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    lighting::Apply(boxModel);
    boxModelLoaded = true;
}

void ArenaObject::Draw()
{
    for (const Piece &piece : pieces)
    {
        if (!piece.visible)
            continue;

        DrawModelEx(boxModel, piece.center, Vector3{ 0.0f, 1.0f, 0.0f }, 0.0f,
                    Vector3Scale(piece.halfExtents, 2.0f), piece.color);
    }

    // Field markings, drawn as unlit lines just above the floor. Explicit lines
    // rather than DrawGrid, which is square and cannot match a 55 x 80 field.
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const Color gridColor = { 90, 190, 255, 45 };
    for (float x = -halfWidth; x <= halfWidth + 0.01f; x += 5.0f)
        DrawLine3D(Vector3{ x, 0.02f, -halfLength }, Vector3{ x, 0.02f, halfLength }, gridColor);
    for (float z = -halfLength; z <= halfLength + 0.01f; z += 5.0f)
        DrawLine3D(Vector3{ -halfWidth, 0.02f, z }, Vector3{ halfWidth, 0.02f, z }, gridColor);

    DrawCircle3D(Vector3{ 0.0f, 0.02f, 0.0f }, 8.0f, Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f,
                 Color{ 90, 190, 255, 190 });
    DrawCubeWires(Vector3{ 0.0f, 0.02f, 0.0f }, width, 0.04f, length, Color{ 90, 190, 255, 120 });
    DrawLine3D(Vector3{ -halfWidth, 0.03f, 0.0f }, Vector3{ halfWidth, 0.03f, 0.0f },
               Color{ 90, 190, 255, 190 });
}

BoundingBox ArenaObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ -width * 0.5f - wallThickness, -floorThickness, -length * 0.5f - wallThickness },
        Vector3{ width * 0.5f + wallThickness, wallHeight + wallThickness, length * 0.5f + wallThickness }
    };
}

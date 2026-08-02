#include "ArenaObject.h"

#include "Scene.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

void ArenaObject::Initialize(Scene &owner)
{
    scene = &owner;

    // Floor slab with its top face at y = 0.
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(width * 0.5f, floorThickness * 0.5f, length * 0.5f));
    shapeSettings.SetEmbedded();
    JPH::ShapeRefC shape = shapeSettings.Create().Get();

    JPH::BodyCreationSettings settings(shape, JPH::RVec3(0.0f, -floorThickness * 0.5f, 0.0f),
                                       JPH::Quat::sIdentity(), JPH::EMotionType::Static, physics::Arena);
    settings.mFriction = 0.9f;
    settings.mRestitution = 0.2f;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::DontActivate);
}

void ArenaObject::Draw()
{
    DrawCube(Vector3{ 0.0f, -floorThickness * 0.5f, 0.0f }, width, floorThickness, length, Color{ 28, 34, 48, 255 });
    DrawGrid((int)(length / 4.0f) & ~1, 4.0f); // kept inside the shorter field axis

    // Center circle and the outline of the playing field.
    DrawCircle3D(Vector3{ 0.0f, 0.02f, 0.0f }, 8.0f, Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 90, 190, 255, 190 });
    DrawCubeWires(Vector3{ 0.0f, 0.02f, 0.0f }, width, 0.04f, length, Color{ 90, 190, 255, 120 });
}

BoundingBox ArenaObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ -width * 0.5f, -floorThickness, -length * 0.5f },
        Vector3{ width * 0.5f, 0.0f, length * 0.5f }
    };
}

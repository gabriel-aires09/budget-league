#include "BallObject.h"

#include "Scene.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

BallObject::~BallObject()
{
    if (modelLoaded)
        UnloadModel(ballModel);
}

void BallObject::Initialize(Scene &owner)
{
    scene = &owner;

    JPH::SphereShapeSettings shapeSettings(radius);
    shapeSettings.SetEmbedded();
    JPH::ShapeRefC shape = shapeSettings.Create().Get();

    JPH::BodyCreationSettings settings(shape,
                                       JPH::RVec3(spawnPosition.x, spawnPosition.y, spawnPosition.z),
                                       JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, physics::Ball);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = mass;
    // The fastest thing in the scene, and the one that must never slip through a
    // wall when the arena is closed in Milestone 06.
    settings.mMotionQuality = JPH::EMotionQuality::LinearCast;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
    ApplyTuning();

    // Low ring/slice counts on purpose: the facets are the look (CLAUDE.md 4.2).
    ballModel = LoadModelFromMesh(GenMeshSphere(radius, 6, 10));
    modelLoaded = true;
}

void BallObject::ApplyTuning()
{
    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    bodies.SetFriction(bodyID, friction);
    bodies.SetRestitution(bodyID, restitution);

    JPH::BodyLockWrite lock(scene->physicsSystem.GetBodyLockInterface(), bodyID);
    if (lock.Succeeded())
    {
        JPH::MotionProperties *motion = lock.GetBody().GetMotionProperties();
        motion->SetLinearDamping(linearDamping);
        motion->SetAngularDamping(angularDamping);
        motion->SetGravityFactor(gravityFactor);
    }
}

void BallObject::Update(float deltaTime)
{
    (void)deltaTime;

    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    JPH::Vec3 velocity = bodies.GetLinearVelocity(bodyID);
    float speed = velocity.Length();
    if (speed > maxSpeed)
        bodies.SetLinearVelocity(bodyID, velocity * (maxSpeed / speed));
}

void BallObject::Draw()
{
    Vector3 position = GetBodyPosition();
    ballModel.transform = GetBodyRotation();

    DrawModel(ballModel, position, 1.0f, ballColor);
    // The wireframe is what makes the spin readable on a flat shaded sphere.
    DrawModelWires(ballModel, position, 1.0f, Color{ 70, 110, 160, 140 });
}

BoundingBox BallObject::GetWorldBounds() const
{
    Vector3 position = GetBodyPosition();
    return BoundingBox{
        Vector3{ position.x - radius, position.y - radius, position.z - radius },
        Vector3{ position.x + radius, position.y + radius, position.z + radius }
    };
}

void BallObject::ResetTo(Vector3 position)
{
    JPH::BodyInterface &bodies = scene->physicsSystem.GetBodyInterface();
    bodies.SetPositionAndRotation(bodyID, JPH::RVec3(position.x, position.y, position.z),
                                  JPH::Quat::sIdentity(), JPH::EActivation::Activate);
    bodies.SetLinearAndAngularVelocity(bodyID, JPH::Vec3::sZero(), JPH::Vec3::sZero());
}

float BallObject::GetSpeed() const
{
    return scene->physicsSystem.GetBodyInterface().GetLinearVelocity(bodyID).Length();
}

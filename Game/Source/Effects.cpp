#include "Effects.h"

#include "Scene.h"

#include <raymath.h>
#include <rlgl.h>

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>

#include <cmath>

namespace
{
    // A goal burst is the biggest single call at 90; the rest are 10 to 20, and
    // the trail leaves a few per frame. Oldest-wins when it is full, which is
    // what keeps a goal burst from being eaten by the trail behind it.
    const int MAX_PARTICLES = 640;

    struct Particle
    {
        Vector3 position;
        Vector3 velocity;
        float life;
        float maxLife;
        float size;
        float gravity;
        Color color;
    };

    Particle particles[MAX_PARTICLES];
    int nextParticle = 0;

    Model cubeMesh = {};
    Model discMesh = {};
    Model coneMesh = {};
    bool meshesLoaded = false;

    float RandomUnit()
    {
        return (float)GetRandomValue(-1000, 1000) / 1000.0f;
    }
}

void effects::Load()
{
    if (meshesLoaded)
        return;

    // None of these three go through lighting::Apply. They are meant to read as
    // light and shadow rather than as lit surfaces, and the flat-shading in the
    // lit shader would turn the flame into a solid orange cone.
    cubeMesh = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    discMesh = LoadModelFromMesh(GenMeshCylinder(1.0f, 0.02f, 18));
    coneMesh = LoadModelFromMesh(GenMeshCone(1.0f, 1.0f, 10));
    meshesLoaded = true;

    effects::Clear();
}

void effects::Unload()
{
    if (!meshesLoaded)
        return;

    UnloadModel(cubeMesh);
    UnloadModel(discMesh);
    UnloadModel(coneMesh);
    meshesLoaded = false;
}

void effects::Clear()
{
    for (Particle &particle : particles)
        particle.life = 0.0f;
    nextParticle = 0;
}

void effects::Update(float deltaTime)
{
    for (Particle &particle : particles)
    {
        if (particle.life <= 0.0f)
            continue;

        particle.life -= deltaTime;
        particle.velocity.y -= particle.gravity * deltaTime;
        particle.position = Vector3Add(particle.position, Vector3Scale(particle.velocity, deltaTime));
        // Anything that reaches the floor stops there rather than sinking through
        // it, which is cheaper than a collision query and reads the same.
        if (particle.position.y < 0.05f)
        {
            particle.position.y = 0.05f;
            particle.velocity = Vector3Scale(particle.velocity, 0.4f);
            particle.velocity.y = 0.0f;
        }
    }
}

void effects::Burst(Vector3 position, Vector3 direction, int count, float speed, float spread,
                    Color color, float size, float life)
{
    Vector3 aim = Vector3Length(direction) > 0.001f ? Vector3Normalize(direction) : Vector3{ 0.0f, 1.0f, 0.0f };

    for (int i = 0; i < count; ++i)
    {
        Particle &particle = particles[nextParticle];
        nextParticle = (nextParticle + 1) % MAX_PARTICLES;

        Vector3 scatter = { RandomUnit(), RandomUnit(), RandomUnit() };
        Vector3 heading = Vector3Normalize(Vector3Add(aim, Vector3Scale(scatter, spread)));

        particle.position = position;
        // The speed spread matters more than the direction spread: an even shell
        // of particles reads as a balloon, a ragged one as an explosion.
        particle.velocity = Vector3Scale(heading, speed * (0.45f + 0.55f * fabsf(RandomUnit())));
        particle.maxLife = life * (0.6f + 0.4f * fabsf(RandomUnit()));
        particle.life = particle.maxLife;
        particle.size = size;
        particle.gravity = 9.0f;
        particle.color = color;
    }
}

void effects::Draw()
{
    if (!meshesLoaded)
        return;

    rlDisableBackfaceCulling();
    for (const Particle &particle : particles)
    {
        if (particle.life <= 0.0f)
            continue;

        // Shrinking and fading together, so a particle leaves rather than blinks.
        float remaining = particle.life / particle.maxLife;
        float size = particle.size * (0.35f + 0.65f * remaining);
        DrawModelEx(cubeMesh, particle.position, Vector3{ 0.0f, 1.0f, 0.0f }, 0.0f,
                    Vector3{ size, size, size }, Fade(particle.color, remaining));
    }
    rlEnableBackfaceCulling();
}

void effects::DrawBoostFlame(Vector3 position, Matrix rotation, float carHalfLength, float strength)
{
    if (!meshesLoaded || strength <= 0.0f)
        return;

    // The cone is generated pointing along +Y, and the exhaust points along the
    // car's local +Z - the car drives towards -Z - so it is turned a quarter
    // turn about X before the car's own rotation is applied.
    float length = 2.3f * strength;
    float width = 0.52f * strength;

    Vector3 exhaust = Vector3Add(position, Vector3Transform(Vector3{ 0.0f, 0.05f, carHalfLength * 0.95f }, rotation));
    Matrix orient = MatrixMultiply(MatrixRotateX(90.0f * DEG2RAD), rotation);

    rlDisableBackfaceCulling();
    coneMesh.transform = MatrixMultiply(MatrixScale(width, length, width), orient);
    DrawModel(coneMesh, exhaust, 1.0f, Fade(Color{ 255, 168, 60, 255 }, 0.75f));
    // A brighter, shorter core inside the first cone, which is what makes it read
    // as a flame instead of a traffic cone.
    coneMesh.transform = MatrixMultiply(MatrixScale(width * 0.55f, length * 0.55f, width * 0.55f), orient);
    DrawModel(coneMesh, exhaust, 1.0f, Fade(Color{ 255, 244, 214, 255 }, 0.9f));
    rlEnableBackfaceCulling();
}

void effects::DrawContactShadow(Scene &scene, Vector3 position, float radius, JPH::BodyID ignore)
{
    if (!meshesLoaded)
        return;

    const float maxDrop = 12.0f;
    const JPH::BroadPhaseLayerFilter broadPhaseFilter;
    const JPH::ObjectLayerFilter objectFilter;
    const JPH::IgnoreSingleBodyFilter selfFilter(ignore);

    JPH::RRayCast ray(JPH::RVec3(position.x, position.y, position.z), JPH::Vec3(0.0f, -maxDrop, 0.0f));
    JPH::RayCastResult hit;
    if (!scene.physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit, broadPhaseFilter, objectFilter, selfFilter))
        return;

    JPH::Vec3 point = ray.GetPointOnRay(hit.mFraction);
    JPH::Vec3 normal(0.0f, 1.0f, 0.0f);
    {
        JPH::BodyLockRead lock(scene.physicsSystem.GetBodyLockInterface(), hit.mBodyID);
        if (lock.Succeeded())
            normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, point);
    }

    // Softer and wider the further the object is from the surface, as a real
    // contact shadow spreads.
    float drop = maxDrop * hit.mFraction;
    float closeness = 1.0f - drop / maxDrop;
    float scale = radius * (1.0f + 0.5f * (1.0f - closeness));
    float alpha = 0.45f * closeness * closeness;
    if (alpha < 0.02f)
        return;

    // Laid on the surface it was found on, so it follows the ramps instead of
    // cutting into them.
    Vector3 up = { 0.0f, 1.0f, 0.0f };
    Vector3 surface = { normal.GetX(), normal.GetY(), normal.GetZ() };
    Vector3 axis = Vector3CrossProduct(up, surface);
    float angle = 0.0f;
    if (Vector3Length(axis) > 0.0001f)
    {
        axis = Vector3Normalize(axis);
        angle = acosf(Clamp(Vector3DotProduct(up, surface), -1.0f, 1.0f)) * RAD2DEG;
    }
    else
    {
        axis = up;
    }

    Vector3 at = { point.GetX() + surface.x * 0.02f, point.GetY() + surface.y * 0.02f,
                   point.GetZ() + surface.z * 0.02f };
    DrawModelEx(discMesh, at, axis, angle, Vector3{ scale, 1.0f, scale }, Fade(BLACK, alpha));
}

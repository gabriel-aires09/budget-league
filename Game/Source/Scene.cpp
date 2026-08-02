#include "Scene.h"

#include "GameObject.h"

#include <Jolt/Physics/PhysicsSettings.h>

// Physics runs at a fixed rate so the arcade forces stay frame rate independent.
static const float FIXED_STEP = 1.0f / 120.0f;
static const float MAX_FRAME_TIME = 0.25f;

Scene::Scene() :
    tempAllocator(8 * 1024 * 1024),
    jobSystem(JPH::cMaxPhysicsJobs)
{
}

void Scene::InitializePhysics()
{
    const JPH::uint maxBodies = 1024;
    const JPH::uint numBodyMutexes = 0;
    const JPH::uint maxBodyPairs = 1024;
    const JPH::uint maxContactConstraints = 1024;

    physicsSystem.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                       layerFilters.broadPhase, layerFilters.objectVsBroadPhase, layerFilters.objectPair);
}

void Scene::StepPhysics(float deltaTime)
{
    if (deltaTime > MAX_FRAME_TIME)
        deltaTime = MAX_FRAME_TIME;

    physicsAccumulator += deltaTime;
    while (physicsAccumulator >= FIXED_STEP)
    {
        for (GameObject *object : objects)
            object->Update(FIXED_STEP);

        physicsSystem.Update(FIXED_STEP, 1, &tempAllocator, &jobSystem);
        physicsAccumulator -= FIXED_STEP;
    }
}

#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include <raylib.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "GameSettings.h"
#include "MenuAction.h"
#include "PhysicsLayers.h"

class GameObject;

// Holds the camera, the GameObjects and the physics system of one scene, and runs
// the logic of each object.
class Scene
{
public:
    Scene();
    virtual ~Scene() {}

    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
    virtual void Shutdown() = 0;

    void InitializePhysics();
    // Steps physics at a fixed rate, updating every object right before each step.
    void StepPhysics(float deltaTime);

    // layerFilters must outlive physicsSystem, so it is declared first.
    physics::LayerFilters layerFilters;
    JPH::PhysicsSystem physicsSystem;
    JPH::TempAllocatorImpl tempAllocator;
    JPH::JobSystemSingleThreaded jobSystem;

    std::vector<GameObject *> objects;
    Camera3D camera = {};

    // Owned by App: the settings instance shared by every menu, and what the
    // scene wants App to do next.
    GameSettings *settings = nullptr;
    MenuAction pendingAction = MenuAction::None;

    float physicsAccumulator = 0.0f;
};

#endif

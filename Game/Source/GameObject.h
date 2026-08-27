#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <raylib.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

class Scene;

// One object in the 3D world, with its own shape, logic and physics body.
class GameObject
{
public:
    virtual ~GameObject() {}

    virtual void Initialize(Scene &owner) = 0;
    // Called once per fixed physics step, right before the simulation runs.
    virtual void Update(float deltaTime) { (void)deltaTime; }
    virtual void Draw() = 0;
    virtual BoundingBox GetWorldBounds() const = 0;

    Vector3 GetBodyPosition() const;
    Vector3 GetBodyVelocity() const;
    Matrix GetBodyRotation() const;

    Scene *scene = nullptr;
    JPH::BodyID bodyID;
};

#endif

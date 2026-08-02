#include "GameObject.h"

#include "Scene.h"

#include <raymath.h>

#include <Jolt/Physics/Body/BodyInterface.h>

Vector3 GameObject::GetBodyPosition() const
{
    JPH::RVec3 position = scene->physicsSystem.GetBodyInterface().GetPosition(bodyID);
    return Vector3{ (float)position.GetX(), (float)position.GetY(), (float)position.GetZ() };
}

Matrix GameObject::GetBodyRotation() const
{
    // Both libraries use right handed Y up and (x, y, z, w) quaternions.
    JPH::Quat rotation = scene->physicsSystem.GetBodyInterface().GetRotation(bodyID);
    return QuaternionToMatrix(Quaternion{ rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() });
}

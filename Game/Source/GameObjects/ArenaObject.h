#ifndef ARENAOBJECT_H
#define ARENAOBJECT_H

#include "GameObject.h"

// Static arena geometry. Milestone 02 only builds the floor; walls, ceiling and
// goals arrive in Milestone 06.
class ArenaObject final : public GameObject
{
public:
    virtual void Initialize(Scene &owner) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    float width = 80.0f;  // along X
    float length = 55.0f; // along Z
    float floorThickness = 2.0f;
};

#endif

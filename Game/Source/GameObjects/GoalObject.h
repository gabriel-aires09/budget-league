#ifndef GOALOBJECT_H
#define GOALOBJECT_H

#include "GameObject.h"

#include <vector>

// One coloured goal: the net structure behind the opening in the back wall, plus
// the trigger volume that decides when the ball is in.
//
// The trigger is an analytic box test rather than a Jolt sensor body. "Fully
// across the line" is a statement about the ball's centre and radius, which is
// exact and one comparison; a sensor would have to be inset by a ball diameter
// and would still depend on when contacts happen to be generated.
class GoalObject final : public GameObject
{
public:
    virtual ~GoalObject();

    virtual void Initialize(Scene &owner) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    // True once no part of the ball is left in front of the goal line.
    bool IsBallFullyInside(Vector3 ballCenter, float ballRadius) const;

    // Set before Initialize. lineZ is the goal line; the net runs from there
    // towards +Z when direction is +1.
    float lineZ = 40.0f;
    float direction = 1.0f;
    float width = 14.0f;
    float height = 5.0f;
    float depth = 4.0f;
    float frameThickness = 0.6f;

    int defendingTeam = 0; // 0 blue, 1 orange. The other team scores here.
    Color teamColor = { 60, 140, 255, 255 };

    struct Piece
    {
        Vector3 center;
        Vector3 halfExtents;
        Color color;
    };
    std::vector<Piece> pieces;

    Model boxModel = {};
    bool boxModelLoaded = false;
};

#endif

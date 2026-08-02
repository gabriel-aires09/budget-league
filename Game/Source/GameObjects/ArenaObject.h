#ifndef ARENAOBJECT_H
#define ARENAOBJECT_H

#include "GameObject.h"

#include <vector>

// Static arena: floor, side walls, ceiling and the two back walls, each with a
// goal-sized opening cut into it. All of it is one Jolt compound body.
//
// Z is the goal-to-goal axis, X is sideline to sideline. Keep every surface flat
// and smooth: the car is a box with no wheels, so any vertical lip taller than a
// few centimetres stops it dead.
class ArenaObject final : public GameObject
{
public:
    virtual ~ArenaObject();

    virtual void Initialize(Scene &owner) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    // The glass walls, which MUST be drawn after every other object in the scene.
    // They are see-through so the camera can sit outside the arena and still show
    // the car, and that only works if nothing writes depth over the car first.
    void DrawGlassWalls();

    float width = 55.0f;  // along X, sideline to sideline
    float length = 80.0f; // along Z, goal line to goal line
    float wallHeight = 15.0f;
    float wallThickness = 2.0f;
    float floorThickness = 2.0f;

    // The opening in each back wall. GoalObject must be built from the same
    // numbers, so MatchScene copies them across rather than repeating them.
    float goalWidth = 14.0f;
    float goalHeight = 5.0f;

    // One box of the arena. Physics and rendering both read this list, so the
    // collision can never drift away from what is on screen.
    // A colour with alpha below 255 marks the piece as glass: it is skipped by
    // Draw and picked up by DrawGlassWalls instead.
    struct Piece
    {
        Vector3 center;
        Vector3 halfExtents;
        Color color;
        bool visible;
    };
    std::vector<Piece> pieces;

    Model boxModel = {}; // unit cube, scaled per piece
    bool boxModelLoaded = false;
};

#endif

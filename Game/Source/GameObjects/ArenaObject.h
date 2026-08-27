#ifndef ARENAOBJECT_H
#define ARENAOBJECT_H

#include "GameObject.h"

#include <vector>

// Static arena: floor, side walls, ceiling and the two back walls, each with a
// goal-sized opening cut into it, plus the curved ramps that join the floor to
// the walls and the walls to the ceiling. All of it is one Jolt compound body.
//
// Z is the goal-to-goal axis, X is sideline to sideline. Keep every surface flat
// and smooth: the car is a box with no wheels, so any vertical lip taller than a
// few centimetres stops it dead. That is exactly why the wall edges are ramped
// rather than square — see AddFillet.
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

    float width = 76.81f;   // along X, sideline to sideline
    float length = 102.41f; // along Z, goal line to goal line
    float wallHeight = 20.73f;
    float wallThickness = 2.0f;
    float floorThickness = 2.0f;

    // Radius of the quarter-circle ramp at each edge of the arena, approximated
    // by rampSegments tilted boxes. The floor one is the one the car drives up,
    // so it is the larger of the two; both eat into the flat playing surface,
    // which is what FlatHalfWidth/FlatHalfLength report.
    float floorRampRadius = 5.0f;
    float ceilingRampRadius = 3.5f;
    int rampSegments = 8;

    // The vertical rounded corner that replaces each 90 degree wall intersection,
    // measured in the XZ plane. Must stay larger than floorRampRadius: the floor
    // ramp is carried around it on a circle of radius (cornerRadius - floorRamp),
    // which has to be positive for the corner to have any flat floor left in it.
    // Its own division is shared by the corner and both of its ramps, so their
    // facets line up and the three surfaces meet without a step.
    float cornerRadius = 8.0f;
    int cornerSegments = 10;

    float FlatHalfWidth() const { return width * 0.5f - floorRampRadius; }
    float FlatHalfLength() const { return length * 0.5f - floorRampRadius; }

    // The opening in each back wall, and how far the net recess reaches behind
    // it. GoalObject must be built from the same numbers, so MatchScene copies
    // them across rather than repeating them. The depth is the arena's business
    // as well as the goal's: the stands have to start beyond the recess, or they
    // stand inside it (see AddStadium).
    // These are the Rocket League reference goal, in the same 100 uu = 1 m the
    // field dimensions above use (Milestone 16): 1786 x 642.775 x 880 uu. The
    // recess used to be 4 m deep, which is barely longer than the 3.2 m car — a
    // car that followed the ball in was wedged nose-to-back-of-net with no room
    // to turn round. 8.8 m is what makes the inside of a goal a place you can
    // actually drive in.
    //
    // goalHeight is no longer tied to floorRampRadius. It used to be exactly 5.0
    // like the ramp, and DrawGlassWalls quietly relied on that: the end-wall seam
    // and the first lattice row start at the top of the ramp, so with a taller
    // mouth they need skipping around the opening. See DrawGlassWalls.
    float goalWidth = 17.86f;
    float goalHeight = 6.43f;
    float goalDepth = 8.8f;

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
        // Identity for everything flat, which is why the walls and floor can
        // still be written as a plain four-field initializer.
        Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        // The stands and the light rigs are drawn but have no collision: they sit
        // outside the glass, where nothing can ever reach them. Everything that
        // makes up the arena proper leaves this true.
        bool solid = true;
    };
    std::vector<Piece> pieces;

    // One ramp, drawn as a single continuous surface. Its collision is still the
    // boxes in `pieces`, which are marked invisible; this mesh is generated from
    // the same arc in the same call, so the two cannot drift apart. Floor ramps
    // carry vertex colors for their subtle gray transparency gradient.
    //
    // Drawing the boxes instead made a transparent ramp read as stacked bands:
    // eight overlapping boxes compound their alpha wherever they overlap, and
    // each one blends its near face and its far face. A strip has neither.
    struct RampMesh
    {
        Model model;
        Color color;
    };
    std::vector<RampMesh> rampMeshes;

    Model boxModel = {}; // unit cube, scaled per piece
    bool boxModelLoaded = false;

private:
    // Appends one quarter-circle ramp, as rampSegments tilted boxes.
    //
    // corner  the line where the wall meets the flat surface
    // inward  horizontal unit vector, from the wall towards the middle of the arena
    // up      unit vector off the flat surface into open air (+Y floor, -Y ceiling)
    // runAxis unit vector the ramp is extruded along, centred on runCenter
    //
    // Nothing here assumes `up` is vertical, which is what lets the same routine
    // build a vertical rounded corner out of two walls.
    // The drawn strip may cover less of the run than the collision does. Straight
    // runs still overlap into the rounded corners, because trimming their solid
    // measurably changed how a car takes a corner — but glass never writes depth,
    // so every overlapping layer that IS drawn keeps blending and lights the
    // corner up. Collision overlaps; drawing stops at the corner.
    void AddFillet(Vector3 corner, Vector3 inward, Vector3 up, Vector3 runAxis,
                   float radius, float runCenter, float runHalfLength, Color color, int segments,
                   float drawCenter, float drawHalfLength, float drawTaper);

    // One rounded vertical corner: the quarter cylinder joining two walls, plus
    // the floor and ceiling ramps carried around it as a ring of short straight
    // fillets — a faceted torus.
    void AddCorner(float sideX, float sideZ);
    // Blocky stands and light rigs outside the glass. Drawn, never collided with.
    void AddStadium();
};

#endif

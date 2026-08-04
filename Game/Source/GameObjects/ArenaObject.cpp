#include "ArenaObject.h"

#include "Lighting.h"
#include "Scene.h"

#include <raymath.h>
#include <rlgl.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <cmath>

static const Color FLOOR_COLOR = { 42, 50, 68, 255 };
// Glass, not masonry: the chase camera regularly ends up outside the arena, and
// a solid wall then hides the car completely. The alpha is what puts these in
// the late, depth-write-free pass (see DrawGlassWalls).
static const Color WALL_COLOR = { 96, 132, 190, 52 };
// Glass too, so the whole edge of the arena — ramp, wall, ceiling ramp — is one
// continuous see-through surface. Held a little more opaque than WALL_COLOR
// because this one is driven on and the slope still has to read.
static const Color RAMP_COLOR = { 96, 132, 190, 90 };

// How deep each ramp box is behind its driving face. Has to clear the car's box
// but stay inside the floor slab and the wall it is buried in, so under the 2 m
// of both.
static const float RAMP_THICKNESS = 1.5f;

ArenaObject::~ArenaObject()
{
    if (boxModelLoaded)
    {
        lighting::Detach(boxModel);
        UnloadModel(boxModel);
    }

    // Detach first, every time: UnloadModel would otherwise destroy the one
    // shared lit shader and leave the rest of the scene drawing with a dead one.
    for (RampMesh &ramp : rampMeshes)
    {
        lighting::Detach(ramp.model);
        UnloadModel(ramp.model);
    }
    rampMeshes.clear();
}

// A point on the ramp's arc. Angle 0 lies on the flat surface, 90 degrees on the
// wall. Both the collision boxes and the drawn mesh come from this, which is what
// keeps them the same shape.
static Vector3 ArcPoint(Vector3 arcCenter, Vector3 inward, Vector3 up, float radius, float angle)
{
    return Vector3Subtract(arcCenter, Vector3Add(Vector3Scale(up, radius * cosf(angle)),
                                                 Vector3Scale(inward, radius * sinf(angle))));
}

// The ramp is a quarter circle of radius R whose centre sits one radius inward
// and one radius up from the corner, so it runs tangent to the flat surface at
// one end and tangent to the wall at the other. Each segment is a box laid on
// the chord between two angles: the chord is a secant, so extending it slightly
// to overlap its neighbour buries the extension in the solid instead of leaving
// a lip in the driving surface.
//
// The boxes are the collision and are never drawn. What is drawn is one strip
// through the same arc — see RampMesh.
void ArenaObject::AddFillet(Vector3 corner, Vector3 inward, Vector3 up, Vector3 runAxis,
                            float radius, float runCenter, float runHalfLength, Color color,
                            int segments, float drawCenter, float drawHalfLength, float drawTaper)
{
    const float step = (PI * 0.5f) / (float)segments;
    const float chordHalf = radius * sinf(step * 0.5f);
    const float midRadius = radius * cosf(step * 0.5f);
    const Vector3 arcCenter = Vector3Add(corner, Vector3Scale(Vector3Add(inward, up), radius));

    for (int i = 0; i < segments; ++i)
    {
        // Angle at the middle of this segment: 0 lies on the flat surface, 90
        // degrees on the wall.
        const float angle = (i + 0.5f) * step;

        // Surface normal, and the direction the surface climbs in.
        Vector3 normal = Vector3Add(Vector3Scale(up, cosf(angle)), Vector3Scale(inward, sinf(angle)));
        Vector3 tangent = Vector3Subtract(Vector3Scale(up, sinf(angle)), Vector3Scale(inward, cosf(angle)));
        Vector3 side = Vector3CrossProduct(tangent, normal);

        // Back the box off along its own normal so the chord is its top face,
        // then slide it into place along the run.
        Vector3 center = Vector3Subtract(arcCenter, Vector3Scale(normal, midRadius + RAMP_THICKNESS * 0.5f));
        center = Vector3Add(center, Vector3Scale(runAxis, runCenter));

        Matrix basis = { tangent.x, normal.x, side.x, 0.0f,
                         tangent.y, normal.y, side.y, 0.0f,
                         tangent.z, normal.z, side.z, 0.0f,
                         0.0f,      0.0f,     0.0f,   1.0f };

        pieces.push_back({ center,
                           Vector3{ chordHalf + 0.02f, RAMP_THICKNESS * 0.5f, runHalfLength },
                           color, false, QuaternionFromMatrix(basis) });
    }

    // The drawn surface: one strip through the points where consecutive chords
    // meet, which are exactly the ends of the box top faces above.
    Mesh mesh = {};
    mesh.triangleCount = segments * 2;
    mesh.vertexCount = mesh.triangleCount * 3;
    mesh.vertices = (float *)MemAlloc((unsigned int)mesh.vertexCount * 3 * sizeof(float));

    // tangent x normal is constant along the arc and equals up x inward, which is
    // the run direction the winding has to agree with. It is not always runAxis:
    // the two side walls are passed the same runAxis but mirror each other, so
    // taking it from the basis is what keeps every ramp facing into the arena.
    const Vector3 runDirection = Vector3CrossProduct(up, inward);
    const Vector3 drawOffset = Vector3Scale(runAxis, drawCenter);
    int vertex = 0;

    for (int i = 0; i < segments; ++i)
    {
        const float angleLow = i * step;
        const float angleHigh = (i + 1) * step;
        // A ring of these runs around a corner is a torus, and a torus narrows as
        // it comes down off the wall: at the top each run wants the full chord,
        // at the toe it wants only drawTaper of it. Without this the runs overlap
        // near the toe — 2.7x over at the corner radii in use — which piles glass
        // on glass and smears the flat shading that makes the slope readable.
        const float halfLow = drawHalfLength * (drawTaper + (1.0f - drawTaper) * sinf(angleLow));
        const float halfHigh = drawHalfLength * (drawTaper + (1.0f - drawTaper) * sinf(angleHigh));

        Vector3 lower = Vector3Add(ArcPoint(arcCenter, inward, up, radius, angleLow), drawOffset);
        Vector3 upper = Vector3Add(ArcPoint(arcCenter, inward, up, radius, angleHigh), drawOffset);
        Vector3 lowBack = Vector3Scale(runDirection, -halfLow);
        Vector3 lowFront = Vector3Scale(runDirection, halfLow);
        Vector3 highBack = Vector3Scale(runDirection, -halfHigh);
        Vector3 highFront = Vector3Scale(runDirection, halfHigh);

        const Vector3 corners[6] = { Vector3Add(lower, lowBack),   Vector3Add(lower, lowFront),
                                     Vector3Add(upper, highBack),  Vector3Add(lower, lowFront),
                                     Vector3Add(upper, highFront), Vector3Add(upper, highBack) };
        for (int k = 0; k < 6; ++k, ++vertex)
        {
            mesh.vertices[vertex * 3 + 0] = corners[k].x;
            mesh.vertices[vertex * 3 + 1] = corners[k].y;
            mesh.vertices[vertex * 3 + 2] = corners[k].z;
        }
    }

    // LoadModelFromMesh does not upload, so this has to happen first.
    UploadMesh(&mesh, false);
    RampMesh ramp = { LoadModelFromMesh(mesh), color };
    lighting::Apply(ramp.model);
    rampMeshes.push_back(ramp);
}

// One rounded vertical corner. Three surfaces have to meet here: the quarter
// cylinder that replaces the 90 degree wall intersection, and the floor and
// ceiling ramps carried around it.
//
// The two ramps become a torus, approximated by a ring of short straight fillets
// laid on the chords of the corner arc — which is why AddFillet does the work
// again rather than anything new being written. All three share `cornerSegments`
// and the same angular division, so every chord midpoint lines up and the
// surfaces meet flush: at the top of the floor ramp the torus is exactly
// (cornerRadius - floorRampRadius) + floorRampRadius = cornerRadius from the
// axis, which is the cylinder.
//
// Nothing existing needs trimming. Everything the straight walls and ramps put
// in this region sits further from the corner axis than these surfaces do, so it
// ends up buried in solid and the rounded corner is what the car touches.
void ArenaObject::AddCorner(float sideX, float sideZ)
{
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const float axisX = sideX * (halfWidth - cornerRadius);
    const float axisZ = sideZ * (halfLength - cornerRadius);
    const float step = (PI * 0.5f) / (float)cornerSegments;
    const float chordHalf = cornerRadius * sinf(step * 0.5f);
    const float midRadius = cornerRadius * cosf(step * 0.5f);

    // The corner itself, over the stretch of wall that is neither ramp. Extended
    // past both ramps so the three always overlap into one solid.
    const float low = floorRampRadius - 0.1f;
    const float high = wallHeight - ceilingRampRadius + 0.1f;
    AddFillet(Vector3{ sideX * halfWidth, 0.0f, sideZ * halfLength },
              Vector3{ -sideX, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, -sideZ },
              Vector3{ 0.0f, 1.0f, 0.0f }, cornerRadius,
              (low + high) * 0.5f, (high - low) * 0.5f, WALL_COLOR, cornerSegments,
              (low + high) * 0.5f, (high - low) * 0.5f, 1.0f);

    for (int i = 0; i < cornerSegments; ++i)
    {
        const float angle = (i + 0.5f) * step;
        // Radial and tangential directions of the corner arc in the XZ plane. Both
        // stay unit length and perpendicular for either sign of sideX and sideZ.
        const Vector3 inward = { -sideX * cosf(angle), 0.0f, -sideZ * sinf(angle) };
        const Vector3 tangent = { -sideX * sinf(angle), 0.0f, sideZ * cosf(angle) };
        // Where this facet's wall meets the floor: the midpoint of its chord.
        const Vector3 mid = { axisX - midRadius * inward.x, 0.0f, axisZ - midRadius * inward.z };

        AddFillet(Vector3{ mid.x, 0.0f, mid.z }, inward, Vector3{ 0.0f, 1.0f, 0.0f },
                  tangent, floorRampRadius, 0.0f, chordHalf + 0.02f, RAMP_COLOR, rampSegments,
                  0.0f, chordHalf, (midRadius - floorRampRadius) / midRadius);
        AddFillet(Vector3{ mid.x, wallHeight, mid.z }, inward, Vector3{ 0.0f, -1.0f, 0.0f },
                  tangent, ceilingRampRadius, 0.0f, chordHalf + 0.02f, WALL_COLOR, rampSegments,
                  0.0f, chordHalf, (midRadius - ceilingRampRadius) / midRadius);
    }
}

// Everything outside the glass: three tiers of blocky stands down each side and
// behind each goal, and a ring of light rigs above the roof line. None of it is
// solid, so none of it can catch a car or a ball, and none of it is in the
// compound shape the camera's occlusion ray tests against either.
void ArenaObject::AddStadium()
{
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const Color STAND_COLOR = { 30, 38, 58, 255 };
    const Color STAND_TRIM = { 46, 60, 92, 255 };
    const Color LIGHT_COLOR = { 226, 240, 255, 255 };

    // Tiers step up and outward, so the stands read as raked seating rather than
    // as a wall standing next to a wall.
    for (int tier = 0; tier < 3; ++tier)
    {
        float out = 4.0f + tier * 5.0f;
        float top = 4.0f + tier * 4.5f;
        Color color = tier % 2 == 0 ? STAND_COLOR : STAND_TRIM;

        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            Piece bank = { { side * (halfWidth + out), top * 0.5f, 0.0f },
                           { 2.4f, top * 0.5f, halfLength + out }, color, true };
            bank.solid = false;
            pieces.push_back(bank);

            Piece end = { { 0.0f, top * 0.5f, side * (halfLength + out) },
                          { halfWidth + out, top * 0.5f, 2.4f }, color, true };
            end.solid = false;
            pieces.push_back(end);
        }
    }

    // Light rigs above the roof line, in from the stands so they hang over the
    // pitch. They are drawn unlit-bright rather than being real lights: the one
    // directional light in Lighting.cpp is still what shades the scene.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        for (int i = -2; i <= 2; ++i)
        {
            Piece rig = { { side * (halfWidth - 2.0f), wallHeight + 3.5f, i * (halfLength * 0.4f) },
                          { 1.6f, 0.35f, 3.2f }, LIGHT_COLOR, true };
            rig.solid = false;
            pieces.push_back(rig);
        }
    }
}

void ArenaObject::Initialize(Scene &owner)
{
    scene = &owner;

    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const float t = wallThickness;
    const float halfGoal = goalWidth * 0.5f;

    // Floor, with its top face at y = 0.
    pieces.push_back({ { 0.0f, -floorThickness * 0.5f, 0.0f },
                       { halfWidth, floorThickness * 0.5f, halfLength }, FLOOR_COLOR, true });

    // Every straight run stops where the rounded corner starts, and the corner
    // takes it from there. They used to overrun into each other, which was
    // harmless while it only meant buried solid — but glass never writes depth,
    // so each overlapping layer still blends and the corners lit up about two and
    // a half times brighter than the walls.
    const float cornerHalfLength = halfLength - cornerRadius;   // where the corner takes over
    const float cornerHalfWidth = halfWidth - cornerRadius;

    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth + t * 0.5f), wallHeight * 0.5f, 0.0f },
                           { t * 0.5f, wallHeight * 0.5f, halfLength + t }, WALL_COLOR, true });
    }

    // Back walls, in three pieces so a goal-sized hole is left in the middle.
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        const float wallZ = end * (halfLength + t * 0.5f);
        const float solidPanelHalf = (halfWidth - halfGoal) * 0.5f;

        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            pieces.push_back({ { side * (halfGoal + solidPanelHalf), wallHeight * 0.5f, wallZ },
                               { solidPanelHalf, wallHeight * 0.5f, t * 0.5f }, WALL_COLOR, true });
        }

        pieces.push_back({ { 0.0f, (goalHeight + wallHeight) * 0.5f, wallZ },
                           { halfGoal, (wallHeight - goalHeight) * 0.5f, t * 0.5f }, WALL_COLOR, true });
    }

    // Ceiling: collision only. Drawing it would put a slab between the chase
    // camera and the field every time the car climbs a wall.
    pieces.push_back({ { 0.0f, wallHeight + t * 0.5f, 0.0f },
                       { halfWidth + t, t * 0.5f, halfLength + t }, WALL_COLOR, false });

    // Ramps from the floor up into each side wall, running the whole length.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        AddFillet(Vector3{ side * halfWidth, 0.0f, 0.0f }, Vector3{ -side, 0.0f, 0.0f },
                  Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f },
                  floorRampRadius, 0.0f, halfLength + t, RAMP_COLOR, rampSegments, 0.0f, cornerHalfLength, 1.0f);
    }

    // Ramps from the floor up into each back wall, in two runs so the goal mouth
    // is left flat — a ramp across it would wall the goal off.
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        const float panelHalfWidth = (cornerHalfWidth - halfGoal) * 0.5f;
        const float solidPanelHalf = (halfWidth - halfGoal) * 0.5f;
        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            AddFillet(Vector3{ 0.0f, 0.0f, end * halfLength }, Vector3{ 0.0f, 0.0f, -end },
                      Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f },
                      floorRampRadius, side * (halfGoal + solidPanelHalf), solidPanelHalf, RAMP_COLOR,
                      rampSegments, side * (halfGoal + panelHalfWidth), panelHalfWidth, 1.0f);
        }
    }

    // The same treatment where the walls meet the ceiling, so a car that carries
    // enough speed up a wall rolls onto the roof instead of hitting a hard edge.
    // Glass, like the walls they continue: the camera has to see through them.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        AddFillet(Vector3{ side * halfWidth, wallHeight, 0.0f }, Vector3{ -side, 0.0f, 0.0f },
                  Vector3{ 0.0f, -1.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f },
                  ceilingRampRadius, 0.0f, halfLength + t, WALL_COLOR, rampSegments, 0.0f, cornerHalfLength, 1.0f);
    }
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        AddFillet(Vector3{ 0.0f, wallHeight, end * halfLength }, Vector3{ 0.0f, 0.0f, -end },
                  Vector3{ 0.0f, -1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f },
                  ceilingRampRadius, 0.0f, halfWidth + t, WALL_COLOR, rampSegments, 0.0f, cornerHalfWidth, 1.0f);
    }

    // Round off the four vertical wall intersections, so a car can carry a wall
    // straight through a corner onto the next one.
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
            AddCorner(sideX, sideZ);
    }

    AddStadium();

    JPH::StaticCompoundShapeSettings compound;
    compound.SetEmbedded();
    for (const Piece &piece : pieces)
    {
        if (!piece.solid)
            continue;

        JPH::BoxShapeSettings boxSettings(JPH::Vec3(piece.halfExtents.x, piece.halfExtents.y,
                                                    piece.halfExtents.z));
        boxSettings.SetEmbedded();
        compound.AddShape(JPH::Vec3(piece.center.x, piece.center.y, piece.center.z),
                          JPH::Quat(piece.rotation.x, piece.rotation.y, piece.rotation.z,
                                    piece.rotation.w).Normalized(),
                          boxSettings.Create().Get());
    }

    JPH::BodyCreationSettings settings(compound.Create().Get(), JPH::RVec3::sZero(),
                                       JPH::Quat::sIdentity(), JPH::EMotionType::Static, physics::Arena);
    settings.mFriction = 0.9f;
    settings.mRestitution = 0.2f;

    bodyID = scene->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::DontActivate);

    boxModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    lighting::Apply(boxModel);
    boxModelLoaded = true;
}

void ArenaObject::Draw()
{
    for (const Piece &piece : pieces)
    {
        if (!piece.visible || piece.color.a < 255)
            continue; // glass is drawn last, by DrawGlassWalls

        Vector3 axis;
        float angle;
        QuaternionToAxisAngle(piece.rotation, &axis, &angle);
        DrawModelEx(boxModel, piece.center, axis, angle * RAD2DEG,
                    Vector3Scale(piece.halfExtents, 2.0f), piece.color);
    }

    // Same rule as the pieces: alpha decides which pass a ramp belongs to.
    for (const RampMesh &ramp : rampMeshes)
    {
        if (ramp.color.a < 255)
            continue;
        DrawModel(ramp.model, Vector3Zero(), 1.0f, ramp.color);
    }

    // Field markings, drawn as unlit lines just above the floor. Explicit lines
    // rather than DrawGrid, which is square and cannot match this rectangular field.
    //
    // The flat floor is a rounded rectangle: the ramps take the outer 5 m, and the
    // corners are arcs of (cornerRadius - floorRampRadius) about each corner axis.
    // Every marking is clipped to that, since past it they would be buried in a
    // ramp — which is also why the outline traces the arcs rather than squaring
    // them off.
    const float halfWidth = FlatHalfWidth();
    const float halfLength = FlatHalfLength();
    const float cornerAxisX = width * 0.5f - cornerRadius;
    const float cornerAxisZ = length * 0.5f - cornerRadius;
    const float flatCorner = cornerRadius - floorRampRadius;
    const Color gridColor = { 90, 190, 255, 45 };
    const Color lineColor = { 90, 190, 255, 190 };

    // How far a grid line may run before it reaches the rounded corner.
    for (float x = -halfWidth; x <= halfWidth + 0.01f; x += 5.0f)
    {
        float over = fabsf(x) - cornerAxisX;
        float limit = over <= 0.0f ? halfLength
                    : cornerAxisZ + sqrtf(fmaxf(flatCorner * flatCorner - over * over, 0.0f));
        DrawLine3D(Vector3{ x, 0.02f, -limit }, Vector3{ x, 0.02f, limit }, gridColor);
    }
    for (float z = -halfLength; z <= halfLength + 0.01f; z += 5.0f)
    {
        float over = fabsf(z) - cornerAxisZ;
        float limit = over <= 0.0f ? halfWidth
                    : cornerAxisX + sqrtf(fmaxf(flatCorner * flatCorner - over * over, 0.0f));
        DrawLine3D(Vector3{ -limit, 0.02f, z }, Vector3{ limit, 0.02f, z }, gridColor);
    }

    DrawCircle3D(Vector3{ 0.0f, 0.02f, 0.0f }, 8.0f, Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f, lineColor);
    DrawLine3D(Vector3{ -halfWidth, 0.03f, 0.0f }, Vector3{ halfWidth, 0.03f, 0.0f }, lineColor);

    // Pitch outline: four straights between the corner arcs, then the arcs.
    const Color outline = { 90, 190, 255, 120 };
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        DrawLine3D(Vector3{ side * halfWidth, 0.02f, -cornerAxisZ },
                   Vector3{ side * halfWidth, 0.02f, cornerAxisZ }, outline);
        DrawLine3D(Vector3{ -cornerAxisX, 0.02f, side * halfLength },
                   Vector3{ cornerAxisX, 0.02f, side * halfLength }, outline);
    }
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            for (int i = 0; i < cornerSegments; ++i)
            {
                float a = (PI * 0.5f) * i / (float)cornerSegments;
                float b = (PI * 0.5f) * (i + 1) / (float)cornerSegments;
                DrawLine3D(Vector3{ sideX * (cornerAxisX + flatCorner * cosf(a)), 0.02f,
                                    sideZ * (cornerAxisZ + flatCorner * sinf(a)) },
                           Vector3{ sideX * (cornerAxisX + flatCorner * cosf(b)), 0.02f,
                                    sideZ * (cornerAxisZ + flatCorner * sinf(b)) }, outline);
            }
        }
    }
}

void ArenaObject::DrawGlassWalls()
{
    // Depth writing off, depth testing still on. Without this the walls would
    // stamp themselves into the depth buffer and every object further away —
    // which is exactly the car, whenever the camera slips outside the arena —
    // would be rejected before it ever got the chance to blend through.
    rlDisableDepthMask();

    for (const Piece &piece : pieces)
    {
        if (!piece.visible || piece.color.a >= 255)
            continue;

        Vector3 axis;
        float angle;
        QuaternionToAxisAngle(piece.rotation, &axis, &angle);
        DrawModelEx(boxModel, piece.center, axis, angle * RAD2DEG,
                    Vector3Scale(piece.halfExtents, 2.0f), piece.color);
    }

    for (const RampMesh &ramp : rampMeshes)
    {
        if (ramp.color.a >= 255)
            continue;
        DrawModel(ramp.model, Vector3Zero(), 1.0f, ramp.color);
    }

    rlEnableDepthMask();

    // Where the ceiling ramps finish and the flat roof begins. Glass never writes
    // depth, so a line buried in solid still shows through it — which is why this
    // traces the real edge of the roof rather than the old square top of the wall,
    // and why the vertical posts at the old sharp corners are gone with them.
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const float cornerAxisX = halfWidth - cornerRadius;
    const float cornerAxisZ = halfLength - cornerRadius;
    const float roofX = halfWidth - ceilingRampRadius;
    const float roofZ = halfLength - ceilingRampRadius;
    const float roofCorner = cornerRadius - ceilingRampRadius;
    const Color edge = { 110, 160, 220, 150 };

    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        DrawLine3D(Vector3{ side * roofX, wallHeight, -cornerAxisZ },
                   Vector3{ side * roofX, wallHeight, cornerAxisZ }, edge);
        DrawLine3D(Vector3{ -cornerAxisX, wallHeight, side * roofZ },
                   Vector3{ cornerAxisX, wallHeight, side * roofZ }, edge);
    }
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            for (int i = 0; i < cornerSegments; ++i)
            {
                float a = (PI * 0.5f) * i / (float)cornerSegments;
                float b = (PI * 0.5f) * (i + 1) / (float)cornerSegments;
                DrawLine3D(Vector3{ sideX * (cornerAxisX + roofCorner * cosf(a)), wallHeight,
                                    sideZ * (cornerAxisZ + roofCorner * sinf(a)) },
                           Vector3{ sideX * (cornerAxisX + roofCorner * cosf(b)), wallHeight,
                                    sideZ * (cornerAxisZ + roofCorner * sinf(b)) }, edge);
            }
        }
    }

    // Glass on its own is almost invisible, so without these the edge of the
    // arena reads as empty space. The seam is the line where ramp becomes wall,
    // and the mullions give the panels enough structure to read as surfaces
    // without making them any less see-through — which the chase camera depends
    // on, and which is now the only thing telling the player where the ramp is.
    const float halfGoal = goalWidth * 0.5f;
    const float rampTop = floorRampRadius;                 // where the ramp becomes wall
    const float wallTop = wallHeight - ceilingRampRadius;   // where the wall becomes ceiling
    const float inset = 0.03f;                              // just inside the panel, so no z-fighting
    const Color seam = { 130, 195, 250, 215 };
    const Color mullion = { 110, 160, 220, 60 };

    // The straights stop where the rounded corners begin; the arcs below carry
    // them round. Past that tangent point a straight line is buried in the corner.
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        const float x = sideX * (halfWidth - inset);
        DrawLine3D(Vector3{ x, rampTop, -cornerAxisZ }, Vector3{ x, rampTop, cornerAxisZ }, seam);
        DrawLine3D(Vector3{ x, wallTop, -cornerAxisZ }, Vector3{ x, wallTop, cornerAxisZ }, mullion);
        for (float z = -cornerAxisZ; z <= cornerAxisZ + 0.01f; z += 5.0f)
            DrawLine3D(Vector3{ x, rampTop, z }, Vector3{ x, wallTop, z }, mullion);
    }

    for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
    {
        const float z = sideZ * (halfLength - inset);
        // The back-wall ramps stop either side of the goal mouth, so the seam does
        // too — and the mouth is exactly rampTop high, so no mullion crosses it.
        for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
        {
            DrawLine3D(Vector3{ sideX * halfGoal, rampTop, z },
                       Vector3{ sideX * cornerAxisX, rampTop, z }, seam);
        }
        DrawLine3D(Vector3{ -cornerAxisX, wallTop, z }, Vector3{ cornerAxisX, wallTop, z }, mullion);
        for (float x = -cornerAxisX; x <= cornerAxisX + 0.01f; x += 5.0f)
            DrawLine3D(Vector3{ x, rampTop, z }, Vector3{ x, wallTop, z }, mullion);
    }

    // Round the corners off with the same two lines plus a mullion per facet, so
    // the seam runs unbroken from one wall to the next the way the surface does.
    const float cornerLine = cornerRadius - inset;
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            for (int i = 0; i <= cornerSegments; ++i)
            {
                float a = (PI * 0.5f) * i / (float)cornerSegments;
                Vector3 at = { sideX * (cornerAxisX + cornerLine * cosf(a)), 0.0f,
                               sideZ * (cornerAxisZ + cornerLine * sinf(a)) };
                DrawLine3D(Vector3{ at.x, rampTop, at.z }, Vector3{ at.x, wallTop, at.z }, mullion);
                if (i == cornerSegments)
                    continue;

                float b = (PI * 0.5f) * (i + 1) / (float)cornerSegments;
                Vector3 next = { sideX * (cornerAxisX + cornerLine * cosf(b)), 0.0f,
                                 sideZ * (cornerAxisZ + cornerLine * sinf(b)) };
                DrawLine3D(Vector3{ at.x, rampTop, at.z }, Vector3{ next.x, rampTop, next.z }, seam);
                DrawLine3D(Vector3{ at.x, wallTop, at.z }, Vector3{ next.x, wallTop, next.z }, mullion);
            }
        }
    }
}

BoundingBox ArenaObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ -width * 0.5f - wallThickness, -floorThickness, -length * 0.5f - wallThickness },
        Vector3{ width * 0.5f + wallThickness, wallHeight + wallThickness, length * 0.5f + wallThickness }
    };
}

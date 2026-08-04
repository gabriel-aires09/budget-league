#include "ArenaObject.h"

#include "Lighting.h"
#include "Scene.h"

#include <raymath.h>
#include <rlgl.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <cmath>

static const Color FLOOR_COLOR = { 34, 74, 45, 255 };
// Glass, not masonry: the chase camera regularly ends up outside the arena, and
// a solid wall then hides the car completely. The alpha is what puts these in
// the late, depth-write-free pass (see DrawGlassWalls).
static const Color WALL_COLOR = { 126, 136, 148, 62 };
// The floor-to-wall ramp is neutral gray and lightly transparent. Its generated
// mesh adds a subtle lightening gradient towards the wall; alpha here still
// identifies it as part of the late transparent pass.
static const Color RAMP_COLOR = { 138, 144, 150, 150 };
static const Color RAMP_COLOR_LOW = { 112, 118, 124, 130 };
static const Color RAMP_COLOR_HIGH = { 168, 174, 180, 175 };

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
    const bool floorRamp = color.r == RAMP_COLOR.r && color.g == RAMP_COLOR.g
                        && color.b == RAMP_COLOR.b && color.a == RAMP_COLOR.a;
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
    if (floorRamp)
        mesh.colors = (unsigned char *)MemAlloc((unsigned int)mesh.vertexCount * 4 * sizeof(unsigned char));

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

            if (mesh.colors != nullptr)
            {
                // Vertices 2, 4 and 5 sit on the upper edge of the quad. The
                // GPU interpolates these endpoint colors across each triangle,
                // producing one continuous gradient along the curved ramp.
                const bool upperVertex = k == 2 || k == 4 || k == 5;
                const float gradient = (i + (upperVertex ? 1.0f : 0.0f)) / (float)segments;
                const Color vertexColor = ColorLerp(RAMP_COLOR_LOW, RAMP_COLOR_HIGH, gradient);
                mesh.colors[vertex * 4 + 0] = vertexColor.r;
                mesh.colors[vertex * 4 + 1] = vertexColor.g;
                mesh.colors[vertex * 4 + 2] = vertexColor.b;
                mesh.colors[vertex * 4 + 3] = vertexColor.a;
            }
        }
    }

    // LoadModelFromMesh does not upload, so this has to happen first.
    UploadMesh(&mesh, false);
    // A white tint preserves the authored vertex gradient. Alpha 254 keeps the
    // ramp classified for the transparent pass without visibly changing it.
    RampMesh ramp = { LoadModelFromMesh(mesh), floorRamp ? Color{ 255, 255, 255, 254 } : color };
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

    // A deterministic triangle mosaic over the flat collider gives the pitch
    // its low-poly grass look without adding texture assets or changing physics.
    // Every triangle is one solid green, so the facets stay crisp at any camera
    // distance instead of blurring like a stretched image.
    const Color grassColors[] = {
        { 42, 100, 55, 255 }, { 49, 112, 59, 255 }, { 55, 124, 65, 255 },
        { 38, 91, 51, 255 },  { 62, 132, 69, 255 }, { 46, 106, 62, 255 }
    };
    const float grassTile = 6.0f;
    const float floorHalfWidth = width * 0.5f;
    const float floorHalfLength = length * 0.5f;
    int tileZ = 0;
    for (float z = -floorHalfLength; z < floorHalfLength; z += grassTile, ++tileZ)
    {
        float nextZ = fminf(z + grassTile, floorHalfLength);
        int tileX = 0;
        for (float x = -floorHalfWidth; x < floorHalfWidth; x += grassTile, ++tileX)
        {
            float nextX = fminf(x + grassTile, floorHalfWidth);
            unsigned int hash = (unsigned int)(tileX * 73856093) ^ (unsigned int)(tileZ * 19349663);
            Color first = grassColors[hash % 6];
            Color second = grassColors[(hash / 7 + 3) % 6];
            const float y = 0.008f;

            if (((tileX + tileZ) & 1) == 0)
            {
                DrawTriangle3D(Vector3{ x, y, z }, Vector3{ nextX, y, nextZ },
                               Vector3{ nextX, y, z }, first);
                DrawTriangle3D(Vector3{ x, y, z }, Vector3{ x, y, nextZ },
                               Vector3{ nextX, y, nextZ }, second);
            }
            else
            {
                DrawTriangle3D(Vector3{ x, y, z }, Vector3{ x, y, nextZ },
                               Vector3{ nextX, y, z }, first);
                DrawTriangle3D(Vector3{ nextX, y, z }, Vector3{ x, y, nextZ },
                               Vector3{ nextX, y, nextZ }, second);
            }
        }
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
    const Color gridColor = { 255, 255, 255, 32 };
    const Color lineColor = { 255, 255, 255, 255 };

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

    // rlgl batches the line vertices but line width is immediate GL state, so
    // flush before changing it or every marking would use the later reset width.
    rlDrawRenderBatchActive();
    rlSetLineWidth(4.0f);
    DrawCircle3D(Vector3{ 0.0f, 0.02f, 0.0f }, 8.0f, Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f, lineColor);
    DrawLine3D(Vector3{ -halfWidth, 0.03f, 0.0f }, Vector3{ halfWidth, 0.03f, 0.0f }, lineColor);

    // Pitch outline: four straights between the corner arcs, then the arcs.
    const Color outline = { 255, 255, 255, 220 };
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
    rlDrawRenderBatchActive();
    rlSetLineWidth(1.0f);
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
    const Color edge = { 255, 255, 255, 235 };

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
    const float rampTop = floorRampRadius;                 // where the ramp becomes wall
    const float wallTop = wallHeight - ceilingRampRadius;   // where the wall becomes ceiling
    // Keep the linework clearly in front of the glass surface. At the old 3 cm
    // offset, depth precision at the far blue wall could reject an isolated
    // diagonal and leave an apparently missing edge in the diamond pattern.
    const float inset = 0.12f;
    const Color seam = { 255, 255, 255, 245 };
    const Color metalFacet = { 210, 216, 222, 180 };
    const float latticeWidth = 7.0f;
    const float latticeHeight = 3.6f;

    // The reference is an open diamond mesh, not diagonal bracing over solid
    // panels. Staggered nodes connect to both neighbours on the next row to
    // form the repeating low-poly metal cells. Thicker lines read as bars.
    rlDrawRenderBatchActive();
    rlSetLineWidth(3.0f);

    // The straights stop where the rounded corners begin; the arcs below carry
    // them round. Past that tangent point a straight line is buried in the corner.
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        const float x = sideX * (halfWidth - inset);
        const int runSegments = (int)ceilf(cornerAxisZ * 2.0f / latticeWidth);
        const float runSpacing = cornerAxisZ * 2.0f / (float)runSegments;
        DrawLine3D(Vector3{ x, rampTop, -cornerAxisZ }, Vector3{ x, rampTop, cornerAxisZ }, seam);
        DrawLine3D(Vector3{ x, wallTop, -cornerAxisZ }, Vector3{ x, wallTop, cornerAxisZ }, seam);
        int row = 0;
        for (float y = rampTop; y < wallTop - 0.01f; y += latticeHeight, ++row)
        {
            float nextY = fminf(y + latticeHeight, wallTop);
            float offset = (row & 1) ? runSpacing * 0.5f : 0.0f;
            for (float z = -cornerAxisZ + offset; z <= cornerAxisZ + 0.01f; z += runSpacing)
            {
                for (float direction = -1.0f; direction <= 1.0f; direction += 2.0f)
                {
                    float nextZ = z + direction * runSpacing * 0.5f;
                    if (nextZ >= -cornerAxisZ && nextZ <= cornerAxisZ)
                        DrawLine3D(Vector3{ x, y, z }, Vector3{ x, nextY, nextZ }, metalFacet);
                }
            }
        }
    }

    // Build the orange end first and mirror each exact segment onto the blue
    // end. Using integer node indices avoids accumulated float comparisons at
    // the boundary and guarantees that neither side can lose a diagonal the
    // other side has.
    const float endWallZ = halfLength - inset;
    const int endSegments = (int)ceilf(cornerAxisX * 2.0f / latticeWidth);
    const float endSpacing = cornerAxisX * 2.0f / (float)endSegments;
    auto DrawEndSegment = [endWallZ, metalFacet](Vector3 from, Vector3 to)
    {
        from.z = -endWallZ;
        to.z = -endWallZ;
        DrawLine3D(from, to, metalFacet);
        from.z = endWallZ;
        to.z = endWallZ;
        DrawLine3D(from, to, metalFacet);
    };

    for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
    {
        const float z = sideZ * endWallZ;
        DrawLine3D(Vector3{ -cornerAxisX, rampTop, z }, Vector3{ cornerAxisX, rampTop, z }, seam);
        DrawLine3D(Vector3{ -cornerAxisX, wallTop, z }, Vector3{ cornerAxisX, wallTop, z }, seam);
    }

    int endRow = 0;
    for (float y = rampTop; y < wallTop - 0.01f; y += latticeHeight, ++endRow)
    {
        const float nextY = fminf(y + latticeHeight, wallTop);
        const bool offsetRow = (endRow & 1) != 0;
        const int nodeCount = offsetRow ? endSegments : endSegments + 1;
        for (int node = 0; node < nodeCount; ++node)
        {
            const float x = -cornerAxisX + (node + (offsetRow ? 0.5f : 0.0f)) * endSpacing;
            for (int direction = -1; direction <= 1; direction += 2)
            {
                const float nextX = x + direction * endSpacing * 0.5f;
                if (nextX < -cornerAxisX - 0.001f || nextX > cornerAxisX + 0.001f)
                    continue;
                DrawEndSegment(Vector3{ x, y, 0.0f }, Vector3{ nextX, nextY, 0.0f });
            }
        }
    }

    // Carry the same diamond lattice around the rounded corners. The collision
    // has ten fine facets, but using every facet as a visual column compressed
    // the grid at each transition. Visual nodes are instead spaced by arc
    // length, matching latticeWidth on the straight walls.
    const float cornerLine = cornerRadius - inset;
    const int latticeCornerSegments = (int)ceilf(cornerLine * PI * 0.5f / latticeWidth);
    const float latticeCornerStep = PI * 0.5f / (float)latticeCornerSegments;
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            int row = 0;
            for (float y = rampTop; y < wallTop - 0.01f; y += latticeHeight, ++row)
            {
                float nextY = fminf(y + latticeHeight, wallTop);
                float offset = (row & 1) ? latticeCornerStep * 0.5f : 0.0f;
                for (float angle = offset; angle <= PI * 0.5f + 0.001f; angle += latticeCornerStep)
                {
                    Vector3 at = { sideX * (cornerAxisX + cornerLine * cosf(angle)), y,
                                   sideZ * (cornerAxisZ + cornerLine * sinf(angle)) };
                    for (float direction = -1.0f; direction <= 1.0f; direction += 2.0f)
                    {
                        float nextAngle = angle + direction * latticeCornerStep * 0.5f;
                        if (nextAngle < 0.0f || nextAngle > PI * 0.5f)
                            continue;
                        Vector3 next = { sideX * (cornerAxisX + cornerLine * cosf(nextAngle)), nextY,
                                         sideZ * (cornerAxisZ + cornerLine * sinf(nextAngle)) };
                        DrawLine3D(at, next, metalFacet);
                    }
                }
            }
        }
    }

    // White bands continue both wall/ramp joins around the rounded corners.
    // These use the collision facet division so their endpoints coincide with
    // the ramp strip seams and meet the straight runs without a visual gap.
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            for (int i = 0; i < cornerSegments; ++i)
            {
                float a = PI * 0.5f * i / (float)cornerSegments;
                float b = PI * 0.5f * (i + 1) / (float)cornerSegments;
                Vector3 at = { sideX * (cornerAxisX + cornerLine * cosf(a)), 0.0f,
                               sideZ * (cornerAxisZ + cornerLine * sinf(a)) };
                Vector3 next = { sideX * (cornerAxisX + cornerLine * cosf(b)), 0.0f,
                                 sideZ * (cornerAxisZ + cornerLine * sinf(b)) };
                DrawLine3D(Vector3{ at.x, rampTop, at.z }, Vector3{ next.x, rampTop, next.z }, seam);
                DrawLine3D(Vector3{ at.x, wallTop, at.z }, Vector3{ next.x, wallTop, next.z }, seam);
            }
        }
    }

    // The ceiling collision stays invisible, but an open diamond lattice makes
    // it part of the same metal cage as the walls and upper ramps without
    // placing an opaque slab between the camera and the field.
    const float ceilingY = wallHeight - inset;
    const float ceilingRow = 5.0f;
    int row = 0;
    for (float z = -roofZ; z < roofZ - 0.01f; z += ceilingRow, ++row)
    {
        float nextZ = fminf(z + ceilingRow, roofZ);
        float offset = (row & 1) ? latticeWidth * 0.5f : 0.0f;
        for (float x = -roofX + offset; x <= roofX + 0.01f; x += latticeWidth)
        {
            for (float direction = -1.0f; direction <= 1.0f; direction += 2.0f)
            {
                float nextX = x + direction * latticeWidth * 0.5f;
                if (nextX >= -roofX && nextX <= roofX)
                    DrawLine3D(Vector3{ x, ceilingY, z }, Vector3{ nextX, ceilingY, nextZ }, metalFacet);
            }
        }
    }
    rlDrawRenderBatchActive();
    rlSetLineWidth(1.0f);
}

BoundingBox ArenaObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ -width * 0.5f - wallThickness, -floorThickness, -length * 0.5f - wallThickness },
        Vector3{ width * 0.5f + wallThickness, wallHeight + wallThickness, length * 0.5f + wallThickness }
    };
}

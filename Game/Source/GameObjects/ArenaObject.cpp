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
                            float radius, float runCenter, float runHalfLength, Color color)
{
    const float step = (PI * 0.5f) / (float)rampSegments;
    const float chordHalf = radius * sinf(step * 0.5f);
    const float midRadius = radius * cosf(step * 0.5f);
    const Vector3 arcCenter = Vector3Add(corner, Vector3Scale(Vector3Add(inward, up), radius));

    for (int i = 0; i < rampSegments; ++i)
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
    mesh.triangleCount = rampSegments * 2;
    mesh.vertexCount = mesh.triangleCount * 3;
    mesh.vertices = (float *)MemAlloc((unsigned int)mesh.vertexCount * 3 * sizeof(float));

    // tangent x normal is constant along the arc and equals up x inward, which is
    // the run direction the winding has to agree with. It is not always runAxis:
    // the two side walls are passed the same runAxis but mirror each other, so
    // taking it from the basis is what keeps every ramp facing into the arena.
    const Vector3 runDirection = Vector3CrossProduct(up, inward);
    const Vector3 runOffset = Vector3Scale(runAxis, runCenter);
    int vertex = 0;

    for (int i = 0; i < rampSegments; ++i)
    {
        Vector3 lower = Vector3Add(ArcPoint(arcCenter, inward, up, radius, i * step), runOffset);
        Vector3 upper = Vector3Add(ArcPoint(arcCenter, inward, up, radius, (i + 1) * step), runOffset);
        Vector3 back = Vector3Scale(runDirection, -runHalfLength);
        Vector3 front = Vector3Scale(runDirection, runHalfLength);

        const Vector3 corners[6] = { Vector3Add(lower, back),  Vector3Add(lower, front),
                                     Vector3Add(upper, back),  Vector3Add(lower, front),
                                     Vector3Add(upper, front), Vector3Add(upper, back) };
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

    // Side walls, overlapping the corners so there is no seam to catch on.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        pieces.push_back({ { side * (halfWidth + t * 0.5f), wallHeight * 0.5f, 0.0f },
                           { t * 0.5f, wallHeight * 0.5f, halfLength + t }, WALL_COLOR, true });
    }

    // Back walls, in three pieces so a goal-sized hole is left in the middle.
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        const float wallZ = end * (halfLength + t * 0.5f);
        const float panelHalfWidth = (halfWidth - halfGoal) * 0.5f;

        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            pieces.push_back({ { side * (halfGoal + panelHalfWidth), wallHeight * 0.5f, wallZ },
                               { panelHalfWidth, wallHeight * 0.5f, t * 0.5f }, WALL_COLOR, true });
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
                  floorRampRadius, 0.0f, halfLength + t, RAMP_COLOR);
    }

    // Ramps from the floor up into each back wall, in two runs so the goal mouth
    // is left flat — a ramp across it would wall the goal off.
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        const float panelHalfWidth = (halfWidth - halfGoal) * 0.5f;
        for (float side = -1.0f; side <= 1.0f; side += 2.0f)
        {
            AddFillet(Vector3{ 0.0f, 0.0f, end * halfLength }, Vector3{ 0.0f, 0.0f, -end },
                      Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f },
                      floorRampRadius, side * (halfGoal + panelHalfWidth), panelHalfWidth, RAMP_COLOR);
        }
    }

    // The same treatment where the walls meet the ceiling, so a car that carries
    // enough speed up a wall rolls onto the roof instead of hitting a hard edge.
    // Glass, like the walls they continue: the camera has to see through them.
    for (float side = -1.0f; side <= 1.0f; side += 2.0f)
    {
        AddFillet(Vector3{ side * halfWidth, wallHeight, 0.0f }, Vector3{ -side, 0.0f, 0.0f },
                  Vector3{ 0.0f, -1.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f },
                  ceilingRampRadius, 0.0f, halfLength + t, WALL_COLOR);
    }
    for (float end = -1.0f; end <= 1.0f; end += 2.0f)
    {
        AddFillet(Vector3{ 0.0f, wallHeight, end * halfLength }, Vector3{ 0.0f, 0.0f, -end },
                  Vector3{ 0.0f, -1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f },
                  ceilingRampRadius, 0.0f, halfWidth + t, WALL_COLOR);
    }

    JPH::StaticCompoundShapeSettings compound;
    compound.SetEmbedded();
    for (const Piece &piece : pieces)
    {
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
    // rather than DrawGrid, which is square and cannot match a 55 x 80 field.
    // They stop where the floor stops being flat, since past that they would be
    // buried inside the ramps.
    const float halfWidth = FlatHalfWidth();
    const float halfLength = FlatHalfLength();
    const Color gridColor = { 90, 190, 255, 45 };
    for (float x = -halfWidth; x <= halfWidth + 0.01f; x += 5.0f)
        DrawLine3D(Vector3{ x, 0.02f, -halfLength }, Vector3{ x, 0.02f, halfLength }, gridColor);
    for (float z = -halfLength; z <= halfLength + 0.01f; z += 5.0f)
        DrawLine3D(Vector3{ -halfWidth, 0.02f, z }, Vector3{ halfWidth, 0.02f, z }, gridColor);

    DrawCircle3D(Vector3{ 0.0f, 0.02f, 0.0f }, 8.0f, Vector3{ 1.0f, 0.0f, 0.0f }, 90.0f,
                 Color{ 90, 190, 255, 190 });
    DrawCubeWires(Vector3{ 0.0f, 0.02f, 0.0f }, halfWidth * 2.0f, 0.04f, halfLength * 2.0f,
                  Color{ 90, 190, 255, 120 });
    DrawLine3D(Vector3{ -halfWidth, 0.03f, 0.0f }, Vector3{ halfWidth, 0.03f, 0.0f },
               Color{ 90, 190, 255, 190 });
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

    // Edges kept solid, so the boundary still reads even though the panels are
    // nearly see-through.
    const float halfWidth = width * 0.5f;
    const float halfLength = length * 0.5f;
    const Color edge = { 110, 160, 220, 150 };
    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        DrawLine3D(Vector3{ sideX * halfWidth, wallHeight, -halfLength },
                   Vector3{ sideX * halfWidth, wallHeight, halfLength }, edge);
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            DrawLine3D(Vector3{ sideX * halfWidth, 0.0f, sideZ * halfLength },
                       Vector3{ sideX * halfWidth, wallHeight, sideZ * halfLength }, edge);
            DrawLine3D(Vector3{ -halfWidth, wallHeight, sideZ * halfLength },
                       Vector3{ halfWidth, wallHeight, sideZ * halfLength }, edge);
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

    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        const float x = sideX * (halfWidth - inset);
        DrawLine3D(Vector3{ x, rampTop, -halfLength }, Vector3{ x, rampTop, halfLength }, seam);
        DrawLine3D(Vector3{ x, wallTop, -halfLength }, Vector3{ x, wallTop, halfLength }, mullion);
        for (float z = -halfLength; z <= halfLength + 0.01f; z += 5.0f)
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
                       Vector3{ sideX * halfWidth, rampTop, z }, seam);
        }
        DrawLine3D(Vector3{ -halfWidth, wallTop, z }, Vector3{ halfWidth, wallTop, z }, mullion);
        for (float x = -halfWidth; x <= halfWidth + 0.01f; x += 5.0f)
            DrawLine3D(Vector3{ x, rampTop, z }, Vector3{ x, wallTop, z }, mullion);
    }
}

BoundingBox ArenaObject::GetWorldBounds() const
{
    return BoundingBox{
        Vector3{ -width * 0.5f - wallThickness, -floorThickness, -length * 0.5f - wallThickness },
        Vector3{ width * 0.5f + wallThickness, wallHeight + wallThickness, length * 0.5f + wallThickness }
    };
}

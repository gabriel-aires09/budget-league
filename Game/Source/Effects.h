#ifndef EFFECTS_H
#define EFFECTS_H

#include <raylib.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

class Scene;

// Visual feedback: the boost flame and trail, contact shadows and the particle
// bursts fired on goals, big hits and jumps.
//
// It is deliberately a dumb renderer with one particle pool. It decides nothing:
// MatchScene watches the match and the ball and calls Burst on the events it
// sees, exactly as it does for the HUD.
namespace effects
{
    // Needs a window. Builds the shared meshes; the shadow disc and the flame
    // cone are unlit on purpose, so they read as light rather than as geometry.
    void Load();
    void Unload();

    void Clear(); // drop every live particle, for a kickoff reset
    void Update(float deltaTime);
    void Draw(); // the particles, inside BeginMode3D

    // direction is the middle of the cone the particles are thrown into; spread
    // of 1 is a full sphere.
    void Burst(Vector3 position, Vector3 direction, int count, float speed, float spread,
               Color color, float size, float life);

    // Behind a boosting car. strength scales the flame and how much trail it
    // leaves, so a car that just tapped boost does not look like one holding it.
    void DrawBoostFlame(Vector3 position, Matrix rotation, float carHalfLength, float strength);

    // A dark disc on whatever surface is under the object, laid on that surface's
    // normal so it follows the ramps. Fades out with height.
    void DrawContactShadow(Scene &scene, Vector3 position, float radius, JPH::BodyID ignore);
}

#endif

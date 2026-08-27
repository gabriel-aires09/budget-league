#ifndef CHASECAMERA_H
#define CHASECAMERA_H

#include <raylib.h>

class CarObject;

// Third person camera that trails the car, smoothing both its own position and
// the point it looks at, with a ball cam on C that frames the ball as well.
// It also keeps itself inside the arena: the ceiling and floor are clamps, and
// everything in between is a ray cast against the real collision geometry.
class ChaseCamera
{
public:
    void Initialize(Camera3D &camera, const CarObject &car);
    // The ball position is passed every frame, not only in ball cam, because the
    // mode can be toggled at any moment.
    void Update(Camera3D &camera, const CarObject &car, Vector3 ballPosition, float deltaTime);

    bool ballCam = false; // toggled with C

    // GameSettings::cameraSensitivity, scaling how tightly the camera follows.
    // MatchScene refreshes it every frame, so changing it in the pause menu is
    // felt as soon as the match resumes.
    float sensitivity = 1.0f;

    // --- Chase mode
    float distance = 9.5f;
    float height = 3.4f;
    float lookHeight = 1.2f;
    float velocityBlend = 0.35f; // how much the travel direction steers the camera

    // --- Ball cam: further back and higher, because it has to frame two things.
    float ballCamDistance = 11.5f;
    float ballCamHeight = 4.4f;
    float ballCamLookBlend = 0.30f; // how far the look point slides from car to ball
    float ballCamNearRange = 7.0f;  // inside this the ball stops steering the camera

    float positionSmoothing = 7.0f; // higher follows tighter
    float targetSmoothing = 12.0f;

    // --- Staying out of the geometry
    float minHeight = 0.6f;
    // Below the ceiling, so the camera never ends up on the wrong side of it.
    // MatchScene sets this from the arena; the default matches a 15 m one.
    float maxHeight = 14.2f;
    float wallMargin = 0.6f;  // clearance kept in front of whatever the view hits
    // Height the camera gains as it loses the room to sit back, so that being up
    // against a ramp turns the view overhead instead of into the car's boot.
    float blockedLift = 4.5f;
    // Floor on the pull-in. It is the last resort, and it is the one clamp that
    // can put the camera in a wall — the lift above is what normally keeps the
    // pull-in from ever getting this short.
    float minDistance = 1.5f;

    Vector3 position = {};
    Vector3 target = {};
};

#endif

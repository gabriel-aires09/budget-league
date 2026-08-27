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
    // A punch on the view, 0 to 1. The strongest pending one wins rather than
    // accumulating, so a goal during a scramble cannot stack into nausea.
    void Shake(float strength);
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
    // How fast the ball-cam side may swing round the car, per second. It exists
    // because that direction can genuinely reverse — the moment the car overtakes
    // the ball, the ball is behind it and the view belongs on the other side —
    // and a reversal is the one thing smoothing the camera's *position* cannot do
    // gracefully: a straight line between the two sides runs through the car.
    // 4.0 measured best of 1.5 / 2.5 / 4 / 6 / 9: the car stays on screen for
    // every frame of every routine, and only 9 was fast enough to start throwing
    // the view again (36 frames of the car lost during an overtake).
    float ballCamTurnRate = 4.0f;

    // Slower than the position, so a bump that flicks the surface normal cannot
    // throw the framing; the wall handover takes about half a second.
    float surfaceUpSmoothing = 3.0f;

    float positionSmoothing = 7.0f; // higher follows tighter
    float targetSmoothing = 12.0f;

    // --- Staying out of the geometry
    float minHeight = 0.6f;
    // Below the ceiling, so the camera never ends up on the wrong side of it.
    // MatchScene sets this from the arena; the default matches a 15 m one.
    float maxHeight = 14.2f;
    float wallMargin = 0.6f;  // clearance kept in front of whatever the view hits
    // Shortest flattened car forward still treated as a usable direction. Below
    // it the last good one is kept: a car on a wall or the ceiling points nearly
    // straight up, and the scrap left after flattening that is pure noise.
    float flatForwardMinimum = 0.25f;
    // Height the camera gains as it loses the room to sit back, so that being up
    // against a ramp turns the view overhead instead of into the car's boot.
    float blockedLift = 4.5f;
    // Floor on how far the view may be pulled in when the geometry crowds it.
    // It applies to the *desired* position only: the clamp that actually keeps
    // the eye out of surfaces has no floor at all, because one there is not a
    // floor but permission to sit on the far side of a wall.
    float minDistance = 1.5f;

    // --- Screen punch
    // The shake rotates the aim and never moves the eye, which is deliberate:
    // the eye is what Milestone 6.2's clipping guarantees are about, so a punch
    // cannot put the camera through a wall however hard it is hit.
    float shakeAngle = 2.4f;   // degrees of aim wobble at full strength
    float shakeDecay = 3.4f;   // strength lost per second
    float shakeStrength = 0.0f;
    float shakeTime = 0.0f;

    Vector3 position = {};
    Vector3 target = {};
    // Last usable follow direction, kept so a car that is not on the floor has
    // something stable to trail behind. See FollowDirection in the .cpp.
    Vector3 followDirection = { 0.0f, 0.0f, -1.0f };
    // Smoothed copy of the car's standing surface, which is what the camera lifts
    // along. Exactly world up on the flat floor, so nothing there changes.
    Vector3 surfaceUp = { 0.0f, 1.0f, 0.0f };
    // The side ball cam is currently sitting on, and whether it was in use last
    // frame. Chase mode never touches either, so it is unaffected by all of this.
    Vector3 ballCamDirection = { 0.0f, 0.0f, -1.0f };
    bool ballCamActive = false;
};

#endif

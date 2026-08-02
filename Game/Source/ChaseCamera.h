#ifndef CHASECAMERA_H
#define CHASECAMERA_H

#include <raylib.h>

class CarObject;

// Third person camera that trails the car, smoothing both its own position and
// the point it looks at. Ball cam arrives in Milestone 09.
class ChaseCamera
{
public:
    void Initialize(Camera3D &camera, const CarObject &car);
    void Update(Camera3D &camera, const CarObject &car, float deltaTime);

    float distance = 9.5f;
    float height = 3.4f;
    float lookHeight = 1.2f;
    float positionSmoothing = 7.0f; // higher follows tighter
    float targetSmoothing = 12.0f;
    float velocityBlend = 0.35f;    // how much the travel direction steers the camera

    Vector3 position = {};
    Vector3 target = {};
};

#endif

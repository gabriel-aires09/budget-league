#ifndef BALLOBJECT_H
#define BALLOBJECT_H

#include "GameObject.h"

// The soccer ball: one dynamic sphere, heavy enough to feel like it has mass but
// light enough that a full speed hit sends it flying.
//
// Every field below is a tunable, and most are on the F1 tuning panel
// (TuningPanel.cpp) in Debug and Development. Call ApplyTuning() after changing
// them at runtime.
class BallObject final : public GameObject
{
public:
    virtual ~BallObject();

    virtual void Initialize(Scene &owner) override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual BoundingBox GetWorldBounds() const override;

    void ApplyTuning();
    void ResetTo(Vector3 position);
    float GetSpeed() const;

    Vector3 spawnPosition = { 0.0f, 1.25f, 0.0f };

    // --- Shape and mass
    float radius = 1.25f;
    float mass = 45.0f;

    // --- Feel
    // The ball falls harder than the car so it arcs like an arcade ball instead
    // of floating. Per body, so Milestone 04's car handling is left alone.
    float gravityFactor = 1.7f;
    float restitution = 0.70f;
    float friction = 0.35f;
    float linearDamping = 0.15f;
    // Carries most of the rolling resistance. At the original 0.35 a 20 m/s roll
    // crossed 69 m of an 80 m field and took 16 s to stop.
    float angularDamping = 1.20f;
    // Keeps a boosted aerial hit from launching the ball out of the arena.
    float maxSpeed = 55.0f;

    Color ballColor = { 232, 238, 248, 255 };

    Model ballModel = {};
    bool modelLoaded = false;
};

#endif

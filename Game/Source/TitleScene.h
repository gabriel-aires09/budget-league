#ifndef TITLESCENE_H
#define TITLESCENE_H

#include "Scene.h"
#include "TextureAsset.h"
#include "GameObjects/ArenaObject.h"
#include "GameObjects/BallObject.h"
#include "GameObjects/GoalObject.h"

// The first screen at launch: the logo and a "press any button" prompt over a
// live view of the real arena with the ball resting on the field. Any key, mouse
// button or gamepad button goes to the main menu.
//
// The arena and the ball are the match's own objects, so the title can never
// show something the game does not look like. Physics is never stepped: both
// bodies exist only so the arena builds its pieces and the ball has a transform.
class TitleScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    ArenaObject arena;
    GoalObject blueGoal;
    GoalObject orangeGoal;
    BallObject ball;
    TextureAsset logo;

    // Seconds since the scene opened. Drives the camera drift, the pulse on the
    // prompt and the fade in, so there is one clock rather than three.
    float elapsed = 0.0f;
};

#endif

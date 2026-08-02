#ifndef MATCHSCENE_H
#define MATCHSCENE_H

#include "Scene.h"
#include "ChaseCamera.h"
#include "PlayerController.h"
#include "SettingsMenu.h"
#include "UserInterface.h"
#include "GameObjects/ArenaObject.h"
#include "GameObjects/BallObject.h"
#include "GameObjects/CarObject.h"

// The gameplay scene. Milestone 02: arena floor, player car and chase camera.
// Milestone 03: pause menu. Milestone 05: the ball.
class MatchScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    ArenaObject arena;
    BallObject ball;
    CarObject playerCar;
    PlayerController playerController;
    ChaseCamera chaseCamera;

    bool paused = false;
    bool settingsOpen = false;
    uistyle::MenuList pauseMenu;
    SettingsMenu settingsMenu;
};

#endif

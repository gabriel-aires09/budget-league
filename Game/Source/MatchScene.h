#ifndef MATCHSCENE_H
#define MATCHSCENE_H

#include "Scene.h"
#include "BotController.h"
#include "ChaseCamera.h"
#include "HUD.h"
#include "Match.h"
#include "PlayerController.h"
#include "SettingsMenu.h"
#include "UserInterface.h"
#include "GameObjects/ArenaObject.h"
#include "GameObjects/BallObject.h"
#include "GameObjects/BoostPadObject.h"
#include "GameObjects/CarObject.h"
#include "GameObjects/GoalObject.h"

#include <vector>

// The gameplay scene. Milestone 02: arena floor, player car and chase camera.
// Milestone 03: pause menu. Milestone 05: the ball. Milestone 06: the enclosed
// arena, goals, score and clock. Milestone 11 moved every readout into HUD.h.
class MatchScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    void BuildBoostPads();

    ArenaObject arena;
    BallObject ball;
    GoalObject blueGoal;
    GoalObject orangeGoal;
    CarObject playerCar;
    // Only built when GameSettings::botEnabled; without it the scene is the solo
    // practice fallback CLAUDE.md allows for.
    CarObject botCar;
    bool botActive = false;
    // Filled completely before any pointer is taken into Scene::objects, because
    // a later push_back would reallocate and leave those pointers dangling.
    std::vector<BoostPadObject> boostPads;
    Match match;
    PlayerController playerController;
    BotController botController;
    ChaseCamera chaseCamera;

    bool paused = false;
    bool settingsOpen = false;
    uistyle::MenuList pauseMenu;
    SettingsMenu settingsMenu;
};

#endif

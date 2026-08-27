#ifndef MAINMENUSCENE_H
#define MAINMENUSCENE_H

#include "Scene.h"
#include "SettingsMenu.h"
#include "StaticModelAsset.h"
#include "UserInterface.h"
#include "GameObjects/ArenaObject.h"
#include "GameObjects/GoalObject.h"

// The main menu, over a live showcase: the real arena with one car parked on the
// field turntabling slowly, and the menu list down the left (Milestone 18).
//
// The showcase car is a StaticModelAsset drawn directly rather than a CarObject,
// because a showroom prop needs no body, no controller and no simulation.
// Physics exists only because ArenaObject builds a body, and is never stepped.
class MainMenuScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    ArenaObject arena;
    GoalObject blueGoal;
    GoalObject orangeGoal;

    // Re-rolled on every Initialize, which is every time the menu is entered.
    StaticModelAsset showcaseCar;
    float showcaseScale = 1.0f;
    Color showcaseColor = {};
    float spinDegrees = 0.0f;

    uistyle::MenuList menu;
    SettingsMenu settingsMenu;
    bool settingsOpen = false;
};

#endif

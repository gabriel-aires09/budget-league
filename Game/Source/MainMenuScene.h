#ifndef MAINMENUSCENE_H
#define MAINMENUSCENE_H

#include "Scene.h"
#include "SettingsMenu.h"
#include "UserInterface.h"

class MainMenuScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    uistyle::MenuList menu;
    SettingsMenu settingsMenu;
    bool settingsOpen = false;
};

#endif
